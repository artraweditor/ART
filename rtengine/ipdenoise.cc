/* -*- C++ -*-
 *
 *  This file is part of RawTherapee.
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

/*
 Structure of the algorithm:

 1. Compute an initial denoise of the image via undecimated wavelet transform
 and universal thresholding modulated by user input.
 2. Decompose the residual image into TSxTS size tiles, shifting by 'offset'
 each step (so roughly each pixel is in (TS/offset)^2 tiles); Discrete Cosine
 transform the tiles.
 3. Filter the DCT data to pick out patterns missed by the wavelet denoise
 4. Inverse DCT the denoised tile data and combine the tiles into a denoised
 output image.
 5. Optional final smoothing via guided filter (for chrominance) and
 non-local means (for luminance), in linear RGB space
 */

#include "ipdenoise.h"
#include "imagesource.h"
#include "improcfun.h"
#include "mytime.h"
#include "nlmeans.h"
#include "pipelineprofile.h"
#include "iccstore.h"
#include "rt_algo.h"
#include "wavelet.h"

/* The two backends share this file but not their entry points.  Each of the
 * two public calls is a thin dispatcher over a CPU and a GPU implementation:
 *
 *   RGB_denoise           -> RGB_denoise_CPU     / RGB_denoise_GPU
 *   denoiseComputeParams  -> computeParamsCPU    / computeParamsGPU
 *
 * with the backend-neutral work factored out ahead of the choice
 * (denoisePrepare -> DenoisePrep, and CropAnalysisJob for the analysis) and
 * one implementation of each phase's CPU maths, which the GPU driver calls
 * when a phase declines.  The CPU drivers mention no gpu:: identifier at all,
 * so the two sides can be optimised without reading each other.  Note that
 * DenoisePrep, DctWorkspace and CropAnalysisJob are deliberately local to this
 * TU -- nothing outside needs them, and DctWorkspace could not leave anyway,
 * since it is built on the TS/offset/blkrad macros that are #undef'd below. */
#include "../rtgui/threadutils.h"
#include "LUT.h"
#include "array2D.h"
#include "boxblur.h"
#include "gauss.h"
#include "guidedfilter.h"
#include "iccmatrices.h"
#include "median.h"
#include "opthelper.h"
#include "rescale.h"
#include "rt_math.h"
#include "rtengine.h"
#include "sleef.h"

#include <cmath>
#include <cstdlib>
#include <fftw3.h>
#include <iostream>
#include <memory>
#ifdef _OPENMP
#include <omp.h>
#endif
#include "StopWatch.h"

namespace rtengine {

extern const Settings *settings;
using namespace procparams;

namespace denoise {

float MadRgb(float *DataList, const int datalen)
{
    if (datalen <= 1) { // Avoid possible buffer underrun
        return 0;
    }

    // computes Median Absolute Deviation
    // DataList values should mostly have abs val < 65536 because we are in RGB
    // mode
    int *histo = new int[65536];

    for (int i = 0; i < 65536; ++i) {
        histo[i] = 0;
    }

    // calculate histogram of absolute values of wavelet coeffs
    int i;

    for (i = 0; i < datalen; ++i) {
        histo[min(65535, static_cast<int>(abs(DataList[i])))]++;
    }

    // find median of histogram
    int median = 0, count = 0;

    while (count < datalen / 2) {
        count += histo[median];
        ++median;
    }

    int count_ = count - histo[median - 1];

    // interpolate
    delete[] histo;
    return (((median - 1) +
             (datalen / 2 - count_) / (static_cast<float>(count - count_))) /
            0.6745);
}

void ShrinkAllL(double scale, wavelet_decomposition &WaveletCoeffs_L,
                float **buffer, int level, int dir, float *noisevarlum,
                float *madL, float *vari, int edge)

{
    // simple wavelet shrinkage
    const float eps = 0.01f;

    float *sfave = buffer[0] + 32;
    float *sfaved = buffer[1] + 64;
    float *blurBuffer = buffer[2] + 96;

    int W_L = WaveletCoeffs_L.level_W(level);
    int H_L = WaveletCoeffs_L.level_H(level);

    float **WavCoeffs_L = WaveletCoeffs_L.level_coeffs(level);
    //      printf("OK lev=%d\n",level);
    float mad_L = madL[dir - 1];

    if (edge == 1 && vari) {
        noisevarlum =
            blurBuffer; // we need one buffer, but fortunately we don't have to
                        // allocate a new one because we can use blurBuffer

        for (int i = 0; i < W_L * H_L; ++i) {
            noisevarlum[i] = vari[level];
        }
    }

    float levelFactor = mad_L * 5.f / static_cast<float>(level + 1);
#ifdef ART_SIMD
    __m128 magv;
    __m128 levelFactorv = _mm_set1_ps(levelFactor);
    __m128 mad_Lv;
    __m128 ninev = _mm_set1_ps(9.0f);
    __m128 epsv = _mm_set1_ps(eps);
    int i;

    for (i = 0; i < W_L * H_L - 3; i += 4) {
        mad_Lv = LVFU(noisevarlum[i]) * levelFactorv;
        magv = SQRV(LVFU(WavCoeffs_L[dir][i]));
        _mm_storeu_ps(
            &sfave[i],
            magv / (magv + mad_Lv * xexpf(-magv / (ninev * mad_Lv)) + epsv));
    }

    // few remaining pixels
    for (; i < W_L * H_L; ++i) {
        float mag = SQR(WavCoeffs_L[dir][i]);
        sfave[i] = mag / (mag +
                          levelFactor * noisevarlum[i] *
                              xexpf(-mag / (9 * levelFactor * noisevarlum[i])) +
                          eps);
    }

#else

    for (int i = 0; i < W_L * H_L; ++i) {

        float mag = SQR(WavCoeffs_L[dir][i]);
        float shrinkfactor =
            mag / (mag +
                   levelFactor * noisevarlum[i] *
                       xexpf(-mag / (9 * levelFactor * noisevarlum[i])) +
                   eps);
        sfave[i] = shrinkfactor;
    }

#endif
    const int blur_rad = max(1, int((level + 2) / scale));
    boxblur(sfave, sfaved, blurBuffer, blur_rad, blur_rad, W_L,
            H_L); // increase smoothness by locally averaging shrinkage

#ifdef ART_SIMD
    __m128 sfv;

    for (i = 0; i < W_L * H_L - 3; i += 4) {
        sfv = LVFU(sfave[i]);
        // use smoothed shrinkage unless local shrinkage is much less
        _mm_storeu_ps(&WavCoeffs_L[dir][i],
                      _mm_loadu_ps(&WavCoeffs_L[dir][i]) *
                          (SQRV(LVFU(sfaved[i])) + SQRV(sfv)) /
                          (LVFU(sfaved[i]) + sfv + epsv));
    }

    // few remaining pixels
    for (; i < W_L * H_L; ++i) {
        float sf = sfave[i];

        // use smoothed shrinkage unless local shrinkage is much less
        WavCoeffs_L[dir][i] *=
            (SQR(sfaved[i]) + SQR(sf)) / (sfaved[i] + sf + eps);
    } // now luminance coefficients are denoised

#else

    for (int i = 0; i < W_L * H_L; ++i) {
        float sf = sfave[i];

        // use smoothed shrinkage unless local shrinkage is much less
        WavCoeffs_L[dir][i] *=
            (SQR(sfaved[i]) + SQR(sf)) / (sfaved[i] + sf + eps);

    } // now luminance coefficients are denoised

#endif
}

void ShrinkAllAB(double scale, wavelet_decomposition &WaveletCoeffs_L,
                 wavelet_decomposition &WaveletCoeffs_ab, float **buffer,
                 int level, int dir, float *noisevarchrom, float noisevar_ab,
                 const bool useNoiseCCurve, bool autoch, float *madL,
                 float *madaab = nullptr, bool madCalculated = false)

{
    // simple wavelet shrinkage
    const float eps = 0.01f;

    if (autoch && noisevar_ab <= 0.001f) {
        noisevar_ab = 0.02f;
    }

    float *sfaveab = buffer[0] + 32;
    float *sfaveabd = buffer[1] + 64;
    float *blurBuffer = buffer[2] + 96;

    int W_ab = WaveletCoeffs_ab.level_W(level);
    int H_ab = WaveletCoeffs_ab.level_H(level);

    float **WavCoeffs_L = WaveletCoeffs_L.level_coeffs(level);
    float **WavCoeffs_ab = WaveletCoeffs_ab.level_coeffs(level);

    float madab;
    float mad_L = madL[dir - 1];

    if (madCalculated) {
        madab = madaab[dir - 1];
    } else {
        madab = SQR(MadRgb(WavCoeffs_ab[dir], W_ab * H_ab));
    }

    if (noisevar_ab > 0.001f) {
        madab = useNoiseCCurve ? madab : madab * noisevar_ab;
#ifdef ART_SIMD
        __m128 onev = _mm_set1_ps(1.f);
        __m128 mad_abrv = _mm_set1_ps(madab);

        __m128 rmadLm9v = onev / _mm_set1_ps(mad_L * 9.f);
        __m128 mad_abv;
        __m128 mag_Lv, mag_abv;
        int coeffloc_ab;

        for (coeffloc_ab = 0; coeffloc_ab < H_ab * W_ab - 3; coeffloc_ab += 4) {
            mad_abv = LVFU(noisevarchrom[coeffloc_ab]) * mad_abrv;

            mag_Lv = LVFU(WavCoeffs_L[dir][coeffloc_ab]);
            mag_abv = SQRV(LVFU(WavCoeffs_ab[dir][coeffloc_ab]));
            mag_Lv = (SQRV(mag_Lv))*rmadLm9v;
            _mm_storeu_ps(&sfaveab[coeffloc_ab],
                          (onev - xexpf(-(mag_abv / mad_abv) - (mag_Lv))));
        }

        // few remaining pixels
        for (; coeffloc_ab < H_ab * W_ab; ++coeffloc_ab) {
            float mag_L = SQR(WavCoeffs_L[dir][coeffloc_ab]);
            float mag_ab = SQR(WavCoeffs_ab[dir][coeffloc_ab]);
            sfaveab[coeffloc_ab] =
                (1.f - xexpf(-(mag_ab / (noisevarchrom[coeffloc_ab] * madab)) -
                             (mag_L / (9.f * mad_L))));
        } // now chrominance coefficients are denoised

#else

        for (int i = 0; i < H_ab; ++i) {
            for (int j = 0; j < W_ab; ++j) {
                int coeffloc_ab = i * W_ab + j;
                float mag_L = SQR(WavCoeffs_L[dir][coeffloc_ab]);
                float mag_ab = SQR(WavCoeffs_ab[dir][coeffloc_ab]);
                sfaveab[coeffloc_ab] =
                    (1.f -
                     xexpf(-(mag_ab / (noisevarchrom[coeffloc_ab] * madab)) -
                           (mag_L / (9.f * mad_L))));
            }
        } // now chrominance coefficients are denoised

#endif

        const int blur_rad = max(1, int((level + 2) / scale));
        boxblur(sfaveab, sfaveabd, blurBuffer, blur_rad, blur_rad, W_ab,
                H_ab); // increase smoothness by locally averaging shrinkage
#ifdef ART_SIMD
        __m128 epsv = _mm_set1_ps(eps);
        __m128 sfabv;
        __m128 sfaveabv;

        for (coeffloc_ab = 0; coeffloc_ab < H_ab * W_ab - 3; coeffloc_ab += 4) {
            sfabv = LVFU(sfaveab[coeffloc_ab]);
            sfaveabv = LVFU(sfaveabd[coeffloc_ab]);

            // use smoothed shrinkage unless local shrinkage is much less
            _mm_storeu_ps(&WavCoeffs_ab[dir][coeffloc_ab],
                          LVFU(WavCoeffs_ab[dir][coeffloc_ab]) *
                              (SQRV(sfaveabv) + SQRV(sfabv)) /
                              (sfaveabv + sfabv + epsv));
        }

        // few remaining pixels
        for (; coeffloc_ab < H_ab * W_ab; ++coeffloc_ab) {
            // modification Jacques feb 2013
            float sfab = sfaveab[coeffloc_ab];

            // use smoothed shrinkage unless local shrinkage is much less
            WavCoeffs_ab[dir][coeffloc_ab] *=
                (SQR(sfaveabd[coeffloc_ab]) + SQR(sfab)) /
                (sfaveabd[coeffloc_ab] + sfab + eps);
        } // now chrominance coefficients are denoised

#else

        for (int i = 0; i < H_ab; ++i) {
            for (int j = 0; j < W_ab; ++j) {
                int coeffloc_ab = i * W_ab + j;
                float sfab = sfaveab[coeffloc_ab];

                // use smoothed shrinkage unless local shrinkage is much less
                WavCoeffs_ab[dir][coeffloc_ab] *=
                    (SQR(sfaveabd[coeffloc_ab]) + SQR(sfab)) /
                    (sfaveabd[coeffloc_ab] + sfab + eps);
            } // now chrominance coefficients are denoised
        }

#endif
    }
}

bool WaveletDenoiseAll_BiShrinkL(double scale,
                                 wavelet_decomposition &WaveletCoeffs_L,
                                 float *noisevarlum, float madL[8][3],
                                 int denoiseNestedLevels)
{
    int maxlvl = min(WaveletCoeffs_L.maxlevel(), 5);
    const float eps = 0.01f;

    int maxWL = 0, maxHL = 0;

    for (int lvl = 0; lvl < maxlvl; ++lvl) {
        if (WaveletCoeffs_L.level_W(lvl) > maxWL) {
            maxWL = WaveletCoeffs_L.level_W(lvl);
        }

        if (WaveletCoeffs_L.level_H(lvl) > maxHL) {
            maxHL = WaveletCoeffs_L.level_H(lvl);
        }
    }

#ifdef _OPENMP
#pragma omp parallel num_threads(                                              \
        denoiseNestedLevels) if (denoiseNestedLevels > 1)
#endif
    {
        float *buffer[3];
        buffer[0] = new /*(std::nothrow)*/ float[maxWL * maxHL + 32];
        buffer[1] = new /*(std::nothrow)*/ float[maxWL * maxHL + 64];
        buffer[2] = new /*(std::nothrow)*/ float[maxWL * maxHL + 96];

        // if (buffer[0] == nullptr || buffer[1] == nullptr || buffer[2] ==
        // nullptr) {

#ifdef _OPENMP
#pragma omp for schedule(dynamic) collapse(2)
#endif

        for (int lvl = maxlvl - 1; lvl >= 0;
             lvl--) { // for levels less than max, use level diff to make
                      // edge mask
            for (int dir = 1; dir < 4; ++dir) {
                int Wlvl_L = WaveletCoeffs_L.level_W(lvl);
                int Hlvl_L = WaveletCoeffs_L.level_H(lvl);

                float **WavCoeffs_L = WaveletCoeffs_L.level_coeffs(lvl);

                if (lvl == maxlvl - 1) {
                    int edge = 0;
                    ShrinkAllL(scale, WaveletCoeffs_L, buffer, lvl, dir,
                               noisevarlum, madL[lvl], nullptr, edge);
                } else {
                    // simple wavelet shrinkage
                    float *sfave = buffer[0] + 32;
                    float *sfaved = buffer[2] + 96;
                    float *blurBuffer = buffer[1] + 64;

                    float mad_Lr = madL[lvl][dir - 1];

                    float levelFactor = mad_Lr * 5.f / (lvl + 1);
#ifdef ART_SIMD
                    __m128 mad_Lv;
                    __m128 ninev = _mm_set1_ps(9.0f);
                    __m128 epsv = _mm_set1_ps(eps);
                    __m128 mag_Lv;
                    __m128 levelFactorv = _mm_set1_ps(levelFactor);
                    int coeffloc_L;

                    for (coeffloc_L = 0; coeffloc_L < Hlvl_L * Wlvl_L - 3;
                         coeffloc_L += 4) {
                        mad_Lv =
                            LVFU(noisevarlum[coeffloc_L]) * levelFactorv;
                        mag_Lv = SQRV(LVFU(WavCoeffs_L[dir][coeffloc_L]));
                        _mm_storeu_ps(
                            &sfave[coeffloc_L],
                            mag_Lv / (mag_Lv +
                                      mad_Lv * xexpf(-mag_Lv /
                                                     (mad_Lv * ninev)) +
                                      epsv));
                    }

                    for (; coeffloc_L < Hlvl_L * Wlvl_L; ++coeffloc_L) {
                        float mag_L = SQR(WavCoeffs_L[dir][coeffloc_L]);
                        sfave[coeffloc_L] =
                            mag_L /
                            (mag_L +
                             levelFactor * noisevarlum[coeffloc_L] *
                                 xexpf(-mag_L / (9.f * levelFactor *
                                                 noisevarlum[coeffloc_L])) +
                             eps);
                    }

#else

                    for (int i = 0; i < Hlvl_L; ++i) {
                        for (int j = 0; j < Wlvl_L; ++j) {

                            int coeffloc_L = i * Wlvl_L + j;
                            float mag_L = SQR(WavCoeffs_L[dir][coeffloc_L]);
                            sfave[coeffloc_L] =
                                mag_L /
                                (mag_L +
                                 levelFactor * noisevarlum[coeffloc_L] *
                                     xexpf(-mag_L /
                                           (9.f * levelFactor *
                                            noisevarlum[coeffloc_L])) +
                                 eps);
                        }
                    }

#endif
                    const int blur_rad = max(1, int((lvl + 2) / scale));
                    boxblur(sfave, sfaved, blurBuffer, blur_rad, blur_rad,
                            Wlvl_L, Hlvl_L); // increase smoothness by
                                             // locally averaging shrinkage
#ifdef ART_SIMD
                    __m128 sfavev;
                    __m128 sf_Lv;

                    for (coeffloc_L = 0; coeffloc_L < Hlvl_L * Wlvl_L - 3;
                         coeffloc_L += 4) {
                        sfavev = LVFU(sfaved[coeffloc_L]);
                        sf_Lv = LVFU(sfave[coeffloc_L]);
                        _mm_storeu_ps(&WavCoeffs_L[dir][coeffloc_L],
                                      LVFU(WavCoeffs_L[dir][coeffloc_L]) *
                                          (SQRV(sfavev) + SQRV(sf_Lv)) /
                                          (sfavev + sf_Lv + epsv));
                        // use smoothed shrinkage unless local shrinkage is
                        // much less
                    }

                    // few remaining pixels
                    for (; coeffloc_L < Hlvl_L * Wlvl_L; ++coeffloc_L) {
                        float sf_L = sfave[coeffloc_L];
                        // use smoothed shrinkage unless local shrinkage is
                        // much less
                        WavCoeffs_L[dir][coeffloc_L] *=
                            (SQR(sfaved[coeffloc_L]) + SQR(sf_L)) /
                            (sfaved[coeffloc_L] + sf_L + eps);
                    } // now luminance coeffs are denoised

#else

                    for (int i = 0; i < Hlvl_L; ++i) {
                        for (int j = 0; j < Wlvl_L; ++j) {
                            int coeffloc_L = i * Wlvl_L + j;
                            float sf_L = sfave[coeffloc_L];
                            // use smoothed shrinkage unless local shrinkage
                            // is much less
                            WavCoeffs_L[dir][coeffloc_L] *=
                                (SQR(sfaved[coeffloc_L]) + SQR(sf_L)) /
                                (sfaved[coeffloc_L] + sf_L + eps);
                        } // now luminance coeffs are denoised
                    }

#endif
                }
            }
        }

        for (int i = 2; i >= 0; i--) {
            if (buffer[i] != nullptr) {
                delete[] buffer[i];
            }
        }
    }
    return true;
}

bool WaveletDenoiseAll_BiShrinkAB(double scale,
                                  wavelet_decomposition &WaveletCoeffs_L,
                                  wavelet_decomposition &WaveletCoeffs_ab,
                                  float *noisevarchrom, float madL[8][3],
                                  float noisevar_ab, const bool useNoiseCCurve,
                                  bool autoch, int denoiseNestedLevels)
{
    int maxlvl = WaveletCoeffs_L.maxlevel();

    if (autoch && noisevar_ab <= 0.001f) {
        noisevar_ab = 0.02f;
    }

    float madab[8][3];

    int maxWL = 0, maxHL = 0;

    for (int lvl = 0; lvl < maxlvl; ++lvl) {
        if (WaveletCoeffs_L.level_W(lvl) > maxWL) {
            maxWL = WaveletCoeffs_L.level_W(lvl);
        }

        if (WaveletCoeffs_L.level_H(lvl) > maxHL) {
            maxHL = WaveletCoeffs_L.level_H(lvl);
        }
    }

#ifdef _OPENMP
#pragma omp parallel num_threads(                                              \
        denoiseNestedLevels) if (denoiseNestedLevels > 1)
#endif
    {
        float *buffer[3];
        buffer[0] = new /*(std::nothrow)*/ float[maxWL * maxHL + 32];
        buffer[1] = new /*(std::nothrow)*/ float[maxWL * maxHL + 64];
        buffer[2] = new /*(std::nothrow)*/ float[maxWL * maxHL + 96];

        // if (buffer[0] == nullptr || buffer[1] == nullptr || buffer[2] ==
        // nullptr) {

#ifdef _OPENMP
#pragma omp for schedule(dynamic) collapse(2)
#endif

        for (int lvl = 0; lvl < maxlvl; ++lvl) {
            for (int dir = 1; dir < 4; ++dir) {
                // compute median absolute deviation (MAD) of detail
                // coefficients as robust noise estimator
                int Wlvl_ab = WaveletCoeffs_ab.level_W(lvl);
                int Hlvl_ab = WaveletCoeffs_ab.level_H(lvl);
                float **WavCoeffs_ab = WaveletCoeffs_ab.level_coeffs(lvl);
                madab[lvl][dir - 1] =
                    SQR(MadRgb(WavCoeffs_ab[dir], Wlvl_ab * Hlvl_ab));
            }
        }

#ifdef _OPENMP
#pragma omp for schedule(dynamic) collapse(2)
#endif

        for (int lvl = maxlvl - 1; lvl >= 0;
             lvl--) { // for levels less than max, use level diff to make
                      // edge mask
            for (int dir = 1; dir < 4; ++dir) {
                int Wlvl_ab = WaveletCoeffs_ab.level_W(lvl);
                int Hlvl_ab = WaveletCoeffs_ab.level_H(lvl);

                float **WavCoeffs_L = WaveletCoeffs_L.level_coeffs(lvl);
                float **WavCoeffs_ab = WaveletCoeffs_ab.level_coeffs(lvl);

                if (lvl == maxlvl - 1) {
                    ShrinkAllAB(scale, WaveletCoeffs_L, WaveletCoeffs_ab,
                                buffer, lvl, dir, noisevarchrom,
                                noisevar_ab, useNoiseCCurve, autoch,
                                madL[lvl], madab[lvl], true);
                } else {
                    // simple wavelet shrinkage

                    float mad_Lr = madL[lvl][dir - 1];
                    float mad_abr =
                        useNoiseCCurve
                            ? noisevar_ab * madab[lvl][dir - 1]
                            : SQR(noisevar_ab) * madab[lvl][dir - 1];

                    if (noisevar_ab > 0.001f) {

#ifdef ART_SIMD
                        __m128 onev = _mm_set1_ps(1.f);
                        __m128 mad_abrv = _mm_set1_ps(mad_abr);
                        __m128 rmad_Lm9v = onev / _mm_set1_ps(mad_Lr * 9.f);
                        __m128 mad_abv;
                        __m128 mag_Lv, mag_abv;
                        __m128 tempabv;
                        int coeffloc_ab;

                        for (coeffloc_ab = 0;
                             coeffloc_ab < Hlvl_ab * Wlvl_ab - 3;
                             coeffloc_ab += 4) {
                            mad_abv =
                                LVFU(noisevarchrom[coeffloc_ab]) * mad_abrv;

                            tempabv = LVFU(WavCoeffs_ab[dir][coeffloc_ab]);
                            mag_Lv = LVFU(WavCoeffs_L[dir][coeffloc_ab]);
                            mag_abv = SQRV(tempabv);
                            mag_Lv = SQRV(mag_Lv) * rmad_Lm9v;
                            _mm_storeu_ps(
                                &WavCoeffs_ab[dir][coeffloc_ab],
                                tempabv * SQRV((onev -
                                                xexpf(-(mag_abv / mad_abv) -
                                                      (mag_Lv)))));
                        }

                        // few remaining pixels
                        for (; coeffloc_ab < Hlvl_ab * Wlvl_ab;
                             ++coeffloc_ab) {
                            float mag_L =
                                SQR(WavCoeffs_L[dir][coeffloc_ab]);
                            float mag_ab =
                                SQR(WavCoeffs_ab[dir][coeffloc_ab]);
                            WavCoeffs_ab[dir][coeffloc_ab] *= SQR(
                                1.f -
                                xexpf(
                                    -(mag_ab / (noisevarchrom[coeffloc_ab] *
                                                mad_abr)) -
                                    (mag_L /
                                     (9.f * mad_Lr))) /*satfactor_a*/);
                        } // now chrominance coefficients are denoised

#else

                        for (int i = 0; i < Hlvl_ab; ++i) {
                            for (int j = 0; j < Wlvl_ab; ++j) {
                                int coeffloc_ab = i * Wlvl_ab + j;

                                float mag_L =
                                    SQR(WavCoeffs_L[dir][coeffloc_ab]);
                                float mag_ab =
                                    SQR(WavCoeffs_ab[dir][coeffloc_ab]);

                                WavCoeffs_ab[dir][coeffloc_ab] *= SQR(
                                    1.f -
                                    xexpf(
                                        -(mag_ab /
                                          (noisevarchrom[coeffloc_ab] *
                                           mad_abr)) -
                                        (mag_L /
                                         (9.f * mad_Lr))) /*satfactor_a*/);
                            }
                        } // now chrominance coefficients are denoised

#endif
                    }
                }
            }
        }

        for (int i = 2; i >= 0; i--) {
            if (buffer[i] != nullptr) {
                delete[] buffer[i];
            }
        }
    }
    return true;
}

bool WaveletDenoiseAllL(double scale, wavelet_decomposition &WaveletCoeffs_L,
                        float *noisevarlum, float madL[8][3], float *vari,
                        int edge, int denoiseNestedLevels) // mod JD

{

    int maxlvl = min(WaveletCoeffs_L.maxlevel(), 5);

    if (edge == 1) {
        maxlvl = 4; // for refine denoise edge wavelet
    }

    int maxWL = 0, maxHL = 0;

    for (int lvl = 0; lvl < maxlvl; ++lvl) {
        if (WaveletCoeffs_L.level_W(lvl) > maxWL) {
            maxWL = WaveletCoeffs_L.level_W(lvl);
        }

        if (WaveletCoeffs_L.level_H(lvl) > maxHL) {
            maxHL = WaveletCoeffs_L.level_H(lvl);
        }
    }

#ifdef _OPENMP
#pragma omp parallel num_threads(                                              \
        denoiseNestedLevels) if (denoiseNestedLevels > 1)
#endif
    {
        float *buffer[4];
        buffer[0] = new /*(std::nothrow)*/ float[maxWL * maxHL + 32];
        buffer[1] = new /*(std::nothrow)*/ float[maxWL * maxHL + 64];
        buffer[2] = new /*(std::nothrow)*/ float[maxWL * maxHL + 96];
        buffer[3] = new /*(std::nothrow)*/ float[maxWL * maxHL + 128];

        // if (buffer[0] == nullptr || buffer[1] == nullptr || buffer[2] ==
        // nullptr || buffer[3] == nullptr) {

#ifdef _OPENMP
#pragma omp for schedule(dynamic) collapse(2)
#endif

        for (int lvl = 0; lvl < maxlvl; ++lvl) {
            for (int dir = 1; dir < 4; ++dir) {
                ShrinkAllL(scale, WaveletCoeffs_L, buffer, lvl, dir,
                           noisevarlum, madL[lvl], vari, edge);
            }
        }

        for (int i = 3; i >= 0; i--) {
            if (buffer[i] != nullptr) {
                delete[] buffer[i];
            }
        }
    }
    return true;
}

bool WaveletDenoiseAllAB(double scale, wavelet_decomposition &WaveletCoeffs_L,
                         wavelet_decomposition &WaveletCoeffs_ab,
                         float *noisevarchrom, float madL[8][3],
                         float noisevar_ab, const bool useNoiseCCurve,
                         bool autoch, int denoiseNestedLevels)

{

    int maxlvl = WaveletCoeffs_L.maxlevel();
    int maxWL = 0, maxHL = 0;

    for (int lvl = 0; lvl < maxlvl; ++lvl) {
        if (WaveletCoeffs_L.level_W(lvl) > maxWL) {
            maxWL = WaveletCoeffs_L.level_W(lvl);
        }

        if (WaveletCoeffs_L.level_H(lvl) > maxHL) {
            maxHL = WaveletCoeffs_L.level_H(lvl);
        }
    }

#ifdef _OPENMP
#pragma omp parallel num_threads(                                              \
        denoiseNestedLevels) if (denoiseNestedLevels > 1)
