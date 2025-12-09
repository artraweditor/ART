/* -*- C++ -*-
 *
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2018 Alberto Griggio <alberto.griggio@gmail.com>
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
 * Haze removal using the algorithm described in the paper:
 *
 * Single Image Haze Removal Using Dark Channel Prior
 * by He, Sun and Tang
 *
 * using a guided filter for the "soft matting" of the transmission map
 *
 */

#include "guidedfilter.h"
#include "improcfun.h"
#include "rescale.h"
#include "rt_algo.h"
#include "rt_math.h"
// #include "gauss.h"
#include "boxblur.h"
#include <iostream>
#include <queue>

extern Options options;

namespace rtengine {

namespace {

#if 0
#define DEBUG_DUMP(arr)                                                        \
    do {                                                                       \
        Imagefloat im(arr.width(), arr.height());                              \
        const char *out = "/tmp/" #arr ".tif";                                 \
        for (int y = 0; y < im.getHeight(); ++y) {                             \
            for (int x = 0; x < im.getWidth(); ++x) {                          \
                im.r(y, x) = im.g(y, x) = im.b(y, x) = arr[y][x] * 65535.f;    \
            }                                                                  \
        }                                                                      \
        im.saveTIFF(out, 16);                                                  \
    } while (false)
#else
#define DEBUG_DUMP(arr)
#endif

float normalize(Imagefloat *rgb, bool multithread)
{
    float maxval = 0.f;
    const int W = rgb->getWidth();
    const int H = rgb->getHeight();
#ifdef _OPENMP
#pragma omp parallel for reduction(max : maxval) if (multithread)
#endif
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            maxval = max(maxval, rgb->r(y, x), rgb->g(y, x), rgb->b(y, x));
        }
    }
    maxval = max(maxval * 2.f, 65535.f);
    rgb->multiply(1.f / maxval, multithread);
    return maxval;
}

void restore(Imagefloat *rgb, float maxval, bool multithread)
{
    rgb->multiply(maxval, multithread);
}

int get_dark_channel(const array2D<float> &R, const array2D<float> &G,
                     const array2D<float> &B, array2D<float> &dst,
                     int patchsize, const float ambient[3], bool clip,
                     bool multithread)
{
    const int W = R.width();
    const int H = R.height();

#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
    for (int y = 0; y < H; y += patchsize) {
        const int pH = min(y + patchsize, H);
        for (int x = 0; x < W; x += patchsize) {
            float val = RT_INFINITY_F;
            const int pW = min(x + patchsize, W);
            for (int yy = y; yy < pH; ++yy) {
                for (int xx = x; xx < pW; ++xx) {
                    float r = R[yy][xx];
                    float g = G[yy][xx];
                    float b = B[yy][xx];
                    if (ambient) {
                        r /= ambient[0];
                        g /= ambient[1];
                        b /= ambient[2];
                    }
                    val = min(val, r, g, b);
                }
            }
            if (clip) {
                val = LIM01(val);
            }
            for (int yy = y; yy < pH; ++yy) {
                std::fill(dst[yy] + x, dst[yy] + pW, val);
            }
        }
    }

    return (W / patchsize + ((W % patchsize) > 0)) *
           (H / patchsize + ((H % patchsize) > 0));
}

