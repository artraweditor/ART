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
// extracted and datapted from ImProcFunctions (improcfun.cc, ipdenoise.cc) of
// RawTherapee

#pragma once

#include "curves.h"
#include "gpu/gpu.h"
#include "improcfun.h"

#ifdef ART_USE_VULKAN
#include "gpu/vk_pass.h"
#endif // ART_USE_VULKAN

namespace rtengine {

namespace denoise {

/* How many wavelet levels to decompose to.  Stronger chroma sliders and the
 * aggressive (QUALITY_HIGH) mode ask for more levels; the image's short side
 * and the preview scale cap it.  8 is the hard ceiling (cplx_wavelet_dec.h,
 * now wavelet.h)'s maxlevels). */
enum nrquality { QUALITY_STANDARD, QUALITY_HIGH };

class NoiseCurve {
private:
    LUTf lutNoiseCurve; // 0xffff range
    float sum;
    void Set(const Curve &pCurve);

public:
    virtual ~NoiseCurve() {};
    NoiseCurve();
    void Reset();
    void Set(const std::vector<double> &curvePoints);

    float getSum() const { return sum; }
    float operator[](float index) const { return lutNoiseCurve[index]; }
    operator bool(void) const { return lutNoiseCurve; }

    /* For handing the table to a GPU kernel verbatim -- see LUTf::rawData()'s
     * own doc comment.  Null/0 when Reset() (never happens on the one fixed
     * curve RGB_denoise builds, but a caller should still check). */
    const float *rawData() const { return lutNoiseCurve.rawData(); }
    unsigned int rawSize() const { return lutNoiseCurve.getSize(); }
};


void denoiseGuidedSmoothing(ImProcData &im, Imagefloat *rgb);

void RGB_denoise(ImProcData &im, Imagefloat *src, //Imagefloat *dst,
                 //Imagefloat *calclum,
                 const procparams::DenoiseParams &dnparams);
//                 const NoiseCurve &noiseLCurve, const NoiseCurve &noiseCCurve);

void finalSmoothing(ImProcData &im, Imagefloat *img, 
                    const procparams::DenoiseParams &dnparams);

enum class Median {
    TYPE_3X3_SOFT,
    TYPE_3X3_STRONG,
    TYPE_5X5_SOFT,
    TYPE_5X5_STRONG,
    TYPE_7X7,
    TYPE_9X9
};

void Median_Denoise(float **src, float **dst, float upperBound, int width,
                    int height, Median medianType, int iterations,
                    int numThreads, float **buffer = nullptr);

void Median_Denoise(float **src, float **dst, int width, int height,
                    Median medianType, int iterations, int numThreads,
                    float **buffer = nullptr);

/* ---------------------------------------------------------------------------
 * RGB_denoise's phases
 *
 * RGB_denoise is one long function whose body decomposes into eight phases
 * (the same eight the ART_PROFILE scopes are named for, and the same eight
 * the Vulkan port is organised around):
 *
 *   1 fill        RGB -> L/a/b through the gamma LUTs  [denoiseFill]
 *   2 decompose   wavelet decomposition of L, a, b
 *   3 mad         madL[level][dir] noise estimate from L's detail bands
 *   4 shrink-ab   chroma shrinkage (cross-channel: reads L's coefficients)
 *   5 shrink-l    luma shrinkage
 *   6 reconstruct wavelet reconstruction of L, a, b
 *   7 dct         sliding-window block DCT detail recovery
 *                 [buildDetailMask + detailRecoveryCPU]
 *   8 out         L/a/b -> RGB through the inverse gamma LUT  [denoiseOutput]
 *
 * DenoiseContext carries what the phases share, so each phase reads as one
 * operation on a named input rather than as a slice of a long function.  It is
 * built by denoisePrepare, which fills the wider DenoisePrep around it; the
 * two drivers -- RGB_denoise_CPU and RGB_denoise_GPU -- then sequence the
 * phases, sharing one implementation of each phase's CPU maths.
 * ------------------------------------------------------------------------ */
struct DenoiseContext {
    /* Geometry.  W2 is the width of the quarter-resolution noisevar maps,
     * and is exactly the width of one wavelet level plane -- the GPU port
     * relies on that coincidence. */
    int W, H, W2;
    double scale;

    /* Mode flags. */
    bool lab_mode;
    // bool useNoiseLCurve;
    bool useNoiseCCurve;

    /* Noise strengths.  noisevarL is the flat luma strength used when there
     * is no luma noise curve; noisevarab_r/b are the per-channel chroma
     * strengths derived from the sliders. */
    float noisevarL;
    float noisevarab_r, noisevarab_b;

