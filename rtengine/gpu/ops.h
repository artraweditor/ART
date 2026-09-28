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
 */

// Shared multi-dispatch GPU helpers; see doc/gpu_pipeline.md §2.6.
#pragma once

#ifdef ART_USE_VULKAN

#include "vk_pass.h"
#include "../iccstore.h"

#include <string>
#include <vector>

namespace rtengine { namespace gpu { namespace ops {

/* Two forms of each helper.
 *
 * The `Pass &` forms only *record* -- the caller submits.  Prefer them: a
 * chain of these then costs one queue round trip instead of one per kernel,
 * with barriers (which Pass derives) ordering the dispatches inside the single
 * command buffer.  That matters most on a discrete GPU, where every extra
 * submission is a drain of the queue and a blocked CPU thread, but it is
 * measurable everywhere -- see doc/gpu_pipeline.md §2.4 and PassSeq.
 *
 * The `Context &` forms open a Pass, record, submit and wait.  They exist so
 * call sites can migrate one at a time; new code should not use them for more
 * than one helper in a row. */

bool logTransform(Pass &pass, Buffer &src, int W, int H, bool inverse,
                  float base, Buffer &out);

bool logGuidedFilterSelf(Pass &pass, BufferPool &pool, Buffer &chan, int W,
                         int H, int r, float epsilon);

bool logGuidedFilterWithGuide(Pass &pass, BufferPool &pool, Buffer &guide,
                              Buffer &chan, int W, int H, int r,
                              float epsilon);

bool rgbLuminance(Pass &pass, Buffer &rC, Buffer &gC, Buffer &bC, int W, int H,
                  const TMatrix &ws, Buffer &yOut);

bool yuvRecombine(Pass &pass, Buffer &targetY, Buffer &chR, Buffer &chG,
                  Buffer &chB, int W, int H, bool bump_ch, const TMatrix &ws,
                  Buffer &outR, Buffer &outG, Buffer &outB);

bool rgb2yuv(Pass &pass, Buffer &R, Buffer &G, Buffer &B, int W, int H,
             const TMatrix &ws, Buffer &outY, Buffer &outU, Buffer &outV);

bool yuv2rgb(Pass &pass, Buffer &Y, Buffer &U, Buffer &V, int W, int H,
             const TMatrix &ws, Buffer &outR, Buffer &outG, Buffer &outB);

bool rescaleBilinear(Pass &pass, Buffer &src, int ws, int hs, Buffer &dst,
                     int wd, int hd);

bool logTransform(Context &ctx, const std::string &label, Buffer &src,
                  int W, int H, bool inverse, float base, Buffer &out);

/* guidedFilterLog(10.f, chan, r, eps): chan is both guide and source. */
bool logGuidedFilterSelf(Context &ctx, const std::string &labelPrefix,
                         Buffer &chan, int W, int H, int r,
                         float epsilon);

/* guidedFilterLog(10.f, guide, chan, r, eps): a separate, already-log-space
 * guide. */
bool logGuidedFilterWithGuide(Context &ctx,
                              const std::string &labelPrefix,
                              Buffer &guide, Buffer &chan, int W,
                              int H, int r, float epsilon);

/* Bridges one plane of `strided` (residency buffer: rows padded to 16 bytes,
 * planes `plane_stride` floats apart) and `packed`, a tightly-packed W x H
 * scratch plane. */
bool transferPlane(Pass &pass, Buffer &packed, Buffer &strided,
                   size_t stridedByteOffset, size_t stridedByteRange,
                   int W, int H, int strideFloats,
                   bool toStrided, float scaling);

bool rgbLuminance(Context &ctx, Buffer &rC, Buffer &gC, Buffer &bC, int W, int H,
                  const TMatrix &ws, Buffer &yOut);

bool yuvRecombine(Context &ctx, Buffer &targetY,
                  Buffer &chR, Buffer &chG, Buffer &chB,
                  int W, int H, bool bump_ch, const TMatrix &ws,
                  Buffer &outR, Buffer &outG, Buffer &outB);

bool rgb2yuv(Context &ctx, Buffer &R, Buffer &G, Buffer &B, int W, int H,
             const TMatrix &ws, Buffer &outY, Buffer &outU, Buffer &outV);

bool yuv2rgb(Context &ctx, Buffer &Y, Buffer &U, Buffer &V, int W, int H,
             const TMatrix &ws, Buffer &outR, Buffer &outG, Buffer &outB);

bool rescaleBilinear(Context &ctx, const char *label, Buffer &src,
                     int ws, int hs, Buffer &dst, int wd, int hd);

}}} // namespace rtengine::gpu::ops

#endif // ART_USE_VULKAN
