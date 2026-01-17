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
#include "imagefloat.h"
#include "alignedbuffer.h"
#include "color.h"
#include "halffloat.h"
#include "iccstore.h"
#include "image16.h"
#include "image8.h"
#include "mytime.h"
#include "rt_math.h"
#include "rtengine.h"
#include "sleef.h"
#include <cstring>
#include <tiffio.h>

namespace rtengine {

Imagefloat::Imagefloat(): color_space_("sRGB"), mode_(Mode::RGB)
{
    ws_[0][0] = RT_INFINITY_F;
    iws_[0][0] = RT_INFINITY_F;
}

Imagefloat::Imagefloat(int w, int h, const Imagefloat *state_from)
    : color_space_("sRGB"), mode_(Mode::RGB)
{
    allocate(w, h);
    ws_[0][0] = RT_INFINITY_F;
    iws_[0][0] = RT_INFINITY_F;
    if (state_from) {
        state_from->copyState(this);
    }
}

Imagefloat::~Imagefloat() {}

// Call this method to handle floating points input values of different size
void Imagefloat::setScanline(int row, unsigned char *buffer, int bps,
                             unsigned int numSamples)
{
    if (data == nullptr) {
        return;
    }

    // The DNG decoder convert to 32 bits float data even if the file contains
    // 16 or 24 bits data. DNG_HalfToFloat and DNG_FP24ToFloat from dcraw.cc can
    // be used to manually convert from 16 and 24 bits to 32 bits float
    // respectively
    constexpr float hival = std::numeric_limits<float>::max() / 100.f;

    switch (sampleFormat) {
    case (IIOSF_FLOAT16): {
        int ix = 0;
        uint16_t *sbuffer = (uint16_t *)buffer;

        for (int i = 0; i < width; i++) {
            r(row, i) =
                std::min(hival, 65535.f * DNG_HalfToFloat(sbuffer[ix++]));
            g(row, i) =
                std::min(hival, 65535.f * DNG_HalfToFloat(sbuffer[ix++]));
            b(row, i) =
                std::min(hival, 65535.f * DNG_HalfToFloat(sbuffer[ix++]));
        }

        break;
    }
    // case (IIOSF_FLOAT24):
    case (IIOSF_FLOAT32): {
        int ix = 0;
        float *sbuffer = (float *)buffer;

        for (int i = 0; i < width; i++) {
            r(row, i) = std::min(hival, 65535.f * sbuffer[ix++]);
            g(row, i) = std::min(hival, 65535.f * sbuffer[ix++]);
            b(row, i) = std::min(hival, 65535.f * sbuffer[ix++]);
        }

        break;
    }

    case (IIOSF_LOGLUV24):
    case (IIOSF_LOGLUV32): {
        int ix = 0;
        float *sbuffer = (float *)buffer;
        float xyzvalues[3], rgbvalues[3];

        for (int i = 0; i < width; i++) {
            xyzvalues[0] = sbuffer[ix++];
            xyzvalues[1] = sbuffer[ix++];
            xyzvalues[2] = sbuffer[ix++];
            // TODO: we may have to handle other color space than sRGB!
            Color::xyz2srgb(xyzvalues[0], xyzvalues[1], xyzvalues[2],
                            rgbvalues[0], rgbvalues[1], rgbvalues[2]);
            r(row, i) = rgbvalues[0];
            g(row, i) = rgbvalues[1];
            b(row, i) = rgbvalues[2];
        }

        break;
    }

    default:
        // Other type are ignored, but could be implemented if necessary
        break;
    }
}

void Imagefloat::getScanline(int row, unsigned char *buffer, int bps,
                             bool isFloat) const
{

    if (data == nullptr) {
        return;
    }

    if (isFloat) {
        if (bps == 32) {
            int ix = 0;
            float *sbuffer = (float *)buffer;
            // agriggio -- assume the image is normalized to [0, 65535]
            for (int i = 0; i < width; i++) {
                sbuffer[ix++] = r(row, i) / 65535.f;
                sbuffer[ix++] = g(row, i) / 65535.f;
                sbuffer[ix++] = b(row, i) / 65535.f;
            }
        } else if (bps == 16) {
            int ix = 0;
            uint16_t *sbuffer = (uint16_t *)buffer;
            // agriggio -- assume the image is normalized to [0, 65535]
            for (int i = 0; i < width; i++) {
                sbuffer[ix++] = DNG_FloatToHalf(r(row, i) / 65535.f);
                sbuffer[ix++] = DNG_FloatToHalf(g(row, i) / 65535.f);
                sbuffer[ix++] = DNG_FloatToHalf(b(row, i) / 65535.f);
            }
        }
    } else {
        unsigned short *sbuffer = (unsigned short *)buffer;
        for (int i = 0, ix = 0; i < width; i++) {
            float ri = r(row, i);
            float gi = g(row, i);
            float bi = b(row, i);
            if (bps == 16) {
                sbuffer[ix++] = CLIP(ri);
                sbuffer[ix++] = CLIP(gi);
                sbuffer[ix++] = CLIP(bi);
            } else if (bps == 8) {
                buffer[ix++] = rtengine::uint16ToUint8Rounded(CLIP(ri));
                buffer[ix++] = rtengine::uint16ToUint8Rounded(CLIP(gi));
                buffer[ix++] = rtengine::uint16ToUint8Rounded(CLIP(bi));
            }
        }
    }
}

Imagefloat *Imagefloat::copy() const
{
    Imagefloat *cp = new Imagefloat(width, height);
    copyTo(cp);
    return cp;
}

void Imagefloat::copyTo(Imagefloat *dst) const
{
    copyData(dst);
    copyState(dst);
}

void Imagefloat::copyState(Imagefloat *to) const
{
    to->color_space_ = color_space_;
    to->mode_ = mode_;
    to->ws_[0][0] = RT_INFINITY_F;
    to->iws_[0][0] = RT_INFINITY_F;
}

// This is called by the StdImageSource class. We assume that fp images from
// StdImageSource don't have to deal with gamma
void Imagefloat::getStdImage(const ColorTemp &ctemp, int tran,
                             Imagefloat *image, PreviewProps pp) const
{

    // compute channel multipliers
    float rm = 1.f, gm = 1.f, bm = 1.f;
    if (ctemp.getTemp() >= 0) {
        double drm, dgm, dbm;
        ctemp.getMultipliers(drm, dgm, dbm);
        rm = drm;
        gm = dgm;
        bm = dbm;

        rm = 1.0 / rm;
        gm = 1.0 / gm;
        bm = 1.0 / bm;
        float mul_lum = 0.299 * rm + 0.587 * gm + 0.114 * bm;
        rm /= mul_lum;
        gm /= mul_lum;
        bm /= mul_lum;
    }

    int sx1, sy1, sx2, sy2;

    transform(pp, tran, sx1, sy1, sx2, sy2);

    int imwidth = image->width;   // Destination image
    int imheight = image->height; // Destination image

    if (((tran & TR_ROT) == TR_R90) || ((tran & TR_ROT) == TR_R270)) {
        int swap = imwidth;
        imwidth = imheight;
        imheight = swap;
    }

    int maxx = width;  // Source image
    int maxy = height; // Source image
    int mtran = tran & TR_ROT;
    int skip = pp.getSkip();

    // improve speed by integrating the area division into the multipliers
    // switched to using ints for the red/green/blue channel buffer.
    // Incidentally this improves accuracy too.
    float area = skip * skip;
    float rm2 = rm;
    float gm2 = gm;
    float bm2 = bm;
    rm /= area;
    gm /= area;
    bm /= area;

    const auto CLIP0 = [](float v) -> float { return std::max(v, 0.f); };

#ifdef _OPENMP
#pragma omp parallel
    {
#endif
        AlignedBuffer<float> abR(imwidth);
        AlignedBuffer<float> abG(imwidth);
        AlignedBuffer<float> abB(imwidth);
        float *lineR = abR.data;
        float *lineG = abG.data;
        float *lineB = abB.data;

#ifdef _OPENMP
#pragma omp for
#endif

        for (int iy = 0; iy < imheight; iy++) {
            if (skip == 1) {
                // special case (speedup for 1:1 scale)
                // i: source image, first line of the current destination row
                int src_y = sy1 + iy;

                // overflow security check, not sure that it's necessary
                if (src_y >= maxy) {
                    continue;
                }

                for (int dst_x = 0, src_x = sx1; dst_x < imwidth;
                     dst_x++, src_x++) {
                    // overflow security check, not sure that it's necessary
                    if (src_x >= maxx) {
                        continue;
                    }

                    lineR[dst_x] = CLIP0(rm2 * r(src_y, src_x));
                    lineG[dst_x] = CLIP0(gm2 * g(src_y, src_x));
                    lineB[dst_x] = CLIP0(bm2 * b(src_y, src_x));
                }
            } else {
                // source image, first line of the current destination row
                int src_y = sy1 + skip * iy;

                if (src_y >= maxy) {
                    continue;
                }

                for (int dst_x = 0, src_x = sx1; dst_x < imwidth;
                     dst_x++, src_x += skip) {
                    if (src_x >= maxx) {
                        continue;
                    }

                    int src_sub_width = MIN(maxx - src_x, skip);
                    int src_sub_height = MIN(maxy - src_y, skip);

                    float rtot, gtot, btot; // RGB accumulators
                    rtot = gtot = btot = 0.;

                    for (int src_sub_y = 0; src_sub_y < src_sub_height;
                         src_sub_y++)
                        for (int src_sub_x = 0; src_sub_x < src_sub_width;
                             src_sub_x++) {
                            rtot += r(src_y + src_sub_y, src_x + src_sub_x);
                            gtot += g(src_y + src_sub_y, src_x + src_sub_x);
                            btot += b(src_y + src_sub_y, src_x + src_sub_x);
                        }

                    // convert back to gamma and clip
                    if (src_sub_width == skip && src_sub_height == skip) {
                        // Common case where the sub-region is complete
                        lineR[dst_x] = CLIP0(rm * rtot);
                        lineG[dst_x] = CLIP0(gm * gtot);
                        lineB[dst_x] = CLIP0(bm * btot);
                    } else {
                        // computing a special factor for this incomplete
                        // sub-region
                        float area = src_sub_width * src_sub_height;
                        lineR[dst_x] = CLIP0(rm2 * rtot / area);
                        lineG[dst_x] = CLIP0(gm2 * gtot / area);
                        lineB[dst_x] = CLIP0(bm2 * btot / area);
                    }
                }
            }

            if (mtran == TR_NONE)
                for (int dst_x = 0, src_x = sx1; dst_x < imwidth;
                     dst_x++, src_x += skip) {
                    image->r(iy, dst_x) = lineR[dst_x];
                    image->g(iy, dst_x) = lineG[dst_x];
                    image->b(iy, dst_x) = lineB[dst_x];
                }
            else if (mtran == TR_R180)
                for (int dst_x = 0; dst_x < imwidth; dst_x++) {
                    image->r(imheight - 1 - iy, imwidth - 1 - dst_x) =
                        lineR[dst_x];
                    image->g(imheight - 1 - iy, imwidth - 1 - dst_x) =
                        lineG[dst_x];
                    image->b(imheight - 1 - iy, imwidth - 1 - dst_x) =
                        lineB[dst_x];
                }
            else if (mtran == TR_R90)
                for (int dst_x = 0, src_x = sx1; dst_x < imwidth;
                     dst_x++, src_x += skip) {
                    image->r(dst_x, imheight - 1 - iy) = lineR[dst_x];
                    image->g(dst_x, imheight - 1 - iy) = lineG[dst_x];
                    image->b(dst_x, imheight - 1 - iy) = lineB[dst_x];
                }
            else if (mtran == TR_R270)
                for (int dst_x = 0, src_x = sx1; dst_x < imwidth;
                     dst_x++, src_x += skip) {
                    image->r(imwidth - 1 - dst_x, iy) = lineR[dst_x];
                    image->g(imwidth - 1 - dst_x, iy) = lineG[dst_x];
                    image->b(imwidth - 1 - dst_x, iy) = lineB[dst_x];
                }
        }

#ifdef _OPENMP
    }
#endif
}

Image8 *Imagefloat::to8() const
{
    Image8 *img8 = new Image8(width, height);
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif

    for (int h = 0; h < height; ++h) {
        for (int w = 0; w < width; ++w) {
            img8->r(h, w) = uint16ToUint8Rounded(CLIP(r(h, w)));
            img8->g(h, w) = uint16ToUint8Rounded(CLIP(g(h, w)));
            img8->b(h, w) = uint16ToUint8Rounded(CLIP(b(h, w)));
        }
    }

    return img8;
}

Image16 *Imagefloat::to16() const
{
    Image16 *img16 = new Image16(width, height);
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif

    for (int h = 0; h < height; ++h) {
        for (int w = 0; w < width; ++w) {
            img16->r(h, w) = CLIP(r(h, w));
            img16->g(h, w) = CLIP(g(h, w));
            img16->b(h, w) = CLIP(b(h, w));
        }
    }

    return img16;
}

void Imagefloat::multiply(float factor, bool multithread)
{
    const int W = width;
    const int H = height;
#ifdef ART_SIMD
    vfloat vfactor = F2V(factor);
#endif

#ifdef _OPENMP
#pragma omp parallel for firstprivate(W, H)                                    \
    schedule(dynamic, 5) if (multithread)
#endif
    for (int y = 0; y < H; y++) {
        int x = 0;
#ifdef ART_SIMD
        for (; x < W - 3; x += 4) {
            vfloat rv = LVF(r(y, x));
            vfloat gv = LVF(g(y, x));
            vfloat bv = LVF(b(y, x));
            STVF(r(y, x), rv * vfactor);
            STVF(g(y, x), gv * vfactor);
            STVF(b(y, x), bv * vfactor);
        }
#endif
        for (; x < W; ++x) {
            r(y, x) *= factor;
            g(y, x) *= factor;
            b(y, x) *= factor;
        }
    }
}

// convert values's range to [0;1] ; this method assumes that the input values's
// range is [0;65535]
void Imagefloat::normalizeFloatTo1(bool multithread)
{
    multiply(1.f / 65535.f, multithread);
}

// convert values's range to [0;65535 ; this method assumes that the input
// values's range is [0;1]
void Imagefloat::normalizeFloatTo65535(bool multithread)
{
    multiply(65535.f, multithread);
}

void Imagefloat::calcCroppedHistogram(const ProcParams &params, float scale,
                                      LUTu &hist)
{

    hist.clear();

    // Set up factors to calc the lightness
    TMatrix wprof =
        ICCStore::getInstance()->workingSpaceMatrix(params.icm.workingProfile);

    float facRed = wprof[1][0];
    float facGreen = wprof[1][1];
    float facBlue = wprof[1][2];

    // calc pixel size
    int x1, x2, y1, y2;
    params.crop.mapToResized(width, height, scale, x1, x2, y1, y2);

#ifdef _OPENMP
#pragma omp parallel
#endif
    {
        LUTu histThr(65536);
        histThr.clear();
#ifdef _OPENMP
#pragma omp for nowait
#endif

        for (int y = y1; y < y2; y++) {
            for (int x = x1; x < x2; x++) {
                int i = (int)(facRed * r(y, x) + facGreen * g(y, x) +
                              facBlue * b(y, x));

                if (i < 0) {
                    i = 0;
                } else if (i > 65535) {
                    i = 65535;
                }

                histThr[i]++;
            }
        }

#ifdef _OPENMP
#pragma omp critical
#endif
        {
            for (int i = 0; i <= 0xffff; i++) {
                hist[i] += histThr[i];
            }
        }
    }
}

// Parallelized transformation; create transform with cmsFLAGS_NOCACHE!
void Imagefloat::ExecCMSTransform(cmsHTRANSFORM hTransform, bool multithread)
{

    // LittleCMS cannot parallelize planar setups -- Hombre: LCMS2.4 can! But it
    // we use this new feature, memory allocation have to be modified too to
    // build temporary buffers that allow multi processor execution
#ifdef _OPENMP
#pragma omp parallel if (multithread)
#endif
    {
        AlignedBuffer<float> pBuf(width * 3);

#ifdef _OPENMP
#pragma omp for schedule(dynamic, 16)
#endif

        for (int y = 0; y < height; y++) {
            float *p = pBuf.data, *pR = r(y), *pG = g(y), *pB = b(y);

            for (int x = 0; x < width; x++) {
                *(p++) = *(pR++);
                *(p++) = *(pG++);
                *(p++) = *(pB++);
            }

            cmsDoTransform(hTransform, pBuf.data, pBuf.data, width);

            p = pBuf.data;
            pR = r(y);
            pG = g(y);
            pB = b(y);

            for (int x = 0; x < width; x++) {
                *(pR++) = *(p++);
                *(pG++) = *(p++);
                *(pB++) = *(p++);
            }
        } // End of parallelization
    }
}

// Parallelized transformation; create transform with cmsFLAGS_NOCACHE!
void Imagefloat::ExecCMSTransform(cmsHTRANSFORM hTransform,
                                  const Imagefloat *src, bool multithread)
{
    mode_ = Mode::RGB;
    constexpr int cx = 0, cy = 0;

    // LittleCMS cannot parallelize planar Lab float images
    // so build temporary buffers to allow multi processor execution
#ifdef _OPENMP
#pragma omp parallel if (multithread)
#endif
    {
        AlignedBuffer<float> bufferSrc(width * 3);
        AlignedBuffer<float> bufferRGB(width * 3);

#ifdef _OPENMP
#pragma omp for schedule(static)
#endif

        for (int y = cy; y < cy + height; y++) {
            float *pRGB, *pR, *pG, *pB;
            float *pSrc, *psR, *psG, *psB;

            pSrc = bufferSrc.data;
            psR = src->r(y) + cx;
            psG = src->g(y) + cx;
            psB = src->b(y) + cx;

            for (int x = 0; x < width; x++) {
                *(pSrc++) = *(psR++) / 65535.f;
                *(pSrc++) = *(psG++) / 65535.f;
                *(pSrc++) = *(psB++) / 65535.f;
            }

            cmsDoTransform(hTransform, bufferSrc.data, bufferRGB.data, width);

            pRGB = bufferRGB.data;
            pR = r(y - cy);
            pG = g(y - cy);
            pB = b(y - cy);

            for (int x = 0; x < width; x++) {
                *(pR++) = *(pRGB++) * 65535.f;
                *(pG++) = *(pRGB++) * 65535.f;
                *(pB++) = *(pRGB++) * 65535.f;
            }
        } // End of parallelization
    }
}

void Imagefloat::assignColorSpace(const Glib::ustring &space)
{
    if (color_space_ != space) {
        color_space_ = space;
        ws_[0][0] = RT_INFINITY_F;
        iws_[0][0] = RT_INFINITY_F;
    }
}

inline void Imagefloat::get_ws()
{
    if (!std::isfinite(ws_[0][0])) {
        TMatrix ws = ICCStore::getInstance()->workingSpaceMatrix(color_space_);
        TMatrix iws =
            ICCStore::getInstance()->workingSpaceInverseMatrix(color_space_);
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                ws_[i][j] = float(ws[i][j]);
                iws_[i][j] = float(iws[i][j]);
            }
        }
#ifdef ART_SIMD
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                vws_[i][j] = F2V(float(ws[i][j]));
                viws_[i][j] = F2V(float(iws[i][j]));
            }
        }
