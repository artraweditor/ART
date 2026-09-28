/* -*- C++ -*-
 *
 *  This file is part of ART.
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
 *
 *  The wavelet_decomposition destructor below is inherited from RawTherapee:
 *  2010 Ilya Popov <ilia_popov@rambler.ru>
 *  2012 Emil Martinec <ejmartin@uchicago.edu>
 */

#include "wavelet.h"
#include "improcfun.h"
#include "mytime.h"
#include "pipelineprofile.h"
#include "iccstore.h"
#include "rt_algo.h"

#include "../rtgui/threadutils.h"
#include "LUT.h"
#include "array2D.h"
#include "boxblur.h"
#include "gauss.h"
#include "iccmatrices.h"
#include "labimage.h"
#include "opthelper.h"
#include "rt_math.h"
#include "rtengine.h"
#include "sleef.h"

#include <cmath>
#include <cstdlib>

#ifdef _OPENMP
#include <omp.h>
#endif

#ifdef ART_USE_VULKAN
#include "gpu/gpu.h"
#include "gpu/plane_io.h"
#include "gpu/ops.h"
#endif // ART_USE_VULKAN

namespace rtengine {

wavelet_decomposition::~wavelet_decomposition()
{
    // for(int i = 0; i <= lvltot; i++) {
    for (size_t i = 0; i < wavelet_decomp.size(); ++i) {
        if (wavelet_decomp[i] != nullptr) {
            delete wavelet_decomp[i];
        }
    }

    delete[] wavfilt_anal;
    delete[] wavfilt_synth;

    if (coeff0) {
        delete[] coeff0;
    }
}

} // namespace rtengine


#ifdef ART_USE_VULKAN

#include "settings.h"
#include "gpu/gpu.h"
#include "gpu/vk_pass.h"

#include <cstring>
#include <utility>

