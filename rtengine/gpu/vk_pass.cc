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

#include "vk_pass.h"

#include "../mytime.h"
#include "../settings.h"
#include "gpu.h"

#include <algorithm>
#include <cstring>
#include <iostream>

namespace rtengine {

extern const Settings *settings;

namespace gpu {

namespace {

const unsigned int MAX_DISPATCHES_PER_PASS = 64;
const unsigned int MAX_BINDINGS_PER_DISPATCH = 8;
// vkCmdUpdateBuffer's spec limit; the data rides inside the command buffer.
const size_t MAX_UPDATE_BUFFER_BYTES = 65536;

} // namespace

unsigned int Pass::maxDispatches() { return MAX_DISPATCHES_PER_PASS; }

size_t Pass::maxUpdateBytes() { return MAX_UPDATE_BUFFER_BYTES; }

Pass::Pass(Context &ctx, const char *debug_name):
    ctx_(ctx), name_(debug_name ? debug_name : "pass"), pool_(VK_NULL_HANDLE),
    cb_(VK_NULL_HANDLE), desc_pool_(VK_NULL_HANDLE), fence_(VK_NULL_HANDLE),
    query_pool_(VK_NULL_HANDLE), n_queries_(0), max_queries_(0),
    submitted_(false), wall_ms_(0.0), device_ms_(0.0)
{
    if (!begin()) {
        // leave cb_ null; valid() is false and every entry point no-ops
    }
}

bool Pass::begin()
{
    if (ctx_.deviceLost()) {
        return false;
    }
    VkDevice dev = ctx_.device();

    pool_ = ctx_.commandPoolForThisThread();
    if (pool_ == VK_NULL_HANDLE) {
        return false;
    }

    VkCommandBufferAllocateInfo cbai;
    std::memset(&cbai, 0, sizeof(cbai));
    cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool = pool_;
    cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;
    if (vkAllocateCommandBuffers(dev, &cbai, &cb_) != VK_SUCCESS) {
        cb_ = VK_NULL_HANDLE;
        return false;
    }

    VkDescriptorPoolSize ps;
    ps.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    ps.descriptorCount = MAX_DISPATCHES_PER_PASS * MAX_BINDINGS_PER_DISPATCH;

    VkDescriptorPoolCreateInfo dpci;
    std::memset(&dpci, 0, sizeof(dpci));
    dpci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dpci.maxSets = MAX_DISPATCHES_PER_PASS;
    dpci.poolSizeCount = 1;
    dpci.pPoolSizes = &ps;
    if (vkCreateDescriptorPool(dev, &dpci, nullptr, &desc_pool_) != VK_SUCCESS) {
        desc_pool_ = VK_NULL_HANDLE;
        vkFreeCommandBuffers(dev, pool_, 1, &cb_);
        cb_ = VK_NULL_HANDLE;
        return false;
    }

    VkFenceCreateInfo fci;
    std::memset(&fci, 0, sizeof(fci));
    fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (vkCreateFence(dev, &fci, nullptr, &fence_) != VK_SUCCESS) {
        fence_ = VK_NULL_HANDLE;
    }

    if (ctx_.caps().timestampsUsable()) {
        // +2: queries 0 and 1 bracket the whole command buffer, so the
        // submission's device time can be separated from the CPU's
        // submit->wait wall.  Per-dispatch queries start at 2.
        max_queries_ = 2 * MAX_DISPATCHES_PER_PASS + 2;
        VkQueryPoolCreateInfo qci;
        std::memset(&qci, 0, sizeof(qci));
        qci.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        qci.queryType = VK_QUERY_TYPE_TIMESTAMP;
        qci.queryCount = max_queries_;
        if (vkCreateQueryPool(dev, &qci, nullptr, &query_pool_) != VK_SUCCESS) {
            query_pool_ = VK_NULL_HANDLE;
            max_queries_ = 0;
        }
    }

    VkCommandBufferBeginInfo bi;
    std::memset(&bi, 0, sizeof(bi));
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(cb_, &bi) != VK_SUCCESS) {
        cb_ = VK_NULL_HANDLE;
        return false;
    }
    if (query_pool_) {
        vkCmdResetQueryPool(cb_, query_pool_, 0, max_queries_);
        vkCmdWriteTimestamp(cb_, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, query_pool_,
                            0);
        n_queries_ = 2;    // 0/1 reserved for the whole-pass bracket
    }
    return true;
}

void Pass::releaseRecorded()
{
    for (size_t i = 0; i < accesses_.size(); ++i) {
        accesses_[i].buffer->recorded_ = false;
    }
}

Pass::~Pass()
{
    releaseRecorded();
    VkDevice dev = ctx_.device();
    if (!dev) {
        return;
    }
    if (ctx_.deviceLost()) {
        // handles may still be referenced by a submission that never
        // finished (see the VK_TIMEOUT case in submitAndWait()); destroying
        // them would be UB, so leak instead.
        return;
    }
    if (cb_ != VK_NULL_HANDLE && !submitted_) {
        vkEndCommandBuffer(cb_);
    }
    if (fence_ != VK_NULL_HANDLE) {
        vkDestroyFence(dev, fence_, nullptr);
    }
    if (query_pool_ != VK_NULL_HANDLE) {
        vkDestroyQueryPool(dev, query_pool_, nullptr);
    }
    if (desc_pool_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(dev, desc_pool_, nullptr);
    }
    if (cb_ != VK_NULL_HANDLE && pool_ != VK_NULL_HANDLE) {
        vkFreeCommandBuffers(dev, pool_, 1, &cb_);
    }
}

void Pass::barrierFor(const std::vector<Binding> &bindings)
{
    // W->R, R->W, W->W need a barrier; R->R doesn't. Stage masks include
    // TRANSFER as well as COMPUTE_SHADER because fillBuffer() runs in the
    // transfer stage: a COMPUTE->COMPUTE-only barrier wouldn't order a
    // shader read against a preceding fill.
    std::vector<VkBufferMemoryBarrier> barriers;

    for (size_t i = 0; i < bindings.size(); ++i) {
        const Binding &b = bindings[i];
        if (!b.buffer || !b.buffer->valid()) {
            continue;
        }
        bool need = false;
        for (size_t a = 0; a < accesses_.size(); ++a) {
            if (accesses_[a].buffer != b.buffer) {
                continue;
            }
            if (accesses_[a].written || b.write) {
                need = true;
            }
            break;
        }
        if (!need) {
            continue;
        }
        VkBufferMemoryBarrier bm;
        std::memset(&bm, 0, sizeof(bm));
        bm.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        bm.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT |
                           VK_ACCESS_SHADER_READ_BIT |
                           VK_ACCESS_TRANSFER_WRITE_BIT;
        bm.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT |
                           VK_ACCESS_SHADER_READ_BIT |
                           VK_ACCESS_TRANSFER_WRITE_BIT;
        bm.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bm.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bm.buffer = b.buffer->handle();
        bm.offset = b.offset;
        bm.size = b.range;
        barriers.push_back(bm);
    }

    if (!barriers.empty()) {
        const VkPipelineStageFlags stages =
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
            VK_PIPELINE_STAGE_TRANSFER_BIT;
        vkCmdPipelineBarrier(cb_, stages, stages, 0, 0, nullptr,
                             (unsigned int)barriers.size(), &barriers[0], 0,
                             nullptr);
    }

    for (size_t i = 0; i < bindings.size(); ++i) {
        const Binding &b = bindings[i];
        if (!b.buffer || !b.buffer->valid()) {
            continue;
        }
        b.buffer->recorded_ = true;   // see Buffer::recorded_
        bool found = false;
        for (size_t a = 0; a < accesses_.size(); ++a) {
            if (accesses_[a].buffer == b.buffer) {
                accesses_[a].written = b.write;
                found = true;
                break;
            }
        }
        if (!found) {
            Access acc;
            acc.buffer = b.buffer;
            acc.written = b.write;
            accesses_.push_back(acc);
        }
    }
}

bool Pass::dispatch1D(const char *kernel, const std::vector<Binding> &bindings,
                     const void *push, unsigned int push_size, size_t n)
{
    if (!valid() || !n) {
        return false;
    }
    unsigned int lx = 256;
    if (lx > ctx_.caps().max_workgroup_invocations) {
        lx = ctx_.caps().max_workgroup_invocations;
    }
    if (lx > ctx_.caps().max_workgroup_size[0]) {
        lx = ctx_.caps().max_workgroup_size[0];
    }
    if (!lx) {
        return false;
    }
    const size_t groups = (n + lx - 1) / lx;
    if (groups > ctx_.caps().max_workgroup_count[0]) {
        logOnce(std::string("GPU: ") + kernel +
                " needs more workgroups than the device allows; tile it");
        return false;
    }
    return dispatchRaw(kernel, bindings, push, push_size,
                       (unsigned int)groups, 1, 1, lx, 1);
}

bool Pass::dispatch2D(const char *kernel, const std::vector<Binding> &bindings,
                      const void *push, unsigned int push_size, int W, int H)
{
    if (!valid() || W <= 0 || H <= 0) {
        return false;
    }
    const unsigned int lx = ctx_.caps().preferred_local_size[0];
    const unsigned int ly = ctx_.caps().preferred_local_size[1];
    const unsigned int gx = ((unsigned int)W + lx - 1) / lx;
    const unsigned int gy = ((unsigned int)H + ly - 1) / ly;
    if (gx > ctx_.caps().max_workgroup_count[0] ||
        gy > ctx_.caps().max_workgroup_count[1]) {
        logOnce(std::string("GPU: ") + kernel +
                " needs more workgroups than the device allows; tile it");
        return false;
    }
    return dispatchRaw(kernel, bindings, push, push_size, gx, gy, 1, lx, ly);
}

bool Pass::dispatchRaw(const char *kernel,
                       const std::vector<Binding> &bindings, const void *push,
                       unsigned int push_size, unsigned int gx,
                       unsigned int gy, unsigned int gz, unsigned int lx,
                       unsigned int ly)
{
    if (!valid()) {
        return false;
    }
    if (bindings.size() > MAX_BINDINGS_PER_DISPATCH ||
        labels_.size() >= MAX_DISPATCHES_PER_PASS) {
        logOnce(std::string("GPU: pass ") + name_ + " exceeded its dispatch or "
                "binding budget");
        return false;
    }

    const ComputePipeline *cp = getComputePipeline(
        ctx_, kernel, (unsigned int)bindings.size(), push_size, lx, ly);
    if (!cp) {
        return false;
    }

    VkDevice dev = ctx_.device();

    VkDescriptorSetAllocateInfo dsai;
    std::memset(&dsai, 0, sizeof(dsai));
    dsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dsai.descriptorPool = desc_pool_;
    dsai.descriptorSetCount = 1;
    dsai.pSetLayouts = &cp->set_layout;

    VkDescriptorSet set = VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(dev, &dsai, &set) != VK_SUCCESS) {
        logOnce(std::string("GPU: vkAllocateDescriptorSets failed in ") + name_);
        return false;
    }

