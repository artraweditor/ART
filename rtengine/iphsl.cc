/* -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright 2019 Alberto Griggio <alberto.griggio@gmail.com>
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

#include "array2D.h"
#include "color.h"
#include "curves.h"
#include "guidedfilter.h"
#include "improcfun.h"

namespace rtengine {

void ImProcFunctions::hslEqualizer(Imagefloat *img)
{
    PlanarWhateverData<float> *editWhatever = nullptr;
    EditUniqueID eid = pipetteBuffer ? pipetteBuffer->getEditID() : EUID_None;
    if ((eid == EUID_HSL_H || eid == EUID_HSL_S || eid == EUID_HSL_V) &&
        pipetteBuffer->getDataProvider()
                ->getCurrSubscriber()
                ->getPipetteBufferType() == BT_SINGLEPLANE_FLOAT) {
        editWhatever = pipetteBuffer->getSinglePlaneBuffer();
    }

    if (!params->hsl.enabled) {
        if (editWhatever) {
            editWhatever->fill(0.f);
        }
        return;
    }

    img->setMode(Imagefloat::Mode::YUV, multiThread);
    img->normalizeFloatTo1(multiThread);

    const int W = img->getWidth();
    const int H = img->getHeight();

    array2D<float> mask(W, H);

    FlatCurve hcurve(params->hsl.hCurve, true, CURVES_MIN_POLY_POINTS / scale);
    FlatCurve scurve(params->hsl.sCurve, true, CURVES_MIN_POLY_POINTS / scale);
    FlatCurve lcurve(params->hsl.lCurve, true, CURVES_MIN_POLY_POINTS / scale);

    array2D<float> Y(W, H, img->g.ptrs, ARRAY2D_BYREFERENCE);
    const float pi2 = 2.f * RT_PI_F;

    const auto hue01 = [=](float h) -> float {
        float v = h / pi2;
        if (v < 0.f) {
            return 1.f + v;
        } else if (v > 1.f) {
            return v - 1.f;
        } else {
            return v;
        }
    };

    const auto tolin = [](float y, float base) -> float {
        float v = (y - 0.5f) * 2.f;
        return SGN(v) * LIM01(xlog2lin(std::abs(v), base));
    };

    if (editWhatever) {
#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                float h, s;
                Color::yuv2hsl(img->b(y, x), img->r(y, x), h, s);
                editWhatever->v(y, x) = hue01(h);
            }
        }
    }

    const float smooth =
        std::pow(10.f, LIM01(params->hsl.smoothing / 10.f)) - 1.f;

#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            float h, s;
            float u = img->b(y, x);
            float v = img->r(y, x);
            Color::yuv2hsl(u, v, h, s);
            img->r(y, x) = h;
            img->b(y, x) = s;
        }
    }

    if (!scurve.isIdentity()) {
#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                float h = img->r(y, x);
                float f = scurve.getVal(hue01(h));
                mask[y][x] = f;
            }
        }

        const int radius = 4 / scale * smooth + 0.5;
        constexpr float eps = 0.001;
        if (radius > 0) {
            guidedFilter(Y, mask, mask, radius, eps, multiThread);
        }

        FlatCurve coeff(
            {FCT_MinMaxCPoints, 0.25, 0.0, 0.5, 0.18, 1, 1, 0, 0.35});

#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                float f = tolin(mask[y][x], 2.f); // 10.f);
                // float s = LIM01(img->b(y, x) * 4.f);
                // img->b(y, x) *= 1.f + (f >= 0.f ? pow_F(s, 1.8f) :
                // pow_F(s, 1.f/1.8f)) * f;
                float s = 1.f + (f < 0 ? coeff.getVal(img->b(y, x))
                                       : 1.f - coeff.getVal(img->b(y, x)));
                img->b(y, x) *= 1.f + SGN(f) * pow_F(LIM01(std::abs(f)), s);
            }
        }
    }

    if (!lcurve.isIdentity()) {
#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                float h = img->r(y, x);
                float f = lcurve.getVal(hue01(h));
                mask[y][x] = f;
            }
        }

        const int radius = 25 / scale * smooth + 0.5;
        constexpr float eps = 0.0001;
        if (radius > 0) {
            guidedFilter(Y, mask, mask, radius, eps, multiThread);
        }

#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                float f = 1.f + tolin(mask[y][x], 10.f);
                img->g(y, x) *= f;
            }
        }
    }

    if (!hcurve.isIdentity()) {
#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                float h = img->r(y, x);
                float f = hcurve.getVal(hue01(h));
                mask[y][x] = f;
            }
        }

        const int radius = 4 / scale * smooth + 0.5;
        constexpr float eps = 0.001;
        if (radius > 0) {
            guidedFilter(Y, mask, mask, radius, eps, multiThread);
        }

#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                float h = img->r(y, x);
                float f = tolin(mask[y][x], 32.f) * RT_PI_F;
                h += f;
                img->r(y, x) = h;
            }
        }
    }

#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            float h = img->r(y, x);
            float s = img->b(y, x);
            float u, v;
            Color::hsl2yuv(h, s, u, v);
            img->b(y, x) = u;
            img->r(y, x) = v;
        }
    }

    img->normalizeFloatTo65535(multiThread);
}

} // namespace rtengine
