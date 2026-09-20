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

/**
 * This is a Fast Guided Filter implementation, derived directly from the
 * pseudo-code of the paper:
 *
 * Fast Guided Filter
 * by Kaiming He, Jian Sun
 *
 * available at https://arxiv.org/abs/1505.00996
 */

#include "guidedfilter.h"
#include "boxblur.h"
#include "imagefloat.h"
#include "rescale.h"
#include "sleef.h"

namespace rtengine {

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

namespace {

int calculate_subsampling(int w, int h, int r)
{
    if (r == 1) {
        return 1;
    }

    if (max(w, h) <= 600) {
        return 1;
    }

    for (int s = 5; s > 0; --s) {
        if (r % s == 0) {
            return s;
        }
    }

    return LIM(r / 2, 2, 4);
}

} // namespace

void guidedFilter(const array2D<float> &guide, const array2D<float> &src,
                  array2D<float> &dst, int r, float epsilon, bool multithread,
                  int subsampling)
{

    const int W = src.width();
    const int H = src.height();

    if (subsampling <= 0) {
        subsampling = calculate_subsampling(W, H, r);
    }

    enum Op { MUL, DIVEPSILON, ADD, SUB, ADDMUL, SUBMUL };

    const auto apply = [=](Op op, array2D<float> &res, const array2D<float> &a,
                           const array2D<float> &b,
                           const array2D<float> &c = array2D<float>()) -> void {
        const int w = res.width();
        const int h = res.height();

#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                float r;
                float aa = a[y][x];
                float bb = b[y][x];
                switch (op) {
                case MUL:
                    r = aa * bb;
                    break;
                case DIVEPSILON:
                    r = aa / (bb + epsilon);
                    break;
                case ADD:
                    r = aa + bb;
                    break;
                case SUB:
                    r = aa - bb;
                    break;
                case ADDMUL:
                    r = aa * bb + c[y][x];
                    break;
                case SUBMUL:
                    r = c[y][x] - (aa * bb);
                    break;
                default:
                    assert(false);
                    r = 0;
                    break;
                }
                res[y][x] = r;
            }
        }
    };

    // use the terminology of the paper (Algorithm 2)
    const array2D<float> &I = guide;
    const array2D<float> &p = src;
    array2D<float> &q = dst;

    const auto f_subsample = [=](array2D<float> &d,
                                 const array2D<float> &s) -> void {
        if (d.width() == s.width() && d.height() == s.height()) {
#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
            for (int y = 0; y < s.height(); ++y) {
                for (int x = 0; x < s.width(); ++x) {
                    d[y][x] = s[y][x];
                }
            }
        } else {
            rescaleBilinear(s, d, multithread);
        }
    };

    // const auto f_upsample = f_subsample;

    const size_t w = W / subsampling;
    const size_t h = H / subsampling;

    const auto f_mean = [multithread](array2D<float> &d, array2D<float> &s,
                                      int rad) -> void {
        rad = LIM(rad, 0, (min(s.width(), s.height()) - 1) / 2 - 1);
        boxblur(s, d, rad, s.width(), s.height(), multithread);
    };

    array2D<float> I1(w, h, ARRAY2D_ALIGNED);
    array2D<float> p1(w, h, ARRAY2D_ALIGNED);

    f_subsample(I1, I);
    f_subsample(p1, p);

    DEBUG_DUMP(I);
    DEBUG_DUMP(p);
    DEBUG_DUMP(I1);
    DEBUG_DUMP(p1);

    float r1 = float(r) / subsampling;

    array2D<float> meanI(w, h, ARRAY2D_ALIGNED);
    f_mean(meanI, I1, r1);
    DEBUG_DUMP(meanI);

    array2D<float> meanp(w, h, ARRAY2D_ALIGNED);
    f_mean(meanp, p1, r1);
    DEBUG_DUMP(meanp);

    array2D<float> &corrIp = p1;
    apply(MUL, corrIp, I1, p1);
    f_mean(corrIp, corrIp, r1);
    DEBUG_DUMP(corrIp);

    array2D<float> &corrI = I1;
    apply(MUL, corrI, I1, I1);
    f_mean(corrI, corrI, r1);
    DEBUG_DUMP(corrI);

    array2D<float> &varI = corrI;
    apply(SUBMUL, varI, meanI, meanI, corrI);
    DEBUG_DUMP(varI);

    array2D<float> &covIp = corrIp;
    apply(SUBMUL, covIp, meanI, meanp, corrIp);
    DEBUG_DUMP(covIp);

    array2D<float> &a = varI;
    apply(DIVEPSILON, a, covIp, varI);
    DEBUG_DUMP(a);

    array2D<float> &b = covIp;
    apply(SUBMUL, b, a, meanI, meanp);
    DEBUG_DUMP(b);

    array2D<float> &meana = a;
    f_mean(meana, a, r1);
    DEBUG_DUMP(meana);

    array2D<float> &meanb = b;
    f_mean(meanb, b, r1);
    DEBUG_DUMP(meanb);

    // speedup by heckflosse67
    const int Ws = meana.width();
    const int Hs = meana.height();
    const int Wd = q.width();
    const int Hd = q.height();
    const float col_scale = float(Ws) / float(Wd);
    const float row_scale = float(Hs) / float(Hd);

