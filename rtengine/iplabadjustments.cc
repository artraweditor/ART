/* -*- C++ -*-
 *
 *  This file is part of RawTherapee.
 *
 *  Copyright 2019 Alberto Griggio <alberto.griggio@gmail.com>
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

#ifdef _OPENMP
#include <omp.h>
#endif

#include "curves.h"
#include "improcfun.h"
#include "mytime.h"
#include "settings.h"

namespace rtengine {

extern const Settings *settings;

namespace {

void fillCurveArray(DiagonalCurve *diagCurve, LUTf &outCurve, int skip,
                    bool needed)
{
    if (needed) {

        for (int i = 0; i <= 0xffff; i += i < 0xffff - skip ? skip : 1) {
            // change to [0,1] range
            float val = (float)i / 65535.f;
            // apply custom/parametric/NURBS curve, if any
            val = diagCurve->getVal(val);
            // store result in a temporary array
            outCurve[i] = val;
        }

        // if skip>1, let apply linear interpolation in the skipped points of
        // the curve
        if (skip > 1) {
            float skipmul = 1.f / (float)skip;

            for (int i = 0; i <= 0x10000 - skip; i += skip) {
                for (int j = 1; j < skip; j++) {
                    outCurve[i + j] =
                        (outCurve[i] * (skip - j) + outCurve[i + skip] * j) *
                        skipmul;
                }
            }
        }

        outCurve *= 65535.f;
    } else {
        outCurve.makeIdentity();
    }
}

void get_L_curve(LUTf &out, int brightness, int contrast,
                 const std::vector<double> &curve, const LUTu &histogram,
                 int skip)
{
    if (brightness) {
        std::vector<double> pts = {
            DCT_NURBS,
            0.0,
            0.0, // black point
            0.1,
            0.1 + std::abs(brightness) / 150.0, // toe
            0.7,
            std::min(1.0, 0.7 + std::abs(brightness) / 300.0), // shoulder
            1.0,
            1.0 // white point
        };
        if (brightness < 0) {
            std::swap(pts[3], pts[4]);
            std::swap(pts[5], pts[6]);
        }

        DiagonalCurve brightcurve(pts, CURVES_MIN_POLY_POINTS / skip);

        for (int i = 0; i < 32768; i++) { // L values range up to 32767, higher
                                          // values are for highlight overflow
            float val = (float)i / 32767.0;
            val = brightcurve.getVal(val);
            out[i] = LIM01(val);
        }
    } else {
        out.makeIdentity(32767.f);
    }

    if (contrast) {
        // compute mean luminance of the image with the curve applied
        int sum = 0;
        float avg = 0;

        for (int i = 0; i < 32768; i++) {
            avg += out[i] * histogram[i];
            sum += histogram[i];
        }

        std::vector<double> pts;

        if (sum) {
            avg /= sum;
            pts = {DCT_NURBS,
                   0.0,
                   0.0,

                   avg - avg * (0.6 - contrast / 250.0), // toe point
                   avg - avg * (0.6 + contrast / 250.0), // value at toe point

                   avg + (1 - avg) * (0.6 - contrast / 250.0), // shoulder point
                   avg +
                       (1 - avg) *
                           (0.6 + contrast / 250.0), // value at shoulder point

                   1.0,
                   1.0};
        } else {
            // sum has an invalid value (next to 0, producing a division by
            // zero, so we create a fake contrast curve, producing a white image
            pts = {DCT_NURBS, 0.0, 1.0, 1.0, 1.0};
        }

        DiagonalCurve contrastcurve(pts, CURVES_MIN_POLY_POINTS / skip);
        for (int i = 0; i < 32768; i++) {
            out[i] = contrastcurve.getVal(out[i]);
        }
    }

    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

    // create a curve if needed
    std::unique_ptr<DiagonalCurve> tcurve;

    if (!curve.empty() && curve[0] != 0) {
        tcurve.reset(new DiagonalCurve(curve, CURVES_MIN_POLY_POINTS / skip));
    }

    if (tcurve && tcurve->isIdentity()) {
        tcurve = nullptr;
    }

    if (tcurve) {
        // L values go up to 32767, last stop is for highlight overflow
        for (int i = 0; i < 32768; i++) {
            float val;
            // apply custom/parametric/NURBS curve, if any
            val = tcurve->getVal(out[i]);

            out[i] = (32767.f * val);
        }
    } else {
        out *= 32767.f;
    }

    for (int i = 32768; i < 32770;
         i++) { // set last two elements of lut to 32768 and 32769 to allow
                // linear interpolation
        out[i] = (float)i;
    }
}

void get_ab_curves(LUTf &aout, LUTf &bout, const std::vector<double> &acurve,
                   const std::vector<double> &bcurve, int skip)
{
    std::unique_ptr<DiagonalCurve> dCurve;

    // create a curve if needed
    if (!acurve.empty() && acurve[0] != 0) {
        dCurve.reset(new DiagonalCurve(acurve, CURVES_MIN_POLY_POINTS / skip));
    }
    fillCurveArray(dCurve.get(), aout, skip, dCurve && !dCurve->isIdentity());

    dCurve = nullptr;

    if (!bcurve.empty() && bcurve[0] != 0) {
        dCurve.reset(new DiagonalCurve(bcurve, CURVES_MIN_POLY_POINTS / skip));
    }
    fillCurveArray(dCurve.get(), bout, skip, dCurve && !dCurve->isIdentity());
}

void lab_adjustments(const ImProcData &im, Imagefloat *img, LUTf &lcurve,
                     LUTf &acurve, LUTf &bcurve, LUTu *histLCurve,
                     PipetteBuffer *pipetteBuffer)
{
    const auto params = im.params;
    const auto multiThread = im.multiThread;

    const int W = img->getWidth();
    const int H = img->getHeight();

    PlanarWhateverData<float> *editWhatever = nullptr;
    EditUniqueID editID = EUID_None;
    bool editPipette = false;

    if (pipetteBuffer) {
        editID = pipetteBuffer->getEditID();

        if ((editID == EUID_Lab_LCurve || editID == EUID_Lab_aCurve ||
             editID == EUID_Lab_bCurve) &&
            pipetteBuffer->getDataProvider()
                    ->getCurrSubscriber()
                    ->getPipetteBufferType() == BT_SINGLEPLANE_FLOAT) {
            editPipette = true;
            editWhatever = pipetteBuffer->getSinglePlaneBuffer();
        }
    }

    if (!params->labCurve.enabled) {
        if (editPipette) {
            // fill pipette buffer with zeros to avoid crashes
            editWhatever->fill(0.f);
        }
        return;
    }

    if (editPipette) {
#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                float val = 0.f;
                switch (editID) {
                case EUID_Lab_LCurve:
                    val = img->g(y, x) / 32768.f;
                    break;
                case EUID_Lab_aCurve:
                    val =
                        (img->r(y, x) + (32768.f * 1.28f)) / (65536.f * 1.28f);
                    break;
                case EUID_Lab_bCurve:
                    val =
                        (img->b(y, x) + (32768.f * 1.28f)) / (65536.f * 1.28f);
                    break;
                default:
                    break;
                }
                editWhatever->v(y, x) = LIM01(val);
            }
        }
    }

    if (histLCurve) {
        histLCurve->clear();
        const int compression = log2(32768.f / histLCurve->getSize());
#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                int L = LIM(int(img->g(y, x)), 0, 32768);
                (*histLCurve)[L >> compression]++;
            }
        }
    }

    const float chroma = (params->labCurve.chromaticity + 100.0f) / 100.0f;
#ifdef __SSE2__
    const vfloat chromav = F2V(chroma);
    const vfloat v32768 = F2V(32768.f);
#endif

#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif
    for (int y = 0; y < H; ++y) {
        int x = 0;
#ifdef __SSE2__
        for (; x < W - 3; x += 4) {
            vfloat L = LVF(img->g(y, x));
            vfloat a = LVF(img->r(y, x));
            vfloat b = LVF(img->b(y, x));
            L = lcurve[L];
            a = (acurve[a + v32768] - v32768) * chromav;
            b = (bcurve[b + v32768] - v32768) * chromav;
            STVF(img->g(y, x), L);
            STVF(img->r(y, x), a);
            STVF(img->b(y, x), b);
        }
#endif
        for (; x < W; ++x) {
            float &L = img->g(y, x);
            float &a = img->r(y, x);
            float &b = img->b(y, x);
            L = lcurve[L];
            a = (acurve[a + 32768.f] - 32768.f) * chroma;
            b = (bcurve[b + 32768.f] - 32768.f) * chroma;
        }
    }
}

} // namespace

void ImProcFunctions::labAdjustments(Imagefloat *rgb)
{
    if (!params->labCurve.enabled) {
        return;
    }

    rgb->setMode(Imagefloat::Mode::LAB, multiThread);

    LUTu hist16;
    LUTf lcurve;
    LUTf acurve;
    LUTf bcurve;

    hist16(65536);
    lcurve(32770, 0); // lumacurve[32768] and lumacurve[32769] will be set to
                      // 32768 and 32769 later to allow linear interpolation
    acurve(65536);
    bcurve(65536);

    if (params->labCurve.contrast != 0) { // only use hist16 for contrast
        hist16.clear();
        int fh = rgb->getHeight();
        int fw = rgb->getWidth();
        float **lab_L = rgb->g.ptrs;

#ifdef _OPENMP
#pragma omp parallel if (multiThread)
#endif
        {
            LUTu hist16thr(
                hist16.getSize()); // one temporary lookup table per thread
            hist16thr.clear();
#ifdef _OPENMP
#pragma omp for schedule(static) nowait
#endif
            for (int i = 0; i < fh; i++) {
                for (int j = 0; j < fw; j++) {
                    hist16thr[(int)((lab_L[i][j]))]++;
                }
            }

#ifdef _OPENMP
#pragma omp critical
#endif
            {
                hist16 += hist16thr;
            }
        }
    }

    get_L_curve(lcurve, params->labCurve.brightness, params->labCurve.contrast,
                params->labCurve.lcurve, hist16, scale);
    get_ab_curves(acurve, bcurve, params->labCurve.acurve,
                  params->labCurve.bcurve, scale);

    lab_adjustments(ImProcData(params, scale, multiThread), rgb, lcurve, acurve,
                    bcurve, histLCurve, pipetteBuffer);
}

} // namespace rtengine
