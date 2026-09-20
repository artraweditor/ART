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

/*
 * Residency tracking for Imagefloat -- see doc/gpu_pipeline.md §2.5.
 *
 * C++11 and Vulkan-free so imagefloat.h can include it; degrades to no-ops
 * when the backend is compiled out.
 */
#ifndef ART_GPU_RESIDENCY_H
#define ART_GPU_RESIDENCY_H

#include <cstddef>
#include "../noncopyable.h"

namespace rtengine {

class Imagefloat;

namespace gpu {
class Buffer;
}

class ImageResidency {
public:
    enum class Loc { CPU_ONLY, GPU_ONLY, BOTH };

    explicit ImageResidency(Imagefloat *owner);
    ~ImageResidency();

    Loc where() const { return loc_; }
    bool onGPU() const { return loc_ == Loc::GPU_ONLY; }

    /* Null return means no device / allocation failed / image too large:
     * caller must use its CPU path.
     *
     * forRead          contents valid, residency unchanged
     * forWrite         contents valid, marks GPU_ONLY (caller will modify)
     * forDiscardWrite  contents NOT uploaded, marks GPU_ONLY (caller
     *                  overwrites every pixel it cares about)
     */
    gpu::Buffer *forRead();
    gpu::Buffer *forWrite();
    gpu::Buffer *forDiscardWrite();

    void syncToCpu();
    void invalidateGPU();

    /* GPU-side equivalent of Imagefloat::copyData; false means the caller
     * must do the host copy instead. */
    bool copyTo(ImageResidency &dst);

    size_t deviceBytes() const;

private:
    ImageResidency(const ImageResidency &);
    ImageResidency &operator=(const ImageResidency &);

    /* PlanarRGBData::allocate can resize an Imagefloat in place, so geometry
     * is re-checked here rather than trusted. */
    bool ensureBuffer();
    /* True when buf_'s size no longer matches the owner's (reallocated
     * without going through ensureBuffer() first). Paths that bypass
     * ensureBuffer() -- syncToCpu/download, copyTo's source side -- must
     * check this themselves. */
    bool staleBuffer() const;
    /* Callers must not update loc_ on a false return: the buffer's contents
     * are untouched/stale, not just "not yet transferred". */
    bool upload();
    bool download();
    void poisonCpuPlanes();
    void trace(const char *what);
    /* Lazily allocates staging_buf_; a no-op (staging_buf_ stays null) when
     * buf_ is already mapped. */
    bool ensureStagingBuffer();

    Imagefloat *owner_;
    gpu::Buffer *buf_;      // owned; raw so this header needs no <memory>
    gpu::Buffer *staging_buf_; // owned; lazily created, see ensureStagingBuffer()
    int buf_w_;
    int buf_h_;
    Loc loc_;
};


class ResidencyGuard: public NonCopyable {
public:
    explicit ResidencyGuard(Imagefloat *img):
        img(img), ok(false)
    {
    }
    ~ResidencyGuard();
    void success() { ok = true; }
    Imagefloat *img;
    bool ok;
};


} // namespace rtengine

#endif // ART_GPU_RESIDENCY_H
