/*
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2004-2010 Gabor Horvath <hgabor@rawtherapee.com>
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

#include "previewimage.h"
#include "iccstore.h"
#include "iimage.h"
#include "imgiomanager.h"
#include "rawimagesource.h"
#include "rtthumbnail.h"
#include "stdimagesource.h"
#include "utils.h"
#define BENCHMARK
#include "StopWatch.h"

namespace rtengine {
extern const Settings *settings;
} // namespace rtengine

using namespace rtengine;
using namespace procparams;

PreviewImage::PreviewImage(const Glib::ustring &fname, const Glib::ustring &ext,
                           int width, int height, bool enable_cms,
                           bool compute_histogram)
    : fname_(fname), ext_(ext), width_(width), height_(height),
      enable_cms_(enable_cms), compute_histogram_(compute_histogram),
      loaded_(false), imgprof_(nullptr)
{
}

PreviewImage::~PreviewImage()
{
    if (imgprof_) {
        cmsCloseProfile(imgprof_);
    }
}

void PreviewImage::render(bool enable_cms)
{
    if (img_) {
        cmsHTRANSFORM xform = nullptr;
        if (enable_cms) {
            cmsHPROFILE mprof =
                ICCStore::getInstance()->getActiveMonitorProfile();
            cmsHPROFILE iprof =
                imgprof_ ? imgprof_ : ICCStore::getInstance()->getsRGBProfile();
            if (mprof) {
                lcmsMutex->lock();
                xform = cmsCreateTransform(
                    iprof, TYPE_RGB_8, mprof, TYPE_RGB_8,
                    settings->monitorIntent,
                    cmsFLAGS_NOCACHE |
                        (settings->monitorBPC ? cmsFLAGS_BLACKPOINTCOMPENSATION
                                              : 0));
                lcmsMutex->unlock();
            }
        }
        const unsigned char *data = img_->data;
        int w = img_->getWidth();
        int h = img_->getHeight();

#ifdef _OPENMP
#pragma omp parallel
#endif
        {
            std::vector<unsigned char> line(w * 3);
            unsigned char *buf = &line[0];

#ifdef _OPENMP
#pragma omp for
#endif
            for (unsigned int i = 0; i < (unsigned int)(h); ++i) {
                const unsigned char *src = data + i * w * 3;
                unsigned char *dst = previewImage->get_data() + i * w * 4;

                if (xform) {
                    cmsDoTransform(xform, src, buf, w);
                    src = buf;
                }

                for (unsigned int j = 0; j < (unsigned int)(w); ++j) {
                    unsigned char r = *(src++);
                    unsigned char g = *(src++);
                    unsigned char b = *(src++);

                    poke255_uc(dst, r, g, b);
                }
            }
        }
        previewImage->mark_dirty();

        if (xform) {
            cmsDeleteTransform(xform);
        }
    }
}

void PreviewImage::load()
{
    loaded_ = true;

    auto lext = ext_.lowercase();

    if (lext == "jpg" || lext == "jpeg" || lext == "png" /*|| lext == "tif" || lext == "tiff" || ImageIOManager::getInstance()->canLoad(lext)*/) {
        img_.reset(load_img(fname_, width_, height_));
    } else if (settings->thumbnail_inspector_mode ==
               Settings::ThumbnailInspectorMode::RAW) {
        img_.reset(load_raw(fname_, width_, height_));
        if (settings->thumbnail_inspector_raw_curve ==
            Settings::ThumbnailInspectorRawCurve::RAW_CLIPPING) {
            enable_cms_ = false;
        }
    } else {
        img_.reset(load_raw_preview(fname_, width_, height_));
    }
    if (!img_ && (lext == "tif" || lext == "tiff" ||
                  ImageIOManager::getInstance()->canLoad(lext))) {
        img_.reset(load_img(fname_, width_, height_));
    }

    if (img_) {
        try {
            previewImage = Cairo::ImageSurface::create(
                Cairo::FORMAT_RGB24, img_->getWidth(), img_->getHeight());
            previewImage->flush();
            render(enable_cms_);
        } catch (std::exception &exc) {
            if (settings->verbose) {
                std::cout << "ERROR in creating PreviewImage: " << exc.what()
                          << std::endl;
            }
            previewImage.clear();
        }
    }
}

