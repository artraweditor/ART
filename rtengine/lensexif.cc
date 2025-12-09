/* -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright (c) 2020 Alberto Griggio <alberto.griggio@gmail.com>
 *
 *  ART is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  ART is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with ART.  If not, see <http://www.gnu.org/licenses/>.
 */

// adapted from mlens.c in PR #7092 of darktable
// original author: Freddie Witherden (https://freddie.witherden.org/)
// copyright of original code follows
/*
    This file is part of darktable,
    Copyright (C) 2010-2020 darktable developers.

    darktable is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    darktable is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with darktable.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "lensexif.h"
#include "metadata.h"
#include "settings.h"
#include <array>
#include <iostream>

namespace rtengine {

extern const Settings *settings;

namespace {

class SonyCorrectionData: public ExifLensCorrection::CorrectionData {
public:
    int nc;
    std::array<short, 16> distortion;
    std::array<short, 16> ca_r;
    std::array<short, 16> ca_b;
    std::array<short, 16> vignetting;

    void get_coeffs(std::vector<float> &knots, std::vector<float> &dist,
                    std::vector<float> &vig,
                    std::array<std::vector<float>, 3> &ca,
                    bool &is_dng) const override
    {
        is_dng = false;
        const float scale = 1.f;

        knots.resize(nc);
        for (int i = 0; i < 3; ++i) {
            ca[i].resize(nc);
        }
        dist.resize(nc);
        vig.resize(nc);

        constexpr float vig_scaling =
            0.7f; // empirically determined on my lenses

        for (int i = 0; i < nc; i++) {
            knots[i] = float(i) / (nc - 1);

            dist[i] = (distortion[i] * std::pow(2.f, -14.f) + 1) * scale;

            ca[0][i] = ca[1][i] = ca[2][i] = 1.f;
            ca[0][i] *= ca_r[i] * std::pow(2.f, -21.f) + 1;
            ca[2][i] *= ca_b[i] * std::pow(2.f, -21.f) + 1;

            vig[i] = std::pow(
                2.f, 0.5f - std::pow(2.f, vig_scaling * vignetting[i] *
                                                  std::pow(2.f, -13.f) -
                                              1));
        }
    }

    bool has_dist() const override { return true; }
    bool has_vign() const override { return true; }
    bool has_ca() const override { return true; }
};

class FujiCorrectionData: public ExifLensCorrection::CorrectionData {
public:
    float cropf;
    std::array<float, 9> knots;
    std::array<float, 9> distortion;
    std::array<float, 9> ca_r;
    std::array<float, 9> ca_b;
    std::array<float, 9> vignetting;

    void get_coeffs(std::vector<float> &knots, std::vector<float> &dist,
                    std::vector<float> &vig,
                    std::array<std::vector<float>, 3> &ca,
                    bool &is_dng) const override
    {
        is_dng = false;
        knots.resize(9);
        dist.resize(9);
        vig.resize(9);
        for (int i = 0; i < 3; ++i) {
            ca[i].resize(9);
        }

        for (int i = 0; i < 9; i++) {
            knots[i] = cropf * this->knots[i];

            dist[i] = distortion[i] / 100 + 1;

            ca[0][i] = ca[1][i] = ca[2][i] = 1.f;

            ca[0][i] *= ca_r[i] + 1;
            ca[2][i] *= ca_b[i] + 1;

            vig[i] = 1 - (1 - vignetting[i] / 100);
        }
    }

    bool has_dist() const override { return true; }
    bool has_vign() const override { return true; }
    bool has_ca() const override { return true; }
};

class DNGCorrectionData: public ExifLensCorrection::CorrectionData {
public:
    float cx_d;
    float cy_d;
    float cx_v;
    float cy_v;
    std::vector<float> warp_rectilinear;
    std::vector<float> vignette_radial;

    DNGCorrectionData()
        : cx_d(0), cy_d(0), cx_v(0), cy_v(0), warp_rectilinear(),
          vignette_radial()
    {
    }

    void get_coeffs(std::vector<float> &knots, std::vector<float> &dist,
                    std::vector<float> &vig,
                    std::array<std::vector<float>, 3> &ca,
                    bool &is_dng) const override
    {
        is_dng = true;
        knots = {cx_d, cy_d, cx_v, cy_v};
        dist = warp_rectilinear;
        vig = vignette_radial;
    }

    static DNGCorrectionData *parse(const std::vector<Exiv2::byte> &buf)
    {
        DNGCorrectionData ret;
        const Exiv2::byte *data = &buf[0];
        uint32_t num_entries = Exiv2::getULong(data, Exiv2::bigEndian);
        size_t idx = 4;
        bool ok = false;

        for (size_t i = 0; i < num_entries && idx < buf.size(); ++i) {
            uint32_t opid = Exiv2::getULong(data + idx, Exiv2::bigEndian);
            idx += 4;
            idx += 4; // version
            idx += 4; // flags
            size_t size = Exiv2::getULong(data + idx, Exiv2::bigEndian);
            idx += 4;
            if (opid == 1) { // WarpRectilinear
                uint32_t n = Exiv2::getULong(data + idx, Exiv2::bigEndian);
                size_t wstart = idx + 4;
                size_t cstart = wstart + 6 * 8;
                if (n == 3) {
                    wstart += 6 * 8;
                    cstart += 6 * 8 * 2;
                } else if (n != 1) {
                    cstart = buf.size() + 1;
                }
                if (cstart + 8 * 2 <= buf.size()) {
                    ret.warp_rectilinear.resize(6);
                    for (int j = 0; j < 6; ++j) {
                        ret.warp_rectilinear[j] =
                            Exiv2::getDouble(data + wstart, Exiv2::bigEndian);
                        wstart += 8;
                    }
                    ret.cx_d =
                        Exiv2::getDouble(data + cstart, Exiv2::bigEndian);
                    cstart += 8;
                    ret.cy_d =
                        Exiv2::getDouble(data + cstart, Exiv2::bigEndian);
                    ok = true;
                }
                if (settings->verbose) {
                    std::cout << "WARP RECTILINEAR: ";
                    for (auto w : ret.warp_rectilinear) {
                        std::cout << w << " ";
                    }
                    std::cout << "| " << ret.cx_d << " " << ret.cy_d
                              << std::endl;
                }
            } else if (opid == 3) { // FixVignetteRadial
                size_t start = idx;
                size_t end = idx + 7 * 8;
                if (end <= buf.size()) {
                    ret.vignette_radial.resize(6);
                    for (int j = 0; j < 5; ++j) {
                        ret.vignette_radial[j] =
                            Exiv2::getDouble(data + start, Exiv2::bigEndian);
                        start += 8;
                    }
                    ret.cx_v = Exiv2::getDouble(data + start, Exiv2::bigEndian);
                    start += 8;
                    ret.cy_v = Exiv2::getDouble(data + start, Exiv2::bigEndian);
                    ok = true;
                }
            }
            idx += size;
        }

        if (!ok) {
            return nullptr;
        } else {
            return new DNGCorrectionData(ret);
        }
    }

    bool has_dist() const override { return !warp_rectilinear.empty(); }
    bool has_vign() const override { return !vignette_radial.empty(); }
    bool has_ca() const override { return false; }
};

// Olympus correction data adapted from src/iop/lens.cc in darktable 4.6
// copyright of original code follows
/*
    This file is part of darktable,
    Copyright (C) 2019-2023 darktable developers.

    darktable is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    darktable is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with darktable.  If not, see <http://www.gnu.org/licenses/>.
*/
class OlympusCorrectionData: public ExifLensCorrection::CorrectionData {
public:
    std::array<float, 4> distortion;
    std::array<float, 6> cacorr;
    bool ca_ok;

