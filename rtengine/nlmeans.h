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

#pragma once

#include "array2D.h"
#include "gpu/gpu.h"

#ifdef ART_USE_VULKAN
#include "gpu/vk_pass.h"
#endif // ART_USE_VULKAN

namespace rtengine {

namespace denoise {

enum class BlurType { OFF, BOX, GAUSS };
void detail_mask(const array2D<float> &src, array2D<float> &mask, float scaling,
                 float threshold, float ceiling, float factor, BlurType blur,
                 float blur_radius, bool multithread);

void NLMeans(array2D<float> &img, float normcoeff, int strength,
             int detail_thresh, float scale, bool multithread);

} // namespace denoise

#ifdef ART_USE_VULKAN
namespace gpu {
class Context;
class Buffer;
class BufferPool;
namespace ops {

bool detailMask(Context &ctx, Buffer &maskOut, Buffer &src, int W, int H,
                float scaling, float threshold, float ceiling, float factor,
                float blurSigma, BufferPool *pool);

bool NLMeans(Context &ctx, BufferPool &pool, Buffer &plane, int W,
             int H, float normcoeff, double scale, int strength,
             int detail_thresh);

} // namespace ops
} // namespace gpu
#endif // ART_USE_VULKAN

} // namespace rtengine
