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
#ifndef ART_GPU_VK_PIPELINE_H
#define ART_GPU_VK_PIPELINE_H

#include "vk_context.h"

#include <string>

namespace rtengine {
namespace gpu {

// see doc/gpu_pipeline.md, §2.3
// $ART_SHADER_PATH/<name>.spv overrides the embedded blob, for iterating on
// a shader without a rebuild.
bool findShader(const char *name, const unsigned int **words, size_t *nbytes,
                std::vector<unsigned int> &scratch);

// Immutable once built, so the cache only needs a lock on insert.
struct ComputePipeline {
    ComputePipeline():
        pipeline(VK_NULL_HANDLE), layout(VK_NULL_HANDLE),
        set_layout(VK_NULL_HANDLE), n_bindings(0), push_size(0)
    {
    }

    bool valid() const { return pipeline != VK_NULL_HANDLE; }

    VkPipeline pipeline;
    VkPipelineLayout layout;
    VkDescriptorSetLayout set_layout;
    unsigned int n_bindings;
    unsigned int push_size;
};

// Cached on {name, n_bindings, push_size, lx, ly}; lx/ly default to the
// device's preferred workgroup size. Returns null on any failure, never
// throws. lx/ly here and the dispatch's group-count math MUST agree, or the
// kernel silently covers the wrong footprint (see doc/gpu_pipeline.md, §2.3).
const ComputePipeline *getComputePipeline(Context &ctx, const char *name,
                                          unsigned int n_bindings,
                                          unsigned int push_size,
                                          unsigned int lx = 0,
                                          unsigned int ly = 0);

// Called from gpu::cleanup(), before the device goes away.
void clearPipelineCache(Context &ctx);

} // namespace gpu
} // namespace rtengine

#endif // ART_GPU_VK_PIPELINE_H
