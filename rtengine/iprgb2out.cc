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
#include "../rtgui/options.h"
#include "alignedbuffer.h"
#include "color.h"
#include "curves.h"
#include "iccmatrices.h"
#include "iccstore.h"
#include "improcfun.h"
#include "rtengine.h"
#include "settings.h"
#include <glibmm.h>

#define BENCHMARK
#include "StopWatch.h"

namespace rtengine {

extern const Settings *settings;

namespace {

inline void copyAndClampLine(const float *src, unsigned char *dst, const int W,
                             bool scale = true)
{
    if (scale) {
        for (int j = 0; j < W * 3; ++j) {
            dst[j] = uint16ToUint8Rounded(CLIP(src[j] * MAXVALF));
        }
    } else {
        for (int j = 0; j < W * 3; ++j) {
            dst[j] = uint16ToUint8Rounded(CLIP(src[j]));
        }
    }
}

inline void copyAndClamp(Imagefloat *src, unsigned char *dst,
                         const float rgb_xyz[3][3], bool multiThread)
{
    src->setMode(Imagefloat::Mode::XYZ, multiThread);

    const int W = src->getWidth();
    const int H = src->getHeight();

#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic, 16) if (multiThread)
#endif
    for (int i = 0; i < H; ++i) {
        float *rx = src->r.ptrs[i];
        float *ry = src->g.ptrs[i];
        float *rz = src->b.ptrs[i];

        int ix = i * 3 * W;

        float R, G, B;
        float x_, y_, z_;

        for (int j = 0; j < W; ++j) {
            x_ = rx[j];
            y_ = ry[j];
            z_ = rz[j];
            Color::xyz2rgb(x_, y_, z_, R, G, B, rgb_xyz);

            dst[ix++] = uint16ToUint8Rounded(Color::gamma2curve[CLIP(R)]);
            dst[ix++] = uint16ToUint8Rounded(Color::gamma2curve[CLIP(G)]);
            dst[ix++] = uint16ToUint8Rounded(Color::gamma2curve[CLIP(B)]);
        }
    }
}

class ARTOutputProfile {
public:
    ARTOutputProfile(cmsHPROFILE prof,
                     const procparams::ColorManagementParams &icm,
                     const Glib::ustring &colorspace, int lutsz)
        : mode_(MODE_INVALID), tc_(nullptr), lutsz_(lutsz),
          factor_(float(lutsz - 1))
    {
        auto ws = ICCStore::getInstance()->workingSpaceMatrix(colorspace);
        Mat33<float> m, mi;
        float g = 0, s = 0;
        cmsCIEXYZ bp;
        if (ICCStore::getProfileMatrix(prof, m) &&
            ICCStore::getProfileParametricTRC(prof, g, s) && inverse(m, mi) &&
            (!icm.outputBPC ||
             !cmsDetectDestinationBlackPoint(&bp, prof, icm.outputIntent, 0) ||
             (bp.X == 0 && bp.Y == 0 && bp.Z == 0))) {
            if (g == -2) {
                mode_ = MODE_PQ;
            } else if (g == -1) {
                mode_ = MODE_HLG;
            } else if (g == 1 && s == 0) {
                mode_ = MODE_LINEAR;
            } else {
                mode_ = MODE_GAMMA;
                LMCSToneCurveParams params;
                Color::compute_LCMS_tone_curve_params(g, s, params);
                auto tc = cmsBuildParametricToneCurve(0, 5, &params[0]);
                if (tc) {
                    tc_ = cmsReverseToneCurve(tc);
                    cmsFreeToneCurve(tc);
                }
                if (!tc_) {
                    mode_ = MODE_INVALID;
                }
            }
            matrix_ = dot_product(mi, ws);
        }
        if (mode_ != MODE_INVALID && lutsz_ > 0) {
            compute_lut();
        }
    }

    ~ARTOutputProfile()
    {
        if (tc_) {
            cmsFreeToneCurve(tc_);
        }
    }

    operator bool() const { return mode_ != MODE_INVALID; }

