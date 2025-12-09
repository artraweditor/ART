////////////////////////////////////////////////////////////////
//
//      Chromatic Aberration Auto-correction
//
//      copyright (c) 2008-2010  Emil Martinec <ejmartin@uchicago.edu>
//
//
// code dated: November 24, 2010
// optimized: September 2013, Ingo Weyrich
// further optimized: February 2018, Ingo Weyrich
//
//  Ingo Weyrich March 2018: The above comment 'Chromatic Aberration
//  Auto-correction' sounds wrong
//                           I guess it should have been 'Purple fringe
//                           correction' though it's not restricted to 'Purple'
//
//  PF_correct_RT.cc is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////

#include "../rtgui/myflatcurve.h"
#include "StopWatch.h"
#include "gauss.h"
#include "improcfun.h"
#include "jaggedarray.h"
#include "median.h"
#include "opthelper.h"
#include "rt_math.h"
#include "sleef.h"

namespace rtengine {

namespace {
// Defringe in Lab mode
void PF_correct_RT(const rtengine::ProcParams *params, Imagefloat *lab,
                   double radius, int thresh, bool multiThread)
{
    BENCHFUN
    std::unique_ptr<FlatCurve> chCurve;
    if (params->defringe.huecurve.size() &&
        FlatCurveType(params->defringe.huecurve.at(0)) > FCT_Linear) {
        chCurve.reset(new FlatCurve(params->defringe.huecurve));
    }

    const int width = lab->getWidth(), height = lab->getHeight();

    // temporary array to store chromaticity
    const std::unique_ptr<float[]> fringe(new float[width * height]);

    JaggedArray<float> tmpa(width, height);
    JaggedArray<float> tmpb(width, height);

    double chromave = 0.0; // use double precision for large summations

#ifdef _OPENMP
#pragma omp parallel
#endif
    {
        gaussianBlur(lab->r.ptrs, tmpa, width, height, radius);
        gaussianBlur(lab->b.ptrs, tmpb, width, height, radius);

#ifdef _OPENMP
#pragma omp for reduction(+ : chromave) schedule(dynamic, 16)
#endif

        for (int i = 0; i < height; i++) {
#ifdef __SSE2__

            // vectorized per row precalculation of the atan2 values
            if (chCurve) {
                int k = 0;

                for (; k < width - 3; k += 4) {
                    STVFU(fringe[i * width + k],
                          xatan2f(LVFU(lab->b(i, k)), LVFU(lab->r(i, k))));
                }

                for (; k < width; k++) {
                    fringe[i * width + k] = xatan2f(lab->b(i, k), lab->r(i, k));
                }
            }

#endif

            for (int j = 0; j < width; j++) {
                float chromaChfactor = 1.f;
                if (chCurve) {
#ifdef __SSE2__
                    // use the precalculated atan values
                    const float HH = fringe[i * width + j];
#else
                    // no precalculated values without SSE => calculate
                    const float HH = xatan2f(lab->b(i, j), lab->r(i, j));
#endif
                    float chparam =
                        chCurve->getVal((Color::huelab_to_huehsv2(HH))) -
                        0.5f; // get C=f(H)

                    if (chparam < 0.f) {
                        chparam *= 2.f; // increased action if chparam < 0
                    }

                    chromaChfactor = SQR(1.f + chparam);
                }

                const float chroma =
                    chromaChfactor *
                    (SQR(lab->r(i, j) - tmpa[i][j]) +
                     SQR(lab->b(i, j) -
                         tmpb[i][j])); // modulate chroma function hue
                chromave += chroma;
                fringe[i * width + j] = chroma;
            }
        }
    }

    chromave /= height * width;

    if (chromave > 0.0) {
        // now as chromave is calculated, we postprocess fringe to reduce the
        // number of divisions in future
#ifdef _OPENMP
#pragma omp parallel for simd
#endif

        for (int j = 0; j < width * height; j++) {
            fringe[j] = 1.f / (fringe[j] + chromave);
        }

        const float threshfactor =
            1.f / (SQR(thresh / 33.f) * chromave * 5.0f + chromave);
        const int halfwin = std::ceil(2 * radius) + 1;

// Issue 1674:
// often, colour fringe is not evenly distributed, e.g. a lot in contrasty
// regions and none in the sky. so it's better to schedule dynamic and let every
// thread only process 16 rows, to avoid running big threads out of work
// Measured it and in fact gives better performance than without
// schedule(dynamic,16). Of course, there could be a better choice for the
// chunk_size than 16 Issue 1972: Split this loop in three parts to avoid most
// of the min and max-operations
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic, 16)
#endif

        for (int i = 0; i < height; i++) {
            int j = 0;
            for (; j < halfwin - 1; j++) {

                // test for pixel darker than some fraction of neighbourhood
                // ave, near an edge, more saturated than average
                if (fringe[i * width + j] < threshfactor) {
                    float atot = 0.f, btot = 0.f, norm = 0.f;

                    for (int i1 = std::max(0, i - halfwin + 1);
                         i1 < std::min(height, i + halfwin); i1++)
                        for (int j1 = 0; j1 < j + halfwin; j1++) {
                            // neighbourhood average of pixels weighted by
                            // chrominance
                            const float wt = fringe[i1 * width + j1];
                            atot += wt * lab->r(i1, j1);
                            btot += wt * lab->b(i1, j1);
                            norm += wt;
                        }

                    lab->r(i, j) = atot / norm;
                    lab->b(i, j) = btot / norm;
                }
            }

            for (; j < width - halfwin + 1; j++) {

                // test for pixel darker than some fraction of neighbourhood
                // ave, near an edge, more saturated than average
                if (fringe[i * width + j] < threshfactor) {
                    float atot = 0.f, btot = 0.f, norm = 0.f;

                    for (int i1 = std::max(0, i - halfwin + 1);
                         i1 < std::min(height, i + halfwin); i1++)
                        for (int j1 = j - halfwin + 1; j1 < j + halfwin; j1++) {
                            // neighbourhood average of pixels weighted by
                            // chrominance
                            const float wt = fringe[i1 * width + j1];
                            atot += wt * lab->r(i1, j1);
                            btot += wt * lab->b(i1, j1);
                            norm += wt;
                        }

                    lab->r(i, j) = atot / norm;
                    lab->b(i, j) = btot / norm;
                }
            }

            for (; j < width; j++) {

                // test for pixel darker than some fraction of neighbourhood
                // ave, near an edge, more saturated than average
                if (fringe[i * width + j] < threshfactor) {
                    float atot = 0.f, btot = 0.f, norm = 0.f;

                    for (int i1 = std::max(0, i - halfwin + 1);
                         i1 < std::min(height, i + halfwin); i1++)
                        for (int j1 = j - halfwin + 1; j1 < width; j1++) {
                            // neighbourhood average of pixels weighted by
                            // chrominance
                            const float wt = fringe[i1 * width + j1];
                            atot += wt * lab->r(i1, j1);
                            btot += wt * lab->b(i1, j1);
                            norm += wt;
                        }

                    lab->r(i, j) = atot / norm;
                    lab->b(i, j) = btot / norm;
                }
            }
        } // end of ab channel averaging
    }
}

} // namespace

void ImProcFunctions::defringe(Imagefloat *rgb)
{
    if (params->defringe.enabled && rgb->getWidth() >= 8 &&
        rgb->getHeight() >= 8)

    {
        rgb->setMode(Imagefloat::Mode::LAB, multiThread);
        PF_correct_RT(params, rgb, params->defringe.radius / scale,
                      params->defringe.threshold, multiThread);
    }
}

} // namespace rtengine