#endif
    {
        float *buffer[3];
        buffer[0] = new /*(std::nothrow)*/ float[maxWL * maxHL + 32];
        buffer[1] = new /*(std::nothrow)*/ float[maxWL * maxHL + 64];
        buffer[2] = new /*(std::nothrow)*/ float[maxWL * maxHL + 96];

        // if (buffer[0] == nullptr || buffer[1] == nullptr || buffer[2] ==
        // nullptr) {

#ifdef _OPENMP
#pragma omp for schedule(dynamic) collapse(2)
#endif

        for (int lvl = 0; lvl < maxlvl; ++lvl) {
            for (int dir = 1; dir < 4; ++dir) {
                ShrinkAllAB(scale, WaveletCoeffs_L, WaveletCoeffs_ab,
                            buffer, lvl, dir, noisevarchrom, noisevar_ab,
                            useNoiseCCurve, autoch, madL[lvl]);
            }
        }

        for (int i = 2; i >= 0; i--) {
            if (buffer[i] != nullptr) {
                delete[] buffer[i];
            }
        }
    }
    return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void ShrinkAll_info(float **WavCoeffs_a, float **WavCoeffs_b, int W_ab,
                    int H_ab, float **noisevarlum, float **noisevarchrom,
                    float **noisevarhue, float &chaut, int &Nb, float &redaut,
                    float &blueaut, float &maxredaut, float &maxblueaut,
                    float &minredaut, float &minblueaut, int schoice, int lvl,
                    float &chromina, float &sigma, float &lumema,
                    float &sigma_L, float &redyel, float &skinc, float &nsknc,
                    float &maxchred, float &maxchblue, float &minchred,
                    float &minchblue, int &nb, float &chau, float &chred,
                    float &chblue)
{

    // simple wavelet shrinkage
    if (lvl == 1) { // only one time
        float chro = 0.f;
        float dev = 0.f;
        float devL = 0.f;
        int nc = 0;
        int nL = 0;
        int nry = 0;
        float lume = 0.f;
        float red_yel = 0.f;
        float skin_c = 0.f;
        int nsk = 0;

        for (int i = 0; i < H_ab; ++i) {
            for (int j = 0; j < W_ab; ++j) {
                chro += noisevarchrom[i][j];
                ++nc;
                dev += SQR(noisevarchrom[i][j] - (chro / nc));

                if (noisevarhue[i][j] > -0.8f && noisevarhue[i][j] < 2.0f &&
                    noisevarchrom[i][j] > 10000.f) { // saturated red yellow
                    red_yel += noisevarchrom[i][j];
                    ++nry;
                }

                if (noisevarhue[i][j] > 0.f && noisevarhue[i][j] < 1.6f &&
                    noisevarchrom[i][j] < 10000.f) { // skin
                    skin_c += noisevarchrom[i][j];
                    ++nsk;
                }

                lume += noisevarlum[i][j];
                ++nL;
                devL += SQR(noisevarlum[i][j] - (lume / nL));
            }
        }

        if (nc > 0) {
            chromina = chro / nc;
            sigma = sqrt(dev / nc);
            nsknc = static_cast<float>(nsk) / static_cast<float>(nc);
        } else {
            nsknc = static_cast<float>(nsk);
        }

        if (nL > 0) {
            lumema = lume / nL;
            sigma_L = sqrt(devL / nL);
        }

        if (nry > 0) {
            redyel = red_yel / nry;
        }

        if (nsk > 0) {
            skinc = skin_c / nsk;
        }
    }

    const float reduc =
        (schoice == 2) ? static_cast<float>(0.9 /*settings->nrhigh*/) : 1.f;

    for (int dir = 1; dir < 4; ++dir) {
        float mada, madb;
        mada = SQR(MadRgb(WavCoeffs_a[dir], W_ab * H_ab));

        chred += mada;

        if (mada > maxchred) {
            maxchred = mada;
        }

        if (mada < minchred) {
            minchred = mada;
        }

        maxredaut = sqrt(reduc * maxchred);
        minredaut = sqrt(reduc * minchred);

        madb = SQR(MadRgb(WavCoeffs_b[dir], W_ab * H_ab));
        chblue += madb;

        if (madb > maxchblue) {
            maxchblue = madb;
        }

        if (madb < minchblue) {
            minchblue = madb;
        }

        maxblueaut = sqrt(reduc * maxchblue);
        minblueaut = sqrt(reduc * minchblue);

        chau += (mada + madb);
        ++nb;
        // here evaluation of automatic
        chaut = sqrt(reduc * chau / (nb + nb));
        redaut = sqrt(reduc * chred / nb);
        blueaut = sqrt(reduc * chblue / nb);
        Nb = nb;
    }
}

void WaveletDenoiseAll_info(
    int levwav, wavelet_decomposition &WaveletCoeffs_a,
    wavelet_decomposition &WaveletCoeffs_b, float **noisevarlum,
    float **noisevarchrom, float **noisevarhue, float &chaut, int &Nb,
    float &redaut, float &blueaut, float &maxredaut, float &maxblueaut,
    float &minredaut, float &minblueaut, int schoice, float &chromina,
    float &sigma, float &lumema, float &sigma_L, float &redyel, float &skinc,
    float &nsknc, float &maxchred, float &maxchblue, float &minchred,
    float &minchblue, int &nb, float &chau, float &chred, float &chblue)
{

    int maxlvl = levwav;

    for (int lvl = 0; lvl < maxlvl; ++lvl) {

        int Wlvl_ab = WaveletCoeffs_a.level_W(lvl);
        int Hlvl_ab = WaveletCoeffs_a.level_H(lvl);

        float **WavCoeffs_a = WaveletCoeffs_a.level_coeffs(lvl);
        float **WavCoeffs_b = WaveletCoeffs_b.level_coeffs(lvl);

        ShrinkAll_info(WavCoeffs_a, WavCoeffs_b, Wlvl_ab, Hlvl_ab, noisevarlum,
                       noisevarchrom, noisevarhue, chaut, Nb, redaut, blueaut,
                       maxredaut, maxblueaut, minredaut, minblueaut, schoice,
                       lvl, chromina, sigma, lumema, sigma_L, redyel, skinc,
                       nsknc, maxchred, maxchblue, minchred, minchblue, nb,
                       chau, chred, chblue);
    }
}

/* How many wavelet levels to decompose to.  Stronger chroma sliders and the
 * aggressive (QUALITY_HIGH) mode ask for more levels; the image's short side
 * and the preview scale cap it.  8 is the hard ceiling (cplx_wavelet_dec.h). */
int waveletLevels(float realred, float realblue, nrquality nrQuality,
                  double scale, int imwidth, int imheight)
{
    const float maxreal = max(realred, realblue);
    int levwav = maxreal < 8.f    ? 5
                 : maxreal < 10.f ? 6
                 : maxreal < 15.f ? 7
                                  : 8;

    if (nrQuality == QUALITY_HIGH) {
        levwav += 2; // settings->nrwavlevel; increase level for enhanced mode
    }

    levwav = min(8, levwav);
    levwav = max(5, int(levwav - std::ceil(std::log(scale))));

    const int minsizetile = min(imwidth, imheight);
    int maxlev2 = 8;

    if (minsizetile < 256) {
        maxlev2 = 7;
    }
    if (minsizetile < 128) {
        maxlev2 = 6;
    }
    if (minsizetile < 64) {
        maxlev2 = 5;
    }
    if (minsizetile < 32) {
        maxlev2 = 4;
    }
    if (minsizetile < 16) {
        maxlev2 = 3;
    }

    return min(maxlev2, levwav);
}

/* Phase 4, for one chroma channel.  In QUALITY_HIGH the bi-shrink pass runs
 * first and the plain pass runs over its output -- note madab is recomputed
 * inside ShrinkAllAB on the second pass, from the already-shrunk
 * coefficients, which is why the two passes are not idempotent. */
void shrinkChroma(const DenoiseContext &c, wavelet_decomposition &Ldecomp,
                  wavelet_decomposition &abdecomp, float madL[8][3],
                  float noisevar_ab, nrquality nrQuality, bool autoch)
{
    if (nrQuality == QUALITY_HIGH) {
        WaveletDenoiseAll_BiShrinkAB(c.scale, Ldecomp, abdecomp,
                                     c.noisevarchrom, madL, noisevar_ab,
                                     c.useNoiseCCurve, autoch,
                                     c.denoiseNestedLevels);
    }
    WaveletDenoiseAllAB(c.scale, Ldecomp, abdecomp, c.noisevarchrom, madL,
                        noisevar_ab, c.useNoiseCCurve, autoch,
                        c.denoiseNestedLevels);
}

/* Phases 2-6 with the planes on the host: decompose L/a/b, estimate the noise
 * level, shrink, reconstruct.
 *
 * The CPU implementation of the phases and nothing else -- no session, no
 * residency, no gpu:: identifier.  Both drivers call it: the GPU one when its
 * own on-device and host-plane attempts have declined.  It takes `levwav`
 * rather than recomputing it, so the two sides cannot disagree about the
 * level count.
 *
 * Ordering is forced by madL: it is computed from L's *original* detail
 * coefficients and is read by the a-, b- and L-shrinks alike, so L's bands
 * must not be modified until the chroma shrinks are done.  Hence
 * decompose(L) -> mad(L) -> {decompose,shrink,reconstruct}(a) ->
 * {...}(b) -> shrink(L) -> reconstruct(L).
 *
 * Lin is an out-parameter: phase 7 needs the *undenoised* L plane to form the
 * residual it runs the block DCT over, so it is snapshotted here, after the
 * luma shrink but before the reconstruction that overwrites labdn->L. */
void denoiseWaveletCPU(const DenoiseContext &c, LabImage *labdn,
                       array2D<float> *&Lin, int levwav, nrquality nrQuality,
                       bool autoch, bool denoiseLuminance)
{
    const int threads = max(1, c.denoiseNestedLevels);

    wavelet_decomposition *Ldecomp;
    {
        ART_PROFILE_SCOPE("denoise:wav:decompose");
        Ldecomp = new wavelet_decomposition(labdn->L[0], labdn->W, labdn->H,
                                            levwav, 1, 1, threads);
    }

    /* madL[level][dir]: squared median absolute deviation of L's detail
     * coefficients, the robust noise estimate every shrink below scales by. */
    float madL[8][3];
    {
        ART_PROFILE_SCOPE("denoise:wav:mad");
        const int maxlvl = Ldecomp->maxlevel();
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic) collapse(2)                         \
    num_threads(c.denoiseNestedLevels) if (c.denoiseNestedLevels > 1)
#endif
        for (int lvl = 0; lvl < maxlvl; ++lvl) {
            for (int dir = 1; dir < 4; ++dir) {
                const int n = Ldecomp->level_W(lvl) * Ldecomp->level_H(lvl);
                float **WavCoeffs_L = Ldecomp->level_coeffs(lvl);
                madL[lvl][dir - 1] = SQR(MadRgb(WavCoeffs_L[dir], n));
            }
        }
    }

    for (int ch = 0; ch < 2; ++ch) {
        float *plane = ch == 0 ? labdn->a[0] : labdn->b[0];
        const float noisevar_ab = ch == 0 ? c.noisevarab_r : c.noisevarab_b;

        wavelet_decomposition *abdecomp;
        {
            ART_PROFILE_SCOPE("denoise:wav:decompose");
            abdecomp = new wavelet_decomposition(plane, labdn->W, labdn->H,
                                                 levwav, 1, 1, threads);
        }
        {
            ART_PROFILE_SCOPE("denoise:wav:shrink-ab");
            shrinkChroma(c, *Ldecomp, *abdecomp, madL, noisevar_ab, nrQuality,
                         autoch);
        }
        {
            ART_PROFILE_SCOPE("denoise:wav:reconstruct");
            abdecomp->reconstruct(plane);
        }
        delete abdecomp;
    }

    if (denoiseLuminance) {
        {
            ART_PROFILE_SCOPE("denoise:wav:shrink-l");
            int edge = 0;
            if (nrQuality == QUALITY_HIGH) {
                WaveletDenoiseAll_BiShrinkL(c.scale, *Ldecomp, c.noisevarlum,
                                            madL, c.denoiseNestedLevels);
            }
            WaveletDenoiseAllL(c.scale, *Ldecomp, c.noisevarlum, madL, nullptr,
                               edge, c.denoiseNestedLevels);
        }
        {
            ART_PROFILE_SCOPE("denoise:wav:reconstruct");
            // snapshot L before reconstruction overwrites it -- phase 7 needs
            // the residual against the undenoised plane
            Lin = new array2D<float>(c.W, c.H);
#ifdef _OPENMP
#pragma omp parallel for num_threads(                                          \
        c.denoiseNestedLevels) if (c.denoiseNestedLevels > 1)
#endif
            for (int i = 0; i < c.H; ++i) {
                for (int j = 0; j < c.W; ++j) {
                    (*Lin)[i][j] = labdn->L[i][j];
                }
            }

            Ldecomp->reconstruct(labdn->L[0]);
        }
    }

    delete Ldecomp;
}

/* The GPU wavelet phases' parameter block.  Built in one place so the
 * on-device entry and the host-plane one cannot be handed different
 * parameters. */
gpu::ops::DenoiseWaveletGPU makeWaveletParams(const DenoiseContext &c,
                                              int levwav, nrquality nrQuality,
                                              bool autoch,
                                              bool denoiseLuminance)
{
    gpu::ops::DenoiseWaveletGPU wp;
    wp.levels = levwav;
    wp.scale = c.scale;
    wp.aggressive = nrQuality == QUALITY_HIGH;
    wp.autoch = autoch;
    wp.useNoiseCCurve = c.useNoiseCCurve;
    wp.denoiseLuminance = denoiseLuminance;
    wp.noisevarab_r = c.noisevarab_r;
    wp.noisevarab_b = c.noisevarab_b;
    wp.noisevarlum = c.noisevarlum;
    wp.noisevarchrom = c.noisevarchrom;
    return wp;
}

/* Phases 2-6 on the device with the planes on the host: uploads them, runs the
 * wavelet core, downloads the result.
 *
 * Lin has to be a host copy here, and it is taken *before* the call because
 * the call overwrites labdn->L in place.  It is the same plane the CPU path
 * snapshots after the shrink: the shrink works on the decomposition, not on
 * the plane.  Discarded if the call declines, so the CPU then takes its own
 * and the two paths' handling of Lin is identical rather than nearly so. */
bool denoiseWaveletGPUHost(const DenoiseContext &c, LabImage *labdn,
                           array2D<float> *&Lin,
                           const gpu::ops::DenoiseWaveletGPU &wp,
                           bool denoiseLuminance, gpu::BufferPool *pool,
                           gpu::Context *ctx)
{
    std::unique_ptr<array2D<float> > snapshot;
    if (denoiseLuminance) {
        snapshot.reset(new array2D<float>(c.W, c.H));
#ifdef _OPENMP
#pragma omp parallel for num_threads(                                          \
        c.denoiseNestedLevels) if (c.denoiseNestedLevels > 1)
#endif
        for (int i = 0; i < c.H; ++i) {
            for (int j = 0; j < c.W; ++j) {
                (*snapshot)[i][j] = labdn->L[i][j];
            }
        }
    }

    if (gpu::ops::denoiseWaveletGPU(labdn->W, labdn->H, labdn->L, labdn->a,
                                    labdn->b, wp, pool, ctx)) {
        Lin = snapshot.release();
        return true;
    }
    return false;
}

} // namespace denoise

namespace {

void adjust_params(procparams::DenoiseParams &dnparams, double scale)
{
    if (scale <= 1.0) {
        return;
    }

    const auto c = [](double x, double f) -> double {
        int s = SGN(x);
        double y = LIM01(std::abs(x) / 100.0);
        return s * intp(y, y * f, y) * 100.0;
    };

    double scale_factor = 1.0 / scale;
    double noise_factor_c = std::pow(scale_factor, 0.46);
    double noise_factor_l = std::pow(scale_factor, 0.62) * scale_factor;
    // noise_factor_l *= intp(std::pow(LIM01(dnparams.luminance / 100.0), 3.0),
    // scale_factor, 1.0); std::cout << "ADJUSTING LUMINANCE SCALE: " <<
    // noise_factor_l << std::endl; dnparams.luminance *= noise_factor_l;
    dnparams.luminance = c(dnparams.luminance, noise_factor_l);
    dnparams.luminanceDetail *= (1.0 + std::pow(1.0 - scale_factor, 2.2));
    dnparams.chrominance = c(dnparams.chrominance, noise_factor_c);
    dnparams.chrominanceRedGreen =
        c(dnparams.chrominanceRedGreen, noise_factor_c);
    dnparams.chrominanceBlueYellow =
        c(dnparams.chrominanceBlueYellow, noise_factor_c);
    // dnparams.chrominance *= noise_factor_c;
    // dnparams.chrominanceRedGreen *= noise_factor_c;
    // dnparams.chrominanceBlueYellow *= noise_factor_c;
}

void calcautodn_info(const ProcParams *params, float &chaut, float &delta,
                     int Nb, int levaut, float maxmax, float lumema,
                     float chromina, int mode, int lissage, float redyel,
                     float skinc, float nsknc)
{

    float reducdelta = 1.f;

    if (params->denoise.aggressive) {
        reducdelta = static_cast<float>(0.9 /*settings->nrhigh*/);
    }

    chaut =
        (chaut * Nb - maxmax) / (Nb - 1); // suppress maximum for chaut calcul

    if ((redyel > 5000.f || skinc > 1000.f) && nsknc < 0.4f &&
        chromina > 3000.f) {
        chaut *= 0.45f; // reduct action in red zone, except skin for high / med
                        // chroma
    } else if ((redyel > 12000.f || skinc > 1200.f) && nsknc < 0.3f &&
               chromina > 3000.f) {
        chaut *= 0.3f;
    }

    if (mode == 0 || mode == 2) { // Preview or Auto multizone
        if (chromina > 10000.f) {
            chaut *= 0.7f; // decrease action for high chroma  (visible noise)
        } else if (chromina > 6000.f) {
            chaut *= 0.9f;
        } else if (chromina < 3000.f) {
            chaut *= 1.2f; // increase action in low chroma==> 1.2  /==>2.0 ==>
                           // curve CC
        } else if (chromina < 2000.f) {
            chaut *= 1.5f; // increase action in low chroma==> 1.5 / ==>2.7
        }

        if (lumema < 2500.f) {
            chaut *= 1.3f; // increase action for low light
        } else if (lumema < 5000.f) {
            chaut *= 1.2f;
        } else if (lumema > 20000.f) {
            chaut *= 0.9f; // decrease for high light
        }
    } else if (mode == 1) { // auto ==> less coefficient because interaction
        if (chromina > 10000.f) {
            chaut *= 0.8f; // decrease action for high chroma  (visible noise)
        } else if (chromina > 6000.f) {
            chaut *= 0.9f;
        } else if (chromina < 3000.f) {
            chaut *= 1.5f; // increase action in low chroma
        } else if (chromina < 2000.f) {
            chaut *= 2.2f; // increase action in low chroma
        }

        if (lumema < 2500.f) {
            chaut *= 1.2f; // increase action for low light
        } else if (lumema < 5000.f) {
            chaut *= 1.1f;
        } else if (lumema > 20000.f) {
            chaut *= 0.9f; // decrease for high light
        }
    }

    if (levaut == 0) { // Low denoise
        if (chaut > 300.f) {
            chaut = 0.714286f * chaut + 85.71428f;
        }
    }

    delta = maxmax - chaut;
    delta *= reducdelta;

    if (lissage == 1 || lissage == 2) {
        if (chaut < 200.f && delta < 200.f) {
            delta *= 0.95f;
        } else if (chaut < 200.f && delta < 400.f) {
            delta *= 0.5f;
        } else if (chaut < 200.f && delta >= 400.f) {
            delta = 200.f;
        } else if (chaut < 400.f && delta < 400.f) {
            delta *= 0.4f;
        } else if (chaut < 400.f && delta >= 400.f) {
            delta = 120.f;
        } else if (chaut < 550.f) {
            delta *= 0.15f;
        } else if (chaut < 650.f) {
            delta *= 0.1f;
        } else { /*if (chaut >= 650.f)*/
            delta *= 0.07f;
        }

        if (mode == 0 || mode == 2) { // Preview or Auto multizone
            if (chromina < 6000.f) {
                delta *= 1.4f; // increase maxi
            }

            if (lumema < 5000.f) {
                delta *= 1.4f;
            }
        } else if (mode == 1) { // Auto
            if (chromina < 6000.f) {
                delta *= 1.2f; // increase maxi
            }

            if (lumema < 5000.f) {
                delta *= 1.2f;
            }
        }
    }

    if (lissage == 0) {
        if (chaut < 200.f && delta < 200.f) {
            delta *= 0.95f;
        } else if (chaut < 200.f && delta < 400.f) {
            delta *= 0.7f;
        } else if (chaut < 200.f && delta >= 400.f) {
            delta = 280.f;
        } else if (chaut < 400.f && delta < 400.f) {
            delta *= 0.6f;
        } else if (chaut < 400.f && delta >= 400.f) {
            delta = 200.f;
        } else if (chaut < 550.f) {
            delta *= 0.3f;
        } else if (chaut < 650.f) {
            delta *= 0.2f;
        } else { /*if (chaut >= 650.f)*/
            delta *= 0.15f;
        }

        if (mode == 0 || mode == 2) { // Preview or Auto multizone
            if (chromina < 6000.f) {
                delta *= 1.4f; // increase maxi
            }

            if (lumema < 5000.f) {
                delta *= 1.4f;
            }
        } else if (mode == 1) { // Auto
            if (chromina < 6000.f) {
                delta *= 1.2f; // increase maxi
            }

            if (lumema < 5000.f) {
                delta *= 1.2f;
            }
        }
    }
}

void RGB_denoise_infoGamCurve(const procparams::DenoiseParams &dnparams,
                              bool isRAW, LUTf &gamcurve, float &gam,
                              float &gamthresh, float &gamslope)
{
    gam = dnparams.gamma;
    gamthresh = 0.001f;

    if (!isRAW) { // reduce gamma under 1 for Lab mode ==> TIF and JPG
        if (gam < 1.9f) {
            gam = 1.f - (1.9f - gam) / 3.f; // minimum gamma 0.7
        } else if (gam >= 1.9f && gam <= 3.f) {
            gam = (1.4f / 1.1f) * gam - 1.41818f;
        }
    }

    gamslope = exp(log(static_cast<double>(gamthresh)) / gam) / gamthresh;
    Color::gammaf2lut(gamcurve, gam, gamthresh, gamslope, 65535.f, 32768.f);
}

void RGB_denoise_info(ImProcData &im, Imagefloat *src, Imagefloat *provicalc,
                      const bool isRAW, LUTf &gamcurve, float gam,
                      float gamthresh, float gamslope,
                      const procparams::DenoiseParams &dnparams,
                      const double expcomp, float &chaut, int &Nb,
                      float &redaut, float &blueaut, float &maxredaut,
                      float &maxblueaut, float &minredaut, float &minblueaut,
                      float &chromina, float &sigma, float &lumema,
                      float &sigma_L, float &redyel, float &skinc, float &nsknc)
{
    const ProcParams *params = im.params;
    double scale = im.scale;
    bool multiThread = im.multiThread;

    if (dnparams.chrominanceMethod !=
        procparams::DenoiseParams::ChrominanceMethod::AUTOMATIC) {
        // nothing to do
        return;
    }

    int hei, wid;
    float **lumcalc;
    float **acalc;
    float **bcalc;
    /* Both are read through the planar accessors below, and either may have
     * been left device-resident by a GPU operator -- provicalc in particular
     * has just been through imgsrc->convertColorSpace, which is one.  See the
     * note in denoise::RGB_denoise for what a missing sync here costs. */
    src->syncCpu();
    provicalc->syncCpu();

    hei = provicalc->getHeight();
    wid = provicalc->getWidth();
    TMatrix wprofi =
        ICCStore::getInstance()->workingSpaceMatrix(params->icm.workingProfile);

    const float wpi[3][3] = {
        {static_cast<float>(wprofi[0][0]), static_cast<float>(wprofi[0][1]),
         static_cast<float>(wprofi[0][2])},
        {static_cast<float>(wprofi[1][0]), static_cast<float>(wprofi[1][1]),
         static_cast<float>(wprofi[1][2])},
        {static_cast<float>(wprofi[2][0]), static_cast<float>(wprofi[2][1]),
         static_cast<float>(wprofi[2][2])}};

    lumcalc = new float *[hei];

    for (int i = 0; i < hei; ++i) {
        lumcalc[i] = new float[wid];
    }

    acalc = new float *[hei];

    for (int i = 0; i < hei; ++i) {
        acalc[i] = new float[wid];
    }

    bcalc = new float *[hei];

    for (int i = 0; i < hei; ++i) {
        bcalc[i] = new float[wid];
    }

#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif

    for (int ii = 0; ii < hei; ++ii) {
        for (int jj = 0; jj < wid; ++jj) {
            float LLum, AAum, BBum;
            float RL = provicalc->r(ii, jj);
            float GL = provicalc->g(ii, jj);
            float BL = provicalc->b(ii, jj);
            // determine luminance for noisecurve
            float XL, YL, ZL;
            Color::rgbxyz(RL, GL, BL, XL, YL, ZL, wpi);
            Color::XYZ2Lab(XL, YL, ZL, LLum, AAum, BBum);
            lumcalc[ii][jj] = LLum;
            acalc[ii][jj] = AAum;
            bcalc[ii][jj] = BBum;
        }
    }

    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

    const int imheight = src->getHeight(), imwidth = src->getWidth();
    const float gain = pow(2.0f, float(expcomp));

    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

    TMatrix wprof =
        ICCStore::getInstance()->workingSpaceMatrix(params->icm.workingProfile);
    const float wp[3][3] = {
        {static_cast<float>(wprof[0][0]), static_cast<float>(wprof[0][1]),
         static_cast<float>(wprof[0][2])},
        {static_cast<float>(wprof[1][0]), static_cast<float>(wprof[1][1]),
         static_cast<float>(wprof[1][2])},
        {static_cast<float>(wprof[2][0]), static_cast<float>(wprof[2][1]),
         static_cast<float>(wprof[2][2])}};

    float chau = 0.f;
    float chred = 0.f;
    float chblue = 0.f;
    float maxchred = 0.f;
    float maxchblue = 0.f;
    float minchred = 100000000.f;
    float minchblue = 100000000.f;
    int nb = 0;
    int comptlevel = 0;

    {
        {
            const int tiletop = 0, tileleft = 0;
            const int tileright = imwidth, tilebottom = imheight;
            const int width = imwidth, height = imheight;
            LabImage *labdn = new LabImage(width, height);
            float **noisevarlum = new float *[(height + 1) / 2];

            for (int i = 0; i < (height + 1) / 2; ++i) {
                noisevarlum[i] = new float[(width + 1) / 2];
            }

            float **noisevarchrom = new float *[(height + 1) / 2];

            for (int i = 0; i < (height + 1) / 2; ++i) {
                noisevarchrom[i] = new float[(width + 1) / 2];
            }

            float **noisevarhue = new float *[(height + 1) / 2];

            for (int i = 0; i < (height + 1) / 2; ++i) {
                noisevarhue[i] = new float[(width + 1) / 2];
            }

            float realred, realblue;
            float interm_med =
                1.5f; // static_cast<float>(dnparams.chrominance) / 10.0;
            float intermred, intermblue;

            intermred = 0.f;
            intermblue = 0.f;
            // if (dnparams.chrominanceRedGreen > 0.) {
            //     intermred = (dnparams.chrominanceRedGreen / 10.);
            // } else {
            //     intermred = static_cast<float>(dnparams.chrominanceRedGreen)
            //     / 7.0;     //increase slower than linear for more sensit
            // }

            // if (dnparams.chrominanceBlueYellow > 0.) {
            //     intermblue = (dnparams.chrominanceBlueYellow / 10.);
            // } else {
            //     intermblue =
            //     static_cast<float>(dnparams.chrominanceBlueYellow) / 7.0;
            //     //increase slower than linear for more sensit
            // }

            realred = interm_med + intermred;

            if (realred < 0.f) {
                realred = 0.001f;
            }

            realblue = interm_med + intermblue;

            if (realblue < 0.f) {
                realblue = 0.001f;
            }

            // fill tile from image; convert RGB to "luma/chroma"

            if (isRAW) { // image is raw; use channel differences for chroma
                         // channels
#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif

                for (int i = tiletop; i < tilebottom; i += 2) {
                    int i1 = i - tiletop;
#ifdef ART_SIMD
                    __m128 aNv, bNv;
                    __m128 c100v = _mm_set1_ps(100.f);
                    int j;

                    for (j = tileleft; j < tileright - 7; j += 8) {
                        int j1 = j - tileleft;
                        aNv = LVFU(acalc[i >> 1][j >> 1]);
                        bNv = LVFU(bcalc[i >> 1][j >> 1]);
                        _mm_storeu_ps(&noisevarhue[i1 >> 1][j1 >> 1],
                                      xatan2f(bNv, aNv));
                        _mm_storeu_ps(
                            &noisevarchrom[i1 >> 1][j1 >> 1],
                            vmaxf(vsqrtf(SQRV(aNv) + SQRV(bNv)), c100v));
                    }

                    for (; j < tileright; j += 2) {
                        int j1 = j - tileleft;
                        float aN = acalc[i >> 1][j >> 1];
                        float bN = bcalc[i >> 1][j >> 1];
                        float cN = sqrtf(SQR(aN) + SQR(bN));
                        noisevarhue[i1 >> 1][j1 >> 1] = xatan2f(bN, aN);

                        if (cN < 100.f) {
                            cN = 100.f; // avoid divided by zero
                        }

                        noisevarchrom[i1 >> 1][j1 >> 1] = cN;
                    }

#else

                    for (int j = tileleft; j < tileright; j += 2) {
                        int j1 = j - tileleft;
                        float aN = acalc[i >> 1][j >> 1];
                        float bN = bcalc[i >> 1][j >> 1];
                        float cN = sqrtf(SQR(aN) + SQR(bN));
                        float hN = xatan2f(bN, aN);

                        if (cN < 100.f) {
                            cN = 100.f; // avoid divided by zero
                        }

                        noisevarchrom[i1 >> 1][j1 >> 1] = cN;
                        noisevarhue[i1 >> 1][j1 >> 1] = hN;
                    }

#endif
                }

#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif

                for (int i = tiletop; i < tilebottom; i += 2) {
                    int i1 = i - tiletop;

                    for (int j = tileleft; j < tileright; j += 2) {
                        int j1 = j - tileleft;
                        float Llum = lumcalc[i >> 1][j >> 1];
                        Llum =
                            Llum < 2.f ? 2.f : Llum; // avoid divided by zero ?
                        Llum = Llum > 32768.f ? 32768.f
                                              : Llum; // not strictly necessary
                        noisevarlum[i1 >> 1][j1 >> 1] = Llum;
                    }
                }

                for (int i = tiletop /*, i1=0*/; i < tilebottom;
                     ++i /*, ++i1*/) {
                    int i1 = i - tiletop;

                    for (int j = tileleft /*, j1=0*/; j < tileright;
                         ++j /*, ++j1*/) {
                        int j1 = j - tileleft;

                        float X = gain * src->r(i, j);
                        float Y = gain * src->g(i, j);
                        float Z = gain * src->b(i, j);

                        X = X < 65535.f ? gamcurve[X]
                                        : (Color::gammaf(X / 65535.f, gam,
                                                         gamthresh, gamslope) *
                                           32768.f);
                        Y = Y < 65535.f ? gamcurve[Y]
                                        : (Color::gammaf(Y / 65535.f, gam,
                                                         gamthresh, gamslope) *
                                           32768.f);
                        Z = Z < 65535.f ? gamcurve[Z]
                                        : (Color::gammaf(Z / 65535.f, gam,
                                                         gamthresh, gamslope) *
                                           32768.f);

                        // labdn->a[i1][j1] = (X - Y);
                        // labdn->b[i1][j1] = (Y - Z);
                        float l, u, v;
                        Color::rgb2yuv(X, Y, Z, l, u, v, wp);
                        labdn->a[i1][j1] = v;
                        labdn->b[i1][j1] = u;
                    }
                }
            } else { // image is not raw; use Lab parametrization
                for (int i = tiletop /*, i1=0*/; i < tilebottom;
                     ++i /*, ++i1*/) {
                    int i1 = i - tiletop;

                    for (int j = tileleft /*, j1=0*/; j < tileright;
                         ++j /*, ++j1*/) {
                        int j1 = j - tileleft;
                        // float L, a, b;
                        float rLum = src->r(i, j); // for luminance denoise
                                                   // curve
                        float gLum = src->g(i, j);
                        float bLum = src->b(i, j);

                        // use gamma sRGB, not good if TIF (JPG) Output profil
                        // not with gamma sRGB  (eg : gamma =1.0, or 1.8...)
                        // very difficult to solve !
                        //  solution ==> save TIF with gamma sRGB and re open
                        float rtmp = Color::igammatab_srgb[src->r(i, j)];
                        float gtmp = Color::igammatab_srgb[src->g(i, j)];
                        float btmp = Color::igammatab_srgb[src->b(i, j)];
                        // modification Jacques feb 2013
                        //  gamma slider different from raw
                        rtmp = rtmp < 65535.f
                                   ? gamcurve[rtmp]
                                   : (Color::gammanf(rtmp / 65535.f, gam) *
                                      32768.f);
                        gtmp = gtmp < 65535.f
                                   ? gamcurve[gtmp]
                                   : (Color::gammanf(gtmp / 65535.f, gam) *
                                      32768.f);
                        btmp = btmp < 65535.f
                                   ? gamcurve[btmp]
                                   : (Color::gammanf(btmp / 65535.f, gam) *
                                      32768.f);

                        // float X, Y, Z;
                        // Color::rgbxyz(rtmp, gtmp, btmp, X, Y, Z, wp);

                        // //convert Lab
                        // Color::XYZ2Lab(X, Y, Z, L, a, b);
                        float Y, u, v;
                        Color::rgb2yuv(rtmp, gtmp, btmp, Y, u, v, wp);

                        if (((i1 | j1) & 1) == 0) {
                            float Llum, alum, blum;
                            float XL, YL, ZL;
                            Color::rgbxyz(rLum, gLum, bLum, XL, YL, ZL, wp);
                            Color::XYZ2Lab(XL, YL, ZL, Llum, alum, blum);
                            float kN = Llum;

                            if (kN < 2.f) {
                                kN = 2.f;
                            }

                            if (kN > 32768.f) {
                                kN = 32768.f;
                            }

                            noisevarlum[i1 >> 1][j1 >> 1] = kN;
                            float aN = alum;
                            float bN = blum;
                            float hN = xatan2f(bN, aN);
                            float cN = sqrt(SQR(aN) + SQR(bN));

                            if (cN < 100.f) {
                                cN = 100.f; // avoid divided by zero
                            }

                            noisevarchrom[i1 >> 1][j1 >> 1] = cN;
                            noisevarhue[i1 >> 1][j1 >> 1] = hN;
                        }

                        labdn->a[i1][j1] = v;
                        labdn->b[i1][j1] = u;
                    }
                }
            }

            int datalen = labdn->W * labdn->H;

            // now perform basic wavelet denoise
            // last two arguments of wavelet decomposition are max number of
            // wavelet decomposition levels; and whether to subsample the image
            // after wavelet filtering.  Subsampling is coded as binary 1 or 0
            // for each level, eg subsampling = 0 means no subsampling, 1 means
            // subsample the first level only, 7 means subsample the first three
            // levels, etc.

            wavelet_decomposition *adecomp;
            wavelet_decomposition *bdecomp;

            int schoice = 0; // shrink method

            if (dnparams.aggressive) {
                schoice = 2;
            }

            const int levwav = max(2, int(5 - std::ceil(std::log(scale))));
#ifdef _OPENMP
#pragma omp parallel sections if (multiThread)
#endif
            {
#ifdef _OPENMP
#pragma omp section
#endif
                {
                    adecomp = new wavelet_decomposition(
                        labdn->data + datalen, labdn->W, labdn->H, levwav, 1);
                }
#ifdef _OPENMP
#pragma omp section
#endif
                {
                    bdecomp = new wavelet_decomposition(
                        labdn->data + 2 * datalen, labdn->W, labdn->H, levwav,
                        1);
                }
            }

            if (comptlevel == 0) {
                denoise::WaveletDenoiseAll_info(
                    levwav, *adecomp, *bdecomp, noisevarlum, noisevarchrom,
                    noisevarhue, chaut, Nb, redaut, blueaut, maxredaut,
                    maxblueaut, minredaut, minblueaut, schoice, chromina, sigma,
                    lumema, sigma_L, redyel, skinc, nsknc, maxchred, maxchblue,
                    minchred, minchblue, nb, chau, chred,
                    chblue); // Enhance mode
            }

            comptlevel += 1;
            delete adecomp;
            delete bdecomp;
            delete labdn;

            for (int i = 0; i < (height + 1) / 2; ++i) {
                delete[] noisevarlum[i];
            }

            delete[] noisevarlum;

            for (int i = 0; i < (height + 1) / 2; ++i) {
                delete[] noisevarchrom[i];
            }

            delete[] noisevarchrom;

            for (int i = 0; i < (height + 1) / 2; ++i) {
                delete[] noisevarhue[i];
            }

            delete[] noisevarhue;

        } // end of tile row
    } // end of tile loop

    for (int i = 0; i < hei; ++i) {
        delete[] lumcalc[i];
    }

    delete[] lumcalc;

    for (int i = 0; i < hei; ++i) {
        delete[] acalc[i];
    }

    delete[] acalc;

    for (int i = 0; i < hei; ++i) {
        delete[] bcalc[i];
    }

    delete[] bcalc;

#undef TS
// #undef fTS
#undef offset
#undef epsilon

} // End of main RGB_denoise

} // namespace

