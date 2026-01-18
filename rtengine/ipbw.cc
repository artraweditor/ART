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
// extracted and datapted from ImProcFunctions::rgbProc (improcfun.cc) of
// RawTherapee

#include "color.h"
#include "curves.h"
#include "improcfun.h"
#include <array>
#include <unordered_map>

namespace rtengine {

/** @brief Compute the B&W constants for the Black and White processing and its
 * GUI
 * @param setting main mode
 * @param filter string of the filter effect to use
 * @param algo choice between linear and special for OYCPM colors
 * @param mixerRed red channel value of the channel mixer [-100 ; +200]
 * @param mixerGreen green channel value of the channel mixer [-100 ; +200]
 * @param mixerBlue blue channel value of the channel mixer [-100 ; +200]
 * @param mixerOrange orange channel value of the channel mixer [-100 ; +200]
 * @param mixerYellow yellow channel value of the channel mixer [-100 ; +200]
 * @param mixerCyan cyan channel value of the channel mixer [-100 ; +200]
 * @param mixerPurple purple channel value of the channel mixer [-100 ; +200]
 * @param mixerMagenta magenta channel value of the channel mixer [-100 ; +200]
 * @param autoc automatic mode of the channel mixer
 * @param complement adjust complementary channel
 * @param kcorec in absolute mode, value to correct the mixer [1 ; 3], usually
 * near 1 (return value)
 * @param rrm red channel of the mixer (return value)
 * @param ggm green channel of the mixer (return value)
 * @param bbm blue channel of the mixer (return value)
 */
void computeBWMixerConstants(const Glib::ustring &setting,
                             const Glib::ustring &filter,
                             const Glib::ustring &algo, float &filcor,
                             float &mixerRed, float &mixerGreen,
                             float &mixerBlue, float &kcorec, double &rrm,
                             double &ggm, double &bbm)
{
    float somm;
    float som = mixerRed + mixerGreen + mixerBlue;

    if (som >= 0.f && som < 1.f) {
        som = 1.f;
    }

    if (som < 0.f && som > -1.f) {
        som = -1.f;
    }

    static const std::unordered_map<std::string, std::array<float, 3>> presets =
        {{"NormalContrast", {43.f, 33.f, 30.f}},
         {"Panchromatic", {33.3f, 33.3f, 33.3f}},
         {"HyperPanchromatic", {41.f, 25.f, 34.f}},
         {"LowSensitivity", {27.f, 27.f, 46.f}},
         {"HighSensitivity", {30.f, 28.f, 42.f}},
         {"Orthochromatic", {0.f, 42.f, 58.f}},
         {"HighContrast", {40.f, 34.f, 60.f}},
         {"Luminance", {30.f, 59.f, 11.f}},
         {"Landscape", {66.f, 24.f, 10.f}},
         {"Portrait", {54.f, 44.f, 12.f}},
         {"InfraRed", {-40.f, 200.f, -17.f}}};

    auto it = presets.find(setting);
    if (it != presets.end()) {
        mixerRed = it->second[0];
        mixerGreen = it->second[1];
        mixerBlue = it->second[2];
    }

    // rM = mixerRed, gM = mixerGreen, bM = mixerBlue !
    // presets
    if (setting == "RGB-Abs" || setting == "ROYGCBPM-Abs") {
        kcorec = som / 100.f;
    }

    // if (setting == "NormalContrast") {
    //     mixerRed = 43.f ;
    //     mixerGreen = 33.f;
    //     mixerBlue = 30.f;
    // } else if (setting == "Panchromatic")      {
    //     mixerRed = 33.3f;
    //     mixerGreen = 33.3f;
    //     mixerBlue = 33.3f;
    // } else if (setting == "HyperPanchromatic") {
    //     mixerRed = 41.f ;
    //     mixerGreen = 25.f;
    //     mixerBlue = 34.f;
    // } else if (setting == "LowSensitivity")    {
    //     mixerRed = 27.f ;
    //     mixerGreen = 27.f;
    //     mixerBlue = 46.f;
    // } else if (setting == "HighSensitivity")   {
    //     mixerRed = 30.f ;
    //     mixerGreen = 28.f;
    //     mixerBlue = 42.f;
    // } else if (setting == "Orthochromatic")    {
    //     mixerRed = 0.f  ;
    //     mixerGreen = 42.f;
    //     mixerBlue = 58.f;
    // } else if (setting == "HighContrast")      {
    //     mixerRed = 40.f ;
    //     mixerGreen = 34.f;
    //     mixerBlue = 60.f;
    // } else if (setting == "Luminance")         {
    //     mixerRed = 30.f ;
    //     mixerGreen = 59.f;
    //     mixerBlue = 11.f;
    // } else if (setting == "Landscape")         {
    //     mixerRed = 66.f ;
    //     mixerGreen = 24.f;
    //     mixerBlue = 10.f;
    // } else if (setting == "Portrait")          {
    //     mixerRed = 54.f ;
    //     mixerGreen = 44.f;
    //     mixerBlue = 12.f;
    // } else if (setting == "InfraRed")          {
    //     mixerRed = -40.f;
    //     mixerGreen = 200.f;
    //     mixerBlue = -17.f;
    // }

    rrm = mixerRed;
    ggm = mixerGreen;
    bbm = mixerBlue;

    somm = mixerRed + mixerGreen + mixerBlue;

    if (somm >= 0.f && somm < 1.f) {
        somm = 1.f;
    }

    if (somm < 0.f && somm > -1.f) {
        somm = -1.f;
    }

    mixerRed = mixerRed / somm;
    mixerGreen = mixerGreen / somm;
    mixerBlue = mixerBlue / somm;

    // Color filters
    float filred, filgreen, filblue;
    filred = 1.f;
    filgreen = 1.f;
    filblue = 1.f;
    filcor = 1.f;

    static const std::unordered_map<std::string, std::array<float, 4>> filters =
        {{"None", {1.f, 1.f, 1.f, 1.f}},
         {"Red", {1.f, 0.05f, 0.f, 1.08f}},
         {"Orange", {1.f, 0.6f, 0.f, 1.35f}},
         {"Yellow", {1.f, 1.f, 0.05f, 1.23f}},
         {"YellowGreen", {0.6f, 1.f, 0.3f, 1.32f}},
         {"Green", {0.2f, 1.f, 0.3f, 1.41f}},
         {"Cyan", {0.05f, 1.f, 1.f, 1.23f}},
         {"Blue", {0.f, 0.05f, 1.f, 1.20f}},
         {"Purple", {1.f, 0.05f, 1.f, 1.23f}}};

    auto it2 = filters.find(filter);
    if (it2 != filters.end()) {
        filred = it2->second[0];
        filgreen = it2->second[1];
        filblue = it2->second[2];
        filcor = it2->second[3];
    }

    mixerRed = mixerRed * filred;
    mixerGreen = mixerGreen * filgreen;
    mixerBlue = mixerBlue * filblue;

    if (mixerRed + mixerGreen + mixerBlue == 0) {
        mixerRed += 1.f;
    }

    mixerRed = filcor * mixerRed / (mixerRed + mixerGreen + mixerBlue);
    mixerGreen = filcor * mixerGreen / (mixerRed + mixerGreen + mixerBlue);
    mixerBlue = filcor * mixerBlue / (mixerRed + mixerGreen + mixerBlue);

    if (filter != "None") {
        som = mixerRed + mixerGreen + mixerBlue;

        if (som >= 0.f && som < 1.f) {
            som = 1.f;
        }

        if (som < 0.f && som > -1.f) {
            som = -1.f;
        }

        if (setting == "RGB-Abs") {
            kcorec = kcorec * som;
        }
    }
}

void ImProcFunctions::blackAndWhite(Imagefloat *img)
{
    if (!params->blackwhite.enabled) {
        return;
    }

    img->setMode(Imagefloat::Mode::RGB, multiThread);

    float bwr = float(params->blackwhite.mixerRed);
    float bwg = float(params->blackwhite.mixerGreen);
    float bwb = float(params->blackwhite.mixerBlue);
    float bwrgam = float(params->blackwhite.gammaRed);
    float bwggam = float(params->blackwhite.gammaGreen);
    float bwbgam = float(params->blackwhite.gammaBlue);

    float gamvalr = 125.f;
    float gamvalg = 125.f;
    float gamvalb = 125.f;

    if (bwrgam < 0) {
        gamvalr = 100.f;
    }

    if (bwggam < 0) {
        gamvalg = 100.f;
    }

    if (bwbgam < 0) {
        gamvalb = 100.f;
    }

    float gammabwr = 1.f;
    float gammabwg = 1.f;
    float gammabwb = 1.f;
    {
        gammabwr = 1.f - bwrgam / gamvalr;
        gammabwg = 1.f - bwggam / gamvalg;
        gammabwb = 1.f - bwbgam / gamvalb;
    }
    bool hasgammabw = gammabwr != 1.f || gammabwg != 1.f || gammabwb != 1.f;

    const int W = img->getWidth();
    const int H = img->getHeight();

    float kcorec = 1.f;
    float filcor;
    double rrm, ggm, bbm;
    computeBWMixerConstants(params->blackwhite.setting,
                            params->blackwhite.filter, "", filcor, bwr, bwg,
                            bwb, kcorec, rrm, ggm, bbm);

    LUTf gamma_r, gamma_g, gamma_b;
    if (hasgammabw) {
        gamma_r(65536);
        gamma_g(65536);
        gamma_b(65536);
        for (int i = 0; i < 65536; ++i) {
            gamma_r[i] = pow_F(float(i) / 65535.f, gammabwr) * 65535.f;
            gamma_g[i] = pow_F(float(i) / 65535.f, gammabwg) * 65535.f;
            gamma_b[i] = pow_F(float(i) / 65535.f, gammabwb) * 65535.f;
        }
    }

#ifdef ART_SIMD
    vfloat bwr_v = F2V(bwr);
    vfloat bwg_v = F2V(bwg);
    vfloat bwb_v = F2V(bwb);
    vfloat kcorec_v = F2V(kcorec);
#endif

#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif
    for (int y = 0; y < H; ++y) {
        int x = 0;
#ifdef ART_SIMD
        for (; x < W - 3; x += 4) {
            vfloat r = LVF(img->r(y, x));
            vfloat g = LVF(img->g(y, x));
            vfloat b = LVF(img->b(y, x));
            if (hasgammabw) {
                r = gamma_r[r];
                g = gamma_g[g];
                b = gamma_b[b];
            }
            vfloat bw = ((bwr_v * r + bwg_v * g + bwb_v * b) * kcorec_v);
            STVF(img->r(y, x), bw);
            STVF(img->g(y, x), bw);
            STVF(img->b(y, x), bw);
        }
#endif

        for (; x < W; ++x) {
            float r = img->r(y, x);
            float g = img->g(y, x);
            float b = img->b(y, x);
            if (hasgammabw) {
                r = gamma_r[r];
                g = gamma_g[g];
                b = gamma_b[b];
            }
            img->r(y, x) = img->g(y, x) = img->b(y, x) =
                ((bwr * r + bwg * g + bwb * b) * kcorec);
        }
    }

    if (params->blackwhite.colorCast.getBottom() > 0) {
        // apply color cast
        float s =
            pow_F(float(params->blackwhite.colorCast.getBottom()) / 100.f, 3.f);
        float h = float(params->blackwhite.colorCast.getTop()) / 180.f *
                  rtengine::RT_PI;
        float u, v;
        Color::hsl2yuv(h, s, u, v);
        img->setMode(Imagefloat::Mode::YUV, multiThread);

        LUTf ulut(65536);
        LUTf vlut(65536);
        DiagonalCurve filmcurve(curves::filmcurve_def);
        FlatCurve satcurve({FCT_MinMaxCPoints, 0, 0, 0.35, 0, 0.5, 1, 0.35,
                            0.35, 1, 0, 0, 0.35});
        for (int i = 0; i < 65536; ++i) {
            float x = Color::gamma_srgbclipped(i) / 65535.f;
            float y = filmcurve.getVal(x) * 65535.f;
            float c = satcurve.getVal(x);
            Color::hsl2yuv(h, s * c, u, v);
            ulut[i] = y * u;
            vlut[i] = y * v;
        }

#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif
        for (int y = 0; y < H; ++y) {
            int x = 0;
#ifdef ART_SIMD
            for (; x < W - 3; x += 4) {
                vfloat yv = LVF(img->g(y, x));
                vfloat uv = ulut[yv];
                vfloat vv = vlut[yv];
                STVF(img->b(y, x), LVF(img->b(y, x)) + uv);
                STVF(img->r(y, x), LVF(img->r(y, x)) + vv);
            }
#endif
            for (; x < W; ++x) {
                float u = ulut[img->g(y, x)];
                float v = vlut[img->g(y, x)];
                img->b(y, x) += u;
                img->r(y, x) += v;
            }
        }
    }
}

} // namespace rtengine
