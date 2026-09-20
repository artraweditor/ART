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
 *  The wavelet_decomposition engine below is inherited from RawTherapee:
 *  2010 Ilya Popov <ilia_popov@rambler.ru>
 *  2012 Emil Martinec <ejmartin@uchicago.edu>
 */

#pragma once

#include <cmath>
#include <cstddef>
#include <vector>

#include "array2D.h"
#include "cplx_wavelet_filter_coeffs.h"
#include "cplx_wavelet_level.h"
#include "gpu/gpu.h"
#include "iccstore.h"
#include "noncopyable.h"

#ifdef ART_USE_VULKAN
#include "gpu/vk_pass.h"
#endif // ART_USE_VULKAN

namespace rtengine {

class wavelet_decomposition: public NonCopyable {
public:
    typedef float internal_type;
    float *coeff0;
    // bool memoryAllocationFailed;

private:
    static const int maxlevels =
        10; // should be greater than any conceivable order of decimation

    int lvltot, subsamp;
    // int numThreads;
    int m_w, m_h; // dimensions

    int wavfilt_len, wavfilt_offset;
    float *wavfilt_anal;
    float *wavfilt_synth;

    // wavelet_level<internal_type> * wavelet_decomp[maxlevels];
    std::vector<wavelet_level<internal_type> *> wavelet_decomp;

public:
    template <typename E>
    wavelet_decomposition(E *src, int width, int height, int maxlvl,
                          int subsampling, int skipcrop = 1, int numThreads = 1,
                          int Daub4Len = 6);

    ~wavelet_decomposition();

    internal_type **level_coeffs(int level) const
    {
        return wavelet_decomp[level]->subbands();
    }

    int level_W(int level) const { return wavelet_decomp[level]->width(); }

    int level_H(int level) const { return wavelet_decomp[level]->height(); }

    int level_stride(int level) const
    {
        return wavelet_decomp[level]->stride();
    }

    int maxlevel() const { return lvltot + 1; }