    void operator()(const Imagefloat *src, Imagefloat *dst, bool multiThread)
    {
        const int W = src->getWidth();
        const int H = src->getHeight();

#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif
        for (int y = 0; y < H; ++y) {
            Vec3<float> rgb;
            for (int x = 0; x < W; ++x) {
                rgb[0] = src->r(y, x) / 65535.f;
                rgb[1] = src->g(y, x) / 65535.f;
                rgb[2] = src->b(y, x) / 65535.f;

                rgb = dot_product(matrix_, rgb);

                for (int i = 0; i < 3; ++i) {
                    if (lutsz_ > 0 && rgb[i] <= 1.f) {
                        rgb[i] = lut_[rgb[i] * factor_];
                    } else {
                        rgb[i] = eval(rgb[i]);
                    }
                }

                dst->r(y, x) = rgb[0] * 65535.f;
                dst->g(y, x) = rgb[1] * 65535.f;
                dst->b(y, x) = rgb[2] * 65535.f;
            }
        }
    }

    void operator()(const float *src, float *dst, int W)
    {
        Vec3<float> rgb;
        const int W3 = W * 3;
        for (int x = 0; x < W3; x += 3) {
            rgb[0] = src[x];
            rgb[1] = src[x + 1];
            rgb[2] = src[x + 2];

            rgb = dot_product(matrix_, rgb);

            for (int i = 0; i < 3; ++i) {
                if (lutsz_ > 0 && rgb[i] <= 1.f) {
                    rgb[i] = lut_[rgb[i] * factor_];
                } else {
                    rgb[i] = eval(rgb[i]);
                }
            }

            dst[x] = rgb[0];
            dst[x + 1] = rgb[1];
            dst[x + 2] = rgb[2];
        }
    }

private:
    float eval(float x)
    {
        switch (mode_) {
        case MODE_LINEAR:
            return x;
        case MODE_PQ:
            return Color::eval_PQ_curve(x, true);
        case MODE_HLG:
            return Color::eval_HLG_curve(x, true);
        default: // MODE_GAMMA
            return cmsEvalToneCurveFloat(tc_, x);
        }
    }

    void compute_lut()
    {
        lut_(lutsz_);
        for (int i = 0; i < lutsz_; ++i) {
            float x = float(i) / float(lutsz_ - 1);
            float y = eval(x);
            lut_[i] = y;
        }
    }