#endif
    }
}

void Imagefloat::setMode(Mode mode, bool multithread)
{
    if (mode == this->mode()) {
        return;
    }

    switch (this->mode()) {
    case Mode::RGB:
        if (mode == Mode::XYZ) {
            rgb_to_xyz(multithread);
        } else if (mode == Mode::YUV) {
            rgb_to_yuv(multithread);
        } else {
            rgb_to_lab(multithread);
        }
        break;
    case Mode::XYZ:
        if (mode == Mode::RGB) {
            xyz_to_rgb(multithread);
        } else if (mode == Mode::YUV) {
            xyz_to_yuv(multithread);
        } else {
            xyz_to_lab(multithread);
        }
        break;
    case Mode::YUV:
        if (mode == Mode::RGB) {
            yuv_to_rgb(multithread);
        } else if (mode == Mode::XYZ) {
            yuv_to_xyz(multithread);
        } else {
            yuv_to_lab(multithread);
        }
        break;
    case Mode::LAB:
        if (mode == Mode::RGB) {
            lab_to_rgb(multithread);
        } else if (mode == Mode::XYZ) {
            lab_to_xyz(multithread);
        } else {
            lab_to_yuv(multithread);
        }
    }

    mode_ = mode;
}

void Imagefloat::rgb_to_xyz(bool multithread)
{
    get_ws();

#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
    for (int y = 0; y < height; ++y) {
        int x = 0;
#ifdef ART_SIMD
        vfloat Xv, Yv, Zv;
        for (; x < width - 3; x += 4) {
            vfloat rv = LVF(r(y, x));
            vfloat gv = LVF(g(y, x));
            vfloat bv = LVF(b(y, x));
            Color::rgbxyz(rv, gv, bv, Xv, Yv, Zv, vws_);
            STVF(r(y, x), Xv);
            STVF(g(y, x), Yv);
            STVF(b(y, x), Zv);
        }
#endif
        for (; x < width; ++x) {
            float X, Y, Z;
            Color::rgbxyz(r(y, x), g(y, x), b(y, x), X, Y, Z, ws_);
            r(y, x) = X;
            g(y, x) = Y;
            b(y, x) = Z;
        }
    }
}

