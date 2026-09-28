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

#pragma once

#include "array2D.h"
#include "opthelper.h"

namespace rtengine {

inline float getBilinearValue(const array2D<float> &src, float x, float y)
{
    const int W = src.width();
    const int H = src.height();

    // Get integer and fractional parts of numbers
    int xi = std::min(int(x), W - 1);
    int yi = std::min(int(y), H - 1);
    float xf = x - xi;
    float yf = y - yi;
    int xi1 = std::min(xi + 1, W - 1);
    int yi1 = std::min(yi + 1, H - 1);

    float bl = src[yi][xi];
    float br = src[yi][xi1];
    float tl = src[yi1][xi];
    float tr = src[yi1][xi1];

    // interpolate
    float b = xf * br + (1.f - xf) * bl;
    float t = xf * tr + (1.f - xf) * tl;
    float pxf = yf * t + (1.f - yf) * b;
    return pxf;
}

#ifdef ART_SIMD

// Same as getBilinearValue, for 4 consecutive destination x's at once (a
// fixed row y, i.e. a fixed yi/yf). The 4 source positions land in different,
// non-contiguous src columns, so the horizontal gather is still scalar; only
// the interpolation arithmetic itself is vectorized, as in
// ImProcFunctions::channelMixer (ipchmixer.cc).
inline vfloat getBilinearValuesSIMD(const array2D<float> &src, float x0,
                                    float col_scale, float y)
{
    const int W = src.width();
    const int H = src.height();

    int yi = std::min(int(y), H - 1);
    float yf = y - yi;
    int yi1 = std::min(yi + 1, H - 1);
    const float *rowLo = src[yi];
    const float *rowHi = src[yi1];

    float xf[4] ALIGNED16;
    float bl[4] ALIGNED16;
    float br[4] ALIGNED16;
    float tl[4] ALIGNED16;
    float tr[4] ALIGNED16;

    for (int i = 0; i < 4; ++i) {
        float x = x0 + i * col_scale;
        int xi = std::min(int(x), W - 1);
        int xi1 = std::min(xi + 1, W - 1);
        xf[i] = x - xi;
        bl[i] = rowLo[xi];
        br[i] = rowLo[xi1];
        tl[i] = rowHi[xi];
        tr[i] = rowHi[xi1];
    }

    vfloat vone = F2V(1.f);
    vfloat vxf = LVF(xf[0]);
    vfloat vb = vxf * LVF(br[0]) + (vone - vxf) * LVF(bl[0]);
    vfloat vt = vxf * LVF(tr[0]) + (vone - vxf) * LVF(tl[0]);
    vfloat vyf = F2V(yf);
    return vyf * vt + (vone - vyf) * vb;
}

#endif // ART_SIMD

inline void rescaleBilinear(const array2D<float> &src, array2D<float> &dst,
                            bool multithread)
{
    const int Ws = src.width();
    const int Hs = src.height();
    const int Wd = dst.width();
    const int Hd = dst.height();

    float col_scale = float(Ws) / float(Wd);
    float row_scale = float(Hs) / float(Hd);

#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif

    for (int y = 0; y < Hd; ++y) {
        float ymrs = y * row_scale;
        int x = 0;

#ifdef ART_SIMD
        for (; x < Wd - 3; x += 4) {
            STVFU(dst[y][x],
                 getBilinearValuesSIMD(src, x * col_scale, col_scale, ymrs));
        }
#endif

        for (; x < Wd; ++x) {
            dst[y][x] = getBilinearValue(src, x * col_scale, ymrs);
        }
    }
}

template <class T>
inline void rescaleNearest(T *const *const src, int sW, int sH, T **dst, int dW,
                           int dH, bool multithread)
{
#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif

    for (int y = 0; y < dH; ++y) {
        int sy = y * sH / dH;

        for (int x = 0; x < dW; ++x) {
            int sx = x * sW / dW;
            dst[y][x] = src[sy][sx];
        }
    }
}

template <class T>
inline void rescaleNearest(const array2D<T> &src, array2D<T> &dst,
                           bool multithread)
{
    rescaleNearest(static_cast<T **>(const_cast<array2D<T> &>(src)),
                   src.width(), src.height(), static_cast<T **>(dst),
                   dst.width(), dst.height(), multithread);
}

} // namespace rtengine
