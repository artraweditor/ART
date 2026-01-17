/** -*- C++ -*-
 *  
 *  This file is part of ART
 *
 *  Copyright (c) 2022 Alberto Griggio <alberto.griggio@gmail.com>
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

#include "clutstore.h"

namespace rtengine {

inline void CLUTApplication::apply_single(int thread_id, float &r, float &g, float &b)
{
    if (!ok_) {
        return;
    }

#ifdef ART_USE_OCIO
    if (ocio_processor_) {
        Vec3<float> v(r / 65535.f, g / 65535.f, b / 65535.f);
        v = dot_product(conv_, v);

        OCIO::PackedImageDesc pd(v, 1, 1, 3);
        ocio_processor_->apply(pd);
            
        v = dot_product(iconv_, v);
        r = v[0];
        g = v[1];
        b = v[2];
    } else
#endif // ART_USE_OCIO
#ifdef ART_USE_CTL
    if (!ctl_func_.empty()) {
        Vec3<float> v(r / 65535.f, g / 65535.f, b / 65535.f);
        v = dot_product(conv_, v);

        if (!ctl_lut_.empty()) {
            const int d = ctl_lut_dim_;
            Imath::V3f p(CTL_shaper(v[0], false),
                         CTL_shaper(v[1], false),
                         CTL_shaper(v[2], false));
            p = Ctl::lookup3D(&ctl_lut_[0], Imath::V3i(d, d, d),
                              Imath::V3f(0, 0, 0), Imath::V3f(1, 1, 1),
                              p);
            v[0] = p.x;
            v[1] = p.y;
            v[2] = p.z;
        } else {
            auto func = ctl_func_[thread_id];
        
            for (int i = 0; i < 3; ++i) {
                *reinterpret_cast<float *>(func->inputArg(i)->data()) = v[i];
            }

            func->callFunction(1);

            for (int i = 0; i < 3; ++i) {
                v[i] = *reinterpret_cast<float *>(func->outputArg(i)->data());
            }
        }

        v = dot_product(iconv_, v);
        r = v[0];
        g = v[1];
        b = v[2];
    } else
#endif // ART_USE_CTL
    {
        float out_rgbx[4] ALIGNED16; // Line buffer for CLUT
        float clutr[1] = {r};
        float clutg[1] = {g};
        float clutb[1] = {b};
    
        if (!clut_and_working_profiles_are_same_) {
            // Convert from working to clut profile
            float x, y, z;
            Color::rgbxyz(r, g, b, x, y, z, wprof_);
            Color::xyz2rgb(x, y, z, clutr[0], clutg[0], clutb[0], xyz2clut_);
        }

        // Apply gamma sRGB (default RT)
        clutr[0] = Color::gamma_srgbclipped(clutr[0]);
        clutg[0] = Color::gamma_srgbclipped(clutg[0]);
        clutb[0] = Color::gamma_srgbclipped(clutb[0]);

        hald_clut_->getRGB(strength_, 1, clutr, clutg, clutb, out_rgbx);

        // Apply inverse gamma sRGB
        clutr[0] = Color::igamma_srgb(out_rgbx[0]);
        clutg[0] = Color::igamma_srgb(out_rgbx[1]);
        clutb[0] = Color::igamma_srgb(out_rgbx[2]);

        if (!clut_and_working_profiles_are_same_) {
            // Convert from clut to working profile
            float x, y, z;
            Color::rgbxyz(clutr[0], clutg[0], clutb[0], x, y, z, clut2xyz_);
            Color::xyz2rgb(x, y, z, clutr[0], clutg[0], clutb[0], wiprof_);
        }

        r = clutr[0];
        g = clutg[0];
        b = clutb[0];
    }
}    


#ifdef ART_SIMD

inline void CLUTApplication::apply_vec(int thread_id, vfloat &r, vfloat &g, vfloat &b)
{
    if (!ok_) {
        return;
    }

#ifdef ART_USE_OCIO
    if (ocio_processor_) {
        Vec3<float> v;
        std::vector<float> data(4 * 3);
        for (int k = 0, i = 0; k < 4; ++k) {
            v[0] = r[k] / 65535.f;
            v[1] = g[k] / 65535.f;
            v[2] = b[k] / 65535.f;
            v = dot_product(conv_, v);
            data[i++] = v[0];
            data[i++] = v[1];
            data[i++] = v[2];
        }
        OCIO::PackedImageDesc pd(&data[0], 4, 1, 3);
        ocio_processor_->apply(pd);

        for (int k = 0, i = 0; k < 4; ++k) {
            v[0] = data[i++];
            v[1] = data[i++];
            v[2] = data[i++];
            v = dot_product(iconv_, v);
            r[k] = v[0];
            g[k] = v[1];
            b[k] = v[2];
        }
    } else
#endif // ART_USE_OCIO
#ifdef ART_USE_CTL
    if (!ctl_func_.empty()) {
        auto func = ctl_func_[thread_id];
        
        Vec3<float> v;

        if (!ctl_lut_.empty()) {
            const int d = ctl_lut_dim_;
            for (int k = 0; k < 4; ++k) {
                v[0] = r[k] / 65535.f;
                v[1] = g[k] / 65535.f;
                v[2] = b[k] / 65535.f;
            
                v = dot_product(conv_, v);

                Imath::V3f p(CTL_shaper(v[0], false),
                             CTL_shaper(v[1], false),
                             CTL_shaper(v[2], false));
                p = Ctl::lookup3D(&ctl_lut_[0], Imath::V3i(d, d, d),
                                  Imath::V3f(0, 0, 0), Imath::V3f(1, 1, 1),
                                  p);
                v[0] = p.x;
                v[1] = p.y;
                v[2] = p.z;
                
                v = dot_product(iconv_, v);
                r[k] = v[0];
                g[k] = v[1];
                b[k] = v[2];
            }
        } else {
            for (int k = 0; k < 4; ++k) {
                v[0] = r[k] / 65535.f;
                v[1] = g[k] / 65535.f;
                v[2] = b[k] / 65535.f;
            
                v = dot_product(conv_, v);

                for (int i = 0; i < 3; ++i) {
                    reinterpret_cast<float *>(func->inputArg(i)->data())[k] = v[i];
                }
            }

            func->callFunction(4);

            for (int k = 0; k < 4; ++k) {
                for (int i = 0; i < 3; ++i) {
                    v[i] = reinterpret_cast<float *>(func->outputArg(i)->data())[k];
                }

                v = dot_product(iconv_, v);
                r[k] = v[0];
                g[k] = v[1];
                b[k] = v[2];
            }
        }
    } else
#endif // ART_USE_CTL
    {
        for (int k = 0; k < 4; ++k) {
            float rk = r[k];
            float gk = g[k];
            float bk = b[k];
            apply_single(thread_id, rk, gk, bk);
            r[k] = rk;
            g[k] = gk;
            b[k] = bk;
        }
    }
}    

#endif // ART_SIMD

} // namespace rtengine
