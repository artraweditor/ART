/* -*- C++ -*-
 *
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2018 Alberto Griggio <alberto.griggio@gmail.com>
 *
 *  RawTherapee is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RawTherapee is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RawTherapee.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../rtgui/mydiagonalcurve.h"
#include "array2D.h"
#include "color.h"
#include "curves.h"
#include "iccstore.h"
#include "improcfun.h"
#include "rawimagesource.h"
#include "rt_math.h"
#include "rtthumbnail.h"
// #define BENCHMARK
// #include "StopWatch.h"
#include <iostream>

namespace rtengine {

extern const Settings *settings;

namespace {

struct CdfInfo {
    std::vector<int> cdf;
    int min_val;
    int max_val;

    CdfInfo(): cdf(256), min_val(-1), max_val(-1) {}
};

typedef int (*PixelGetter)(const IImage8 &img, int y, int x);

int get_luminance(const IImage8 &img, int y, int x)
{
    return LIM(int(Color::rgbLuminance(float(img.r(y, x)), float(img.g(y, x)),
                                       float(img.b(y, x)))),
               0, 255);
}

int get_r(const IImage8 &img, int y, int x) { return img.r(y, x); }

int get_g(const IImage8 &img, int y, int x) { return img.g(y, x); }

int get_b(const IImage8 &img, int y, int x) { return img.b(y, x); }

CdfInfo getCdf(const IImage8 &img, PixelGetter getpix, float expcomp = 0)
{
    CdfInfo ret;
    const float factor = std::pow(2.f, expcomp);

    for (int y = 0; y < img.getHeight(); ++y) {
        for (int x = 0; x < img.getWidth(); ++x) {
            int lum = LIM(int(getpix(img, y, x) * factor), 0, 255);
            ++ret.cdf[lum];
        }
    }

    int sum = 0;
    for (size_t i = 0; i < ret.cdf.size(); ++i) {
        if (ret.cdf[i] > 0) {
            if (ret.min_val < 0) {
                ret.min_val = i;
            }
            ret.max_val = i;
        }
        sum += ret.cdf[i];
        ret.cdf[i] = sum;
    }

    return ret;
}

int findMatch(int val, const std::vector<int> &cdf, int j)
{
    if (cdf[j] <= val) {
        for (; j < int(cdf.size()); ++j) {
            if (cdf[j] == val) {
                return j;
            } else if (cdf[j] > val) {
                return (cdf[j] - val <= val - cdf[j - 1] ? j : j - 1);
            }
        }
        return 255;
    } else {
        for (; j >= 0; --j) {
            if (cdf[j] == val) {
                return j;
            } else if (cdf[j] < val) {
                return (val - cdf[j] <= cdf[j + 1] - val ? j : j + 1);
            }
        }
        return 0;
    }
}

void ensure_not_clipping(std::vector<double> &curve)
{
    DiagonalCurve c(curve);
    double pivot = curve[5];
    double start = pivot / 2;
    while (start >= 0.01) {
        double mid = start / 2;
        double y = c.getVal(mid);
        if (y <= 0) {
            curve[4] += (curve[3] - curve[4]) / 2;
            if (settings->verbose > 1) {
                std::cout << "histogram matching: bumping up (" << curve[3]
                          << ", " << c.getVal(curve[3]) << ") to " << curve[4]
                          << " to avoid negative clipping at " << mid
                          << std::endl;
            }
            ensure_not_clipping(curve);
            return;
        } else {
            start = mid;
        }
    }
    start = pivot + (1.0 - pivot) / 2.0;
    while (start <= 0.9) {
        double mid = start + (1 - start) / 2;
        double y = c.getVal(mid);
        if (y >= 1) {
            curve[8] += (curve[7] - curve[8]) * 0.1;
            if (settings->verbose > 1) {
                std::cout << "histogram matching: bumping down (" << curve[7]
                          << ", " << c.getVal(curve[7]) << ") to " << curve[8]
                          << " to avoid positive clipping at " << mid
                          << std::endl;
            }
            ensure_not_clipping(curve);
            return;
        } else {
            start = mid;
        }
    }
}

void mappingToCurve(const std::vector<int> &mapping, std::vector<double> &curve)
{
    curve.clear();

    int idx = 15;
    for (; idx < int(mapping.size()); ++idx) {
        if (mapping[idx] >= idx) {
            break;
        }
    }
    if (idx == int(mapping.size())) {
        for (idx = 1; idx < int(mapping.size()) - 1; ++idx) {
            if (mapping[idx] >= idx) {
                break;
            }
        }
    }

    auto coord = [](int v) -> double { return double(v) / 255.0; };
    auto doit = [&](int start, int stop, int step, bool addstart,
                    int maxdelta = 0) -> void {
        if (!maxdelta)
            maxdelta = step * 2;
        int prev = start;
        if (addstart && mapping[start] >= 0) {
            curve.push_back(coord(start));
            curve.push_back(coord(mapping[start]));
        }
        for (int i = start; i < stop; ++i) {
            int v = mapping[i];
            if (v < 0) {
                continue;
            }
            bool change = i > 0 && v != mapping[i - 1];
            int diff = i - prev;
            if ((change && std::abs(diff - step) <= 1) || diff > maxdelta) {
                curve.push_back(coord(i));
                curve.push_back(coord(v));
                prev = i;
            }
        }
    };

    curve.push_back(0.0);
    curve.push_back(0.0);

    int start = 0;
    while (start < idx && (mapping[start] < 0 || start < idx / 2)) {
        ++start;
    }

    const int npoints = 8;
    int step = std::max(int(mapping.size()) / npoints, 1);
    int end = mapping.size();
    if (idx <= end / 3) {
        doit(start, idx, idx / 2, true);
        step = (end - idx) / 4;
        doit(idx, end, step, false, step);
    } else {
        doit(start, idx, idx > step ? step : idx / 2, true);
        doit(idx, end, step,
             idx - step > step / 2 &&
                 std::abs(curve[curve.size() - 2] - coord(idx)) > 0.01);
    }

    if (curve.size() > 2 && (1 - curve[curve.size() - 2] <= coord(step) / 3)) {
        curve.pop_back();
        curve.pop_back();
    }

    curve.push_back(1.0);
    curve.push_back(1.0);

    // we assume we are matching an S-shaped curve, so try to avoid
    // concavities in the upper part of the S
    const auto getpos = [](float x, float xa, float ya, float xb, float yb) {
        // line equation:
        // (x - xa) / (xb - xa) = (y - ya) / (yb - ya)
        return (x - xa) / (xb - xa) * (yb - ya) + ya;
    };
    idx = -1;
    for (ssize_t i = curve.size() - 1; i > 0; i -= 2) {
        if (curve[i] <= 0.f) {
            idx = i + 1;
            break;
        }
    }
    if (idx >= 0 && size_t(idx) < curve.size()) {
        // idx is the position of the first point in the upper part of the S
        // for each 3 consecutive points (xa, ya), (x, y), (xb, yb) we check
        // that y is above the point at x of the line between the other two
        // if this is not the case, we remove (x, y) from the curve
        while (size_t(idx + 5) < curve.size()) {
            float xa = curve[idx];
            float ya = curve[idx + 1];
            float x = curve[idx + 2];
            float y = curve[idx + 3];
            float xb = curve[idx + 4];
            float yb = curve[idx + 5];
            float yy = getpos(x, xa, ya, xb, yb);
            if (yy > y) {
                // we have to remove (x, y) from the curve
                curve.erase(curve.begin() + (idx + 2),
                            curve.begin() + (idx + 4));
            } else {
                // move on to the next point
                idx += 2;
            }
        }
    }

    if (curve.size() < 4) {
        curve = {DCT_Linear}; // not enough points, fall back to linear
    } else {
        curve.insert(curve.begin(), DCT_Spline);
        DiagonalCurve c(curve);
        curve = {DCT_Spline /*DCT_CatmullRom*/};
        double pivot = -1.0;
        for (int i = 25; i < 256; ++i) {
            double xx = double(i) / 255.0;
            if (c.getVal(xx) > xx) {
                pivot = xx;
                break;
            }
        }
        if (pivot > 0) {
            curve.push_back(0.0);
            curve.push_back(c.getVal(0.0));
            curve.push_back(pivot / 2.0);
            curve.push_back(c.getVal(pivot / 2.0));
            curve.push_back(pivot);
            curve.push_back(c.getVal(pivot));
            curve.push_back(pivot + (1.0 - pivot) / 2.0);
            curve.push_back(c.getVal(pivot + (1.0 - pivot) / 2.0));
            curve.push_back(1.0);
            curve.push_back(c.getVal(1.0));
            ensure_not_clipping(curve);
        } else {
            double x = 0.0;
            double gap = 0.05;
            while (x < 1.0) {
                curve.push_back(x);
                curve.push_back(c.getVal(x));
                x += gap;
                gap *= 1.4;
            }
            curve.push_back(1.0);
            curve.push_back(c.getVal(1.0));
        }
    }
}

