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
#include "improcfun.h"
#include "rtengine.h"
#ifdef _OPENMP
#include <omp.h>
#endif
#include "../rtgui/multilangmgr.h"
#include "lensexif.h"
#include "mytime.h"
#include "opthelper.h"
#include "perspectivecorrection.h"
#include "rt_math.h"
#include "rtlensfun.h"

namespace rtengine {

namespace {

float pow3(float x) { return x * x * x; }

float pow4(float x) { return (x * x) * (x * x); }

float pown(float x, int n)
{

    switch (n) {
    case 0:
        return 1;

    case 2:
        return x * x;

    case 4:
        return pow4(x);

    case 6:
        return (x * x) * pow4(x);

    case 8:
        return pow4(x) * pow4(x);

    default:
        return pow_F(x, n);
    }
}

float normn(float a, float b, int n)
{
    switch (n) {
    case 2:
        return sqrtf(a * a + b * b);

    case 4:
        return sqrtf(sqrtf(pow4(a) + pow4(b)));

    case 6:
        return sqrtf(xcbrtf(pow3(a) * pow3(a) + pow3(b) * pow3(b)));

    case 8:
        return sqrtf(sqrtf(sqrtf(pow4(a) * pow4(a) + pow4(b) * pow4(b))));

    default:
        return pow_F(pown(a, n) + pown(b, n), 1.f / n);
    }
}

template <class T1, class T2> inline T1 CLIPTOC(T1 a, T2 b, T2 c, bool &clipped)
{
    if (a >= b) {
        if (a <= c) {
            return a;
        } else {
            clipped = true;
            return c;
        }
    } else {
        clipped = true;
        return b;
    }
}

void encode(rtengine::Imagefloat *src, rtengine::Imagefloat *dest,
            bool multiThread)
{
    LUTf lut(65536);
    for (int i = 0; i < 65536; ++i) {
        lut[i] = Color::gamma2(float(i) / 65535.f) * 65535.f;
    }

    const auto enc = [&](float f) -> float {
        if (f <= 0.f) {
            return f;
        } else if (f <= 65535.f) {
            return lut[f];
        } else {
            return Color::gamma2(f / 65535.f) * 65535.f;
        }
    };

#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic, 16) if (multiThread)
#endif
    for (int y = 0; y < src->getHeight(); ++y) {
        for (int x = 0; x < src->getWidth(); ++x) {
            dest->r(y, x) = enc(src->r(y, x));
            dest->g(y, x) = enc(src->g(y, x));
            dest->b(y, x) = enc(src->b(y, x));
        }
    }
}

void decode(rtengine::Imagefloat *img, bool multiThread)
{
    LUTf lut(65536);
    for (int i = 0; i < 65536; ++i) {
        lut[i] = Color::igamma2(float(i) / 65535.f) * 65535.f;
    }

    const auto dec = [&](float f) -> float {
        if (f <= 0.f) {
            return f;
        } else if (f <= 65535.f) {
            return lut[f];
        } else {
            return Color::igamma2(f / 65535.f) * 65535.f;
        }
    };

#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic, 16) if (multiThread)
#endif
    for (int y = 0; y < img->getHeight(); ++y) {
        for (int x = 0; x < img->getWidth(); ++x) {
            img->r(y, x) = dec(img->r(y, x));
            img->g(y, x) = dec(img->g(y, x));
            img->b(y, x) = dec(img->b(y, x));
        }
    }
}

#ifdef __SSE2__

inline void interpolateTransformCubic(rtengine::Imagefloat *src, int xs, int ys,
                                      float Dx, float Dy, float &r, float &g,
                                      float &b)
{
    constexpr float A = -0.85f;

    // Vertical
    const float t1Vert = A * (Dy - Dy * Dy);
    const float t2Vert = (3.f - 2.f * Dy) * Dy * Dy;
    const vfloat w3Vert = F2V(t1Vert * Dy);
    const vfloat w2Vert = F2V(t1Vert * Dy - t1Vert + t2Vert);
    const vfloat w1Vert = F2V(1.f - (t1Vert * Dy) - t2Vert);
    const vfloat w0Vert = F2V(t1Vert - (t1Vert * Dy));

    const vfloat rv =
        (w0Vert * LVFU(src->r(ys, xs)) + w1Vert * LVFU(src->r(ys + 1, xs))) +
        (w2Vert * LVFU(src->r(ys + 2, xs)) + w3Vert * LVFU(src->r(ys + 3, xs)));
    const vfloat gv =
        (w0Vert * LVFU(src->g(ys, xs)) + w1Vert * LVFU(src->g(ys + 1, xs))) +
        (w2Vert * LVFU(src->g(ys + 2, xs)) + w3Vert * LVFU(src->g(ys + 3, xs)));
    const vfloat bv =
        (w0Vert * LVFU(src->b(ys, xs)) + w1Vert * LVFU(src->b(ys + 1, xs))) +
        (w2Vert * LVFU(src->b(ys + 2, xs)) + w3Vert * LVFU(src->b(ys + 3, xs)));

    // Horizontal
    const float t1Hor = A * (Dx - Dx * Dx);
    const float t2Hor = (3.f - 2.f * Dx) * Dx * Dx;
    const vfloat weight =
        _mm_set_ps(t1Hor * Dx, t1Hor * Dx - t1Hor + t2Hor,
                   1.f - (t1Hor * Dx) - t2Hor, t1Hor - (t1Hor * Dx));
    r = vhadd(weight * rv);
    g = vhadd(weight * gv);
    b = vhadd(weight * bv);
}

#else // __SSE2__

inline void interpolateTransformCubic(rtengine::Imagefloat *src, int xs, int ys,
                                      float Dx, float Dy, float &r, float &g,
                                      float &b)
{
    constexpr float A = -0.85f;

    // Vertical
    const float t1Vert = A * (Dy - Dy * Dy);
    const float t2Vert = (3.f - 2.f * Dy) * Dy * Dy;
    const float w3Vert = t1Vert * Dy;
    const float w2Vert = t1Vert * Dy - t1Vert + t2Vert;
    const float w1Vert = 1.f - (t1Vert * Dy) - t2Vert;
    const float w0Vert = t1Vert - (t1Vert * Dy);

    float rv[4], gv[4], bv[4];
    for (int i = 0; i < 4; ++i) {
        rv[i] = w0Vert * src->r(ys, xs + i) + w1Vert * src->r(ys + 1, xs + i) +
                w2Vert * src->r(ys + 2, xs + i) +
                w3Vert * src->r(ys + 3, xs + i);
        gv[i] = w0Vert * src->g(ys, xs + i) + w1Vert * src->g(ys + 1, xs + i) +
                w2Vert * src->g(ys + 2, xs + i) +
                w3Vert * src->g(ys + 3, xs + i);
        bv[i] = w0Vert * src->b(ys, xs + i) + w1Vert * src->b(ys + 1, xs + i) +
                w2Vert * src->b(ys + 2, xs + i) +
                w3Vert * src->b(ys + 3, xs + i);
    }

    // Horizontal
    const float t1Hor = A * (Dx - Dx * Dx);
    const float t2Hor = (3.f - 2.f * Dx) * Dx * Dx;
    const float w3Hor = t1Hor * Dx;
    const float w2Hor = t1Hor * Dx - t1Hor + t2Hor;
    const float w1Hor = 1.f - (t1Hor * Dx) - t2Hor;
    const float w0Hor = t1Hor - (t1Hor * Dx);

    r = (rv[0] * w0Hor + rv[1] * w1Hor + rv[2] * w2Hor + rv[3] * w3Hor);
    g = (gv[0] * w0Hor + gv[1] * w1Hor + gv[2] * w2Hor + gv[3] * w3Hor);
    b = (bv[0] * w0Hor + bv[1] * w1Hor + bv[2] * w2Hor + bv[3] * w3Hor);
}

#endif // __SSE2__

#ifdef __SSE2__

inline void interpolateTransformChannelsCubic(const float *const *src, int xs,
                                              int ys, float Dx, float Dy,
                                              float &dest)
{
    constexpr float A = -0.85f;

    // Vertical
    const float t1Vert = A * (Dy - Dy * Dy);
    const float t2Vert = (3.f - 2.f * Dy) * Dy * Dy;
    const vfloat w3Vert = F2V(t1Vert * Dy);
    const vfloat w2Vert = F2V(t1Vert * Dy - t1Vert + t2Vert);
    const vfloat w1Vert = F2V(1.f - (t1Vert * Dy) - t2Vert);
    const vfloat w0Vert = F2V(t1Vert - (t1Vert * Dy));

    const vfloat cv =
        (w0Vert * LVFU(src[ys][xs]) + w1Vert * LVFU(src[ys + 1][xs])) +
        (w2Vert * LVFU(src[ys + 2][xs]) + w3Vert * LVFU(src[ys + 3][xs]));

    // Horizontal
    const float t1Hor = A * (Dx - Dx * Dx);
    const float t2Hor = (3.f - 2.f * Dx) * Dx * Dx;
    const vfloat weight =
        _mm_set_ps(t1Hor * Dx, t1Hor * Dx - t1Hor + t2Hor,
                   1.f - (t1Hor * Dx) - t2Hor, t1Hor - (t1Hor * Dx));
    dest = vhadd(weight * cv);
}

#else // __SSE2__

inline void interpolateTransformChannelsCubic(const float *const *src, int xs,
                                              int ys, float Dx, float Dy,
                                              float &dest)
{
    constexpr float A = -0.85f;

    // Vertical
    const float t1Vert = A * (Dy - Dy * Dy);
    const float t2Vert = (3.f - 2.f * Dy) * Dy * Dy;
    const float w3Vert = t1Vert * Dy;
    const float w2Vert = t1Vert * Dy - t1Vert + t2Vert;
    const float w1Vert = 1.f - (t1Vert * Dy) - t2Vert;
    const float w0Vert = t1Vert - (t1Vert * Dy);

    float cv[4];
    for (int i = 0; i < 4; ++i) {
        cv[i] = w0Vert * src[ys][xs + i] + w1Vert * src[ys + 1][xs + i] +
                w2Vert * src[ys + 2][xs + i] + w3Vert * src[ys + 3][xs + i];
    }

    // Horizontal
    const float t1Hor = A * (Dx - Dx * Dx);
    const float t2Hor = (3.f - 2.f * Dx) * Dx * Dx;
    const float w3Hor = t1Hor * Dx;
    const float w2Hor = t1Hor * Dx - t1Hor + t2Hor;
    const float w1Hor = 1.f - (t1Hor * Dx) - t2Hor;
    const float w0Hor = t1Hor - (t1Hor * Dx);

    dest = (cv[0] * w0Hor + cv[1] * w1Hor + cv[2] * w2Hor + cv[3] * w3Hor);
}

#endif // __SSE2__

void transform_perspective(const ProcParams *params,
                           const FramesMetaData *metadata, Imagefloat *orig,
                           Imagefloat *dest, int cx, int cy, int sx, int sy,
                           int oW, int oH, int fW, int fH, bool multiThread)
{
    PerspectiveCorrection pc;
    pc.init(fW, fH, params->perspective, params->commonTrans.autofill,
            metadata);
    double s = double(fW) / double(oW);

    int W = dest->getWidth();
    int H = dest->getHeight();
    int orig_W = orig->getWidth();
    int orig_H = orig->getHeight();

    constexpr float invalid = 0.f;

#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            double Dx = (x + cx) * s;
            double Dy = (y + cy) * s;
            pc(Dx, Dy);
            Dx /= s;
            Dy /= s;
            Dx -= sx;
            Dy -= sy;

            // Extract integer and fractions of source screen coordinates
            int xc = Dx;
            Dx -= xc;
            int yc = Dy;
            Dy -= yc;

            // Convert only valid pixels
            if (yc >= 0 && yc < orig_H && xc >= 0 && xc < orig_W) {
                if (yc > 0 && yc < orig_H - 2 && xc > 0 && xc < orig_W - 2) {
                    // all interpolation pixels inside image
                    interpolateTransformCubic(orig, xc - 1, yc - 1, Dx, Dy,
                                              dest->r(y, x), dest->g(y, x),
                                              dest->b(y, x));
                } else {
                    // edge pixels
                    int y1 = LIM(yc, 0, orig_H - 1);
                    int y2 = LIM(yc + 1, 0, orig_H - 1);
                    int x1 = LIM(xc, 0, orig_W - 1);
                    int x2 = LIM(xc + 1, 0, orig_W - 1);

                    dest->r(y, x) = (orig->r(y1, x1) * (1.0 - Dx) * (1.0 - Dy) +
                                     orig->r(y1, x2) * Dx * (1.0 - Dy) +
                                     orig->r(y2, x1) * (1.0 - Dx) * Dy +
                                     orig->r(y2, x2) * Dx * Dy);
                    dest->g(y, x) = (orig->g(y1, x1) * (1.0 - Dx) * (1.0 - Dy) +
                                     orig->g(y1, x2) * Dx * (1.0 - Dy) +
                                     orig->g(y2, x1) * (1.0 - Dx) * Dy +
                                     orig->g(y2, x2) * Dx * Dy);
                    dest->b(y, x) = (orig->b(y1, x1) * (1.0 - Dx) * (1.0 - Dy) +
                                     orig->b(y1, x2) * Dx * (1.0 - Dy) +
                                     orig->b(y2, x1) * (1.0 - Dx) * Dy +
                                     orig->b(y2, x2) * Dx * Dy);
                }
            } else {
                dest->r(y, x) = invalid;
                dest->g(y, x) = invalid;
                dest->b(y, x) = invalid;
            }
        }
    }
}