void Imagefloat::rgb_to_yuv(bool multithread)
{
    get_ws();

#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
    for (int y = 0; y < height; ++y) {
        int x = 0;
#ifdef ART_SIMD
        vfloat Yv, uv, vv;
        for (; x < width - 3; x += 4) {
            vfloat rv = LVF(r(y, x));
            vfloat gv = LVF(g(y, x));
            vfloat bv = LVF(b(y, x));
            Color::rgb2yuv(rv, gv, bv, Yv, uv, vv, vws_);
            STVF(g(y, x), Yv);
            STVF(b(y, x), uv);
            STVF(r(y, x), vv);
        }
#endif
        for (; x < width; ++x) {
            Color::rgb2yuv(r(y, x), g(y, x), b(y, x), g(y, x), b(y, x), r(y, x),
                           ws_);
        }
    }
}

void Imagefloat::xyz_to_rgb(bool multithread)
{
    get_ws();

#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
    for (int y = 0; y < height; ++y) {
        int x = 0;
#ifdef ART_SIMD
        vfloat Rv, Gv, Bv;
        for (; x < width - 3; x += 4) {
            vfloat xv = LVF(r(y, x));
            vfloat yv = LVF(g(y, x));
            vfloat zv = LVF(b(y, x));
            Color::xyz2rgb(xv, yv, zv, Rv, Gv, Bv, viws_);
            STVF(r(y, x), Rv);
            STVF(g(y, x), Gv);
            STVF(b(y, x), Bv);
        }
#endif
        for (; x < width; ++x) {
            float R, G, B;
            Color::xyz2rgb(r(y, x), g(y, x), b(y, x), R, G, B, iws_);
            r(y, x) = R;
            g(y, x) = G;
            b(y, x) = B;
        }
    }
}