namespace denoise {

NoiseCurve::NoiseCurve(): sum(0.f) {}

void NoiseCurve::Reset()
{
    lutNoiseCurve.reset();
    sum = 0.f;
}

void NoiseCurve::Set(const Curve &pCurve)
{
    if (pCurve.isIdentity()) {
        Reset(); // raise this value if the quality suffers from this number of
                 // samples
        return;
    }

    lutNoiseCurve(501); // raise this value if the quality suffers from this
                        // number of samples
    sum = 0.f;

    for (int i = 0; i < 501; i++) {
        lutNoiseCurve[i] = pCurve.getVal(double(i) / 500.);

        if (lutNoiseCurve[i] < 0.01f) {
            lutNoiseCurve[i] = 0.01f; // avoid 0.f for wavelet : under 0.01f
                                      // quasi no action for each value
        }

        sum += lutNoiseCurve[i]; // minima for Wavelet about 6.f or 7.f quasi no
                                 // action
    }

    // lutNoisCurve.dump("Nois");
}

void NoiseCurve::Set(const std::vector<double> &curvePoints)
{

    if (!curvePoints.empty() && curvePoints[0] > FCT_Linear &&
        curvePoints[0] < FCT_Unchanged) {
        FlatCurve tcurve(curvePoints, false, CURVES_MIN_POLY_POINTS / 2);
        tcurve.setIdentityValue(0.);
        Set(tcurve);
    } else {
        Reset();
    }
}

} // namespace denoise

void ImProcFunctions::DenoiseInfoStore::reset()
{
    chM = 0;
    for (int i = 0; i < 9; ++i) {
        max_r[i] = 0.f;
        max_b[i] = 0.f;
        ch_M[i] = 0.f;
    }
    valid = false;
    DenoiseParams p;
    chrominance = p.chrominance;
    chrominanceRedGreen = p.chrominanceRedGreen;
    chrominanceBlueYellow = p.chrominanceBlueYellow;
}

bool ImProcFunctions::DenoiseInfoStore::update_pparams(
    const procparams::ProcParams &p)
{
    if (!valid) {
        // std::cout << "** INVALID ** " << std::endl;
        pparams = p;
        return false;
    } else {
        const auto &d1 = pparams.denoise;
        const auto &d2 = p.denoise;
        const auto dn_eq = [&]() -> bool {
            return (d1.enabled == d2.enabled) &&
                   (d1.colorSpace == d2.colorSpace) &&
                   (d1.aggressive == d2.aggressive) && (d1.gamma == d2.gamma);
        };
        const auto &w1 = pparams.wb;
        const auto &w2 = p.wb;
        const auto wb_eq = [&]() -> bool {
            if (w1.enabled == w2.enabled && w1.method == w2.method &&
                (w1.method == procparams::WBParams::CAMERA ||
                 w1.method == procparams::WBParams::AUTO)) {
                return true;
            }
            return w1 == w2;
        };
        const auto &e1 = pparams.exposure;
        const auto &e2 = p.exposure;
        const auto exposure_eq = [&]() -> bool {
            return e1.enabled == e2.enabled && e1.hrmode == e2.hrmode;
        };
        const auto &r1 = pparams.raw;
        const auto &r2 = p.raw;
        const auto raw_eq = [&]() -> bool {
            auto r2b = r2;
#define MK_EQ_(k) r2b.k = r1.k
            MK_EQ_(bayersensor.method);
            MK_EQ_(bayersensor.lmmse_iterations);
            MK_EQ_(bayersensor.dualDemosaicAutoContrast);
            MK_EQ_(bayersensor.dualDemosaicContrast);
            MK_EQ_(xtranssensor.method);
#undef MK_EQ_
            return r1 == r2b;
        };
        const bool changed =
            !dn_eq() || !wb_eq() || !exposure_eq() || !raw_eq();
        // if (changed) {
        //     std::cout << "** CHANGED " << std::endl;
        // }
        pparams = p;
        return !changed;
    }
}

namespace denoise {

/* Everything the nine sample crops share, plus the eleven numbers each of them
 * produces.
 *
 * The two crop loops differ structurally -- the CPU one runs nine-way under
 * OpenMP with per-thread buffers, the device one runs sequentially with a
 * single pair reused -- so they are two functions rather than a flag inside
 * one, and this is what they have in common.  `store` is written directly for
 * ch_M/max_r/max_b, exactly as the loop always did; the other eight arrays
 * live here because nothing outside the reduction reads them. */
struct CropAnalysisJob {
    CropAnalysisJob():
        ipf(nullptr), imgsrc(nullptr), currWB(nullptr), params(nullptr),
        dnparams(nullptr), store(nullptr), multiThread(true), tr(0), crW(0),
        crH(0), levwav_info(0), info_expcomp(0.0), gamcurve(65536, 0),
        gam(1.f), gamthresh(0.001f), gamslope(1.f)
    {
    }

    ImProcFunctions *ipf;
    ImageSource *imgsrc;
    const ColorTemp *currWB;
    const ProcParams *params;
    const procparams::DenoiseParams *dnparams;
    ImProcFunctions::DenoiseInfoStore *store;

    bool multiThread;
    int tr;
    int crW, crH;
    int coordW[3], coordH[3];
    int levwav_info;
    double info_expcomp;

    LUTf gamcurve;
    float gam, gamthresh, gamslope;

    // outputs, one slot per crop
    int Nb[9];
    float min_r[9], min_b[9], lumL[9], chromC[9], ry[9], sk[9], pcsk[9];

private:
    CropAnalysisJob(const CropAnalysisJob &);
    CropAnalysisJob &operator=(const CropAnalysisJob &);
};

/* Decode one sample crop, build the quarter-size image the chroma statistics
 * need, and colour-convert it.  Shared by both loops: this half is the same
 * work whichever backend then analyses it.
 *
 * The two residency calls here are required on BOTH paths and must not be
 * mistaken for GPU-analysis bookkeeping: getImage and convertColorSpace are
 * themselves ported operators, so they run on the device regardless of which
 * analysis follows.  See the comments at each call. */
void prepareCropForAnalysis(const CropAnalysisJob &j, int wcr, int hcr,
                            Imagefloat *origCropPart, Imagefloat *provicalc)
{
    PreviewProps ppP(j.coordW[wcr], j.coordH[hcr], j.crW, j.crH, 1);
    {
        /* Raw decode + demosaic of one sample crop, not denoise math.  Scoped
         * separately because whether this or the analysis dominates decides
         * whether denoiseComputeParams is worth porting at all. */
        ART_PROFILE_SCOPE("denoise:params:getImage");
        j.imgsrc->getImage(*j.currWB, j.tr, origCropPart, ppP,
                           j.params->exposure, j.params->raw);
    }

    /* Read through the planar accessors below, so it has to be on the host:
     * getImage's own tail runs GPU operators and can leave the crop
     * device-resident. */
    origCropPart->syncCpu();

    /* The subsample loop below writes provicalc's planes directly, so the
     * device copy has to be marked stale.  Without this the ported
     * convertColorSpace's forWrite() sees Loc::BOTH -- left there by the
     * previous iteration's download -- and returns the stale buffer without
     * uploading, so it converts the *previous* crop.  provicalc is reused
     * across the crops one thread gets, which is why the symptom scaled with
     * the thread count: at nine crops over ten threads almost every thread
     * handles one crop and nothing goes wrong, and the sequential path hits it
     * on eight crops out of nine.
     *
     * invalidateGPU and not syncCpuForWrite because the loop overwrites every
     * pixel of provicalc (its dimensions are exactly the loop's extent, for
     * odd and even crops alike), so the download syncCpuForWrite does first is
     * pure cost. */
    provicalc->residency().invalidateGPU();

    ART_PROFILE_SCOPE_NAMED(prof_sub, "denoise:params:sub");
    // we only need image reduced to 1/4 here
    for (int ii = 0; ii < j.crH; ii += 2) {
        for (int jj = 0; jj < j.crW; jj += 2) {
            provicalc->r(ii >> 1, jj >> 1) = origCropPart->r(ii, jj);
            provicalc->g(ii >> 1, jj >> 1) = origCropPart->g(ii, jj);
            provicalc->b(ii >> 1, jj >> 1) = origCropPart->b(ii, jj);
        }
    }

    prof_sub.stop();

    {
        ART_PROFILE_SCOPE("denoise:params:ccs");
        j.imgsrc->convertColorSpace(provicalc, j.params->icm, *j.currWB);
    }
}

/* Record one crop's eleven numbers.  pondcorrec was a hardcoded 1.0f. */
void storeCropStats(CropAnalysisJob &j, int wcr, int hcr, int nb, float chaut,
                    float maxredaut, float maxblueaut, float minredaut,
                    float minblueaut, float chromina, float lumema,
                    float redyel, float skinc, float nsknc)
{
    const int k = hcr * 3 + wcr;
    j.Nb[k] = nb;
    j.store->ch_M[k] = chaut;
    j.store->max_r[k] = maxredaut;
    j.store->max_b[k] = maxblueaut;
    j.min_r[k] = minredaut;
    j.min_b[k] = minblueaut;
    j.lumL[k] = lumema;
    j.chromC[k] = chromina;
    j.ry[k] = redyel;
    j.sk[k] = skinc;
    j.pcsk[k] = nsknc;
}

/* One crop, analysed on the host. */
void analyseCropCPU(CropAnalysisJob &j, int wcr, int hcr,
                    Imagefloat *origCropPart, Imagefloat *provicalc)
{
    prepareCropForAnalysis(j, wcr, hcr, origCropPart, provicalc);

    float chaut = 0.f, redaut = 0.f, blueaut = 0.f, maxredaut = 0.f,
          maxblueaut = 0.f, minredaut = 0.f, minblueaut = 0.f, chromina = 0.f,
          sigma = 0.f, lumema = 0.f, sigma_L = 0.f, redyel = 0.f, skinc = 0.f,
          nsknc = 0.f;
    int nb = 0;

    ART_PROFILE_SCOPE_NAMED(prof_info, "denoise:params:info");
    ImProcData im(j.params, 1.f /*scale*/, j.multiThread);
    RGB_denoise_info(im, origCropPart, provicalc, j.imgsrc->isRAW(), j.gamcurve,
                     j.gam, j.gamthresh, j.gamslope, *j.dnparams,
                     j.info_expcomp, chaut, nb, redaut, blueaut, maxredaut,
                     maxblueaut, minredaut, minblueaut, chromina, sigma, lumema,
                     sigma_L, redyel, skinc, nsknc);
    prof_info.stop();

    storeCropStats(j, wcr, hcr, nb, chaut, maxredaut, maxblueaut, minredaut,
                   minblueaut, chromina, lumema, redyel, skinc, nsknc);
}

/* One crop, analysed on the device.  False means it declined and the caller
 * should run analyseCropCPU for this crop instead. */
bool analyseCropGPU(CropAnalysisJob &j, int wcr, int hcr,
                    Imagefloat *origCropPart, Imagefloat *provicalc)
{
    ART_PROFILE_SCOPE_NAMED(prof_info, "denoise:params:info");
    gpu::ops::DenoiseInfoGPU g;
    g.levels = j.levwav_info;
    g.aggressive = j.dnparams->aggressive;
    g.gain = pow(2.0f, float(j.info_expcomp));
    g.gam = j.gam;
    g.gamthresh = j.gamthresh;
    g.gamslope = j.gamslope;
    TMatrix wp = ICCStore::getInstance()->workingSpaceMatrix(
        j.params->icm.workingProfile);
    for (int i = 0; i < 3; ++i) {
        for (int k = 0; k < 3; ++k) {
            g.ws[i][k] = static_cast<float>(wp[i][k]);
        }
    }

    gpu::ops::DenoiseInfoResult r;
    if (!gpu::ops::denoiseInfo(origCropPart, provicalc, g, r,
                               j.ipf->getGPUPool(), j.ipf->getGPUContext())) {
        return false;
    }
    prof_info.stop();

    storeCropStats(j, wcr, hcr, r.Nb, r.chaut, r.maxredaut, r.maxblueaut,
                   r.minredaut, r.minblueaut, r.chromina, r.lumema, r.redyel,
                   r.skinc, r.nsknc);
    return true;
}

/* The nine crops on the host: nine-way over the crops and single-threaded
 * within each one, since each runs inside an OpenMP region and its own
 * `omp parallel for`s collapse to one thread. */
void computeParamsCPU(CropAnalysisJob &j)
{
#ifdef _OPENMP
#pragma omp parallel if (j.multiThread)
#endif
    {
        Imagefloat *origCropPart =
            new Imagefloat(j.crW, j.crH); // allocate memory
        Imagefloat *provicalc = new Imagefloat(
            (j.crW + 1) / 2, (j.crH + 1) / 2); // for denoise curves

#ifdef _OPENMP
#pragma omp for schedule(dynamic) collapse(2) nowait
#endif
        for (int wcr = 0; wcr <= 2; wcr++) {
            for (int hcr = 0; hcr <= 2; hcr++) {
                analyseCropCPU(j, wcr, hcr, origCropPart, provicalc);
            }
        }

        delete provicalc;
        delete origCropPart;
    }
}

/* The nine crops on the device.  False means the device analysis is not usable
 * at all and nothing was written, so the caller runs computeParamsCPU.
 *
 * Sequential, since the queue is, and that is not a concession: nine
 * concurrent copies of this contend badly for memory bandwidth (one crop's
 * analysis measures 128 ms alone and 245 ms nine-up, so nine threads buy 2.4x,
 * not 9x) and hold nine sets of intermediates at once.  Running them one at a
 * time also gives getImage the whole machine, and lets one pair of buffers be
 * reused across all nine.
 *
 * A single crop's device analysis declining is not a reason to abandon the
 * loop: that crop falls back to the host, which is a pure reduction with no
 * residency consequences. */
bool computeParamsGPU(CropAnalysisJob &j)
{
    if (!gpu::ops::denoiseInfoUsable(j.crW, j.crH, j.levwav_info,
                                     j.imgsrc->isRAW(),
                                     j.ipf->getGPUContext())) {
        return false;
    }

    Imagefloat origCropPart(j.crW, j.crH);
    Imagefloat provicalc((j.crW + 1) / 2, (j.crH + 1) / 2);
    for (int wcr = 0; wcr <= 2; ++wcr) {
        for (int hcr = 0; hcr <= 2; ++hcr) {
            prepareCropForAnalysis(j, wcr, hcr, &origCropPart, &provicalc);
            if (!analyseCropGPU(j, wcr, hcr, &origCropPart, &provicalc)) {
                float chaut = 0.f, redaut = 0.f, blueaut = 0.f,
                      maxredaut = 0.f, maxblueaut = 0.f, minredaut = 0.f,
                      minblueaut = 0.f, chromina = 0.f, sigma = 0.f,
                      lumema = 0.f, sigma_L = 0.f, redyel = 0.f, skinc = 0.f,
                      nsknc = 0.f;
                int nb = 0;
                ImProcData im(j.params, 1.f /*scale*/, j.multiThread);
                RGB_denoise_info(im, &origCropPart, &provicalc,
                                 j.imgsrc->isRAW(), j.gamcurve, j.gam,
                                 j.gamthresh, j.gamslope, *j.dnparams,
                                 j.info_expcomp, chaut, nb, redaut, blueaut,
                                 maxredaut, maxblueaut, minredaut, minblueaut,
                                 chromina, sigma, lumema, sigma_L, redyel,
                                 skinc, nsknc);
                storeCropStats(j, wcr, hcr, nb, chaut, maxredaut, maxblueaut,
                               minredaut, minblueaut, chromina, lumema, redyel,
                               skinc, nsknc);
            }
        }
    }
    return true;
}

/* Nine crops' statistics -> the three chrominance sliders.  Pure arithmetic,
 * identical on both paths, and shared rather than duplicated: it is 170 lines
 * of float reductions with running minima and maxima whose visiting order is
 * observable, so two copies would drift. */
void computeParamsReduce(const CropAnalysisJob &j,
                         procparams::DenoiseParams &dnparams)
{
    const float autoNR = 10;    // settings->nrauto;
    const float autoNRmax = 40; // settings->nrautomax;
    const float lowdenoise = 1.f;
    const int levaut = 0;

    /* The eleven numbers per crop that the whole analysis exists to
     * produce, in place of the commented-out printf this replaces.  They
     * are worth a verbose line because they are the only observable
     * output of a long reduction: a change anywhere in it -- a residency
     * slip, a ported kernel, a different summation order -- shows up
     * here as a number, where in the rendered image it arrives diffused
     * through calcautodn_info's threshold ladder and nine-way averaging,
     * attenuated to a fraction of an LSB. */
    if (settings->verbose > 1) {
        /* Nine significant digits, restored afterwards: these numbers
         * exist to be compared between two implementations of the same
         * reduction, and the default six hides everything the comparison
         * is about. */
        const std::streamsize prec = std::cout.precision(9);
        for (int k = 0; k < 9; ++k) {
            std::cout << "denoise auto-chroma crop " << k << ": Nb=" << j.Nb[k]
                      << " chM=" << j.store->ch_M[k]
                      << " max_r=" << j.store->max_r[k]
                      << " max_b=" << j.store->max_b[k]
                      << " min_r=" << j.min_r[k] << " min_b=" << j.min_b[k]
                      << " lum=" << j.lumL[k] << " chrom=" << j.chromC[k]
                      << " redyel=" << j.ry[k] << " skin=" << j.sk[k]
                      << " nsknc=" << j.pcsk[k] << std::endl;
        }
        std::cout.precision(prec);
    }

    float chM = 0.f;
    float MaxR = 0.f;
    float MaxB = 0.f;
    float MinR = 100000000000.f;
    float MinB = 100000000000.f;
    float maxr = 0.f;
    float maxb = 0.f;
    float Max_R[9] = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f};
    float Max_B[9] = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f};
    float Min_R[9];
    float Min_B[9];
    float MaxRMoy = 0.f;
    float MaxBMoy = 0.f;
    float MinRMoy = 0.f;
    float MinBMoy = 0.f;

    float multip = 1.f;

    if (!j.imgsrc->isRAW()) {
        multip = 2.f; // take into account gamma for TIF / JPG approximate
                      // value...not good for gamma=1
    }

    float adjustr = 1.f;

    // if (params->icm.workingProfile == "ProPhoto")   {
    //     adjustr = 1.f;   //
    // } else if (params->icm.workingProfile == "Adobe RGB")  {
    //     adjustr = 1.f / 1.3f;
    // } else if (params->icm.workingProfile == "sRGB")       {
    //     adjustr = 1.f / 1.3f;
    // } else if (params->icm.workingProfile == "WideGamut")  {
    //     adjustr = 1.f / 1.1f;
    // } else if (params->icm.workingProfile == "Beta RGB")   {
    //     adjustr = 1.f / 1.2f;
    // } else if (params->icm.workingProfile == "BestRGB")    {
    //     adjustr = 1.f / 1.2f;
    // } else if (params->icm.workingProfile == "BruceRGB")   {
    //     adjustr = 1.f / 1.2f;
    // }

    float delta[9];
    int mode = 1;
    int lissage = 0; // settings->leveldnliss;

    ART_PROFILE_SCOPE_NAMED(prof_autodn, "denoise:params:calcautodn");

    for (int k = 0; k < 9; k++) {
        float maxmax = max(j.store->max_r[k], j.store->max_b[k]);
        calcautodn_info(j.params, j.store->ch_M[k], delta[k], j.Nb[k], levaut,
                        maxmax, j.lumL[k], j.chromC[k], mode, lissage, j.ry[k],
                        j.sk[k], j.pcsk[k]);
        //  printf("ch_M=%f delta=%f\n",ch_M[k], delta[k]);
    }

    prof_autodn.stop();

    for (int k = 0; k < 9; k++) {
        if (j.store->max_r[k] > j.store->max_b[k]) {
            Max_R[k] = (delta[k]) /
                       ((autoNRmax * multip * adjustr * lowdenoise) / 2.f);
            Min_B[k] = -(j.store->ch_M[k] - j.min_b[k]) /
                       (autoNRmax * multip * adjustr * lowdenoise);
            Max_B[k] = 0.f;
            Min_R[k] = 0.f;
        } else {
            Max_B[k] = (delta[k]) /
                       ((autoNRmax * multip * adjustr * lowdenoise) / 2.f);
            Min_R[k] = -(j.store->ch_M[k] - j.min_r[k]) /
                       (autoNRmax * multip * adjustr * lowdenoise);
            Min_B[k] = 0.f;
            Max_R[k] = 0.f;
        }
    }

    for (int k = 0; k < 9; k++) {
        //  printf("ch_M= %f Max_R=%f Max_B=%f min_r=%f
        //  min_b=%f\n",ch_M[k],Max_R[k], Max_B[k],Min_R[k], Min_B[k]);
        chM += j.store->ch_M[k];
        MaxBMoy += Max_B[k];
        MaxRMoy += Max_R[k];
        MinRMoy += Min_R[k];
        MinBMoy += Min_B[k];

        if (Max_R[k] > MaxR) {
            MaxR = Max_R[k];
        }

        if (Max_B[k] > MaxB) {
            MaxB = Max_B[k];
        }

        if (Min_R[k] < MinR) {
            MinR = Min_R[k];
        }

        if (Min_B[k] < MinB) {
            MinB = Min_B[k];
        }
    }

    chM /= 9;
    MaxBMoy /= 9;
    MaxRMoy /= 9;
    MinBMoy /= 9;
    MinRMoy /= 9;

    if (MaxR > MaxB) {
        maxr = MaxRMoy + (MaxR - MaxRMoy) * 0.66f; // #std Dev
        // maxb=MinB;
        maxb = MinBMoy + (MinB - MinBMoy) * 0.66f;
    } else {
        maxb = MaxBMoy + (MaxB - MaxBMoy) * 0.66f;
        maxr = MinRMoy + (MinR - MinRMoy) * 0.66f;
    }

    //                  printf("DCROP skip=%d cha=%f red=%f bl=%f \n",skip,
    //                  chM,maxr,maxb);
    j.store->chrominance = chM / (autoNR * multip * adjustr);
    j.store->chrominanceRedGreen = maxr;
    j.store->chrominanceBlueYellow = maxb;

    dnparams.chrominance =
        j.store->chrominance * dnparams.chrominanceAutoFactor;
    dnparams.chrominanceRedGreen =
        j.store->chrominanceRedGreen * dnparams.chrominanceAutoFactor;
    dnparams.chrominanceBlueYellow =
        j.store->chrominanceBlueYellow * dnparams.chrominanceAutoFactor;

    j.store->valid = true;

    // printf("DENOISE STORE FINAL:\n  chM = %.6f", store.chM);
    // printf("  max_r = {");
    // for (int i = 0; i < 9; ++i) printf(" %.6f", j.store->max_r[i]);
    // printf(" }\n  max_b = {");
    // for (int i = 0; i < 9; ++i) printf(" %.6f", j.store->max_b[i]);
    // printf(" }\n  ch_M = {");
    // for (int i = 0; i < 9; ++i) printf(" %.6f", j.store->ch_M[i]);
    // printf("}\n");
    // printf("*****************\n\n");
    // fflush(stdout);
}

} // namespace denoise