    void get_coeffs(std::vector<float> &knots, std::vector<float> &dist,
                    std::vector<float> &vig,
                    std::array<std::vector<float>, 3> &ca,
                    bool &is_dng) const override
    {
        is_dng = false;
        constexpr int nc = 16;

        float drs = distortion[3];
        float dk2 = distortion[0];
        float dk4 = distortion[1];
        float dk6 = distortion[2];

        float car0 = cacorr[0];
        float car2 = cacorr[1];
        float car4 = cacorr[2];
        float cab0 = cacorr[3];
        float cab2 = cacorr[4];
        float cab4 = cacorr[5];

        knots.resize(nc);
        for (int i = 0; i < 3; ++i) {
            ca[i].resize(nc);
        }
        dist.resize(nc);

        for (int i = 0; i < nc; i++) {
            const float r = float(i) / (nc - 1);
            knots[i] = r;

            // Convert the polynomial to a spline by evaluating it at each knot
            //
            // The distortion polynomial maps a radius Rout in the output
            // (undistorted) image, where the corner is defined as Rout=1, to a
            // radius in the input (distorted) image, where the corner is
            // defined as Rin=1. Rin = Rout*dk0 * (1 + dk2 * (Rout*dk0)^2 + dk4
            // * (Rout*dk0)^4 + dk6 * (Rout*dk0)^6)
            //
            // r_cor is Rin / Rout.
            const float rs2 = powf(r * drs, 2);
            const float r_cor =
                drs * (1 + rs2 * (dk2 + rs2 * (dk4 + rs2 * dk6)));
            dist[i] = r_cor;

            const float rd = r;
            const float rd2 = powf(rd, 2);

            ca[0][i] = ca[1][i] = ca[2][i] = 1.f;
            if (r > 0) {
                ca[0][i] += rd * (car0 + rd2 * (car2 + rd2 * car4)) / r;
                ca[2][i] += rd * (cab0 + rd2 * (cab2 + rd2 * cab4)) / r;
            }
        }
    }