void get_rotation(const ProcParams *params, double &cost, double &sint)
{
    if (!params->rotate.enabled) {
        cost = 1.0;
        sint = 0.0;
    } else {
        cost = cos(params->rotate.degree * rtengine::RT_PI / 180.0);
        sint = sin(params->rotate.degree * rtengine::RT_PI / 180.0);
    }
}

} // namespace

bool ImProcFunctions::transCoord(int W, int H, const std::vector<Coord2D> &src,
                                 std::vector<Coord2D> &red,
                                 std::vector<Coord2D> &green,
                                 std::vector<Coord2D> &blue, double ascaleDef,
                                 const LensCorrection *pLCPMap)
{

    bool clipped = false;

    red.clear();
    green.clear();
    blue.clear();

    if (!needsCA() && !needsDistortion() && !needsRotation() &&
        !needsPerspective() &&
        (!params->lensProf.useDist || pLCPMap == nullptr)) {
        for (size_t i = 0; i < src.size(); i++) {
            red.push_back(Coord2D(src[i].x, src[i].y));
            green.push_back(Coord2D(src[i].x, src[i].y));
            blue.push_back(Coord2D(src[i].x, src[i].y));
        }

        return clipped;
    }

    double oW = W, oH = H;
    double w2 = (double)oW / 2.0 - 0.5;
    double h2 = (double)oH / 2.0 - 0.5;
    double maxRadius = sqrt((double)(oW * oW + oH * oH)) / 2;

    // auxiliary variables for distortion correction
    bool needsDist = needsDistortion(); // for performance
    double distAmount = params->distortion.amount;

    // auxiliary variables for rotation
    double cost, sint;
    get_rotation(params, cost, sint);

    double ascale = ascaleDef > 0 ? ascaleDef
                                  : (params->commonTrans.autofill
                                         ? getTransformAutoFill(oW, oH, pLCPMap)
                                         : 1.0);

    for (size_t i = 0; i < src.size(); i++) {
        double x_d = src[i].x, y_d = src[i].y;

        if (pLCPMap && params->lensProf.useDist) {
            pLCPMap->correctDistortion(x_d, y_d, 0, 0, ascale);
        } else {
            x_d *= ascale;
            y_d *= ascale;
        }

        x_d += ascale * (0 - w2); // centering x coord & scale
        y_d += ascale * (0 - h2); // centering y coord & scale

        // rotate
        double Dx = x_d * cost - y_d * sint;
        double Dy = x_d * sint + y_d * cost;

        // distortion correction
        double s = 1;

        if (needsDist) {
            double r = sqrt(Dx * Dx + Dy * Dy) / maxRadius; // sqrt is slow
            s = 1.0 - distAmount + distAmount * r;
        }

        // LCP CA is not reflected in preview (and very small), so don't add it
        // here

        red.push_back(Coord2D(Dx * (s + params->cacorrection.red) + w2,
                              Dy * (s + params->cacorrection.red) + h2));
        green.push_back(Coord2D(Dx * s + w2, Dy * s + h2));
        blue.push_back(Coord2D(Dx * (s + params->cacorrection.blue) + w2,
                               Dy * (s + params->cacorrection.blue) + h2));
    }

    // Clip all points and track if they were any corrections
    for (size_t i = 0; i < src.size(); i++) {
        red[i].x = CLIPTOC(red[i].x, 0, W - 1, clipped);
        red[i].y = CLIPTOC(red[i].y, 0, H - 1, clipped);
        green[i].x = CLIPTOC(green[i].x, 0, W - 1, clipped);
        green[i].y = CLIPTOC(green[i].y, 0, H - 1, clipped);
        blue[i].x = CLIPTOC(blue[i].x, 0, W - 1, clipped);
        blue[i].y = CLIPTOC(blue[i].y, 0, H - 1, clipped);
    }

    return clipped;
}

