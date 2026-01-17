/** -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright (c) 2021 Alberto Griggio <alberto.griggio@gmail.com>
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

#include "alignedbuffer.h"
#include "gauss.h"
#include "improcfun.h"
#include "ipdenoise.h"
#include "rescale.h"
#include "settings.h"
#include "sleef.h"
#ifdef _OPENMP
#include <omp.h>
#endif
#include <iostream>

#define BENCHMARK
#include "StopWatch.h"

namespace rtengine {

extern const Settings *settings;

namespace denoise {

// basic idea taken from Algorithm 3 in the paper:
// "Parameter-Free Fast Pixelwise Non-Local Means Denoising"
// by Jacques Froment

//
// thanks to Ingo Weyrich <heckflosse67@gmx.de> for many speedup suggestions!
//

void NLMeans(array2D<float> &img, float normcoeff, int strength,
             int detail_thresh, float scale, bool multithread)
{
    if (!strength) {
        return;
    }

    BENCHFUN

    // these two can be changed if needed. increasing max_patch_radius doesn't
    // affect performance, whereas max_search_radius *really* does
    // (the complexity is O(max_search_radius^2 * W * H))
    constexpr int max_patch_radius = 2;
    constexpr int max_search_radius = 5;

    const int search_radius = int(std::ceil(float(max_search_radius) / scale));
    const int patch_radius = int(std::ceil(float(max_patch_radius) / scale));

    const int W = img.width();
    const int H = img.height();

    // the strength parameter controls the scaling of the weights
    // (called h^2 in the papers)
    const float h2 =
        SQR(std::pow(float(strength) / 100.f, 0.9f) / 10.f /*30.f*/ / scale);

    // this is the main difference between our version and more conventional
    // nl-means implementations: instead of varying the patch size, we control
    // the detail preservation by using a varying weight scaling for the
    // pixels, depending on our estimate of how much details there are in the
    // pixel neighborhood. We do this by computing a "detail mask", using a
    // laplacian filter with additional averaging and smoothing. The
    // detail_thresh parameter controls the degree of detail preservation: the
    // (averaged, smoothed) laplacian is first normalized to [0,1], and then
    // modified by compression and offseting depending on the detail_thresh
    // parameter, i.e. mask[y][x] = mask[y][x] * (1 - f) + f,
    // where f = detail_thresh / 100
    float amount = LIM(float(detail_thresh) / 100.f, 0.f, 0.99f);
    array2D<float> mask(W, H, ARRAY2D_ALIGNED);
    {
        array2D<float> &LL = img;
        detail_mask(LL, mask, normcoeff, 1e-3f * normcoeff, normcoeff, amount,
                    BlurType::GAUSS, 2.f / scale, multithread);
    }

    auto &dst = img;
    const int border = search_radius + patch_radius;
    const int WW = W + border * 2;
    const int HH = H + border * 2;

    const float factor = normcoeff;
    array2D<float> src(WW, HH, ARRAY2D_ALIGNED);
#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
    for (int y = 0; y < HH; ++y) {
        int yy = y <= border ? 0 : y >= H ? H - 1 : y - border;
        for (int x = 0; x < WW; ++x) {
            int xx = x <= border ? 0 : x >= W ? W - 1 : x - border;
            float Y = img[yy][xx] / factor;
            src[y][x] = Y;
        }
    }

#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            // dst->g(y, x) = 0.f;
            dst[y][x] = 0.f;
        }
    }

    constexpr int lutsz = 8192;
    constexpr float lutfactor = 100.f / float(lutsz - 1);
    LUTf explut(lutsz);
    for (int i = 0; i < lutsz; ++i) {
        float x = float(i) * lutfactor;
        explut[i] = xexpf(-x);
    }

#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            mask[y][x] = (1.f / (mask[y][x] * h2)) / lutfactor;
        }
    }

    // process by tiles to avoid numerical accuracy errors in the computation
    // of the integral image
    const int tile_size = 150;
    const int ntiles_x = int(std::ceil(float(WW) / (tile_size - 2 * border)));
    const int ntiles_y = int(std::ceil(float(HH) / (tile_size - 2 * border)));
    const int ntiles = ntiles_x * ntiles_y;

#ifdef ART_SIMD
    const vfloat zerov = F2V(0.0);
    const vfloat v1e_5f = F2V(1e-5f);
    const vfloat v65535f = F2V(factor);
#endif