void Imagefloat::xyz_to_yuv(bool multithread)
{
    get_ws();

#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
    for (int y = 0; y < height; ++y) { // TODO - SSE2 optimization
        for (int x = 0; x < width; ++x) {
            float R, G, B;
            float Y = g(y, x);
            Color::xyz2rgb(r(y, x), Y, b(y, x), R, G, B, iws_);
            r(y, x) = R - Y;
            b(y, x) = Y - B;
        }
    }
}

void Imagefloat::yuv_to_rgb(bool multithread)
{
    get_ws();

#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
    for (int y = 0; y < height; ++y) {
        int x = 0;
#ifdef ART_SIMD
        vfloat Rv, Gv, Bv;
        for (; x < width - 3; x += 4) {
            vfloat Yv = LVF(g(y, x));
            vfloat uv = LVF(b(y, x));
            vfloat vv = LVF(r(y, x));
            Color::yuv2rgb(Yv, uv, vv, Rv, Gv, Bv, vws_);
            STVF(r(y, x), Rv);
            STVF(g(y, x), Gv);
            STVF(b(y, x), Bv);
        }
#endif
        for (; x < width; ++x) {
            Color::yuv2rgb(g(y, x), b(y, x), r(y, x), r(y, x), g(y, x), b(y, x),
                           ws_);
        }
    }
}