// Transform all corners and critical sidelines of an image
bool ImProcFunctions::transCoord(int W, int H, int x, int y, int w, int h,
                                 int &xv, int &yv, int &wv, int &hv,
                                 double ascaleDef,
                                 const LensCorrection *pLCPMap)
{
    const int DivisionsPerBorder = 32;

    int x1 = x, y1 = y;
    int x2 = x1 + w - 1;
    int y2 = y1 + h - 1;

    // Build all edge points and half-way points
    std::vector<Coord2D> corners(8);
    corners[0].set(x1, y1);
    corners[1].set(x1, y2);
    corners[2].set(x2, y2);
    corners[3].set(x2, y1);
    corners[4].set((x1 + x2) / 2, y1);
    corners[5].set((x1 + x2) / 2, y2);
    corners[6].set(x1, (y1 + y2) / 2);
    corners[7].set(x2, (y1 + y2) / 2);

    // Add several steps inbetween
    int xstep = (x2 - x1) / DivisionsPerBorder;

    if (xstep < 1) {
        xstep = 1;
    }

    for (int i = x1 + xstep; i <= x2 - xstep; i += xstep) {
        corners.push_back(Coord2D(i, y1));
        corners.push_back(Coord2D(i, y2));
    }

    int ystep = (y2 - y1) / DivisionsPerBorder;

    if (ystep < 1) {
        ystep = 1;
    }

    for (int i = y1 + ystep; i <= y2 - ystep; i += ystep) {
        corners.push_back(Coord2D(x1, i));
        corners.push_back(Coord2D(x2, i));
    }

    std::vector<Coord2D> r, g, b;

    bool clipped = transCoord(W, H, corners, r, g, b, ascaleDef, pLCPMap);

    // Merge all R G Bs into one X/Y pool
    std::vector<Coord2D> transCorners;
    transCorners.insert(transCorners.end(), r.begin(), r.end());
    transCorners.insert(transCorners.end(), g.begin(), g.end());
    transCorners.insert(transCorners.end(), b.begin(), b.end());

    // find the min/max of all coordinates, so the borders
    double x1d = transCorners[0].x;

    for (size_t i = 1; i < transCorners.size(); i++)
        if (transCorners[i].x < x1d) {
            x1d = transCorners[i].x;
        }

    int x1v = (int)(x1d);

    double y1d = transCorners[0].y;

    for (size_t i = 1; i < transCorners.size(); i++)
        if (transCorners[i].y < y1d) {
            y1d = transCorners[i].y;
        }

    int y1v = (int)(y1d);

    double x2d = transCorners[0].x;

    for (size_t i = 1; i < transCorners.size(); i++)
        if (transCorners[i].x > x2d) {
            x2d = transCorners[i].x;
        }

    int x2v = (int)ceil(x2d);

    double y2d = transCorners[0].y;

    for (size_t i = 1; i < transCorners.size(); i++)
        if (transCorners[i].y > y2d) {
            y2d = transCorners[i].y;
        }

    int y2v = (int)ceil(y2d);

    xv = x1v;
    yv = y1v;
    wv = x2v - x1v + 1;
    hv = y2v - y1v + 1;

    return clipped;
}