namespace rtengine {

extern const Settings *settings;

namespace gpu {
namespace ops {

namespace {

/* cplx_wavelet_filter_coeffs.h's Daub4_anal[2][6], and its time-reversed
 * synthesis counterpart (cplx_wavelet_dec.h:100-108:
 * wavfilt_synth[i] = Daub4_anal[n][5-i]). This project's callers never use
 * any other filter length (verified during research), so these are the only
 * taps this primitive needs. */
constexpr float kAnalLo[6] = {0.f,           0.f,          0.34150635f,
                              0.59150635f,   0.15849365f,  -0.091506351f};
constexpr float kAnalHi[6] = {-0.091506351f, -0.15849365f, 0.59150635f,
                              -0.34150635f,  0.f,          0.f};
constexpr float kSynthLo[6] = {-0.091506351f, 0.15849365f, 0.59150635f,
                               0.34150635f,   0.f,         0.f};
constexpr float kSynthHi[6] = {0.f,          0.f,          -0.34150635f,
                               0.59150635f,  -0.15849365f, -0.091506351f};
constexpr int kTaps = 6;
constexpr int kOffset = 2;

struct FirAnalysisVPC {
    unsigned int w, h, h2;
    int skip, offset;
    float filterLo[6], filterHi[6];
};
struct FirAnalysisHPC {
    unsigned int w, h2, w2;
    int skip, offset;
    float filterLo[6], filterHi[6];
};
struct HaarPC { unsigned int w, h; int skip; };
struct FirSynthHPC {
    unsigned int w2, w, h2;
    int skip, shift;
    float filterLo[6], filterHi[6];
};
struct FirSynthVPC {
    unsigned int w, h2, h;
    int skip, shift;
    float filterLo[6], filterHi[6];
};

/* cplx_wavelet_level.h's wavelet_level ctor: skip=1 at level 0; for level
 * L>=1, skip = 2^(L-1) when subsamp==1 (the only value any caller in this
 * project's scope passes) and skipcrop==1. */
int levelSkip(int level)
{
    if (level <= 0) {
        return 1;
    }
    int skip = 1;
    for (int n = 1; n < level; ++n) {
        skip *= 2;
    }
    return skip;
}

bool firAnalysisV(Pass &pass, Buffer &src, int W, int H, int H2, Buffer &lo,
                  Buffer &hi)
{
    FirAnalysisVPC pc{};
    pc.w = (unsigned)W;
    pc.h = (unsigned)H;
    pc.h2 = (unsigned)H2;
    pc.skip = 1;
    pc.offset = kOffset;
    std::memcpy(pc.filterLo, kAnalLo, sizeof(kAnalLo));
    std::memcpy(pc.filterHi, kAnalHi, sizeof(kAnalHi));
    std::vector<Pass::Binding> b;
    b.push_back(Pass::Binding(&src, false));
    b.push_back(Pass::Binding(&lo, true));
    b.push_back(Pass::Binding(&hi, true));
    return pass.dispatch2D("wav_fir_analysis_v", b, &pc, sizeof(pc), W, H2);
}

bool firAnalysisH(Pass &pass, Buffer &src, int W, int H2, int W2,
                  const Pass::Binding &lo, const Pass::Binding &hi)
{
    FirAnalysisHPC pc{};
    pc.w = (unsigned)W;
    pc.h2 = (unsigned)H2;
    pc.w2 = (unsigned)W2;
    pc.skip = 1;
    pc.offset = kOffset;
    std::memcpy(pc.filterLo, kAnalLo, sizeof(kAnalLo));
    std::memcpy(pc.filterHi, kAnalHi, sizeof(kAnalHi));
    std::vector<Pass::Binding> b;
    b.push_back(Pass::Binding(&src, false));
    b.push_back(lo);
    b.push_back(hi);
    return pass.dispatch2D("wav_fir_analysis_h", b, &pc, sizeof(pc), W2, H2);
}

bool haarAnalysisV(Pass &pass, Buffer &src, int W, int H, int skip,
                   Buffer &lo, Buffer &hi)
{
    HaarPC pc{(unsigned)W, (unsigned)H, skip};
    std::vector<Pass::Binding> b;
    b.push_back(Pass::Binding(&src, false));
    b.push_back(Pass::Binding(&lo, true));
    b.push_back(Pass::Binding(&hi, true));
    return pass.dispatch2D("wav_haar_analysis_v", b, &pc, sizeof(pc), W, H);
}

bool haarAnalysisH(Pass &pass, Buffer &src, int W, int H, int skip,
                   const Pass::Binding &lo, const Pass::Binding &hi)
{
    HaarPC pc{(unsigned)W, (unsigned)H, skip};
    std::vector<Pass::Binding> b;
    b.push_back(Pass::Binding(&src, false));
    b.push_back(lo);
    b.push_back(hi);
    return pass.dispatch2D("wav_haar_analysis_h", b, &pc, sizeof(pc), W, H);
}

bool firSynthesisH(Pass &pass, const Pass::Binding &srcLo,
                   const Pass::Binding &srcHi, int W2, int W, int H2,
                   Buffer &dst)
{
    FirSynthHPC pc{};
    pc.w2 = (unsigned)W2;
    pc.w = (unsigned)W;
    pc.h2 = (unsigned)H2;
    pc.skip = 1;
    pc.shift = 1 * (kTaps - kOffset - 1);
    std::memcpy(pc.filterLo, kSynthLo, sizeof(kSynthLo));
    std::memcpy(pc.filterHi, kSynthHi, sizeof(kSynthHi));
    std::vector<Pass::Binding> b;
    b.push_back(srcLo);
    b.push_back(srcHi);
    b.push_back(Pass::Binding(&dst, true));
    return pass.dispatch2D("wav_fir_synthesis_h", b, &pc, sizeof(pc), W, H2);
}

bool firSynthesisV(Pass &pass, Buffer &srcLo, Buffer &srcHi, int W, int H2,
                   int H, Buffer &dst)
{
    FirSynthVPC pc{};
    pc.w = (unsigned)W;
    pc.h2 = (unsigned)H2;
    pc.h = (unsigned)H;
    pc.skip = 1;
    pc.shift = 1 * (kTaps - kOffset - 1);
    std::memcpy(pc.filterLo, kSynthLo, sizeof(kSynthLo));
    std::memcpy(pc.filterHi, kSynthHi, sizeof(kSynthHi));
    std::vector<Pass::Binding> b;
    b.push_back(Pass::Binding(&srcLo, false));
    b.push_back(Pass::Binding(&srcHi, false));
    b.push_back(Pass::Binding(&dst, true));
    return pass.dispatch2D("wav_fir_synthesis_v", b, &pc, sizeof(pc), W, H);
}

bool haarSynthesisH(Pass &pass, const Pass::Binding &srcLo,
                    const Pass::Binding &srcHi, int W, int H, int skip,
                    Buffer &dst)
{
    HaarPC pc{(unsigned)W, (unsigned)H, skip};
    std::vector<Pass::Binding> b;
    b.push_back(srcLo);
    b.push_back(srcHi);
    b.push_back(Pass::Binding(&dst, true));
    return pass.dispatch2D("wav_haar_synthesis_h", b, &pc, sizeof(pc), W, H);
}

bool haarSynthesisV(Pass &pass, Buffer &srcLo, Buffer &srcHi, int W, int H,
                    int skip, Buffer &dst)
{
    HaarPC pc{(unsigned)W, (unsigned)H, skip};
    std::vector<Pass::Binding> b;
    b.push_back(Pass::Binding(&srcLo, false));
    b.push_back(Pass::Binding(&srcHi, false));
    b.push_back(Pass::Binding(&dst, true));
    return pass.dispatch2D("wav_haar_synthesis_v", b, &pc, sizeof(pc), W, H);
}

} // namespace

Pass::Binding waveletBandBinding(Buffer &band, int level, int w, int h,
                                bool write)
{
    Pass::Binding b(&band, write);
    const size_t bytes = (size_t)w * h * sizeof(float);
    b.offset = (size_t)level * bytes;
    b.range = bytes;
    return b;
}

/* Every dispatch below is recorded into the caller's `pass` -- none of them
 * submit -- so a caller that also has its own GPU work to interleave (e.g.
 * op_smoothing.cc's waveletShrink, which needs per-direction MAD-estimation
 * dispatches between decompose and reconstruct) can batch all of it into one
 * submission. levels<=8 in every caller (the UI's wav_levels range is 2-8),
 * so this is always well under Pass's own MAX_DISPATCHES_PER_PASS=64 budget
 * (3*levels dispatches here). */
bool waveletDecompose(Context &ctx, Pass &pass, BufferPool &pool, Buffer &src,
                     int W, int H, int levels, WaveletBandsGPU &bandsOut,
                     Buffer *&llOut, int &llW, int &llH)
{
    (void)ctx; // every allocation goes through the pool now
    if (levels <= 0) {
        return false;
    }

    const int W0 = (W + 1) / 2, H0 = (H + 1) / 2;
    const size_t bytes0 = (size_t)W0 * H0 * sizeof(float);
    const size_t bandBytes = bytes0 * (size_t)levels;

    bandsOut.levels = levels;
    bandsOut.w = W0;
    bandsOut.h = H0;
    bandsOut.hi1 = pool.get(bandBytes);
    bandsOut.hi2 = pool.get(bandBytes);
    bandsOut.hi3 = pool.get(bandBytes);
    if (!bandsOut.hi1 || !bandsOut.hi2 || !bandsOut.hi3) {
        return false;
    }

    // Level 0: real decimated FIR, vertical then horizontal.
    Buffer *tmpLo = pool.get((size_t)W * H0 * sizeof(float));
    Buffer *tmpHi = pool.get((size_t)W * H0 * sizeof(float));
    if (!tmpLo || !tmpHi) {
        return false;
    }
    if (!firAnalysisV(pass, src, W, H, H0, *tmpLo, *tmpHi)) {
        return false;
    }

    Buffer *ll = pool.get(bytes0);
    if (!ll) {
        return false;
    }
    if (!firAnalysisH(pass, *tmpLo, W, H0, W0, Pass::Binding(ll, true),
                      waveletBandBinding(*bandsOut.hi1, 0, W0, H0, true)) ||
        !firAnalysisH(pass, *tmpHi, W, H0, W0,
                      waveletBandBinding(*bandsOut.hi2, 0, W0, H0, true),
                      waveletBandBinding(*bandsOut.hi3, 0, W0, H0, true))) {
        return false;
    }

    /* Levels 1..levels-1: undecimated a-trous Haar, fixed W0 x H0.
     *
     * Every level reuses the same four buffers instead of allocating its
     * own: one aLo/aHi pair for the vertical stage's output, and two `ll`
     * planes ping-ponged so each level reads one and writes the other.  The
     * levels are strictly serial, and Pass::barrierFor already derives the
     * write-after-read barrier from its per-buffer access tracking, so
     * rewriting a buffer a previously recorded dispatch reads is safe -- the
     * GPU cannot run the two out of order.
     *
     * This is what keeps the scratch peak independent of the level count.
     * Allocating fresh (which the earlier version did, precisely because
     * recorded dispatches have not executed yet and every buffer had to
     * outlive submitAndWait) cost 3 buffers per level per channel: ~2 GB for
     * three channels at 8 levels and 24 Mpix, against ~150 MB here. */
    Buffer *llAlt = nullptr, *aLo = nullptr, *aHi = nullptr;
    if (levels > 1) {
        llAlt = pool.get(bytes0);
        aLo = pool.get(bytes0);
        aHi = pool.get(bytes0);
        if (!llAlt || !aLo || !aHi) {
            return false;
        }
    }
    Buffer *cur = ll, *nxt = llAlt;

    for (int level = 1; level < levels; ++level) {
        const int skip = levelSkip(level);
        if (!haarAnalysisV(pass, *cur, W0, H0, skip, *aLo, *aHi)) {
            return false;
        }
        if (!haarAnalysisH(pass, *aLo, W0, H0, skip, Pass::Binding(nxt, true),
                           waveletBandBinding(*bandsOut.hi1, level, W0, H0,
                                             true)) ||
            !haarAnalysisH(pass, *aHi, W0, H0, skip,
                           waveletBandBinding(*bandsOut.hi2, level, W0, H0,
                                             true),
                           waveletBandBinding(*bandsOut.hi3, level, W0, H0,
                                             true))) {
            return false;
        }
        std::swap(cur, nxt);
    }

    /* `cur` is whichever of the two planes the last level wrote.  Both stay
     * checked out of the pool until the caller recycles it. */
    llOut = cur;
    llW = W0;
    llH = H0;
    return true;
}

bool waveletReconstruct(Context &ctx, Pass &pass, BufferPool &pool,
                        WaveletBandsGPU &bands, Buffer *ll, int llW, int llH,
                        Buffer &dstFull, int W, int H)
{
    (void)ctx;
    const int nlevels = bands.levels;
    if (nlevels <= 0 || !ll || !bands.hi1 || !bands.hi2 || !bands.hi3) {
        return false;
    }
    const size_t bytes0 = (size_t)llW * llH * sizeof(float);

    /* Levels nlevels-1 .. 1: a-trous Haar synthesis at W0 x H0, reusing one
     * tmpLo/tmpHi pair and ping-ponging the low-pass plane between the
     * caller's `ll` and one spare -- see the matching comment in
     * waveletDecompose for why reuse is safe inside a single recorded Pass. */
    Buffer *llAlt = nullptr, *tmpLo = nullptr, *tmpHi = nullptr;
    if (nlevels > 1) {
        llAlt = pool.get(bytes0);
        tmpLo = pool.get(bytes0);
        tmpHi = pool.get(bytes0);
        if (!llAlt || !tmpLo || !tmpHi) {
            return false;
        }
    }
    Buffer *cur = ll, *nxt = llAlt;

    for (int level = nlevels - 1; level >= 1; --level) {
        const int skip = levelSkip(level);
        if (!haarSynthesisH(pass,
                            waveletBandBinding(*bands.hi2, level, llW, llH,
                                              false),
                            waveletBandBinding(*bands.hi3, level, llW, llH,
                                              false),
                            llW, llH, skip, *tmpHi) ||
            !haarSynthesisH(pass, Pass::Binding(cur, false),
                            waveletBandBinding(*bands.hi1, level, llW, llH,
                                              false),
                            llW, llH, skip, *tmpLo)) {
            return false;
        }
        if (!haarSynthesisV(pass, *tmpLo, *tmpHi, llW, llH, skip, *nxt)) {
            return false;
        }
        std::swap(cur, nxt);
    }

    // Level 0: FIR synthesis back to full resolution.
    const size_t bytesHalfFull = (size_t)W * llH * sizeof(float);
    Buffer *firLo = pool.get(bytesHalfFull);
    Buffer *firHi = pool.get(bytesHalfFull);
    if (!firLo || !firHi) {
        return false;
    }
    if (!firSynthesisH(pass,
                       waveletBandBinding(*bands.hi2, 0, llW, llH, false),
                       waveletBandBinding(*bands.hi3, 0, llW, llH, false),
                       llW, W, llH, *firHi) ||
        !firSynthesisH(pass, Pass::Binding(cur, false),
                       waveletBandBinding(*bands.hi1, 0, llW, llH, false),
                       llW, W, llH, *firLo)) {
        return false;
    }
    /* dstFull is the caller's buffer; everything else stays checked out of
     * the pool, which the caller must not recycle before its submitAndWait
     * -- dispatch here is recorded, not executed. */
    return firSynthesisV(pass, *firLo, *firHi, W, llH, H, dstFull);
}

bool waveletMadExact(Context &ctx, Pass &pass, BufferPool &pool,
                     WaveletBandsGPU &bands, Buffer &madOut)
{
    (void)ctx;
    const int levels = bands.levels;
    if (levels <= 0 || !madOut.valid()) {
        return false;
    }
    const size_t segments = 3u * (size_t)levels;
    const size_t n = (size_t)bands.w * bands.h;
    if (!n) {
        return false;
    }
    if (madOut.size() < segments * sizeof(float)) {
        return false;
    }

    Buffer *hist = pool.get(segments * 65536u * sizeof(unsigned int));
    Buffer *gsum = pool.get(segments * 256u * sizeof(unsigned int));
    if (!hist || !gsum) {
        return false;
    }

    /* Measured flat from 8 to 256 workgroups per segment and worse past
     * that; 128 is the middle of the plateau. */
    const unsigned int wgPerSeg = 128;

    struct SegPC {
        unsigned int segments;
    } gpc{(unsigned)segments};
    struct FinPC {
        unsigned int n, segments;
    } fpc{(unsigned)n, (unsigned)segments};

    /* The pool may hand back a buffer larger than the histogram needs, so
     * clear only the range in use rather than the whole allocation. */
    const size_t histBytes = segments * 65536u * sizeof(unsigned int);
    if (!pass.fillBuffer(*hist, 0u, 0, histBytes)) {
        return false;
    }

    struct HistPC {
        unsigned int n, levels, wgPerSeg;
    } hpc{(unsigned)n, (unsigned)levels, wgPerSeg};
    std::vector<Pass::Binding> bh;
    bh.push_back(Pass::Binding(bands.hi1, false));
    bh.push_back(Pass::Binding(bands.hi2, false));
    bh.push_back(Pass::Binding(bands.hi3, false));
    bh.push_back(Pass::Binding(hist, true));
    if (!pass.dispatch1D("dn_mad_hist_bands", bh, &hpc, sizeof(hpc),
                        segments * wgPerSeg * 256u)) {
        return false;
    }

    std::vector<Pass::Binding> bg;
    bg.push_back(Pass::Binding(hist, false));
    bg.push_back(Pass::Binding(gsum, true));
    if (!pass.dispatch1D("dn_mad_group", bg, &gpc, sizeof(gpc),
                        segments * 256u)) {
        return false;
    }

    std::vector<Pass::Binding> bf;
    bf.push_back(Pass::Binding(hist, false));
    bf.push_back(Pass::Binding(gsum, false));
    bf.push_back(Pass::Binding(&madOut, true));
    if (!pass.dispatch1D("dn_mad_finalize", bf, &fpc, sizeof(fpc), segments)) {
        return false;
    }

    return true;
}

} // namespace ops
} // namespace gpu
} // namespace rtengine

#endif // ART_USE_VULKAN