void Imagefloat::yuv_to_xyz(bool multithread)
{
    get_ws();

#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
    for (int y = 0; y < height; ++y) { // TODO - SSE2 optimization
        for (int x = 0; x < width; ++x) {
            float R, G, B;
            Color::yuv2rgb(g(y, x), b(y, x), r(y, x), R, G, B, ws_);
            Color::rgbxyz(R, G, B, r(y, x), g(y, x), b(y, x), ws_);
        }
    }
}

void Imagefloat::toLab(LabImage &dst, bool multithread)
{
    setMode(Mode::LAB, multithread);

#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            dst.L[y][x] = g(y, x);
            dst.a[y][x] = r(y, x);
            dst.b[y][x] = b(y, x);
        }
    }
}

void Imagefloat::rgb_to_lab(bool multithread)
{
    get_ws();

#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
    for (int y = 0; y < height; ++y) {
        int x = 0;
#ifdef ART_SIMD
        vfloat Rv, Gv, Bv;
        vfloat Lv, av, bv;
        for (; x < width - 3; x += 4) {
            Rv = LVF(r(y, x));
            Gv = LVF(g(y, x));
            Bv = LVF(b(y, x));
            Color::rgb2lab(Rv, Gv, Bv, Lv, av, bv, vws_);
            STVF(g(y, x), Lv);
            STVF(r(y, x), av);
            STVF(b(y, x), bv);
        }
#endif
        for (; x < width; ++x) {
            rgb_to_lab(y, x, g(y, x), r(y, x), b(y, x));
        }
    }
}

