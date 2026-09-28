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

#include "ops.h"
#include "../guidedfilter.h"

#include <string>
#include <vector>

namespace rtengine { namespace gpu { namespace ops {

namespace {

struct LogPC { unsigned int w, h, inverse; float base; };
struct RgbLuminancePC { unsigned int w, h; float ws1[3]; };
struct YuvRecombinePC { unsigned int w, h; int bump; float ws1[3]; };
struct Rgb2YuvPC { unsigned int w, h; float ws1[3]; };
struct Yuv2RgbPC { unsigned int w, h; float ws1[3]; };
struct RescalePC { unsigned int ws, hs, wd, hd; };

/* Wraps a single recording helper back up as its own submission, for the
 * Context & forms below.  Every one of them used to be written out by hand,
 * identically. */
template <class F>
bool submitOne(Context &ctx, const char *label, F record)
{
    Pass pass(ctx, label);
    if (!pass.valid() || !record(pass) || !pass.submitAndWait()) {
        return false;
    }
    pass.reportTimings();
    return true;
}

} // namespace

//-----------------------------------------------------------------------------
// Recording forms
//-----------------------------------------------------------------------------

bool logTransform(Pass &pass, Buffer &src, int W, int H, bool inverse,
                  float base, Buffer &out)
{
    LogPC pc{(unsigned)W, (unsigned)H, inverse ? 1u : 0u, base};
    std::vector<Pass::Binding> b;
    b.push_back(Pass::Binding(&src, false));
    b.push_back(Pass::Binding(&out, true));
    return pass.dispatch2D("log_lin", b, &pc, sizeof(pc), W, H);
}

/* guidedFilterLog(10.f, chan, r, eps): chan is both guide and source. */
bool logGuidedFilterSelf(Pass &pass, BufferPool &pool, Buffer &chan, int W,
                         int H, int r, float epsilon)
{
    return logTransform(pass, chan, W, H, false, 10.f, chan) &&
           guidedFilterGPU(pass, pool, chan, chan, chan, W, H, r, epsilon) &&
           logTransform(pass, chan, W, H, true, 10.f, chan);
}

/* guidedFilterLog(10.f, guide, chan, r, eps): a separate, already-log-space
 * guide. */
bool logGuidedFilterWithGuide(Pass &pass, BufferPool &pool, Buffer &guide,
                              Buffer &chan, int W, int H, int r, float epsilon)
{
    return logTransform(pass, chan, W, H, false, 10.f, chan) &&
           guidedFilterGPU(pass, pool, guide, chan, chan, W, H, r, epsilon) &&
           logTransform(pass, chan, W, H, true, 10.f, chan);
}

bool rgbLuminance(Pass &pass, Buffer &rC, Buffer &gC, Buffer &bC, int W, int H,
                  const TMatrix &ws, Buffer &yOut)
{
    RgbLuminancePC pc{unsigned(W), unsigned(H), {ws[1][0], ws[1][1], ws[1][2]}};
    std::vector<Pass::Binding> b;
    b.push_back(Pass::Binding(&rC, false));
    b.push_back(Pass::Binding(&gC, false));
    b.push_back(Pass::Binding(&bC, false));
    b.push_back(Pass::Binding(&yOut, true));
    return pass.dispatch2D("rgb_luminance", b, &pc, sizeof(pc), W, H);
}

bool yuvRecombine(Pass &pass, Buffer &targetY, Buffer &chR, Buffer &chG,
                  Buffer &chB, int W, int H, bool bump_ch, const TMatrix &ws,
                  Buffer &outR, Buffer &outG, Buffer &outB)
{
    YuvRecombinePC pc{unsigned(W), unsigned(H), bump_ch,
                      {ws[1][0], ws[1][1], ws[1][2]}};
    std::vector<Pass::Binding> b;
    b.push_back(Pass::Binding(&chR, false));
    b.push_back(Pass::Binding(&chG, false));
    b.push_back(Pass::Binding(&chB, false));
    b.push_back(Pass::Binding(&targetY, false));
    b.push_back(Pass::Binding(&outR, true));
    b.push_back(Pass::Binding(&outG, true));
    b.push_back(Pass::Binding(&outB, true));
    return pass.dispatch2D("yuv_recombine", b, &pc, sizeof(pc), W, H);
}

bool rgb2yuv(Pass &pass, Buffer &R, Buffer &G, Buffer &B, int W, int H,
             const TMatrix &ws, Buffer &outY, Buffer &outU, Buffer &outV)
{
    Rgb2YuvPC pc{(unsigned)W, (unsigned)H, {ws[1][0], ws[1][1], ws[1][2]}};
    std::vector<Pass::Binding> b;
    b.push_back(Pass::Binding(&R, false));
    b.push_back(Pass::Binding(&G, false));
    b.push_back(Pass::Binding(&B, false));
    b.push_back(Pass::Binding(&outY, true));
    b.push_back(Pass::Binding(&outU, true));
    b.push_back(Pass::Binding(&outV, true));
    return pass.dispatch2D("rgb2yuv", b, &pc, sizeof(pc), W, H);
}

bool yuv2rgb(Pass &pass, Buffer &Y, Buffer &U, Buffer &V, int W, int H,
             const TMatrix &ws, Buffer &outR, Buffer &outG, Buffer &outB)
{
    Yuv2RgbPC pc{(unsigned)W, (unsigned)H, {ws[1][0], ws[1][1], ws[1][2]}};
    std::vector<Pass::Binding> b;
    b.push_back(Pass::Binding(&Y, false));
    b.push_back(Pass::Binding(&U, false));
    b.push_back(Pass::Binding(&V, false));
    b.push_back(Pass::Binding(&outR, true));
    b.push_back(Pass::Binding(&outG, true));
    b.push_back(Pass::Binding(&outB, true));
    return pass.dispatch2D("yuv2rgb", b, &pc, sizeof(pc), W, H);
}

bool rescaleBilinear(Pass &pass, Buffer &src, int ws, int hs, Buffer &dst,
                     int wd, int hd)
{
    RescalePC pc{(unsigned)ws, (unsigned)hs, (unsigned)wd, (unsigned)hd};
    std::vector<Pass::Binding> b;
    b.push_back(Pass::Binding(&src, false));
    b.push_back(Pass::Binding(&dst, true));
    return pass.dispatch2D("mask_rescale_bilinear", b, &pc, sizeof(pc), wd, hd);
}

bool transferPlane(Pass &pass, Buffer &packed, Buffer &strided,
                   size_t stridedByteOffset, size_t stridedByteRange,
                   int W, int H, int strideFloats,
                   bool toStrided, float scaling)
{
    struct PlaneTransferPC {
        unsigned int w, h, stride, toStrided;
        float scaling;
    };
    PlaneTransferPC pc{(unsigned)W, (unsigned)H, (unsigned)strideFloats,
                        toStrided ? 1u : 0u, scaling};
    Pass::Binding packedBinding(&packed, !toStrided);
    Pass::Binding stridedBinding(&strided, toStrided);
    stridedBinding.offset = stridedByteOffset;
    stridedBinding.range = stridedByteRange;
    std::vector<Pass::Binding> b;
    b.push_back(packedBinding);
    b.push_back(stridedBinding);
    return pass.dispatch2D("transfer_plane", b, &pc, sizeof(pc), W, H);
}

//-----------------------------------------------------------------------------
// Submitting forms -- one queue round trip each, so do not chain them
//-----------------------------------------------------------------------------

bool logTransform(Context &ctx, const std::string &label, Buffer &src,
                  int W, int H, bool inverse, float base, Buffer &out)
{
    return submitOne(ctx, label.c_str(), [&](Pass &p) {
        return logTransform(p, src, W, H, inverse, base, out);
    });
}

bool logGuidedFilterSelf(Context &ctx, const std::string &labelPrefix,
                         Buffer &chan, int W, int H, int r, float epsilon)
{
    BufferPool pool(ctx, HostMemoryMode::PREFER_DEVICE_LOCAL);
    return submitOne(ctx, labelPrefix.c_str(), [&](Pass &p) {
        return logGuidedFilterSelf(p, pool, chan, W, H, r, epsilon);
    });
}

bool logGuidedFilterWithGuide(Context &ctx, const std::string &labelPrefix,
                              Buffer &guide, Buffer &chan, int W, int H, int r,
                              float epsilon)
{
    BufferPool pool(ctx, HostMemoryMode::PREFER_DEVICE_LOCAL);
    return submitOne(ctx, labelPrefix.c_str(), [&](Pass &p) {
        return logGuidedFilterWithGuide(p, pool, guide, chan, W, H, r, epsilon);
    });
}

bool rgbLuminance(Context &ctx, Buffer &rC, Buffer &gC, Buffer &bC, int W, int H,
                  const TMatrix &ws, Buffer &yOut)
{
    return submitOne(ctx, "rgbLuminance", [&](Pass &p) {
        return rgbLuminance(p, rC, gC, bC, W, H, ws, yOut);
    });
}

bool yuvRecombine(Context &ctx, Buffer &targetY,
                  Buffer &chR, Buffer &chG, Buffer &chB,
                  int W, int H, bool bump_ch, const TMatrix &ws,
                  Buffer &outR, Buffer &outG, Buffer &outB)
{
    return submitOne(ctx, "yuvRecombine", [&](Pass &p) {
        return yuvRecombine(p, targetY, chR, chG, chB, W, H, bump_ch, ws, outR,
                            outG, outB);
    });
}

bool rgb2yuv(Context &ctx, Buffer &R, Buffer &G, Buffer &B, int W, int H,
             const TMatrix &ws, Buffer &outY, Buffer &outU, Buffer &outV)
{
    return submitOne(ctx, "rgb2yuv", [&](Pass &p) {
        return rgb2yuv(p, R, G, B, W, H, ws, outY, outU, outV);
    });
}

bool yuv2rgb(Context &ctx, Buffer &Y, Buffer &U, Buffer &V, int W, int H,
             const TMatrix &ws, Buffer &outR, Buffer &outG, Buffer &outB)
{
    return submitOne(ctx, "yuv2rgb", [&](Pass &p) {
        return yuv2rgb(p, Y, U, V, W, H, ws, outR, outG, outB);
    });
}

bool rescaleBilinear(Context &ctx, const char *label, Buffer &src,
                     int ws, int hs, Buffer &dst, int wd, int hd)
{
    return submitOne(ctx, label, [&](Pass &p) {
        return rescaleBilinear(p, src, ws, hs, dst, wd, hd);
    });
}

}}} // namespace rtengine::gpu::ops
