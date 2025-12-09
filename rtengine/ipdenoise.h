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
// extracted and datapted from ImProcFunctions (improcfun.cc, FTblockDN.cc) of
// RawTherapee

#pragma once

#include "curves.h"
#include "improcfun.h"

namespace rtengine {
namespace denoise {

class NoiseCurve {
private:
    LUTf lutNoiseCurve; // 0xffff range
    float sum;
    void Set(const Curve &pCurve);

public:
    virtual ~NoiseCurve() {};
    NoiseCurve();
    void Reset();
    void Set(const std::vector<double> &curvePoints);

    float getSum() const { return sum; }
    float operator[](float index) const { return lutNoiseCurve[index]; }
    operator bool(void) const { return lutNoiseCurve; }
};

void Tile_calc(int tilesize, int overlap, int kall, int imwidth, int imheight,
               int &numtiles_W, int &numtiles_H, int &tilewidth,
               int &tileheight, int &tileWskip, int &tileHskip);

void denoiseGuidedSmoothing(ImProcData &im, Imagefloat *rgb);

void RGB_denoise(ImProcData &im, int kall, Imagefloat *src, Imagefloat *dst,
                 Imagefloat *calclum, float *ch_M, float *max_r, float *max_b,
                 bool isRAW, const procparams::DenoiseParams &dnparams,
                 const double expcomp, const NoiseCurve &noiseLCurve,
                 const NoiseCurve &noiseCCurve, float &nresi, float &highresi);

enum class Median {
    TYPE_3X3_SOFT,
    TYPE_3X3_STRONG,
    TYPE_5X5_SOFT,
    TYPE_5X5_STRONG,
    TYPE_7X7,
    TYPE_9X9
};

void Median_Denoise(float **src, float **dst, float upperBound, int width,
                    int height, Median medianType, int iterations,
                    int numThreads, float **buffer = nullptr);

void Median_Denoise(float **src, float **dst, int width, int height,
                    Median medianType, int iterations, int numThreads,
                    float **buffer = nullptr);

void WaveletDenoiseAll_info(
    int levwav, wavelet_decomposition &WaveletCoeffs_a,
    wavelet_decomposition &WaveletCoeffs_b, float **noisevarlum,
    float **noisevarchrom, float **noisevarhue, float &chaut, int &Nb,
    float &redaut, float &blueaut, float &maxredaut, float &maxblueaut,
    float &minredaut, float &minblueaut, int schoice, float &chromina,
    float &sigma, float &lumema, float &sigma_L, float &redyel, float &skinc,
    float &nsknc, float &maxchred, float &maxchblue, float &minchred,
    float &minchblue, int &nb, float &chau, float &chred, float &chblue);

enum class BlurType { OFF, BOX, GAUSS };
void detail_mask(const array2D<float> &src, array2D<float> &mask, float scaling,
                 float threshold, float ceiling, float factor, BlurType blur,
                 float blur_radius, bool multithread);

void NLMeans(array2D<float> &img, float normcoeff, int strength,
             int detail_thresh, float scale, bool multithread);

} // namespace denoise
} // namespace rtengine