inline void Imagefloat::rgb_to_lab(int y, int x, float &L, float &a, float &b)
{
    float X, Y, Z;
    Color::rgbxyz(this->r(y, x), this->g(y, x), this->b(y, x), X, Y, Z, ws_);
    Color::XYZ2Lab(X, Y, Z, L, a, b);
}

void Imagefloat::xyz_to_lab(bool multithread)
{
    get_ws();

#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            xyz_to_lab(y, x, g(y, x), r(y, x), b(y, x));
        }
    }
}

inline void Imagefloat::xyz_to_lab(int y, int x, float &L, float &a, float &b)
{
    Color::XYZ2Lab(this->r(y, x), this->g(y, x), this->b(y, x), L, a, b);
}

void Imagefloat::yuv_to_lab(bool multithread)
{
    get_ws();

#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
    for (int y = 0; y < height; ++y) {
        int x = 0;
#ifdef ART_SIMD
        vfloat Rv, Gv, Bv;
        vfloat Xv, Yv, Zv;
        vfloat Lv, av, bv;
        for (; x < width - 3; x += 4) {
            Rv = LVF(r(y, x));
            Gv = LVF(g(y, x));
            Bv = LVF(b(y, x));
            Color::yuv2rgb(Gv, Bv, Rv, Rv, Gv, Bv, vws_);
            Color::rgbxyz(Rv, Gv, Bv, Xv, Yv, Zv, vws_);
            Color::XYZ2Lab(Xv, Yv, Zv, Lv, av, bv);
            STVF(g(y, x), Lv);
            STVF(r(y, x), av);
            STVF(b(y, x), bv);
        }
#endif
        for (; x < width; ++x) {
            yuv_to_lab(y, x, g(y, x), r(y, x), b(y, x));
        }
    }
}

