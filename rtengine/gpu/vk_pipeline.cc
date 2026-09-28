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

#include "vk_pipeline.h"

#include "../settings.h"
#include "gpu.h"

#include <glibmm/miscutils.h>

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>

// generated from rtengine/gpu/shaders/*.comp; see doc/gpu_pipeline.md, §2.3
#include "gpu_shaders_generated.h"

namespace rtengine {

extern const Settings *settings;

namespace gpu {

namespace {

struct Key {
    std::string name;
    unsigned int n_bindings;
    unsigned int push_size;
    unsigned int lx;
    unsigned int ly;

    bool operator<(const Key &o) const
    {
        if (name != o.name) return name < o.name;
        if (n_bindings != o.n_bindings) return n_bindings < o.n_bindings;
        if (push_size != o.push_size) return push_size < o.push_size;
        if (lx != o.lx) return lx < o.lx;
        return ly < o.ly;
    }
};

std::map<Key, ComputePipeline> cache_;
std::mutex cache_mutex_;

} // namespace

bool findShader(const char *name, const unsigned int **words, size_t *nbytes,
                std::vector<unsigned int> &scratch)
{
    const char *dir = std::getenv("ART_SHADER_PATH");
    if (dir && *dir) {
        const std::string path =
            Glib::build_filename(dir, std::string(name) + ".spv");
        std::ifstream f(path.c_str(), std::ios::binary | std::ios::ate);
        if (f) {
            const std::streamsize sz = f.tellg();
            if (sz > 0 && (sz % 4) == 0) {
                scratch.resize((size_t)sz / 4);
                f.seekg(0);
                f.read(reinterpret_cast<char *>(&scratch[0]), sz);
                if (f) {
                    *words = &scratch[0];
                    *nbytes = (size_t)sz;
                    if (settings && settings->verbose > 1) {
                        logOnce("GPU: using external shader " + path);
                    }
                    return true;
                }
            }
        }
    }

    for (unsigned int i = 0; i < art_gpu_shader_count; ++i) {
        if (std::strcmp(art_gpu_shaders[i].name, name) == 0) {
            *words = art_gpu_shaders[i].words;
            *nbytes = art_gpu_shaders[i].size;
            return true;
        }
    }
    return false;
}

const ComputePipeline *getComputePipeline(Context &ctx, const char *name,
                                          unsigned int n_bindings,
                                          unsigned int push_size,
                                          unsigned int lx, unsigned int ly)
{
    if (push_size > ctx.caps().max_push_constants_bytes) {
        logOnce(std::string("GPU: kernel ") + name + " wants " +
                std::to_string(push_size) +
                " bytes of push constants, device allows " +
                std::to_string(ctx.caps().max_push_constants_bytes));
        return nullptr;
    }

    Key key;
    key.name = name;
    key.n_bindings = n_bindings;
    key.push_size = push_size;
    key.lx = lx ? lx : ctx.caps().preferred_local_size[0];
    key.ly = ly ? ly : ctx.caps().preferred_local_size[1];

    std::lock_guard<std::mutex> lock(cache_mutex_);
    std::map<Key, ComputePipeline>::iterator it = cache_.find(key);
    if (it != cache_.end()) {
        return it->second.valid() ? &it->second : nullptr;
    }
    // failures are cached too, so a missing kernel costs one lookup, not a
    // repeated failed build
    ComputePipeline &cp = cache_[key];

    const unsigned int *words = nullptr;
    size_t nbytes = 0;
    std::vector<unsigned int> scratch;
    if (!findShader(name, &words, &nbytes, scratch)) {
        logOnce(std::string("GPU: no SPIR-V module named '") + name + "'");
        return nullptr;
    }

    VkDevice dev = ctx.device();

    VkShaderModuleCreateInfo smci;
    std::memset(&smci, 0, sizeof(smci));
    smci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    smci.codeSize = nbytes;
    smci.pCode = words;

    VkShaderModule module = VK_NULL_HANDLE;
    if (vkCreateShaderModule(dev, &smci, nullptr, &module) != VK_SUCCESS) {
        logOnce(std::string("GPU: vkCreateShaderModule failed for ") + name);
        return nullptr;
    }

    std::vector<VkDescriptorSetLayoutBinding> binds(n_bindings);
    for (unsigned int i = 0; i < n_bindings; ++i) {
        std::memset(&binds[i], 0, sizeof(binds[i]));
        binds[i].binding = i;
        binds[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        binds[i].descriptorCount = 1;
        binds[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    }

    VkDescriptorSetLayoutCreateInfo dlci;
    std::memset(&dlci, 0, sizeof(dlci));
    dlci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dlci.bindingCount = n_bindings;
    dlci.pBindings = n_bindings ? &binds[0] : nullptr;

    if (vkCreateDescriptorSetLayout(dev, &dlci, nullptr, &cp.set_layout) !=
        VK_SUCCESS) {
        vkDestroyShaderModule(dev, module, nullptr);
        cp.set_layout = VK_NULL_HANDLE;
        return nullptr;
    }

    VkPushConstantRange pcr;
    std::memset(&pcr, 0, sizeof(pcr));
    pcr.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pcr.offset = 0;
    pcr.size = push_size;

    VkPipelineLayoutCreateInfo plci;
    std::memset(&plci, 0, sizeof(plci));
    plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plci.setLayoutCount = 1;
    plci.pSetLayouts = &cp.set_layout;
    plci.pushConstantRangeCount = push_size ? 1 : 0;
    plci.pPushConstantRanges = push_size ? &pcr : nullptr;

    if (vkCreatePipelineLayout(dev, &plci, nullptr, &cp.layout) != VK_SUCCESS) {
        vkDestroyDescriptorSetLayout(dev, cp.set_layout, nullptr);
        cp.set_layout = VK_NULL_HANDLE;
        cp.layout = VK_NULL_HANDLE;
        vkDestroyShaderModule(dev, module, nullptr);
        return nullptr;
    }

    const unsigned int spec_data[2] = {key.lx, key.ly};
    VkSpecializationMapEntry spec_map[2];
    for (int i = 0; i < 2; ++i) {
        spec_map[i].constantID = (unsigned int)i;
        spec_map[i].offset = (unsigned int)(i * sizeof(unsigned int));
        spec_map[i].size = sizeof(unsigned int);
    }
    VkSpecializationInfo spec;
    std::memset(&spec, 0, sizeof(spec));
    spec.mapEntryCount = 2;
    spec.pMapEntries = spec_map;
    spec.dataSize = sizeof(spec_data);
    spec.pData = spec_data;

    VkComputePipelineCreateInfo cpci;
    std::memset(&cpci, 0, sizeof(cpci));
    cpci.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    cpci.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    cpci.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    cpci.stage.module = module;
    cpci.stage.pName = "main";
    cpci.stage.pSpecializationInfo = &spec;
    cpci.layout = cp.layout;

    const VkResult r = vkCreateComputePipelines(
        dev, ctx.pipelineCache(), 1, &cpci, nullptr, &cp.pipeline);

    vkDestroyShaderModule(dev, module, nullptr);

    if (r != VK_SUCCESS) {
        logOnce(std::string("GPU: vkCreateComputePipelines failed for ") + name +
                ": " + vkResultName(r));
        vkDestroyPipelineLayout(dev, cp.layout, nullptr);
        vkDestroyDescriptorSetLayout(dev, cp.set_layout, nullptr);
        cp.layout = VK_NULL_HANDLE;
        cp.set_layout = VK_NULL_HANDLE;
        cp.pipeline = VK_NULL_HANDLE;
        return nullptr;
    }

    cp.n_bindings = n_bindings;
    cp.push_size = push_size;
    return &cp;
}

void clearPipelineCache(Context &ctx)
{
    std::lock_guard<std::mutex> lock(cache_mutex_);
    VkDevice dev = ctx.device();
    for (std::map<Key, ComputePipeline>::iterator it = cache_.begin();
         it != cache_.end(); ++it) {
        ComputePipeline &cp = it->second;
        if (cp.pipeline) {
            vkDestroyPipeline(dev, cp.pipeline, nullptr);
        }
        if (cp.layout) {
            vkDestroyPipelineLayout(dev, cp.layout, nullptr);
        }
        if (cp.set_layout) {
            vkDestroyDescriptorSetLayout(dev, cp.set_layout, nullptr);
        }
    }
    cache_.clear();
}

} // namespace gpu
} // namespace rtengine
