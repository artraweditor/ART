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

#include "vk_api.h"

#include <cstdlib>
#include <cstring>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#define ART_VK_DEFINE(name) PFN_##name name = nullptr;
ART_VK_GLOBAL_FUNCS(ART_VK_DEFINE)
ART_VK_INSTANCE_FUNCS(ART_VK_DEFINE)
ART_VK_DEVICE_FUNCS(ART_VK_DEFINE)
#undef ART_VK_DEFINE

namespace rtengine {
namespace gpu {

namespace {

#ifdef _WIN32
typedef HMODULE LibHandle;
inline LibHandle openLib(const char *p) { return LoadLibraryA(p); }
inline void *libSym(LibHandle h, const char *n)
{
    return (void *)GetProcAddress(h, n);
}
inline void closeLib(LibHandle h) { FreeLibrary(h); }
#else
typedef void *LibHandle;
inline LibHandle openLib(const char *p)
{
    return dlopen(p, RTLD_NOW | RTLD_LOCAL);
}
inline void *libSym(LibHandle h, const char *n) { return dlsym(h, n); }
inline void closeLib(LibHandle h) { dlclose(h); }
#endif

LibHandle lib_ = nullptr;
PFN_vkGetInstanceProcAddr gipa_ = nullptr;
std::mutex load_mutex_;
std::string loaded_path_;

/* Candidate library names, in preference order. macOS opens MoltenVK
 * directly rather than through a loader, which is why Vulkan layers
 * (validation) are unavailable there -- layers are a loader feature. */
const char *const *candidates()
{
#if defined(__APPLE__)
    static const char *const c[] = {"libMoltenVK.dylib",
                                    "@executable_path/../Frameworks/libMoltenVK.dylib",
                                    "libvulkan.1.dylib",
                                    "/opt/homebrew/lib/libMoltenVK.dylib",
                                    "/usr/local/lib/libMoltenVK.dylib",
                                    "/usr/local/lib/libvulkan.1.dylib",
                                    nullptr};
#elif defined(_WIN32)
    static const char *const c[] = {"vulkan-1.dll", nullptr};
#else
    static const char *const c[] = {"libvulkan.so.1", "libvulkan.so", nullptr};
#endif
    return c;
}

} // namespace

bool loadVulkanLibrary(std::string &err)
{
    std::lock_guard<std::mutex> lock(load_mutex_);

    if (gipa_) {
        return true;
    }

    const char *override_path = std::getenv("ART_VULKAN_LIBRARY");
    if (override_path && *override_path) {
        lib_ = openLib(override_path);
        if (!lib_) {
            err = std::string("could not open ART_VULKAN_LIBRARY=") +
                  override_path;
            return false;
        }
        loaded_path_ = override_path;
    } else {
        const char *const *c = candidates();
        for (int i = 0; c[i] && !lib_; ++i) {
            lib_ = openLib(c[i]);
            if (lib_) {
                loaded_path_ = c[i];
            }
        }
        if (!lib_) {
            err = "no Vulkan loader or ICD could be opened";
            return false;
        }
    }

    gipa_ =
        (PFN_vkGetInstanceProcAddr)libSym(lib_, "vkGetInstanceProcAddr");
    if (!gipa_) {
        err = loaded_path_ + " has no vkGetInstanceProcAddr";
        closeLib(lib_);
        lib_ = nullptr;
        return false;
    }

#define ART_VK_LOAD_GLOBAL(name)                                               \
    name = (PFN_##name)gipa_(VK_NULL_HANDLE, #name);
    ART_VK_GLOBAL_FUNCS(ART_VK_LOAD_GLOBAL)
#undef ART_VK_LOAD_GLOBAL

    if (!vkCreateInstance) {
        err = loaded_path_ + " does not resolve vkCreateInstance";
        return false;
    }
    return true;
}

void loadInstanceFuncs(VkInstance inst)
{
#define ART_VK_LOAD_INSTANCE(name) name = (PFN_##name)gipa_(inst, #name);
    ART_VK_INSTANCE_FUNCS(ART_VK_LOAD_INSTANCE)
    /* Replaced by device-specific pointers in loadDeviceFuncs, see §2.1. */
    ART_VK_DEVICE_FUNCS(ART_VK_LOAD_INSTANCE)
#undef ART_VK_LOAD_INSTANCE
}

void loadDeviceFuncs(VkDevice dev)
{
    if (!vkGetDeviceProcAddr) {
        return;
    }
#define ART_VK_LOAD_DEVICE(name)                                               \
    if (PFN_vkVoidFunction f = vkGetDeviceProcAddr(dev, #name)) {              \
        name = (PFN_##name)f;                                                  \
    }
    ART_VK_DEVICE_FUNCS(ART_VK_LOAD_DEVICE)
#undef ART_VK_LOAD_DEVICE
}

void unloadVulkanLibrary()
{
    std::lock_guard<std::mutex> lock(load_mutex_);
    if (!lib_) {
        return;
    }
#define ART_VK_CLEAR(name) name = nullptr;
    ART_VK_GLOBAL_FUNCS(ART_VK_CLEAR)
    ART_VK_INSTANCE_FUNCS(ART_VK_CLEAR)
    ART_VK_DEVICE_FUNCS(ART_VK_CLEAR)
#undef ART_VK_CLEAR
    gipa_ = nullptr;
    closeLib(lib_);
    lib_ = nullptr;
    loaded_path_.clear();
}

const char *vkResultName(VkResult r)
{
    switch (r) {
    case VK_SUCCESS: return "VK_SUCCESS";
    case VK_NOT_READY: return "VK_NOT_READY";
    case VK_TIMEOUT: return "VK_TIMEOUT";
    case VK_INCOMPLETE: return "VK_INCOMPLETE";
    case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED:
        return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT:
        return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
    case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
    case VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";
    case VK_ERROR_OUT_OF_POOL_MEMORY: return "VK_ERROR_OUT_OF_POOL_MEMORY";
    case VK_ERROR_INVALID_EXTERNAL_HANDLE:
        return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
    default: return "VK_ERROR_<unknown>";
    }
}

} // namespace gpu
} // namespace rtengine