    /* Working-space matrices, forward and inverse. */
    const float (*wpi)[3];
    const float (*wpi_inverse)[3];

    /* Gamma.  The LUTs cover [0, 65535]; outside that range the phases fall
     * back to the closed form, which is what applyGamma/applyIGamma below
     * encapsulate (they were lambdas inside RGB_denoise). */
    float gam;
    float gamthresh, gamslope;
    float igamthresh, igamslope;
    const LUTf *gamcurve;
    const LUTf *igamcurve;

    /* Quarter-resolution per-pixel noise maps, W2 x ((H+1)/2). */
    float *noisevarlum;
    float *noisevarchrom;

    /* Nested OpenMP thread count for the phases' inner parallel regions,
     * computed once in denoisePrepare from the host's processor count and
     * options.rgbDenoiseThreadLimit. */
    int denoiseNestedLevels;

    float applyGamma(float v) const
    {
        if (gam > 1.f && v > 0.f) {
            return v < 65535.f
                       ? (*gamcurve)[v]
                       : (Color::gammaf(v / 65535.f, gam, gamthresh, gamslope) *
                          65535.f);
        }
        return v;
    }

    /* Note the 65536 here against applyGamma's 65535.  That asymmetry is in
     * the original code and is preserved deliberately: for v in
     * [65535, 65536) the forward transform takes the closed form while the
     * inverse takes the LUT (which clamps internally, so it is a defined if
     * slightly different value).  Not worth "fixing" -- it would change
     * output for no benefit. */
    float applyIGamma(float v) const
    {
        if (gam > 1.f && v > 0.f) {
            return v < 65536.f ? (*igamcurve)[v]
                               : (Color::gammaf(v / 65535.f, 1.f / gam,
                                                igamthresh, igamslope) *
                                  65535.f);
        }
        return v;
    }
};

} // namespace denoise

namespace gpu {
namespace ops {

/* ---------------------------------------------------------------------------
 * Denoise.
 *
 * Unlike every other operator here, denoise is not a single call: it is a
 * sequence of eight phases (see the DenoiseContext comment above), and the
 * phases have to hand device-resident planes to each other rather than each
 * uploading and downloading its own copy.  DenoiseSession owns those buffers
 * for the lifetime of one RGB_denoise_GPU call.
 *
 * Ownership model, and why it is a class rather than the usual free function:
 * a phase running on the GPU leaves its output on the device, and the next
 * phase may decline it -- over its memory budget, or a geometry it cannot
 * take.  Every boundary therefore has to be
 * crossable in both directions cheaply, which on this backend it is --
 * unified memory makes upload and download plain memcpy.  syncToHost() /
 * syncToDevice() are those crossings, and each phase makes one itself when it
 * has to hand off to the CPU.
 *
 * Pimpl because ops.h is deliberately Vulkan-free.  A default-constructed
 * session is inert; init() returning false means "no GPU here", and the
 * caller runs every phase on the CPU exactly as before. */

/* Phases 2-6: denoiseWavelet -- decompose L/a/b, estimate the noise level
 * from L, shrink chroma then luma, and reconstruct.  The whole wavelet core
 * in one call, because its phases have to hand device-resident band sets to
 * each other: madL comes from L's *original* detail coefficients and is read
 * by all three shrinks, so L's bands have to survive both chroma channels,
 * and splitting the phases at the public boundary would mean downloading and
 * re-uploading them.
 *
 * `L`, `a` and `b` are LabImage row pointers, read and written in place.
 * Note that L is only written when `denoiseLuminance` -- the CPU decomposes
 * it either way (madL needs it) but reconstructs it only in that case.
 *
 * The caller keeps phase 7's Lin snapshot: it is the L plane as it enters
 * this call, since the CPU's own snapshot is taken after the shrink but
 * before the reconstruction, and the shrink works on the decomposition
 * rather than on the plane.  So copy L before calling, not after.
 *
 * Returns false having changed nothing if the GPU cannot take it, and the
 * caller runs the CPU path unchanged. */
struct DenoiseWaveletGPU {
    DenoiseWaveletGPU():
        levels(0), scale(1.0), aggressive(false), autoch(false),
        useNoiseCCurve(false), denoiseLuminance(true), noisevarab_r(0.f),
        noisevarab_b(0.f), noisevarlum(nullptr), noisevarchrom(nullptr)
    {
    }

