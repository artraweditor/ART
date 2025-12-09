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

#include "color.h"
#include "curves.h"
#include "improcfun.h"

namespace rtengine {

namespace {

void RGBCurve(const std::vector<double> &curvePoints, LUTf &outCurve, int skip)
{

    // create a curve if needed
    std::unique_ptr<DiagonalCurve> tcurve;

    if (!curvePoints.empty() && curvePoints[0] != 0) {
        tcurve = std::unique_ptr<DiagonalCurve>(
            new DiagonalCurve(curvePoints, CURVES_MIN_POLY_POINTS / skip));
    }

    if (tcurve && tcurve->isIdentity()) {
        tcurve = nullptr;
    }

    if (tcurve) {
        if (!outCurve) {
            outCurve(65536, 0);
        }

        for (int i = 0; i < 65536; i++) {
            // apply custom/parametric/NURBS curve, if any
            // RGB curves are defined with sRGB gamma, but operate on linear
            // data
            float val = Color::gamma2curve[i] / 65535.f;
            val = tcurve->getVal(val);
            outCurve[i] = Color::igammatab_srgb[val * 65535.f];
        }
    } else { // let the LUTf empty for identity curves
        outCurve.reset();
    }
}

} // namespace

void ImProcFunctions::rgbCurves(Imagefloat *img)
{
    PlanarWhateverData<float> *editWhatever = nullptr;
    EditUniqueID eid = pipetteBuffer ? pipetteBuffer->getEditID() : EUID_None;
    if ((eid == EUID_RGB_R || eid == EUID_RGB_G || eid == EUID_RGB_B) &&
        pipetteBuffer->getDataProvider()
                ->getCurrSubscriber()
                ->getPipetteBufferType() == BT_SINGLEPLANE_FLOAT) {
        editWhatever = pipetteBuffer->getSinglePlaneBuffer();
    }

    if (!params->rgbCurves.enabled) {
        if (editWhatever) {
            editWhatever->fill(0.f);
        }
        return;
    }

    img->setMode(Imagefloat::Mode::RGB, multiThread);

    LUTf rCurve, gCurve, bCurve;
    RGBCurve(params->rgbCurves.rcurve, rCurve, scale);
    RGBCurve(params->rgbCurves.gcurve, gCurve, scale);
    RGBCurve(params->rgbCurves.bcurve, bCurve, scale);

    const int W = img->getWidth();
    const int H = img->getHeight();

    if (editWhatever) {
        float **chan = nullptr;
        switch (eid) {
        case EUID_RGB_R:
            chan = img->r.ptrs;
            break;
        case EUID_RGB_G:
            chan = img->g.ptrs;
            break;
        case EUID_RGB_B:
            chan = img->b.ptrs;
            break;
        default:
            assert(false);
        }

#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                editWhatever->v(y, x) =
                    LIM01(Color::gamma2curve[chan[y][x]] / 65535.f);
            }
        }
    }

    if (rCurve || gCurve || bCurve) { // if any of the RGB curves is engaged
#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif
        for (int y = 0; y < H; ++y) {
            int x = 0;
#ifdef __SSE2__
            for (; x < W - 3; x += 4) {
                if (rCurve) {
                    STVF(img->r(y, x), rCurve[LVF(img->r(y, x))]);
                }
                if (gCurve) {
                    STVF(img->g(y, x), gCurve[LVF(img->g(y, x))]);
                }
                if (bCurve) {
                    STVF(img->b(y, x), bCurve[LVF(img->b(y, x))]);
                }
            }
#endif // __SSE2__
            for (; x < W; ++x) {
                if (rCurve) {
                    img->r(y, x) = rCurve[img->r(y, x)];
                }
                if (gCurve) {
                    img->g(y, x) = gCurve[img->g(y, x)];
                }
                if (bCurve) {
                    img->b(y, x) = bCurve[img->b(y, x)];
                }
            }
        }
    }
}

} // namespace rtengine