void ImProcFunctions::denoiseComputeParams(ImageSource *imgsrc,
                                           const ColorTemp &currWB,
                                           DenoiseInfoStore &store,
                                           procparams::DenoiseParams &dnparams)
{
    if (store.valid ||
        dnparams.chrominanceMethod !=
            procparams::DenoiseParams::ChrominanceMethod::AUTOMATIC) {
        if (dnparams.chrominanceMethod ==
            procparams::DenoiseParams::ChrominanceMethod::AUTOMATIC) {
            dnparams.chrominance =
                store.chrominance * dnparams.chrominanceAutoFactor;
            dnparams.chrominanceRedGreen =
                store.chrominanceRedGreen * dnparams.chrominanceAutoFactor;
            dnparams.chrominanceBlueYellow =
                store.chrominanceBlueYellow * dnparams.chrominanceAutoFactor;
        }
        return;
    }

    if (settings->verbose) {
        std::cout << "Denoise: computing auto chrominance params..."
                  << std::endl;
    }

    MyTime t1aue, t2aue;
    t1aue.set();

    store.reset(); // = DenoiseInfoStore();

    denoise::CropAnalysisJob job;
    job.ipf = this;
    job.imgsrc = imgsrc;
    job.currWB = &currWB;
    job.params = params;
    job.dnparams = &dnparams;
    job.store = &store;
    job.multiThread = multiThread;

    int widIm, heiIm;
    job.tr = getCoarseBitMask(params->coarse);
    imgsrc->getFullSize(widIm, heiIm, job.tr);
    job.crW = widIm / 2;
    job.crH = heiIm / 2;

    const int begW = 50;
    const int begH = 50;
    job.coordW[0] = begW;
    job.coordW[1] = widIm / 2 - job.crW / 2;
    job.coordW[2] = widIm - job.crW - begW;
    job.coordH[0] = begH;
    job.coordH[1] = heiIm / 2 - job.crH / 2;
    job.coordH[2] = heiIm - job.crH - begH;

    RGB_denoise_infoGamCurve(dnparams, imgsrc->isRAW(), job.gamcurve, job.gam,
                             job.gamthresh, job.gamslope);

    /* The analysis's own level count, at the scale of 1 it always runs at. */
    job.levwav_info = max(2, int(5 - std::ceil(std::log(1.0))));

    /* Not imgsrc->getDirPyrDenoiseExpComp(): the analysis has always been run
     * at a fixed +log2(5) stops. */
    job.info_expcomp = std::log(5.f) / std::log(2.f);

    {
        MyTime t1p, t2p;
        t1p.set();
        bool onGPU = denoise::computeParamsGPU(job);
        if (!onGPU) {
            denoise::computeParamsCPU(job);
        }
        if (settings->verbose) {
            t2p.set();
            std::cout << "denoiseComputeParams: executed on the "
                      << (onGPU ? "GPU" : "CPU") << " in "
                      << t2p.etime(t1p) << " usec" << std::endl;
        }
    }

    denoise::computeParamsReduce(job, dnparams);

    if (settings->verbose) {
        t2aue.set();
        printf("Info denoise auto performed in %d usec:\n",
               t2aue.etime(t1aue));
    }
    // end evaluate noise
}

void ImProcFunctions::denoise(ImageSource *imgsrc, const ColorTemp &currWB,
                              Imagefloat *img,
                              const procparams::DenoiseParams &dnparams)
{
    if (!dnparams.enabled) {
        return;
    }

    if (plistener) {
        plistener->setProgressStr("PROGRESSBAR_DENOISING");
        plistener->setProgress(0);
    }

    procparams::DenoiseParams denoiseParams = dnparams;
    adjust_params(denoiseParams, scale);

    if (plistener) {
        plistener->setProgress(0.1);
    }

    ImProcData im(params, scale, multiThread, this);
    double ecomp = params->exposure.enabled ? params->exposure.expcomp : 0.0;
    ExposureParams expparams;
    expparams.enabled = true;
    expparams.expcomp = ecomp;

    if (ecomp > 0) {
        expcomp(img, &expparams);
    }

    {
        ART_PROFILE_SCOPE("denoise:rgb_denoise");
        denoise::RGB_denoise(im, img, denoiseParams);
    }

    if (plistener) {
        plistener->setProgress(0.8);
    }

    if (denoiseParams.smoothingEnabled) {
        denoise::finalSmoothing(im, img, denoiseParams);
    }

    if (ecomp > 0) {
        expparams.expcomp = -ecomp;
        expcomp(img, &expparams);
    }

    if (plistener) {
        plistener->setProgress(1);
    }
}

#define TS 64     // Tile size
#define offset 25 // shift between tiles
// #define fTS ((TS/2+1))  // second dimension of Fourier tiles
#define blkrad 1 // radius of block averaging

// #define epsilon 0.001f/(TS*TS) //tolerance


using namespace denoise;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

extern const Settings *settings;
extern MyMutex *fftwMutex;

namespace {

template <bool useUpperBound>
void do_median_denoise(float **src, float **dst, float upperBound, int width,
                       int height, denoise::Median medianType, int iterations,
                       int numThreads, float **buffer)
{
    iterations = max(1, iterations);

    typedef denoise::Median Median;

    int border = 1;

    switch (medianType) {
    case Median::TYPE_3X3_SOFT:
    case Median::TYPE_3X3_STRONG: {
        border = 1;
        break;
    }

    case Median::TYPE_5X5_SOFT: {
        border = 2;
        break;
    }

    case Median::TYPE_5X5_STRONG: {
        border = 2;
        break;
    }

    case Median::TYPE_7X7: {
        border = 3;
        break;
    }

    case Median::TYPE_9X9: {
        border = 4;
        break;
    }
    }

    float **allocBuffer = nullptr;
    float **medBuffer[2];
    medBuffer[0] = src;

    // we need a buffer if src == dst or if (src != dst && iterations > 1)
    if (src == dst || iterations > 1) {
        if (buffer == nullptr) { // we didn't get a buffer => create one
            allocBuffer = new float *[height];

            for (int i = 0; i < height; ++i) {
                allocBuffer[i] = new float[width];
            }

            medBuffer[1] = allocBuffer;
        } else { // we got a buffer => use it
            medBuffer[1] = buffer;
        }
    } else { // we can write directly into destination
        medBuffer[1] = dst;
    }

    float **medianIn, **medianOut = nullptr;
    int BufferIndex = 0;

    for (int iteration = 1; iteration <= iterations; ++iteration) {
        medianIn = medBuffer[BufferIndex];
        medianOut = medBuffer[BufferIndex ^ 1];

        if (iteration == 1) { // upper border
            for (int i = 0; i < border; ++i) {
                for (int j = 0; j < width; ++j) {
                    medianOut[i][j] = medianIn[i][j];
                }
            }
        }

#ifdef _OPENMP
#pragma omp parallel for num_threads(numThreads) if (numThreads > 1)           \
    schedule(dynamic, 16)
#endif

        for (int i = border; i < height - border; ++i) {
            int j = 0;

            for (; j < border; ++j) {
                medianOut[i][j] = medianIn[i][j];
            }

            switch (medianType) {
            case Median::TYPE_3X3_SOFT: {
                for (; j < width - border; ++j) {
                    if (!useUpperBound || medianIn[i][j] <= upperBound) {
                        medianOut[i][j] =
                            median(medianIn[i - 1][j], medianIn[i][j - 1],
                                   medianIn[i][j], medianIn[i][j + 1],
                                   medianIn[i + 1][j]);
                    } else {
                        medianOut[i][j] = medianIn[i][j];
                    }
                }

                break;
            }

            case Median::TYPE_3X3_STRONG: {
                for (; j < width - border; ++j) {
                    if (!useUpperBound || medianIn[i][j] <= upperBound) {
                        medianOut[i][j] =
                            median(medianIn[i - 1][j - 1], medianIn[i - 1][j],
                                   medianIn[i - 1][j + 1], medianIn[i][j - 1],
                                   medianIn[i][j], medianIn[i][j + 1],
                                   medianIn[i + 1][j - 1], medianIn[i + 1][j],
                                   medianIn[i + 1][j + 1]);
                    } else {
                        medianOut[i][j] = medianIn[i][j];
                    }
                }

                break;
            }

            case Median::TYPE_5X5_SOFT: {
                for (; j < width - border; ++j) {
                    if (!useUpperBound || medianIn[i][j] <= upperBound) {
                        medianOut[i][j] =
                            median(medianIn[i - 2][j], medianIn[i - 1][j - 1],
                                   medianIn[i - 1][j], medianIn[i - 1][j + 1],
                                   medianIn[i][j - 2], medianIn[i][j - 1],
                                   medianIn[i][j], medianIn[i][j + 1],
                                   medianIn[i][j + 2], medianIn[i + 1][j - 1],
                                   medianIn[i + 1][j], medianIn[i + 1][j + 1],
                                   medianIn[i + 2][j]);
                    } else {
                        medianOut[i][j] = medianIn[i][j];
                    }
                }

                break;
            }

            case Median::TYPE_5X5_STRONG: {
#ifdef ART_SIMD

                for (; !useUpperBound && j < width - border - 3; j += 4) {
                    STVFU(medianOut[i][j],
                          median(LVFU(medianIn[i - 2][j - 2]),
                                 LVFU(medianIn[i - 2][j - 1]),
                                 LVFU(medianIn[i - 2][j]),
                                 LVFU(medianIn[i - 2][j + 1]),
                                 LVFU(medianIn[i - 2][j + 2]),
                                 LVFU(medianIn[i - 1][j - 2]),
                                 LVFU(medianIn[i - 1][j - 1]),
                                 LVFU(medianIn[i - 1][j]),
                                 LVFU(medianIn[i - 1][j + 1]),
                                 LVFU(medianIn[i - 1][j + 2]),
                                 LVFU(medianIn[i][j - 2]),
                                 LVFU(medianIn[i][j - 1]), LVFU(medianIn[i][j]),
                                 LVFU(medianIn[i][j + 1]),
                                 LVFU(medianIn[i][j + 2]),
                                 LVFU(medianIn[i + 1][j - 2]),
                                 LVFU(medianIn[i + 1][j - 1]),
                                 LVFU(medianIn[i + 1][j]),
                                 LVFU(medianIn[i + 1][j + 1]),
                                 LVFU(medianIn[i + 1][j + 2]),
                                 LVFU(medianIn[i + 2][j - 2]),
                                 LVFU(medianIn[i + 2][j - 1]),
                                 LVFU(medianIn[i + 2][j]),
                                 LVFU(medianIn[i + 2][j + 1]),
                                 LVFU(medianIn[i + 2][j + 2])));
                }

#endif

                for (; j < width - border; ++j) {
                    if (!useUpperBound || medianIn[i][j] <= upperBound) {
                        medianOut[i][j] = median(
                            medianIn[i - 2][j - 2], medianIn[i - 2][j - 1],
                            medianIn[i - 2][j], medianIn[i - 2][j + 1],
                            medianIn[i - 2][j + 2], medianIn[i - 1][j - 2],
                            medianIn[i - 1][j - 1], medianIn[i - 1][j],
                            medianIn[i - 1][j + 1], medianIn[i - 1][j + 2],
                            medianIn[i][j - 2], medianIn[i][j - 1],
                            medianIn[i][j], medianIn[i][j + 1],
                            medianIn[i][j + 2], medianIn[i + 1][j - 2],
                            medianIn[i + 1][j - 1], medianIn[i + 1][j],
                            medianIn[i + 1][j + 1], medianIn[i + 1][j + 2],
                            medianIn[i + 2][j - 2], medianIn[i + 2][j - 1],
                            medianIn[i + 2][j], medianIn[i + 2][j + 1],
                            medianIn[i + 2][j + 2]);
                    } else {
                        medianOut[i][j] = medianIn[i][j];
                    }
                }

                break;
            }

            case Median::TYPE_7X7: {
#ifdef ART_SIMD
                std::array<vfloat, 49> vpp ALIGNED16;

                for (; !useUpperBound && j < width - border - 3; j += 4) {
                    for (int kk = 0, ii = -border; ii <= border; ++ii) {
                        for (int jj = -border; jj <= border; ++jj, ++kk) {
                            vpp[kk] = LVFU(medianIn[i + ii][j + jj]);
                        }
                    }

                    STVFU(medianOut[i][j], median(vpp));
                }

#endif

                std::array<float, 49> pp;

                for (; j < width - border; ++j) {
                    if (!useUpperBound || medianIn[i][j] <= upperBound) {
                        for (int kk = 0, ii = -border; ii <= border; ++ii) {
                            for (int jj = -border; jj <= border; ++jj, ++kk) {
                                pp[kk] = medianIn[i + ii][j + jj];
                            }
                        }

                        medianOut[i][j] = median(pp);
                    } else {
                        medianOut[i][j] = medianIn[i][j];
                    }
                }

                break;
            }

            case Median::TYPE_9X9: {
#ifdef ART_SIMD
                std::array<vfloat, 81> vpp ALIGNED16;

                for (; !useUpperBound && j < width - border - 3; j += 4) {
                    for (int kk = 0, ii = -border; ii <= border; ++ii) {
                        for (int jj = -border; jj <= border; ++jj, ++kk) {
                            vpp[kk] = LVFU(medianIn[i + ii][j + jj]);
                        }
                    }

                    STVFU(medianOut[i][j], median(vpp));
                }

#endif

                std::array<float, 81> pp;

                for (; j < width - border; ++j) {
                    if (!useUpperBound || medianIn[i][j] <= upperBound) {
                        for (int kk = 0, ii = -border; ii <= border; ++ii) {
                            for (int jj = -border; jj <= border; ++jj, ++kk) {
                                pp[kk] = medianIn[i + ii][j + jj];
                            }
                        }

                        medianOut[i][j] = median(pp);
                    } else {
                        medianOut[i][j] = medianIn[i][j];
                    }
                }

                for (; j < width; ++j) {
                    medianOut[i][j] = medianIn[i][j];
                }

                break;
            }
            }

            for (; j < width; ++j) {
                medianOut[i][j] = medianIn[i][j];
            }
        }

        if (iteration == 1) { // lower border
            for (int i = height - border; i < height; ++i) {
                for (int j = 0; j < width; ++j) {
                    medianOut[i][j] = medianIn[i][j];
                }
            }
        }

        BufferIndex ^= 1; // swap buffers
    }

    if (medianOut != dst) {
#ifdef _OPENMP
#pragma omp parallel for num_threads(numThreads) if (numThreads > 1)
#endif

        for (int i = 0; i < height; ++i) {
            for (int j = 0; j < width; ++j) {
                dst[i][j] = medianOut[i][j];
            }
        }
    }

    if (allocBuffer != nullptr) { // we allocated memory, so let's free it now
        for (int i = 0; i < height; ++i) {
            delete[] allocBuffer[i];
        }

        delete[] allocBuffer;
    }
}

} // namespace

namespace denoise {

void Median_Denoise(float **src, float **dst, const int width, const int height,
                    const Median medianType, const int iterations,
                    const int numThreads, float **buffer)
{
    do_median_denoise<false>(src, dst, 0.f, width, height, medianType,
                             iterations, numThreads, buffer);
}

void Median_Denoise(float **src, float **dst, float upperBound, const int width,
                    const int height, const Median medianType,
                    const int iterations, const int numThreads, float **buffer)
{
    do_median_denoise<true>(src, dst, upperBound, width, height, medianType,
                            iterations, numThreads, buffer);
}

} // namespace denoise

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

namespace {

void RGBtile_denoise(double scale, float *fLblox, int hblproc,
                     float *noisevar_Ldetail, float *nbrwt,
                     float *blurbuffer) // for DCT
{
    // const int TS = max(int(default_TS / scale), 4);
    // const int offset = max(int(default_offset / scale), 1);

    int blkstart = hblproc * TS * TS;

    const int blur_rad = max(1, int(3 / scale));
    boxabsblur(fLblox + blkstart, nbrwt, blur_rad, blur_rad, TS, TS,
               blurbuffer); // blur neighbor weights for more robust estimation
                            // //for DCT

#ifdef ART_SIMD
    __m128 tempv;
    //__m128  noisevar_Ldetailv = _mm_set1_ps(noisevar_Ldetail);
    __m128 onev = _mm_set1_ps(1.0f);

    for (int n = 0; n < TS * TS; n += 4) { // for DCT
        tempv = onev - xexpf(-SQRV(LVF(nbrwt[n])) /
                             LVF(noisevar_Ldetail[blkstart + n]));
        _mm_storeu_ps(&fLblox[blkstart + n],
                      LVFU(fLblox[blkstart + n]) * tempv);
    } // output neighbor averaged result

#else

    for (int n = 0; n < TS * TS; ++n) { // for DCT
        fLblox[blkstart + n] *=
            (1 - xexpf(-SQR(nbrwt[n]) / noisevar_Ldetail[blkstart + n]));
    } // output neighbor averaged result

#endif

    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    // printf("vblk=%d  hlk=%d  wsqave=%f   ||   ",vblproc,hblproc,wsqave);

} // end of function tile_denoise

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void RGBoutput_tile_row(double scale, float *bloxrow_L, float **Ldetail,
                        float **tilemask_out, int height, int width, int top)
{
    // const int TS = max(int(default_TS / scale), 4);
    // const int offset = max(int(default_offset / scale), 1);

    const int numblox_W = ceil((static_cast<float>(width)) / (offset));
    const float DCTnorm = 1.0f / (4 * TS * TS); // for DCT

    int imin = MAX(0, -top);
    int bottom = MIN(top + TS, height);
    int imax = bottom - top;

    // add row of tiles to output image
    for (int i = imin; i < imax; ++i) {
        for (int hblk = 0; hblk < numblox_W; ++hblk) {
            int left = (hblk - blkrad) * offset;
            int right = MIN(left + TS, width);
            int jmin = MAX(0, -left);
            int jmax = right - left;
            int indx = hblk * TS;

            for (int j = jmin; j < jmax;
                 ++j) { // this loop gets auto vectorized by gcc
                Ldetail[top + i][left + j] += tilemask_out[i][j] *
                                              bloxrow_L[(indx + i) * TS + j] *
                                              DCTnorm; // for DCT
            }
        }
    }
}
/*
#undef TS
#undef fTS
#undef offset
#undef epsilon
*/

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


} // namespace

namespace denoise {



/* Phase 7's CPU-only working set: the two tile masks, the FFTW plans and the
 * per-thread blox arrays.
 *
 * Built on first use rather than up front, because FFTW_MEASURE genuinely runs
 * and times candidate transforms -- a call whose phase 7 goes to the device,
 * or that has no phase 7 at all, should not pay for plans it never executes.
 * The CPU driver calls ensure() eagerly; the GPU driver only when the device
 * entry declines.
 *
 * It also takes both ends of the blox arrays' lifetime, which used to be
 * split: detail_recovery allocated them and RGB_denoise freed them. */
class DctWorkspace {
public:
    DctWorkspace(int imwidth, std::size_t bloxArraySize):
        maxNumbloxW_(ceil((static_cast<float>(imwidth)) / (offset)) +
                     2 * blkrad),
        maskIn_(TS, TS, ARRAY2D_ALIGNED), maskOut_(TS, TS, ARRAY2D_ALIGNED),
        forward_(nullptr), backward_(nullptr), built_(false),
        lblox_(bloxArraySize, nullptr), flblox_(bloxArraySize, nullptr)
    {
    }

    ~DctWorkspace()
    {
        for (std::size_t i = 0; i < lblox_.size(); ++i) {
            if (lblox_[i]) {
                fftwf_free(lblox_[i]);
            }
            if (flblox_[i]) {
                fftwf_free(flblox_[i]);
            }
        }
        /* Guarded, and the members are nullptr-initialised: before this moved
         * here the two plans were plain uninitialised locals destroyed under
         * `if (denoiseLuminance)`, so a lazily-built workspace that was never
         * ensure()d would have destroyed garbage. */
        if (forward_) {
            fftwf_destroy_plan(forward_);
        }
        if (backward_) {
            fftwf_destroy_plan(backward_);
        }
    }

    void ensure()
    {
        if (built_) {
            return;
        }
        built_ = true;
        buildTileMasks();
        buildPlans();
    }

    array2D<float> &maskIn() { return maskIn_; }
    array2D<float> &maskOut() { return maskOut_; }
    fftwf_plan forward() const { return forward_; }
    fftwf_plan backward() const { return backward_; }
    int maxNumbloxW() const { return maxNumbloxW_; }
    float **lblox() { return lblox_.empty() ? nullptr : &lblox_[0]; }
    float **flblox() { return flblox_.empty() ? nullptr : &flblox_[0]; }
    std::size_t bloxArraySize() const { return lblox_.size(); }

private:
    DctWorkspace(const DctWorkspace &);
    DctWorkspace &operator=(const DctWorkspace &);

    void buildTileMasks()
    {
        const int border = MAX(2, TS / 16);
        for (int i = 0; i < TS; ++i) {
            float i1 = abs((i > TS / 2 ? i - TS + 1 : i));
            float vmask =
                (i1 < border ? SQR(sin((rtengine::RT_PI * i1) / (2 * border)))
                             : 1.0f);
            float vmask2 =
                (i1 < 2 * border
                     ? SQR(sin((rtengine::RT_PI * i1) / (2 * border)))
                     : 1.0f);

            for (int j = 0; j < TS; ++j) {
                float j1 = abs((j > TS / 2 ? j - TS + 1 : j));
                maskIn_[i][j] =
                    (vmask * (j1 < border
                                  ? SQR(sin((rtengine::RT_PI * j1) /
                                            (2 * border)))
                                  : 1.0f)) +
                    kEpsilon;
                maskOut_[i][j] =
                    (vmask2 *
                     (j1 < 2 * border
                          ? SQR(sin((rtengine::RT_PI * j1) / (2 * border)))
                          : 1.0f)) +
                    kEpsilon;
            }
        }
    }

    void buildPlans()
    {
        /* FFTW_MEASURE actually runs and times candidate algorithms, so plan
         * creation is real work, not bookkeeping -- worth its own scope. */
        ART_PROFILE_SCOPE("denoise:fftw-plan");
        float *Lbloxtmp = reinterpret_cast<float *>(
            fftwf_malloc(maxNumbloxW_ * TS * TS * sizeof(float)));
        float *fLbloxtmp = reinterpret_cast<float *>(
            fftwf_malloc(maxNumbloxW_ * TS * TS * sizeof(float)));

        int nfwd[2] = {TS, TS};

        // for DCT:
        fftw_r2r_kind fwdkind[2] = {FFTW_REDFT10, FFTW_REDFT10};
        fftw_r2r_kind bwdkind[2] = {FFTW_REDFT01, FFTW_REDFT01};

        /* There used to be a second, min_numblox_W-sized pair of plans for the
         * narrower right-edge tile.  With one tile it equals maxNumbloxW_, and
         * detail_recovery's plan_idx (numblox_W != max_numblox_W) was
         * therefore always 0, so the second pair was built with FFTW_MEASURE
         * and never executed. */
        forward_ = fftwf_plan_many_r2r(2, nfwd, maxNumbloxW_, Lbloxtmp, nullptr,
                                       1, TS * TS, fLbloxtmp, nullptr, 1,
                                       TS * TS, fwdkind,
                                       FFTW_MEASURE | FFTW_DESTROY_INPUT);
        backward_ = fftwf_plan_many_r2r(2, nfwd, maxNumbloxW_, fLbloxtmp,
                                        nullptr, 1, TS * TS, Lbloxtmp, nullptr,
                                        1, TS * TS, bwdkind,
                                        FFTW_MEASURE | FFTW_DESTROY_INPUT);
        fftwf_free(Lbloxtmp);
        fftwf_free(fLbloxtmp);
    }

    static const float kEpsilon;