float estimate_ambient_light(const array2D<float> &R, const array2D<float> &G,
                             const array2D<float> &B,
                             const array2D<float> &dark, int patchsize,
                             int npatches, float ambient[3])
{
    const int W = R.width();
    const int H = R.height();

    const auto get_percentile = [](std::priority_queue<float> &q,
                                   float prcnt) -> float {
        size_t n = LIM<size_t>(q.size() * prcnt, 1, q.size());
        while (q.size() > n) {
            q.pop();
        }
        return q.top();
    };

    const auto OOG = [](float val, float high) -> bool {
        return (val < 0.f) || (val > high);
    };

    float darklim = RT_INFINITY_F;
    {
        std::priority_queue<float> p;
        for (int y = 0; y < H; y += patchsize) {
            for (int x = 0; x < W; x += patchsize) {
                if (!OOG(dark[y][x], 1.f - 1e-5f)) {
                    p.push(dark[y][x]);
                }
            }
        }

        if (p.empty()) {
            return -1.f;
        }
        darklim = get_percentile(p, 0.95);
    }

    std::vector<std::pair<int, int>> patches;
    patches.reserve(npatches);

    for (int y = 0; y < H; y += patchsize) {
        for (int x = 0; x < W; x += patchsize) {
            if (dark[y][x] >= darklim && !OOG(dark[y][x], 1.f)) {
                patches.push_back(std::make_pair(x, y));
            }
        }
    }

    if (options.rtSettings.verbose) {
        std::cout << "dehaze: computing ambient light from " << patches.size()
                  << " patches" << std::endl;
    }

    float bright_lim = RT_INFINITY_F;
    {
        std::priority_queue<float> l;

        for (auto &p : patches) {
            const int pW = min(p.first + patchsize, W);
            const int pH = min(p.second + patchsize, H);

            for (int y = p.second; y < pH; ++y) {
                for (int x = p.first; x < pW; ++x) {
                    l.push(R[y][x] + G[y][x] + B[y][x]);
                }
            }
        }

        if (l.empty()) {
            return -1.f;
        }
        bright_lim = get_percentile(l, 0.95);
    }

    double rr = 0, gg = 0, bb = 0;
    int n = 0;
    for (auto &p : patches) {
        const int pW = min(p.first + patchsize, W);
        const int pH = min(p.second + patchsize, H);

        for (int y = p.second; y < pH; ++y) {
            for (int x = p.first; x < pW; ++x) {
                float r = R[y][x];
                float g = G[y][x];
                float b = B[y][x];
                if (r + g + b >= bright_lim) {
                    rr += r;
                    gg += g;
                    bb += b;
                    ++n;
                }
            }
        }
    }
    n = std::max(n, 1);
    ambient[0] = rr / n;
    ambient[1] = gg / n;
    ambient[2] = bb / n;

    // taken from darktable
    return darklim > 0 ? -1.125f * std::log(darklim)
                       : std::log(std::numeric_limits<float>::max()) / 2;
}

void extract_channels(Imagefloat *img, array2D<float> &r, array2D<float> &g,
                      array2D<float> &b, int radius, float epsilon,
                      bool multithread)
{
    const int W = img->getWidth();
    const int H = img->getHeight();

    array2D<float> imgR(W, H, img->r.ptrs, ARRAY2D_BYREFERENCE);
    rtengine::guidedFilter(imgR, imgR, r, radius, epsilon, multithread);

    array2D<float> imgG(W, H, img->g.ptrs, ARRAY2D_BYREFERENCE);
    rtengine::guidedFilter(imgG, imgG, g, radius, epsilon, multithread);

    array2D<float> imgB(W, H, img->b.ptrs, ARRAY2D_BYREFERENCE);
    rtengine::guidedFilter(imgB, imgB, b, radius, epsilon, multithread);
}

void subtract_black(Imagefloat *img, int percent, bool multithread)
{
    const int W = img->getWidth();
    const int H = img->getHeight();

    constexpr int sizecap = 200;
    float r = float(W) / float(H);
    int ww = r >= 1.f ? sizecap : float(sizecap) / r;
    int hh = r >= 1.f ? float(sizecap) / r : sizecap;

    float black[3] = {RT_INFINITY_F, RT_INFINITY_F, RT_INFINITY_F};
    {
        array2D<float> rr(ww, hh, ARRAY2D_ALIGNED);
        array2D<float> gg(ww, hh, ARRAY2D_ALIGNED);
        array2D<float> bb(ww, hh, ARRAY2D_ALIGNED);
        rescaleNearest(img->r.ptrs, W, H, static_cast<float **>(rr), ww, hh,
                       multithread);
        rescaleNearest(img->g.ptrs, W, H, static_cast<float **>(gg), ww, hh,
                       multithread);
        rescaleNearest(img->b.ptrs, W, H, static_cast<float **>(bb), ww, hh,
                       multithread);

        int radius = std::max(std::max(ww, hh) / 20, 1);
        boxblur(rr, rr, radius, ww, hh, multithread);
        boxblur(gg, gg, radius, ww, hh, multithread);
        boxblur(bb, bb, radius, ww, hh, multithread);

        for (int y = 0; y < hh; ++y) {
            for (int x = 0; x < ww; ++x) {
                black[0] = std::min(black[0], rr[y][x]);
                black[1] = std::min(black[1], gg[y][x]);
                black[2] = std::min(black[2], bb[y][x]);
            }
        }
    }

    const float scaling = float(percent) / 100.f;
    for (int c = 0; c < 3; ++c) {
        black[c] = std::max(0.f, black[c] * scaling);
    }

    if (options.rtSettings.verbose) {
        std::cout << "BLACK POINTS: " << black[0] << " " << black[1] << " "
                  << black[2] << std::endl;
    }

#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            img->r(y, x) = std::max(img->r(y, x) - black[0], 0.f);
            img->g(y, x) = std::max(img->g(y, x) - black[1], 0.f);
            img->b(y, x) = std::max(img->b(y, x) - black[2], 0.f);
        }
    }
}

} // namespace

