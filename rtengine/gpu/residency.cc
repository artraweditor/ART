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

#include "residency.h"

#include "../imagefloat.h"
#include "gpu.h"

#ifdef ART_USE_VULKAN

#include "../settings.h"
#include "vk_context.h"

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace rtengine {

extern const Settings *settings;

namespace {

/* See doc/gpu_pipeline.md §2.5/§4. On unconditionally in debug builds;
 * opt-in in release since the memset over every device-resident image
 * is not free. */
bool poisonEnabled()
{
    static const bool v = []() {
#ifndef NDEBUG
        return true;
#else
        const char *e = std::getenv("ART_GPU_DEBUG");
        return e && std::strstr(e, "poison") != nullptr;
#endif
    }();
    return v;
}

bool traceEnabled()
{
    static const bool v = []() {
        const char *e = std::getenv("ART_GPU_TRACE");
        return e && *e && std::strcmp(e, "0") != 0;
    }();
    return v;
}

const char *locName(ImageResidency::Loc l)
{
    switch (l) {
    case ImageResidency::Loc::CPU_ONLY: return "CPU";
    case ImageResidency::Loc::GPU_ONLY: return "GPU";
    default: return "BOTH";
    }
}

} // namespace

ImageResidency::ImageResidency(Imagefloat *owner):
    owner_(owner), buf_(nullptr), staging_buf_(nullptr), buf_w_(0), buf_h_(0),
    loc_(Loc::CPU_ONLY)
{
}

ImageResidency::~ImageResidency()
{
    delete buf_;
    delete staging_buf_;
}

size_t ImageResidency::deviceBytes() const
{
    if (!owner_ || owner_->getWidth() <= 0 || owner_->getHeight() <= 0) {
        return 0;
    }
    return 3 * (size_t)owner_->getPlaneStride();
}

void ImageResidency::trace(const char *what)
{
    if (!traceEnabled()) {
        return;
    }
    std::cerr << "GPU trace: " << what << " " << owner_->getWidth() << "x"
              << owner_->getHeight() << " -> " << locName(loc_) << std::endl;
}

bool ImageResidency::ensureBuffer()
{
    if (!owner_) {
        return false;
    }
    const int W = owner_->getWidth();
    const int H = owner_->getHeight();
    if (W <= 0 || H <= 0) {
        return false;
    }

    gpu::Context *ctx = gpu::Context::get();
    if (!ctx || ctx->deviceLost()) {
        return false;
    }

    if (buf_ && (buf_w_ != W || buf_h_ != H)) {
        delete buf_;
        buf_ = nullptr;
        delete staging_buf_;
        staging_buf_ = nullptr;
        loc_ = Loc::CPU_ONLY;
    }
    if (buf_) {
        return true;
    }

    const size_t bytes = deviceBytes();
    if (!bytes) {
        return false;
    }
    if (bytes > ctx->caps().max_storage_buffer_range) {
        gpu::logOnce("GPU: image exceeds maxStorageBufferRange; needs tiling");
        return false;
    }

    /* Requires the three planes to be contiguous in one allocation (checked
     * once per buffer, not trusted) so a transfer is a single memcpy. */
    const size_t plane_floats = (size_t)owner_->getPlaneStride() / sizeof(float);
    const float *base = owner_->r.ptrs[0];
    if (owner_->g.ptrs[0] != base + plane_floats ||
        owner_->b.ptrs[0] != base + 2 * plane_floats) {
        gpu::logOnce("GPU: Imagefloat planes are not contiguous as expected; "
                     "GPU path disabled");
        return false;
    }

    gpu::Buffer b = ctx->createBuffer(bytes, gpu::HostMemoryMode::PREFER_DEVICE_LOCAL);
    if (!b.valid()) {
        gpu::logOnce("GPU: buffer allocation failed; using the CPU");
        return false;
    }
    /* b.mapped() may be null here (discrete GPU); upload()/download() stage
     * through ensureStagingBuffer() in that case. */
    buf_ = new gpu::Buffer(std::move(b));
    buf_w_ = W;
    buf_h_ = H;
    loc_ = Loc::CPU_ONLY;
    return true;
}

bool ImageResidency::ensureStagingBuffer()
{
    if (buf_->hostVisible()) {
        return true;    // no staging needed
    }
    if (staging_buf_) {
        return true;
    }
    gpu::Context *ctx = gpu::Context::get();
    if (!ctx || ctx->deviceLost()) {
        return false;
    }
    gpu::Buffer b = ctx->createBuffer(deviceBytes(), gpu::HostMemoryMode::STAGING_ONLY);
    if (!b.valid() || !b.mapped()) {
        gpu::logOnce("GPU: no host-visible memory available for staging; "
                     "using the CPU");
        return false;
    }
    staging_buf_ = new gpu::Buffer(std::move(b));
    return true;
}

bool ImageResidency::staleBuffer() const
{
    return !owner_ || !buf_ || buf_w_ != owner_->getWidth() ||
           buf_h_ != owner_->getHeight();
}

bool ImageResidency::upload()
{
    const size_t bytes = deviceBytes();
    if (buf_->hostVisible()) {
        std::memcpy(buf_->mapped(), owner_->r.ptrs[0], bytes);
        buf_->flush(0, bytes);
        return true;
    }
    gpu::Context *ctx = gpu::Context::get();
    if (!ctx || !ensureStagingBuffer()) {
        return false;
    }
    std::memcpy(staging_buf_->mapped(), owner_->r.ptrs[0], bytes);
    staging_buf_->flush(0, bytes);
    return gpu::copyBufferToBuffer(*ctx, *staging_buf_, 0, *buf_, 0, bytes);
}

bool ImageResidency::download()
{
    const size_t bytes = deviceBytes();
    if (buf_->hostVisible()) {
        buf_->invalidate(0, bytes);
        std::memcpy(owner_->r.ptrs[0], buf_->mapped(), bytes);
        return true;
    }
    gpu::Context *ctx = gpu::Context::get();
    if (!ctx || !ensureStagingBuffer() ||
        !gpu::copyBufferToBuffer(*ctx, *buf_, 0, *staging_buf_, 0, bytes)) {
        return false;
    }
    staging_buf_->invalidate(0, bytes);
    std::memcpy(owner_->r.ptrs[0], staging_buf_->mapped(), bytes);
    return true;
}

void ImageResidency::poisonCpuPlanes()
{
    if (!poisonEnabled() || !owner_) {
        return;
    }
    const unsigned int pattern = 0x7FC0DEADu; // quiet-NaN, recognisable in a debugger
    float *p = owner_->r.ptrs[0];
    const size_t n = deviceBytes() / sizeof(float);
    for (size_t i = 0; i < n; ++i) {
        std::memcpy(p + i, &pattern, sizeof(pattern));
    }
}

gpu::Buffer *ImageResidency::forRead()
{
    if (!ensureBuffer()) {
        return nullptr;
    }
    if (loc_ == Loc::CPU_ONLY) {
        if (!upload()) {
            return nullptr;
        }
        loc_ = Loc::BOTH;
        trace("upload");
    }
    return buf_;
}

gpu::Buffer *ImageResidency::forWrite()
{
    if (!forRead()) {
        return nullptr;
    }
    loc_ = Loc::GPU_ONLY;
    poisonCpuPlanes();
    trace("forWrite");
    return buf_;
}

gpu::Buffer *ImageResidency::forDiscardWrite()
{
    if (!ensureBuffer()) {
        return nullptr;
    }
    loc_ = Loc::GPU_ONLY;
    poisonCpuPlanes();
    trace("forDiscardWrite");
    return buf_;
}

void ImageResidency::syncToCpu()
{
    if (loc_ != Loc::GPU_ONLY || !buf_) {
        return;
    }
    if (staleBuffer()) {
        /* Owner resized while GPU_ONLY: no correct pixels to recover, so
         * abandon buf_ rather than memcpy the wrong byte count. */
        gpu::logOnce("GPU: image resized while GPU-resident; discarding "
                     "stale device buffer");
        loc_ = Loc::CPU_ONLY;
        return;
    }
    /* Leave loc_ as GPU_ONLY on failure: CPU planes may still carry the
     * poison pattern, so BOTH would hand a caller garbage. */
    if (download()) {
        loc_ = Loc::BOTH;
        trace("download");
    }
}

void ImageResidency::invalidateGPU()
{
    if (loc_ == Loc::CPU_ONLY) {
        return;
    }
    loc_ = Loc::CPU_ONLY; // buf_ is kept for reuse, not freed
    trace("invalidateGPU");
}

bool ImageResidency::copyTo(ImageResidency &dst)
{
    if (loc_ != Loc::GPU_ONLY || !buf_ || !owner_ || !dst.owner_) {
        return false;
    }
    if (staleBuffer()) {
        return false; // same stale-geometry hazard as syncToCpu()
    }
    if (owner_->getWidth() != dst.owner_->getWidth() ||
        owner_->getHeight() != dst.owner_->getHeight()) {
        return false;
    }
    if (!dst.ensureBuffer()) {
        return false;
    }

    const size_t bytes = deviceBytes();
    if (dst.buf_->size() < bytes) {
        return false;
    }

    bool ok;
    if (buf_->hostVisible() && dst.buf_->hostVisible()) {
        buf_->invalidate(0, bytes);
        std::memcpy(dst.buf_->mapped(), buf_->mapped(), bytes);
        dst.buf_->flush(0, bytes);
        ok = true;
    } else {
        gpu::Context *ctx = gpu::Context::get();
        ok = ctx && gpu::copyBufferToBuffer(*ctx, *buf_, 0, *dst.buf_, 0, bytes);
    }
    if (!ok) {
        return false;
    }

    dst.loc_ = Loc::GPU_ONLY;
    dst.poisonCpuPlanes();
    dst.trace("copyTo");
    return true;
}


ResidencyGuard::~ResidencyGuard()
{
    if (!ok) {
        img->residency().invalidateGPU();
    }
}

} // namespace rtengine