#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
    for (int y = 0; y < Hd; ++y) {
        float ymrs = y * row_scale;
        for (int x = 0; x < Wd; ++x) {
            q[y][x] = getBilinearValue(meana, x * col_scale, ymrs) * I[y][x] +
                      getBilinearValue(meanb, x * col_scale, ymrs);
        }
    }
}

void guidedFilterLog(const array2D<float> &guide, float base,
                     array2D<float> &chan, int r, float eps, bool multithread,
                     int subsampling)
{
#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
    for (int y = 0; y < chan.height(); ++y) {
        for (int x = 0; x < chan.width(); ++x) {
            chan[y][x] = xlin2log(max(chan[y][x], 0.f), base);
        }
    }

    guidedFilter(guide, chan, chan, r, eps, multithread, subsampling);

#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
    for (int y = 0; y < chan.height(); ++y) {
        for (int x = 0; x < chan.width(); ++x) {
            chan[y][x] = xlog2lin(max(chan[y][x], 0.f), base);
        }
    }
}

void guidedFilterLog(float base, array2D<float> &chan, int r, float eps,
                     bool multithread, int subsampling)
{
    guidedFilterLog(chan, base, chan, r, eps, multithread, subsampling);
}

} // namespace rtengine

#ifdef ART_USE_VULKAN

#include "gpu/gpu.h"
#include "gpu/ops.h"
#include "gpu/vk_pass.h"