    int maxNumbloxW_;
    array2D<float> maskIn_, maskOut_;
    fftwf_plan forward_, backward_;
    bool built_;
    std::vector<float *> lblox_, flblox_;
};

const float DctWorkspace::kEpsilon = 0.001f / (TS * TS);

/* The detail mask phase 7 modulates its correction with.  Shared: the GPU
 * host-plane entry is handed the CPU's own copy rather than deriving its own,
 * because recomputing it on the device would put a second approximation
 * between the two sides for no gain -- the CPU has to build it anyway
 * whenever it takes the phase.
 *
 * Leaves `mask` empty when detail_thresh <= 0, which is exactly the condition
 * both paths test to decide whether to use it. */
void buildDetailMask(int width, int height, LabImage *labdn, int detail_thresh,
                     double scale, array2D<float> &mask)
{
    if (detail_thresh > 0) {
        array2D<float> LL(width, height, labdn->L, ARRAY2D_BYREFERENCE);
        float amount = LIM01(float(detail_thresh) / 100.f);
        detail_mask(LL, mask, 65535.f, 25.f, 10000.f, amount, BlurType::GAUSS,
                    25.f / scale, false);
    }
}

/* Phase 7 on the device, with the planes on the host: uploads what it needs
 * and downloads the result.  Worth attempting before the CPU path allocates
 * anything -- Ldetail, totwt, the FFTW blox arrays and detail_factor come to
 * ~670 MiB at 24 Mpix, and there is no reason to pay for them when the GPU
 * takes the phase.  False means it declined (no device, over its memory
 * budget, a failed dispatch) and nothing was changed. */
bool detailRecoveryGPUHost(int width, int height, LabImage *labdn,
                           array2D<float> *Lin, float params_Ldetail,
                           int detail_thresh, array2D<float> &mask,
                           double scale, gpu::BufferPool *poolp,
                           gpu::Context *ctx)
{
    gpu::ops::DetailRecoveryGPU drp;
    drp.params_Ldetail = params_Ldetail;
    drp.detail_thresh = detail_thresh;
    drp.mask = detail_thresh > 0 ? static_cast<float **>(mask) : nullptr;
    drp.scale = scale;
    return gpu::ops::denoiseDetailRecovery(width, height, labdn->L,
                                           static_cast<float **>(*Lin), drp,
                                           poolp, ctx);
}

/* Phase 7 on the host: the sliding-window block DCT that puts back detail the
 * wavelet shrink removed.  The CPU implementation and nothing else -- both
 * drivers call it, the GPU one when its own entry has declined.  `mask` is
 * buildDetailMask's output, read only when detail_thresh > 0. */
void detailRecoveryCPU(int width, int height, LabImage *labdn,
                       array2D<float> *Lin, int numthreads,
                       int denoiseNestedLevels, DctWorkspace &dct,
                       float params_Ldetail, int detail_thresh,
                       const array2D<float> &mask, double scale,
                       bool denoise_aggressive)
{
    dct.ensure();
    /* Aliases, so the block loop below reads exactly as it did when these were
     * eight separate parameters. */
    float **LbloxArray = dct.lblox();
    float **fLbloxArray = dct.flblox();
    array2D<float> &tilemask_in = dct.maskIn();
    array2D<float> &tilemask_out = dct.maskOut();
    const fftwf_plan plan_forward_blox = dct.forward();
    const fftwf_plan plan_backward_blox = dct.backward();
    const int max_numblox_W = dct.maxNumbloxW();

    const auto compute_detail = [](float d) -> float {
        return SQR(static_cast<float>(SQR(100. - d) + 50. * (100. - d)) * TS *
                   0.5f);
    };
    const float detail_hi = compute_detail(params_Ldetail);
    const float detail_lo = compute_detail(0.f);

    // calculation for detail recovery blocks
    const int numblox_W =
        ceil((static_cast<float>(width)) / (offset)) + 2 * blkrad;
    const int numblox_H =
        ceil((static_cast<float>(height)) / (offset)) + 2 * blkrad;

    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    // Main detail recovery algorithm: Block loop
    // DCT block data storage

    // residual between input and denoised L channel
    array2D<float> Ldetail(width, height, ARRAY2D_CLEAR_DATA | ARRAY2D_ALIGNED);
    array2D<float> totwt(
        width, height,
        ARRAY2D_CLEAR_DATA |
            ARRAY2D_ALIGNED); // weight for combining DCT blocks

    {
        for (int i = 0; i < denoiseNestedLevels * numthreads; ++i) {
            LbloxArray[i] = reinterpret_cast<float *>(
                fftwf_malloc(max_numblox_W * TS * TS * sizeof(float)));
            fLbloxArray[i] = reinterpret_cast<float *>(
                fftwf_malloc(max_numblox_W * TS * TS * sizeof(float)));
        }
    }

#ifdef _OPENMP
    int masterThread = omp_get_thread_num();
#endif
#ifdef _OPENMP
#pragma omp parallel num_threads(                                              \
        denoiseNestedLevels) if (denoiseNestedLevels > 1)
#endif
    {
#ifdef _OPENMP
        int subThread =
            masterThread * denoiseNestedLevels + omp_get_thread_num();
#else
        int subThread = 0;
#endif
        float blurbuffer[TS * TS] ALIGNED64;
        float *Lblox = LbloxArray[subThread];
        float *fLblox = fLbloxArray[subThread];
        float pBuf[width + TS + 2 * blkrad * offset] ALIGNED16;
        float nbrwt[TS * TS] ALIGNED64;
        AlignedBuffer<float> detail_factor_buf(numblox_W * TS * TS);
        float *detail_factor = detail_factor_buf.data;

        /* Block rows overlap: consecutive vblk tops are `offset` (25) apart
         * while each block spans TS (64) rows, so a plain `omp for` over vblk
         * had several threads doing `+=` into the same Ldetail/totwt elements
         * at once.  That is a data race and therefore UB, even though in
         * practice the lost updates were small enough not to be the source of
         * denoise's ~1 LSB run-to-run scatter -- disabling this phase
         * entirely does not make the output deterministic, so that scatter
         * comes from somewhere else in the wavelet path and is still open.
         *
         * Fixed by colouring the loop instead of guarding it: blocks whose
         * indices differ by kPhases = ceil(TS / offset) = 3 are at least
         * 3*25 = 75 >= 64 rows apart and therefore cannot overlap, so the
         * three residue classes can each be distributed across threads and
         * separated by the implicit barrier at the end of `omp for`.  No
         * atomics (which would remove the race but not the nondeterminism,
         * since float addition order would still vary), no per-thread
         * accumulators (10 threads x 2 x W x H floats is ~2 GB at 24 Mpix),
         * and no loss of parallelism worth measuring: numblox_H/3 is still
         * dozens of blocks per round. */
        constexpr int kPhases = (TS + offset - 1) / offset;
        static_assert(kPhases * offset >= TS,
                      "block rows in the same phase class must not overlap");

        for (int phase = 0; phase < kPhases; ++phase) {
#ifdef _OPENMP
#pragma omp for
#endif

            for (int vblk = phase; vblk < numblox_H; vblk += kPhases) {

                int top = (vblk - blkrad) * offset;
                float *datarow = pBuf + blkrad * offset;

                for (int i = 0; i < TS; ++i) {
                    int row = top + i;
                    int rr = row;

                    if (row < 0) {
                        rr = MIN(-row, height - 1);
                    } else if (row >= height) {
                        rr = MAX(0, 2 * height - 2 - row);
                    }

                    for (int j = 0; j < labdn->W; ++j) {
                        datarow[j] = ((*Lin)[rr][j] - labdn->L[rr][j]);
                    }

                    for (int j = -blkrad * offset; j < 0; ++j) {
                        datarow[j] = datarow[MIN(-j, width - 1)];
                    }

                    for (int j = width; j < width + TS + blkrad * offset; ++j) {
                        datarow[j] = datarow[MAX(0, 2 * width - 2 - j)];
                    } // now we have a padded data row

                    // now fill this row of the blocks with Lab high pass data
                    for (int hblk = 0; hblk < numblox_W; ++hblk) {
                        int left = (hblk - blkrad) * offset;
                        int indx = (hblk)*TS; // index of block in malloc

                        if (top + i >= 0 && top + i < height) {
                            int j;

                            for (j = 0; j < min((-left), TS); ++j) {
                                Lblox[(indx + i) * TS + j] =
                                    tilemask_in[i][j] *
                                    datarow[left + j]; // luma data
                                detail_factor[(indx + i) * TS + j] = detail_lo;
                            }

                            for (; j < min(TS, width - left); ++j) {
                                Lblox[(indx + i) * TS + j] =
                                    tilemask_in[i][j] *
                                    datarow[left + j]; // luma data
                                totwt[top + i][left + j] +=
                                    tilemask_in[i][j] * tilemask_out[i][j];
                                detail_factor[(indx + i) * TS + j] =
                                    detail_thresh > 0
                                        ? compute_detail(params_Ldetail *
                                                         mask[top + i][left + j])
                                        : detail_hi;
                            }

                            for (; j < TS; ++j) {
                                Lblox[(indx + i) * TS + j] =
                                    tilemask_in[i][j] *
                                    datarow[left + j]; // luma data
                                detail_factor[(indx + i) * TS + j] = detail_lo;
                            }
                        } else {
                            for (int j = 0; j < TS; ++j) {
                                Lblox[(indx + i) * TS + j] =
                                    tilemask_in[i][j] *
                                    datarow[left + j]; // luma data
                                detail_factor[(indx + i) * TS + j] = detail_lo;
                            }
                        }
                    }

                } // end of filling block row

                //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
                // fftwf_print_plan (plan_forward_blox);
                fftwf_execute_r2r(plan_forward_blox, Lblox,
                                  fLblox); // DCT an entire row of tiles
                //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
                // now process the vblk row of blocks for noise reduction
                for (int hblk = 0; hblk < numblox_W; ++hblk) {
                    RGBtile_denoise(scale, fLblox, hblk, detail_factor, nbrwt,
                                    blurbuffer);
                } // end of horizontal block loop

                //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

                // now perform inverse FT of an entire row of blocks
                fftwf_execute_r2r(plan_backward_blox, fLblox,
                                  fLblox); // for DCT
                int topproc = (vblk - blkrad) * offset;
                // add row of blocks to output image tile
                RGBoutput_tile_row(scale, fLblox, Ldetail, tilemask_out, height,
                                   width, topproc);
                //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
            } // end of vertical block loop
        } // end of phase class

        //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    }
    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

#ifdef _OPENMP
#pragma omp parallel for num_threads(                                          \
        denoiseNestedLevels) if (denoiseNestedLevels > 1)
#endif

    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            labdn->L[i][j] +=
                Ldetail[i][j] / totwt[i][j]; // note that labdn initially stores
                                             // the denoised hipass data
        }
    }
}



/* ---------------------------------------------------------------------------
 * RGB_denoise's phases
 *
 * RGB_denoise is one long function whose body decomposes into eight phases
 * (the same eight the ART_PROFILE scopes are named for, and the same eight
 * the Vulkan port is organised around):
 *
 *   1 fill        RGB -> L/a/b through the gamma LUTs  [denoiseFill]
 *   2 decompose   wavelet decomposition of L, a, b
 *   3 mad         madL[level][dir] noise estimate from L's detail bands
 *   4 shrink-ab   chroma shrinkage (cross-channel: reads L's coefficients)
 *   5 shrink-l    luma shrinkage
 *   6 reconstruct wavelet reconstruction of L, a, b
 *   7 dct         sliding-window block DCT detail recovery
 *                 [buildDetailMask + detailRecoveryCPU]
 *   8 out         L/a/b -> RGB through the inverse gamma LUT  [denoiseOutput]
 *
 * DenoiseContext carries what the phases share, so each phase reads as one
 * operation on a named input rather than as a slice of a long function.
 * ------------------------------------------------------------------------ */

/* Phase 1a/1c fused, on the host: classify each half-resolution sample's
 * chroma against the noise curve and expand the result -- plus a flat luma
 * term -- into the quarter-resolution maps phases 4 and 5 read. */
void computeNoisevarMapsCPU(const DenoiseContext &c, Imagefloat *src,
                            const denoise::NoiseCurve &noiseCCurve)
{
    const float maxNoiseVarab = max(c.noisevarab_b, c.noisevarab_r);
    const int H2 = (c.H + 1) / 2;
    const float cn100Precalc = SQR(1.f + 4.f * noiseCCurve[100.f / 60.f]);
    const float (*wpi)[3] = c.wpi;

#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic, 16) num_threads(                    \
        c.denoiseNestedLevels) if (c.denoiseNestedLevels > 1)
#endif
    for (int ii = 0; ii < H2; ++ii) {
        const int si = ii * 2;
        for (int jj = 0; jj < c.W2; ++jj) {
            const int sj = jj * 2;

            float LLum, AAum, BBum;
            const float RL = src->r(si, sj);
            const float GL = src->g(si, sj);
            const float BL = src->b(si, sj);
            // determine luminance and chrominance for noisecurves
            float XL, YL, ZL;
            Color::rgbxyz(RL, GL, BL, XL, YL, ZL, wpi);
            Color::XYZ2Lab(XL, YL, ZL, LLum, AAum, BBum);

            const float cN = sqrtf(SQR(AAum) + SQR(BBum));
            const float ccalcVal = cN > 100
                                       ? SQR(1.f + 4.f * noiseCCurve[cN / 60.f])
                                       : cn100Precalc;

            const int k = ii * c.W2 + jj;
            c.noisevarlum[k] = c.noisevarL;
            c.noisevarchrom[k] = maxNoiseVarab * ccalcVal;
        }
    }
}

/* Phase 1b.  Pointwise: optional inverse "denoise gamma" (Lab mode only), the
 * forward gamma LUT, then RGB -> YUV (or Lab) with the working-space matrix. */
void denoiseFill(const DenoiseContext &c, Imagefloat *src, LabImage *labdn)
{
#ifdef _OPENMP
#pragma omp parallel for num_threads(                                          \
        c.denoiseNestedLevels) if (c.denoiseNestedLevels > 1)
#endif
    for (int i = 0; i < c.H; ++i) {
        for (int j = 0; j < c.W; ++j) {
            float X = src->r(i, j);
            float Y = src->g(i, j);
            float Z = src->b(i, j);

            if (c.lab_mode) {
                X = Color::denoiseIGammaTab[X];
                Y = Color::denoiseIGammaTab[Y];
                Z = Color::denoiseIGammaTab[Z];
            }

            // conversion colorspace to determine luminance with no gamma
            X = c.applyGamma(X);
            Y = c.applyGamma(Y);
            Z = c.applyGamma(Z);

            float l, u, v;
            if (c.lab_mode) {
                Color::rgb2lab(X, Y, Z, l, v, u, c.wpi);
            } else {
                Color::rgb2yuv(X, Y, Z, l, u, v, c.wpi);
            }
            labdn->L[i][j] = l;
            labdn->a[i][j] = v;
            labdn->b[i][j] = u;
        }
    }
}

/* Phase 8.  The inverse of phase 1, plus the chroma boost that phase 1 has no
 * counterpart for: coefficients whose chroma magnitude exceeds 3000 get pushed
 * outwards by qhighFactor * realred/realblue, which is how the aggressive
 * (QUALITY_HIGH) mode restores saturation the shrinkage removed. */
void denoiseOutput(const DenoiseContext &c, LabImage *labdn, Imagefloat *dst,
                   float qhighFactor, float realred, float realblue)
{
#ifdef _OPENMP
#pragma omp parallel for num_threads(c.denoiseNestedLevels)
#endif
    for (int i = 0; i < c.H; ++i) {
        for (int j = 0; j < c.W; ++j) {
            const float c_h =
                sqrt(SQR(labdn->a[i][j]) + SQR(labdn->b[i][j]));

            if (c_h > 3000.f) {
                labdn->a[i][j] *= 1.f + qhighFactor * realred / 100.f;
                labdn->b[i][j] *= 1.f + qhighFactor * realblue / 100.f;
            }

            float X, Y, Z;
            if (c.lab_mode) {
                Color::lab2rgb(labdn->L[i][j], labdn->a[i][j], labdn->b[i][j],
                               X, Y, Z, c.wpi_inverse);
            } else {
                Color::yuv2rgb(labdn->L[i][j], labdn->b[i][j], labdn->a[i][j],
                               X, Y, Z, c.wpi);
            }

            X = c.applyIGamma(X);
            Y = c.applyIGamma(Y);
            Z = c.applyIGamma(Z);

            if (c.lab_mode) {
                X = Color::denoiseGammaTab[X];
                Y = Color::denoiseGammaTab[Y];
                Z = Color::denoiseGammaTab[Z];
            }

            dst->r(i, j) = X;
            dst->g(i, j) = Y;
            dst->b(i, j) = Z;
        }
    }
}


/* Everything RGB_denoise derives before it knows -- or cares -- which backend
 * runs the eight phases: the modes, the strengths, the gamma tables, the
 * working-space matrices and the quarter-resolution noise maps.
 *
 * Non-copyable on purpose: `ctx` holds pointers into this object's own
 * members, so a copy would alias the wrong tables and a temporary would
 * dangle.  finalize() sets those pointers and must run after the members it
 * points at are filled. */
struct DenoisePrep {
    DenoisePrep():
        W(0), H(0), scale(1.0), nrQuality(QUALITY_STANDARD), autoch(false),
        denoiseLuminance(false), lab_mode(false), useNoiseCCurve(true),
        qhighFactor(1.f), noisevarL(0.f), noisevarab_r(0.f), noisevarab_b(0.f),
        realred(0.f), realblue(0.f), params_Ldetail(0.f), detail_thresh(0),
        gam(1.f), gamthresh(0.001f), gamslope(1.f), igamthresh(0.001f),
        igamslope(1.f), gamcurve(65536, LUT_CLIP_BELOW),
        igamcurve(65536, LUT_CLIP_BELOW), levwav(0), numthreads(1),
        denoiseNestedLevels(1)
    {
    }

    int W, H;
    double scale;
    nrquality nrQuality;
    bool autoch, denoiseLuminance, lab_mode, useNoiseCCurve;
    float qhighFactor;

    float noisevarL, noisevarab_r, noisevarab_b;
    float realred, realblue;

    float params_Ldetail;      // phase 7
    int detail_thresh;         // phase 7

    float wpi[3][3], wpi_inverse[3][3];

    float gam, gamthresh, gamslope, igamthresh, igamslope;
    LUTf gamcurve, igamcurve;

    /* The chroma noise curve RGB_denoise's phase 1 classifies half-resolution
     * samples against.  One fixed curve, built once here regardless of which
     * driver runs -- construction is O(1), not O(image) -- and read by
     * whichever of computeNoisevarMapsCPU / DenoiseSession::fillNoiseVarMaps
     * ends up doing the O(image) work below. */
    denoise::NoiseCurve noiseCCurve;

    /* The quarter-resolution maps phases 4 and 5 read.  Backing store for
     * ctx.noisevarlum/noisevarchrom, sized here so those pointers are never
     * null (both DenoiseSession::waveletCore and the standalone wavelet entry
     * gate on non-null, though only the latter actually reads through them --
     * see the note where RGB_denoise_GPU handles a mid-flight decline), but
     * left uninitialized: computeNoisevarMapsCPU or a successful device fill
     * is what actually writes them, and exactly one of those runs. */
    std::vector<float> lumcalcBuf, ccalcBuf;

    int levwav;                // waveletLevels(), computed once so the CPU
                               // and GPU phases cannot disagree about it
    int numthreads;
    int denoiseNestedLevels;   // nested OpenMP thread count for the phases'
                               // inner parallel regions

    DenoiseContext ctx;

    void finalize()
    {
        ctx.W = W;
        ctx.H = H;
        ctx.W2 = (W + 1) / 2;
        ctx.scale = scale;
        ctx.lab_mode = lab_mode;
        ctx.useNoiseCCurve = useNoiseCCurve;
        ctx.noisevarL = noisevarL;
        ctx.noisevarab_r = noisevarab_r;
        ctx.noisevarab_b = noisevarab_b;
        ctx.wpi = wpi;
        ctx.wpi_inverse = wpi_inverse;
        ctx.gam = gam;
        ctx.gamthresh = gamthresh;
        ctx.gamslope = gamslope;
        ctx.igamthresh = igamthresh;
        ctx.igamslope = igamslope;
        ctx.gamcurve = &gamcurve;
        ctx.igamcurve = &igamcurve;
        ctx.noisevarlum = lumcalcBuf.empty() ? nullptr : &lumcalcBuf[0];
        ctx.noisevarchrom = ccalcBuf.empty() ? nullptr : &ccalcBuf[0];
        /* Both buffers are sized (below in denoisePrepare) but not yet
         * filled; computeNoisevarMapsCPU or a device fill still has to run
         * before either is read. */
        ctx.denoiseNestedLevels = denoiseNestedLevels;
    }

private:
    DenoisePrep(const DenoisePrep &);
    DenoisePrep &operator=(const DenoisePrep &);
};

/* Fills `p` from the parameters and the source pixels.  False means there is
 * nothing to denoise and neither driver should run.
 *
 * Reads src->r/g/b through the planar accessors to build the chroma noise
 * map, so the caller must have brought the image to the host first.  That is
 * the one thing here a GPU driver would rather not pay for; moving the map
 * onto the device is what would let the entry sync go with it. */
bool denoisePrepare(ImProcData &im, Imagefloat *src,
                    const procparams::DenoiseParams &dnparams, DenoisePrep &p)
{
    const ProcParams *params = im.params;

    p.noiseCCurve.Set(
        {FCT_MinMaxCPoints, 0.05, 0.50, 0.35, 0.35, 0.35, 0.05, 0.35, 0.35});

    p.scale = im.scale;
    p.nrQuality = (!dnparams.aggressive) ? QUALITY_STANDARD : QUALITY_HIGH;
    p.qhighFactor = (p.nrQuality == QUALITY_HIGH)
                        ? 1.f / static_cast<float>(0.9 /*settings->nrhigh*/)
                        : 1.0f;
    p.useNoiseCCurve = true;
    p.autoch = dnparams.chrominanceMethod ==
               procparams::DenoiseParams::ChrominanceMethod::AUTOMATIC;
    p.lab_mode =
        dnparams.colorSpace == procparams::DenoiseParams::ColorSpace::LAB;

    // init luma noisevarL
    const float noiseluma = static_cast<float>(dnparams.luminance);
    p.noisevarL =
        static_cast<float>(SQR((noiseluma / 125.0) * (1.0 + noiseluma / 25.0)));
    p.denoiseLuminance = (p.noisevarL > 0.00001f);

    TMatrix wprofi =
        ICCStore::getInstance()->workingSpaceMatrix(params->icm.workingProfile);
    TMatrix wprofi_inverse = ICCStore::getInstance()->workingSpaceInverseMatrix(
        params->icm.workingProfile);
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            p.wpi[i][j] = static_cast<float>(wprofi[i][j]);
            p.wpi_inverse[i][j] = static_cast<float>(wprofi_inverse[i][j]);
        }
    }

    if (p.useNoiseCCurve) {
        /* Sized here so ctx.noisevarlum/noisevarchrom are never null (see
         * the note on DenoisePrep), but not filled: that O(image) work is
         * each driver's own, in computeNoisevarMapsCPU or
         * DenoiseSession::fillNoiseVarMaps, so that a full-GPU run pays for
         * neither the host loop nor a needless upload. */
        const int wid = (src->getWidth() + 1) / 2;
        const int hei = (src->getHeight() + 1) / 2;
        p.lumcalcBuf.assign(std::size_t(hei) * wid, 0.f);
        p.ccalcBuf.assign(std::size_t(hei) * wid, 0.f);
    }

    p.H = src->getHeight();
    p.W = src->getWidth();

    if (dnparams.luminance == 0 && dnparams.chrominance == 0) {
        return false;
    }

    // gamma transform for input data
    p.gam = dnparams.gamma;
    p.gamthresh = 0.001f;
    p.gamslope =
        exp(log(static_cast<double>(p.gamthresh)) / p.gam) / p.gamthresh;
    Color::gammaf2lut(p.gamcurve, p.gam, p.gamthresh, p.gamslope, 65535.f,
                      65535.f);

    // inverse gamma transform for output data
    const float igam = 1.f / p.gam;
    p.igamthresh = p.gamthresh * p.gamslope;
    p.igamslope = 1.f / p.gamslope;
    Color::gammaf2lut(p.igamcurve, igam, p.igamthresh, p.igamslope, 65535.f,
                      65535.f);

    // max out to avoid div by zero when using noisevar_Ldetail as divisor
    p.params_Ldetail = min(float(dnparams.luminanceDetail), 99.9f);
    p.detail_thresh = dnparams.luminanceDetailThreshold;

    /* The two `ponder` overrides that used to sit here -- one reading
     * ch_M/max_r/max_b, one zeroing the chroma strengths -- were both gated on
     * a hardcoded `false`.  They are the sole readers of ch_M/max_r/max_b,
     * which is why those parameters are now unused (see the note on
     * RGB_denoise's signature). */
    const float interm_med = static_cast<float>(dnparams.chrominance) / 10.0;
    // increase slower than linear for more sensitivity below zero
    const float intermred = dnparams.chrominanceRedGreen > 0.
                                ? float(dnparams.chrominanceRedGreen / 10.)
                                : float(dnparams.chrominanceRedGreen) / 7.0f;
    const float intermblue = dnparams.chrominanceBlueYellow > 0.
                                 ? float(dnparams.chrominanceBlueYellow / 10.)
                                 : float(dnparams.chrominanceBlueYellow) / 7.0f;

    p.realred = interm_med + intermred;
    if (p.realred <= 0.f) {
        p.realred = 0.001f;
    }
    p.realblue = interm_med + intermblue;
    if (p.realblue <= 0.f) {
        p.realblue = 0.001f;
    }
    p.noisevarab_r = SQR(p.realred);
    p.noisevarab_b = SQR(p.realblue);

    p.levwav =
        waveletLevels(p.realred, p.realblue, p.nrQuality, p.scale, p.W, p.H);

    p.numthreads = 1;

    /* p.denoiseNestedLevels is read by every phase's num_threads(...) pragma
     * here and in wavelet.cc, so it has to be set before any phase runs --
     * and only on a call that actually denoises, which is why this sits
     * after the early return above rather than at the top. */
#ifdef _OPENMP
    p.denoiseNestedLevels = omp_get_num_procs() / p.numthreads;

    if (p.denoiseNestedLevels < 2) {
        p.denoiseNestedLevels = 1;
    }

    if (options.rgbDenoiseThreadLimit > 0)
        while (p.denoiseNestedLevels * p.numthreads >
               options.rgbDenoiseThreadLimit) {
            p.denoiseNestedLevels--;
        }

    if (settings->verbose) {
        printf("RGB_denoise uses %d thread(s)\n", p.denoiseNestedLevels);
    }
#endif // _OPENMP

    p.finalize();
    return true;
}

// True when a GPU denoise is worth attempting at all.
bool denoiseGPUUsable(gpu::Context *ctx)
{
    return ctx && gpu::available() && gpu::opEnabled("denoise");
}

/* Phases 1-8 on the host.  No session, no residency, no gpu:: identifier: this
 * is the CPU implementation of denoise and nothing else. */
void RGB_denoise_CPU(Imagefloat *src, const DenoisePrep &prep)
{
    Imagefloat *dst = src;
    const DenoiseContext &ctx = prep.ctx;
    const int width = prep.W, height = prep.H;

    DctWorkspace dct(width,
                     std::size_t(prep.denoiseNestedLevels) * prep.numthreads);

    array2D<float> *Lin = nullptr;              // input L channel
    LabImage *labdn = new LabImage(width, height);  // wavelet denoised image

    {
        ART_PROFILE_SCOPE("denoise:fill"); // phase 1
        computeNoisevarMapsCPU(ctx, src, prep.noiseCCurve);
        denoiseFill(ctx, src, labdn);
    }

    denoiseWaveletCPU(ctx, labdn, Lin, prep.levwav, prep.nrQuality, prep.autoch,
                      prep.denoiseLuminance); // phases 2-6

    if (prep.denoiseLuminance) {
        // now do detail recovery using block DCT to detect patterns missed by
        // wavelet denoise; blocks are not the same thing as tiles!
        ART_PROFILE_SCOPE("denoise:dct"); // phase 7
        array2D<float> mask(ARRAY2D_ALIGNED);
        buildDetailMask(width, height, labdn, prep.detail_thresh, prep.scale,
                        mask);
        detailRecoveryCPU(width, height, labdn, Lin, prep.numthreads,
                          prep.denoiseNestedLevels, dct, prep.params_Ldetail,
                          prep.detail_thresh, mask, prep.scale,
                          prep.nrQuality == QUALITY_HIGH);
    }

    // Phase 8: inverse colour transform back to RGB.
    {
        ART_PROFILE_SCOPE("denoise:out"); // phase 8
        denoiseOutput(ctx, labdn, dst, prep.qhighFactor, prep.realred,
                      prep.realblue);
    }

    delete labdn;
    delete Lin;
}

/* Phases 1-8 with the GPU in play.
 *
 * False means the GPU is not usable here and nothing was touched, so the
 * caller runs RGB_denoise_CPU.  Past that point this owns the call: each phase
 * attempts the device and, when it declines, brings the planes down and calls
 * the same CPU phase helper the CPU driver uses.  `onDevice` is the single
 * piece of state recording which side currently owns the planes.
 *
 * Falling back per phase rather than as a whole is deliberate.  Phase 7's
 * block buffer is ~670 MiB at 24 Mpix and can exceed the device's memory
 * budget while the wavelet core still fits, so "phases 1-6 on the device,
 * phase 7 on the host" is a configuration real hardware lands in; an
 * all-or-nothing gate would demote those runs to fully-CPU.  Every fallback is
 * decided before the dispatch that would make it unsafe. */
bool RGB_denoise_GPU(ImProcData &im, Imagefloat *src, const DenoisePrep &prep)
{
    gpu::Context *gpuCtx = im.ipf ? im.ipf->getGPUContext() : nullptr;
    if (!denoiseGPUUsable(gpuCtx)) {
        return false;
    }

    Imagefloat *dst = src;
    const DenoiseContext &ctx = prep.ctx;
    const int width = prep.W, height = prep.H;

    gpu::ops::DenoiseSession session;
    session.pool = im.ipf ? im.ipf->getGPUPool() : nullptr;
    {
        ART_PROFILE_SCOPE("denoise:gpu:init");
        session.init(width, height, gpuCtx);
    }

    gpu::ops::DenoiseSession::FillParams fpar;
    fpar.lab_mode = prep.lab_mode;
    fpar.gam = prep.gam;
    fpar.gamthresh = prep.gamthresh;
    fpar.gamslope = prep.gamslope;
    fpar.igamthresh = prep.igamthresh;
    fpar.igamslope = prep.igamslope;
    fpar.wp = &prep.wpi[0][0];
    fpar.iwp = &prep.wpi_inverse[0][0];
    fpar.boost_a = 1.f + prep.qhighFactor * prep.realred / 100.f;
    fpar.boost_b = 1.f + prep.qhighFactor * prep.realblue / 100.f;

    /* Phase 7's CPU-only resources.  Constructing this is cheap; the FFTW
     * plans and tile masks inside it are built on first use, so a run whose
     * phase 7 stays on the device never pays for them. */
    DctWorkspace dct(width,
                     std::size_t(prep.denoiseNestedLevels) * prep.numthreads);

    array2D<float> *Lin = nullptr;
    LabImage *labdn = new LabImage(width, height);

    bool onDevice = false;
    /* Whether the noise-variance maps are currently the device's own copy
     * (fillNoiseVarMaps wrote them straight into the session) rather than
     * DenoisePrep's host arrays.  Independent of `onDevice`: this kernel
     * reads src's original pixels directly and does not depend on phase 1b's
     * (the RGB->Lab/YUV conversion's) own success or failure. */
    bool nvOnDevice = false;

    {
        ART_PROFILE_SCOPE_NAMED(prof_fill, "denoise:fill"); // phase 1
        nvOnDevice =
            session.valid() &&
            session.fillNoiseVarMaps(src, prep.noiseCCurve, prep.wpi,
                                     prep.noisevarL,
                                     max(prep.noisevarab_b, prep.noisevarab_r),
                                     gpuCtx);
        if (!nvOnDevice) {
            computeNoisevarMapsCPU(ctx, src, prep.noiseCCurve);
        }
        if (session.valid() && session.fill(src->r.ptrs, src->g.ptrs,
                                            src->b.ptrs, fpar, gpuCtx)) {
            onDevice = true;
            if (!nvOnDevice) {
                session.syncNoisevarToDevice(ctx.noisevarlum,
                                             ctx.noisevarchrom);
            }
        } else {
            denoiseFill(ctx, src, labdn);
        }
    }

    /* Phases 2-6.  No unconditional sync between phase 1 and here: both sides
     * have on-device entries, so the planes stay where phase 1 left them and
     * `onDevice` records where that is.  The crossing this used to make
     * unconditional is three planes each way, ~20 ms at 24 Mpix, spent purely
     * to hand one device kernel's output to another. */
    {
        const gpu::ops::DenoiseWaveletGPU wp =
            makeWaveletParams(ctx, prep.levwav, prep.nrQuality, prep.autoch,
                              prep.denoiseLuminance);

        /* Already on the device: run in place and leave them there.  This is
         * the case that matters -- the host-plane entry below costs a full
         * round trip of three planes purely to hand phase 1's output to phase
         * 2 and phase 6's to phase 7, ~35 ms at 24 Mpix.  Lin stays on the
         * device too, so the host copy is never made. */
        bool wavDone = onDevice && session.waveletCore(wp, gpuCtx);

        if (!wavDone) {
            /* Phase 1 ran on the CPU, or the on-device attempt declined.
             * Either way the planes have to be on the host from here on. */
            if (onDevice) {
                ART_PROFILE_SCOPE("denoise:gpu:down");
                session.syncToHost(labdn->L, labdn->a, labdn->b);
                onDevice = false;
            }
            /* Same for the noise-variance maps: the standalone entry below
             * re-uploads ctx.noisevarlum/noisevarchrom itself, from these
             * host arrays -- it has no persistent session state of its own.
             * If fillNoiseVarMaps left them device-only, bring them down now;
             * nothing has read their values yet. */
            if (nvOnDevice) {
                ART_PROFILE_SCOPE("denoise:gpu:down");
                session.syncNoisevarToHost(ctx.noisevarlum, ctx.noisevarchrom);
                nvOnDevice = false;
            }
            wavDone = denoiseWaveletGPUHost(ctx, labdn, Lin, wp,
                                            prep.denoiseLuminance,
                                            session.pool, gpuCtx);
        }

        if (!wavDone) {
            denoiseWaveletCPU(ctx, labdn, Lin, prep.levwav, prep.nrQuality,
                              prep.autoch, prep.denoiseLuminance);
        }
    }

    // wavelet denoised L channel
    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    if (prep.denoiseLuminance) {
        // now do detail recovery using block DCT to detect patterns missed by
        // wavelet denoise; blocks are not the same thing as tiles!
        ART_PROFILE_SCOPE("denoise:dct"); // phase 7

        /* On-device first, and only when the planes are already there: this
         * entry needs Lin on the device, which only DenoiseSession::waveletCore
         * leaves behind -- a non-null Lin means a *host* snapshot exists, i.e.
         * waveletCore did not run.  It builds the detail mask itself when
         * luminanceDetailThreshold is set, from the same denoised L plane the
         * CPU uses, so that setting no longer forces the host path either. */
        bool dctDone = false;
        if (onDevice && session.valid() && !Lin) {
            gpu::ops::DetailRecoveryGPU drp;
            drp.params_Ldetail = prep.params_Ldetail;
            drp.detail_thresh = prep.detail_thresh;
            drp.mask = nullptr;
            drp.scale = prep.scale;
            dctDone = session.detailRecovery(drp, gpuCtx);
        }

        if (!dctDone) {
            /* Bring everything down, Lin included: the host-plane entry and
             * the CPU one both need Lin as an array2D, and when waveletCore
             * ran there is no host copy yet. */
            if (onDevice) {
                ART_PROFILE_SCOPE("denoise:gpu:down");
                session.syncToHost(labdn->L, labdn->a, labdn->b);
                onDevice = false;
                if (!Lin) {
                    Lin = new array2D<float>(width, height);
                    session.syncLinToHost(*Lin);
                }
            }

            array2D<float> mask(ARRAY2D_ALIGNED);
            buildDetailMask(width, height, labdn, prep.detail_thresh,
                            prep.scale, mask);
            if (!detailRecoveryGPUHost(width, height, labdn, Lin,
                                       prep.params_Ldetail, prep.detail_thresh,
                                       mask, prep.scale, session.pool,
                                       gpuCtx)) {
                detailRecoveryCPU(width, height, labdn, Lin, prep.numthreads,
                                  prep.denoiseNestedLevels, dct,
                                  prep.params_Ldetail, prep.detail_thresh,
                                  mask, prep.scale,
                                  prep.nrQuality == QUALITY_HIGH);
            }
        }
    }
    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    // transform denoised "Lab" to output RGB

    {
        ART_PROFILE_SCOPE("denoise:out"); // phase 8
        bool outOnDevice = false;
        if (session.valid()) {
            /* Upload only if some earlier phase brought the planes down; when
             * phases 1-7 all ran on the device this is the first point at
             * which anything crosses, in either direction. */
            bool ready = onDevice;
            if (!ready) {
                ART_PROFILE_SCOPE("denoise:gpu:up");
                ready = session.syncToDevice(labdn->L, labdn->a, labdn->b);
            }
            /* session.output writes dst's residency buffer and leaves the
             * image GPU-resident, so nothing crosses here at all; whatever
             * runs next either picks it up on the device or pulls it back
             * once, at a boundary that actually needs host pixels. */
            outOnDevice = ready && session.output(dst, fpar, gpuCtx);
            if (outOnDevice) {
                onDevice = false; // session planes are spent; dst holds RGB
            }
        }
        if (!outOnDevice) {
            if (onDevice) {
                ART_PROFILE_SCOPE("denoise:gpu:down");
                session.syncToHost(labdn->L, labdn->a, labdn->b);
                onDevice = false;
            }
            denoiseOutput(ctx, labdn, dst, prep.qhighFactor, prep.realred,
                          prep.realblue);
        }
    }

    delete labdn;
    delete Lin;

    /* The "copy denoised image to output" step and the sRGB gamma restore that
     * used to sit here are both gone: the former only ran for numtiles > 1
     * (never -- Tile_calc always yields one tile, so dsttmp is dst itself),
     * the latter only for !isRAW (never -- the only call site passes a literal
     * true). */
    return true;
}