    bool has_dist() const override
    {
        return distortion[0] || distortion[1] || distortion[2];
    }
    bool has_vign() const override { return false; }
    bool has_ca() const override { return ca_ok; }
};

float interpolate(const std::vector<float> &xi, const std::vector<float> &yi,
                  float x)
{
    if (x < xi[0]) {
        return yi[0];
    }

    for (size_t i = 1; i < xi.size(); i++) {
        if (x >= xi[i - 1] && x <= xi[i]) {
            float dydx = (yi[i] - yi[i - 1]) / (xi[i] - xi[i - 1]);

            return yi[i - 1] + (x - xi[i - 1]) * dydx;
        }
    }

    return yi[yi.size() - 1];
}

} // namespace

ExifLensCorrection::ExifLensCorrection(const FramesMetaData *meta, int width,
                                       int height,
                                       const CoarseTransformParams &coarse,
                                       int rawRotationDeg)
    : data_(), is_dng_(false), swap_xy_(false)
{
    if (rawRotationDeg >= 0) {
        int rot = (coarse.rotate + rawRotationDeg) % 360;
        swap_xy_ = (rot == 90 || rot == 270);
        if (swap_xy_) {
            std::swap(width, height);
        }
    }

    w2_ = width * 0.5f;
    h2_ = height * 0.5f;
    r_ = 1 / std::sqrt(SQR(w2_) + SQR(h2_));

    std::string make = Glib::ustring(meta->getMake()).uppercase();
    static const std::unordered_set<std::string> makers = {
        "SONY", "FUJIFILM", "OLYMPUS", "OM DIGITAL SOLUTIONS"};
    if (makers.find(make) == makers.end() && !meta->isDNG()) {
        return;
    }

    try {
        auto md = Exiv2Metadata(meta->getFileName());
        if (meta->isDNG()) {
            // check if this is a DNG
            md.load();
            auto &exif = md.exifData();
            auto it =
                exif.findKey(Exiv2::ExifKey("Exif.SubImage1.OpcodeList3"));
            if (it != exif.end()) {
                std::vector<Exiv2::byte> buf;
                buf.resize(it->value().size());
                it->value().copy(&buf[0], Exiv2::invalidByteOrder);
                data_.reset(DNGCorrectionData::parse(buf));
            }
        } else if (make == "OLYMPUS" || make == "OM DIGITAL SOLUTIONS") {
            if (Exiv2::versionNumber() < EXIV2_MAKE_VERSION(0, 27, 4)) {
                return;
            }
            md.load();
            auto &exif = md.exifData();
            auto it = exif.findKey(Exiv2::ExifKey("Exif.OlympusIp.0x150a"));
            if (it != exif.end() && it->count() == 4) {
                OlympusCorrectionData *od = new OlympusCorrectionData();
                data_.reset(od);
                od->ca_ok = false;
                for (int i = 0; i < 4; ++i) {
                    const float kd = it->toFloat(i);
                    od->distortion[i] = kd;
                }
                it = exif.findKey(Exiv2::ExifKey("Exif.OlympusIp.0x150c"));
                if (it != exif.end() && it->count() == 6) {
                    for (int i = 0; i < 6; ++i) {
                        const float kc = it->toFloat(i);
                        od->cacorr[i] = kc;
                    }
                    od->ca_ok = true;
                } else {
                    for (int i = 0; i < 6; ++i) {
                        od->cacorr[i] = 0;
                    }
                }
            }
        } else {
            auto mn = md.getMakernotes();

            const auto gettag = [&](const char *name) -> std::string {
                auto it = mn.find(name);
                if (it != mn.end()) {
                    return it->second;
                }
                return "";
            };

            const auto getvec = [&](const char *tag) -> std::vector<float> {
                std::istringstream src(gettag(tag));
                std::vector<float> ret;
                float val;
                while (src >> val) {
                    ret.push_back(val);
                }
                return ret;
            };

            if (make == "SONY") {
                auto posd = getvec("DistortionCorrParams");
                auto posc = getvec("ChromaticAberrationCorrParams");
                auto posv = getvec("VignettingCorrParams");

                if (!posd.empty() && !posc.empty() && !posv.empty() &&
                    posd[0] <= 16 && posc[0] == 2 * posd[0] &&
                    posv[0] == posd[0]) {
                    SonyCorrectionData *sony = new SonyCorrectionData();
                    data_.reset(sony);
                    sony->nc = posd[0];
                    for (int i = 0; i < sony->nc; ++i) {
                        sony->distortion[i] = posd[i + 1];
                        sony->ca_r[i] = posc[i + 1];
                        sony->ca_b[i] = posc[sony->nc + i + 1];
                        sony->vignetting[i] = posv[i + 1];
                    }
                }
            } else if (make == "FUJIFILM") {
                auto posd = getvec("GeometricDistortionParams");
                auto posc = getvec("ChromaticAberrationParams");
                auto posv = getvec("VignettingParams");

                if (posd.size() == 19 && posc.size() == 29 &&
                    posv.size() == 19) {
                    FujiCorrectionData *fuji = new FujiCorrectionData();
                    data_.reset(fuji);

                    for (int i = 0; i < 9; i++) {
                        float kd = posd[i + 1], kc = posc[i + 1],
                              kv = posv[i + 1];
                        if (kd != kc || kd != kv) {
                            data_.reset(nullptr);
                            break;
                        }

                        fuji->knots[i] = kd;
                        fuji->distortion[i] = posd[i + 10];
                        fuji->ca_r[i] = posc[i + 10];
                        fuji->ca_b[i] = posc[i + 19];
                        fuji->vignetting[i] = posv[i + 10];
                    }

                    if (data_) {
                        // Account for the 1.25x crop modes in some Fuji cameras
                        std::string val = gettag("CropMode");
                        if (val == "2" || val == "4") {
                            fuji->cropf = 1.25f;
                        } else {
                            fuji->cropf = 1;
                        }
                    }
                }
            }
        }
    } catch (std::exception &exc) {
        data_.reset(nullptr);
    }

    if (data_) {
        data_->get_coeffs(knots_, dist_, vig_, ca_, is_dng_);

        if (is_dng_) {
            float cx_d = knots_[0] * width;
            float cy_d = knots_[1] * height;
            float cx_v = knots_[2] * width;
            float cy_v = knots_[3] * height;
            float mx_d = std::max(cx_d, width - cx_d);
            float my_d = std::max(cy_d, height - cy_d);
            float m_d = std::sqrt(SQR(mx_d) + SQR(my_d));
            float mx_v = std::max(cx_v, width - cx_v);
            float my_v = std::max(cy_v, height - cy_v);
            float m_v = std::sqrt(SQR(mx_v) + SQR(my_v));
            knots_[0] = cx_d;
            knots_[1] = cy_d;
            knots_[2] = cx_v;
            knots_[3] = cy_v;
            knots_.push_back(m_d);
            knots_.push_back(m_v);
        }
    }
}