void ImProcFunctions::transform(Imagefloat *original, Imagefloat *transformed,
                                int cx, int cy, int sx, int sy, int oW, int oH,
                                int fW, int fH, const FramesMetaData *metadata,
                                int rawRotationDeg, bool highQuality)
{
    double focalLen = metadata->getFocalLen();
    double focalLen35mm = metadata->getFocalLen35mm();
    float focusDist = metadata->getFocusDist();
    double fNumber = metadata->getFNumber();

    std::unique_ptr<const LensCorrection> pLCPMap;

    if (params->lensProf.useExif()) {
        auto corr = new ExifLensCorrection(metadata, oW, oH, params->coarse,
                                           rawRotationDeg);
        pLCPMap.reset(corr);
        if (!corr->ok()) {
            LensProfParams lf;
            lf.lcMode = LensProfParams::LcMode::LENSFUNAUTOMATCH;
            pLCPMap = LFDatabase::getInstance()->findModifier(
                lf, metadata, oW, oH, params->coarse, rawRotationDeg);
        }
    } else if (needsLensfun()) {
        pLCPMap = LFDatabase::getInstance()->findModifier(
            params->lensProf, metadata, oW, oH, params->coarse, rawRotationDeg);
    } else if (needsLCP()) { // don't check focal length to allow distortion
                             // correction for lenses without chip
        const std::shared_ptr<LCPProfile> pLCPProf =
            LCPStore::getInstance()->getProfile(params->lensProf.lcpFile);

        if (pLCPProf) {
            pLCPMap.reset(new LCPMapper(pLCPProf, focalLen, focalLen35mm,
                                        focusDist, fNumber, false, false, oW,
                                        oH, params->coarse, rawRotationDeg));
        } else if (!params->lensProf.lcpFile.empty() && plistener) {
            plistener->error(Glib::ustring::compose(M("ERROR_MSG_FILE_READ"),
                                                    params->lensProf.lcpFile));
        }
    }

    if (needsCA() || scale == 1) {
        highQuality = true;
    }
    const bool needs_dist_rot_ca = needsCA() || needsDistortion() ||
                                   needsRotation() || params->lensProf.needed();
    const bool needs_luminance = needsVignetting();
    const bool needs_lcp_ca = highQuality && pLCPMap &&
                              params->lensProf.useCA &&
                              pLCPMap->isCACorrectionAvailable();
    const bool needs_perspective = needsPerspective() || needs_lcp_ca;
    const bool needs_transform_general = needs_dist_rot_ca || needs_luminance;
    const bool do_encode = highQuality && (needs_transform_general ||
                                           needs_perspective || needs_lcp_ca);

    if (!(needs_dist_rot_ca || needs_perspective) && needs_luminance) {
        transformLuminanceOnly(original, transformed, cx, cy, oW, oH, fW, fH,
                               false);
    } else {
        std::unique_ptr<Imagefloat> logimg;
        if (do_encode) {
            logimg.reset(
                new Imagefloat(original->getWidth(), original->getHeight()));
            if (needs_luminance) {
                transformLuminanceOnly(original, logimg.get(), cx, cy, oW, oH,
                                       fW, fH, false);
                original = logimg.get();
            }
            encode(original, logimg.get(), multiThread);
            original = logimg.get();
        }

        std::unique_ptr<Imagefloat> tmpimg;
        Imagefloat *dest = transformed;
        int dest_x = cx, dest_y = cy;

        if (needs_lcp_ca || needs_perspective) {
            tmpimg.reset(
                new Imagefloat(original->getWidth(), original->getHeight()));
            dest = tmpimg.get();
            dest_x = sx;
            dest_y = sy;
        }

        if (needs_transform_general) {
            transformGeneral(highQuality, original, dest, dest_x, dest_y, sx,
                             sy, oW, oH, fW, fH, pLCPMap.get());
        } else {
            dest = original;
        }

        if (needs_lcp_ca) {
            Imagefloat *out = transformed;
            dest_x = cx;
            dest_y = cy;
            if (needs_perspective) {
                out = new Imagefloat(dest->getWidth(), dest->getHeight());
                dest_x = sx;
                dest_y = sy;
            }
            transformLCPCAOnly(dest, out, dest_x, dest_y, pLCPMap.get());
            if (needs_perspective) {
                tmpimg.reset(out);
                dest = out;
            }
        }
        if (needs_perspective) {
            transform_perspective(params, metadata, dest, transformed, cx, cy,
                                  sx, sy, oW, oH, fW, fH, multiThread);
        }

        if (do_encode) {
            decode(transformed, multiThread);
        }
    }
}

// helper function
namespace {

void calcVignettingParams(int oW, int oH, const VignettingParams &vignetting,
                          double &w2, double &h2, double &maxRadius, double &v,
                          double &b, double &mul)
{
    // vignette center is a point with coordinates between -1 and +1
    double x = vignetting.centerX / 100.0;
    double y = vignetting.centerY / 100.0;

    // calculate vignette center in pixels
    w2 = (double)oW / 2.0 - 0.5 + x * oW;
    h2 = (double)oH / 2.0 - 0.5 + y * oH;

    // max vignette radius in pixels
    maxRadius = sqrt((double)(oW * oW + oH * oH)) / 2.;

    // vignette variables with applied strength
    v = 1.0 + vignetting.strength * fabs(vignetting.amount) * 3.0 / 400.0;
    b = 1.0 + vignetting.radius * 7.0 / 100.0;
    mul = (1.0 - v) / tanh(b);
}

struct grad_params {
    bool angle_is_zero, transpose, bright_top;
    float ta, yc, xc;
    float ys, ys_inv;
    float scale, botmul, topmul;
    float top_edge_0;
    int h;
};

void calcGradientParams(int oW, int oH, const GradientParams &gradient,
                        struct grad_params &gp)
{
    int w = oW;
    int h = oH;
    double gradient_stops = gradient.strength;
    double gradient_span = gradient.feather / 100.0;
    double gradient_center_x = gradient.centerX / 200.0 + 0.5;
    double gradient_center_y = gradient.centerY / 200.0 + 0.5;
    double gradient_angle = gradient.degree / 180.0 * rtengine::RT_PI;
    // fprintf(stderr, "%f %f %f %f %f %d %d\n", gradient_stops, gradient_span,
    // gradient_center_x, gradient_center_y, gradient_angle, w, h);

    // make 0.0 <= gradient_angle < 2 * rtengine::RT_PI
    gradient_angle = fmod(gradient_angle, 2 * rtengine::RT_PI);

    if (gradient_angle < 0.0) {
        gradient_angle += 2.0 * rtengine::RT_PI;
    }

    gp.bright_top = false;
    gp.transpose = false;
    gp.angle_is_zero = false;
    gp.h = h;
    double cosgrad = cos(gradient_angle);

    if (fabs(cosgrad) < 0.707) {
        // we transpose to avoid division by zero at 90 degrees
        // (actually we could transpose only for 90 degrees, but this way we
        // avoid division with extremely small numbers
        gp.transpose = true;
        gradient_angle += 0.5 * rtengine::RT_PI;
        double gxc = gradient_center_x;
        gradient_center_x = 1.0 - gradient_center_y;
        gradient_center_y = gxc;
    }

    gradient_angle = fmod(gradient_angle, 2 * rtengine::RT_PI);