class CurveEvaluator {
public:
    CurveEvaluator(const IImage8 &source, const IImage8 &target): srchist_{}
    {
        int sw = source.getWidth();
        int sh = source.getHeight();
        float s = 300 / float(std::max(sw, sh));
        int w = sw * s;
        int h = sh * s;
        img_(w, h);

        for (int y = 0; y < h; ++y) {
            int sy = y / s;
            for (int x = 0; x < w; ++x) {
                int sx = x / s;
                int l = get_luminance(source, sy, sx);
                img_[y][x] = float(get_luminance(target, sy, sx)) / 255.f;
                ++srchist_[l];
            }
        }
    }

    double operator()(const std::vector<double> &curve)
    {
        std::array<float, 256> hist = {};
        DiagonalCurve c(curve);

        for (int y = 0; y < img_.height(); ++y) {
            for (int x = 0; x < img_.width(); ++x) {
                int l = LIM01(c.getVal(img_[y][x])) * 255.f;
                ++hist[l];
            }
        }

        size_t ret = 0;
        for (size_t i = 0; i < hist.size(); ++i) {
            ret += std::abs(srchist_[i] - hist[i]);
        }
        return ret * (is_scurve(curve) ? 0.1 : 1);
    }

private:
    bool is_scurve(const std::vector<double> &curve) const
    {
        int shoulder = -1;
        float prev = 0.f;
        for (size_t i = 1; i < curve.size(); i += 2) {
            if (shoulder < 0) {
                if (curve[i] >= curve[i + 1] && curve[i] > 0) {
                    shoulder = 1;
                } else if (curve[i] > 0) {
                    return false;
                }
            } else if (shoulder == 1) {
                if (curve[i] < curve[i + 1]) {
                    shoulder = 0;
                }
            } else {
                if (curve[i] >= curve[i + 1] && curve[i] < 1) {
                    return false;
                } else if (curve[i + 1] < prev) {
                    return false;
                }
                prev = curve[i + 1];
            }
        }
        return shoulder >= 0;
    }

