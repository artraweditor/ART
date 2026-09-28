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

// Upload/download of one W x H float plane to/from a device buffer; see doc/gpu_pipeline.md §2.6.
#pragma once

#ifdef ART_USE_VULKAN

#include "vk_context.h"
#include "vk_pass.h"

#include <cstring>
#include <vector>

namespace rtengine {
namespace gpu {

/* Copies into an already-allocated buffer -- use this, not uploadPlane()
 * below, when `out` came from a pool (uploadPlane() replaces it with a
 * fresh buffer, silently defeating the pool). */
inline bool uploadPlaneInto(Context &ctx, float **src, int W, int H, Buffer &out,
                            BufferPool *staging_pool = nullptr)
{
    const size_t bytes = (size_t)W * H * sizeof(float);
    if (!out.valid() || out.size() < bytes) {
        return false;
    }
    if (out.mapped()) {
        float *m = (float *)out.mapped();
        for (int y = 0; y < H; ++y) {
            std::memcpy(m + (size_t)y * W, src[y], (size_t)W * sizeof(float));
        }
        out.flush(0, bytes);
        return true;
    }
    // out isn't mapped: pack rows into one contiguous host buffer, then move it on-device
    std::vector<float> packed((size_t)W * H);
    for (int y = 0; y < H; ++y) {
        std::memcpy(&packed[(size_t)y * W], src[y], (size_t)W * sizeof(float));
    }
    return uploadToBuffer(ctx, staging_pool, packed.data(), bytes, out);
}

inline bool uploadPlane(Context &ctx, float **src, int W, int H, Buffer &out,
                        BufferPool *staging_pool = nullptr)
{
    const size_t bytes = (size_t)W * H * sizeof(float);
    out = ctx.createBuffer(bytes, HostMemoryMode::PREFER_DEVICE_LOCAL);
    if (!out.valid()) {
        return false;
    }
    return uploadPlaneInto(ctx, src, W, H, out, staging_pool);
}

inline bool downloadPlane(Context &ctx, Buffer &buf, int W, int H, float **dst,
                          BufferPool *staging_pool = nullptr)
{
    const size_t bytes = (size_t)W * H * sizeof(float);
    if (!buf.valid() || buf.size() < bytes) {
        return false;
    }
    if (buf.mapped()) {
        buf.invalidate(0, bytes);
        const float *m = (const float *)buf.mapped();
        for (int y = 0; y < H; ++y) {
            std::memcpy(dst[y], m + (size_t)y * W, (size_t)W * sizeof(float));
        }
        return true;
    }
    std::vector<float> packed((size_t)W * H);
    if (!downloadFromBuffer(ctx, staging_pool, buf, 0, packed.data(), bytes)) {
        return false;
    }
    for (int y = 0; y < H; ++y) {
        std::memcpy(dst[y], &packed[(size_t)y * W], (size_t)W * sizeof(float));
    }
    return true;
}


} // namespace gpu
} // namespace rtengine

#endif // ART_USE_VULKAN