    if (gradient_angle > 0.5 * rtengine::RT_PI &&
        gradient_angle < rtengine::RT_PI) {
        gradient_angle += rtengine::RT_PI;
        gp.bright_top = true;
    } else if (gradient_angle >= rtengine::RT_PI &&
               gradient_angle < 1.5 * rtengine::RT_PI) {
        gradient_angle -= rtengine::RT_PI;
        gp.bright_top = true;
    }

    if (fabs(gradient_angle) < 0.001 ||
        fabs(gradient_angle - 2 * rtengine::RT_PI) < 0.001) {
        gradient_angle = 0;
        gp.angle_is_zero = true;
    }

    if (gp.transpose) {
        gp.bright_top = !gp.bright_top;
    }

    if (gp.transpose) {
        int tmp = w;
        w = h;
        h = tmp;
    }

    gp.scale = 1.0 / pow(2, gradient_stops);

    if (gp.bright_top) {
        gp.topmul = 1.0;
        gp.botmul = gp.scale;
    } else {
        gp.topmul = gp.scale;
        gp.botmul = 1.0;
    }

    gp.ta = tan(gradient_angle);
    gp.xc = w * gradient_center_x;
    gp.yc = h * gradient_center_y;
    gp.ys = sqrt((float)h * h + (float)w * w) *
            (gradient_span / cos(gradient_angle));
    gp.ys_inv = 1.0 / gp.ys;
    gp.top_edge_0 = gp.yc - gp.ys / 2.0;

    if (gp.ys < 1.0 / h) {
        gp.ys_inv = 0;
        gp.ys = 0;
    }
}

float calcGradientFactor(const struct grad_params &gp, int x, int y)
{
    if (gp.angle_is_zero) {
        int gy = gp.transpose ? x : y;

        if (gy < gp.top_edge_0) {
            return gp.topmul;
        } else if (gy >= gp.top_edge_0 + gp.ys) {
            return gp.botmul;
        } else {
            float val = ((float)(gy - gp.top_edge_0) * gp.ys_inv);

            if (gp.bright_top) {
                val = 1.f - val;
            }

            val *= rtengine::RT_PI_F_2;

            if (gp.scale < 1.f) {
                val = pow3(xsinf(val));
            } else {
                val = 1.f - pow3(xcosf(val));
            }

            return gp.scale + val * (1.0 - gp.scale);
        }
    } else {
        int gy = gp.transpose ? x : y;
        int gx = gp.transpose ? gp.h - y - 1 : x;
        float top_edge = gp.top_edge_0 - gp.ta * (gx - gp.xc);

        if (gy < top_edge) {
            return gp.topmul;
        } else if (gy >= top_edge + gp.ys) {
            return gp.botmul;
        } else {
            float val = ((float)(gy - top_edge) * gp.ys_inv);

            val = gp.bright_top ? 1.f - val : val;

            val *= rtengine::RT_PI_F_2;

            if (gp.scale < 1.f) {
                val = pow3(xsinf(val));
            } else {
                val = 1.f - pow3(xcosf(val));
            }

            return gp.scale + val * (1.0 - gp.scale);
        }
    }
}

struct pcv_params {
    float oe_a, oe_b, oe1_a, oe1_b, oe2_a, oe2_b;
    float ie_mul, ie1_mul, ie2_mul;
    float sepmix, feather;
    int x1, x2, y1, y2;
    int ex, ey, ew, eh;
    int sep;
    bool is_super_ellipse_mode, is_portrait;
    float scale;
    float fadeout_mul;
};

void calcPCVignetteParams(int fW, int fH, int oW, int oH,
                          const PCVignetteParams &pcvignette,
                          const CropParams &crop, pcv_params &pcv)
{

    // ellipse formula: (x/a)^2 + (y/b)^2 = 1
    double roundness = pcvignette.roundness / 100.0;
    pcv.feather = pcvignette.feather / 100.0;

    if (crop.enabled) {
        float sW = float(oW) / fW;
        float sH = float(oH) / fH;
        pcv.ew = crop.w * sW;
        pcv.eh = crop.h * sH;
        pcv.x1 = crop.x * sW;
        pcv.y1 = crop.y * sH;
    } else {
        pcv.ew = oW;
        pcv.eh = oH;
        pcv.x1 = 0;
        pcv.y1 = 0;
    }
    float dW = pcvignette.centerX / 200.f * pcv.ew;
    float dH = pcvignette.centerY / 200.f * pcv.eh;
    pcv.ex = pcv.x1 + dW;
    pcv.ey = pcv.y1 + dH;
    pcv.x2 = pcv.x1 + pcv.ew + std::abs(dW);
    pcv.y2 = pcv.y1 + pcv.eh + std::abs(dH);

    pcv.fadeout_mul = 1.0 / (0.05 * sqrtf(oW * oW + oH * oH));
    float short_side = (pcv.ew < pcv.eh) ? pcv.ew : pcv.eh;
    float long_side = (pcv.ew > pcv.eh) ? pcv.ew : pcv.eh;

    pcv.sep = 2;
    pcv.sepmix = 0;
    pcv.oe_a = sqrt(2.0) * long_side * 0.5;
    pcv.oe_b = pcv.oe_a * short_side / long_side;
    pcv.ie_mul = (1.0 / sqrt(2.0)) * (1.0 - pcv.feather);
    pcv.is_super_ellipse_mode = false;
    pcv.is_portrait = (pcv.ew < pcv.eh);

    if (roundness < 0.5) {
        // make super-ellipse of higher and higher degree
        pcv.is_super_ellipse_mode = true;
        float sepf = 2 + 4 * powf(1.0 - 2 * roundness,
                                  1.3); // gamma 1.3 used to balance the effect
                                        // in the 0.0...0.5 roundness range
        pcv.sep = ((int)sepf) & ~0x1;
        pcv.sepmix = (sepf - pcv.sep) * 0.5; // 0.0 to 1.0
        pcv.oe1_a = powf(2.0, 1.0 / pcv.sep) * long_side * 0.5;
        pcv.oe1_b = pcv.oe1_a * short_side / long_side;
        pcv.ie1_mul = (1.0 / powf(2.0, 1.0 / pcv.sep)) * (1.0 - pcv.feather);
        pcv.oe2_a = powf(2.0, 1.0 / (pcv.sep + 2)) * long_side * 0.5;
        pcv.oe2_b = pcv.oe2_a * short_side / long_side;
        pcv.ie2_mul =
            (1.0 / powf(2.0, 1.0 / (pcv.sep + 2))) * (1.0 - pcv.feather);
    }

    if (roundness > 0.5) {
        // scale from fitted ellipse towards circle
        float rad = sqrtf(pcv.ew * pcv.ew + pcv.eh * pcv.eh) / 2.0;
        float diff_a = rad - pcv.oe_a;
        float diff_b = rad - pcv.oe_b;
        pcv.oe_a = pcv.oe_a + diff_a * 2 * (roundness - 0.5);
        pcv.oe_b = pcv.oe_b + diff_b * 2 * (roundness - 0.5);
    }

    pcv.scale = powf(2, -pcvignette.strength);

    if (pcvignette.strength >= 6.0) {
        pcv.scale = 0.0;
    }
}

float calcPCVignetteFactor(const pcv_params &pcv, int x, int y)
{

    float fo = 1.f;

    if (x < pcv.x1 || x > pcv.x2 || y < pcv.y1 || y > pcv.y2) {
        /*
          The initial plan was to have 1.0 directly outside the crop box (ie no
          fading), but due to rounding/trunction here and there I didn't succeed
          matching up exactly on the pixel with the crop box. To hide that
          mismatch I made a fade.
         */
        int dist_x = (x < pcv.x1) ? pcv.x1 - x : x - pcv.x2;
        int dist_y = (y < pcv.y1) ? pcv.y1 - y : y - pcv.y2;

        if (dist_x < 0) {
            dist_x = 0;
        }

        if (dist_y < 0) {
            dist_y = 0;
        }

        fo = sqrtf(dist_x * dist_x + dist_y * dist_y) * pcv.fadeout_mul;

        if (fo >= 1.f) {
            return 1.f;
        }
    }

    float a = fabs((x - pcv.ex) - pcv.ew * 0.5f);
    float b = fabs((y - pcv.ey) - pcv.eh * 0.5f);

    if (pcv.is_portrait) {
        std::swap(a, b);
    }

    float dist = normn(a, b, 2);
    float dist_oe, dist_ie;
    float2 sincosval;

    if (dist == 0.0f) {
        sincosval.y = 1.0f; // cos
        sincosval.x = 0.0f; // sin
    } else {
        sincosval.y = a / dist; // cos
        sincosval.x = b / dist; // sin
    }

    if (pcv.is_super_ellipse_mode) {
        float dist_oe1 =
            pcv.oe1_a * pcv.oe1_b /
            normn(pcv.oe1_b * sincosval.y, pcv.oe1_a * sincosval.x, pcv.sep);
        float dist_oe2 = pcv.oe2_a * pcv.oe2_b /
                         normn(pcv.oe2_b * sincosval.y, pcv.oe2_a * sincosval.x,
                               pcv.sep + 2);
        float dist_ie1 = pcv.ie1_mul * dist_oe1;
        float dist_ie2 = pcv.ie2_mul * dist_oe2;
        dist_oe = dist_oe1 * (1.f - pcv.sepmix) + dist_oe2 * pcv.sepmix;
        dist_ie = dist_ie1 * (1.f - pcv.sepmix) + dist_ie2 * pcv.sepmix;
    } else {
        dist_oe =
            pcv.oe_a * pcv.oe_b /
            sqrtf(SQR(pcv.oe_b * sincosval.y) + SQR(pcv.oe_a * sincosval.x));
        dist_ie = pcv.ie_mul * dist_oe;
    }

    if (dist <= dist_ie) {
        return 1.f;
    }

    float val;

    if (dist >= dist_oe) {
        val = pcv.scale;
    } else {
        val = rtengine::RT_PI_F_2 * (dist - dist_ie) / (dist_oe - dist_ie);

        if (pcv.scale < 1.f) {
            val = pow4(xcosf(val));
        } else {
            val = 1 - pow4(xsinf(val));
        }

        val = pcv.scale + val * (1.f - pcv.scale);
    }

    if (fo < 1.f) {
        val = fo + val * (1.f - fo);
    }

    return val;
}

} // namespace

