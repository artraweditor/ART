/** -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright (c) 2026 Alberto Griggio <alberto.griggio@gmail.com>
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

#pragma once

#include "options.h"
#include "guiutils.h"
#include <array>
#include <cmath>


class SurroundCompensation {
    typedef std::array<guint8, 256> LUT;
public:
    SurroundCompensation():
        verybright_(),
        bright_(),
        dim_(),
        dark_(),
        identity_()
    {
        for (int i = 0; i < 256; ++i) {
            identity_[i] = guint8(i);
            float x = float(i)/255.f;
            verybright_[i] = guint8(255.f * std::pow(x, 2.2f/2.6f));
            bright_[i] = guint8(255.f * std::pow(x, 2.2f/2.4f));
            dim_[i] = guint8(255.f * std::pow(x, 2.4f/2.2f));
            dark_[i] = guint8(255.f * std::pow(x, 2.6f/2.2f));
        }
    }
    
    void apply(Glib::RefPtr<Gdk::Pixbuf> pixbuf,
               Options::ViewingConditions cond) const
    {
        if (cond == Options::ViewingConditions::NORMAL) {
            return;
        }

        auto &lut = get_lut(cond);
        guint8 *pix = pixbuf->get_pixels();

        const int stride = pixbuf->get_rowstride();
        const int H = pixbuf->get_height();
        const int W = pixbuf->get_width();

        const auto get_L = [](guint8 *rgb) -> float {
            return rtengine::LIM(0.299f * rgb[0] + 0.587f * rgb[1] + 0.114f * rgb[2], 0.f, 255.f);
        };

        const auto corr = [](guint8 v, float f, float l) -> guint8 {
            auto res = v * f;
            if (f > 0) {
                auto d = res - l;
                res = l + d / f;
            }
            return rtengine::LIM(res, 0.f, 255.f);
        };
        
#ifdef _OPENMP
#       pragma omp parallel for schedule(dynamic, 16)
#endif
        for (int i = 0; i < H; i++) {
            guint8 *curr = pix + i * stride;
            for (int j = 0; j < W; j++) {
                auto l = get_L(curr);
                auto ll = lut[l];
                auto f = l > 0 ? ll/l : 1;
                // curr[0] = lut[curr[0]];
                // curr[1] = lut[curr[1]];
                // curr[2] = lut[curr[2]];
                curr[0] = corr(curr[0], f, ll);
                curr[1] = corr(curr[1], f, ll);
                curr[2] = corr(curr[2], f, ll);
                curr += 3;
            }
        }
    }

private:
    const LUT &get_lut(Options::ViewingConditions cond) const
    {
        switch (cond) {
        case Options::ViewingConditions::VERY_BRIGHT: return verybright_;
        case Options::ViewingConditions::BRIGHT: return bright_;
        case Options::ViewingConditions::DIM: return dim_;
        case Options::ViewingConditions::DARK: return dark_;
        default: return identity_;
        }
    }
    
    LUT verybright_;
    LUT bright_;
    LUT dim_;
    LUT dark_;
    LUT identity_;
};