    int levels;                  // waveletLevels(), 1..8
    double scale;
    bool aggressive;             // nrQuality == QUALITY_HIGH
    bool autoch;
    bool useNoiseCCurve;
    bool denoiseLuminance;
    float noisevarab_r;          // the a channel's strength
    float noisevarab_b;          // the b channel's
    /* The quarter-resolution noise maps, ((W+1)/2) x ((H+1)/2) -- which is
     * exactly one wavelet level plane, the coincidence the shrink shaders
     * rely on to index every level with the same offset. */
    const float *noisevarlum;
    const float *noisevarchrom;
};

bool denoiseWaveletGPU(int W, int H, float **L, float **a, float **b,
                       const DenoiseWaveletGPU &params, BufferPool *poolp,
                       Context *ctx);

struct DetailRecoveryGPU;

class DenoiseSession {
public:
    DenoiseSession();
    ~DenoiseSession();

    /* The pools waveletCore/detailRecovery allocate their scratch from --
     * owned by the ImProcFunctions driving this call (RGB_denoise sets this
     * right after constructing the session), null when there is none (e.g.
     * a standalone caller with no ImProcFunctions), in which case those two
     * methods decline rather than fall back to an unpooled allocation. */
    gpu::BufferPool *pool = nullptr;

    /* Allocate the device buffers: three W x H planes (L, a, b) and two
     * ((W+1)/2) x ((H+1)/2) noise-variance maps.  false on any failure, and
     * the session stays inert. */
    bool init(int W, int H, Context *ctx);
    bool valid() const;

    /* host -> device and device -> host for the three L/a/b planes.  Row
     * pointers, as LabImage stores them. */
    bool syncToDevice(float **L, float **a, float **b);
    bool syncToHost(float **L, float **a, float **b);

    /* host -> device for the quarter-resolution noise maps, which phase 1
     * fills and phases 4/5 read.  Flat arrays of ((W+1)/2)*((H+1)/2). */
    bool syncNoisevarToDevice(const float *lum, const float *chrom);

    /* device -> host for the same two maps, the reverse of the above.  Needed
     * only when fillNoiseVarMaps below filled them on the device and a later
     * phase then declines and falls back to host code that reads
     * DenoiseContext::noisevarlum/noisevarchrom directly (denoiseWaveletCPU,
     * and the standalone host-plane wavelet entry, which re-uploads them
     * itself from those host arrays).  Cheap -- the maps are a 64th of a
     * plane -- and paid only on that fallback, not on the common path. */
    bool syncNoisevarToHost(float *lum, float *chrom);

    /* Phase 1a/1c fused, entirely on the device: classifies src's own pixels
     * against the chroma noise curve and expands the result straight into the
     * session's noisevarlum/noisevarchrom buffers, with no host round trip
     * for src (reads via residency().forRead(), which uploads only if src is
     * not already device-resident) and none for the maps themselves.
     *
     * False means it declined -- no device, opEnabled("denoise") off, a
     * residency failure, a dispatch failure -- and neither buffer was
     * touched; the caller computes the maps on the host and uploads them via
     * syncNoisevarToDevice as before. */
    bool fillNoiseVarMaps(Imagefloat *src, const denoise::NoiseCurve &curve,
                          const float wpi[3][3], float noisevarL,
                          float maxNoiseVarab, Context *ctx);

    /* What the phases need beyond the session's own geometry.  No gamma
     * tables: every warp denoise applies is a closed form, evaluated directly
     * on the device -- see dn_gamma.comp. */
    struct FillParams {
        bool lab_mode;
        float gam;              // <= 1 disables the gamma warp entirely
        float gamthresh, gamslope;
        float igamthresh, igamslope;
        const float *wp;        // 9 floats, row-major working space
        const float *iwp;       // 9 floats, its inverse
        float boost_a, boost_b; // phase 8 chroma boost factors
    };

    /* Phase 1b: RGB -> L/a/b, leaving the result in the session's device
     * planes.  Uploads srcR/G/B itself.  Returns false having done nothing if
     * the GPU cannot take it, and the caller runs denoiseFill on the CPU. */
    bool fill(float **srcR, float **srcG, float **srcB, const FillParams &p,
             Context *ctx);

    /* Phase 8: the inverse, reading the session's device planes and writing
     * `dst` through its residency buffer.  Nothing is downloaded -- the image
     * is left GPU-resident, so a following ported operator costs no transfer.
     * False when the geometry does not match, so the caller can fall back to
     * denoiseOutput() on the host. */
    bool output(Imagefloat *dst, const FillParams &p, Context *ctx);