    std::array<float, 256> srchist_;
    array2D<float> img_;
};

int avg_luminance(const IImage8 &img, int starty, int startx, int tilesize)
{
    int ret = 0;
    for (int j = 0; j < tilesize; ++j) {
        for (int i = 0; i < tilesize; ++i) {
            int l = get_luminance(img, starty + j, startx + i);
            ret += l;
        }
    }
    return float(ret) / float(SQR(tilesize));
}

int max_corner_luminance(const IImage8 &img)
{
    int w = img.getWidth();
    int h = img.getHeight();
    int l_tl = avg_luminance(img, 0, 0, 4);
    int l_tr = avg_luminance(img, 0, w - 1 - 4, 4);
    int l_bl = avg_luminance(img, h - 1 - 4, 0, 4);
    int l_br = avg_luminance(img, h - 1 - 4, w - 1 - 4, 4);
    return max(l_tl, l_tr, l_bl, l_br);
}

double get_expcomp(const FramesMetaData *md)
{
    if (md->getMake() == "FUJIFILM") {
        auto mn = Exiv2Metadata(md->getFileName()).getMakernotes();
        auto it = mn.find("RawExposureBias");
        if (it != mn.end()) {
            double e = -std::atof(it->second.c_str());
            if (e > 1) {
                return std::log(e) / std::log(2.4);
            } else if (e > 0) {
                return e / 2.4;
            }
        }
    }
    return 0.0;
}

} // namespace