void RGB_denoise(ImProcData &im, Imagefloat *src,
                 const procparams::DenoiseParams &dnparams)
{
    // Bring the pixels to the host before touching r/g/b directly.
    src->syncCpuForWrite();

    BENCHFUN
    MyTime t1e, t2e;
    t1e.set();

    MyMutex::MyLock lock(*fftwMutex);

    /* denoisePrepare returning false means there is nothing to denoise.  The
     * timing line below still prints in that case, as it always has. */
    DenoisePrep prep;
    if (denoisePrepare(im, src, dnparams, prep)) {
        MyTime t1p, t2p;
        t1p.set();
        bool onGPU = RGB_denoise_GPU(im, src, prep);
        if (!onGPU) {
            RGB_denoise_CPU(src, prep);
        }
        if (settings->verbose) {
            t2p.set();
            std::cout << "RGB_denoise: executed on the "
                      << (onGPU ? "GPU" : "CPU") << " in "
                      << t2p.etime(t1p) << " usec" << std::endl;
        }
    }

    // #ifdef _DEBUG
    if (settings->verbose) {
        t2e.set();
        printf("Denoise performed in %d usec:\n", t2e.etime(t1e));
    }
    // #endif
}

#undef TS
#undef offset
#undef blkrad


void finalSmoothing(ImProcData &im, Imagefloat *img,
                    const procparams::DenoiseParams &dnparams)
{
    MyTime t1p, t2p;
    t1p.set();
    bool onGPU = gpu::ops::finalSmoothingGPU(im, img, dnparams);
    if (!onGPU) {
        img->syncCpuForWrite();
        denoise::denoiseGuidedSmoothing(im, img);
        if (dnparams.nlStrength) {
            img->setMode(Imagefloat::Mode::YUV, im.multiThread);
            array2D<float> tmp(img->getWidth(), img->getHeight(),
                               img->g.ptrs, ARRAY2D_BYREFERENCE);
            denoise::NLMeans(tmp, 65535.f, dnparams.nlStrength,
                             dnparams.nlDetail, im.scale, im.multiThread);
            img->setMode(Imagefloat::Mode::RGB, im.multiThread);
        }
    }
    if (settings->verbose) {
        t2p.set();
        std::cout << "finalSmoothing: executed on the "
                  << (onGPU ? "GPU" : "CPU") << " in " << t2p.etime(t1p)
                  << " usec" << std::endl;
    }
}

} // namespace denoise


} // namespace rtengine



#ifdef ART_USE_VULKAN

#include "color.h"
#include "imagefloat.h"
#include "settings.h"
#include "gpu/gpu.h"
#include "gpu/vk_pass.h"
#include "gpu/plane_io.h"
#include "gpu/ops.h"
#include "gpu/mat_layout.h"
#include "pipelineprofile.h"

#include <cmath>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <sstream>
#include <vector>

namespace rtengine {

extern const Settings *settings;

namespace gpu {
namespace ops {

namespace {

/* Dispatch counts, so PassSeq can be asked for the right amount of room.
 * Kept next to each other because they are a contract with op_wavelet.cc
 * rather than an implementation detail of it: if one of those ops changes its
 * dispatch count and these are not updated, the symptom is a failed dispatch
 * and a silent fall back to the CPU, not a wrong answer. */
unsigned int dwDecomposeDispatches(int levels)
{
    return (unsigned int)(3 * levels); // one FIR level, then two per a-trous
}

unsigned int dwMadDispatches()
{
    return 3; // the vkCmdFillBuffer does not consume a descriptor set
}

unsigned int dwShrinkChromaDispatches(bool aggressive)
{
    /* Level-fused, so independent of the level count: 3 per direction for the
     * blurred path, plus (when aggressive) a bi-shrink pass of one pointwise
     * dispatch per direction and a second blurred path over the top level,
     * and a second MAD. */
    return aggressive ? dwMadDispatches() + 3 + 9 + dwMadDispatches() + 9
                      : dwMadDispatches() + 9;
}

unsigned int dwShrinkLumaDispatches(int passes)
{
    return (unsigned int)(9 * passes);
}

} // namespace

/* Denoise phase 4 -- see denoiseShrinkChroma's contract in ipdenoise.h. */
bool denoiseShrinkChroma(Context &ctx, Pass &pass, BufferPool &pool,
                         WaveletBandsGPU &lBands, WaveletBandsGPU &abBands,
                         Buffer &noisevarchrom, Buffer &madL,
                         const int *radius, float noisevarAb, bool autoch,
                         bool useNoiseCCurve, bool aggressive)
{
    const int kMaxSegments = 24; // MAX_SEGMENTS in dn_boxblur_seg_h.comp

    const int levels = abBands.levels;
    if (levels <= 0 || levels > kMaxSegments || lBands.levels != levels ||
        !lBands.hi1 || !abBands.hi1 || !noisevarchrom.valid() ||
        !madL.valid() || !radius) {
        return false;
    }
    const size_t n = (size_t)abBands.w * abBands.h;
    if (!n || lBands.w != abBands.w || lBands.h != abBands.h) {
        return false;
    }

    /* ShrinkAllAB's and BiShrinkAB's shared preamble. */
    if (autoch && noisevarAb <= 0.001f) {
        noisevarAb = 0.02f;
    }
    if (noisevarAb <= 0.001f) {
        /* ShrinkAllAB's whole body is inside `if (noisevar_ab > 0.001f)`,
         * and the bi-shrink's pointwise branch likewise, so the CPU leaves
         * the coefficients alone.  Nothing to dispatch. */
        return true;
    }

    const float eps = 0.01f;
    const size_t total = n * (size_t)levels;

    Buffer *madAb = pool.get((size_t)levels * 3u * sizeof(float));
    Buffer *sf = pool.get(total * sizeof(float));
    Buffer *blurH = pool.get(total * sizeof(float));
    if (!madAb || !sf || !blurH) {
        return false;
    }

    struct SfPC {
        unsigned int n, levels, madBase;
        float madabScale;
    };
    struct BlurPC {
        unsigned int w, h, segments;
        int radius[24];
    };
    struct BlurApplyPC {
        unsigned int w, h, segments;
        int radius[24];
        float eps;
    };

    Buffer *abDirs[3] = {abBands.hi1, abBands.hi2, abBands.hi3};
    Buffer *lDirs[3] = {lBands.hi1, lBands.hi2, lBands.hi3};

    /* The blurred three-stage path over `segCount` levels starting at
     * `levelBase`: shrink factor, horizontal blur, then the vertical blur
     * fused with the recombination.  Shared by ShrinkAllAB (every level) and
     * by the bi-shrink's top level (one level). */
    const auto blurredPath = [&](int levelBase, int segCount,
                                 float madabScale) {
        BlurPC bpc{};
        bpc.w = (unsigned)abBands.w;
        bpc.h = (unsigned)abBands.h;
        bpc.segments = (unsigned)segCount;
        BlurApplyPC vpc{};
        vpc.w = bpc.w;
        vpc.h = bpc.h;
        vpc.segments = bpc.segments;
        vpc.eps = eps;
        for (int k = 0; k < segCount; ++k) {
            bpc.radius[k] = radius[levelBase + k];
            vpc.radius[k] = radius[levelBase + k];
        }

        for (int d = 0; d < 3; ++d) {
            const size_t byteOff = (size_t)levelBase * n * sizeof(float);
            const size_t byteLen = (size_t)segCount * n * sizeof(float);

            Pass::Binding abIn(abDirs[d], false);
            abIn.offset = byteOff;
            abIn.range = byteLen;
            Pass::Binding lIn(lDirs[d], false);
            lIn.offset = byteOff;
            lIn.range = byteLen;

            SfPC spc{(unsigned)n, (unsigned)segCount,
                     (unsigned)(d * levels + levelBase), madabScale};
            std::vector<Pass::Binding> bs;
            bs.push_back(abIn);
            bs.push_back(lIn);
            bs.push_back(Pass::Binding(&noisevarchrom, false));
            bs.push_back(Pass::Binding(madAb, false));
            bs.push_back(Pass::Binding(&madL, false));
            bs.push_back(Pass::Binding(sf, true));
            if (!pass.dispatch1D("dn_shrink_ab_sf", bs, &spc, sizeof(spc),
                                n * (size_t)segCount)) {
                return false;
            }

            std::vector<Pass::Binding> bh;
            bh.push_back(Pass::Binding(sf, false));
            bh.push_back(Pass::Binding(blurH, true));
            if (!pass.dispatch2D("dn_boxblur_seg_h", bh, &bpc, sizeof(bpc),
                                abBands.w, abBands.h * segCount)) {
                return false;
            }

            Pass::Binding abOut(abDirs[d], true);
            abOut.offset = byteOff;
            abOut.range = byteLen;
            std::vector<Pass::Binding> bv;
            bv.push_back(Pass::Binding(blurH, false));
            bv.push_back(Pass::Binding(sf, false));
            bv.push_back(abOut);
            if (!pass.dispatch2D("dn_shrink_blurv_apply", bv, &vpc,
                                sizeof(vpc), abBands.w,
                                abBands.h * segCount)) {
                return false;
            }
        }
        return true;
    };

    if (aggressive) {
        /* madab from the *original* chroma coefficients.  BiShrinkAB
         * computes all of them in an omp-for that finishes before any shrink
         * begins, so every level's estimate predates every modification. */
        if (!waveletMadExact(ctx, pass, pool, abBands, *madAb)) {
            return false;
        }

        const float scaleHigh = useNoiseCCurve ? noisevarAb
                                               : noisevarAb * noisevarAb;

        /* Levels below the top: pointwise, squared factor, no blur -- one
         * dispatch per direction across all of them. */
        if (levels > 1) {
            for (int d = 0; d < 3; ++d) {
                SfPC spc{(unsigned)n, (unsigned)(levels - 1),
                         (unsigned)(d * levels), scaleHigh};
                std::vector<Pass::Binding> bb;
                bb.push_back(Pass::Binding(abDirs[d], true));
                bb.push_back(Pass::Binding(lDirs[d], false));
                bb.push_back(Pass::Binding(&noisevarchrom, false));
                bb.push_back(Pass::Binding(madAb, false));
                bb.push_back(Pass::Binding(&madL, false));
                if (!pass.dispatch1D("dn_shrink_ab_bishrink", bb, &spc,
                                    sizeof(spc), n * (size_t)(levels - 1))) {
                    return false;
                }
            }
        }

        /* The top level takes the blurred path, with ShrinkAllAB's own
         * weighting rather than the bi-shrink's. */
        const float scaleTop = useNoiseCCurve ? 1.f : noisevarAb;
        if (!blurredPath(levels - 1, 1, scaleTop)) {
            return false;
        }
    }

    /* WaveletDenoiseAllAB, always: every level through the blurred path,
     * with madab recomputed from whatever the coefficients are now -- which
     * after a bi-shrink means the shrunk ones.  That second estimate is not
     * an oversight in the CPU; ShrinkAllAB only skips the recomputation when
     * its caller passes madCalculated, and this caller does not. */
    if (!waveletMadExact(ctx, pass, pool, abBands, *madAb)) {
        return false;
    }
    const float scaleAll = useNoiseCCurve ? 1.f : noisevarAb;
    return blurredPath(0, levels, scaleAll);
}

/* Denoise phase 5 -- see denoiseShrinkLuma's contract in ipdenoise.h. */
bool denoiseShrinkLuma(Context &ctx, Pass &pass, BufferPool &pool,
                       WaveletBandsGPU &bands, Buffer &noisevarlum,
                       Buffer &mad, const int *radius, int shrinkLevels,
                       int passes)
{
    (void)ctx;
    /* Matches MAX_SEGMENTS in dn_boxblur_seg_{h,v}.comp, which bounds their
     * per-segment radius array. */
    const int kMaxSegments = 24;

    const int levels = bands.levels;
    if (levels <= 0 || shrinkLevels <= 0 || shrinkLevels > levels ||
        shrinkLevels > kMaxSegments || passes <= 0 || !noisevarlum.valid() ||
        !mad.valid() || !radius) {
        return false;
    }
    const size_t n = (size_t)bands.w * bands.h;
    const size_t total = n * (size_t)shrinkLevels;
    if (!n) {
        return false;
    }

    /* Two scratch planes, reused across directions and passes: the stages are
     * strictly serial and Pass::barrierFor derives the write-after-read
     * barriers.  sf holds the unblurred shrink factor, blurH the horizontal
     * half of its box blur; the vertical half is never materialised, since
     * dn_shrink_blurv_apply consumes it as it computes it.  This stage is
     * bandwidth-bound (about ten plane-passes per direction at 64 GB/s
     * against an 82 GB/s copy floor), so removing a plane write and a plane
     * read is worth more than the dispatch it saves.
     *
     * Sized for one direction rather than all three, which costs 9
     * dispatches per pass instead of 3 but a third of the memory (67 MB
     * against 200 at 12.5 Mpix and 8 levels).  Worth it here because a
     * dispatch over levels*n coefficients -- 25M in that case -- already
     * saturates this GPU; the penalty measured for undersized dispatches in
     * waveletMadExact does not apply at this size. */
    Buffer *sf = pool.get(total * sizeof(float));
    Buffer *blurH = pool.get(total * sizeof(float));
    if (!sf || !blurH) {
        return false;
    }

    const float eps = 0.01f;

    struct SfPC {
        unsigned int n, levels, madBase;
        float eps;
    };
    struct BlurPC {
        unsigned int w, h, segments;
        int radius[24];
    };
    struct BlurApplyPC {
        unsigned int w, h, segments;
        int radius[24];
        float eps;
    };

    BlurPC bpc{};
    bpc.w = (unsigned)bands.w;
    bpc.h = (unsigned)bands.h;
    bpc.segments = (unsigned)shrinkLevels;
    BlurApplyPC vpc{};
    vpc.w = bpc.w;
    vpc.h = bpc.h;
    vpc.segments = bpc.segments;
    vpc.eps = eps;
    for (int lvl = 0; lvl < shrinkLevels; ++lvl) {
        bpc.radius[lvl] = radius[lvl];
        vpc.radius[lvl] = radius[lvl];
    }

    Buffer *dirs[3] = {bands.hi1, bands.hi2, bands.hi3};

    for (int p = 0; p < passes; ++p) {
        for (int d = 0; d < 3; ++d) {
            /* `mad` is laid out direction-major over the decomposition's
             * full level count, so this direction's slice starts at
             * d*levels -- not d*shrinkLevels. */
            SfPC spc{(unsigned)n, (unsigned)shrinkLevels,
                     (unsigned)(d * levels), eps};
            std::vector<Pass::Binding> bs;
            bs.push_back(Pass::Binding(dirs[d], false));
            bs.push_back(Pass::Binding(&noisevarlum, false));
            bs.push_back(Pass::Binding(&mad, false));
            bs.push_back(Pass::Binding(sf, true));
            if (!pass.dispatch1D("dn_shrink_l_sf", bs, &spc, sizeof(spc),
                                total)) {
                return false;
            }

            std::vector<Pass::Binding> bh;
            bh.push_back(Pass::Binding(sf, false));
            bh.push_back(Pass::Binding(blurH, true));
            if (!pass.dispatch2D("dn_boxblur_seg_h", bh, &bpc, sizeof(bpc),
                                bands.w, bands.h * shrinkLevels)) {
                return false;
            }

            std::vector<Pass::Binding> bv;
            bv.push_back(Pass::Binding(blurH, false));
            bv.push_back(Pass::Binding(sf, false));
            bv.push_back(Pass::Binding(dirs[d], true));
            if (!pass.dispatch2D("dn_shrink_blurv_apply", bv, &vpc,
                                sizeof(vpc), bands.w,
                                bands.h * shrinkLevels)) {
                return false;
            }
        }
    }

    return true;
}

/* Phases 2-6 over buffers the caller already owns.  `bLdn` is the luma
 * reconstruction target: pass &bL to reconstruct in place (the caller has
 * snapshotted Lin on the host), or a separate plane to keep bL as Lin for a
 * device-side phase 7.
 *
 * Records into `seq` and leaves it unflushed except at the chroma channel
 * boundaries, where the pool has to reclaim the bands. */
bool waveletCoreImpl(Context &ctx, BufferPool &pool, PassSeq &seq, int W,
                     int H, Buffer &bL, Buffer &bA, Buffer &bB, Buffer &bLdn,
                     Buffer &bNvLum, Buffer &bNvChrom,
                     const DenoiseWaveletGPU &p)
{
    const int levels = p.levels;

    Buffer *bMadL = pool.get(size_t(levels) * 3u * sizeof(float));
    if (!bMadL) {
        return false;
    }

    /* ShrinkAllL/ShrinkAllAB's box-blur radius, max(1, int((level+2)/scale))
     * (ipdenoise.cc:197,315), one entry per level because the shrink dispatches
     * every level in one go and indexes this by segment. */
    int radius[24];
    for (int lvl = 0; lvl < levels; ++lvl) {
        radius[lvl] = std::max(1, int((lvl + 2) / p.scale));
    }

    /* Phase 2 for L, then phase 3.  L's bands and its coarsest low-pass plane
     * stay checked out for the whole call: madL and the chroma shrink both
     * read the *original* coefficients. */
    WaveletBandsGPU lBands;
    Buffer *llL = nullptr;
    int llW = 0, llH = 0;
    {
        Pass *ps = seq.reserve(dwDecomposeDispatches(levels) +
                               dwMadDispatches());
        if (!ps ||
            !waveletDecompose(ctx, *ps, pool, bL, W, H, levels, lBands, llL,
                              llW, llH) ||
            !waveletMadExact(ctx, *ps, pool, lBands, *bMadL)) {
            logOnce("GPU: denoise wavelet decompose/mad failed; using the CPU");
            return false;
        }
    }

    /* Everything from here is per-channel scratch.  The mark is taken after
     * L's bands so release() below cannot hand them out as a chroma band. */
    const size_t mark = pool.mark();

    /* Phases 4 and 6 for a and b.  Both chroma channels run before the luma
     * shrink, because the chroma shrink factor reads L's unshrunk detail
     * coefficients -- the CPU's ordering, and not negotiable. */
    for (int ch = 0; ch < 2; ++ch) {
        Buffer &plane = ch == 0 ? bA : bB;
        const float noisevarAb = ch == 0 ? p.noisevarab_r : p.noisevarab_b;

        WaveletBandsGPU abBands;
        Buffer *llAb = nullptr;
        int aW = 0, aH = 0;
        Pass *ps = seq.reserve(dwDecomposeDispatches(levels) +
                               dwShrinkChromaDispatches(p.aggressive));
        if (!ps ||
            !waveletDecompose(ctx, *ps, pool, plane, W, H, levels, abBands,
                              llAb, aW, aH) ||
            !denoiseShrinkChroma(ctx, *ps, pool, lBands, abBands, bNvChrom,
                                 *bMadL, radius, noisevarAb, p.autoch,
                                 p.useNoiseCCurve, p.aggressive)) {
            logOnce("GPU: denoise chroma shrink failed; using the CPU");
            return false;
        }
        ps = seq.reserve(dwDecomposeDispatches(levels));
        if (!ps || !waveletReconstruct(ctx, *ps, pool, abBands, llAb, aW, aH,
                                       plane, W, H)) {
            logOnce("GPU: denoise chroma reconstruct failed; using the CPU");
            return false;
        }
        /* The channel's bands and scratch may only go back to the pool once
         * the GPU has actually run the dispatches that read them. */
        if (!seq.flush()) {
            return false;
        }
        pool.release(mark);
    }

    /* Phases 5 and 6 for L.  Skipped entirely when the luminance slider is
     * off: the CPU decomposes L regardless (madL needs it) but neither
     * shrinks nor reconstructs it, so labdn->L comes out unchanged. */
    if (p.denoiseLuminance) {
        /* Both CPU luma functions cap themselves at five levels
         * (ipdenoise.cc:368,702), so levels 5..7 of an 8-level
         * decomposition keep the coefficients the chroma shrink already
         * used.  QUALITY_HIGH runs the shrink twice with the same madL --
         * WaveletDenoiseAll_BiShrinkL's else branch is ShrinkAllL's body. */
        const int shrinkLevels = std::min(levels, 5);
        const int passes = p.aggressive ? 2 : 1;
        Pass *ps = seq.reserve(dwShrinkLumaDispatches(passes) +
                               dwDecomposeDispatches(levels));
        if (!ps ||
            !denoiseShrinkLuma(ctx, *ps, pool, lBands, bNvLum, *bMadL, radius,
                               shrinkLevels, passes) ||
            !waveletReconstruct(ctx, *ps, pool, lBands, llL, llW, llH, bLdn, W,
                                H)) {
            logOnce("GPU: denoise luma shrink failed; using the CPU");
            return false;
        }
    }
    return true;
}

/* Common validation for both entry points. */
bool dwGeometryOk(int W, int H, const DenoiseWaveletGPU &p, Context *ctx,
                  size_t *totalOut)
{
    const int levels = p.levels;
    const int W2 = (W + 1) / 2, H2 = (H + 1) / 2;
    const size_t planeBytes = size_t(W) * size_t(H) * sizeof(float);
    const size_t lvlBytes = size_t(W2) * size_t(H2) * sizeof(float);
    const size_t bandBytes = lvlBytes * size_t(levels);

    if (planeBytes > ctx->caps().max_storage_buffer_range ||
        bandBytes > ctx->caps().max_storage_buffer_range) {
        logOnce("GPU: denoise wavelet planes exceed maxStorageBufferRange; "
                "needs tiling");
        return false;
    }

    /* Peak, with the pool sharing scratch across the two chroma channels:
     * L's three band buffers plus one chroma set (6*bandBytes), the shrink's
     * sf and blurH (2*bandBytes), the decompose/reconstruct scratch (two
     * W x H2 planes plus four level planes), the three full planes, the two
     * noise maps, and the MAD histograms.  ~1.5 GB at 24 Mpix and 5 levels.
     *
     * Declining is a real outcome, not a formality: the CPU path allocates
     * comparable amounts in wavelet_decomposition objects, so a machine that
     * cannot hold this cannot hold either, and falling back keeps the failure
     * on the side that at least works. */
    const size_t scratchBytes = 2 * size_t(W) * size_t(H2) * sizeof(float) +
                                4 * lvlBytes;
    const size_t histBytes = size_t(levels) * 3u * 65536u * sizeof(unsigned);
    const size_t total = 8 * bandBytes + scratchBytes + 3 * planeBytes +
                         2 * lvlBytes + histBytes;
    const size_t budget = dnMemoryBudget(ctx);
    if (total > budget) {
        dnLogOverBudget("wavelet phases", total, budget);
        return false;
    }
    if (totalOut) {
        *totalOut = total;
    }
    return true;
}

namespace {

/* Upload W x H row-pointer CPU data into a (possibly unmapped, on a
 * discrete GPU) pool buffer: mapped() fast path, or pack-and-stage via
 * gpu::uploadToBuffer otherwise. Mirrors Impl::up() below, for callers that
 * are not DenoiseSession methods. */
bool dnUploadRows(Context &ctx, BufferPool &staging, float * const *rows,
                  int W, int H, Buffer &dst)
{
    const size_t bytes = size_t(W) * size_t(H) * sizeof(float);
    if (dst.mapped()) {
        float *m = (float *)dst.mapped();
        for (int y = 0; y < H; ++y) {
            std::memcpy(m + (size_t)y * W, rows[y], (size_t)W * sizeof(float));
        }
        dst.flush(0, bytes);
        return true;
    }
    std::vector<float> packed((size_t)W * H);
    for (int y = 0; y < H; ++y) {
        std::memcpy(&packed[(size_t)y * W], rows[y], (size_t)W * sizeof(float));
    }
    return uploadToBuffer(ctx, &staging, packed.data(), bytes, dst);
}

/* Download a W x H pool buffer back into row-pointer CPU data. Counterpart
 * to dnUploadRows(). */
bool dnDownloadRows(Context &ctx, BufferPool &staging, Buffer &src,
                    float * const *rows, int W, int H)
{
    const size_t bytes = size_t(W) * size_t(H) * sizeof(float);
    if (src.mapped()) {
        src.invalidate(0, bytes);
        const float *m = (const float *)src.mapped();
        for (int y = 0; y < H; ++y) {
            std::memcpy(rows[y], m + (size_t)y * W, (size_t)W * sizeof(float));
        }
        return true;
    }
    std::vector<float> packed((size_t)W * H);
    if (!downloadFromBuffer(ctx, &staging, src, 0, packed.data(), bytes)) {
        return false;
    }
    for (int y = 0; y < H; ++y) {
        std::memcpy(rows[y], &packed[(size_t)y * W], (size_t)W * sizeof(float));
    }
    return true;
}

bool dnWaveletStandalone(int W, int H, float **L, float **a, float **b,
                         const DenoiseWaveletGPU &p, BufferPool *poolp,
                         Context *ctx)
{
    if (W <= 0 || H <= 0 || !L || !a || !b || !p.noisevarlum ||
        !p.noisevarchrom || !poolp || !opEnabled("denoise") ||
        !available()) {
        return false;
    }
    /* 24 is MAX_SEGMENTS in the box-blur and shrink shaders, which bounds
     * their per-level radius array; denoise never asks for more than 8. */
    if (p.levels < 1 || p.levels > 24) {
        return false;
    }
    if (!ctx || !dwGeometryOk(W, H, p, ctx, nullptr)) {
        return false;
    }

    const int W2 = (W + 1) / 2, H2 = (H + 1) / 2;
    const size_t planeBytes = size_t(W) * size_t(H) * sizeof(float);
    const size_t lvlBytes = size_t(W2) * size_t(H2) * sizeof(float);

    // BufferPool *poolp = pools->get(ctx, "denoise", W, H);
    // if (!poolp) {
    //     return false;
    // }
    BufferPool &pool = *poolp;
    DnPoolCheckout checkout(pool);

    PipelineProfile::Timer t_alloc("denoise:wav:gpu:alloc");
    Buffer *bL = pool.get(planeBytes);
    Buffer *bA = pool.get(planeBytes);
    Buffer *bB = pool.get(planeBytes);
    Buffer *bNvLum = pool.get(lvlBytes);
    Buffer *bNvChrom = pool.get(lvlBytes);
    if (!bL || !bA || !bB || !bNvLum || !bNvChrom) {
        logOnce("GPU: denoise wavelet allocation failed; using the CPU");
        return false;
    }
    t_alloc.stop();

    PipelineProfile::Timer t_up("denoise:wav:gpu:up");
    {
        BufferPool &staging = ctx->stagingPoolForThisThread();
        if (!dnUploadRows(*ctx, staging, L, W, H, *bL) ||
            !dnUploadRows(*ctx, staging, a, W, H, *bA) ||
            !dnUploadRows(*ctx, staging, b, W, H, *bB) ||
            !uploadToBuffer(*ctx, &staging, p.noisevarlum, lvlBytes, *bNvLum) ||
            !uploadToBuffer(*ctx, &staging, p.noisevarchrom, lvlBytes,
                            *bNvChrom)) {
            return false;
        }
    }
    t_up.stop();

    PipelineProfile::Timer t_pass("denoise:wav:gpu:pass");
    PassSeq seq(*ctx, "denoise-wavelet");
    /* Reconstruct L in place: this path's caller keeps Lin on the host. */
    const bool ok = waveletCoreImpl(*ctx, pool, seq, W, H, *bL, *bA, *bB, *bL,
                                    *bNvLum, *bNvChrom, p) &&
                    seq.flush();
    t_pass.stop();
    if (!ok) {
        return false;
    }

    PipelineProfile::Timer t_down("denoise:wav:gpu:down");
    {
        BufferPool &staging = ctx->stagingPoolForThisThread();
        if (!dnDownloadRows(*ctx, staging, *bA, a, W, H) ||
            !dnDownloadRows(*ctx, staging, *bB, b, W, H)) {
            return false;
        }
        if (p.denoiseLuminance && !dnDownloadRows(*ctx, staging, *bL, L, W, H)) {
            return false;
        }
    }
    t_down.stop();

    if (settings && settings->verbose > 1) {
        std::cout << "GPU: denoise wavelet core (host planes), " << p.levels
                  << " levels, " << seq.submissions() << " submission(s), "
                  << seq.wallMs() << " ms" << std::endl;
    }
    return true;
}

} // namespace

bool denoiseWaveletGPU(int W, int H, float **L, float **a, float **b,
                       const DenoiseWaveletGPU &p, BufferPool *pool,
                       Context *ctx)
{
    return dnWaveletStandalone(W, H, L, a, b, p, pool, ctx);
}


struct DenoiseSession::Impl {
    Impl(): W(0), H(0), W2(0), H2(0) {}