Cairo::RefPtr<Cairo::ImageSurface> PreviewImage::getImage()
{
    if (!loaded_) {
        load();
    }
    return previewImage;
}

Image8 *PreviewImage::load_img(const Glib::ustring &fname, int w, int h)
{
    StdImageSource imgSrc;
    if (imgSrc.load(fname, std::max(w, 0), std::max(h, 0))) {
        return nullptr;
    }

    ImageIO *img = imgSrc.getImageIO();

    if (w < 0) {
        w = img->getWidth();
        h = img->getHeight();
    } else {
        // (w, h) is a bounding box
        double sw = std::max(double(img->getWidth()) / w, 1.0);
        double sh = std::max(double(img->getHeight()) / h, 1.0);
        if (sw > sh) {
            h = img->getHeight() / sw;
            w = img->getWidth() / sw;
        } else {
            w = img->getWidth() / sh;
            h = img->getHeight() / sh;
        }
    }

    bool has_profile = img->getEmbeddedProfile();

    Image8 *ret = new Image8(w, h);

    if (img->getType() == sImage8) {
        static_cast<Image8 *>(img)->resizeImgTo(w, h, TI_Bilinear, ret);
    } else if (img->getType() == sImage16) {
        static_cast<Image16 *>(img)->resizeImgTo(w, h, TI_Bilinear, ret);
    } else if (img->getType() == sImagefloat) {
        // do the CMS conversion here
        Imagefloat *f = static_cast<Imagefloat *>(img);
        if (has_profile) {
            lcmsMutex->lock();
            cmsHTRANSFORM xform =
                cmsCreateTransform(img->getEmbeddedProfile(), TYPE_RGB_FLT,
                                   ICCStore::getInstance()->getsRGBProfile(),
                                   TYPE_RGB_FLT, INTENT_RELATIVE_COLORIMETRIC,
                                   cmsFLAGS_NOOPTIMIZE | cmsFLAGS_NOCACHE);
            lcmsMutex->unlock();
            f->normalizeFloatTo1();
            f->ExecCMSTransform(xform, true);
            f->normalizeFloatTo65535();
            cmsDeleteTransform(xform);
        }
        has_profile = false;
        f->resizeImgTo(w, h, TI_Bilinear, ret);
    } else {
        delete ret;
        ret = nullptr;
    }

    if (ret && has_profile) {
        int length = 0;
        unsigned char *data = nullptr;
        img->getEmbeddedProfileData(length, data);
        if (data) {
            imgprof_ = cmsOpenProfileFromMem(data, length);
        }
    }

    if (ret && compute_histogram_) {
        get_histogram(ret);
    }

    return ret;
}

void PreviewImage::get_histogram(Image8 *img)
{
    for (int i = 0; i < 3; ++i) {
        hist_[i](256);
    }

    const int W = img->getWidth();
    const int H = img->getHeight();
#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            hist_[0][img->r(y, x)]++;
            hist_[1][img->g(y, x)]++;
            hist_[2][img->b(y, x)]++;
        }
    }
}

void PreviewImage::getHistogram(LUTu &r, LUTu &g, LUTu &b)
{
    r = hist_[0];
    g = hist_[1];
    b = hist_[2];
}