void ImProcFunctions::transformLuminanceOnly(Imagefloat *original,
                                             Imagefloat *transformed, int cx,
                                             int cy, int oW, int oH, int fW,
                                             int fH, bool creative)
{

    const bool applyVignetting = !creative && needsVignetting();
    const bool applyGradient = creative && needsGradient();
    const bool applyPCVignetting = creative && needsPCVignetting();

    double vig_w2, vig_h2, maxRadius, v, b, mul;

    if (applyVignetting) {
        calcVignettingParams(oW, oH, params->vignetting, vig_w2, vig_h2,
                             maxRadius, v, b, mul);
    }

    struct grad_params gp;

    if (applyGradient) {
        calcGradientParams(oW, oH, params->gradient, gp);
    }

    struct pcv_params pcv;

    if (applyPCVignetting) {
        // fprintf(stderr, "%d %d | %d %d | %d %d | %d %d [%d %d]\n", fW, fH,
        // oW, oH, transformed->getWidth(), transformed->getHeight(), cx, cy,
        // params->crop.w, params->crop.h);
        calcPCVignetteParams(fW, fH, oW, oH, params->pcvignette, params->crop,
                             pcv);
    }

    bool darkening = (params->vignetting.amount <= 0.0);
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic, 16) if (multiThread)
#endif

    for (int y = 0; y < transformed->getHeight(); y++) {
        double vig_y_d = applyVignetting ? (double)(y + cy) - vig_h2 : 0.0;

        for (int x = 0; x < transformed->getWidth(); x++) {
            double factor = 1.0;

            if (applyVignetting) {
                double vig_x_d = (double)(x + cx) - vig_w2;
                double r = sqrt(vig_x_d * vig_x_d + vig_y_d * vig_y_d);

                if (darkening) {
                    factor /= std::max(
                        v + mul * tanh(b * (maxRadius - r) / maxRadius), 0.001);
                } else {
                    factor = v + mul * tanh(b * (maxRadius - r) / maxRadius);
                }
            }

            if (applyGradient) {
                factor *= calcGradientFactor(gp, cx + x, cy + y);
            }

            if (applyPCVignetting) {
                factor *= calcPCVignetteFactor(pcv, cx + x, cy + y);
            }

            transformed->r(y, x) = original->r(y, x) * factor;
            transformed->g(y, x) = original->g(y, x) * factor;
            transformed->b(y, x) = original->b(y, x) * factor;
        }
    }
}