    std::vector<VkDescriptorBufferInfo> infos(bindings.size());
    std::vector<VkWriteDescriptorSet> writes(bindings.size());
    for (size_t i = 0; i < bindings.size(); ++i) {
        if (!bindings[i].buffer || !bindings[i].buffer->valid()) {
            return false;
        }
        infos[i].buffer = bindings[i].buffer->handle();
        infos[i].offset = bindings[i].offset;
        infos[i].range = bindings[i].range;

        std::memset(&writes[i], 0, sizeof(writes[i]));
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = set;
        // positional: bindings vector order IS the binding contract, must
        // be contiguous 0..N-1 (see CLAUDE.md, "Shader binding numbering").
        writes[i].dstBinding = (unsigned int)i;
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[i].pBufferInfo = &infos[i];
    }
    if (!writes.empty()) {
        vkUpdateDescriptorSets(dev, (unsigned int)writes.size(), &writes[0], 0,
                               nullptr);
    }

    barrierFor(bindings);

    const bool timed = query_pool_ && (n_queries_ + 2 <= max_queries_);
    if (timed) {
        vkCmdWriteTimestamp(cb_, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                            query_pool_, n_queries_);
    }

    vkCmdBindPipeline(cb_, VK_PIPELINE_BIND_POINT_COMPUTE, cp->pipeline);
    vkCmdBindDescriptorSets(cb_, VK_PIPELINE_BIND_POINT_COMPUTE, cp->layout, 0,
                            1, &set, 0, nullptr);
    if (push_size && push) {
        vkCmdPushConstants(cb_, cp->layout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
                           push_size, push);
    }