    int subsample() const { return subsamp; }
    template <typename E> void reconstruct(E *dst, const float blend = 1.f);
};

template <typename E>
wavelet_decomposition::wavelet_decomposition(E *src, int width, int height,
                                             int maxlvl, int subsampling,
                                             int skipcrop, int numThreads,
                                             int Daub4Len)
    : coeff0(nullptr),
      // memoryAllocationFailed(false),
      lvltot(0), subsamp(subsampling), /*numThreads(numThreads),*/ m_w(width),
      m_h(height)
{

    // initialize wavelet filters
    wavfilt_len = Daub4Len;
    wavfilt_offset = Daub4_offset;
    wavfilt_anal = new float[2 * wavfilt_len];
    wavfilt_synth = new float[2 * wavfilt_len];

    if (wavfilt_len == 6) {
        for (int n = 0; n < 2; n++) {
            for (int i = 0; i < wavfilt_len; i++) {
                wavfilt_anal[wavfilt_len * (n) + i] = Daub4_anal[n][i];
                wavfilt_synth[wavfilt_len * (n) + i] =
                    Daub4_anal[n][wavfilt_len - 1 - i];
                // n=0 lopass, n=1 hipass
            }
        }
    } else if (wavfilt_len == 8) {
        for (int n = 0; n < 2; n++) {
            for (int i = 0; i < wavfilt_len; i++) {
                wavfilt_anal[wavfilt_len * (n) + i] = Daub4_anal8[n][i];
                wavfilt_synth[wavfilt_len * (n) + i] =
                    Daub4_anal8[n][wavfilt_len - 1 - i];
                // n=0 lopass, n=1 hipass
            }
        }
    } else if (wavfilt_len == 12) {
        for (int n = 0; n < 2; n++) {
            for (int i = 0; i < wavfilt_len; i++) {
                wavfilt_anal[wavfilt_len * (n) + i] = Daub4_anal12[n][i];
                wavfilt_synth[wavfilt_len * (n) + i] =
                    Daub4_anal12[n][wavfilt_len - 1 - i];
                // n=0 lopass, n=1 hipass
            }
        }
    } else if (wavfilt_len == 16) {
        for (int n = 0; n < 2; n++) {
            for (int i = 0; i < wavfilt_len; i++) {
                wavfilt_anal[wavfilt_len * (n) + i] = Daub4_anal16[n][i];
                wavfilt_synth[wavfilt_len * (n) + i] =
                    Daub4_anal16[n][wavfilt_len - 1 - i];
                // n=0 lopass, n=1 hipass
            }
        }
    } else if (wavfilt_len == 4) {
        for (int n = 0; n < 2; n++) {
            for (int i = 0; i < wavfilt_len; i++) {
                wavfilt_anal[wavfilt_len * (n) + i] = Daub4_anal0[n][i];
                wavfilt_synth[wavfilt_len * (n) + i] =
                    Daub4_anal0[n][wavfilt_len - 1 - i];
                // n=0 lopass, n=1 hipass
            }
        }
    }

    // after coefficient rotation, data structure is:
    // wavelet_decomp[scale][channel={lo,hi1,hi2,hi3}][pixel_array]

    lvltot = 0;
    E *buffer[2];
    buffer[0] = new /*(std::nothrow)*/ E[(m_w / 2 + 1) * (m_h / 2 + 1)];

    // if(buffer[0] == nullptr) {
    //     memoryAllocationFailed = true;
    //     return;
    // }

    buffer[1] = new /*(std::nothrow)*/ E[(m_w / 2 + 1) * (m_h / 2 + 1)];

    // if(buffer[1] == nullptr) {
    //     memoryAllocationFailed = true;
    //     delete[] buffer[0];
    //     buffer[0] = nullptr;
    //     return;
    // }

    int bufferindex = 0;
    wavelet_decomp.reserve(maxlevels);

    wavelet_decomp.push_back(new wavelet_level<internal_type>(
        src, buffer[bufferindex ^ 1], lvltot /*level*/, subsamp, m_w, m_h,
        wavfilt_anal, wavfilt_anal, wavfilt_len, wavfilt_offset, skipcrop,
        numThreads));

    // if(wavelet_decomp[lvltot]->memoryAllocationFailed) {
    //     memoryAllocationFailed = true;
    // }

    while (lvltot < maxlvl - 1) {
        lvltot++;
        bufferindex ^= 1;
        wavelet_decomp.push_back(new wavelet_level<internal_type>(
            buffer[bufferindex], buffer[bufferindex ^ 1] /*lopass*/,
            lvltot /*level*/, subsamp, wavelet_decomp[lvltot - 1]->width(),
            wavelet_decomp[lvltot - 1]->height(), wavfilt_anal, wavfilt_anal,
            wavfilt_len, wavfilt_offset, skipcrop, numThreads));

        // if(wavelet_decomp[lvltot]->memoryAllocationFailed) {
        //     memoryAllocationFailed = true;
        // }
    }

    coeff0 = buffer[bufferindex ^ 1];
    delete[] buffer[bufferindex];
}

template <typename E>
void wavelet_decomposition::reconstruct(E *dst, const float blend)
{

    // if(memoryAllocationFailed) {
    //     return;
    // }

    // data structure is wavcoeffs[scale][channel={lo,hi1,hi2,hi3}][pixel_array]

    if (lvltot >= 1) {
        int width = wavelet_decomp[1]->m_w;
        int height = wavelet_decomp[1]->m_h;

        E *tmpHi = new /*(std::nothrow)*/ E[width * height];

        // if(tmpHi == nullptr) {
        //     memoryAllocationFailed = true;
        //     return;
        // }

        for (int lvl = lvltot; lvl > 0; lvl--) {
            E *tmpLo =
                wavelet_decomp[lvl]->wavcoeffs[2]; // we can use this as buffer
            wavelet_decomp[lvl]->reconstruct_level(tmpLo, tmpHi, coeff0, coeff0,
                                                   wavfilt_synth, wavfilt_synth,
                                                   wavfilt_len, wavfilt_offset);
            delete wavelet_decomp[lvl];
            wavelet_decomp[lvl] = nullptr;
        }

        delete[] tmpHi;
    }

    int width = wavelet_decomp[0]->m_w;
    int height = wavelet_decomp[0]->m_h2;
    E *tmpLo;

    // if(wavelet_decomp[0]->bigBlockOfMemoryUsed()) { // bigBlockOfMemoryUsed
    // means that wavcoeffs[2] points to a block of memory big enough to hold
    // the data
    tmpLo = wavelet_decomp[0]->wavcoeffs[2];
    // } else {                                      // allocate new block of
    // memory
    //     tmpLo = new (std::nothrow) E[width * height];

    //     if(tmpLo == nullptr) {
    //         memoryAllocationFailed = true;
    //         return;
    //     }
    // }

    E *tmpHi = new /*(std::nothrow)*/ E[width * height];

    // if(tmpHi == nullptr) {
    //     memoryAllocationFailed = true;

    //     if(!wavelet_decomp[0]->bigBlockOfMemoryUsed()) {
    //         delete[] tmpLo;
    //     }

    //     return;
    // }

    wavelet_decomp[0]->reconstruct_level(tmpLo, tmpHi, coeff0, dst,
                                         wavfilt_synth, wavfilt_synth,
                                         wavfilt_len, wavfilt_offset, blend);

    // if(!wavelet_decomp[0]->bigBlockOfMemoryUsed()) {
    //     delete[] tmpLo;
    // }

    delete[] tmpHi;
    delete wavelet_decomp[0];
    wavelet_decomp[0] = nullptr;
    delete[] coeff0;
    coeff0 = nullptr;
}

#ifdef ART_USE_VULKAN
namespace gpu {
class Context;
class Buffer;
class BufferPool;
namespace ops {

struct WaveletBandsGPU {
    WaveletBandsGPU(): levels(0), w(0), h(0), hi1(nullptr), hi2(nullptr),
                       hi3(nullptr) {}
    int levels, w, h;
    /* Owned by the BufferPool passed to waveletDecompose, not by this
     * struct: pointers rather than values so that the pool can hand the
     * same memory back on the next call, and because Pass::Binding stores a
     * Buffer* whose address has to stay put. */
    Buffer *hi1, *hi2, *hi3;
};

Pass::Binding waveletBandBinding(Buffer &band, int level, int w, int h,
                                bool write);

/* waveletDecompose consumes `src` (read-only) and produces `llOut` (the
 * coarsest level's low-pass plane, `llW x llH`) plus `bandsOut`.
 * waveletReconstruct consumes `ll`/`bands` and writes the final `W x H`
 * result into `dstFull`. Neither call is self-contained across the pair --
 * both record into the caller's shared `pass` without submitting, so the
 * two calls together (plus whatever the caller dispatches between them)
 * are one decompose-shrink-reconstruct unit the caller submits once.
 *
 * `pool` supplies every buffer these two need -- the output bands and `ll`
 * as well as their internal scratch (level-0 FIR intermediates, per-level
 * Haar intermediates, the ping-ponged low-pass planes).  Since dispatch is
 * recorded into the caller's `pass` but not submitted here, the GPU has not
 * read any of them by the time this returns, so none may be recycled before
 * the caller's `pass.submitAndWait()`.  The caller owns `pool`, keeps it
 * alive across that submitAndWait, and gets the reuse by holding it across
 * several calls -- across the three colour channels of one image, say. */
bool waveletDecompose(Context &ctx, Pass &pass, BufferPool &pool, Buffer &src,
                     int W, int H, int levels, WaveletBandsGPU &bandsOut,
                     Buffer *&llOut, int &llW, int &llH);

bool waveletReconstruct(Context &ctx, Pass &pass, BufferPool &pool,
                        WaveletBandsGPU &bands, Buffer *ll, int llW, int llH,
                        Buffer &dstFull, int W, int H);

/* Median absolute deviation of every subband of `bands`, bit-identical to
 * MadRgb (ipdenoise.cc:89) -- same 65536 integer bins, same cumulative
 * search, same interpolation, and the same double-rounded division by 0.6745
 * (art_mad_scale in dn_mad.glsl).  Not the 4096-bucket approximation
 * op_smoothing.cc's waveletShrink uses.
 *
 * `madOut` receives 3*bands.levels floats, laid out direction-major:
 * index dir*levels + level, with dir 0/1/2 for hi1/hi2/hi3.  Values are
 * MadRgb's own return, *not* squared -- denoise's callers square it
 * (madL[lvl][dir-1] = SQR(MadRgb(...)) at ipdenoise.cc:1062), but leaving
 * that to them keeps this directly comparable to MadRgb in a test.
 *
 * The result stays on the device.  That is the structural point rather than
 * a detail: waveletShrink is forced into three submissions because it reads
 * its maxAbs and then its medians back to the host between passes, and a
 * device-side mad buffer that the shrink shaders index by segment removes
 * both round trips.  Four commands per call (a fill plus three dispatches),
 * whatever the level count.
 *
 * Records into the caller's `pass` without submitting, and takes its
 * histogram scratch from `pool`, on the same terms as the two functions
 * above: nothing may be recycled before the caller's submitAndWait.  Needs
 * 3*levels*256 KB of histogram (6.3 MB at 8 levels). */
bool waveletMadExact(Context &ctx, Pass &pass, BufferPool &pool,
                     WaveletBandsGPU &bands, Buffer &madOut);

} // namespace ops
} // namespace gpu
#endif // ART_USE_VULKAN

} // namespace rtengine