Image8 *PreviewImage::load_raw_preview(const Glib::ustring &fname, int w, int h)
{
    RawImage ri(fname);
    unsigned int imageNum = 0;
    int r = ri.loadRaw(false, imageNum, false);

    if (r) {
        return nullptr;
    }

    Image8 *img = ri.getThumbnail();
    if (!img) {
        return nullptr;
    }

    if (w > 0 && h > 0) {
        double fw = img->getWidth();
        double fh = img->getHeight();
        if ((ri.get_rotateDegree() == 90 || ri.get_rotateDegree() == 270) &&
            ri.thumbNeedsRotation()) {
            std::swap(w, h);
        }
        double sw = std::max(fw / w, 1.0);
        double sh = std::max(fh / h, 1.0);
        if (sw > sh) {
            h = fh / sw;
            w = fw / sw;
        } else {
            w = fw / sh;
            h = fh / sh;
        }
        if ((w != fw || h != fh) && w <= fw && h <= fh) {
            Image8 *img2 = new Image8(w, h);
            img->resizeImgTo(w, h, TI_Bilinear, img2);
            delete img;
            img = img2;
        }
    }

    if (ri.get_rotateDegree() > 0 && ri.thumbNeedsRotation()) {
        img->rotate(ri.get_rotateDegree());
    }

    if (compute_histogram_) {
        for (int i = 0; i < 3; ++i) {
            hist_[i](256);
            for (int j = 0; j < 256; ++j) {
                hist_[i][j] = 0;
            }
        }

        const int W = img->getWidth();
        const int H = img->getHeight();
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                hist_[0][img->r(y, x)]++;
                hist_[1][img->g(y, x)]++;
                hist_[2][img->b(y, x)]++;
            }
        }
    }

    return img;
}

namespace {

class PreviewRawImageSource: public RawImageSource {
public:
    PreviewRawImageSource(int bw, int bh)
        : RawImageSource(), bbox_W_(bw), bbox_H_(bh), demosaiced_(false),
          scaled_(false)
    {
    }

    bool mark_clipped()
    {
        array2D<float> *channels[3] = {&red, &green, &blue};

        constexpr float hl = 65534.f;
        constexpr float sh = hl / 2.f;

        if ((ri->getSensorType() == ST_BAYER ||
             ri->getSensorType() == ST_FUJI_XTRANS) &&
            ri->get_colors() == 3) {
            const bool bayer = ri->getSensorType() == ST_BAYER;
#ifdef _OPENMP
#pragma omp parallel for
#endif
            for (int y = 0; y < H; ++y) {
                for (int x = 0; x < W; ++x) {
                    int c = bayer ? ri->FC(y, x) : ri->XTRANSFC(y, x);
                    if (bayer && c == 3) {
                        c = 1;
                    }
                    if (rawData[y][x] >= clmax[c]) {
                        for (int i = 0; i < 3; ++i) {
                            (*channels[i])[y][x] = 0.f;
                        }
                        (*channels[c])[y][x] = hl;
                    } else if (rawData[y][x] <= 0.f) {
                        for (int i = 0; i < 3; ++i) {
                            (*channels[i])[y][x] = 0.f;
                        }
                        (*channels[c])[y][x] = sh;
                    }
                }
            }
            return true;
        } else if (ri->get_colors() == 1) {
            const float chmax = 65535.f;

#ifdef _OPENMP
#pragma omp parallel for
#endif
            for (int y = 0; y < H; ++y) {
                for (int xx = 0; xx < W; ++xx) {
                    int x = 3 * xx;
                    bool ch = true;
                    bool cl = true;
                    for (int i = 0; i < 3; ++i) {
                        if ((*channels[i])[y][x] < chmax) {
                            ch = false;
                        }
                        if ((*channels[i])[y][x] > 0.f) {
                            cl = false;
                        }
                    }
                    if (ch) {
                        (*channels[0])[y][x] = (*channels[1])[y][x] = hl;
                        (*channels[2])[y][x] = 0.f;
                    } else if (cl) {
                        (*channels[0])[y][x] = (*channels[1])[y][x] = sh;
                        (*channels[2])[y][x] = 0.f;
                    }
                }
            }
            return true;
        }
        return false;
    }

    void mark_clipped_rgb(Imagefloat *img)
    {
        const int iw = img->getWidth();
        const int ih = img->getHeight();

#ifdef _OPENMP
#pragma omp parallel for
#endif
        for (int y = 0; y < ih; ++y) {
            for (int x = 0; x < iw; ++x) {
                float &r = img->r(y, x);
                float &g = img->g(y, x);
                float &b = img->b(y, x);
                if (r >= MAXVALF && b >= MAXVALF && b >= MAXVALF) {
                    r = g = 65534.f;
                    b = 0.f;
                } else if (r <= 0.f && g <= 0.f && b <= 0.f) {
                    r = g = 65534.f / 2.f;
                    b = 0.f;
                } else {
                    r = g = b = Color::rgbLuminance(r, g, b);
                }
            }
        }
    }