void ImProcFunctions::dehaze(Imagefloat *img)
{
    PlanarWhateverData<float> *editWhatever = nullptr;
    EditUniqueID editID =
        pipetteBuffer ? pipetteBuffer->getEditID() : EUID_None;

    if (editID == EUID_DehazeStrength &&
        pipetteBuffer->getDataProvider()
                ->getCurrSubscriber()
                ->getPipetteBufferType() == BT_SINGLEPLANE_FLOAT) {
        editWhatever = pipetteBuffer->getSinglePlaneBuffer();
    }

    if (!params->dehaze.enabled) {
        if (editWhatever) {
            editWhatever->fill(0.f);
        }
        return;
    }

    TMatrix ws =
        ICCStore::getInstance()->workingSpaceMatrix(params->icm.workingProfile);

    img->setMode(Imagefloat::Mode::RGB, multiThread);
    const float maxchan = normalize(img, multiThread);

    const int W = img->getWidth();
    const int H = img->getHeight();

    if (editWhatever) {
#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif
        for (int y = 0; y < editWhatever->getHeight(); ++y) {
            int yy = y + offset_y;
            for (int x = 0; x < editWhatever->getWidth(); ++x) {
                int xx = x + offset_x;
                float r = img->r(yy, xx), g = img->g(yy, xx),
                      b = img->b(yy, xx);
                float Y = Color::rgbLuminance(r, g, b, ws) * maxchan;
                float s = Color::gamma2curve[Y];
                editWhatever->v(y, x) = LIM01(s / 65535.f);
            }
        }
    }

    if (params->dehaze.blackpoint) {
        subtract_black(img, params->dehaze.blackpoint, multiThread);
    }

    // const float strength = LIM01(float(std::abs(params->dehaze.strength)) /
    // 100.f * 0.9f); const bool add_haze = params->dehaze.strength < 0;

    // if (options.rtSettings.verbose) {
    //     std::cout << "dehaze: strength = " << strength << std::endl;
    // }

    array2D<float> dark(W, H, ARRAY2D_ALIGNED);

    int patchsize = max(int(5 / scale), 2);
    float ambient[3];
    array2D<float> &t_tilde = dark;
    float max_t = 0.f;

    {
        // array2D<float> R(W, H);
        array2D<float> &R =
            dark; // R and dark can safely use the same buffer, which is faster
                  // and reduces memory allocations/deallocations
        array2D<float> G(W, H, ARRAY2D_ALIGNED);
        array2D<float> B(W, H, ARRAY2D_ALIGNED);
        extract_channels(img, R, G, B, patchsize, 1e-1, multiThread);

        {
            constexpr int sizecap = 200;
            float r = float(W) / float(H);
            int ww = r >= 1.f ? sizecap : float(sizecap) / r;
            int hh = r >= 1.f ? float(sizecap) / r : sizecap;
            array2D<float> RR(ww, hh, ARRAY2D_ALIGNED);
            array2D<float> GG(ww, hh, ARRAY2D_ALIGNED);
            array2D<float> BB(ww, hh, ARRAY2D_ALIGNED);
            rescaleNearest(R, RR, multiThread);
            rescaleNearest(G, GG, multiThread);
            rescaleNearest(B, BB, multiThread);
            array2D<float> D(ww, hh);

            patchsize = 2;
            int npatches = get_dark_channel(RR, GG, BB, D, patchsize, nullptr,
                                            false, multiThread);
            max_t = estimate_ambient_light(RR, GG, BB, D, patchsize, npatches,
                                           ambient);
            if (max_t < 0.f) {
                if (options.rtSettings.verbose) {
                    std::cout << "dehaze: no haze detected" << std::endl;
                }
                restore(img, maxchan, multiThread);
                return; // probably no haze at all
            }
        }

        patchsize = max(max(W, H) / 600, 2);

        if (options.rtSettings.verbose) {
            std::cout << "dehaze: ambient light is " << ambient[0] << ", "
                      << ambient[1] << ", " << ambient[2] << std::endl;
        }

        get_dark_channel(R, G, B, dark, patchsize, ambient, true, multiThread);
    }

    // if (min(ambient[0], ambient[1], ambient[2]) < 0.01f) {
    //     if (options.rtSettings.verbose) {
    //         std::cout << "dehaze: no haze detected" << std::endl;
    //     }
    //     restore(img, maxchan, multiThread);
    //     return; // probably no haze at all
    // }

    DEBUG_DUMP(t_tilde);

    array2D<bool> add_haze(W, H);

    FlatCurve strength_curve(params->dehaze.strength, false);
    strength_curve.setIdentityValue(0.5);
    LUTf strength(65536);
    for (int i = 0; i < 65536; ++i) {
        strength[i] =
            (strength_curve.getVal(Color::gamma2curve[i] / 65535.f) - 0.5f) *
            1.3f;
    }

#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            float Y = Color::rgbLuminance(img->r(y, x), img->g(y, x),
                                          img->b(y, x), ws) *
                      maxchan;
            float s = strength[Y];
            add_haze[y][x] = s < 0;
            dark[y][x] = 1.f - std::abs(s) * dark[y][x];
        }
    }

    const int radius = patchsize * 4;
    const float epsilon = 1e-5;
    array2D<float> &t = t_tilde;

    {
        array2D<float> guideB(W, H, img->b.ptrs, ARRAY2D_BYREFERENCE);
        rtengine::guidedFilter(guideB, t_tilde, t, radius, epsilon,
                               multiThread);
    }

    DEBUG_DUMP(t);

    if (options.rtSettings.verbose) {
        std::cout << "dehaze: max distance is " << max_t << std::endl;
    }

    float depth = -float(params->dehaze.depth) / 100.f;
    const float teps = 1e-6f;
    const float t0 = max(teps, std::exp(depth * max_t));
    const bool luminance = params->dehaze.luminance;
    const float ambientY =
        Color::rgbLuminance(ambient[0], ambient[1], ambient[2], ws);