void RawImageSource::getAutoMatchedToneCurve(const ColorManagementParams &cp,
                                             std::vector<double> &outCurve,
                                             std::vector<double> &outCurve2)
{
    //    BENCHFUN

    if (settings->verbose) {
        std::cout << "performing histogram matching for " << getFileName()
                  << " on the embedded thumbnail" << std::endl;
    }

    const auto same_profile = [](const ColorManagementParams &a,
                                 const ColorManagementParams &b) -> bool {
        return (
            a.inputProfile == b.inputProfile && a.toneCurve == b.toneCurve &&
            a.applyLookTable == b.applyLookTable &&
            a.applyBaselineExposureOffset == b.applyBaselineExposureOffset &&
            a.applyHueSatMap == b.applyHueSatMap &&
            a.dcpIlluminant == b.dcpIlluminant);
    };

    if (!histMatchingCache.empty() && same_profile(histMatchingParams, cp)) {
        if (settings->verbose) {
            std::cout << "tone curve found in cache" << std::endl;
        }
        outCurve = histMatchingCache;
        outCurve2 = histMatchingCache2;
        return;
    }

    outCurve = {DCT_Linear};
    outCurve2 = {DCT_Linear};

    int fw, fh;
    getFullSize(fw, fh, TR_NONE);
    if (getRotateDegree() == 90 || getRotateDegree() == 270) {
        std::swap(fw, fh);
    }
    int skip = 3;

    if (settings->verbose > 1) {
        std::cout << "histogram matching: full raw image size is " << fw << "x"
                  << fh << std::endl;
    }

    ProcParams neutral;
    neutral.icm = cp;
    neutral.raw.bayersensor.method = RAWParams::BayerSensor::Method::FAST;
    neutral.raw.xtranssensor.method = RAWParams::XTransSensor::Method::FAST;
    neutral.icm.outputProfile = ColorManagementParams::NoICMString;

    std::unique_ptr<IImage8> source;
    {
        eSensorType sensor_type;
        int w = -1, h = -1;
        std::unique_ptr<Thumbnail> thumb(Thumbnail::loadQuickFromRaw(
            getFileName(), sensor_type, w, h, 1, false, true));
        if (!thumb) {
            if (settings->verbose) {
                std::cout << "histogram matching: no thumbnail found, "
                             "generating a neutral curve"
                          << std::endl;
            }
            histMatchingCache = outCurve;
            histMatchingCache2 = outCurve2;
            histMatchingParams = cp;
            return;
        } else if (w * 10 < fw) {
            if (settings->verbose) {
                std::cout << "histogram matching: the embedded thumbnail is "
                             "too small: "
                          << w << "x" << h << std::endl;
            }
            histMatchingCache = outCurve;
            histMatchingCache2 = outCurve2;
            histMatchingParams = cp;
            return;
        }
        skip = LIM(skip * fh / h, 6,
                   10); // adjust the skip factor -- the larger the thumbnail,
                        // the less we should skip to get a good match
        source.reset(thumb->quickProcessImage(neutral, fh / skip, TI_Nearest));

        if (settings->verbose > 1) {
            std::cout << "histogram matching: extracted embedded thumbnail"
                      << std::endl;
        }
    }

    std::unique_ptr<IImage8> target;
    {
        eSensorType sensor_type;
        double scale;
        int w = fw / skip, h = fh / skip;
        std::unique_ptr<Thumbnail> thumb(Thumbnail::loadFromRaw(
            getFileName(), sensor_type, w, h, 1, false, false, true));
        if (!thumb) {
            if (settings->verbose) {
                std::cout << "histogram matching: raw decoding failed, "
                             "generating a neutral curve"
                          << std::endl;
            }
            histMatchingCache = outCurve;
            histMatchingCache2 = outCurve2;
            histMatchingParams = cp;
            return;
        }
        target.reset(thumb->processImage(neutral, sensor_type, fh / skip,
                                         TI_Nearest, getMetaData(), scale,
                                         false, true));

        int sw = source->getWidth(), sh = source->getHeight();
        int tw = target->getWidth(), th = target->getHeight();

        // check if we need auto-distortion correction -- try to see if we
        // have dark corners
        int tl = max_corner_luminance(*target);
        int sl = max_corner_luminance(*source);
        const int l_noise = 10;
        if ((tl <= l_noise && sl > l_noise) || (tl > 0 && sl > l_noise && sl / tl > 2)) {
            if (settings->verbose > 1) {
                std::cout << "histogram matching: corners luminance is " << tl
                          << " for target, " << sl << " for source, "
                          << "trying to perform auto-distortion correction"
                          << std::endl;
            }
            neutral.distortion.enabled = true;
            neutral.distortion.amount =
                ImProcFunctions::getAutoDistor(getFileName(), 300);
            if (neutral.distortion.amount != 0) {
                if (settings->verbose > 1) {
                    std::cout << "histogram matching: dark corners detected, "
                                 "reprocessing with auto-distortion correction"
                              << std::endl;
                }
                target.reset(thumb->processImage(
                    neutral, sensor_type, fh / skip, TI_Nearest, getMetaData(),
                    scale, false, true));
            }
        }

        float thumb_ratio = float(std::max(sw, sh)) / float(std::min(sw, sh));
        float target_ratio = float(std::max(tw, th)) / float(std::min(tw, th));
        int cx = 0, cy = 0;
        if (std::abs(thumb_ratio - target_ratio) > 0.01) {
            if (thumb_ratio > target_ratio) {
                // crop the height
                int ch = th - (tw * float(sh) / float(sw));
                cy += ch / 2;
                th -= ch;
            } else {
                // crop the width
                int cw = tw - (th * float(sw) / float(sh));
                cx += cw / 2;
                tw -= cw;
            }
            if (settings->verbose > 1) {
                std::cout << "histogram matching: cropping target to get an "
                             "aspect ratio of "
                          << round(thumb_ratio * 100) / 100.0
                          << ":1, new size is " << tw << "x" << th << std::endl;
            }

            if (cx || cy) {
                Image8 *tmp = new Image8(tw, th);
#ifdef _OPENMP
#pragma omp parallel for
#endif
                for (int y = 0; y < th; ++y) {
                    for (int x = 0; x < tw; ++x) {
                        tmp->r(y, x) = target->r(y + cy, x + cx);
                        tmp->g(y, x) = target->g(y + cy, x + cx);
                        tmp->b(y, x) = target->b(y + cy, x + cx);
                    }
                }
                target.reset(tmp);
            }
        }

        if (settings->verbose > 1) {
            std::cout << "histogram matching: generated neutral rendering"
                      << std::endl;
        }
    }
    if (target->getWidth() != source->getWidth() ||
        target->getHeight() != source->getHeight()) {
        Image8 *tmp = new Image8(source->getWidth(), source->getHeight());
        target->resizeImgTo(source->getWidth(), source->getHeight(), TI_Nearest,
                            tmp);
        target.reset(tmp);
    }

    static const std::vector<PixelGetter> getters = {&get_luminance, &get_r,
                                                     &get_g, &get_b};
    std::vector<std::vector<double>> candidates;
    double expcomp = get_expcomp(getMetaData());
    for (auto g : getters) {
        CdfInfo scdf = getCdf(*source, g);
        CdfInfo tcdf = getCdf(*target, g, expcomp);

        std::vector<int> mapping;
        int j = 0;
        for (int i = 0; i < int(tcdf.cdf.size()); ++i) {
            j = findMatch(tcdf.cdf[i], scdf.cdf, j);
            if (i >= tcdf.min_val && i <= tcdf.max_val && j >= scdf.min_val &&
                j <= scdf.max_val) {
                mapping.push_back(j);
            } else {
                mapping.push_back(-1);
            }
        }

        candidates.push_back({});
        mappingToCurve(mapping, candidates.back());
    }
    CurveEvaluator eval(*source, *target);
    size_t best = candidates.size();
    double bestscore = RT_INFINITY;
    for (size_t i = 0; i < candidates.size(); ++i) {
        double score = eval(candidates[i]);
        if (i == 0) {
            score *= 0.9; // give more weight to the luminance curve
        }
        if (settings->verbose > 1) {
            std::cout << "histogram matching: candidate " << i << " has score "
                      << score << std::endl;
        }
        if (score < bestscore) {
            best = i;
            bestscore = score;
        }
    }
    outCurve = candidates[best];
    if (expcomp > 0.f) {
        double x = 0.3;
        double y = x * std::pow(2.0, expcomp);
        outCurve2 = {DCT_CatmullRom, 0.0, 0.0, x, y, 1.0, 1.0};
        if (outCurve.size() > 5 && outCurve[4] > outCurve[3]) {
            outCurve = outCurve2;
            outCurve2 = {DCT_Linear};
        }
    }

    if (settings->verbose) {
        std::cout << "histogram matching: best match found at " << best
                  << " with score " << bestscore << std::endl;
        std::cout << "histogram matching: generated curve with "
                  << outCurve.size() / 2 << " control points" << std::endl;
    }

    histMatchingCache = outCurve;
    histMatchingCache2 = outCurve2;
    histMatchingParams = cp;
}

} // namespace rtengine