    /* Phases 2-6 and phase 7 on the session's own planes, with no crossing. */
    bool waveletCore(const DenoiseWaveletGPU &p, Context *ctx);
    bool detailRecovery(const DetailRecoveryGPU &p, Context *ctx);
    bool syncLinToHost(float **Lin);

private:
    DenoiseSession(const DenoiseSession &);
    DenoiseSession &operator=(const DenoiseSession &);

    struct Impl;
    Impl *p_;
};

/* Phase 7: denoise::detailRecoveryCPU, the sliding-window
 * block DCT that puts back detail the wavelet shrink removed. */
struct DetailRecoveryGPU {
    DetailRecoveryGPU():
        params_Ldetail(0.f), detail_thresh(0), mask(nullptr), scale(1.0)
    {
    }

    float params_Ldetail;   // min(dnparams.luminanceDetail, 99.9f)
    int detail_thresh;      // dnparams.luminanceDetailThreshold
    float **mask;           // W x H, only when detail_thresh > 0
    double scale;
};

bool denoiseDetailRecovery(int W, int H, float **L, float **Lin,
                           const DetailRecoveryGPU &params,
                           BufferPool *poolp, Context *ctx);

// Denoise's automatic-chrominance analysis, for one of the nine sample crops.
struct DenoiseInfoGPU {
    DenoiseInfoGPU():
        levels(0), aggressive(false), gain(1.f), gam(1.f), gamthresh(0.001f),
        gamslope(1.f)
    {
    }

    int levels;             // levwav: max(2, 5 - ceil(log(scale)))
    bool aggressive;        // dnparams.aggressive; ShrinkAll_info's schoice==2
    float gain;             // pow(2, expcomp)
    float gam, gamthresh, gamslope;   // RGB_denoise_infoGamCurve's outputs
    /* workingSpaceMatrix(icm.workingProfile).  The CPU builds this twice, as
     * `wp` for rgb2yuv and `wpi` for rgbxyz, from the same profile. */
    float ws[3][3];
};

/* RGB_denoise_info's outputs, minus the four it writes and no caller reads:
 * `sigma`, `sigma_L`, `redaut` and `blueaut` (ipdenoise.cc:966 declares them,
 * passes them by reference and drops them).  Dropping the first two is what
 * makes the statistics pass a plain reduction -- see dn_info_stats.comp. */
struct DenoiseInfoResult {
    float chaut;
    float maxredaut, maxblueaut, minredaut, minblueaut;
    float chromina, lumema, redyel, skinc, nsknc;
    int Nb;
};

bool denoiseInfoUsable(int W, int H, int levels, bool isRAW, Context *ctx);
bool denoiseInfo(Imagefloat *src, Imagefloat *provicalc,
                 const DenoiseInfoGPU &params, DenoiseInfoResult &out,
                 BufferPool *poolp, Context *ctx);

bool finalSmoothingGPU(ImProcData &im, Imagefloat *rgb, 
                       const procparams::DenoiseParams &dnparams);

} // namespace ops
} // namespace gpu

#ifdef ART_USE_VULKAN
namespace gpu {
class Context;
class Buffer;
class BufferPool;
namespace ops {

/* How much device memory one GPU denoise phase may hold: a quarter of the
 * largest device-local heap (see the definition in ipdenoise.cc for why a
 * quarter and not more), with a floor so a device reporting a small or zero
 * heap does not disqualify itself outright. */
size_t dnMemoryBudget(Context *ctx);

/* Both the wavelet-core and detail-recovery phases decline the same way when
 * they exceed dnMemoryBudget(); this is the one place that logs it, with
 * actual numbers, so it can be diagnosed from the log alone. */
void dnLogOverBudget(const char *phase, size_t want, size_t budget);

/* Hands every buffer back on the way out, on every return path.  Nothing is
 * freed -- that is the point -- but a buffer left checked out would never be
 * handed to the next call, so the pool would grow by a full set per call. */
struct DnPoolCheckout {
    explicit DnPoolCheckout(BufferPool &p): pool(p) {}
    ~DnPoolCheckout() { pool.recycle(); }
    BufferPool &pool;

private:
    DnPoolCheckout(const DnPoolCheckout &);
    DnPoolCheckout &operator=(const DnPoolCheckout &);
};

/* Moved to rtengine/gpu/vk_pass.h, where every op file can reach it -- the
 * per-operator submission cost it exists to amortize is not specific to
 * denoise.  Kept visible here under the name the denoise code already uses. */
using gpu::PassSeq;

} // namespace ops
} // namespace gpu
#endif // ART_USE_VULKAN

} // namespace rtengine
