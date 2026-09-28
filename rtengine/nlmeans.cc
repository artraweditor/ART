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

#include "nlmeans.h"
#include "alignedbuffer.h"
#include "array2D.h"
#include "boxblur.h"
#include "gauss.h"
#include "improcfun.h"
#include "rescale.h"
#include "rt_math.h"
#include "settings.h"
#include "sleef.h"
#ifdef _OPENMP
#include <omp.h>
#endif
#include <iostream>

#define BENCHMARK
#include "StopWatch.h"

#ifdef ART_USE_VULKAN
#include "gpu/plane_io.h"
#include "gpu/ops.h"
#endif // ART_USE_VULKAN

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

namespace rtengine {
namespace denoise {

namespace {

void laplacian(const array2D<float> &src, array2D<float> &dst, float threshold,
               float ceiling, float factor, bool multiThread)
{
    const int W = src.width();
    const int H = src.height();

    const auto X = [W](int x) -> int {
        return x < 0 ? x + 2 : (x >= W ? x - 2 : x);
    };

    const auto Y = [H](int y) -> int {
        return y < 0 ? y + 2 : (y >= H ? y - 2 : y);
    };

    const auto get = [&src](int y, int x) -> float {
        return std::max(src[y][x], 0.f);
    };

    dst(W, H);
    const float f = factor / ceiling;

#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif
    for (int y = 0; y < H; ++y) {
        int n = Y(y - 1), s = Y(y + 1);
        for (int x = 0; x < W; ++x) {
            int w = X(x - 1), e = X(x + 1);
            float v = -8.f * get(y, x) + get(n, x) + get(s, x) + get(y, w) +
                      get(y, e) + get(n, w) + get(n, e) + get(s, w) + get(s, e);
            dst[y][x] = LIM(std::abs(v) - threshold, 0.f, ceiling) * f;
        }
    }
}

} // namespace

void detail_mask(const array2D<float> &src, array2D<float> &mask, float scaling,
                 float threshold, float ceiling, float factor,
                 BlurType blur_type, float blur, bool multithread)
{
    const int W = src.width();
    const int H = src.height();
    mask(W, H);

    if (W < 8 || H < 8) {
        mask.fill(1.f);
    } else {
        array2D<float> L2(W / 4, H / 4, ARRAY2D_ALIGNED);
        array2D<float> m2(W / 4, H / 4, ARRAY2D_ALIGNED);
        rescaleBilinear(src, L2, multithread);
#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
        for (int y = 0; y < H / 4; ++y) {
            for (int x = 0; x < W / 4; ++x) {
                L2[y][x] = xlin2log(L2[y][x] / scaling, 50.f);
            }
        }
        laplacian(L2, m2, threshold / scaling, ceiling / scaling, factor,
                  multithread);
        rescaleBilinear(m2, mask, multithread);

        const auto scurve = [](float x) -> float {
            constexpr float b = 101.f;
            constexpr float a = 2.23f;
            return xlin2log(pow_F(x, a), b);
        };

        const float thr = 1.f - factor;
#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                mask[y][x] = scurve(LIM01(mask[y][x] + thr));
            }
        }

        if (blur_type == BlurType::GAUSS) {
#ifdef _OPENMP
#pragma omp parallel if (multithread)
#endif
            {
                gaussianBlur(mask, mask, W, H, blur);
            }
        } else if (blur_type == BlurType::BOX) {
            if (int(blur) > 0) {
                for (int i = 0; i < 3; ++i) {
                    boxblur(mask, mask, blur, W, H, multithread);
                }
            }
        }
    }

#if 0
    {
        Imagefloat tmp(W, H);
        for (int i = 0; i < H; ++i) {
            for (int j = 0; j < W; ++j) {
                tmp.r(i, j) = tmp.g(i, j) = tmp.b(i, j) = mask[i][j] * 65535.f;
            }
        }
        tmp.saveTIFF("/tmp/mask.tif", 16);
    }
#endif
}


} // namespace denoise
} // namespace rtengine

