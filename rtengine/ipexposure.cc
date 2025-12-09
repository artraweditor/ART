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

namespace rtengine {

void ImProcFunctions::expcomp(Imagefloat *img,
                              const procparams::ExposureParams *expparams)
{
    if (!expparams) {
        expparams = &params->exposure;
    }

    if (!expparams->enabled) {
        return;
    }

    img->setMode(Imagefloat::Mode::RGB, multiThread);

    const float exp_scale = pow(2.f, expparams->expcomp);
    const float black = expparams->black * 2000.f;
#ifdef __SSE2__
    vfloat exp_scalev = F2V(exp_scale);
    vfloat blackv = F2V(black);
#endif

    const int W = img->getWidth();
    const int H = img->getHeight();

    float **chan[3] = {img->r.ptrs, img->g.ptrs, img->b.ptrs};

#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif
    for (int y = 0; y < H; ++y) {
        int x = 0;
#ifdef __SSE2__
        for (; x < W - 3; x += 4) {
            for (int c = 0; c < 3; ++c) {
                vfloat v = LVF(chan[c][y][x]);
                STVF(chan[c][y][x], vmaxf(v * exp_scalev - blackv, ZEROV));
            }
        }
#endif
        for (; x < W; ++x) {
            for (int c = 0; c < 3; ++c) {
                float &v = chan[c][y][x];
                v = std::max(v * exp_scale - black, 0.f);
            }
        }
    }
}

void ImProcFunctions::exposure(Imagefloat *img) { expcomp(img, nullptr); }

} // namespace rtengine