    void fast_demosaic(bool mono)
    {
        if (settings->verbose > 1) {
            std::cout << "FAST PREVIEW DEMOSAIC: " << fileName << std::endl;
        }

        BENCHFUN

        scaled_ = rescale(mono);

        if (demosaiced_) {
            return;
        } else if (!mono && scaled_ && ri->isBayer()) {
            bayer_bilinear_demosaic(rawData, red, green, blue);
            return;
        }

        ProcParams pp;
        pp.raw.bayersensor.method = RAWParams::BayerSensor::Method::FAST;
        pp.raw.xtranssensor.method = RAWParams::XTransSensor::Method::FAST;

        if (mono) {
            pp.raw.bayersensor.method = RAWParams::BayerSensor::Method::MONO;
            pp.raw.xtranssensor.method = RAWParams::XTransSensor::Method::MONO;
        }

        double t = 0;
        RawImageSource::demosaic(pp.raw, false, t);
    }

    void getFullSize(int &w, int &h, int tr = TR_NONE) override
    {
        if (scaled_) {
            w = W;
            h = H;

            if (ri) {
                tr = defTransform(ri, tr);
            }

            if ((tr & TR_ROT) == TR_R90 || (tr & TR_ROT) == TR_R270) {
                std::swap(w, h);
            }

            constexpr int border = 8;
            w -= border;
            h -= border;
        } else {
            RawImageSource::getFullSize(w, h, tr);
        }
    }

private:
    bool rescale(bool mono)
    {
        // return false;

        if (bbox_W_ < 0 || bbox_H_ < 0) {
            return false;
        }
        if (fuji || d1x) {
            return false;
        }
        if (numFrames != 1) {
            return false;
        }

        // (w, h) is a bounding box
        double sw = std::max(double(W) / bbox_W_, 1.0);
        double sh = std::max(double(H) / bbox_H_, 1.0);
        int skip = std::max(sw, sh);

        if (settings->verbose > 1) {
            std::cout << "  skip calculation: W = " << W
                      << ", bbox_W = " << bbox_W_ << ", H = " << H
                      << ", bbox_H_ = " << bbox_H_ << ", skip = " << skip
                      << std::endl;
        }

        if (skip <= 1) {
            return false;
        }

        if (ri->getSensorType() == ST_BAYER) {
            if (settings->verbose > 1) {
                std::cout << "SKIP: " << skip << ", FROM: " << W << "x" << H
                          << " to " << (W / skip) << "x" << (H / skip)
                          << std::endl;
            }

            if (!mono) {
                // direct half-size demosaic
                if (!ri || ri->get_ISOspeed() > 200) {
                    skip /= 2;
                }
                if (skip & 1) {
                    --skip;
                }
                if (skip <= 1) {
                    return false;
                }

                int ww = (W / skip) - 1;
                int hh = (H / skip) - 1;
                flushRGB();
                red(ww, hh);
                green(ww, hh);
                blue(ww, hh);

                int yr = 0, xr = 0;
                int yg = 0, xg = 0;
                int yb = 0, xb = 0;
                switch (FC(0, 0)) {
                case 0:
                    xg = 1;
                    xb = 1;
                    yb = 1;
                    break;
                case 1:
                    if (FC(0, 1) == 2) {
                        xb = 1;
                        yr = 1;
                    } else {
                        xr = 1;
                        yb = 1;
                    }
                    break;
                default:
                    xg = 1;
                    xr = 1;
                    yr = 1;
                    break;
                }

#ifdef _OPENMP
#pragma omp parallel for
#endif
                for (int y = 0; y < hh; ++y) {
                    int yy = y * skip;
                    for (int x = 0; x < ww; ++x) {
                        int xx = x * skip;
                        red[y][x] = rawData[yy + yr][xx + xr];
                        green[y][x] = rawData[yy + yg][xx + xg];
                        blue[y][x] = rawData[yy + yb][xx + xb];
                    }
                }

                rawData.free();
                W = ww;
                H = hh;
                demosaiced_ = true;
            } else {
                array2D<float> tmp;
                tmp(W, H, static_cast<float *>(rawData), 0);
                W /= skip;
                H /= skip;
                rawData.free();
                rawData(W, H);

#ifdef _OPENMP
#pragma omp parallel for
#endif
                for (int y = 0; y < H; ++y) {
                    int yy = y * skip + int(y & 1);
                    for (int x = 0; x < W; ++x) {
                        int xx = x * skip + int(x & 1);
                        rawData[y][x] = tmp[yy][xx];
                    }
                }
                flushRGB();
                red(W, H);
                green(W, H);
                blue(W, H);
            }
        } else if (ri->getSensorType() == ST_FUJI_XTRANS) {
            return false; // TODO
        } else {
            int c = ri->get_colors();
            if (c > 3) {
                return false;
            }

            if (settings->verbose > 1) {
                std::cout << "SKIP: " << skip << ", FROM: " << W << "x" << H
                          << " to " << (W / skip) << "x" << (H / skip)
                          << std::endl;
            }

            array2D<float> tmp;
            tmp(W * c, H, static_cast<float *>(rawData), 0);
            W /= skip;
            H /= skip;
            rawData.free();
            rawData(W * c, H);

#ifdef _OPENMP
#pragma omp parallel for
#endif
            for (int y = 0; y < H; ++y) {
                int yy = y * skip;
                for (int x = 0; x < W; ++x) {
                    int xx = x * skip;
                    for (int j = 0; j < c; ++j) {
                        rawData[y][c * x + j] = tmp[yy][c * xx + j];
                    }
                }
            }
            flushRGB();
            red(W, H);
            green(W, H);
            blue(W, H);
        }
        return true;
    }