    vkCmdDispatch(cb_, gx, gy, gz);

    if (timed) {
        vkCmdWriteTimestamp(cb_, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                            query_pool_, n_queries_ + 1);
        n_queries_ += 2;
    }
    labels_.push_back(kernel);
    return true;
}

bool Pass::fillBuffer(const Buffer &buffer, unsigned int value, size_t offset,
                      size_t size)
{
    if (!valid() || !buffer.valid()) {
        return false;
    }
    if (offset % 4u) {
        logOnce(std::string("GPU: fillBuffer offset must be 4-byte aligned in ")
                + name_);
        return false;
    }
    if (size != VK_WHOLE_SIZE) {
        if (size % 4u || offset + size > buffer.size()) {
            logOnce(std::string("GPU: bad fillBuffer range in ") + name_);
            return false;
        }
    } else if (buffer.size() % 4u) {
        logOnce(std::string("GPU: fillBuffer whole-size buffer is not a "
                            "multiple of 4 in ") + name_);
        return false;
    }

    std::vector<Binding> b;
    Binding bd(&buffer, true);
    bd.offset = offset;
    bd.range = size;
    b.push_back(bd);
    barrierFor(b);

    vkCmdFillBuffer(cb_, buffer.handle(), offset, size, value);

    // uses neither a descriptor set nor a pipeline, so it isn't counted
    // against MAX_DISPATCHES_PER_PASS or timestamped.
    return true;
}