void ImProcFunctions::transformGeneral(bool highQuality, Imagefloat *original,
                                       Imagefloat *transformed, int cx, int cy,
                                       int sx, int sy, int oW, int oH, int fW,
                                       int fH, const LensCorrection *pLCPMap)
{
    // set up stuff, depending on the mode we are
    bool enableLCPDist = pLCPMap && params->lensProf.useDist;
    const bool enableCA = highQuality && needsCA();
    constexpr bool enableGradient = false;
    constexpr bool enablePCVignetting = false;
    bool enableVignetting = !highQuality && needsVignetting();
    bool enableDistortion = needsDistortion();

    double w2 = (double)oW / 2.0 - 0.5;
    double h2 = (double)oH / 2.0 - 0.5;

    double vig_w2, vig_h2, maxRadius, v, b, mul;
    calcVignettingParams(oW, oH, params->vignetting, vig_w2, vig_h2, maxRadius,
                         v, b, mul);

    struct grad_params gp;

    if (enableGradient) {
        calcGradientParams(oW, oH, params->gradient, gp);
    }

    struct pcv_params pcv;

    if (enablePCVignetting) {
        calcPCVignetteParams(fW, fH, oW, oH, params->pcvignette, params->crop,
                             pcv);
    }

    float **chOrig[3];
    chOrig[0] = original->r.ptrs;
    chOrig[1] = original->g.ptrs;
    chOrig[2] = original->b.ptrs;

    float **chTrans[3];
    chTrans[0] = transformed->r.ptrs;
    chTrans[1] = transformed->g.ptrs;
    chTrans[2] = transformed->b.ptrs;

    // auxiliary variables for c/a correction
    double chDist[3];
    chDist[0] = enableCA ? params->cacorrection.red : 0.0;
    chDist[1] = 0.0;
    chDist[2] = enableCA ? params->cacorrection.blue : 0.0;

    // auxiliary variables for distortion correction
    double distAmount = params->distortion.amount;

    // auxiliary variables for rotation
    double cost, sint;
    get_rotation(params, cost, sint);

    double ascale = params->commonTrans.autofill
                        ? getTransformAutoFill(oW, oH, pLCPMap)
                        : 1.0;

    const bool use_enc = highQuality;
    constexpr float invalid = 0.f;

#if defined(__GNUC__) && __GNUC__ >= 7 // silence warning
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif
#if defined(__GNUC__) && __GNUC__ >= 7
#pragma GCC diagnostic pop
#endif
    // main cycle
    bool darkening = (params->vignetting.amount <= 0.0);
#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif

    for (int y = 0; y < transformed->getHeight(); y++) {
        for (int x = 0; x < transformed->getWidth(); x++) {
            double x_d = x, y_d = y;

            if (enableLCPDist) {
                pLCPMap->correctDistortion(x_d, y_d, cx, cy,
                                           ascale); // must be first transform
            } else {
                x_d *= ascale;
                y_d *= ascale;
            }

            x_d += ascale * (cx - w2); // centering x coord & scale
            y_d += ascale * (cy - h2); // centering y coord & scale

            double vig_x_d = 0., vig_y_d = 0.;

            if (enableVignetting) {
                vig_x_d =
                    ascale * (x + cx - vig_w2); // centering x coord & scale
                vig_y_d =
                    ascale * (y + cy - vig_h2); // centering y coord & scale
            }

            // rotate
            double Dxc = x_d * cost - y_d * sint;
            double Dyc = x_d * sint + y_d * cost;

            // distortion correction
            double s = 1;

            if (enableDistortion) {
                double r =
                    sqrt(Dxc * Dxc + Dyc * Dyc) / maxRadius; // sqrt is slow
                s = 1.0 - distAmount + distAmount * r;
            }

            double r2 = 0.;

            if (enableVignetting) {
                double vig_Dx = vig_x_d * cost - vig_y_d * sint;
                double vig_Dy = vig_x_d * sint + vig_y_d * cost;
                r2 = sqrt(vig_Dx * vig_Dx + vig_Dy * vig_Dy);
            }

            for (int c = 0; c < (enableCA ? 3 : 1); c++) {
                double Dx = Dxc * (s + chDist[c]);
                double Dy = Dyc * (s + chDist[c]);

                // de-center
                Dx += w2;
                Dy += h2;

                // Extract integer and fractions of source screen coordinates
                int xc = (int)Dx;
                Dx -= (double)xc;
                xc -= sx;
                int yc = (int)Dy;
                Dy -= (double)yc;
                yc -= sy;

                // Convert only valid pixels
                if (yc >= 0 && yc < original->getHeight() && xc >= 0 &&
                    xc < original->getWidth()) {

                    // multiplier for vignetting correction
                    double vignmul = 1.0;

                    if (enableVignetting) {
                        if (darkening) {
                            vignmul /= std::max(
                                v + mul * tanh(b * (maxRadius - s * r2) /
                                               maxRadius),
                                0.001);
                        } else {
                            vignmul *=
                                (v + mul * tanh(b * (maxRadius - s * r2) /
                                                maxRadius));
                        }
                    }

                    if (enableGradient) {
                        vignmul *= calcGradientFactor(gp, cx + x, cy + y);
                    }

                    if (enablePCVignetting) {
                        vignmul *= calcPCVignetteFactor(pcv, cx + x, cy + y);
                    }

                    if (use_enc) {
                        vignmul = vignmul > 0.f ? xcbrtf(vignmul) : 0.f;
                    }

                    if (yc > 0 && yc < original->getHeight() - 2 && xc > 0 &&
                        xc < original->getWidth() - 2) {
                        // all interpolation pixels inside image
                        if (!highQuality) {
                            transformed->r(y, x) =
                                vignmul *
                                (original->r(yc, xc) * (1.0 - Dx) * (1.0 - Dy) +
                                 original->r(yc, xc + 1) * Dx * (1.0 - Dy) +
                                 original->r(yc + 1, xc) * (1.0 - Dx) * Dy +
                                 original->r(yc + 1, xc + 1) * Dx * Dy);
                            transformed->g(y, x) =
                                vignmul *
                                (original->g(yc, xc) * (1.0 - Dx) * (1.0 - Dy) +
                                 original->g(yc, xc + 1) * Dx * (1.0 - Dy) +
                                 original->g(yc + 1, xc) * (1.0 - Dx) * Dy +
                                 original->g(yc + 1, xc + 1) * Dx * Dy);
                            transformed->b(y, x) =
                                vignmul *
                                (original->b(yc, xc) * (1.0 - Dx) * (1.0 - Dy) +
                                 original->b(yc, xc + 1) * Dx * (1.0 - Dy) +
                                 original->b(yc + 1, xc) * (1.0 - Dx) * Dy +
                                 original->b(yc + 1, xc + 1) * Dx * Dy);
                        } else if (enableCA) {
                            interpolateTransformChannelsCubic(chOrig[c], xc - 1,
                                                              yc - 1, Dx, Dy,
                                                              chTrans[c][y][x]);
                            chTrans[c][y][x] *= vignmul;
                        } else {
                            interpolateTransformCubic(
                                original, xc - 1, yc - 1, Dx, Dy,
                                transformed->r(y, x), transformed->g(y, x),
                                transformed->b(y, x));
                            transformed->r(y, x) *= vignmul;
                            transformed->g(y, x) *= vignmul;
                            transformed->b(y, x) *= vignmul;
                        }
                    } else {
                        // edge pixels
                        int y1 = LIM(yc, 0, original->getHeight() - 1);
                        int y2 = LIM(yc + 1, 0, original->getHeight() - 1);
                        int x1 = LIM(xc, 0, original->getWidth() - 1);
                        int x2 = LIM(xc + 1, 0, original->getWidth() - 1);

                        if (enableCA) {
                            chTrans[c][y][x] =
                                vignmul *
                                (chOrig[c][y1][x1] * (1.0 - Dx) * (1.0 - Dy) +
                                 chOrig[c][y1][x2] * Dx * (1.0 - Dy) +
                                 chOrig[c][y2][x1] * (1.0 - Dx) * Dy +
                                 chOrig[c][y2][x2] * Dx * Dy);
                        } else {
                            transformed->r(y, x) =
                                (original->r(y1, x1) * (1.0 - Dx) * (1.0 - Dy) +
                                 original->r(y1, x2) * Dx * (1.0 - Dy) +
                                 original->r(y2, x1) * (1.0 - Dx) * Dy +
                                 original->r(y2, x2) * Dx * Dy);
                            transformed->g(y, x) =
                                (original->g(y1, x1) * (1.0 - Dx) * (1.0 - Dy) +
                                 original->g(y1, x2) * Dx * (1.0 - Dy) +
                                 original->g(y2, x1) * (1.0 - Dx) * Dy +
                                 original->g(y2, x2) * Dx * Dy);
                            transformed->b(y, x) =
                                (original->b(y1, x1) * (1.0 - Dx) * (1.0 - Dy) +
                                 original->b(y1, x2) * Dx * (1.0 - Dy) +
                                 original->b(y2, x1) * (1.0 - Dx) * Dy +
                                 original->b(y2, x2) * Dx * Dy);
                            transformed->r(y, x) *= vignmul;
                            transformed->g(y, x) *= vignmul;
                            transformed->b(y, x) *= vignmul;
                        }
                    }
                } else {
                    if (enableCA) {
                        // not valid (source pixel x,y not inside source image,
                        // etc.)
                        chTrans[c][y][x] = invalid;
                    } else {
                        transformed->r(y, x) = invalid;
                        transformed->g(y, x) = invalid;
                        transformed->b(y, x) = invalid;
                    }
                }
            }
        }
    }
}