bool ExifLensCorrection::ok() const { return data_.get(); }

bool ExifLensCorrection::ok(const FramesMetaData *meta)
{
    ExifLensCorrection corr(meta, -1, -1, CoarseTransformParams(), -1);
    return corr.ok();
}

void ExifLensCorrection::correctDistortion(double &x, double &y, int cx, int cy,
                                           double scale) const
{
    if (!data_ || !data_->has_dist()) {
        x *= scale;
        y *= scale;
        return;
    }

    if (!is_dng_) {
        float xx = x + cx;
        float yy = y + cy;
        if (swap_xy_) {
            std::swap(xx, yy);
        }

        float ccx = xx - w2_;
        float ccy = yy - h2_;
        float dr =
            interpolate(knots_, dist_, r_ * std::sqrt(SQR(ccx) + SQR(ccy)));

        x = dr * ccx + w2_;
        y = dr * ccy + h2_;
        if (swap_xy_) {
            std::swap(x, y);
        }
        x -= cx;
        y -= cy;

        x *= scale;
        y *= scale;
    } else if (dist_.size() == 6) {
        float xx = x + cx;
        float yy = y + cy;
        if (swap_xy_) {
            std::swap(xx, yy);
        }

        const float cx1 = knots_[0];
        const float cy1 = knots_[1];
        const float m = knots_[4];

        const float dx = (xx - cx1) / m;
        const float dy = (yy - cy1) / m;
        const float dx2 = SQR(dx);
        const float dy2 = SQR(dy);
        const float r2 = dx2 + dy2;
        const float f =
            dist_[0] + r2 * (dist_[1] + r2 * (dist_[2] + r2 * dist_[3]));
        const float dx_r = f * dx;
        const float dy_r = f * dy;
        const float dxdy2 = 2 * dx * dy;
        const float dx_t = dist_[4] * dxdy2 + dist_[5] * (r2 + 2 * dx2);
        const float dy_t = dist_[5] * dxdy2 + dist_[4] * (r2 + 2 * dx2);

        x = cx1 + m * (dx_r + dx_t);
        y = cy1 + m * (dy_r + dy_t);

        if (swap_xy_) {
            std::swap(x, y);
        }
        x -= cx;
        y -= cy;

        x *= scale;
        y *= scale;
    }
}