namespace rtengine {
namespace gpu {
namespace ops {

namespace {

/* guidedfilter.cc's calculate_subsampling, ported verbatim: pure integer
 * arithmetic, no GPU-specific behaviour to reconsider. */
int calcSubsampling(int w, int h, int r)
{
    if (r == 1) {
        return 1;
    }
    if (std::max(w, h) <= 600) {
        return 1;
    }
    for (int s = 5; s > 0; --s) {
        if (r % s == 0) {
            return s;
        }
    }
    int v = r / 2;
    return std::min(std::max(v, 2), 4);
}

int clampBoxRadius(float rad_f, int w, int h)
{
    int rad = (int)rad_f;
    int lim = (std::min(w, h) - 1) / 2 - 1;
    return std::max(0, std::min(rad, lim));
}

struct GfPass1PC { unsigned int w, h; int radius; };
struct GfPass2PC { unsigned int w, h; int radius; float epsilon; };
struct RescalePC { unsigned int ws, hs, wd, hd; };

bool rescale(Pass &pass, Buffer &src, Buffer &dst, int ws, int hs, int wd,
            int hd)
{
    RescalePC pc{(unsigned)ws, (unsigned)hs, (unsigned)wd, (unsigned)hd};
    std::vector<Pass::Binding> b;
    b.push_back(Pass::Binding(&src, false));
    b.push_back(Pass::Binding(&dst, true));
    return pass.dispatch2D("mask_rescale_bilinear", b, &pc, sizeof(pc), wd, hd);
}

/* Stage 1/4 (horizontal): shrinking-window horizontal mean of I, p, I^2,
 * I*p, fused into one dispatch -- see gf_pass1_horiz.comp. */
bool gfPass1(Pass &pass, Buffer &I, Buffer &p, Buffer &hI, Buffer &hp,
            Buffer &hII, Buffer &hIp, int w, int h, int radius)
{
    GfPass1PC pc{(unsigned)w, (unsigned)h, radius};
    std::vector<Pass::Binding> b;
    b.push_back(Pass::Binding(&I, false));
    b.push_back(Pass::Binding(&p, false));
    b.push_back(Pass::Binding(&hI, true));
    b.push_back(Pass::Binding(&hp, true));
    b.push_back(Pass::Binding(&hII, true));
    b.push_back(Pass::Binding(&hIp, true));
    return pass.dispatch2D("gf_pass1_horiz", b, &pc, sizeof(pc), w, h);
}

/* Stage 2/4 (vertical + coefficients): completes the 2D shrinking-window
 * mean and derives a,b in the same dispatch -- see gf_pass2_vert_ab.comp. */
bool gfPass2(Pass &pass, Buffer &hI, Buffer &hp, Buffer &hII, Buffer &hIp,
            Buffer &a, Buffer &b, int w, int h, int radius, float epsilon)
{
    GfPass2PC pc{(unsigned)w, (unsigned)h, radius, epsilon};
    std::vector<Pass::Binding> bd;
    bd.push_back(Pass::Binding(&hI, false));
    bd.push_back(Pass::Binding(&hp, false));
    bd.push_back(Pass::Binding(&hII, false));
    bd.push_back(Pass::Binding(&hIp, false));
    bd.push_back(Pass::Binding(&a, true));
    bd.push_back(Pass::Binding(&b, true));
    return pass.dispatch2D("gf_pass2_vert_ab", bd, &pc, sizeof(pc), w, h);
}

/* Stage 3/4 (horizontal): shrinking-window horizontal mean of a,b -- see
 * gf_pass3_horiz_ab.comp. */
bool gfPass3(Pass &pass, Buffer &a, Buffer &b, Buffer &ha, Buffer &hb, int w,
            int h, int radius)
{
    GfPass1PC pc{(unsigned)w, (unsigned)h, radius};
    std::vector<Pass::Binding> bd;
    bd.push_back(Pass::Binding(&a, false));
    bd.push_back(Pass::Binding(&b, false));
    bd.push_back(Pass::Binding(&ha, true));
    bd.push_back(Pass::Binding(&hb, true));
    return pass.dispatch2D("gf_pass3_horiz_ab", bd, &pc, sizeof(pc), w, h);
}

/* Stage 4/4 (vertical): completes box(a),box(b) -- see gf_pass4_vert_ab.comp.
 */
bool gfPass4(Pass &pass, Buffer &ha, Buffer &hb, Buffer &meanA, Buffer &meanB,
            int w, int h, int radius)
{
    GfPass1PC pc{(unsigned)w, (unsigned)h, radius};
    std::vector<Pass::Binding> bd;
    bd.push_back(Pass::Binding(&ha, false));
    bd.push_back(Pass::Binding(&hb, false));
    bd.push_back(Pass::Binding(&meanA, true));
    bd.push_back(Pass::Binding(&meanB, true));
    return pass.dispatch2D("gf_pass4_vert_ab", bd, &pc, sizeof(pc), w, h);
}

} // namespace

/* guidedfilter.cc's guidedFilter, Algorithm 2 of the Fast Guided Filter
 * paper, rewritten as a 4-stage separable pipeline (adapted from a classic
 * 4-pass "horizontal / vertical+ab / horizontal / vertical+compose" GLSL
 * fragment-shader formulation of the fast guided filter) instead of the
 * previous chain of ~10 generic elementwise + 2D-box-blur dispatches.
 * guideFull/srcFull/dstFull are W x H; dstFull may alias srcFull.
 *
 * The 4 stages (gf_pass1_horiz / gf_pass2_vert_ab / gf_pass3_horiz_ab /
 * gf_pass4_vert_ab) fuse the I*I/I*p products into stage 1's horizontal
 * box-mean sweep, and the var/cov/a/b elementwise algebra into stage 2's
 * vertical box-mean sweep, cutting the box-filtering + coefficient part of
 * this function from ~10 dispatches (2D box blur, O(radius^2) per pixel,
 * over 4 separate planes) down to these 4 (O(radius) per pixel, one
 * horizontal or vertical sweep at a time). */
bool guidedFilterGPU(Pass &pass, BufferPool &pool, Buffer &guideFull,
                     Buffer &srcFull, Buffer &dstFull, int W, int H, int r,
                     float epsilon)
{
    const int subsampling = calcSubsampling(W, H, r);
    const int w = std::max(1, W / subsampling);
    const int h = std::max(1, H / subsampling);
    const float r1 = float(r) / float(subsampling);
    const int radius = clampBoxRadius(r1, w, h);
    const size_t subBytes = (size_t)w * h * sizeof(float);

    /* I1/p1 hold the subsampled guide/src until stage 1 consumes them, then
     * are reused to hold stage 2's a,b output.
     *
     * Pure GPU-resident scratch -- never touched from the CPU, so a discrete
     * GPU's DEVICE_LOCAL-only (unmapped) buffer is fine here, and so is a
     * pooled one.  They come from the caller's pool rather than six fresh
     * createBuffer() calls because this runs per mask per channel, and
     * vkAllocateMemory on that path is exactly what BufferPool exists to keep
     * off the hot path (vk_context.h).  The caller must not recycle the pool
     * before submitting and waiting on `pass`. */
    Buffer *bufs[6];
    for (int i = 0; i < 6; ++i) {
        bufs[i] = pool.get(subBytes);
        if (!bufs[i] || !bufs[i]->valid()) {
            logOnce("GPU: guided filter buffer allocation failed; using the "
                    "CPU");
            return false;
        }
    }
    Buffer &I1 = *bufs[0];
    Buffer &p1 = *bufs[1];
    Buffer &hI = *bufs[2];
    Buffer &hp = *bufs[3];
    Buffer &hII = *bufs[4];
    Buffer &hIp = *bufs[5];

    if (!rescale(pass, guideFull, I1, W, H, w, h)) {
        return false;
    }
    if (!rescale(pass, srcFull, p1, W, H, w, h)) {
        return false;
    }

    // Stage 1: horizontal shrinking-window mean of I, p, I^2, I*p.
    if (!gfPass1(pass, I1, p1, hI, hp, hII, hIp, w, h, radius)) {
        return false;
    }

    // Stage 2: vertical mean completes the 2D mean; derive a,b into I1,p1
    if (!gfPass2(pass, hI, hp, hII, hIp, I1, p1, w, h, radius, epsilon)) {
        return false;
    }

    // Stage 3: horizontal mean of a,b (I1,p1 -> ha,hb into hI,hp).
    if (!gfPass3(pass, I1, p1, hI, hp, w, h, radius)) {
        return false;
    }

    // Stage 4: vertical mean completes box(a),box(b) (hI,hp -> meanA,meanB
    // into hII,hIp).
    if (!gfPass4(pass, hI, hp, hII, hIp, w, h, radius)) {
        return false;
    }

    // dst = bilinear(meanA)*guideFull + bilinear(meanB)
    RescalePC cpc{(unsigned)w, (unsigned)h, (unsigned)W, (unsigned)H};
    std::vector<Pass::Binding> cb;
    cb.push_back(Pass::Binding(&hII, false));
    cb.push_back(Pass::Binding(&hIp, false));
    cb.push_back(Pass::Binding(&guideFull, false));
    cb.push_back(Pass::Binding(&dstFull, true));
    return pass.dispatch2D("mask_guided_combine", cb, &cpc, sizeof(cpc), W, H);
}

bool guidedFilterGPU(Context &ctx, Buffer &guideFull, Buffer &srcFull,
                     Buffer &dstFull, int W, int H, int r, float epsilon)
{
    BufferPool pool(ctx, HostMemoryMode::PREFER_DEVICE_LOCAL);
    Pass pass(ctx, "generateMasks:guidedFilter");
    if (!pass.valid() ||
        !guidedFilterGPU(pass, pool, guideFull, srcFull, dstFull, W, H, r,
                         epsilon) ||
        !pass.submitAndWait()) {
        return false;
    }
    pass.reportTimings();
    return true;
}

} // namespace ops
} // namespace gpu
} // namespace rtengine

#endif // ART_USE_VULKAN