inline void Imagefloat::yuv_to_lab(int y, int x, float &L, float &a, float &b)
{
    float R, G, B;
    float X, Y, Z;
    Color::yuv2rgb(this->g(y, x), this->b(y, x), this->r(y, x), R, G, B, ws_);
    Color::rgbxyz(R, G, B, X, Y, Z, ws_);
    Color::XYZ2Lab(X, Y, Z, L, a, b);
}

void Imagefloat::lab_to_rgb(bool multithread)
{
    get_ws();

#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
    for (int y = 0; y < height; ++y) {
        // float X, Y, Z;
        int x = 0;
#ifdef ART_SIMD
        vfloat Lv, av, bv;
        vfloat Rv, Gv, Bv;
        for (; x < width - 3; x += 4) {
            Lv = LVF(g(y, x));
            av = LVF(r(y, x));
            bv = LVF(b(y, x));
            Color::lab2rgb(Lv, av, bv, Rv, Gv, Bv, viws_);
            STVF(r(y, x), Rv);
            STVF(g(y, x), Gv);
            STVF(b(y, x), Bv);
        }
#endif
        for (; x < width; ++x) {
            Color::lab2rgb(this->g(y, x), this->r(y, x), this->b(y, x),
                           this->r(y, x), this->g(y, x), this->b(y, x), iws_);
            // Color::Lab2XYZ(this->g(y, x), this->r(y, x), this->b(y, x), X, Y,
            // Z); Color::xyz2rgb(X, Y, Z, this->r(y, x), this->g(y, x),
            // this->b(y, x), iws_);
        }
    }
}