bool ExifLensCorrection::isCACorrectionAvailable() const
{
    return data_.get() && data_->has_ca();
}

void ExifLensCorrection::correctCA(double &x, double &y, int cx, int cy,
                                   int channel) const
{
    if (data_ && data_->has_ca()) {
        float xx = x + cx;
        float yy = y + cy;
        if (swap_xy_) {
            std::swap(xx, yy);
        }

        float ccx = xx - w2_;
        float ccy = yy - h2_;
        float dr = interpolate(knots_, ca_[channel],
                               r_ * std::sqrt(SQR(ccx) + SQR(ccy)));

        x = dr * ccx + w2_;
        y = dr * ccy + h2_;
        if (swap_xy_) {
            std::swap(x, y);
        }
        x -= cx;
        y -= cy;
    }
}

void ExifLensCorrection::processVignette(int width, int height,
                                         float **rawData) const
{
    if (!data_ || !data_->has_vign()) {
        return;
    }

    if (!is_dng_) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                float cx = x - w2_;
                float cy = y - h2_;
                float sf = interpolate(knots_, vig_,
                                       r_ * std::sqrt(SQR(cx) + SQR(cy)));
                rawData[y][x] /= SQR(sf);
            }
        }
    } else if (vig_.size() == 5) {
        const float cx = knots_[2];
        const float cy = knots_[3];
        const float m2 = 1.f / SQR(knots_[5]);

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const float r2 = m2 * (SQR(x - cx) + SQR(y - cy));
                const float g =
                    1.f +
                    r2 *
                        (vig_[0] +
                         r2 * (vig_[1] +
                               r2 * (vig_[2] + r2 * (vig_[3] + r2 * vig_[4]))));
                rawData[y][x] *= g;
            }
        }
    }
}

} // namespace rtengine