    /* Three full-resolution planes, plus the two quarter-resolution noise
     * maps and one buffer for the gamma tables.
     *
     * The three planes hold RGB on the way in, L/a/b in the middle, and RGB
     * again on the way out -- there is no separate set for each, because
     * wav_rgb2yuv.comp (reused here) transforms in place and writes
     * (p0,p1,p2) = (u, Y, v).  So the Lab view of the same buffers is
     * L = p1, a = p2, b = p0, which is what labL()/labA()/labB() below name;
     * dn_rgb2lab.comp deliberately adopts the same layout so nothing outside
     * those accessors has to care which colour space is in use.  Aliasing
     * this way saves three plane allocations (150 MB at 12.5 Mpix) and two
     * full-image copies per call.
     *
     * Kept as separate buffers rather than one packed allocation because the
     * wavelet primitive downstream binds whole buffers (and slices of them
     * via Pass::Binding::offset). */
    Buffer p0, p1, p2;
    Buffer noisevarlum, noisevarchrom;
    /* Phase 7's Lin: the L plane as it was before the wavelet reconstruction
     * overwrote it.  Allocated on first use rather than in init(), because
     * only a run that keeps the planes on the device across phases 2-7 needs
     * it, and it is a full plane (96 MB at 24 Mpix). */
    Buffer lin;
    int W, H, W2, H2;

    Buffer &labL() { return p1; }
    Buffer &labA() { return p2; }
    Buffer &labB() { return p0; }

    /* One dn_gamma dispatch over all three planes.  See dn_gamma.comp for
     * why there is no LUT: every warp denoise uses is a closed form the GPU
     * can evaluate more accurately than the CPU's interpolated table. */
    bool gammaDispatch(Pass &pass, unsigned mode, float gam, float thresh,
                       float slope)
    {
        struct PC {
            unsigned int w, h, mode;
            float gam, thresh, slope;
            float tail0, tail1;
        } pc{(unsigned)W, (unsigned)H, mode, gam, thresh, slope, 0.f, 0.f};

        /* Modes 2 and 3 replace an *unguarded* LUT lookup
         * (Color::denoiseGammaTab / denoiseIGammaTab, both LUTf(65536, 0)),
         * so above index 65534 they must extrapolate off the end of the
         * table exactly as LUTf does instead of continuing the closed-form
         * curve -- see the tail note in dn_gamma.comp.  Take the two
         * endpoints from the tables themselves rather than recomputing them:
         * the slope is a difference of two values near 65535, so in float it
         * loses most of its significant digits, and the only way to lose
         * exactly the same ones the CPU does is to start from the same
         * stored floats. */
        if (mode == 2u || mode == 3u) {
            const LUTf &tab = (mode == 2u) ? Color::denoiseGammaTab
                                           : Color::denoiseIGammaTab;
            pc.tail0 = tab[(int)65534];
            pc.tail1 = tab[(int)65535];
        }

        std::vector<Pass::Binding> bd;
        bd.push_back(Pass::Binding(&p0, true));
        bd.push_back(Pass::Binding(&p1, true));
        bd.push_back(Pass::Binding(&p2, true));
        return pass.dispatch2D("dn_gamma", bd, &pc, sizeof(pc), W, H);
    }

    size_t planeBytes() const { return (size_t)W * H * sizeof(float); }
    size_t mapBytes() const { return (size_t)W2 * H2 * sizeof(float); }

    /* LabImage rows are contiguous (LabImage allocates one block and points
     * the row pointers into it), but this does not assume that: it copies row
     * by row, which is correct either way and costs nothing measurable next
     * to the total.
     *
     * Self-contained: unlike the old version, this does its own flush/
     * invalidate rather than leaving it to the caller, because the staged
     * (discrete-GPU) branch needs a different sequence (memcpy into a
     * staging buffer, then an on-device copy) that a caller-side flush()/
     * invalidate() on `dst` itself wouldn't apply to. Returns false only on
     * a discrete GPU whose staging path itself failed (no host-visible
     * memory, or the on-device copy failed) -- the mapped fast path always
     * succeeds once dst exists. */
    bool up(Buffer &dst, float **src) const
    {
        const size_t bytes = planeBytes();
        if (dst.mapped()) {
            float *m = (float *)dst.mapped();
            for (int y = 0; y < H; ++y) {
                std::memcpy(m + (size_t)y * W, src[y], (size_t)W * sizeof(float));
            }
            dst.flush(0, bytes);
            return true;
        }
        Context *ctx = Context::get();
        if (!ctx) {
            return false;
        }
        std::vector<float> packed((size_t)W * H);
        for (int y = 0; y < H; ++y) {
            std::memcpy(&packed[(size_t)y * W], src[y], (size_t)W * sizeof(float));
        }
        return uploadToBuffer(*ctx, &ctx->stagingPoolForThisThread(),
                              packed.data(), bytes, dst);
    }