void Imagefloat::lab_to_xyz(bool multithread)
{
    get_ws();

#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Color::Lab2XYZ(this->g(y, x), this->r(y, x), this->b(y, x),
                           this->r(y, x), this->g(y, x), this->b(y, x));
        }
    }
}

void Imagefloat::lab_to_yuv(bool multithread)
{
    get_ws();

#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
    for (int y = 0; y < height; ++y) {
        float X, Y, Z;
        float R, G, B;
        int x = 0;
#ifdef ART_SIMD
        vfloat Lv, av, bv;
        vfloat Rv, Gv, Bv;
        vfloat Xv, Yv, Zv;
        for (; x < width - 3; x += 4) {
            Lv = LVF(g(y, x));
            av = LVF(r(y, x));
            bv = LVF(b(y, x));
            Color::Lab2XYZ(Lv, av, bv, Xv, Yv, Zv);
            Color::xyz2rgb(Xv, Yv, Zv, Rv, Gv, Bv, viws_);
            STVF(g(y, x), Yv);
            STVF(b(y, x), Yv - Bv);
            STVF(r(y, x), Rv - Yv);
        }
#endif
        for (; x < width; ++x) {
            Color::Lab2XYZ(this->g(y, x), this->r(y, x), this->b(y, x), X, Y,
                           Z);
            Color::xyz2rgb(X, Y, Z, R, G, B, iws_);
            this->g(y, x) = Y;
            this->b(y, x) = Y - B;
            this->r(y, x) = R - Y;
        }
    }
}

void Imagefloat::getLab(int y, int x, float &L, float &a, float &b)
{
    get_ws();
    switch (mode()) {
    case Mode::RGB:
        rgb_to_lab(y, x, L, a, b);
        break;
    case Mode::XYZ:
        xyz_to_lab(y, x, L, a, b);
        break;
    case Mode::YUV:
        yuv_to_lab(y, x, L, a, b);
        break;
    default:
        L = this->g(y, x);
        a = this->r(y, x);
        b = this->b(y, x);
        break;
    }
}

} // namespace rtengine
