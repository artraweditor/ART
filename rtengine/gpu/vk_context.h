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
#ifndef ART_GPU_VK_CONTEXT_H
#define ART_GPU_VK_CONTEXT_H

#include "gpu.h"
#include "vk_api.h"
#include "../noncopyable.h"

#include <mutex>
#include <string>
#include <memory>
#include <vector>

namespace rtengine {
namespace gpu {

/* Everything downstream code may branch on; populated once at device
 * creation. "spec floor" values are the guaranteed minimum -- honour them
 * unless caps say otherwise at run time. See doc/gpu_pipeline.md, §2.2. */
struct Caps {
    Caps();

    unsigned int api_version;
    unsigned int max_workgroup_count[3];
    unsigned int max_workgroup_size[3];
    unsigned int max_workgroup_invocations;
    unsigned int max_shared_memory_bytes;    // spec floor 16384
    unsigned int max_push_constants_bytes;   // spec floor 128
    unsigned long long max_storage_buffer_range; // spec floor 128 MiB
    unsigned long long min_storage_buffer_offset_alignment;
    unsigned long long non_coherent_atom_size;
    unsigned long long buffer_image_granularity;
    unsigned int max_memory_allocation_count;
    float timestamp_period_ns;               // 0 => timestamps unusable
    unsigned int timestamp_valid_bits;       // 0 => timestamps unusable
    unsigned int subgroup_size;              // advisory only, never hard-code
    bool subgroup_arithmetic;
    bool subgroup_in_compute;
    bool push_descriptor;
    bool external_memory_host;
    unsigned long long min_imported_host_pointer_alignment;
    bool portability_subset;                 // i.e. MoltenVK
    /* True when some memory type is both DEVICE_LOCAL and HOST_VISIBLE.
    * NOT a test for unified memory: every discrete NVIDIA card exposes such a
    * type (the PCIe BAR window -- 256 MB without resizable BAR, all of VRAM
    * with it), so this is true on an RTX 4500 Ada too.  It says only that
    * mapping device memory is *possible*, not that it is cheap. */
    bool host_visible_device_local;

    /* True when device memory really is system memory, i.e. the device is not
    * discrete.  This is the one to branch on when deciding whether mapping
    * device memory is a fast path or a PCIe round trip. */
    bool unified_memory;
    unsigned long long device_local_bytes;   // largest DEVICE_LOCAL heap; 0 if none reported
    bool is_software;                        // lavapipe / llvmpipe / SwiftShader
    unsigned int preferred_local_size[2];

    bool timestampsUsable() const
    {
        return timestamp_period_ns > 0.f && timestamp_valid_bits > 0;
    }
};

/* A device buffer plus its memory. Move-only. mapped() is null when the
 * memory isn't host-visible (see HostMemoryMode below and §2.2). */
class Context;

class Buffer {
public:
    Buffer();
    ~Buffer();
    Buffer(Buffer &&o);
    Buffer &operator=(Buffer &&o);

    bool valid() const { return buf_ != VK_NULL_HANDLE; }
    VkBuffer handle() const { return buf_; }
    size_t size() const { return size_; }

    /* Non-null only when the memory is host-visible. */
    void *mapped() const { return mapped_; }
    bool hostVisible() const { return mapped_ != nullptr; }
    bool hostCoherent() const { return coherent_; }

    /* No-ops when the memory is coherent.  Ranges are rounded out to
     * nonCoherentAtomSize as the spec requires. */
    void flush(size_t offset, size_t len);
    void invalidate(size_t offset, size_t len);

    void reset();

private:
    friend class Context;
    friend class Pass;
    Buffer(const Buffer &);
    Buffer &operator=(const Buffer &);

    VkBuffer buf_;
    VkDeviceMemory mem_;
    void *mapped_;
    size_t size_;
    bool coherent_;