    void bayer_bilinear_demosaic(const array2D<float> &rawData,
                                 array2D<float> &red, array2D<float> &green,
                                 array2D<float> &blue)
    {
        // BENCHFUN
#ifdef _OPENMP
#pragma omp parallel for
#endif
        for (int i = 1; i < H - 1; ++i) {
            float **nonGreen1 = red;
            float **nonGreen2 = blue;
            if (FC(i, 0) == 2 || FC(i, 1) == 2) { // blue row => swap pointers
                std::swap(nonGreen1, nonGreen2);
            }
#if defined(__clang__)
#pragma clang loop vectorize(assume_safety)
#elif defined(__GNUC__)
#pragma GCC ivdep
#endif
            for (int j = 2 - (FC(i, 1) & 1); j < W - 2;
                 j += 2) { // always begin with a green pixel
                green[i][j] = rawData[i][j];
                nonGreen1[i][j] =
                    (rawData[i][j - 1] + rawData[i][j + 1]) * 0.5f;
                nonGreen2[i][j] =
                    (rawData[i - 1][j] + rawData[i + 1][j]) * 0.5f;
                green[i][j + 1] =
                    ((rawData[i - 1][j + 1] + rawData[i][j]) +
                     (rawData[i][j + 2] + rawData[i + 1][j + 1])) *
                    0.25f;
                nonGreen1[i][j + 1] = rawData[i][j + 1];
                nonGreen2[i][j + 1] =
                    ((rawData[i - 1][j] + rawData[i - 1][j + 2]) +
                     (rawData[i + 1][j] + rawData[i + 1][j + 2])) *
                    0.25f;
            }
        }
        border_interpolate2(W, H, 2, rawData, red, green, blue);
    }

    int bbox_W_;
    int bbox_H_;
    bool demosaiced_;
    bool scaled_;
};

} // namespace