#ifdef _OPENMP
#pragma omp parallel if (multithread)
#endif
    {

#ifdef ART_SIMD
        // flush denormals to zero to avoid performance penalty
        const auto oldMode = _MM_GET_FLUSH_ZERO_MODE();
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
#endif

#ifdef _OPENMP
#pragma omp for schedule(dynamic, 2)
#endif
        for (int tile = 0; tile < ntiles; ++tile) {
            const int tile_y = tile / ntiles_x;
            const int tile_x = tile % ntiles_x;

            const int start_y = tile_y * (tile_size - 2 * border);
            const int end_y = std::min(start_y + tile_size, HH);
            const int TH = end_y - start_y;

            const int start_x = tile_x * (tile_size - 2 * border);
            const int end_x = std::min(start_x + tile_size, WW);
            const int TW = end_x - start_x;

            const auto Y = [=](int y) -> int {
                return LIM(y + start_y, 0, HH - 1);
            };
            const auto X = [=](int x) -> int {
                return LIM(x + start_x, 0, WW - 1);
            };

            const auto score = [&](int tx, int ty, int zx, int zy) -> float {
                return SQR(src[Y(zy)][X(zx)] - src[Y(zy + ty)][X(zx + tx)]);
            };

            array2D<float> St(TW, TH, ARRAY2D_ALIGNED);
            array2D<float> SW(TW, TH, ARRAY2D_ALIGNED | ARRAY2D_CLEAR_DATA);

            for (int ty = -search_radius; ty <= search_radius; ++ty) {
                for (int tx = -search_radius; tx <= search_radius; ++tx) {
                    // Step 1 — Compute the integral image St
                    St[0][0] = 0.f;
                    for (int xx = 1; xx < TW; ++xx) {
                        St[0][xx] = St[0][xx - 1] + score(tx, ty, xx, 0);
                    }
                    for (int yy = 1; yy < TH; ++yy) {
                        St[yy][0] = St[yy - 1][0] + score(tx, ty, 0, yy);
                    }
                    for (int yy = 1; yy < TH; ++yy) {
                        for (int xx = 1; xx < TW; ++xx) {
                            // operation grouping tuned for performance
                            // (empirically)
                            St[yy][xx] =
                                (St[yy][xx - 1] + St[yy - 1][xx]) -
                                (St[yy - 1][xx - 1] - score(tx, ty, xx, yy));
                        }
                    }
                    // Step 2 — Compute weight and estimate for patches
                    // V(x), V(y) with y = x + t
                    for (int yy = start_y + border; yy < end_y - border; ++yy) {
                        int y = yy - border;
                        int xx = start_x + border;
#ifdef ART_SIMD
                        for (; xx < end_x - border - 3; xx += 4) {
                            int x = xx - border;
                            int sx = xx + tx;
                            int sy = yy + ty;

                            int sty = yy - start_y;
                            int stx = xx - start_x;

                            vfloat dist2 =
                                LVFU(St[sty + patch_radius]
                                       [stx + patch_radius]) +
                                LVFU(St[sty - patch_radius]
                                       [stx - patch_radius]) -
                                LVFU(St[sty + patch_radius]
                                       [stx - patch_radius]) -
                                LVFU(
                                    St[sty - patch_radius][stx + patch_radius]);
                            dist2 = vmaxf(dist2, zerov);
                            vfloat d = dist2 * LVFU(mask[y][x]);
                            vfloat weight = explut[d];
                            STVFU(SW[y - start_y][x - start_x],
                                  LVFU(SW[y - start_y][x - start_x]) + weight);
                            vfloat Y = weight * LVFU(src[sy][sx]);
                            STVFU(dst[y][x], LVFU(dst[y][x]) + Y);
                        }
#endif
                        for (; xx < end_x - border; ++xx) {
                            int x = xx - border;
                            int sx = xx + tx;
                            int sy = yy + ty;

                            int sty = yy - start_y;
                            int stx = xx - start_x;

                            float dist2 =
                                St[sty + patch_radius][stx + patch_radius] +
                                St[sty - patch_radius][stx - patch_radius] -
                                St[sty + patch_radius][stx - patch_radius] -
                                St[sty - patch_radius][stx + patch_radius];
                            dist2 = std::max(dist2, 0.f);
                            float d = dist2 * mask[y][x];
                            float weight = explut[d];
                            SW[y - start_y][x - start_x] += weight;
                            float Y = weight * src[sy][sx];
                            dst[y][x] += Y;

                            assert(!xisinff(dst[y][x]));
                            assert(!xisnanf(dst[y][x]));
                        }
                    }
                }
            }

            // Compute final estimate at pixel x = (x1, x2)
            for (int yy = start_y + border; yy < end_y - border; ++yy) {
                int y = yy - border;
                int xx = start_x + border;
#ifdef ART_SIMD
                for (; xx < end_x - border - 3; xx += 4) {
                    int x = xx - border;

                    const vfloat Y = LVFU(dst[y][x]);
                    const vfloat f =
                        (v1e_5f + LVFU(SW[y - start_y][x - start_x]));
                    STVFU(dst[y][x], (Y / f) * v65535f);
                }
#endif
                for (; xx < end_x - border; ++xx) {
                    int x = xx - border;

                    const float Y = dst[y][x];
                    const float f = (1e-5f + SW[y - start_y][x - start_x]);
                    dst[y][x] = (Y / f) * factor;

                    assert(!xisnanf(dst[y][x]));
                }
            }
        }

#ifdef ART_SIMD
        _MM_SET_FLUSH_ZERO_MODE(oldMode);
#endif
    } // omp parallel
}

} // namespace denoise
} // namespace rtengine