    enum Mode { MODE_INVALID, MODE_LINEAR, MODE_GAMMA, MODE_HLG, MODE_PQ };
    Mode mode_;
    Mat33<float> matrix_;
    cmsToneCurve *tc_;
    const int lutsz_;
    const float factor_;
    LUTf lut_;
};

} // namespace

void ImProcFunctions::rgb2monitor(Imagefloat *img, Image8 *image,
                                  bool bypass_out)
{
    //    BENCHFUN
    Imagefloat *inimg = img;

    image->allocate(img->getWidth(), img->getHeight());

    if (monitorTransform) {
        std::unique_ptr<Imagefloat> del_img;
        if (!bypass_out) {
            img = rgb2out(img, params->icm);
            del_img.reset(img);
            img->setMode(Imagefloat::Mode::RGB, multiThread);
        } else {
            img->setMode(Imagefloat::Mode::LAB, multiThread);
        }
        if (gamutWarning) {
            inimg->setMode(Imagefloat::Mode::LAB, multiThread);
        }

        const int W = img->getWidth();
        const int H = img->getHeight();
        unsigned char *data = image->data;

        // cmsDoTransform is relatively expensive
#ifdef _OPENMP
#pragma omp parallel firstprivate(img, data, W, H) if (multiThread)
#endif
        {
            AlignedBuffer<float> pBuf(3 * W);
            AlignedBuffer<float> mBuf(3 * W);

            AlignedBuffer<float> gwBuf1;
            AlignedBuffer<float> gwBuf2;
            AlignedBuffer<float> gwSrcBuf;

            if (gamutWarning) {
                gwSrcBuf.resize(3 * W);
                gwBuf1.resize(3 * W);
                gwBuf2.resize(3 * W);
            }

            float *buffer = pBuf.data;
            float *outbuffer = mBuf.data;
            float *gwbuffer = gwSrcBuf.data;

#ifdef _OPENMP
#pragma omp for schedule(dynamic, 16)
#endif

            for (int i = 0; i < H; i++) {

                const int ix = i * 3 * W;
                int iy = 0;

                if (gamutWarning) {
                    float *rL = inimg->g(i);
                    float *ra = inimg->r(i);
                    float *rb = inimg->b(i);

                    for (int j = 0; j < W; j++) {
                        gwbuffer[iy++] = rL[j] / 327.68f;
                        gwbuffer[iy++] = ra[j] / 327.68f;
                        gwbuffer[iy++] = rb[j] / 327.68f;
                    }
                }

                iy = 0;
                if (!bypass_out) {
                    float *rr = img->r(i);
                    float *rg = img->g(i);
                    float *rb = img->b(i);

                    for (int j = 0; j < W; j++) {
                        buffer[iy++] = rr[j] / 65535.f;
                        buffer[iy++] = rg[j] / 65535.f;
                        buffer[iy++] = rb[j] / 65535.f;
                    }
                } else {
                    float *rL = inimg->g(i);
                    float *ra = inimg->r(i);
                    float *rb = inimg->b(i);

                    for (int j = 0; j < W; j++) {
                        buffer[iy++] = rL[j] / 327.68f;
                        buffer[iy++] = ra[j] / 327.68f;
                        buffer[iy++] = rb[j] / 327.68f;
                    }
                }

                cmsDoTransform(monitorTransform, buffer, outbuffer, W);
                copyAndClampLine(outbuffer, data + ix, W);

                if (gamutWarning) {
                    gamutWarning->markLine(image, i, gwSrcBuf.data, gwBuf1.data,
                                           gwBuf2.data);
                }
            }
        } // End of parallelization
    } else {
        img->setMode(Imagefloat::Mode::LAB, multiThread);
        copyAndClamp(img, image->data, sRGB_xyz, multiThread);
    }
}

Image8 *ImProcFunctions::rgb2out(Imagefloat *img, int cx, int cy, int cw,
                                 int ch,
                                 const procparams::ColorManagementParams &icm,
                                 bool consider_histogram_settings)
{
    if (cx < 0) {
        cx = 0;
    }

    if (cy < 0) {
        cy = 0;
    }

    const int W = img->getWidth();
    const int H = img->getHeight();

    if (cx + cw > W) {
        cw = W - cx;
    }

    if (cy + ch > H) {
        ch = H - cy;
    }

    Image8 *image = new Image8(cw, ch);
    Glib::ustring profile;

    cmsHPROFILE oprof = nullptr;

    if (settings->HistogramWorking && consider_histogram_settings) {
        profile = icm.workingProfile;
    } else {
        profile = icm.outputProfile;

        if (icm.outputProfile.empty() ||
            icm.outputProfile == ColorManagementParams::NoICMString) {
            profile = "sRGB";
        }
        oprof = ICCStore::getInstance()->getProfile(profile);
    }

    if (oprof) {
        img->setMode(Imagefloat::Mode::RGB, true);

        cmsHTRANSFORM hTransform = nullptr;

        ARTOutputProfile op(oprof, icm, img->colorSpace(), 256);

        if (!op) {
            cmsUInt32Number flags = cmsFLAGS_NOOPTIMIZE | cmsFLAGS_NOCACHE;

            if (icm.outputBPC) {
                flags |= cmsFLAGS_BLACKPOINTCOMPENSATION;
            }

            lcmsMutex->lock();
            auto iprof =
                ICCStore::getInstance()->workingSpace(img->colorSpace());
            hTransform = cmsCreateTransform(
                iprof, TYPE_RGB_FLT, oprof, TYPE_RGB_FLT, icm.outputIntent,
                flags); // NOCACHE is important for thread safety
            lcmsMutex->unlock();
        }

        unsigned char *data = image->data;

        // cmsDoTransform is relatively expensive
#ifdef _OPENMP
#pragma omp parallel if (multiThread)
#endif
        {
            AlignedBuffer<float> pBuf(3 * cw);
            AlignedBuffer<float> oBuf(3 * cw);
            float *buffer = pBuf.data;
            float *outbuffer = oBuf.data;
            int condition = cy + ch;

#ifdef _OPENMP
#pragma omp for firstprivate(img) schedule(dynamic, 16)
#endif

            for (int i = cy; i < condition; i++) {
                const int ix = i * 3 * cw;
                int iy = 0;
                float *rr = img->r(i);
                float *rg = img->g(i);
                float *rb = img->b(i);

                for (int j = cx; j < cx + cw; j++) {
                    buffer[iy++] = rr[j] / 65535.f;
                    buffer[iy++] = rg[j] / 65535.f;
                    buffer[iy++] = rb[j] / 65535.f;
                }

                if (op) {
                    op(buffer, outbuffer, cw);
                } else {
                    cmsDoTransform(hTransform, buffer, outbuffer, cw);
                }
                copyAndClampLine(outbuffer, data + ix, cw);
            }
        } // End of parallelization

        if (hTransform) {
            cmsDeleteTransform(hTransform);
        }
    } else {
        const auto xyz_rgb =
            ICCStore::getInstance()->workingSpaceInverseMatrix(profile);
        copyAndClamp(img, image->data, xyz_rgb, multiThread);
    }

    return image;
}

Imagefloat *
ImProcFunctions::rgb2out(Imagefloat *img,
                         const procparams::ColorManagementParams &icm)
{
    // BENCHFUN

    constexpr int cx = 0;
    constexpr int cy = 0;
    const int cw = img->getWidth();
    const int ch = img->getHeight();

    Imagefloat *image = new Imagefloat(cw, ch);
    cmsHPROFILE oprof = ICCStore::getInstance()->getProfile(icm.outputProfile);

    if (oprof) {
        img->setMode(Imagefloat::Mode::RGB, multiThread);

        ARTOutputProfile op(
            oprof, icm, img->colorSpace(),
            cur_pipeline == Pipeline::OUTPUT
                ? -1
                : (cur_pipeline == Pipeline::PREVIEW && scale == 1
                       ? 65536
                       : (cur_pipeline == Pipeline::THUMBNAIL ? 256 : 1024)));
        if (op) {
            // if (settings->verbose) {
            //     std::cout << "rgb2out: converting using fast path" <<
            //     std::endl;
            // }
            op(img, image, multiThread);
        } else {
            cmsUInt32Number flags = cmsFLAGS_NOOPTIMIZE | cmsFLAGS_NOCACHE;

            if (icm.outputBPC) {
                flags |= cmsFLAGS_BLACKPOINTCOMPENSATION;
            }

            lcmsMutex->lock();
            cmsHPROFILE iprof =
                ICCStore::getInstance()->workingSpace(img->colorSpace());
            cmsHTRANSFORM hTransform =
                cmsCreateTransform(iprof, TYPE_RGB_FLT, oprof, TYPE_RGB_FLT,
                                   icm.outputIntent, flags);
            lcmsMutex->unlock();

            image->ExecCMSTransform(hTransform, img, multiThread);
            cmsDeleteTransform(hTransform);
        }
    } else if (icm.outputProfile !=
               procparams::ColorManagementParams::NoProfileString) {
        img->setMode(Imagefloat::Mode::XYZ, multiThread);

#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic, 16) if (multiThread)
#endif
        for (int i = cy; i < cy + ch; i++) {
            float R, G, B;

            for (int j = cx; j < cx + cw; j++) {
                float x_ = img->r(i, j);
                float y_ = img->g(i, j);
                float z_ = img->b(i, j);

                Color::xyz2srgb(x_, y_, z_, R, G, B);

                image->r(i - cy, j - cx) = Color::gamma2curve[CLIP(R)];
                image->g(i - cy, j - cx) = Color::gamma2curve[CLIP(G)];
                image->b(i - cy, j - cx) = Color::gamma2curve[CLIP(B)];
            }
        }
    } else {
        img->copyTo(image);
        image->setMode(Imagefloat::Mode::RGB, multiThread);
    }

    return image;
}

} // namespace rtengine
