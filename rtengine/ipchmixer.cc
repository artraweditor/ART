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
// matrix mixing extracted and adapted from ImProcFunctions::rgbProc
// (improcfun.cc) of RawTherapee

#include "improcfun.h"
#include "linalgebra.h"

namespace rtengine {

using namespace procparams;

//
// Computes the color correction matrix corresponding to the desired tweak of
// the primaries in terms of hue and saturation.
// Analogous to the "camera calibration" tool of Lightroom.
// Uses the four-color method described in the paper:
//
// Four-Color Matrix Method for Correction of Tristimulus Colorimeters
// by Yoshihiro Ohno and Jonathan E. Hardis
//    National Institute of Standards and Technology
// published in Proc., IS&T Fifth Color Imaging Conference, 301-305 (1997)
//
// implemented by Alberto Griggio
//
void get_mixer_matrix(const ChannelMixerParams &chmix,
                      const Glib::ustring &workingProfile, float &rr, float &rg,
                      float &rb, float &gr, float &gg, float &gb, float &br,
                      float &bg, float &bb)
{
    typedef Vec3f A3;
    typedef Mat33f M33;

    TMatrix ws = ICCStore::getInstance()->workingSpaceMatrix(workingProfile);

    constexpr float D65_x = 0.3127f;
    constexpr float D65_y = 0.3290f;

    const A3 D65_bb_white(D65_x, D65_y, 1.f - D65_x - D65_y);

    const auto rgb2xy = [&](const A3 &rgb) -> A3 {
        A3 xyz = dot_product(ws, rgb);
        float sum = xyz[0] + xyz[1] + xyz[2];
        if (sum == 0.f) {
            return D65_bb_white;
        }
        float x = xyz[0] / sum;
        float y = xyz[1] / sum;
        return A3(x, y, 1.f - x - y);
    };

    const auto get_matrix = [&](const A3 &r_xy, const A3 &g_xy, const A3 &b_xy,
                                const A3 &white) -> M33 {
        M33 m(r_xy[0], g_xy[0], b_xy[0], r_xy[1], g_xy[1], b_xy[1], r_xy[2],
              g_xy[2], b_xy[2]);

        M33 mi = inverse(m);
        A3 kr = dot_product(mi, white);
        M33 kr_m = diagonal(kr[0], kr[1], kr[2]);
        M33 ret = dot_product(m, kr_m);
        return ret;
    };

    A3 red(1.f, 0.f, 0.f);
    A3 green(0.f, 1.f, 0.f);
    A3 blue(0.f, 0.f, 1.f);

    A3 r_xy = rgb2xy(red);
    A3 g_xy = rgb2xy(green);
    A3 b_xy = rgb2xy(blue);
    M33 M = get_matrix(r_xy, g_xy, b_xy, D65_bb_white);

    const auto tweak = [&](const A3 &c, int hue, int sat, float hrange,
                           float srange) -> A3 {
        const float x = c[0], y = c[1];
        const CoordD w(D65_x, D65_y);

        PolarCoord p(CoordD(x, y) - w);
        const float dh = float(hue) / 100.f * 360.f * hrange;
        const float ds = 1.f + (float(sat) / 100.f * srange);
        p.radius *= ds;
        p.angle += dh;
        CoordD d(p);
        d += w;

        return A3(float(d.x), float(d.y), float(1.0 - d.x - d.y));
    };

    M33 N = get_matrix(
        tweak(r_xy, chmix.hue_tweak[0], chmix.sat_tweak[0], 0.075f, 0.3f),
        tweak(g_xy, chmix.hue_tweak[1], chmix.sat_tweak[1], 0.1f, 0.5f),
        tweak(b_xy, chmix.hue_tweak[2], chmix.sat_tweak[2], 0.075f, 0.5f),
        D65_bb_white);

    M33 Minv = inverse(M);
    if (!Minv[1][1]) {
        rr = 1.f;
        rg = 0.f;
        rb = 0.f;

        gr = 0.f;
        gg = 1.f;
        gb = 0.f;

        br = 0.f;
        bg = 0.f;
        bb = 1.f;
    } else {
        M33 res = dot_product(N, Minv);

        rr = res[0][0];
        rg = res[0][1];
        rb = res[0][2];

        gr = res[1][0];
        gg = res[1][1];
        gb = res[1][2];

        br = res[2][0];
        bg = res[2][1];
        bb = res[2][2];
    }
}

void ImProcFunctions::channelMixer(Imagefloat *img)
{
    if (params->chmixer.enabled) {
        img->setMode(Imagefloat::Mode::RGB, multiThread);

        float RR = float(params->chmixer.red[0]) / 1000.f;
        float RG = float(params->chmixer.red[1]) / 1000.f;
        float RB = float(params->chmixer.red[2]) / 1000.f;
        float GR = float(params->chmixer.green[0]) / 1000.f;
        float GG = float(params->chmixer.green[1]) / 1000.f;
        float GB = float(params->chmixer.green[2]) / 1000.f;
        float BR = float(params->chmixer.blue[0]) / 1000.f;
        float BG = float(params->chmixer.blue[1]) / 1000.f;
        float BB = float(params->chmixer.blue[2]) / 1000.f;

        if (params->chmixer.mode ==
            ChannelMixerParams::Mode::PRIMARIES_CHROMA) {
            get_mixer_matrix(params->chmixer, params->icm.workingProfile, RR,
                             RG, RB, GR, GG, GB, BR, BG, BB);
            if (options.rtSettings.verbose) {
                printf("Channel mixer matrix:\n"
                       "   %.1f %.1f %.1f\n"
                       "   %.1f %.1f %.1f\n"
                       "   %.1f %.1f %.1f\n",
                       RR, RG, RB, GR, GG, GB, BR, BG, BB);
                fflush(stdout);
            }
        }

#ifdef __SSE2__
        vfloat vRR = F2V(RR);
        vfloat vRG = F2V(RG);
        vfloat vRB = F2V(RB);
        vfloat vGR = F2V(GR);
        vfloat vGG = F2V(GG);
        vfloat vGB = F2V(GB);
        vfloat vBR = F2V(BR);
        vfloat vBG = F2V(BG);
        vfloat vBB = F2V(BB);
#endif // __SSE2__

#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif
        for (int y = 0; y < img->getHeight(); ++y) {
            int x = 0;
#ifdef __SSE2__
            for (; x < img->getWidth() - 3; x += 4) {
                vfloat r = LVF(img->r(y, x));
                vfloat g = LVF(img->g(y, x));
                vfloat b = LVF(img->b(y, x));

                vfloat rmix = (r * vRR + g * vRG + b * vRB);
                vfloat gmix = (r * vGR + g * vGG + b * vGB);
                vfloat bmix = (r * vBR + g * vBG + b * vBB);

                STVF(img->r(y, x), vmaxf(rmix, ZEROV));
                STVF(img->g(y, x), vmaxf(gmix, ZEROV));
                STVF(img->b(y, x), vmaxf(bmix, ZEROV));
            }
#endif
            for (; x < img->getWidth(); ++x) {
                float r = img->r(y, x);
                float g = img->g(y, x);
                float b = img->b(y, x);

                float rmix = (r * RR + g * RG + b * RB);
                float gmix = (r * GR + g * GG + b * GB);
                float bmix = (r * BR + g * BG + b * BB);

                img->r(y, x) = max(rmix, 0.f);
                img->g(y, x) = max(gmix, 0.f);
                img->b(y, x) = max(bmix, 0.f);
            }
        }
    }
}

} // namespace rtengine