    bool down(float **dst, Buffer &srcbuf) const
    {
        const size_t bytes = planeBytes();
        if (srcbuf.mapped()) {
            srcbuf.invalidate(0, bytes);
            const float *m = (const float *)srcbuf.mapped();
            for (int y = 0; y < H; ++y) {
                std::memcpy(dst[y], m + (size_t)y * W, (size_t)W * sizeof(float));
            }
            return true;
        }
        Context *ctx = Context::get();
        if (!ctx) {
            return false;
        }
        std::vector<float> packed((size_t)W * H);
        if (!downloadFromBuffer(*ctx, &ctx->stagingPoolForThisThread(), srcbuf,
                                0, packed.data(), bytes)) {
            return false;
        }
        for (int y = 0; y < H; ++y) {
            std::memcpy(dst[y], &packed[(size_t)y * W], (size_t)W * sizeof(float));
        }
        return true;
    }
};

DenoiseSession::DenoiseSession(): p_(nullptr) {}

DenoiseSession::~DenoiseSession() { delete p_; }

bool DenoiseSession::valid() const { return p_ != nullptr; }

bool DenoiseSession::init(int W, int H, Context *ctx)
{
    delete p_;
    p_ = nullptr;

    if (W <= 0 || H <= 0 || !opEnabled("denoise") || !available()) {
        return false;
    }

    if (!ctx) {
        return false;
    }

    std::unique_ptr<Impl> impl(new Impl);
    impl->W = W;
    impl->H = H;
    impl->W2 = (W + 1) / 2;
    impl->H2 = (H + 1) / 2;

    const size_t pb = impl->planeBytes();
    const size_t mb = impl->mapBytes();
    if (pb > ctx->caps().max_storage_buffer_range) {
        logOnce("GPU: denoise plane exceeds maxStorageBufferRange; "
                "needs tiling");
        return false;
    }

    impl->p0 = ctx->createBuffer(pb, true);
    impl->p1 = ctx->createBuffer(pb, true);
    impl->p2 = ctx->createBuffer(pb, true);
    impl->noisevarlum = ctx->createBuffer(mb, true);
    impl->noisevarchrom = ctx->createBuffer(mb, true);

    /* All five are genuinely touched from the CPU (up()/down()/
     * syncNoisevarTo{Device,Host}), but that no longer requires mapped():
     * those helpers now stage through a host-visible buffer plus an
     * on-device copy when a discrete GPU hands back an unmapped one. */
    const Buffer *all[] = {&impl->p0, &impl->p1, &impl->p2,
                           &impl->noisevarlum, &impl->noisevarchrom};
    for (const Buffer *bp : all) {
        if (!bp->valid()) {
            logOnce("GPU: denoise buffer allocation failed; using the CPU");
            return false;
        }
    }

    p_ = impl.release();
    return true;
}

bool DenoiseSession::syncToDevice(float **L, float **a, float **b)
{
    if (!p_ || !L || !a || !b) {
        return false;
    }
    return p_->up(p_->labL(), L) && p_->up(p_->labA(), a) &&
           p_->up(p_->labB(), b);
}

bool DenoiseSession::syncToHost(float **L, float **a, float **b)
{
    if (!p_ || !L || !a || !b) {
        return false;
    }
    return p_->down(L, p_->labL()) && p_->down(a, p_->labA()) &&
           p_->down(b, p_->labB());
}

bool DenoiseSession::syncNoisevarToDevice(const float *lum, const float *chrom)
{
    if (!p_ || !lum || !chrom) {
        return false;
    }
    const size_t mb = p_->mapBytes();
    Context *ctx = Context::get();
    if (!ctx) {
        return false;
    }
    BufferPool &staging = ctx->stagingPoolForThisThread();
    return uploadToBuffer(*ctx, &staging, lum, mb, p_->noisevarlum) &&
           uploadToBuffer(*ctx, &staging, chrom, mb, p_->noisevarchrom);
}

bool DenoiseSession::syncNoisevarToHost(float *lum, float *chrom)
{
    if (!p_ || !lum || !chrom) {
        return false;
    }
    const size_t mb = p_->mapBytes();
    Context *ctx = Context::get();
    if (!ctx) {
        return false;
    }
    BufferPool &staging = ctx->stagingPoolForThisThread();
    return downloadFromBuffer(*ctx, &staging, p_->noisevarlum, 0, lum, mb) &&
           downloadFromBuffer(*ctx, &staging, p_->noisevarchrom, 0, chrom, mb);
}


namespace {

struct GammaPC {
    unsigned int w, h;
    float cutoff, gam, thresh, slope;
};
struct Yuv3PC {
    unsigned int w, h;
    float ws1x, ws1y, ws1z;
};
/* Rows first, then the extents: a vec4 in a push-constant block is 16-byte
 * aligned, so uints ahead of them would introduce padding on the shader side
 * that this struct does not have.  See the note in dn_rgb2lab.comp. */
struct Mat3PC {
    float m[3][4]; // vec4 rows, .xyz used
    unsigned int w, h;
};
struct BoostPC {
    unsigned int w, h;
    float boost_a, boost_b;
};

std::vector<Pass::Binding> planeBindings(Buffer &a, Buffer &b, Buffer &c,
                                         bool write)
{
    std::vector<Pass::Binding> v;
    v.push_back(Pass::Binding(&a, write));
    v.push_back(Pass::Binding(&b, write));
    v.push_back(Pass::Binding(&c, write));
    return v;
}

} // namespace

namespace {
Pass::Binding imagePlaneBinding(Buffer &buf, int plane, size_t planeBytes);
}

namespace {
struct DnNoiseVarPrepPC {
    float ws[3][4]; // vec4 rows, .xyz used -- see fillMat3
    unsigned int w, h;
    unsigned int stride;
    unsigned int lutSize;
    float noisevarL;
    float maxNoiseVarab;
    float cn100Precalc;
};
} // namespace

bool DenoiseSession::fillNoiseVarMaps(Imagefloat *src,
                                      const denoise::NoiseCurve &curve,
                                      const float wpi[3][3], float noisevarL,
                                      float maxNoiseVarab, Context *ctx)
{
    if (!p_ || !src || !ctx || !opEnabled("denoise") || !available()) {
        return false;
    }
    const unsigned int lutSize = curve.rawSize();
    const float *lutData = curve.rawData();
    if (!lutData || lutSize < 2) {
        return false;
    }
    if (src->getWidth() != p_->W || src->getHeight() != p_->H) {
        return false;
    }

    Buffer *bSrc = src->residency().forRead();
    if (!bSrc) {
        return false;
    }
    src->residency().invalidateGPU();
    const size_t srcPlaneBytes = src->getPlaneStride();

    const size_t lutBytes = size_t(lutSize) * sizeof(float);
    Buffer bLut = ctx->createBuffer(lutBytes, true);
    if (!bLut.valid()) {
        return false;
    }
    if (!uploadToBuffer(*ctx, &ctx->stagingPoolForThisThread(), lutData,
                        lutBytes, bLut)) {
        return false;
    }

    Pass pass(*ctx, "denoise:noisevar");
    if (!pass.valid()) {
        return false;
    }

    DnNoiseVarPrepPC pc{};
    fillMat3(pc.ws, &wpi[0][0]);
    pc.w = (unsigned int)p_->W2;
    pc.h = (unsigned int)p_->H2;
    pc.stride = (unsigned int)(src->getRowStride() / sizeof(float));
    pc.lutSize = lutSize;
    pc.noisevarL = noisevarL;
    pc.maxNoiseVarab = maxNoiseVarab;
    pc.cn100Precalc = SQR(1.f + 4.f * curve[100.f / 60.f]);

    std::vector<Pass::Binding> b;
    b.push_back(imagePlaneBinding(*bSrc, 0, srcPlaneBytes));
    b.push_back(imagePlaneBinding(*bSrc, 1, srcPlaneBytes));
    b.push_back(imagePlaneBinding(*bSrc, 2, srcPlaneBytes));
    b.push_back(Pass::Binding(&bLut, false));
    b.push_back(Pass::Binding(&p_->noisevarlum, true));
    b.push_back(Pass::Binding(&p_->noisevarchrom, true));
    if (!pass.dispatch2D("dn_noisevar_prep", b, &pc, sizeof(pc), p_->W2,
                         p_->H2)) {
        return false;
    }
    if (!pass.submitAndWait()) {
        return false;
    }
    pass.reportTimings();
    return true;
}

/* Both phases are three or four pointwise dispatches over the same planes, so
 * each is one Pass and one submission.  Reusing wav_rgb2yuv/wav_yuv2rgb costs
 * one extra full-image read+write against a hypothetical single fused kernel;
 * the alternative was a fourth and fifth near-duplicate of transforms that
 * already exist and are already exercised by op_smoothing. */
bool DenoiseSession::fill(float **srcR, float **srcG, float **srcB,
                          const FillParams &fp, Context *ctx)
{
    if (!p_ || !srcR || !srcG || !srcB) {
        return false;
    }
    if (!ctx) {
        return false;
    }

    if (!p_->up(p_->p0, srcR) || !p_->up(p_->p1, srcG) ||
        !p_->up(p_->p2, srcB)) {
        return false;
    }

    Pass pass(*ctx, "denoise:fill");
    if (!pass.valid()) {
        return false;
    }

    /* Lab mode only: the inverse "denoise gamma" that undoes the working
     * gamma the raw pipeline applied, before the denoise gamma goes on. */
    if (fp.lab_mode && !p_->gammaDispatch(pass, 3u, 0.f, 0.f, 0.f)) {
        return false; // DN_GAMMA_ID55
    }
    if (fp.gam > 1.f && !p_->gammaDispatch(pass, 0u, fp.gam, fp.gamthresh,
                                           fp.gamslope)) {
        return false; // DN_GAMMA_FWD
    }

    if (fp.lab_mode) {
        Mat3PC pc{};
        fillMat3(pc.m, fp.wp);
        pc.w = (unsigned)p_->W;
        pc.h = (unsigned)p_->H;
        if (!pass.dispatch2D("dn_rgb2lab",
                             planeBindings(p_->p0, p_->p1, p_->p2, true), &pc,
                             sizeof(pc), p_->W, p_->H)) {
            return false;
        }
    } else {
        Yuv3PC pc{};
        pc.w = (unsigned)p_->W;
        pc.h = (unsigned)p_->H;
        pc.ws1x = fp.wp[3];
        pc.ws1y = fp.wp[4];
        pc.ws1z = fp.wp[5];
        if (!pass.dispatch2D("wav_rgb2yuv",
                             planeBindings(p_->p0, p_->p1, p_->p2, true), &pc,
                             sizeof(pc), p_->W, p_->H)) {
            return false;
        }
    }

    if (!pass.submitAndWait()) {
        return false;
    }
    pass.reportTimings();
    return true;
}

/* Writes the denoised RGB into `dst`'s residency buffer rather than reading it
 * back plane by plane.
 *
 * The old shape ended in three Impl::down() calls.  p0/p1/p2 are pooled
 * buffers and on a discrete GPU they are usually not host-visible, so each
 * down() took the staged path: a device->host copy, a fresh zero-initialised
 * std::vector of a whole plane, and two more memcpys.  On an RTX 4500 Ada the
 * `denoise:out` scope cost 1479 ms around a 7.2 ms kernel, and the trace shows
 * why the readback was pointless as well as slow -- the very next step
 * (finalSmoothing) called forWrite() and uploaded all 507 MB straight back.
 *
 * Packing into the residency buffer on-device removes both halves of that
 * round trip and leaves the image GPU-resident for whatever runs next. */
bool DenoiseSession::output(Imagefloat *dst, const FillParams &fp, Context *ctx)
{
    if (!p_ || !dst || !ctx) {
        return false;
    }
    if (dst->getWidth() != p_->W || dst->getHeight() != p_->H) {
        return false;   // caller falls back to the CPU output path
    }

    /* forDiscardWrite, not forWrite: every pixel of all three planes is
     * overwritten below, so uploading the pre-denoise contents first would be
     * pure cost.  transferPlane writes only the W valid pixels of each row and
     * leaves the row padding alone, which is never read as pixel data. */
    Buffer *resid = dst->residency().forDiscardWrite();
    if (!resid) {
        return false;
    }
    ResidencyGuard guard(dst);
    const size_t planeStride = dst->getPlaneStride();
    const int strideFloats = int(dst->getRowStride() / sizeof(float));

    Pass pass(*ctx, "denoise:out");
    if (!pass.valid()) {
        return false;
    }

    {
        BoostPC pc{(unsigned)p_->W, (unsigned)p_->H, fp.boost_a, fp.boost_b};
        std::vector<Pass::Binding> bd;
        bd.push_back(Pass::Binding(&p_->labA(), true));
        bd.push_back(Pass::Binding(&p_->labB(), true));
        if (!pass.dispatch2D("dn_chroma_boost", bd, &pc, sizeof(pc), p_->W,
                             p_->H)) {
            return false;
        }
    }

    if (fp.lab_mode) {
        Mat3PC pc{};
        fillMat3(pc.m, fp.iwp);
        pc.w = (unsigned)p_->W;
        pc.h = (unsigned)p_->H;
        if (!pass.dispatch2D("dn_lab2rgb",
                             planeBindings(p_->p0, p_->p1, p_->p2, true), &pc,
                             sizeof(pc), p_->W, p_->H)) {
            return false;
        }
    } else {
        /* yuv2rgb uses the forward matrix's row 1, not the inverse: the CPU
         * calls Color::yuv2rgb(..., wpi), which solves for green from the
         * luminance relation rather than inverting the matrix. */
        Yuv3PC pc{};
        pc.w = (unsigned)p_->W;
        pc.h = (unsigned)p_->H;
        pc.ws1x = fp.wp[3];
        pc.ws1y = fp.wp[4];
        pc.ws1z = fp.wp[5];
        if (!pass.dispatch2D("wav_yuv2rgb",
                             planeBindings(p_->p0, p_->p1, p_->p2, true), &pc,
                             sizeof(pc), p_->W, p_->H)) {
            return false;
        }
    }

    if (fp.gam > 1.f && !p_->gammaDispatch(pass, 1u, 1.f / fp.gam,
                                           fp.igamthresh, fp.igamslope)) {
        return false; // DN_GAMMA_INV
    }
    /* Lab mode only, mirroring fill()'s igamma55 step. */
    if (fp.lab_mode && !p_->gammaDispatch(pass, 2u, 0.f, 0.f, 0.f)) {
        return false; // DN_GAMMA_D55
    }

    /* Packed W x H planes -> the row-padded residency buffer, on-device, in
     * the same submission as the kernels above. */
    if (!transferPlane(pass, p_->p0, *resid, 0, planeStride, p_->W, p_->H,
                       strideFloats, true, 1.f) ||
        !transferPlane(pass, p_->p1, *resid, planeStride, planeStride, p_->W,
                       p_->H, strideFloats, true, 1.f) ||
        !transferPlane(pass, p_->p2, *resid, 2 * planeStride, planeStride,
                       p_->W, p_->H, strideFloats, true, 1.f)) {
        return false;
    }

    if (!pass.submitAndWait()) {
        return false;
    }
    pass.reportTimings();

    guard.success();
    return true;
}


namespace {

/* ipdenoise.cc's compile-time block geometry (TS/offset/blkrad,
 * ipdenoise.cc:2550-2553), which detail_recovery bakes in rather than
 * deriving. */
const int kDrTS = 64;
const int kDrOffset = 25;
const int kDrBlkrad = 1;

struct DrBlockPC {
    unsigned int w, h, nbW, nbH;
    float detail_hi, detail_lo, pLdetail, eps;
    unsigned int use_mask, blur_rad, no_shrink;
};
struct DrTotwtPC {
    unsigned int w, h, nbW, nbH;
    float eps;
};
struct DrGatherPC {
    unsigned int w, h, nbW, nbH;
};
struct DrResidPC {
    unsigned int n;
};

/* compute_detail (ipdenoise.cc:3202-3205), including the double intermediate:
 * `SQR(100. - d)` and `50. * (100. - d)` are double expressions, so the sum
 * is formed in double and narrowed only at the cast. */
float drComputeDetail(float d)
{
    const double t = 100. - d;
    const float s = static_cast<float>(t * t + 50. * t) * kDrTS * 0.5f;
    return s * s;
}

} // namespace

size_t dnMemoryBudget(Context *ctx)
{
    const unsigned long long heap = ctx->caps().device_local_bytes;
    const size_t quarter = size_t(heap / 4);
    const size_t floor = size_t(512) << 20;
    return quarter > floor ? quarter : floor;
}

void dnLogOverBudget(const char *phase, size_t want, size_t budget)
{
    std::ostringstream os;
    os << "GPU: denoise " << phase << " needs " << (want >> 20)
       << " MiB but may use " << (budget >> 20)
       << " MiB (a quarter of the device-local heap); using the CPU";
    logOnce(os.str());
}

bool DenoiseSession::waveletCore(const DenoiseWaveletGPU &p, Context *ctx)
{
    if (!p_ || !p.noisevarlum || !p.noisevarchrom || p.levels < 1 ||
        p.levels > 24 || !opEnabled("denoise")) {
        return false;
    }
    if (!ctx || !dwGeometryOk(p_->W, p_->H, p, ctx, nullptr)) {
        return false;
    }

    const size_t planeBytes = p_->planeBytes();
    if (p.denoiseLuminance && !p_->lin.valid()) {
        p_->lin = ctx->createBuffer(planeBytes, true);
        if (!p_->lin.valid()) {
            return false;
        }
    }

    if (!pool) {
        return false;
    }
    // BufferPool *poolp = pools->get(ctx, "denoise", p_->W, p_->H);
    // if (!poolp) {
    //     return false;
    // }
    DnPoolCheckout checkout(*pool);

    PipelineProfile::Timer t_pass("denoise:wav:gpu:pass");
    PassSeq seq(*ctx, "denoise-wavelet");

    /* Snapshot L for phase 7 before the reconstruction overwrites it.  The
     * CPU's own snapshot is taken after the shrink and before the
     * reconstruction, and it is the same plane either way: the shrink works
     * on the decomposition, not on the plane.  Done first here so the
     * ordering is obvious rather than load-bearing. */
    if (p.denoiseLuminance) {
        struct CopyPC { unsigned int n; } cpc{
            (unsigned int)(size_t(p_->W) * p_->H)};
        Pass *ps = seq.reserve(1);
        std::vector<Pass::Binding> bc;
        bc.push_back(Pass::Binding(&p_->labL(), false));
        bc.push_back(Pass::Binding(&p_->lin, true));
        if (!ps || !ps->dispatch1D("dn_plane_copy", bc, &cpc, sizeof(cpc),
                                   size_t(p_->W) * p_->H)) {
            return false;
        }
    }

    const bool ok = waveletCoreImpl(*ctx, *pool, seq, p_->W, p_->H,
                                    p_->labL(), p_->labA(), p_->labB(),
                                    p_->labL(), p_->noisevarlum,
                                    p_->noisevarchrom, p) &&
                    seq.flush();
    t_pass.stop();
    if (!ok) {
        logOnce("GPU: denoise wavelet core failed; using the CPU");
        return false;
    }
    if (settings && settings->verbose > 1) {
        std::cout << "GPU: denoise wavelet core (on-device planes), "
                  << p.levels << " levels, " << seq.submissions()
                  << " submission(s), " << seq.wallMs() << " ms" << std::endl;
    }
    return true;
}

bool DenoiseSession::syncLinToHost(float **Lin)
{
    if (!p_ || !Lin || !p_->lin.valid()) {
        return false;
    }
    return p_->down(Lin, p_->lin);
}

namespace {

/* Phase 7's geometry and its two hard limits.  Shared by both entry points so
 * they cannot disagree about when the phase is available. */
struct DrGeom {
    int nbW_fill, nbW_out, nbH;
    size_t nblocks, planeBytes, blockBytes, cfBytes, profBytes, maskBytes;
};

bool drGeometry(int W, int H, Context *ctx, bool wantMask, DrGeom &g)
{
    if (W < kDrTS || H < kDrTS) {
        return false;
    }
    g.nbW_fill = int(std::ceil(double(W) / kDrOffset)) + 2 * kDrBlkrad;
    g.nbW_out = int(std::ceil(double(W) / kDrOffset));
    g.nbH = int(std::ceil(double(H) / kDrOffset)) + 2 * kDrBlkrad;
    g.nblocks = size_t(g.nbW_out) * size_t(g.nbH);
    g.planeBytes = size_t(W) * size_t(H) * sizeof(float);
    g.blockBytes = g.nblocks * size_t(kDrTS) * kDrTS * sizeof(float);
    g.cfBytes = size_t(kDrTS) * kDrTS * sizeof(float);
    g.profBytes = size_t(kDrTS) * sizeof(float);
    g.maskBytes = wantMask ? g.planeBytes : 4;

    if (g.blockBytes > ctx->caps().max_storage_buffer_range ||
        g.planeBytes > ctx->caps().max_storage_buffer_range) {
        logOnce("GPU: denoise detail recovery exceeds maxStorageBufferRange; "
                "would need banding by block row");
        return false;
    }
    if (g.nblocks > ctx->caps().max_workgroup_count[0]) {
        logOnce("GPU: denoise detail recovery needs more workgroups than the "
                "device allows");
        return false;
    }
    const size_t total = g.blockBytes + 3 * g.planeBytes + g.maskBytes +
                         g.cfBytes + 2 * g.profBytes;
    const size_t budget = dnMemoryBudget(ctx);
    if (total > budget) {
        dnLogOverBudget("detail recovery", total, budget);
        return false;
    }
    return true;
}

/* The four dispatches, over buffers the caller owns.  `bLin` and `bLcur` are
 * the pre-wavelet and denoised L planes; `bLcur` is read and written. */
bool detailRecoveryImpl(Context &ctx, BufferPool &pool, int W, int H,
                        Buffer &bLin, Buffer &bLcur, Buffer &bMask,
                        const DrGeom &g, const DetailRecoveryGPU &params)
{
    Buffer *bResid = pool.get(g.planeBytes);
    Buffer *bTotwt = pool.get(g.planeBytes);
    Buffer *bBlocks = pool.get(g.blockBytes);
    Buffer *bCf = pool.get(g.cfBytes);
    Buffer *bFv = pool.get(g.profBytes);
    Buffer *bGv = pool.get(g.profBytes);
    if (!bResid || !bTotwt || !bBlocks || !bCf || !bFv || !bGv) {
        logOnce("GPU: denoise detail recovery allocation failed; "
                "using the CPU");
        return false;
    }

    /* cf[k][j] = cos(pi*(2j+1)k/128), built in double and rounded once.  One
     * matrix serves all four DCT passes: row-major it is the DCT-II kernel,
     * column-major the DCT-III kernel, since cf[j][k] = cos(pi*(2k+1)j/128).
     *
     * Rebuilt per call rather than cached: 4096 cosines against a phase that
     * costs tens of milliseconds, and a cache would have to be invalidated
     * when the pool is cleared. */
    {
        std::vector<float> cf((size_t)kDrTS * kDrTS);
        for (int k = 0; k < kDrTS; ++k) {
            for (int j = 0; j < kDrTS; ++j) {
                cf[k * kDrTS + j] =
                    float(std::cos(RT_PI * double(2 * j + 1) * double(k) /
                                   double(2 * kDrTS)));
            }
        }
        if (!uploadToBuffer(ctx, &ctx.stagingPoolForThisThread(), cf.data(),
                            g.cfBytes, *bCf)) {
            return false;
        }
    }

    /* The separable halves of tilemask_in / tilemask_out:
     * the CPU's tables are exactly fv[i]*fv[j] + eps and gv[i]*gv[j] + eps, so
     * 64 floats each replace two 16 KB planes.  Built with the same std::sin
     * the CPU path uses, so the masks agree on both sides to the last bit and
     * any divergence downstream belongs to the DCT.  They are two buffers
     * rather than one of 128 on purpose -- see the driver hazard note in
     * shaders/include/dn_dct.glsl. */
    const float epsilon = 0.001f / (kDrTS * kDrTS);
    {
        std::vector<float> fv(kDrTS), gv(kDrTS);
        const int border = std::max(2, kDrTS / 16);
        for (int i = 0; i < kDrTS; ++i) {
            const float i1 = std::abs((i > kDrTS / 2 ? i - kDrTS + 1 : i));
            const float sv = SQR(std::sin((RT_PI * i1) / (2 * border)));
            fv[i] = i1 < border ? sv : 1.0f;
            gv[i] = i1 < 2 * border ? sv : 1.0f;
        }
        BufferPool &staging = ctx.stagingPoolForThisThread();
        if (!uploadToBuffer(ctx, &staging, fv.data(), g.profBytes, *bFv) ||
            !uploadToBuffer(ctx, &staging, gv.data(), g.profBytes, *bGv)) {
            return false;
        }
    }

    DrResidPC rpc;
    rpc.n = (unsigned int)(size_t(W) * H);

    DrTotwtPC tpc;
    tpc.w = (unsigned int)W;
    tpc.h = (unsigned int)H;
    tpc.nbW = (unsigned int)g.nbW_fill;
    tpc.nbH = (unsigned int)g.nbH;
    tpc.eps = epsilon;

    DrBlockPC bpc;
    bpc.w = (unsigned int)W;
    bpc.h = (unsigned int)H;
    bpc.nbW = (unsigned int)g.nbW_out;
    bpc.nbH = (unsigned int)g.nbH;
    bpc.detail_hi = drComputeDetail(params.params_Ldetail);
    bpc.detail_lo = drComputeDetail(0.f);
    bpc.pLdetail = params.params_Ldetail;
    bpc.eps = epsilon;
    bpc.use_mask = params.detail_thresh > 0 ? 1u : 0u;
    bpc.blur_rad = (unsigned int)std::max(1, int(3 / params.scale));
    bpc.no_shrink = 0u;

    DrGatherPC gpc;
    gpc.w = (unsigned int)W;
    gpc.h = (unsigned int)H;
    gpc.nbW = (unsigned int)g.nbW_out;
    gpc.nbH = (unsigned int)g.nbH;

    /* One Pass: four dispatches, one submission, and the barriers between
     * them derived from the bindings.  totwt depends on neither the residual
     * nor the blocks, so it is recorded first and needs no barrier at all. */
    PipelineProfile::Timer t_pass("denoise:dct:pass");
    Pass pass(ctx, "denoise-dct");
    if (!pass.valid()) {
        return false;
    }
    typedef Pass::Binding B;
    const std::vector<B> tot{B(bFv, false), B(bGv, false), B(bTotwt, true)};
    const std::vector<B> res{B(bResid, true), B(&bLin, false),
                             B(&bLcur, false)};
    const std::vector<B> blk{B(bResid, false), B(bCf, false),
                             B(bFv, false),    B(bGv, false),
                             B(&bMask, false), B(bBlocks, true)};
    const std::vector<B> gat{B(bBlocks, false), B(bTotwt, false),
                             B(&bLcur, true)};

    const bool ok =
        pass.dispatch2D("dn_dct_totwt", tot, &tpc, sizeof(tpc), W, H) &&
        pass.dispatch1D("dn_dct_resid", res, &rpc, sizeof(rpc),
                        size_t(W) * H) &&
        pass.dispatch1D("dn_dct_block", blk, &bpc, sizeof(bpc),
                        g.nblocks * 256) &&
        pass.dispatch2D("dn_dct_gather", gat, &gpc, sizeof(gpc), W, H) &&
        pass.submitAndWait();
    t_pass.stop();
    if (!ok) {
        logOnce("GPU: denoise detail recovery dispatch failed; using the CPU");
        return false;
    }
    if (settings && settings->verbose > 1) {
        pass.reportTimings();
    }
    return true;
}

} // namespace

bool DenoiseSession::detailRecovery(const DetailRecoveryGPU &params,
                                    Context *ctx)
{
    if (!p_ || !p_->lin.valid() || !opEnabled("denoise")) {
        return false;
    }
    if (!ctx) {
        return false;
    }
    const bool wantMask = params.detail_thresh > 0;
    DrGeom g;
    if (!drGeometry(p_->W, p_->H, ctx, wantMask, g)) {
        return false;
    }

    if (!pool) {
        return false;
    }
    // BufferPool *poolp = pools->get(ctx, "denoise", p_->W, p_->H);
    // if (!poolp) {
    //     return false;
    // }
    DnPoolCheckout checkout(*pool);

    Buffer *bMask = pool->get(g.maskBytes);
    if (!bMask) {
        return false;
    }

    if (wantMask) {
        PipelineProfile::Timer t_mask("denoise:dct:mask");
        const float amount =
            std::min(1.f, std::max(0.f, float(params.detail_thresh) / 100.f));
        if (!detailMask(*ctx, *bMask, p_->labL(), p_->W, p_->H, 65535.f, 25.f,
                        10000.f, amount, 25.f / float(params.scale), pool)) {
            logOnce("GPU: denoise detail mask failed; using the CPU");
            return false;
        }
        t_mask.stop();
    }

    return detailRecoveryImpl(*ctx, *pool, p_->W, p_->H, p_->lin, p_->labL(),
                              *bMask, g, params);
}

namespace {

bool dnDetailRecoveryStandalone(int W, int H, float **L, float **Lin,
                                const DetailRecoveryGPU &params,
                                BufferPool *poolp, Context *ctx)
{
    if (!L || !Lin || !poolp || !opEnabled("denoise") || !available()) {
        return false;
    }
    if (params.detail_thresh > 0 && !params.mask) {
        return false;
    }
    if (!ctx) {
        return false;
    }

    DrGeom g;
    if (!drGeometry(W, H, ctx, params.detail_thresh > 0, g)) {
        return false;
    }

    // BufferPool *poolp = pools->get(ctx, "denoise", W, H);
    // if (!poolp) {
    //     return false;
    // }
    BufferPool &pool = *poolp;
    DnPoolCheckout checkout(pool);

    PipelineProfile::Timer t_alloc("denoise:dct:alloc");
    Buffer *bLin = pool.get(g.planeBytes);
    Buffer *bLcur = pool.get(g.planeBytes);
    Buffer *bMask = pool.get(g.maskBytes);
    if (!bLin || !bLcur || !bMask) {
        logOnce("GPU: denoise detail recovery allocation failed; "
                "using the CPU");
        return false;
    }
    t_alloc.stop();

    PipelineProfile::Timer t_up("denoise:dct:up");
    {
        BufferPool &staging = ctx->stagingPoolForThisThread();
        if (!dnUploadRows(*ctx, staging, Lin, W, H, *bLin) ||
            !dnUploadRows(*ctx, staging, L, W, H, *bLcur)) {
            return false;
        }
        if (params.detail_thresh > 0 &&
            !dnUploadRows(*ctx, staging, params.mask, W, H, *bMask)) {
            return false;
        }
    }
    t_up.stop();

    if (!detailRecoveryImpl(*ctx, pool, W, H, *bLin, *bLcur, *bMask, g,
                            params)) {
        return false;
    }

    PipelineProfile::Timer t_down("denoise:dct:down");
    if (!dnDownloadRows(*ctx, ctx->stagingPoolForThisThread(), *bLcur, L, W,
                        H)) {
        return false;
    }
    t_down.stop();
    return true;
}

} // namespace

bool denoiseDetailRecovery(int W, int H, float **L, float **Lin,
                           const DetailRecoveryGPU &params,
                           BufferPool *pool, Context *ctx)
{
    return dnDetailRecoveryStandalone(W, H, L, Lin, params, pool, ctx);
}

/* ---------------------------------------------------------------------------
 * Automatic chrominance: RGB_denoise_info.
 * ------------------------------------------------------------------------ */

namespace {

struct DnInfoPrepPC {
    float ws[3][4];
    unsigned int w, h, stride;
};

struct DnInfoAbPC {
    float ws1[4];
    unsigned int w, h, stride;
    float gain, gam, thresh, slope;
};

struct DnInfoStatsPC {
    unsigned int n;
};

/* Per-workgroup partial count in dn_info_stats.comp. */
const unsigned int DN_INFO_ACC = 6;

// One plane of an Imagefloat's device buffer as a binding of its own.
Pass::Binding imagePlaneBinding(Buffer &buf, int plane, size_t planeBytes)
{
    Pass::Binding b(&buf, false);
    b.offset = size_t(plane) * planeBytes;
    b.range = planeBytes;
    return b;
}

/* ShrinkAll_info's accumulation over the median absolute deviations, from the
 * two mad buffers waveletMadExact filled.  */
void dnInfoAccumulateMad(const float *madA, const float *madB, int levels,
                         bool aggressive, DenoiseInfoResult &out)
{
    const float reduc = aggressive ? static_cast<float>(0.9) : 1.f;
    // float chred = 0.f;
    // float chblue = 0.f;
    float chau = 0.f;
    float maxchred = 0.f, maxchblue = 0.f;
    float minchred = 100000000.f, minchblue = 100000000.f;
    int nb = 0;

    for (int lvl = 0; lvl < levels; ++lvl) {
        for (int dir = 0; dir < 3; ++dir) {
            const float mada = SQR(madA[dir * levels + lvl]);
            // chred += mada;
            if (mada > maxchred) {
                maxchred = mada;
            }
            if (mada < minchred) {
                minchred = mada;
            }
            out.maxredaut = std::sqrt(reduc * maxchred);
            out.minredaut = std::sqrt(reduc * minchred);

            const float madb = SQR(madB[dir * levels + lvl]);
            // chblue += madb;
            if (madb > maxchblue) {
                maxchblue = madb;
            }
            if (madb < minchblue) {
                minchblue = madb;
            }
            out.maxblueaut = std::sqrt(reduc * maxchblue);
            out.minblueaut = std::sqrt(reduc * minchblue);

            chau += (mada + madb);
            ++nb;
            out.chaut = std::sqrt(reduc * chau / (nb + nb));
            out.Nb = nb;
        }
    }
}

// Peak pool residency of one crop's analysis, for the memory budget.
size_t dnInfoBytes(int W, int H, int levels)
{
    const int W2 = (W + 1) / 2, H2 = (H + 1) / 2;
    const size_t planeBytes = size_t(W) * size_t(H) * sizeof(float);
    const size_t lvlBytes = size_t(W2) * size_t(H2) * sizeof(float);
    const size_t bandBytes = lvlBytes * size_t(levels);
    /* Per waveletDecompose call: two W x H2 planes plus four level planes. */
    const size_t scratchBytes = 2 * size_t(W) * size_t(H2) * sizeof(float) +
                                4 * lvlBytes;
    const size_t histBytes = size_t(levels) * 3u * 65536u * sizeof(unsigned);
    return 2 * (3 * bandBytes + lvlBytes + scratchBytes + histBytes) +
           2 * planeBytes + 3 * lvlBytes;
}

bool dnInfoGeometryOk(int W, int H, int levels, Context *ctx)
{
    if (W <= 0 || H <= 0 || levels < 1 || levels > 24) {
        return false;
    }
    const size_t planeBytes = size_t(W) * size_t(H) * sizeof(float);
    const int W2 = (W + 1) / 2, H2 = (H + 1) / 2;
    const size_t bandBytes =
        size_t(W2) * size_t(H2) * sizeof(float) * size_t(levels);
    if (planeBytes > ctx->caps().max_storage_buffer_range ||
        bandBytes > ctx->caps().max_storage_buffer_range) {
        logOnce("GPU: denoise info planes exceed maxStorageBufferRange; "
                "needs tiling");
        return false;
    }
    const size_t want = dnInfoBytes(W, H, levels);
    const size_t budget = dnMemoryBudget(ctx);
    if (want > budget) {
        dnLogOverBudget("denoise auto-chrominance analysis", want, budget);
        return false;
    }
    return true;
}

} // namespace

bool denoiseInfoUsable(int W, int H, int levels, bool isRAW, Context *ctx)
{
    /* Two names, because the analysis and RGB_denoise are separate decisions:
     * ART_GPU_DISABLE_OPS=denoise switches off all of denoise's GPU work, and
     * =denoiseParams switches off only this, which is what isolates the
     * analysis's contribution to a rendered diff without also changing the
     * loop the crops run in. */
    if (!isRAW || !opEnabled("denoise") || !opEnabled("denoiseParams") ||
        !available()) {
        return false;
    }
    return ctx && dnInfoGeometryOk(W, H, levels, ctx);
}

bool denoiseInfo(Imagefloat *src, Imagefloat *provicalc,
                 const DenoiseInfoGPU &p, DenoiseInfoResult &out,
                 BufferPool *poolp, Context *ctx)
{
    if (!src || !provicalc || !poolp || !opEnabled("denoise") ||
        !opEnabled("denoiseParams") || !available()) {
        return false;
    }
    if (!ctx) {
        return false;
    }
    const int W = src->getWidth(), H = src->getHeight();
    const int W2 = (W + 1) / 2, H2 = (H + 1) / 2;
    if (!dnInfoGeometryOk(W, H, p.levels, ctx)) {
        return false;
    }
    /* The noisevar maps are indexed by the CPU as noisevar[i >> 1][j >> 1] out
     * of provicalc's own pixels, which is an identity map only when provicalc
     * is exactly the half-resolution image.  denoiseComputeParams allocates it
     * that way; check rather than assume, because getting it wrong would read
     * out of bounds rather than produce a visible artefact. */
    if (provicalc->getWidth() != W2 || provicalc->getHeight() != H2) {
        logOnce("GPU: denoise info got an unexpected provicalc size; "
                "using the CPU");
        return false;
    }

    const size_t planeBytes = size_t(W) * size_t(H) * sizeof(float);
    const size_t lvlBytes = size_t(W2) * size_t(H2) * sizeof(float);
    const size_t n = size_t(W2) * size_t(H2);
    const size_t groups = (n + 255) / 256;

    // BufferPool *poolp = pools->get(ctx, "denoiseinfo", W, H);
    // if (!poolp) {
    //     return false;
    // }
    BufferPool &pool = *poolp;
    DnPoolCheckout checkout(pool);

    PipelineProfile::Timer t_up("denoise:info:up");
    Buffer *bSrc = src->residency().forRead();
    Buffer *bPro = provicalc->residency().forRead();
    if (!bSrc || !bPro) {
        return false;
    }
    t_up.stop();
    const size_t srcPlaneBytes = src->getPlaneStride();
    const size_t proPlaneBytes = provicalc->getPlaneStride();

    Buffer *bNvLum = pool.get(lvlBytes);
    Buffer *bNvChrom = pool.get(lvlBytes);
    Buffer *bNvHue = pool.get(lvlBytes);
    Buffer *bA = pool.get(planeBytes);
    Buffer *bB = pool.get(planeBytes);
    Buffer *bPart = pool.get(groups * DN_INFO_ACC * sizeof(float));
    Buffer *bMadA = pool.get(size_t(p.levels) * 3u * sizeof(float));
    Buffer *bMadB = pool.get(size_t(p.levels) * 3u * sizeof(float));
    if (!bNvLum || !bNvChrom || !bNvHue || !bA || !bB || !bPart || !bMadA ||
        !bMadB) {
        logOnce("GPU: denoise info allocation failed; using the CPU");
        return false;
    }

    Pass pass(*ctx, "denoise-info");
    if (!pass.valid()) {
        return false;
    }

    {
        DnInfoPrepPC pc;
        fillMat3(pc.ws, &p.ws[0][0]);
        pc.w = (unsigned int)W2;
        pc.h = (unsigned int)H2;
        pc.stride = (unsigned int)(provicalc->getRowStride() / sizeof(float));
        std::vector<Pass::Binding> b;
        b.push_back(imagePlaneBinding(*bPro, 0, proPlaneBytes));
        b.push_back(imagePlaneBinding(*bPro, 1, proPlaneBytes));
        b.push_back(imagePlaneBinding(*bPro, 2, proPlaneBytes));
        b.push_back(Pass::Binding(bNvLum, true));
        b.push_back(Pass::Binding(bNvChrom, true));
        b.push_back(Pass::Binding(bNvHue, true));
        if (!pass.dispatch2D("dn_info_prep", b, &pc, sizeof(pc), W2, H2)) {
            return false;
        }
    }

    {
        DnInfoAbPC pc;
        pc.ws1[0] = p.ws[1][0];
        pc.ws1[1] = p.ws[1][1];
        pc.ws1[2] = p.ws[1][2];
        pc.ws1[3] = 0.f;
        pc.w = (unsigned int)W;
        pc.h = (unsigned int)H;
        pc.stride = (unsigned int)(src->getRowStride() / sizeof(float));
        pc.gain = p.gain;
        pc.gam = p.gam;
        pc.thresh = p.gamthresh;
        pc.slope = p.gamslope;
        std::vector<Pass::Binding> b;
        b.push_back(imagePlaneBinding(*bSrc, 0, srcPlaneBytes));
        b.push_back(imagePlaneBinding(*bSrc, 1, srcPlaneBytes));
        b.push_back(imagePlaneBinding(*bSrc, 2, srcPlaneBytes));
        b.push_back(Pass::Binding(bA, true));
        b.push_back(Pass::Binding(bB, true));
        if (!pass.dispatch2D("dn_info_ab", b, &pc, sizeof(pc), W, H)) {
            return false;
        }
    }

    {
        DnInfoStatsPC pc;
        pc.n = (unsigned int)n;
        std::vector<Pass::Binding> b;
        b.push_back(Pass::Binding(bNvLum, false));
        b.push_back(Pass::Binding(bNvChrom, false));
        b.push_back(Pass::Binding(bNvHue, false));
        b.push_back(Pass::Binding(bPart, true));
        if (!pass.dispatch1D("dn_info_stats", b, &pc, sizeof(pc), n)) {
            return false;
        }
    }

    for (int ch = 0; ch < 2; ++ch) {
        WaveletBandsGPU bands;
        Buffer *ll = nullptr;
        int llW = 0, llH = 0;
        if (!waveletDecompose(*ctx, pass, pool, ch == 0 ? *bA : *bB, W, H,
                              p.levels, bands, ll, llW, llH) ||
            !waveletMadExact(*ctx, pass, pool, bands,
                             ch == 0 ? *bMadA : *bMadB)) {
            logOnce("GPU: denoise info decompose/mad failed; using the CPU");
            return false;
        }
    }

    PipelineProfile::Timer t_pass("denoise:info:pass");
    if (!pass.submitAndWait()) {
        return false;
    }
    t_pass.stop();
    if (settings && settings->verbose > 1) {
        pass.reportTimings();
    }

    const size_t madBytes = size_t(p.levels) * 3u * sizeof(float);
    const size_t partBytes = groups * DN_INFO_ACC * sizeof(float);
    std::vector<float> madA((size_t)p.levels * 3u), madB((size_t)p.levels * 3u);
    std::vector<float> part(groups * DN_INFO_ACC);
    BufferPool &staging = ctx->stagingPoolForThisThread();
    if (!downloadFromBuffer(*ctx, &staging, *bMadA, 0, madA.data(), madBytes) ||
        !downloadFromBuffer(*ctx, &staging, *bMadB, 0, madB.data(), madBytes) ||
        !downloadFromBuffer(*ctx, &staging, *bPart, 0, part.data(),
                            partBytes)) {
        return false;
    }

    out.chaut = 0.f;
    out.maxredaut = out.maxblueaut = out.minredaut = out.minblueaut = 0.f;
    out.chromina = out.lumema = out.redyel = out.skinc = out.nsknc = 0.f;
    out.Nb = 0;

    dnInfoAccumulateMad(madA.data(), madB.data(), p.levels, p.aggressive, out);

    /* Finish the statistics reduction.  In double, which the CPU's sequential
     * float accumulation is not -- see dn_info_stats.comp. */
    double chro = 0.0, lume = 0.0, red_yel = 0.0, skin_c = 0.0;
    double nry = 0.0, nsk = 0.0;
    for (size_t g = 0; g < groups; ++g) {
        const float *q = part.data() + g * DN_INFO_ACC;
        chro += q[0];
        lume += q[1];
        red_yel += q[2];
        nry += q[3];
        skin_c += q[4];
        nsk += q[5];
    }
    /* nc and nL are both exactly n: the CPU counts every element of the maps
     * in both accumulators, so its `nc > 0` and `nL > 0` guards are always
     * taken here.  The two conditional ones are not. */
    out.chromina = float(chro / double(n));
    out.lumema = float(lume / double(n));
    out.nsknc = float(nsk / double(n));
    if (nry > 0.0) {
        out.redyel = float(red_yel / nry);
    }
    if (nsk > 0.0) {
        out.skinc = float(skin_c / nsk);
    }

    if (settings && settings->verbose > 1) {
        std::cout << "GPU: denoise auto-chrominance analysis " << W << "x" << H
                  << ", " << p.levels << " levels, " << pass.wallMs() << " ms"
                  << std::endl;
    }
    return true;
}


bool finalSmoothingGPU(ImProcData &im, Imagefloat *rgb, 
                       const procparams::DenoiseParams &dnparams)
{
    if (!im.ipf) {
        return false;
    }

    auto ctx = im.ipf->getGPUContext();
    auto poolp = im.ipf->getGPUPool();

    if (!available() || !ctx || !poolp) {
        return false;
    }
    
    const int W = rgb->getWidth();
    const int H = rgb->getHeight();

    TMatrix ws = ICCStore::getInstance()->workingSpaceMatrix(
        im.params->icm.workingProfile);
    // TMatrix iws = ICCStore::getInstance()->workingSpaceInverseMatrix(
    //     im.params->icm.workingProfile);

    const int radius =
        std::max(int(std::lround(dnparams.guidedChromaRadius / im.scale)), 0);

    if (radius == 0 && !dnparams.nlStrength) {
        return true;
    }

    const size_t planeBytes = (size_t)W * H * sizeof(float);
    const size_t planeStride = rgb->getPlaneStride();
    const int strideFloats = int(rgb->getRowStride() / sizeof(float));

    Buffer *resid = rgb->residency().forWrite();
    if (!resid) {
        // no device, allocation failed, or image too large
        return false;
    }

    ResidencyGuard rguard(rgb);
    auto &pool = *poolp;
    PoolScope scope(*poolp);

    Buffer *wRp = pool.get(planeBytes);
    Buffer *wGp = pool.get(planeBytes);
    Buffer *wBp = pool.get(planeBytes);
    Buffer *guideP = pool.get(planeBytes);
    Buffer *lumaP = pool.get(planeBytes);
    if (!wRp || !wGp || !wBp || !guideP || !lumaP) {
        logOnce("GPU: denoise::finalSmoothing buffer allocation failed; using the CPU");
        return false;
    }
    Buffer &wR = *wRp, &wG = *wGp, &wB = *wBp;
    Buffer &guide = *guideP;
    Buffer &luma = *lumaP;

    {
        Pass pass(*ctx, "denoise::finalSmoothing:extract");
        if (!pass.valid()) {
            return false;
        }
        if (!transferPlane(pass, wR, *resid, 0, planeStride, W, H,
                           strideFloats, false, 1.f/65535.f) ||
            !transferPlane(pass, wG, *resid, planeStride, planeStride, W, H,
                           strideFloats, false, 1.f/65535.f) ||
            !transferPlane(pass, wB, *resid, 2 * planeStride, planeStride, W,
                           H, strideFloats, false, 1.f/65535.f)) {
            return false;
        }
        if (!pass.submitAndWait()) {
            return false;
        }
        pass.reportTimings();
    }

    if (!rgbLuminance(*ctx, wR, wG, wB, W, H, ws, luma) ||
        !logTransform(*ctx, "buildGuideLuma", luma, W, H, false, 10.f, guide)) {
        return false;
    }
    
    if (radius > 0) {
        constexpr float epsilon = 0.001f;

        if (!logGuidedFilterWithGuide(*ctx, "guidedSmoothing", guide, wR, W, H,
                                      radius, epsilon) ||
            !logGuidedFilterWithGuide(*ctx, "guidedSmoothing", guide, wG, W, H,
                                      radius, epsilon) ||
            !logGuidedFilterWithGuide(*ctx, "guidedSmoothing", guide, wB, W, H,
                                      radius, epsilon)) {
            return false;
        }
    }

    if (dnparams.nlStrength > 0 && 
        !NLMeans(*ctx, pool, luma, W, H, 1.f, im.scale,
                 dnparams.nlStrength, dnparams.nlDetail)) {
        return false;
    }

    if (!yuvRecombine(*ctx, luma, wR, wG, wB, W, H, true, ws, wR, wG, wB)) {
        return false;
    }
    
    {
        Pass pass(*ctx, "denoise::finalSmoothing:store");
        if (!pass.valid()) {
            return false;
        }
        if (!transferPlane(pass, wR, *resid, 0, planeStride, W, H,
                           strideFloats, true, 65535.f) ||
            !transferPlane(pass, wG, *resid, planeStride, planeStride, W, H,
                           strideFloats, true, 65535.f) ||
            !transferPlane(pass, wB, *resid, 2 * planeStride, planeStride, W,
                           H, strideFloats, true, 65535.f)) {
            return false;
        }
        if (!pass.submitAndWait()) {
            return false;
        }
        pass.reportTimings();
    }
        
    rguard.success();
    return true;
}


}}} // namespace rtengine::gpu::ops

#else // !ART_USE_VULKAN

namespace rtengine { namespace gpu { namespace ops {

bool denoiseWaveletGPU(int, int, float **, float **, float **,
                       const DenoiseWaveletGPU &, BufferPool *, Context *)
{
    return false;
}

struct DenoiseSession::Impl {};

DenoiseSession::DenoiseSession(): p_(nullptr) {}
DenoiseSession::~DenoiseSession() {}
bool DenoiseSession::valid() const { return false; }
bool DenoiseSession::init(int, int, Context *) { return false; }
bool DenoiseSession::syncToDevice(float **, float **, float **) { return false; }
bool DenoiseSession::syncToHost(float **, float **, float **) { return false; }
bool DenoiseSession::syncNoisevarToDevice(const float *, const float *)
{
    return false;
}
bool DenoiseSession::syncNoisevarToHost(float *, float *) { return false; }
bool DenoiseSession::fillNoiseVarMaps(Imagefloat *, const denoise::NoiseCurve &,
                                      const float[3][3], float, float,
                                      Context *)
{
    return false;
}
bool DenoiseSession::fill(float **, float **, float **, const FillParams &,
                          Context *)
{
    return false;
}
bool DenoiseSession::output(Imagefloat *, const FillParams &, Context *)
{
    return false;
}
bool denoiseDetailRecovery(int, int, float **, float **,
                           const DetailRecoveryGPU &, BufferPool *,
                           Context *)
{
    return false;
}
bool DenoiseSession::waveletCore(const DenoiseWaveletGPU &, Context *)
{
    return false;
}
bool DenoiseSession::detailRecovery(const DetailRecoveryGPU &, Context *)
{
    return false;
}
bool DenoiseSession::syncLinToHost(float **) { return false; }

bool denoiseInfoUsable(int, int, int, bool, Context *) { return false; }

bool denoiseInfo(Imagefloat *, Imagefloat *, const DenoiseInfoGPU &,
                 DenoiseInfoResult &, BufferPool *, Context *)
{
    return false;
}

bool finalSmoothingGPU(ImProcData &im, Imagefloat *src, 
                       const procparams::DenoiseParams &dnparams)
{
    return false;
}

}}} // namespace rtengine::gpu::ops

#endif // ART_USE_VULKAN