void ImProcFunctions::transformLCPCAOnly(Imagefloat *original,
                                         Imagefloat *transformed, int cx,
                                         int cy, const LensCorrection *pLCPMap)
{
    assert(pLCPMap && params->lensProf.useCA &&
           pLCPMap->isCACorrectionAvailable());

    float **chOrig[3];
    chOrig[0] = original->r.ptrs;
    chOrig[1] = original->g.ptrs;
    chOrig[2] = original->b.ptrs;

    float **chTrans[3];
    chTrans[0] = transformed->r.ptrs;
    chTrans[1] = transformed->g.ptrs;
    chTrans[2] = transformed->b.ptrs;

#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif

    for (int y = 0; y < transformed->getHeight(); y++) {
        for (int x = 0; x < transformed->getWidth(); x++) {
            for (int c = 0; c < 3; c++) {
                double Dx = x;
                double Dy = y;

                pLCPMap->correctCA(Dx, Dy, cx, cy, c);

                // Extract integer and fractions of coordinates
                int xc = (int)Dx;
                Dx -= (double)xc;
                int yc = (int)Dy;
                Dy -= (double)yc;

                // Convert only valid pixels
                if (yc >= 0 && yc < original->getHeight() && xc >= 0 &&
                    xc < original->getWidth()) {

                    // multiplier for vignetting correction
                    if (yc > 0 && yc < original->getHeight() - 2 && xc > 0 &&
                        xc < original->getWidth() - 2) {
                        // all interpolation pixels inside image
                        interpolateTransformChannelsCubic(chOrig[c], xc - 1,
                                                          yc - 1, Dx, Dy,
                                                          chTrans[c][y][x]);
                    } else {
                        // edge pixels
                        int y1 = LIM(yc, 0, original->getHeight() - 1);
                        int y2 = LIM(yc + 1, 0, original->getHeight() - 1);
                        int x1 = LIM(xc, 0, original->getWidth() - 1);
                        int x2 = LIM(xc + 1, 0, original->getWidth() - 1);

                        chTrans[c][y][x] =
                            chOrig[c][y1][x1] * (1.0 - Dx) * (1.0 - Dy) +
                            chOrig[c][y1][x2] * Dx * (1.0 - Dy) +
                            chOrig[c][y2][x1] * (1.0 - Dx) * Dy +
                            chOrig[c][y2][x2] * Dx * Dy;
                    }
                } else {
                    // not valid (source pixel x,y not inside source image,
                    // etc.)
                    chTrans[c][y][x] = -1.f;
                }
            }
        }
    }
}

double ImProcFunctions::getTransformAutoFill(int oW, int oH,
                                             const LensCorrection *pLCPMap)
{
    if (!needsCA() && !needsDistortion() && !needsRotation() &&
        !needsPerspective() &&
        (!params->lensProf.useDist || pLCPMap == nullptr)) {
        return 1;
    }

    double scaleU = 2,
           scaleL = 0.001; // upper and lower border, iterate inbetween

    do {
        double scale = (scaleU + scaleL) * 0.5;

        int orx, ory, orw, orh;
        bool clipped = transCoord(oW, oH, 0, 0, oW, oH, orx, ory, orw, orh,
                                  scale, pLCPMap);

        if (clipped) {
            scaleU = scale;
        } else {
            scaleL = scale;
        }
    } while (scaleU - scaleL > 0.001);

    return scaleL;
}

bool ImProcFunctions::needsCA()
{
    return params->cacorrection.enabled &&
           (fabs(params->cacorrection.red) > 1e-15 ||
            fabs(params->cacorrection.blue) > 1e-15);
}

bool ImProcFunctions::needsDistortion()
{
    return params->distortion.enabled &&
           (fabs(params->distortion.amount) > 1e-15);
}

bool ImProcFunctions::needsRotation()
{
    return params->rotate.enabled && (fabs(params->rotate.degree) > 1e-15);
}

bool ImProcFunctions::needsPerspective()
{
    return params->perspective.enabled &&
           (params->perspective.horizontal || params->perspective.vertical ||
            params->perspective.angle || params->perspective.shear);
}

bool ImProcFunctions::needsGradient()
{
    return params->gradient.enabled && fabs(params->gradient.strength) > 1e-15;
}

bool ImProcFunctions::needsPCVignetting()
{
    return params->pcvignette.enabled &&
           fabs(params->pcvignette.strength) > 1e-15;
}

bool ImProcFunctions::needsVignetting()
{
    return params->vignetting.enabled && params->vignetting.amount;
}

bool ImProcFunctions::needsLCP() { return params->lensProf.useLcp(); }

bool ImProcFunctions::needsLensfun() { return params->lensProf.useLensfun(); }

bool ImProcFunctions::needsTransform()
{
    return needsCA() || needsDistortion() || needsRotation() ||
           needsPerspective() || needsVignetting() || params->lensProf.needed();
}

bool ImProcFunctions::needsLuminanceOnly()
{
    return !(needsCA() || needsDistortion() || needsRotation() ||
             needsPerspective() || params->lensProf.needed()) &&
           needsVignetting();
}

void ImProcFunctions::creativeGradients(Imagefloat *img)
{
    if (needsGradient() || needsPCVignetting()) {
        img->setMode(Imagefloat::Mode::RGB, multiThread);
        int cx = offset_x;
        int cy = offset_y;
        int cw, ch;
        if (full_width < 0) {
            cw = img->getWidth();
            ch = img->getHeight();
        } else {
            cw = full_width;
            ch = full_height;
        }
        int fw = cw * scale;
        int fh = ch * scale;
        transformLuminanceOnly(img, img, cx, cy, cw, ch, fw, fh, true);
    }
}

} // namespace rtengine