bool Pass::updateBuffer(const Buffer &buffer, const void *data, size_t offset,
                        size_t size)
{
    if (!valid() || !buffer.valid() || !data || !size) {
        return false;
    }
    /* vkCmdUpdateBuffer's own limits: 4-byte aligned offset and size, and at
     * most 65536 bytes, because the data is copied into the command buffer
     * itself. */
    if (offset % 4u || size % 4u) {
        logOnce(std::string("GPU: updateBuffer offset and size must be "
                            "4-byte aligned in ") + name_);
        return false;
    }
    if (size > MAX_UPDATE_BUFFER_BYTES || offset + size > buffer.size()) {
        logOnce(std::string("GPU: bad updateBuffer range in ") + name_);
        return false;
    }

    std::vector<Binding> b;
    Binding bd(&buffer, true);
    bd.offset = offset;
    bd.range = size;
    b.push_back(bd);
    barrierFor(b);

    vkCmdUpdateBuffer(cb_, buffer.handle(), offset, size, data);

    // transfer-stage command with no descriptor set or pipeline, so it is not
    // counted against MAX_DISPATCHES_PER_PASS or timestamped -- same as
    // fillBuffer() and copyBuffer().
    return true;
}

bool Pass::copyBuffer(const Buffer &src, const Buffer &dst, size_t src_offset,
                      size_t dst_offset, size_t size)
{
    if (!valid() || !src.valid() || !dst.valid()) {
        return false;
    }
    if (size == VK_WHOLE_SIZE) {
        const size_t src_avail = src.size() > src_offset ? src.size() - src_offset : 0;
        const size_t dst_avail = dst.size() > dst_offset ? dst.size() - dst_offset : 0;
        size = std::min(src_avail, dst_avail);
    }
    if (src_offset + size > src.size() || dst_offset + size > dst.size()) {
        logOnce(std::string("GPU: bad copyBuffer range in ") + name_);
        return false;
    }

    std::vector<Binding> b;
    Binding sb(&src, false);
    sb.offset = src_offset;
    sb.range = size;
    Binding db(&dst, true);
    db.offset = dst_offset;
    db.range = size;
    b.push_back(sb);
    b.push_back(db);
    barrierFor(b);

    VkBufferCopy region;
    region.srcOffset = src_offset;
    region.dstOffset = dst_offset;
    region.size = size;
    vkCmdCopyBuffer(cb_, src.handle(), dst.handle(), 1, &region);

    // not counted against MAX_DISPATCHES_PER_PASS or timestamped, same as
    // fillBuffer.
    return true;
}

