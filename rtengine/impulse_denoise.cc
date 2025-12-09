/*
 *  This file is part of RawTherapee.
 *
 *  RawTherapee is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RawTherapee is distributed in the hope that it will be useful,
 *  but widthITheightOUT ANY widthARRANTY; without even the implied warranty of
 *  MERCheightANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RawTherapee.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  2010 Emil Martinec <ejmartin@uchicago.edu>
 *
 */
#include "gauss.h"
#include "improcfun.h"
#include "labimage.h"
#include "opthelper.h"
#include "rt_algo.h"
#include "rt_math.h"
#include "sleef.h"
#include <cstddef>

namespace rtengine {

namespace {

void impulse_nr(Imagefloat *lab, double thresh, bool multithread)
{
    // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    // impulse noise removal
    // local variables

    const int width = lab->getWidth();
    const int height = lab->getHeight();
    float **lab_L = lab->g.ptrs;
    float **lab_a = lab->r.ptrs;
    float **lab_b = lab->b.ptrs;

    // buffer for the lowpass image
    // float * lpf[height] ALIGNED16;
    // lpf[0] = new float [width * height];
    // buffer for the highpass image
    char *impish[height] ALIGNED16;
    impish[0] = new char[width * height];

    for (int i = 1; i < height; i++) {
        // lpf[i] = lpf[i - 1] + width;
        impish[i] = impish[i - 1] + width;
    }

    markImpulse(width, height, lab_L, impish, thresh);

    //     //The cleaning algorithm starts here

    //     //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    //     // modified bilateral filter for lowpass image, omitting input pixel;
    //     or Gaussian blur

    const float eps = 1.0;

// now impulsive values have been identified

// Issue 1671:
// often, noise isn't evenly distributed, e.g. only a few noisy pixels in the
// bright sky, but many in the dark foreground, so it's better to schedule
// dynamic and let every thread only process 16 rows, to avoid running big
// threads out of work Measured it and in fact gives better performance than
// without schedule(dynamic,16). Of course, there could be a better choice for
// the chunk_size than 16 race conditions are avoided by the array impish
#ifdef _OPENMP
#pragma omp parallel if (multithread)
#endif
    {
        int i1, j1, j;
        float wtdsum[3], dirwt, norm;
#ifdef _OPENMP
#pragma omp for schedule(dynamic, 16)
#endif

        for (int i = 0; i < height; i++) {
            for (j = 0; j < 2; j++) {
                if (!impish[i][j]) {
                    continue;
                }

                norm = 0.0;
                wtdsum[0] = wtdsum[1] = wtdsum[2] = 0.0;

                for (i1 = max(0, i - 2); i1 <= min(i + 2, height - 1); i1++)
                    for (j1 = 0; j1 <= j + 2; j1++) {
                        if (impish[i1][j1]) {
                            continue;
                        }

                        dirwt = 1 / (SQR(lab_L[i1][j1] - lab_L[i][j]) +
                                     eps); // use more sophisticated rangefn???
                        wtdsum[0] += dirwt * lab_L[i1][j1];
                        wtdsum[1] += dirwt * lab_a[i1][j1];
                        wtdsum[2] += dirwt * lab_b[i1][j1];
                        norm += dirwt;
                    }

                if (norm) {
                    lab_L[i][j] = wtdsum[0] / norm; // low pass filter
                    lab_a[i][j] = wtdsum[1] / norm; // low pass filter
                    lab_b[i][j] = wtdsum[2] / norm; // low pass filter
                }
            }

            for (; j < width - 2; j++) {
                if (!impish[i][j]) {
                    continue;
                }

                norm = 0.0;
                wtdsum[0] = wtdsum[1] = wtdsum[2] = 0.0;

                for (i1 = max(0, i - 2); i1 <= min(i + 2, height - 1); i1++)
                    for (j1 = j - 2; j1 <= j + 2; j1++) {
                        if (impish[i1][j1]) {
                            continue;
                        }

                        dirwt = 1 / (SQR(lab_L[i1][j1] - lab_L[i][j]) +
                                     eps); // use more sophisticated rangefn???
                        wtdsum[0] += dirwt * lab_L[i1][j1];
                        wtdsum[1] += dirwt * lab_a[i1][j1];
                        wtdsum[2] += dirwt * lab_b[i1][j1];
                        norm += dirwt;
                    }

                if (norm) {
                    lab_L[i][j] = wtdsum[0] / norm; // low pass filter
                    lab_a[i][j] = wtdsum[1] / norm; // low pass filter
                    lab_b[i][j] = wtdsum[2] / norm; // low pass filter
                }
            }

            for (; j < width; j++) {
                if (!impish[i][j]) {
                    continue;
                }

                norm = 0.0;
                wtdsum[0] = wtdsum[1] = wtdsum[2] = 0.0;

                for (i1 = max(0, i - 2); i1 <= min(i + 2, height - 1); i1++)
                    for (j1 = j - 2; j1 < width; j1++) {
                        if (impish[i1][j1]) {
                            continue;
                        }

                        dirwt = 1 / (SQR(lab_L[i1][j1] - lab_L[i][j]) +
                                     eps); // use more sophisticated rangefn???
                        wtdsum[0] += dirwt * lab_L[i1][j1];
                        wtdsum[1] += dirwt * lab_a[i1][j1];
                        wtdsum[2] += dirwt * lab_b[i1][j1];
                        norm += dirwt;
                    }

                if (norm) {
                    lab_L[i][j] = wtdsum[0] / norm; // low pass filter
                    lab_a[i][j] = wtdsum[1] / norm; // low pass filter
                    lab_b[i][j] = wtdsum[2] / norm; // low pass filter
                }
            }
        }
    }
    // now impulsive values have been corrected

    // delete [] lpf[0];
    delete[] impish[0];
}

} // namespace

void ImProcFunctions::impulsedenoise(Imagefloat *rgb)
{

    if (params->impulseDenoise.enabled && rgb->getWidth() >= 8 &&
        rgb->getHeight() >= 8 && scale >= 0.5)

    {
        float t = params->impulseDenoise.thresh / scale;
        rgb->setMode(Imagefloat::Mode::LAB, multiThread);
        impulse_nr(rgb, t / 20.0, multiThread);
    }
}

} // namespace rtengine
