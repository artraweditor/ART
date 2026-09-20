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

#pragma once

#include "array2D.h"

namespace rtengine {

void guidedFilter(const array2D<float> &guide, const array2D<float> &src,
                  array2D<float> &dst, int r, float epsilon, bool multithread,
                  int subsampling = 0);

void guidedFilterLog(float base, array2D<float> &chan, int r, float eps,
                     bool multithread, int subsampling = 0);

void guidedFilterLog(const array2D<float> &guide, float base,
                     array2D<float> &chan, int r, float eps, bool multithread,
                     int subsampling = 0);

#ifdef ART_USE_VULKAN

namespace gpu {

class Context;
class Buffer;
class BufferPool;
class Pass;

namespace ops {

/* The Fast Guided Filter (He/Sun, "Fast Guided Filter", 2015) over one
 * already device-resident W x H plane triple.
 *
 * Recording form: records into `pass` and submits nothing, so a caller can
 * fold the surrounding log transform (ops::logGuidedFilterSelf) and whatever
 * else it needs into the same command buffer.  Scratch comes from `pool`,
 * which must not be recycled before the caller submits and waits. */
bool guidedFilterGPU(Pass &pass, BufferPool &pool, Buffer &guideFull,
                     Buffer &srcFull, Buffer &dstFull, int W, int H, int r,
                     float epsilon);

bool guidedFilterGPU(Context &ctx, Buffer &guideFull, Buffer &srcFull,
                     Buffer &dstFull, int W, int H, int r, float epsilon);

} // namespace ops
} // namespace gpu

#endif // ART_USE_VULKAN

} // namespace rtengine