#ifdef ART_USE_VULKAN

#include "settings.h"
#include "boxblur.h"
#include "gauss.h"
#include "gpu/gpu.h"
#include "gpu/ops.h"
#include "gpu/vk_pass.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>

namespace rtengine {

extern const Settings *settings;

namespace gpu {
namespace ops {

namespace {

struct LogRemapPC { unsigned int w, h; float invScaling, base; };
struct LaplacianPC { unsigned int w, h; float threshold, ceiling, f; };
struct ScurvePC { unsigned int w, h; float thr; };

// downloadPlane itself is gpu::downloadPlane, from gpu/plane_io.h -- this
// file's own copy was identical and has been dropped in favour of it.

} // namespace

/* detail_mask() (above, nlmeans.cc:357): downscale 4x, log
 * remap, laplacian, upscale, S-curve, Gaussian blur. Only BlurType::GAUSS is
 * implemented -- the only blur type either caller ever requests. */
bool detailMask(Context &ctx, Buffer &maskOut, Buffer &src, int W, int H,
                float scaling, float threshold, float ceiling, float factor,
                float blurSigma, BufferPool *pool)
{
    const size_t bytes = (size_t)W * H * sizeof(float);
    if (!maskOut.valid()) {
        return false;
    }
    if (W < 8 || H < 8) {
        /* uploadToBuffer takes the mapped() fast path when there is one, and
         * stages through ctx's own staging pool otherwise -- maskOut itself
         * may be an unmapped PreferDeviceLocal buffer on a discrete GPU. */
        std::vector<float> ones((size_t)W * H, 1.f);
        return uploadToBuffer(ctx, &ctx.stagingPoolForThisThread(),
                              ones.data(), bytes, maskOut);
    }

    /* Scratch, from the pool when there is one.  The locals keep the
     * non-pooled buffers alive for the whole function; `l2`/`m2` below point
     * at whichever pair is in use. */
    const int W4 = W / 4, H4 = H / 4;
    const size_t bytes4 = (size_t)W4 * H4 * sizeof(float);
    Buffer ownL2, ownM2;
    Buffer *l2p, *m2p;
    if (pool) {
        l2p = pool->get(bytes4);
        m2p = pool->get(bytes4);
    } else {
        ownL2 = ctx.createBuffer(bytes4, true);
        ownM2 = ctx.createBuffer(bytes4, true);
        l2p = &ownL2;
        m2p = &ownM2;
    }
    /* Pure GPU-resident scratch -- l2/m2 are only ever produced and consumed
     * by the dispatches below, never touched from the CPU. */
    if (!l2p || !m2p || !l2p->valid() || !m2p->valid()) {
        return false;
    }
    Buffer &l2 = *l2p;
    Buffer &m2 = *m2p;

    /* One Pass for the whole mask: six dispatches that used to be six
     * submissions, each draining the queue and blocking this thread.  The
     * barriers Pass derives order them inside the single command buffer, so
     * nothing here needs a round trip -- the CPU never looks at an
     * intermediate.  The weights upload below is the one exception and is
     * recorded, not submitted, for the same reason. */
    Pass pass(ctx, "nlmeans:detailMask");
    if (!pass.valid()) {
        return false;
    }

    if (!rescaleBilinear(pass, src, W, H, l2, W4, H4)) {
        return false;
    }
    {
        LogRemapPC pc{(unsigned)W4, (unsigned)H4, 1.f / scaling, 50.f};
        std::vector<Pass::Binding> b;
        b.push_back(Pass::Binding(&l2, true));
        if (!pass.dispatch2D("nlm_logremap", b, &pc, sizeof(pc), W4, H4)) {
            return false;
        }
    }
    {
        const float threshScaled = threshold / scaling;
        const float ceilScaled = ceiling / scaling;
        LaplacianPC pc{(unsigned)W4, (unsigned)H4, threshScaled, ceilScaled,
                      factor / ceilScaled};
        std::vector<Pass::Binding> b;
        b.push_back(Pass::Binding(&l2, false));
        b.push_back(Pass::Binding(&m2, true));
        if (!pass.dispatch2D("nlm_laplacian", b, &pc, sizeof(pc), W4, H4)) {
            return false;
        }
    }
    if (!rescaleBilinear(pass, m2, W4, H4, maskOut, W, H)) {
        return false;
    }
    {
        ScurvePC pc{(unsigned)W, (unsigned)H, 1.f - factor};
        std::vector<Pass::Binding> b;
        b.push_back(Pass::Binding(&maskOut, true));
        if (!pass.dispatch2D("nlm_scurve", b, &pc, sizeof(pc), W, H)) {
            return false;
        }
    }
    /* blurSigma <= 0 means no blur, matching BlurType::OFF on the CPU side.
     * The non-pooled Buffers must stay alive until the submit below, so they
     * are declared out here rather than inside the branch. */
    Buffer ownWt, ownScratch;
    {
        const int radius = blurSigma > 0.f ? gaussRadius(blurSigma) : 0;
        if (radius > 0) {
            std::vector<float> wt;
            gaussWeights(blurSigma, radius, wt);
            const size_t wtBytes = wt.size() * sizeof(float);
            Buffer *wtp, *scratchp;
            if (pool) {
                wtp = pool->get(wtBytes);
                scratchp = pool->get(bytes);
            } else {
                ownWt = ctx.createBuffer(wtBytes, true);
                ownScratch = ctx.createBuffer(bytes, true);
                wtp = &ownWt;
                scratchp = &ownScratch;
            }
            /* scratch is pure GPU-resident (ping-pong buffer inside
             * gaussianBlurPasses); wtBuf is CPU-computed. */
            if (!wtp || !scratchp || !wtp->valid() || !scratchp->valid()) {
                return false;
            }
            Buffer &wtBuf = *wtp;
            Buffer &scratch = *scratchp;
            /* A Gaussian kernel is a handful of floats, so it rides inside
             * the command buffer -- vkCmdUpdateBuffer copies it at record
             * time, so `wt` need not outlive this call.  uploadToBuffer()
             * would have been a whole staged submission of its own on any
             * device where wtBuf is not host-visible.  Only a kernel too big
             * for that (far larger than any sigma the UI allows) still needs
             * the staged path. */
            const bool wt_ok =
                wtBytes <= Pass::maxUpdateBytes()
                    ? pass.updateBuffer(wtBuf, wt.data(), 0, wtBytes)
                    : uploadToBuffer(ctx, &ctx.stagingPoolForThisThread(),
                                     wt.data(), wtBytes, wtBuf);
            if (!wt_ok) {
                return false;
            }
            if (!gaussianBlurPasses(pass, maskOut, scratch, wtBuf, W, H,
                                   radius)) {
                return false;
            }
        }
    }
    if (!pass.submitAndWait()) {
        return false;
    }
    pass.reportTimings();
    return true;
}

namespace {

struct NlmPoolCheckout {
    explicit NlmPoolCheckout(BufferPool &p): pool(p) {}
    ~NlmPoolCheckout() { pool.recycle(); }
    BufferPool &pool;

private:
    NlmPoolCheckout(const NlmPoolCheckout &);
    NlmPoolCheckout &operator=(const NlmPoolCheckout &);
};

/* J. Froment, "Parameter-Free Fast Pixelwise Non-Local Means Denoising",
 * IPOL 4 (2014), Algorithm 1 -- one dispatch, one thread per output pixel,
 * direct O(D^2 d^2) search with no intermediate buffers at all (see
 * nlm_simple.comp). */
bool nlmeansSimpleOnDevice(Context &ctx, BufferPool &pool, Buffer &plane,
                          int W, int H, float normcoeff, double scale,
                          int strength, int detail_thresh)
{
    if (W <= 0 || H <= 0) {
        return false;
    }
    if (!strength) {
        return true; // matches NLMeans()'s own strength==0 no-op
    }

    constexpr int max_patch_radius = 2;
    constexpr int max_search_radius = 5;
    const int search_radius =
        (int)std::ceil((float)max_search_radius / (float)scale);
    const int patch_radius =
        (int)std::ceil((float)max_patch_radius / (float)scale);
    const float strengthTerm =
        std::pow((float)strength / 100.f, 0.9f) / 10.f / (float)scale;
    const float h2 = strengthTerm * strengthTerm;
    const float amount =
        std::max(0.f, std::min((float)detail_thresh / 100.f, 0.99f));

    const size_t bytes = (size_t)W * H * sizeof(float);
    if (bytes > ctx.caps().max_storage_buffer_range) {
        logOnce("GPU: nlmeans plane exceeds maxStorageBufferRange; needs "
               "tiling");
        return false;
    }

    Buffer *maskBuf = pool.get(bytes);
    Buffer *dstBuf = pool.get(bytes);
    if (!maskBuf || !dstBuf) {
        logOnce("GPU: nlmeans allocation failed; using the CPU");
        return false;
    }

    if (!detailMask(ctx, *maskBuf, plane, W, H, normcoeff, 1e-3f * normcoeff,
                    normcoeff, amount, 2.f / (float)scale, &pool)) {
        return false;
    }

    struct SimplePC { int w, h, ds, Ds; float invH2; float invNormcoeff; };
    Pass pass(ctx, "nlmeansSimple");
    if (!pass.valid()) {
        return false;
    }
    SimplePC pc{W, H, patch_radius, search_radius, 1.f / h2, 1.f / normcoeff};
    std::vector<Pass::Binding> b;
    b.push_back(Pass::Binding(&plane, false));
    b.push_back(Pass::Binding(maskBuf, false));
    b.push_back(Pass::Binding(dstBuf, true));
    if (!pass.dispatch2D("nlm_simple", b, &pc, sizeof(pc), W, H)) {
        return false;
    }
    /* dstBuf is pool scratch, not `plane` -- the caller's buffer, which on
     * nlmeansSmoothingRegion's path is itself a pooled working copy a
     * different call may reclaim once this returns.  One more flat dispatch
     * (denoise's own dn_plane_copy.comp, generic and already verified)
     * rather than a new shader. */
    struct CopyPC { unsigned int n; };
    CopyPC cpc{(unsigned)(W * H)};
    std::vector<Pass::Binding> cb;
    cb.push_back(Pass::Binding(dstBuf, false));
    cb.push_back(Pass::Binding(&plane, true));
    if (!pass.dispatch1D("dn_plane_copy", cb, &cpc, sizeof(cpc),
                         (size_t)W * H)) {
        return false;
    }
    if (!pass.submitAndWait()) {
        return false;
    }
    if (settings && settings->verbose > 1) {
        pass.reportTimings();
    }
    return true;
}

} // namespace

bool NLMeans(Context &ctx, BufferPool &pool, Buffer &plane, int W,
             int H, float normcoeff, double scale, int strength,
             int detail_thresh)
{
    return nlmeansSimpleOnDevice(ctx, pool, plane, W, H, normcoeff, scale,
                                 strength, detail_thresh);
}

} // namespace ops
} // namespace gpu
} // namespace rtengine

#else // !ART_USE_VULKAN

namespace rtengine {
namespace gpu {
namespace ops {

bool NLMeans(Context &ctx, BufferPool &pool, Buffer &plane, int W,
             int H, float normcoeff, double scale, int strength,
             int detail_thresh)
{
    return false;
}

} // namespace ops
} // namespace gpu
} // namespace rtengine

#endif // ART_USE_VULKAN