bool Pass::submitAndWait()
{
    if (!valid() || submitted_) {
        return false;
    }
    submitted_ = true;

    VkDevice dev = ctx_.device();
    if (query_pool_) {
        vkCmdWriteTimestamp(cb_, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                            query_pool_, 1);
    }
    if (vkEndCommandBuffer(cb_) != VK_SUCCESS) {
        return false;
    }
    if (fence_ == VK_NULL_HANDLE) {
        return false;
    }
    vkResetFences(dev, 1, &fence_);

    MyTime t0, t1;
    t0.set();

    VkResult r = ctx_.submit(cb_, fence_);
    if (r != VK_SUCCESS) {
        if (r == VK_ERROR_DEVICE_LOST) {
            ctx_.markDeviceLost("vkQueueSubmit");
        } else {
            logOnce(std::string("GPU: vkQueueSubmit failed: ") +
                    vkResultName(r));
        }
        return false;
    }

    r = vkWaitForFences(dev, 1, &fence_, VK_TRUE, 10ull * 1000 * 1000 * 1000);
    t1.set();
    wall_ms_ = t1.etime(t0) / 1000.0;

    if (r != VK_SUCCESS) {
        // recorded even on failure: the thread was blocked for wall_ms_
        // either way.  No device time: the queries never resolved.
        addSubmission(wall_ms_, 0.0);
        if (r == VK_ERROR_DEVICE_LOST) {
            ctx_.markDeviceLost("vkWaitForFences");
        } else if (r == VK_TIMEOUT) {
            // command buffer may still be referenced by the queue; treat as
            // device loss so ~Pass() leaks instead of destroying live handles.
            ctx_.markDeviceLost("vkWaitForFences timeout");
        } else {
            logOnce(std::string("GPU: vkWaitForFences returned ") +
                    vkResultName(r));
        }
        return false;
    }

    if (query_pool_ && n_queries_) {
        std::vector<uint64_t> stamps(n_queries_, 0);
        if (vkGetQueryPoolResults(dev, query_pool_, 0, n_queries_,
                                  n_queries_ * sizeof(uint64_t), &stamps[0],
                                  sizeof(uint64_t),
                                  VK_QUERY_RESULT_64_BIT |
                                      VK_QUERY_RESULT_WAIT_BIT) == VK_SUCCESS) {
            const double period = ctx_.caps().timestamp_period_ns;
            // queries 0/1 bracket the command buffer as a whole, so this
            // covers the copies and fills the per-dispatch pairs below skip.
            device_ms_ = double(stamps[1] - stamps[0]) * period / 1e6;
            for (unsigned int i = 2; i + 1 < n_queries_; i += 2) {
                const size_t label = (i - 2) / 2;
                const double ms =
                    double(stamps[i + 1] - stamps[i]) * period / 1e6;
                timings_.push_back(std::make_pair(
                    label < labels_.size() ? labels_[label] : std::string("?"),
                    ms));
            }
        }
    }
    addSubmission(wall_ms_, device_ms_);
    releaseRecorded();   // the GPU is done with them; safe to free or reuse
    return true;
}

void Pass::reportTimings() const
{
    if (!settings || settings->verbose <= 1) {
        return;
    }
    for (size_t i = 0; i < timings_.size(); ++i) {
        std::cout << timings_[i].first << " took " << timings_[i].second
                  << " ms (GPU)" << std::endl;
    }
    std::cout << name_ << " submit->wait " << wall_ms_ << " ms (wall), "
              << device_ms_ << " ms (device), " << (wall_ms_ - device_ms_)
              << " ms (overhead)" << std::endl;
}

PassSeq::PassSeq(Context &ctx, const char *name):
    ctx_(ctx), name_(name), used_(0), ms_(0.0), submissions_(0)
{
}

Pass *PassSeq::reserve(unsigned int n)
{
    if (n > Pass::maxDispatches()) {
        return nullptr; // no single op may exceed one Pass
    }
    if (pass_ && used_ + n > Pass::maxDispatches() && !flush()) {
        return nullptr;
    }
    if (!pass_) {
        pass_.reset(new Pass(ctx_, name_));
        if (!pass_->valid()) {
            pass_.reset();
            return nullptr;
        }
        used_ = 0;
    }
    used_ += n;
    return pass_.get();
}

bool PassSeq::flush()
{
    if (!pass_) {
        return true;
    }
    const bool ok = pass_->submitAndWait();
    if (ok) {
        ms_ += pass_->wallMs();
        ++submissions_;
        pass_->reportTimings();
    }
    pass_.reset();
    used_ = 0;
    return ok;
}

} // namespace gpu
} // namespace rtengine