#else // !ART_USE_VULKAN

namespace rtengine {

ImageResidency::ImageResidency(Imagefloat *owner):
    owner_(owner), buf_(nullptr), staging_buf_(nullptr), buf_w_(0), buf_h_(0),
    loc_(Loc::CPU_ONLY)
{
}

ImageResidency::~ImageResidency() {}

size_t ImageResidency::deviceBytes() const { return 0; }
bool ImageResidency::ensureBuffer() { return false; }
bool ImageResidency::ensureStagingBuffer() { return false; }
bool ImageResidency::staleBuffer() const { return false; }
bool ImageResidency::upload() { return false; }
bool ImageResidency::download() { return false; }
void ImageResidency::poisonCpuPlanes() {}
void ImageResidency::trace(const char *) {}

gpu::Buffer *ImageResidency::forRead() { return nullptr; }
gpu::Buffer *ImageResidency::forWrite() { return nullptr; }
gpu::Buffer *ImageResidency::forDiscardWrite() { return nullptr; }
void ImageResidency::syncToCpu() {}
void ImageResidency::invalidateGPU() {}
bool ImageResidency::copyTo(ImageResidency &) { return false; }

ResidencyGuard::~ResidencyGuard() {}

} // namespace rtengine

#endif // ART_USE_VULKAN
