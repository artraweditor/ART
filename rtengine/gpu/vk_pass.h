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
#ifndef ART_GPU_VK_PASS_H
#define ART_GPU_VK_PASS_H

#include "vk_context.h"
#include "vk_pipeline.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace rtengine {
namespace gpu {

// One command buffer, many dispatches, one submission; see
// doc/gpu_pipeline.md, §2.4. Every entry point returns bool rather than
// throwing.
class Pass {
public:
    Pass(Context &ctx, const char *debug_name);
    ~Pass();

    bool valid() const { return cb_ != VK_NULL_HANDLE; }

    struct Binding {
        Binding(): buffer(nullptr), offset(0), range(VK_WHOLE_SIZE), write(false)
        {
        }
        Binding(const Buffer *b, bool w):
            buffer(b), offset(0), range(VK_WHOLE_SIZE), write(w)
        {
        }

        const Buffer *buffer;
        size_t offset;
        size_t range;      // VK_WHOLE_SIZE for the whole buffer
        bool write;
    };

    bool dispatch2D(const char *kernel, const std::vector<Binding> &bindings,
                    const void *push, unsigned int push_size, int W, int H);

    // `offset`/`size` must be multiples of 4 (Vulkan requirement); pass
    // VK_WHOLE_SIZE for the rest of the buffer, requiring its size be a
    // multiple of 4 too. Used to zero histogram buffers without a host
    // touch in the middle of a device-only phase.
    bool fillBuffer(const Buffer &buffer, unsigned int value = 0,
                    size_t offset = 0, size_t size = VK_WHOLE_SIZE);

    /* Records host data straight into the command buffer, so a small upload
     * (a LUT, a parameter block) costs no staging buffer and -- unlike
     * uploadToBuffer() against an unmapped destination -- no separate queue
     * submission.  `offset` and `size` must be multiples of 4 and `size` at
     * most maxUpdateBytes(). */
    bool updateBuffer(const Buffer &buffer, const void *data, size_t offset,
                      size_t size);

    static size_t maxUpdateBytes();

    // VK_WHOLE_SIZE copies min(src.size()-src_offset, dst.size()-dst_offset).
    // Neither buffer needs to be host-visible.
    bool copyBuffer(const Buffer &src, const Buffer &dst, size_t src_offset = 0,
                    size_t dst_offset = 0, size_t size = VK_WHOLE_SIZE);

    // Lets a caller sequencing more dispatches than this place its own Pass
    // boundaries instead of discovering the limit as a failed dispatch.
    static unsigned int maxDispatches();

    // Flat 1D variant (local_size_x only), for kernels treating the image
    // as a contiguous run rather than a 2D grid.
    bool dispatch1D(const char *kernel, const std::vector<Binding> &bindings,
                    const void *push, unsigned int push_size, size_t n);

    // Returns false on device loss or any submission failure.
    bool submitAndWait();

    // Empty when the device cannot do timestamps.
    const std::vector<std::pair<std::string, double> > &timings() const
    {
        return timings_;
    }

    double wallMs() const { return wall_ms_; }

    /* Device execution time for the whole submission, from a timestamp pair
     * bracketing the command buffer.  0 when the device cannot do timestamps.
     * wallMs() - deviceMs() is the per-submission overhead. */
    double deviceMs() const { return device_ms_; }

    void reportTimings() const;

private:
    Pass(const Pass &);
    Pass &operator=(const Pass &);

    bool dispatchRaw(const char *kernel, const std::vector<Binding> &bindings,
                     const void *push, unsigned int push_size, unsigned int gx,
                     unsigned int gy, unsigned int gz, unsigned int lx,
                     unsigned int ly);
    bool begin();
    void barrierFor(const std::vector<Binding> &bindings);
    void releaseRecorded();

    Context &ctx_;
    std::string name_;
    VkCommandPool pool_;
    VkCommandBuffer cb_;
    VkDescriptorPool desc_pool_;
    VkFence fence_;
    VkQueryPool query_pool_;
    unsigned int n_queries_;
    unsigned int max_queries_;
    bool submitted_;
    double wall_ms_;
    double device_ms_;

    struct Access {
        const Buffer *buffer;
        bool written;
    };
    std::vector<Access> accesses_;
    std::vector<std::string> labels_;
    std::vector<std::pair<std::string, double> > timings_;
};

/* A chain of Passes with the submission boundaries placed by dispatch count
 * rather than by hand.  Callers that record more dispatches than one Pass can
 * hold (denoise's wavelet core needs ~220 at 8 levels, against
 * Pass::maxDispatches() == 64) would otherwise have to either split by hand or
 * submit per operator -- and submitting per operator is what makes the GPU
 * path slow, since each submission drains the queue and blocks the calling
 * thread.  Measured at 85% of wall time in the wavelet primitive before it was
 * batched (CURRENT_GPU_PLAN.md), and worse on a discrete GPU, where the idle
 * gap between submissions also keeps the device's clocks down.
 *
 * Two rules callers have to respect, both about the pool: a buffer recorded
 * into a Pass must not be recycled until that Pass has been submitted *and*
 * waited on (call flush() first); and nothing may be released across a
 * boundary this class inserts on its own, which is why it never touches the
 * pool itself. */
class PassSeq {
public:
    PassSeq(Context &ctx, const char *name);

    /* A Pass with room for `n` more dispatches, submitting and reopening the
     * chain if the current one is too full.  Null on failure. */
    Pass *reserve(unsigned int n);

    /* Submit what has been recorded and start over.  Safe to call with
     * nothing pending. */
    bool flush();

    double wallMs() const { return ms_; }
    int submissions() const { return submissions_; }

private:
    PassSeq(const PassSeq &);
    PassSeq &operator=(const PassSeq &);

    Context &ctx_;
    const char *name_;
    std::unique_ptr<Pass> pass_;
    unsigned int used_;
    double ms_;
    int submissions_;
};

} // namespace gpu
} // namespace rtengine

#endif // ART_GPU_VK_PASS_H
