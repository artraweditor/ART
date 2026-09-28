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
 * Runtime-loaded Vulkan entry points; see doc/gpu_pipeline.md, §2.1.
 *
 * Function pointers are declared as globals under their real Vulkan names so
 * call sites read like ordinary Vulkan code, and so this file is a drop-in
 * for the volk meta-loader should the entry-point list outgrow hand
 * maintenance.
 */
#ifndef ART_GPU_VK_API_H
#define ART_GPU_VK_API_H

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include <string>

/* Entry points resolved from the library itself, before any instance exists. */
#define ART_VK_GLOBAL_FUNCS(X)                                                 \
    X(vkCreateInstance)                                                        \
    X(vkEnumerateInstanceVersion)                                              \
    X(vkEnumerateInstanceExtensionProperties)                                  \
    X(vkEnumerateInstanceLayerProperties)

/* Entry points resolved from a VkInstance. */
#define ART_VK_INSTANCE_FUNCS(X)                                               \
    X(vkDestroyInstance)                                                       \
    X(vkEnumeratePhysicalDevices)                                              \
    X(vkGetPhysicalDeviceProperties)                                           \
    X(vkGetPhysicalDeviceProperties2)                                          \
    X(vkGetPhysicalDeviceMemoryProperties)                                     \
    X(vkGetPhysicalDeviceQueueFamilyProperties)                                \
    X(vkGetPhysicalDeviceFeatures)                                             \
    X(vkEnumerateDeviceExtensionProperties)                                    \
    X(vkCreateDevice)                                                          \
    X(vkGetDeviceProcAddr)

/* Entry points resolved from a VkDevice. */
#define ART_VK_DEVICE_FUNCS(X)                                                 \
    X(vkDestroyDevice)                                                         \
    X(vkDeviceWaitIdle)                                                        \
    X(vkGetDeviceQueue)                                                        \
    X(vkQueueSubmit)                                                           \
    X(vkQueueWaitIdle)                                                         \
    X(vkAllocateMemory)                                                        \
    X(vkFreeMemory)                                                            \
    X(vkMapMemory)                                                             \
    X(vkUnmapMemory)                                                           \
    X(vkFlushMappedMemoryRanges)                                               \
    X(vkInvalidateMappedMemoryRanges)                                          \
    X(vkCreateBuffer)                                                          \
    X(vkDestroyBuffer)                                                         \
    X(vkGetBufferMemoryRequirements)                                           \
    X(vkBindBufferMemory)                                                      \
    X(vkCreateShaderModule)                                                    \
    X(vkDestroyShaderModule)                                                   \
    X(vkCreateDescriptorSetLayout)                                             \
    X(vkDestroyDescriptorSetLayout)                                            \
    X(vkCreateDescriptorPool)                                                  \
    X(vkDestroyDescriptorPool)                                                 \
    X(vkResetDescriptorPool)                                                   \
    X(vkAllocateDescriptorSets)                                                \
    X(vkUpdateDescriptorSets)                                                  \
    X(vkCreatePipelineLayout)                                                   \
    X(vkDestroyPipelineLayout)                                                  \
    X(vkCreateComputePipelines)                                                \
    X(vkDestroyPipeline)                                                        \
    X(vkCreatePipelineCache)                                                    \
    X(vkDestroyPipelineCache)                                                   \
    X(vkGetPipelineCacheData)                                                   \
    X(vkCreateCommandPool)                                                      \
    X(vkDestroyCommandPool)                                                     \
    X(vkResetCommandPool)                                                       \
    X(vkAllocateCommandBuffers)                                                 \
    X(vkFreeCommandBuffers)                                                     \
    X(vkBeginCommandBuffer)                                                     \
    X(vkEndCommandBuffer)                                                       \
    X(vkCmdBindPipeline)                                                        \
    X(vkCmdBindDescriptorSets)                                                  \
    X(vkCmdPushConstants)                                                       \
    X(vkCmdDispatch)                                                            \
    X(vkCmdPipelineBarrier)                                                     \
    X(vkCmdCopyBuffer)                                                          \
    X(vkCmdFillBuffer)                                                          \
    X(vkCmdUpdateBuffer)                                                        \
    X(vkCmdResetQueryPool)                                                      \
    X(vkCmdWriteTimestamp)                                                      \
    X(vkCreateQueryPool)                                                        \
    X(vkDestroyQueryPool)                                                       \
    X(vkGetQueryPoolResults)                                                    \
    X(vkCreateFence)                                                            \
    X(vkDestroyFence)                                                           \
    X(vkResetFences)                                                            \
    X(vkWaitForFences)                                                          \
    X(vkGetFenceStatus)

#define ART_VK_DECLARE(name) extern PFN_##name name;
ART_VK_GLOBAL_FUNCS(ART_VK_DECLARE)
ART_VK_INSTANCE_FUNCS(ART_VK_DECLARE)
ART_VK_DEVICE_FUNCS(ART_VK_DECLARE)
#undef ART_VK_DECLARE

namespace rtengine {
namespace gpu {

/* Idempotent. `err` receives a human-readable reason on failure. */
bool loadVulkanLibrary(std::string &err);

void loadInstanceFuncs(VkInstance inst);
void loadDeviceFuncs(VkDevice dev);

/* Only for cleanup(); entry points become invalid. */
void unloadVulkanLibrary();

const char *vkResultName(VkResult r);

} // namespace gpu
} // namespace rtengine

#endif // ART_GPU_VK_API_H