#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            // ensure that the transmission is such that to avoid clipping...
            float rgb[3] = {img->r(y, x), img->g(y, x), img->b(y, x)};
            // ... t >= tl to avoid negative values
            float tl = 1.f - min(rgb[0] / ambient[0], rgb[1] / ambient[1],
                                 rgb[2] / ambient[2]);
            // // ... t >= tu to avoid values > 1
            // float tu = t0 - teps;
            // for (int c = 0; c < 3; ++c) {
            //     if (ambient[c] < 1) {
            //         tu = max(tu, (rgb[c] - ambient[c])/(1.f - ambient[c]));
            //     }
            // }
            float &ir = img->r(y, x);
            float &ig = img->g(y, x);
            float &ib = img->b(y, x);

            float mt = max(t[y][x], t0, tl + teps); //, tu + teps);
            if (params->dehaze.showDepthMap) {
                img->r(y, x) = img->g(y, x) = img->b(y, x) = LIM01(1.f - mt);
            } else if (luminance) {
                float Y = Color::rgbLuminance(rgb[0], rgb[1], rgb[2], ws);
                float YY = (Y - ambientY) / mt + ambientY;
                if (Y > 1e-5f) {
                    if (add_haze[y][x]) {
                        YY = Y + Y - YY;
                    }
                    float f = YY / Y;
                    ir = rgb[0] * f;
                    ig = rgb[1] * f;
                    ib = rgb[2] * f;
                }
            } else {
                float r = (rgb[0] - ambient[0]) / mt + ambient[0];
                float g = (rgb[1] - ambient[1]) / mt + ambient[1];
                float b = (rgb[2] - ambient[2]) / mt + ambient[2];

                if (add_haze[y][x]) {
                    ir += (ir - r);
                    ig += (ig - g);
                    ib += (ib - b);
                } else {
                    ir = r;
                    ig = g;
                    ib = b;
                }
            }
        }
    }

    restore(img, maxchan, multiThread);
}

} // namespace rtengine