    /* Set while some Pass has recorded a binding to this buffer but has not
     * yet been submitted and waited on.  Destroying or move-assigning over a
     * buffer in that state frees memory a recorded descriptor set still
     * points at, and the symptom is a GPU hang -- a ten-second fence timeout
     * and a lost device, several operators later, with nothing naming the
     * real culprit.  reset() logs instead of leaving that to be discovered on
     * one vendor's hardware.
     *
     * Harmless before Passes were batched, because every op submitted and
     * waited before the next statement; it became reachable the moment a
     * chain of ops shared one open Pass. */
    mutable bool recorded_;
};

/* Which memory a buffer is allocated from -- see Context::createBuffer and
 * doc/gpu_pipeline.md, §2.2. STAGING_ONLY buffers are always mapped() on
 * success and must never be bound as a shader input. */
enum class HostMemoryMode {
    PREFER_DEVICE_LOCAL,
    DEVICE_LOCAL_ONLY,
    STAGING_ONLY
};

/* A reusable set of device buffers; see §2.2 for why (~28% of wall time in
 * the wavelet denoise port was fresh VkDeviceMemory allocation).
 *
 * Two invariants, both silent-corruption risks if broken:
 *  - only recycle() after submitAndWait() -- barrierFor() tracks buffers by
 *    address, so recycling mid-recording aliases two logically distinct uses.
 *  - buffer addresses must stay stable across get() calls, since
 *    Pass::Binding stores a Buffer*; hence vector<unique_ptr<Entry>> rather
 *    than vector<Buffer>.
 *
 * Not thread-safe; give each thread its own, as with command pools. */
class BufferPool {
public:
    /* `mode` is fixed for the pool's lifetime. A staging pool must be
     * constructed with STAGING_ONLY explicitly (see
     * Context::stagingPoolForThisThread()) so a caller expecting a mapped
     * buffer can never be handed a possibly-unmapped PREFER_DEVICE_LOCAL one. */
    explicit BufferPool(Context &ctx,
                        HostMemoryMode mode = HostMemoryMode::PREFER_DEVICE_LOCAL):
        ctx_(ctx), mode_(mode)
    {
    }

    /* Buffer of at least `bytes`, marked in use; null on allocation failure.
     * Picks the smallest free buffer that fits, so a large request doesn't
     * consume a buffer a later large request needs. */
    Buffer *get(size_t bytes);

    /* Scoped checkout: mark() records the current depth, release(m) frees
     * everything handed out since, leaving earlier checkouts untouched --
     * lets a caller keep some buffers for the whole run while reclaiming
     * per-iteration scratch. */
    size_t mark() const { return checked_out_.size(); }
    void release(size_t mark);

    /* Make every buffer reusable.  Nothing is freed. */
    void recycle() { release(0); }

    /* Release the memory outright. */
    void clear();

    size_t count() const { return entries_.size(); }
    size_t bytes() const;

private:
    BufferPool(const BufferPool &);
    BufferPool &operator=(const BufferPool &);

    struct Entry {
        Buffer buf;
        bool in_use;
        Entry(): in_use(false) {}
    };

    Context &ctx_;
    HostMemoryMode mode_;
    std::vector<std::unique_ptr<Entry> > entries_;
    /* Checkout order, so release() can undo a suffix of it. */
    std::vector<Entry *> checked_out_;
};


class PoolScope: public NonCopyable {
public:
    explicit PoolScope(BufferPool &p): pool_(p), mark_(p.mark())
    {
    }
    ~PoolScope() { pool_.release(mark_); }
private:
    BufferPool &pool_;
    size_t mark_;
};


class Context {
public:
    /* Opens the device if it is not open yet.  Returns null when unavailable.
     * Thread-safe; the slow path runs at most once. */
    static Context *get();
    static void destroy();

    /* Recorded by gpu::init before any device is opened. */
    static void configure(const Glib::ustring &user_settings_dir,
                          const Glib::ustring &device_preference,
                          bool allow_software);

    static std::vector<DeviceInfo> enumerate();

