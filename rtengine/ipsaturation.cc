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
#include "rt_math.h"

namespace rtengine {

namespace {

float apply_vibrance(float x, float vib)
{
    static const float noise = pow_F(2.f, -16.f);
    float ax = std::abs(x / 65535.f);
    if (ax > noise) {
        return SGN(x) * pow_F(ax, vib) * 65535.f;
    } else {
        return x;
    }
}

} // namespace

void ImProcFunctions::saturationVibrance(Imagefloat *rgb)
{
    if (params->saturation.enabled &&
        (params->saturation.saturation || params->saturation.vibrance)) {
        rgb->setMode(Imagefloat::Mode::RGB, multiThread);
        const int W = rgb->getWidth();
        const int H = rgb->getHeight();
        const float saturation = 1.f + params->saturation.saturation / 100.f;
        const float vibrance = 1.f - params->saturation.vibrance / 1000.f;
        TMatrix ws = ICCStore::getInstance()->workingSpaceMatrix(
            params->icm.workingProfile);
        const float noise = pow_F(2.f, -16.f);
        const bool vib = params->saturation.vibrance;

#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif
        for (int i = 0; i < H; ++i) {
            for (int j = 0; j < W; ++j) {
                float &r = rgb->r(i, j);
                float &g = rgb->g(i, j);
                float &b = rgb->b(i, j);
                float l = Color::rgbLuminance(r, g, b, ws);
                float rl = r - l;
                float gl = g - l;
                float bl = b - l;
                if (vib) {
                    rl = apply_vibrance(rl, vibrance);
                    gl = apply_vibrance(gl, vibrance);
                    bl = apply_vibrance(bl, vibrance);
                    assert(rl == rl);
                    assert(gl == gl);
                    assert(bl == bl);
                }
                r = max(l + saturation * rl, noise);
                g = max(l + saturation * gl, noise);
                b = max(l + saturation * bl, noise);
            }
        }
    }
}

} // namespace rtengine