Image8 *PreviewImage::load_raw(const Glib::ustring &fname, int w, int h)
{
    PreviewRawImageSource src(w, h);
    int err = src.load(fname, true);
    if (err) {
        return nullptr;
    }

    const bool show_clip = settings->thumbnail_inspector_raw_curve ==
                           Settings::ThumbnailInspectorRawCurve::RAW_CLIPPING;

    ProcParams neutral;
    neutral.icm.inputProfile = "(camera)";
    neutral.icm.workingProfile = "sRGB";
    if (show_clip) {
        neutral.raw.bayersensor.method = RAWParams::BayerSensor::Method::MONO;
        neutral.raw.xtranssensor.method = RAWParams::XTransSensor::Method::MONO;
    }

    ColorTemp wb = show_clip ? ColorTemp() : src.getWB();
    src.preprocess(neutral.raw, neutral.lensProf, neutral.coarse, false, wb);

    // src.rescale();
    src.fast_demosaic(show_clip);

    if (compute_histogram_) {
        src.getRAWHistogram(hist_[0], hist_[1], hist_[2]);
    }

    bool marked = show_clip && src.mark_clipped();

    int fw, fh;
    src.getFullSize(fw, fh);
    double scale = 1.0;

    if (w < 0) {
        w = fw;
        h = fh;
        scale = 1.0;
    } else {
        // (w, h) is a bounding box
        double sw = std::max(double(fw) / w, 1.0);
        double sh = std::max(double(fh) / h, 1.0);
        if (sw > sh) {
            scale = sw;
        } else {
            scale = sh;
        }
        w = fw / scale;
        h = fh / scale;
    }

    PreviewProps pp(0, 0, fw, fh, int(scale)); // 1);
    int iw = fw / int(scale);
    int ih = fh / int(scale);

    Imagefloat tmp(iw, ih);
    if (show_clip) {
        wb = ColorTemp();
    }
    src.getImage(wb, TR_NONE, &tmp, pp, neutral.exposure, neutral.raw);
    if (!show_clip) {
        src.convertColorSpace(&tmp, neutral.icm, wb);
    } else if (!marked) {
        src.mark_clipped_rgb(&tmp);
    }

    LUTi gamma(65536);
    const bool apply_curve = settings->thumbnail_inspector_raw_curve !=
                                 Settings::ThumbnailInspectorRawCurve::LINEAR &&
                             !show_clip;
    if (apply_curve) {
        if (settings->thumbnail_inspector_raw_curve ==
            Settings::ThumbnailInspectorRawCurve::FILM) {
            DiagonalCurve curve(curves::filmcurve_def);
            for (int i = 0; i < 65536; ++i) {
                float x = Color::gamma_srgbclipped(i) / 65535.f;
                float y = curve.getVal(x) * 255.f;
                gamma[i] = y;
            }
        } else {
            constexpr float base = 500.f;
            for (int i = 0; i < 65536; ++i) {
                float x = float(i) / 65535.f;
                float y = xlin2log(x, base);
                gamma[i] = y * 255.f;
            }
        }
    } else {
        for (int i = 0; i < 65536; ++i) {
            gamma[i] = int(Color::gamma_srgbclipped(i)) * 255 / 65535;
        }
    }

    Image8 *img = new Image8(iw, ih);
    const int maxval = MAXVALF;
#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (int y = 0; y < ih; ++y) {
        for (int x = 0; x < iw; ++x) {
            int r = tmp.r(y, x);
            int g = tmp.g(y, x);
            int b = tmp.b(y, x);
            // avoid magenta highlights
            if (r >= maxval && b >= maxval) {
                int v = CLIP((r + g + b) / 3) * 255 / 65535;
                img->r(y, x) = img->g(y, x) = img->b(y, x) = v;
            } else {
                img->r(y, x) = gamma[r];
                img->g(y, x) = gamma[g];
                img->b(y, x) = gamma[b];
            }
        }
    }

    if (w != iw || h != ih) {
        Image8 *rimg = new Image8(w, h);
        img->resizeImgTo(w, h, TI_Bilinear, rimg);
        delete img;
        img = rimg;
    }

    return img;
}