    VkDevice device() const { return dev_; }
    VkPhysicalDevice physicalDevice() const { return pdev_; }
    unsigned int queueFamily() const { return queue_family_; }
    VkPipelineCache pipelineCache() const { return pipeline_cache_; }
    const Caps &caps() const { return caps_; }
    const DeviceInfo &info() const { return info_; }
    const std::string &description() const { return description_; }

    /* The only place vkQueueSubmit is called (Vulkan queues aren't
     * thread-safe); lock is never held across a wait. */
    VkResult submit(VkCommandBuffer cb, VkFence fence);

    /* Not thread-safe; one per thread, built once per (long-lived) worker. */
    VkCommandPool commandPoolForThisThread();

    Buffer createBuffer(size_t bytes, HostMemoryMode mode);

    /* See the definition: whether PREFER_DEVICE_LOCAL may map device memory
     * directly, which is a fast path on unified memory and a PCIe-readback
     * trap on a discrete GPU. */
    bool hostVisibleDeviceLocalWanted() const;
    /* Compatibility overload: true == PREFER_DEVICE_LOCAL, false == DEVICE_LOCAL_ONLY. */
    Buffer createBuffer(size_t bytes, bool want_host_visible);

    /* Per-thread STAGING_ONLY pool for callers without their own (e.g.
     * ImageResidency keeps a dedicated one instead, needing only one buffer). */
    BufferPool &stagingPoolForThisThread();

    /* Latches: once lost, everything falls back to CPU for the rest of the
     * process rather than retrying a dead device. */
    void markDeviceLost(const char *where);
    bool deviceLost() const { return device_lost_; }

    void flushPipelineCacheToDisk();

private:
    Context();
    ~Context();
    bool open(std::string &err);
    bool pickPhysicalDevice(VkInstance inst, std::string &err);
    void queryCaps();
    void loadPipelineCacheFromDisk();
    Buffer allocateBuffer(size_t bytes, const VkMemoryPropertyFlags *tiers,
                          int n_tiers);

    VkInstance inst_;
    VkPhysicalDevice pdev_;
    VkDevice dev_;
    VkQueue queue_;
    unsigned int queue_family_;
    VkPipelineCache pipeline_cache_;
    std::string pipeline_cache_path_;
    Caps caps_;
    DeviceInfo info_;
    std::string description_;
    VkPhysicalDeviceMemoryProperties mem_props_;

    std::mutex submit_mutex_;
    std::mutex pool_mutex_;
    std::vector<VkCommandPool> pools_;   // owned, for teardown
    bool device_lost_;
};

/* Empty when Context::get() succeeded or the GPU is deliberately off. */
const std::string &contextError();

class Pass;

/* Synchronous upload/download that work whether or not the buffer is
 * host-visible: plain memcpy when mapped, otherwise routed through
 * `staging_pool` (must be non-null and STAGING_ONLY) via Pass::copyBuffer.
 * See §2.2.
 *
 * Pass `pass` whenever the caller already has one open and the upload feeds
 * dispatches recorded into it.  A small payload (a LUT, a parameter block)
 * is then recorded with Pass::updateBuffer instead of becoming a staged
 * submission of its own -- which on a discrete GPU is a queue round trip per
 * upload, and there are often several before a single dispatch. */
bool uploadToBuffer(Context &ctx, BufferPool *staging_pool, const void *src,
                    size_t bytes, Buffer &dst, size_t dst_offset = 0,
                    Pass *pass = nullptr);
bool downloadFromBuffer(Context &ctx, BufferPool *staging_pool, Buffer &src,
                        size_t src_offset, void *dst, size_t bytes);

/* Device-to-device, no host trip regardless of host-visibility. */
bool copyBufferToBuffer(Context &ctx, const Buffer &src, size_t src_offset,
                        Buffer &dst, size_t dst_offset, size_t bytes);

} // namespace gpu
} // namespace rtengine

#endif // ART_GPU_VK_CONTEXT_H
