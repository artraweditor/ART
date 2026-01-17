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

#include "ipdenoise.h"
#include "imagesource.h"
#include "improcfun.h"
#include "mytime.h"
#include "rt_algo.h"
#ifdef _OPENMP
#include <omp.h>
#endif

namespace rtengine {

extern const Settings *settings;
using namespace procparams;

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

    int tilesize;
    int overlap;

    // if (settings->leveldnti == 0) {
    tilesize = 1024;
    overlap = 128;
    // }

    // if (settings->leveldnti == 1) {
    //     tilesize = 768;
    //     overlap = 96;
    // }

    int numtiles_W, numtiles_H, tilewidth, tileheight, tileWskip, tileHskip;

    // always no Tiles
    int kall = 0;
    rtengine::denoise::Tile_calc(tilesize, overlap, kall, imwidth, imheight,
                                 numtiles_W, numtiles_H, tilewidth, tileheight,
                                 tileWskip, tileHskip);

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

    for (int tiletop = 0; tiletop < imheight; tiletop += tileHskip) {
        for (int tileleft = 0; tileleft < imwidth; tileleft += tileWskip) {

            int tileright = MIN(imwidth, tileleft + tilewidth);
            int tilebottom = MIN(imheight, tiletop + tileheight);
            int width = tileright - tileleft;
            int height = tilebottom - tiletop;
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

    float autoNR = 10;    // settings->nrauto;
    float autoNRmax = 40; // settings->nrautomax;

    int tilesize;
    int overlap;

    // if (settings->leveldnti == 0) {
    tilesize = 1024;
    overlap = 128;
    // }

    // if (settings->leveldnti == 1) {
    //     tilesize = 768;
    //     overlap = 96;
    // }

    int numtiles_W, numtiles_H, tilewidth, tileheight, tileWskip, tileHskip;
    int kall = 2;

    int widIm, heiIm;
    int tr = getCoarseBitMask(params->coarse);
    imgsrc->getFullSize(widIm, heiIm, tr);

    denoise::Tile_calc(tilesize, overlap, kall, widIm, heiIm, numtiles_W,
                       numtiles_H, tilewidth, tileheight, tileWskip, tileHskip);
    kall = 0;

    float min_b[9];
    float min_r[9];
    float lumL[9];
    float chromC[9];
    float ry[9];
    float sk[9];
    float pcsk[9];
    std::vector<int> centerTile_X(numtiles_W);
    std::vector<int> centerTile_Y(numtiles_H);

    for (int cX = 0; cX < numtiles_W; cX++) {
        centerTile_X[cX] = tileWskip / 2 + tileWskip * cX;
    }

    for (int cY = 0; cY < numtiles_H; cY++) {
        centerTile_Y[cY] = tileHskip / 2 + tileHskip * cY;
    }

    if (!store.valid &&
        dnparams.chrominanceMethod ==
            procparams::DenoiseParams::ChrominanceMethod::AUTOMATIC) {
        MyTime t1aue, t2aue;
        t1aue.set();

        store.reset(); // = DenoiseInfoStore();

        int crW = 100; // settings->leveldnv == 0
        int crH = 100; // settings->leveldnv == 0

        // if (settings->leveldnv == 1) {
        //     crW = 250;
        //     crH = 250;
        // }

        // //  if(settings->leveldnv ==2)
        // {crW=int(tileWskip/2);crH=int((tileWskip/2));}//adapted to scale of
        // preview if (settings->leveldnv == 2) {
        crW = int(tileWskip / 2);
        crH = int(tileHskip / 2);
        // }

        // if (settings->leveldnv == 3) {
        //     crW = tileWskip - 10;
        //     crH = tileHskip - 10;
        // }

        float lowdenoise = 1.f;
        int levaut = 0;

        // if (levaut == 1) { //Standard
        //     lowdenoise = 0.7f;
        // }

        LUTf gamcurve(65536, 0);
        float gam, gamthresh, gamslope;
        RGB_denoise_infoGamCurve(dnparams, imgsrc->isRAW(), gamcurve, gam,
                                 gamthresh, gamslope);
        int Nb[9];

#ifdef _OPENMP
#pragma omp parallel if (multiThread)
#endif
        {
            Imagefloat *origCropPart =
                new Imagefloat(crW, crH); // allocate memory
            Imagefloat *provicalc = new Imagefloat(
                (crW + 1) / 2, (crH + 1) / 2); // for denoise curves

            int coordW[3]; // coordinate of part of image to measure noise
            int coordH[3];
            int begW = 50;
            int begH = 50;
            coordW[0] = begW;
            coordW[1] = widIm / 2 - crW / 2;
            coordW[2] = widIm - crW - begW;
            coordH[0] = begH;
            coordH[1] = heiIm / 2 - crH / 2;
            coordH[2] = heiIm - crH - begH;

#ifdef _OPENMP
#pragma omp for schedule(dynamic) collapse(2) nowait
#endif

            for (int wcr = 0; wcr <= 2; wcr++) {
                for (int hcr = 0; hcr <= 2; hcr++) {
                    PreviewProps ppP(coordW[wcr], coordH[hcr], crW, crH, 1);
                    imgsrc->getImage(currWB, tr, origCropPart, ppP,
                                     params->exposure, params->raw);

                    // we only need image reduced to 1/4 here
                    for (int ii = 0; ii < crH; ii += 2) {
                        for (int jj = 0; jj < crW; jj += 2) {
                            provicalc->r(ii >> 1, jj >> 1) =
                                origCropPart->r(ii, jj);
                            provicalc->g(ii >> 1, jj >> 1) =
                                origCropPart->g(ii, jj);
                            provicalc->b(ii >> 1, jj >> 1) =
                                origCropPart->b(ii, jj);
                        }
                    }

                    imgsrc->convertColorSpace(
                        provicalc, params->icm,
                        currWB); // for denoise luminance curve

                    float pondcorrec = 1.0f;
                    float chaut = 0.f, redaut = 0.f, blueaut = 0.f,
                          maxredaut = 0.f, maxblueaut = 0.f, minredaut = 0.f,
                          minblueaut = 0.f, chromina = 0.f, sigma = 0.f,
                          lumema = 0.f, sigma_L = 0.f, redyel = 0.f,
                          skinc = 0.f, nsknc = 0.f;
                    int nb = 0;
                    ImProcData im(params, 1.f /*scale*/, multiThread);
                    RGB_denoise_info(
                        im, origCropPart, provicalc, imgsrc->isRAW(), gamcurve,
                        gam, gamthresh, gamslope, dnparams,
                        std::log(5.f) /
                            std::log(2.f) /*imgsrc->getDirPyrDenoiseExpComp()*/,
                        chaut, nb, redaut, blueaut, maxredaut, maxblueaut,
                        minredaut, minblueaut, chromina, sigma, lumema, sigma_L,
                        redyel, skinc, nsknc);

                    // printf("DCROP skip=%d cha=%f red=%f bl=%f redM=%f bluM=%f
                    // chrom=%f sigm=%f lum=%f\n",skip, chaut,redaut,blueaut,
                    // maxredaut, maxblueaut, chromina, sigma, lumema);
                    Nb[hcr * 3 + wcr] = nb;
                    store.ch_M[hcr * 3 + wcr] = pondcorrec * chaut;
                    store.max_r[hcr * 3 + wcr] = pondcorrec * maxredaut;
                    store.max_b[hcr * 3 + wcr] = pondcorrec * maxblueaut;
                    min_r[hcr * 3 + wcr] = pondcorrec * minredaut;
                    min_b[hcr * 3 + wcr] = pondcorrec * minblueaut;
                    lumL[hcr * 3 + wcr] = lumema;
                    chromC[hcr * 3 + wcr] = chromina;
                    ry[hcr * 3 + wcr] = redyel;
                    sk[hcr * 3 + wcr] = skinc;
                    pcsk[hcr * 3 + wcr] = nsknc;
                }
            }

            delete provicalc;
            delete origCropPart;
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

        if (!imgsrc->isRAW()) {
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

        for (int k = 0; k < 9; k++) {
            float maxmax = max(store.max_r[k], store.max_b[k]);
            calcautodn_info(params, store.ch_M[k], delta[k], Nb[k], levaut,
                            maxmax, lumL[k], chromC[k], mode, lissage, ry[k],
                            sk[k], pcsk[k]);
            //  printf("ch_M=%f delta=%f\n",ch_M[k], delta[k]);
        }

        for (int k = 0; k < 9; k++) {
            if (store.max_r[k] > store.max_b[k]) {
                Max_R[k] = (delta[k]) /
                           ((autoNRmax * multip * adjustr * lowdenoise) / 2.f);
                Min_B[k] = -(store.ch_M[k] - min_b[k]) /
                           (autoNRmax * multip * adjustr * lowdenoise);
                Max_B[k] = 0.f;
                Min_R[k] = 0.f;
            } else {
                Max_B[k] = (delta[k]) /
                           ((autoNRmax * multip * adjustr * lowdenoise) / 2.f);
                Min_R[k] = -(store.ch_M[k] - min_r[k]) /
                           (autoNRmax * multip * adjustr * lowdenoise);
                Min_B[k] = 0.f;
                Max_R[k] = 0.f;
            }
        }

        for (int k = 0; k < 9; k++) {
            //  printf("ch_M= %f Max_R=%f Max_B=%f min_r=%f
            //  min_b=%f\n",ch_M[k],Max_R[k], Max_B[k],Min_R[k], Min_B[k]);
            chM += store.ch_M[k];
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
        store.chrominance = chM / (autoNR * multip * adjustr);
        store.chrominanceRedGreen = maxr;
        store.chrominanceBlueYellow = maxb;

        dnparams.chrominance =
            store.chrominance * dnparams.chrominanceAutoFactor;
        dnparams.chrominanceRedGreen =
            store.chrominanceRedGreen * dnparams.chrominanceAutoFactor;
        dnparams.chrominanceBlueYellow =
            store.chrominanceBlueYellow * dnparams.chrominanceAutoFactor;

        store.valid = true;

        // printf("DENOISE STORE FINAL:\n  chM = %.6f", store.chM);
        // printf("  max_r = {");
        // for (int i = 0; i < 9; ++i) printf(" %.6f", store.max_r[i]);
        // printf(" }\n  max_b = {");
        // for (int i = 0; i < 9; ++i) printf(" %.6f", store.max_b[i]);
        // printf(" }\n  ch_M = {");
        // for (int i = 0; i < 9; ++i) printf(" %.6f", store.ch_M[i]);
        // printf("}\n");
        // printf("*****************\n\n");
        // fflush(stdout);

        if (settings->verbose) {
            t2aue.set();
            printf("Info denoise auto performed in %d usec:\n",
                   t2aue.etime(t1aue));
        }
        // end evaluate noise
    }
}

void ImProcFunctions::denoise(ImageSource *imgsrc, const ColorTemp &currWB,
                              Imagefloat *img, const DenoiseInfoStore &store,
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
    denoise::NoiseCurve noiseLCurve;
    denoise::NoiseCurve noiseCCurve;

    const int W = img->getWidth();
    const int H = img->getHeight();

    Imagefloat *calclum = nullptr;
    {
        const int fw = W;
        const int fh = H;
        // we only need image reduced to 1/4 here
        calclum = new Imagefloat((fw + 1) / 2,
                                 (fh + 1) / 2); // for luminance denoise curve
#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif

        for (int ii = 0; ii < fh; ii += 2) {
            for (int jj = 0; jj < fw; jj += 2) {
                calclum->r(ii >> 1, jj >> 1) = img->r(ii, jj);
                calclum->g(ii >> 1, jj >> 1) = img->g(ii, jj);
                calclum->b(ii >> 1, jj >> 1) = img->b(ii, jj);
            }
        }
        imgsrc->convertColorSpace(calclum, params->icm, currWB);
    }

    float nresi, highresi;
    DenoiseInfoStore &dnstore = const_cast<DenoiseInfoStore &>(store);

    adjust_params(denoiseParams, scale);

    noiseCCurve.Set(
        {FCT_MinMaxCPoints, 0.05, 0.50, 0.35, 0.35, 0.35, 0.05, 0.35, 0.35});

    if (plistener) {
        plistener->setProgress(0.1);
    }

    ImProcData im(params, scale, multiThread);
    double ecomp = params->exposure.enabled ? params->exposure.expcomp : 0.0;
    ExposureParams expparams;
    expparams.enabled = true;
    expparams.expcomp = ecomp;

    if (ecomp > 0) {
        expcomp(img, &expparams);
    }

    denoise::RGB_denoise(im, 0, img, img, calclum, dnstore.ch_M, dnstore.max_r,
                         dnstore.max_b, true /*imgsrc->isRAW()*/, denoiseParams,
                         0, noiseLCurve, noiseCCurve, nresi, highresi);

    if (plistener) {
        plistener->setProgress(0.8);
    }

    if (denoiseParams.smoothingEnabled) {
        denoise::denoiseGuidedSmoothing(im, img);
        if (denoiseParams.nlStrength) {
            img->setMode(Imagefloat::Mode::YUV, multiThread);
            array2D<float> tmp(img->getWidth(), img->getHeight(), img->g.ptrs,
                               ARRAY2D_BYREFERENCE);
            denoise::NLMeans(tmp, 65535.f, denoiseParams.nlStrength,
                             denoiseParams.nlDetail, scale, multiThread);
            img->setMode(Imagefloat::Mode::RGB, multiThread);
        }
    }

    if (ecomp > 0) {
        expparams.expcomp = -ecomp;
        expcomp(img, &expparams);
    }

    if (plistener) {
        plistener->setProgress(1);
    }
}

} // namespace rtengine
