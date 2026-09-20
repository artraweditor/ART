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

#include "vk_context.h"

#include "gpu.h"
#include "vk_pass.h"
#include "../settings.h"

#include <glibmm/fileutils.h>
#include <glib/gstdio.h>
#include <glibmm/miscutils.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

namespace rtengine {

extern const Settings *settings;

namespace gpu {

namespace {

Glib::ustring cfg_user_settings_dir_;
Glib::ustring cfg_device_preference_("off");
bool cfg_allow_software_ = false;

Context *ctx_ = nullptr;
bool ctx_tried_ = false;
std::mutex ctx_mutex_;
std::string ctx_error_;

std::string envOr(const char *name, const char *dflt)
{
    const char *v = std::getenv(name);
    return (v && *v) ? std::string(v) : std::string(dflt);
}

bool envFlag(const char *name)
{
    const char *v = std::getenv(name);
    return v && *v && std::strcmp(v, "0") != 0;
}

std::string lower(const std::string &s)
{
    std::string r(s);
    std::transform(r.begin(), r.end(), r.begin(), ::tolower);
    return r;
}

bool looksLikeSoftware(const std::string &device_name,
                       VkPhysicalDeviceType type)
{
    if (type == VK_PHYSICAL_DEVICE_TYPE_CPU) {
        return true;
    }
    const std::string n = lower(device_name);
    return n.find("llvmpipe") != std::string::npos ||
           n.find("lavapipe") != std::string::npos ||
           n.find("swiftshader") != std::string::npos;
}

DeviceInfo::Kind kindOf(VkPhysicalDeviceType t, bool software)
{
    if (software) {
        return DeviceInfo::SOFTWARE;
    }
    switch (t) {
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return DeviceInfo::DISCRETE;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return DeviceInfo::INTEGRATED;
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: return DeviceInfo::VIRTUAL;
    default: return DeviceInfo::OTHER;
    }
}

int scoreOf(DeviceInfo::Kind k)
{
    switch (k) {
    case DeviceInfo::DISCRETE: return 1000;
    case DeviceInfo::INTEGRATED: return 500;
    case DeviceInfo::VIRTUAL: return 100;
    case DeviceInfo::SOFTWARE: return 0;
    default: return 50;
    }
}

/* Two-attempt macOS dance, see doc/gpu_pipeline.md §2.2: MoltenVK directly
 * wants VK_KHR_portability_enumeration absent, a loader >=1.3.216 wants it
 * present; try with, then without. Measured on MoltenVK 1.4.2: with -> -7,
 * without -> 0. */
VkResult createInstance(VkInstance *out, bool &portability_requested)
{
    VkApplicationInfo ai;
    std::memset(&ai, 0, sizeof(ai));
    ai.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    ai.pApplicationName = "ART";
    ai.apiVersion = VK_API_VERSION_1_1;

    const char *exts[1] = {"VK_KHR_portability_enumeration"};
    const char *layers[1] = {"VK_LAYER_KHRONOS_validation"};
    const bool want_validation = envFlag("ART_VULKAN_VALIDATION");

    VkInstanceCreateInfo ci;
    std::memset(&ci, 0, sizeof(ci));
    ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo = &ai;
    ci.enabledExtensionCount = 1;
    ci.ppEnabledExtensionNames = exts;
    ci.flags = 0x00000001; // VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR
    if (want_validation) {
        ci.enabledLayerCount = 1;
        ci.ppEnabledLayerNames = layers;
    }

    portability_requested = true;
    VkResult r = vkCreateInstance(&ci, nullptr, out);
    if (r == VK_SUCCESS) {
        return r;
    }

    portability_requested = false;
    ci.enabledExtensionCount = 0;
    ci.ppEnabledExtensionNames = nullptr;
    ci.flags = 0;
    r = vkCreateInstance(&ci, nullptr, out);
    if (r != VK_SUCCESS && want_validation) {
        // layers are a loader feature, unavailable via direct MoltenVK
        ci.enabledLayerCount = 0;
        ci.ppEnabledLayerNames = nullptr;
        r = vkCreateInstance(&ci, nullptr, out);
    }
    return r;
}

bool deviceHasExtension(VkPhysicalDevice pd, const char *name)
{
    unsigned int n = 0;
    if (vkEnumerateDeviceExtensionProperties(pd, nullptr, &n, nullptr) !=
            VK_SUCCESS ||
        !n) {
        return false;
    }
    std::vector<VkExtensionProperties> props(n);
    if (vkEnumerateDeviceExtensionProperties(pd, nullptr, &n, &props[0]) !=
        VK_SUCCESS) {
        return false;
    }
    for (unsigned int i = 0; i < n; ++i) {
        if (std::strcmp(props[i].extensionName, name) == 0) {
            return true;
        }
    }
    return false;
}

/* Prefer a compute-only family (an async compute queue on AMD/NVIDIA, useful
 * since ART shares the GPU with the desktop compositor); else any family with
 * COMPUTE. -1 if none. */
int pickQueueFamily(VkPhysicalDevice pd)
{
    unsigned int n = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(pd, &n, nullptr);
    if (!n) {
        return -1;
    }
    std::vector<VkQueueFamilyProperties> qf(n);
    vkGetPhysicalDeviceQueueFamilyProperties(pd, &n, &qf[0]);

    int any_compute = -1;
    for (unsigned int i = 0; i < n; ++i) {
        if (!(qf[i].queueFlags & VK_QUEUE_COMPUTE_BIT) || !qf[i].queueCount) {
            continue;
        }
        if (!(qf[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            return (int)i;
        }
        if (any_compute < 0) {
            any_compute = (int)i;
        }
    }
    return any_compute;
}

} // namespace

//-----------------------------------------------------------------------------
// Caps
//-----------------------------------------------------------------------------

Caps::Caps():
    api_version(0), max_workgroup_invocations(0),
    max_shared_memory_bytes(16384), max_push_constants_bytes(128),
    max_storage_buffer_range(134217728), min_storage_buffer_offset_alignment(256),
    non_coherent_atom_size(256), buffer_image_granularity(1),
    max_memory_allocation_count(4096), timestamp_period_ns(0.f),
    timestamp_valid_bits(0), subgroup_size(0), subgroup_arithmetic(false),
    subgroup_in_compute(false), push_descriptor(false),
    external_memory_host(false), min_imported_host_pointer_alignment(0),
    portability_subset(false), host_visible_device_local(false),
    unified_memory(false), device_local_bytes(0),
    is_software(false)
{
    max_workgroup_count[0] = max_workgroup_count[1] = max_workgroup_count[2] = 0;
    max_workgroup_size[0] = max_workgroup_size[1] = max_workgroup_size[2] = 0;
    preferred_local_size[0] = 16;
    preferred_local_size[1] = 16;
}

//-----------------------------------------------------------------------------
// Buffer
//-----------------------------------------------------------------------------

Buffer::Buffer():
    buf_(VK_NULL_HANDLE), mem_(VK_NULL_HANDLE), mapped_(nullptr), size_(0),
    coherent_(true), recorded_(false)
{
}

Buffer::Buffer(Buffer &&o):
    buf_(o.buf_), mem_(o.mem_), mapped_(o.mapped_), size_(o.size_),
    coherent_(o.coherent_), recorded_(o.recorded_)
{
    o.buf_ = VK_NULL_HANDLE;
    o.mem_ = VK_NULL_HANDLE;
    o.mapped_ = nullptr;
    o.size_ = 0;
}

Buffer &Buffer::operator=(Buffer &&o)
{
    if (this != &o) {
        reset();
        buf_ = o.buf_;
        mem_ = o.mem_;
        mapped_ = o.mapped_;
        size_ = o.size_;
        coherent_ = o.coherent_;
        recorded_ = o.recorded_;
        o.buf_ = VK_NULL_HANDLE;
        o.mem_ = VK_NULL_HANDLE;
        o.mapped_ = nullptr;
        o.size_ = 0;
        o.recorded_ = false;
    }
    return *this;
}

Buffer::~Buffer() { reset(); }

void Buffer::reset()
{
    if (recorded_ && buf_ != VK_NULL_HANDLE) {
        logOnce("GPU: a buffer was destroyed or reassigned while a Pass still "
                "referenced it; batch the op differently or keep the buffer "
                "alive until submitAndWait()");
    }
    recorded_ = false;
    Context *c = ctx_;   // do not re-open the device while tearing down
    if (!c || !c->device()) {
        buf_ = VK_NULL_HANDLE;
        mem_ = VK_NULL_HANDLE;
        mapped_ = nullptr;
        size_ = 0;
        return;
    }
    if (mapped_ && mem_ != VK_NULL_HANDLE) {
        vkUnmapMemory(c->device(), mem_);
        mapped_ = nullptr;
    }
    if (buf_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(c->device(), buf_, nullptr);
        buf_ = VK_NULL_HANDLE;
    }
    if (mem_ != VK_NULL_HANDLE) {
        vkFreeMemory(c->device(), mem_, nullptr);
        mem_ = VK_NULL_HANDLE;
        removeAllocation();
    }
    size_ = 0;
}

void Buffer::flush(size_t offset, size_t len)
{
    if (coherent_ || !mapped_ || !ctx_) {
        return;
    }
    const size_t atom = (size_t)ctx_->caps().non_coherent_atom_size;
    const size_t begin = (offset / atom) * atom;
    size_t end = offset + len;
    end = ((end + atom - 1) / atom) * atom;
    if (end > size_) {
        end = size_;
    }
    VkMappedMemoryRange r;
    std::memset(&r, 0, sizeof(r));
    r.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    r.memory = mem_;
    r.offset = begin;
    r.size = end - begin;
    vkFlushMappedMemoryRanges(ctx_->device(), 1, &r);
}

void Buffer::invalidate(size_t offset, size_t len)
{
    if (coherent_ || !mapped_ || !ctx_) {
        return;
    }
    const size_t atom = (size_t)ctx_->caps().non_coherent_atom_size;
    const size_t begin = (offset / atom) * atom;
    size_t end = offset + len;
    end = ((end + atom - 1) / atom) * atom;
    if (end > size_) {
        end = size_;
    }
    VkMappedMemoryRange r;
    std::memset(&r, 0, sizeof(r));
    r.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    r.memory = mem_;
    r.offset = begin;
    r.size = end - begin;
    vkInvalidateMappedMemoryRanges(ctx_->device(), 1, &r);
}

//-----------------------------------------------------------------------------
// Context
//-----------------------------------------------------------------------------

Context::Context():
    inst_(VK_NULL_HANDLE), pdev_(VK_NULL_HANDLE), dev_(VK_NULL_HANDLE),
    queue_(VK_NULL_HANDLE), queue_family_(0),
    pipeline_cache_(VK_NULL_HANDLE), device_lost_(false)
{
}

Context::~Context()
{
    if (dev_) {
        vkDeviceWaitIdle(dev_);
        flushPipelineCacheToDisk();
        if (pipeline_cache_) {
            vkDestroyPipelineCache(dev_, pipeline_cache_, nullptr);
            pipeline_cache_ = VK_NULL_HANDLE;
        }
        for (size_t i = 0; i < pools_.size(); ++i) {
            vkDestroyCommandPool(dev_, pools_[i], nullptr);
        }
        pools_.clear();
        vkDestroyDevice(dev_, nullptr);
        dev_ = VK_NULL_HANDLE;
    }
    if (inst_) {
        vkDestroyInstance(inst_, nullptr);
        inst_ = VK_NULL_HANDLE;
    }
}

void Context::configure(const Glib::ustring &user_settings_dir,
                        const Glib::ustring &device_preference,
                        bool allow_software)
{
    cfg_user_settings_dir_ = user_settings_dir;
    cfg_device_preference_ = device_preference;
    cfg_allow_software_ = allow_software || envFlag("ART_VULKAN_ALLOW_SOFTWARE");

    const std::string art_gpu = envOr("ART_GPU", "");
    if (art_gpu == "0") {
        cfg_device_preference_ = "off";
    } else if (art_gpu == "1" || art_gpu == "force") {
        if (cfg_device_preference_.empty() || cfg_device_preference_ == "off") {
            cfg_device_preference_ = "auto";
        }
    }
    const std::string dev = envOr("ART_VULKAN_DEVICE", "");
    if (!dev.empty()) {
        cfg_device_preference_ = dev;
    }
}

Context *Context::get()
{
    std::lock_guard<std::mutex> lock(ctx_mutex_);
    if (ctx_tried_) {
        return ctx_;
    }
    ctx_tried_ = true;

    if (cfg_device_preference_.empty() || cfg_device_preference_ == "off") {
        ctx_error_ = "";   // deliberately off; not an error, say nothing
        return nullptr;
    }

    Context *c = new Context;
    std::string err;
    if (!c->open(err)) {
        ctx_error_ = err;
        delete c;
        return nullptr;
    }
    ctx_ = c;
    return ctx_;
}

void Context::destroy()
{
    std::lock_guard<std::mutex> lock(ctx_mutex_);
    delete ctx_;
    ctx_ = nullptr;
    ctx_tried_ = false;
}

std::vector<DeviceInfo> Context::enumerate()
{
    std::vector<DeviceInfo> out;
    std::string err;
    if (!loadVulkanLibrary(err)) {
        return out;
    }

    VkInstance inst = VK_NULL_HANDLE;
    bool portability = false;
    if (createInstance(&inst, portability) != VK_SUCCESS) {
        return out;
    }
    loadInstanceFuncs(inst);

    unsigned int n = 0;
    if (vkEnumeratePhysicalDevices(inst, &n, nullptr) != VK_SUCCESS || !n) {
        vkDestroyInstance(inst, nullptr);
        return out;
    }
    std::vector<VkPhysicalDevice> pds(n);
    vkEnumeratePhysicalDevices(inst, &n, &pds[0]);

    for (unsigned int i = 0; i < n; ++i) {
        VkPhysicalDeviceProperties p;
        vkGetPhysicalDeviceProperties(pds[i], &p);
        const bool sw = looksLikeSoftware(p.deviceName, p.deviceType);

        DeviceInfo d;
        d.index = (int)i;
        d.name = p.deviceName;
        d.api_version = p.apiVersion;
        d.kind = kindOf(p.deviceType, sw);

        VkPhysicalDeviceMemoryProperties mp;
        vkGetPhysicalDeviceMemoryProperties(pds[i], &mp);
        for (unsigned int h = 0; h < mp.memoryHeapCount; ++h) {
            if (mp.memoryHeaps[h].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                if (mp.memoryHeaps[h].size > d.device_local_bytes) {
                    d.device_local_bytes = mp.memoryHeaps[h].size;
                }
            }
        }
        for (unsigned int t = 0; t < mp.memoryTypeCount; ++t) {
            const VkMemoryPropertyFlags f = mp.memoryTypes[t].propertyFlags;
            if ((f & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) &&
                (f & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
                d.host_visible_device_local = true;
                break;
            }
        }
        d.unified_memory = d.kind != DeviceInfo::DISCRETE;
        std::ostringstream drv;
        drv << "driverVersion " << p.driverVersion;
        d.driver = drv.str();
        out.push_back(d);
    }

    vkDestroyInstance(inst, nullptr);
    return out;
}

bool Context::pickPhysicalDevice(VkInstance inst, std::string &err)
{
    unsigned int n = 0;
    if (vkEnumeratePhysicalDevices(inst, &n, nullptr) != VK_SUCCESS || !n) {
        err = "no Vulkan physical device found";
        return false;
    }
    std::vector<VkPhysicalDevice> pds(n);
    vkEnumeratePhysicalDevices(inst, &n, &pds[0]);

    const std::string pref(cfg_device_preference_.raw());
    const bool by_index =
        !pref.empty() &&
        pref.find_first_not_of("0123456789") == std::string::npos;
    const long want_index = by_index ? std::atol(pref.c_str()) : -1;
    const std::string want_name = by_index ? std::string() : lower(pref);
    const bool auto_pick = (pref == "auto");

    int best = -1;
    int best_score = -1;
    std::string rejected;

    for (unsigned int i = 0; i < n; ++i) {
        VkPhysicalDeviceProperties p;
        vkGetPhysicalDeviceProperties(pds[i], &p);

        if (VK_VERSION_MAJOR(p.apiVersion) == 1 &&
            VK_VERSION_MINOR(p.apiVersion) < 1) {
            continue;   // we require Vulkan 1.1 core
        }
        if (pickQueueFamily(pds[i]) < 0) {
            continue;   // no compute queue
        }

        const bool sw = looksLikeSoftware(p.deviceName, p.deviceType);
        if (sw && !cfg_allow_software_) {
            // slower than the OpenMP CPU path; opt-in only, see §2.2
            if (rejected.empty()) {
                rejected = std::string(p.deviceName) +
                           " is a software rasteriser (set "
                           "ART_VULKAN_ALLOW_SOFTWARE=1 to use it)";
            }
            continue;
        }

        if (by_index) {
            if ((long)i != want_index) {
                continue;
            }
            best = (int)i;
            break;
        }
        if (!auto_pick && !want_name.empty()) {
            if (lower(p.deviceName).find(want_name) == std::string::npos) {
                continue;
            }
            best = (int)i;
            break;
        }

        const int score = scoreOf(kindOf(p.deviceType, sw));
        if (score > best_score) {
            best_score = score;
            best = (int)i;
        }
    }

    if (best < 0) {
        err = rejected.empty() ? "no suitable Vulkan device (need 1.1 with a "
                                 "compute queue)"
                               : rejected;
        return false;
    }
    pdev_ = pds[best];
    info_.index = best;
    return true;
}

void Context::queryCaps()
{
    VkPhysicalDeviceProperties p;
    vkGetPhysicalDeviceProperties(pdev_, &p);

    caps_.api_version = p.apiVersion;
    for (int i = 0; i < 3; ++i) {
        caps_.max_workgroup_count[i] = p.limits.maxComputeWorkGroupCount[i];
        caps_.max_workgroup_size[i] = p.limits.maxComputeWorkGroupSize[i];
    }
    caps_.max_workgroup_invocations = p.limits.maxComputeWorkGroupInvocations;
    caps_.max_shared_memory_bytes = p.limits.maxComputeSharedMemorySize;
    caps_.max_push_constants_bytes = p.limits.maxPushConstantsSize;
    caps_.max_storage_buffer_range = p.limits.maxStorageBufferRange;
    caps_.min_storage_buffer_offset_alignment =
        p.limits.minStorageBufferOffsetAlignment;
    caps_.non_coherent_atom_size = p.limits.nonCoherentAtomSize;
    caps_.buffer_image_granularity = p.limits.bufferImageGranularity;
    caps_.max_memory_allocation_count = p.limits.maxMemoryAllocationCount;
    caps_.timestamp_period_ns = p.limits.timestampPeriod;
    caps_.is_software = looksLikeSoftware(p.deviceName, p.deviceType);

    unsigned int nq = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(pdev_, &nq, nullptr);
    if (nq) {
        std::vector<VkQueueFamilyProperties> qf(nq);
        vkGetPhysicalDeviceQueueFamilyProperties(pdev_, &nq, &qf[0]);
        if (queue_family_ < nq) {
            caps_.timestamp_valid_bits = qf[queue_family_].timestampValidBits;
        }
    }

    caps_.portability_subset =
        deviceHasExtension(pdev_, "VK_KHR_portability_subset");
    caps_.push_descriptor =
        deviceHasExtension(pdev_, "VK_KHR_push_descriptor");
    caps_.external_memory_host =
        deviceHasExtension(pdev_, "VK_EXT_external_memory_host");

    if (vkGetPhysicalDeviceProperties2) {
        VkPhysicalDeviceSubgroupProperties sg;
        std::memset(&sg, 0, sizeof(sg));
        sg.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;

        VkPhysicalDeviceExternalMemoryHostPropertiesEXT hp;
        std::memset(&hp, 0, sizeof(hp));
        hp.sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_HOST_PROPERTIES_EXT;

        VkPhysicalDeviceProperties2 p2;
        std::memset(&p2, 0, sizeof(p2));
        p2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        p2.pNext = &sg;
        if (caps_.external_memory_host) {
            sg.pNext = &hp;
        }
        vkGetPhysicalDeviceProperties2(pdev_, &p2);

        caps_.subgroup_size = sg.subgroupSize;
        caps_.subgroup_arithmetic =
            (sg.supportedOperations & VK_SUBGROUP_FEATURE_ARITHMETIC_BIT) != 0;
        caps_.subgroup_in_compute =
            (sg.supportedStages & VK_SHADER_STAGE_COMPUTE_BIT) != 0;
        caps_.min_imported_host_pointer_alignment =
            hp.minImportedHostPointerAlignment;
    }

    /* queryCaps() runs after info_.kind is filled in (Context::open), so the
     * device type is available here.  Deliberately not derived from the
     * memory types: see the comment on host_visible_device_local. */
    caps_.unified_memory = info_.kind != DeviceInfo::DISCRETE;

    vkGetPhysicalDeviceMemoryProperties(pdev_, &mem_props_);
    for (unsigned int t = 0; t < mem_props_.memoryTypeCount; ++t) {
        const VkMemoryPropertyFlags f = mem_props_.memoryTypes[t].propertyFlags;
        if ((f & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) &&
            (f & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
            caps_.host_visible_device_local = true;
            break;
        }
    }
    for (unsigned int h = 0; h < mem_props_.memoryHeapCount; ++h) {
        if ((mem_props_.memoryHeaps[h].flags &
             VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) &&
            mem_props_.memoryHeaps[h].size > caps_.device_local_bytes) {
            caps_.device_local_bytes = mem_props_.memoryHeaps[h].size;
        }
    }

    // small integrated parts prefer smaller groups; keep variant count tiny,
    // since each is a fresh SPIRV-Cross->MSL compile on MoltenVK
    unsigned int lx = 16, ly = 16;
    if (caps_.max_workgroup_invocations < 256) {
        lx = 8;
        ly = 8;
    } else if (info_.kind == DeviceInfo::INTEGRATED) {
        lx = 16;
        ly = 8;
    }
    if (lx > caps_.max_workgroup_size[0]) {
        lx = caps_.max_workgroup_size[0];
    }
    if (ly > caps_.max_workgroup_size[1]) {
        ly = caps_.max_workgroup_size[1];
    }
    caps_.preferred_local_size[0] = lx ? lx : 1;
    caps_.preferred_local_size[1] = ly ? ly : 1;
}

bool Context::open(std::string &err)
{
    if (!loadVulkanLibrary(err)) {
        return false;
    }

    bool portability = false;
    VkResult r = createInstance(&inst_, portability);
    if (r != VK_SUCCESS) {
        err = std::string("vkCreateInstance failed: ") + vkResultName(r);
        return false;
    }
    loadInstanceFuncs(inst_);

    if (!pickPhysicalDevice(inst_, err)) {
        return false;
    }

    const int qf = pickQueueFamily(pdev_);
    if (qf < 0) {
        err = "selected device has no compute queue";
        return false;
    }
    queue_family_ = (unsigned int)qf;

    VkPhysicalDeviceProperties p;
    vkGetPhysicalDeviceProperties(pdev_, &p);
    const bool sw = looksLikeSoftware(p.deviceName, p.deviceType);
    info_.name = p.deviceName;
    info_.api_version = p.apiVersion;
    info_.kind = kindOf(p.deviceType, sw);
    {
        std::ostringstream drv;
        drv << "driverVersion " << p.driverVersion;
        info_.driver = drv.str();
    }

    /* The spec *requires* VK_KHR_portability_subset to be enabled when the
     * device advertises it, which MoltenVK always does. */
    std::vector<const char *> dev_exts;
    if (deviceHasExtension(pdev_, "VK_KHR_portability_subset")) {
        dev_exts.push_back("VK_KHR_portability_subset");
    }

    const float prio = 1.f;
    VkDeviceQueueCreateInfo qci;
    std::memset(&qci, 0, sizeof(qci));
    qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qci.queueFamilyIndex = queue_family_;
    qci.queueCount = 1;
    qci.pQueuePriorities = &prio;

    VkDeviceCreateInfo dci;
    std::memset(&dci, 0, sizeof(dci));
    dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;
    dci.enabledExtensionCount = (unsigned int)dev_exts.size();
    dci.ppEnabledExtensionNames = dev_exts.empty() ? nullptr : &dev_exts[0];

    r = vkCreateDevice(pdev_, &dci, nullptr, &dev_);
    if (r != VK_SUCCESS) {
        err = std::string("vkCreateDevice failed: ") + vkResultName(r);
        dev_ = VK_NULL_HANDLE;
        return false;
    }
    loadDeviceFuncs(dev_);
    vkGetDeviceQueue(dev_, queue_family_, 0, &queue_);

    queryCaps();
    loadPipelineCacheFromDisk();

    {
        std::ostringstream d;
        d << info_.name << " (";
        switch (info_.kind) {
        case DeviceInfo::DISCRETE: d << "discrete"; break;
        case DeviceInfo::INTEGRATED: d << "integrated"; break;
        case DeviceInfo::VIRTUAL: d << "virtual"; break;
        case DeviceInfo::SOFTWARE: d << "software"; break;
        default: d << "other"; break;
        }
        d << ", Vulkan " << VK_VERSION_MAJOR(caps_.api_version) << "."
          << VK_VERSION_MINOR(caps_.api_version) << "."
          << VK_VERSION_PATCH(caps_.api_version);
        if (caps_.portability_subset) {
            d << ", portability subset";
        }
        if (caps_.unified_memory) {
            d << ", unified memory";
        } else if (caps_.host_visible_device_local) {
            /* Worth printing: it is why mapped() comes back non-null on a
             * discrete GPU, which is the difference between a memcpy and an
             * uncached read across PCIe. */
            d << ", host-visible VRAM";
        }
        d << ")";
        description_ = d.str();
    }

    if (settings && settings->verbose) {
        std::cout << "GPU: using " << description_ << std::endl;
        if (settings->verbose > 1) {
            std::cout << "GPU:   workgroup " << caps_.preferred_local_size[0]
                      << "x" << caps_.preferred_local_size[1]
                      << ", shared mem " << caps_.max_shared_memory_bytes
                      << " B, push const " << caps_.max_push_constants_bytes
                      << " B, subgroup " << caps_.subgroup_size
                      << (caps_.subgroup_arithmetic ? " (arithmetic)" : "")
                      << ", timestamps "
                      << (caps_.timestampsUsable() ? "yes" : "no")
                      << "\nGPU:   device-local heap "
                      << (caps_.device_local_bytes >> 20) << " MiB"
                      << (caps_.unified_memory
                              ? " (unified, shared with the host)"
                              : (caps_.host_visible_device_local
                                     ? " (host-visible)"
                                     : ""))
                      << "\nGPU:   max storage buffer range "
                      << (caps_.max_storage_buffer_range >> 20)
                      << " MiB, max workgroup count "
                      << caps_.max_workgroup_count[0] << "x"
                      << caps_.max_workgroup_count[1] << std::endl;
        }
    }
    return true;
}

VkResult Context::submit(VkCommandBuffer cb, VkFence fence)
{
    VkSubmitInfo si;
    std::memset(&si, 0, sizeof(si));
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cb;

    std::lock_guard<std::mutex> lock(submit_mutex_);
    const VkResult r = vkQueueSubmit(queue_, 1, &si, fence);
    if (r == VK_ERROR_DEVICE_LOST) {
        device_lost_ = true;
    }
    return r;
}

VkCommandPool Context::commandPoolForThisThread()
{
    // vkAllocateCommandBuffers/vkResetCommandPool need external sync on the pool
    static thread_local VkCommandPool tls_pool = VK_NULL_HANDLE;
    static thread_local Context *tls_owner = nullptr;

    if (tls_pool != VK_NULL_HANDLE && tls_owner == this) {
        return tls_pool;
    }

    VkCommandPoolCreateInfo ci;
    std::memset(&ci, 0, sizeof(ci));
    ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    ci.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
               VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    ci.queueFamilyIndex = queue_family_;

    VkCommandPool pool = VK_NULL_HANDLE;
    if (vkCreateCommandPool(dev_, &ci, nullptr, &pool) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        pools_.push_back(pool);   // owned by the context, destroyed with it
    }
    tls_pool = pool;
    tls_owner = this;
    return pool;
}

Buffer Context::allocateBuffer(size_t bytes, const VkMemoryPropertyFlags *tiers,
                               int n_tiers)
{
    Buffer b;
    if (!bytes || !dev_) {
        return b;
    }

    /* maxMemoryAllocationCount is a hard driver limit (4096 on several
     * desktop drivers), and hitting it turns every later GPU op into a silent
     * decline to the CPU -- which reads as "the GPU path randomly stopped
     * working" rather than as a resource problem.  Say so once instead. */
    if (stats().live_allocations >= caps_.max_memory_allocation_count) {
        logOnce("GPU: out of memory allocations (maxMemoryAllocationCount "
                "reached); using the CPU");
        return b;
    }

    VkBufferCreateInfo ci;
    std::memset(&ci, 0, sizeof(ci));
    ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    ci.size = bytes;
    ci.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
               VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
               VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(dev_, &ci, nullptr, &b.buf_) != VK_SUCCESS) {
        b.buf_ = VK_NULL_HANDLE;
        return b;
    }

    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(dev_, b.buf_, &req);

    // Try every tier in order; a tier whose type exists but whose backing
    // heap is too small (e.g. a ReBAR-less discrete GPU's ~256MB
    // DEVICE_LOCAL|HOST_VISIBLE window) must fall through to the next tier
    // rather than fail outright -- see doc/gpu_pipeline.md §2.2.
    int type_index = -1;
    VkMemoryPropertyFlags chosen = 0;
    for (int w = 0; w < n_tiers; ++w) {
        for (unsigned int t = 0; t < mem_props_.memoryTypeCount; ++t) {
            if (!(req.memoryTypeBits & (1u << t))) {
                continue;
            }
            const VkMemoryPropertyFlags f =
                mem_props_.memoryTypes[t].propertyFlags;
            if ((f & tiers[w]) != tiers[w]) {
                continue;
            }

            VkMemoryAllocateInfo ai;
            std::memset(&ai, 0, sizeof(ai));
            ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            ai.allocationSize = req.size;
            ai.memoryTypeIndex = t;

            if (vkAllocateMemory(dev_, &ai, nullptr, &b.mem_) == VK_SUCCESS) {
                type_index = (int)t;
                chosen = f;
                addAllocation();
            }
            break;    // first matching type per tier, win or lose
        }
        if (type_index >= 0) {
            break;
        }
    }
    if (type_index < 0) {
        vkDestroyBuffer(dev_, b.buf_, nullptr);
        b.buf_ = VK_NULL_HANDLE;
        return b;
    }

    if (vkBindBufferMemory(dev_, b.buf_, b.mem_, 0) != VK_SUCCESS) {
        b.reset();
        return b;
    }

    b.size_ = bytes;
    b.coherent_ = (chosen & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;
    if (chosen & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        if (vkMapMemory(dev_, b.mem_, 0, VK_WHOLE_SIZE, 0, &b.mapped_) !=
            VK_SUCCESS) {
            b.mapped_ = nullptr;
        }
    }
    return b;
}

/* Whether a PREFER_DEVICE_LOCAL buffer may land in memory that is both
 * DEVICE_LOCAL and HOST_VISIBLE.
 *
 * On a unified-memory device that is simply how all memory is, and taking it
 * makes every transfer a memcpy with no submission at all.
 *
 * On a discrete GPU the same memory type is the PCIe BAR window, and the
 * theory was that mapping it is a trap: the pointer is write-combined and
 * uncached, so host *reads* in ImageResidency::download() and downloadPlane()
 * cross PCIe uncached.  **Measurement says otherwise**, at least on an RTX
 * 4500 Ada with resizable BAR: routing those transfers through the staging
 * path instead took one export from 16.4 s to 54.7 s, with `denoise:out`
 * alone going 1.5 s -> 33.8 s.
 *
 * The reason is that the staging path is far more expensive than the copy it
 * replaces, not that BAR reads are cheap.  plane_io.h packs each plane into a
 * fresh zero-initialised std::vector, memcpys that into a staging buffer, and
 * only then issues the device copy -- three passes over 169 MB per plane per
 * direction, plus an allocation, against the one memcpy the mapped path does.
 * Fixing that is the prerequisite for revisiting this default; until then,
 * mapping wins wherever it is available.
 *
 * So: take the mapped path whenever the device offers it, which is what this
 * backend did before the tier was made configurable.  ART_GPU_HOST_VISIBLE_
 * DEVICE_LOCAL=0/1 forces it either way, and =0 is how to measure the staging
 * path again once plane_io.h no longer copies three times. */
bool Context::hostVisibleDeviceLocalWanted() const
{
    const char *v = std::getenv("ART_GPU_HOST_VISIBLE_DEVICE_LOCAL");
    if (v && *v) {
        return std::strcmp(v, "0") != 0;
    }
    return true;
}

Buffer Context::createBuffer(size_t bytes, HostMemoryMode mode)
{
    static const VkMemoryPropertyFlags prefer_device_local[3] = {
        VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
        VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT),
        VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};
    static const VkMemoryPropertyFlags device_local_only[1] = {
        VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};
    // no tier requires DEVICE_LOCAL: every conformant device has some
    // HOST_VISIBLE|HOST_COHERENT type, which is all staging needs
    static const VkMemoryPropertyFlags staging_only[2] = {
        VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
        VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)};

    switch (mode) {
    case HostMemoryMode::DEVICE_LOCAL_ONLY:
        return allocateBuffer(bytes, device_local_only, 1);
    case HostMemoryMode::STAGING_ONLY:
        return allocateBuffer(bytes, staging_only, 2);
    case HostMemoryMode::PREFER_DEVICE_LOCAL:
    default:
        // debug aid, see doc/gpu_pipeline.md §4 (ART_GPU_FORCE_DISCRETE_STAGING)
        static const bool force_staging = envFlag("ART_GPU_FORCE_DISCRETE_STAGING");
        if (force_staging || !hostVisibleDeviceLocalWanted()) {
            return allocateBuffer(bytes, &prefer_device_local[2], 1);
        }
        return allocateBuffer(bytes, prefer_device_local, 3);
    }
}

Buffer Context::createBuffer(size_t bytes, bool want_host_visible)
{
    return createBuffer(bytes, want_host_visible
                                    ? HostMemoryMode::PREFER_DEVICE_LOCAL
                                    : HostMemoryMode::DEVICE_LOCAL_ONLY);
}

BufferPool &Context::stagingPoolForThisThread()
{
    static thread_local std::unique_ptr<BufferPool> tls_pool;
    static thread_local Context *tls_owner = nullptr;

    if (tls_pool && tls_owner == this) {
        return *tls_pool;
    }
    tls_pool.reset(new BufferPool(*this, HostMemoryMode::STAGING_ONLY));
    tls_owner = this;
    return *tls_pool;
}

void Context::markDeviceLost(const char *where)
{
    if (!device_lost_) {
        device_lost_ = true;
        std::cerr << "GPU: device lost in " << (where ? where : "?")
                  << "; falling back to the CPU for the rest of this session"
                  << std::endl;
    }
}

void Context::loadPipelineCacheFromDisk()
{
    std::vector<char> blob;

    if (!cfg_user_settings_dir_.empty()) {
        VkPhysicalDeviceProperties p;
        vkGetPhysicalDeviceProperties(pdev_, &p);
        std::ostringstream name;
        name << "vulkan-pipelines-";
        for (unsigned i = 0; i < VK_UUID_SIZE; ++i) {
            char hex[3];
            std::snprintf(hex, sizeof(hex), "%02x", p.pipelineCacheUUID[i]);
            name << hex;
        }
        name << ".bin";
        // UUID in the filename: a driver update or GPU swap can't feed stale data in
        pipeline_cache_path_ = Glib::build_filename(
            cfg_user_settings_dir_, "cache", name.str());

        std::ifstream f(pipeline_cache_path_.c_str(),
                        std::ios::binary | std::ios::ate);
        if (f) {
            const std::streamsize sz = f.tellg();
            if (sz > 32) {
                blob.resize((size_t)sz);
                f.seekg(0);
                f.read(&blob[0], sz);

                // validate header: some drivers are UB on a corrupt blob
                unsigned int hdr_len = 0, hdr_ver = 0, vendor = 0, device = 0;
                std::memcpy(&hdr_len, &blob[0], 4);
                std::memcpy(&hdr_ver, &blob[4], 4);
                std::memcpy(&vendor, &blob[8], 4);
                std::memcpy(&device, &blob[12], 4);
                const bool ok = hdr_len >= 32 && hdr_len <= (unsigned)sz &&
                                hdr_ver == VK_PIPELINE_CACHE_HEADER_VERSION_ONE &&
                                vendor == p.vendorID && device == p.deviceID &&
                                std::memcmp(&blob[16], p.pipelineCacheUUID,
                                            VK_UUID_SIZE) == 0;
                if (!ok) {
                    blob.clear();
                    if (settings && settings->verbose) {
                        std::cout << "GPU: discarding stale pipeline cache"
                                  << std::endl;
                    }
                }
            }
        }
    }

    VkPipelineCacheCreateInfo ci;
    std::memset(&ci, 0, sizeof(ci));
    ci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    ci.initialDataSize = blob.size();
    ci.pInitialData = blob.empty() ? nullptr : &blob[0];
    if (vkCreatePipelineCache(dev_, &ci, nullptr, &pipeline_cache_) !=
        VK_SUCCESS) {
        pipeline_cache_ = VK_NULL_HANDLE;
    }
}

void Context::flushPipelineCacheToDisk()
{
    if (!pipeline_cache_ || pipeline_cache_path_.empty() || !dev_) {
        return;
    }
    size_t sz = 0;
    if (vkGetPipelineCacheData(dev_, pipeline_cache_, &sz, nullptr) !=
            VK_SUCCESS ||
        !sz) {
        return;
    }
    std::vector<char> blob(sz);
    if (vkGetPipelineCacheData(dev_, pipeline_cache_, &sz, &blob[0]) !=
        VK_SUCCESS) {
        return;
    }

    const std::string dir = Glib::path_get_dirname(pipeline_cache_path_);
    if (!Glib::file_test(dir, Glib::FILE_TEST_IS_DIR)) {
        g_mkdir_with_parents(dir.c_str(), 0755);
    }

    /* temp + rename, so a crash cannot leave a truncated cache behind */
    const std::string tmp = pipeline_cache_path_ + ".tmp";
    {
        std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc);
        if (!f) {
            return;
        }
        f.write(&blob[0], (std::streamsize)sz);
        if (!f) {
            return;
        }
    }
    std::rename(tmp.c_str(), pipeline_cache_path_.c_str());
}

const std::string &contextError() { return ctx_error_; }

Buffer *BufferPool::get(size_t bytes)
{
    if (!bytes) {
        return nullptr;
    }
    Entry *best = nullptr;
    for (size_t i = 0; i < entries_.size(); ++i) {
        Entry *e = entries_[i].get();
        if (e->in_use || e->buf.size() < bytes) {
            continue;
        }
        if (!best || e->buf.size() < best->buf.size()) {
            best = e;
        }
    }
    if (best) {
        best->in_use = true;
        checked_out_.push_back(best);
        return &best->buf;
    }

    std::unique_ptr<Entry> e(new Entry);
    e->buf = ctx_.createBuffer(bytes, mode_);
    // PREFER_DEVICE_LOCAL may legitimately come back unmapped on a discrete
    // GPU; only STAGING_ONLY requires mapped() -- see doc/gpu_pipeline.md §2.2
    if (!e->buf.valid()) {
        return nullptr;
    }
    if (mode_ == HostMemoryMode::STAGING_ONLY && !e->buf.mapped()) {
        return nullptr;
    }
    e->in_use = true;
    entries_.push_back(std::move(e));
    checked_out_.push_back(entries_.back().get());
    return &entries_.back()->buf;
}

void BufferPool::release(size_t mark)
{
    if (mark > checked_out_.size()) {
        return;
    }
    for (size_t i = mark; i < checked_out_.size(); ++i) {
        checked_out_[i]->in_use = false;
    }
    checked_out_.resize(mark);
}

void BufferPool::clear()
{
    checked_out_.clear();
    entries_.clear();
}

size_t BufferPool::bytes() const
{
    size_t n = 0;
    for (size_t i = 0; i < entries_.size(); ++i) {
        n += entries_[i]->buf.size();
    }
    return n;
}

//-----------------------------------------------------------------------------
// Staged upload/download/copy
//-----------------------------------------------------------------------------

bool uploadToBuffer(Context &ctx, BufferPool *staging_pool, const void *src,
                    size_t bytes, Buffer &dst, size_t dst_offset,
                    Pass *pass)
{
    if (!src || !bytes || !dst.valid() || dst_offset + bytes > dst.size()) {
        return false;
    }
    if (dst.mapped()) {
        std::memcpy((char *)dst.mapped() + dst_offset, src, bytes);
        dst.flush(dst_offset, bytes);
        return true;
    }
    /* Record it instead of submitting for it.  Only small payloads qualify
     * (vkCmdUpdateBuffer carries the data inside the command buffer), which
     * is exactly the LUT/parameter-block case; a full plane still stages. */
    if (pass && bytes <= Pass::maxUpdateBytes() && !(bytes % 4u) &&
        !(dst_offset % 4u)) {
        return pass->updateBuffer(dst, src, dst_offset, bytes);
    }
    if (!staging_pool) {
        logOnce("GPU: uploadToBuffer needs a staging pool for an unmapped "
                "destination");
        return false;
    }
    /* Hands the buffer back on every exit path.  Without this the staging
     * pool only ever grows: get() reuses an entry only once it has been
     * released, so each transfer would allocate fresh device memory and keep
     * it for the life of the thread -- which is the cost BufferPool exists to
     * avoid.  Safe here because the submitAndWait() below completes before
     * the scope ends, satisfying the pool's "never recycle before the
     * submission has been waited on" invariant (vk_context.h). */
    PoolScope stage_scope(*staging_pool);
    Buffer *stage = staging_pool->get(bytes);
    if (!stage) {
        return false;
    }
    std::memcpy(stage->mapped(), src, bytes);
    stage->flush(0, bytes);

    Pass staged(ctx, "upload-staging");
    if (!staged.valid()) {
        return false;
    }
    if (!staged.copyBuffer(*stage, dst, 0, dst_offset, bytes)) {
        return false;
    }
    if (!staged.submitAndWait()) {
        return false;
    }
    staged.reportTimings();
    return true;
}

bool downloadFromBuffer(Context &ctx, BufferPool *staging_pool, Buffer &src,
                        size_t src_offset, void *dst, size_t bytes)
{
    if (!dst || !bytes || !src.valid() || src_offset + bytes > src.size()) {
        return false;
    }
    if (src.mapped()) {
        src.invalidate(src_offset, bytes);
        std::memcpy(dst, (const char *)src.mapped() + src_offset, bytes);
        return true;
    }
    if (!staging_pool) {
        logOnce("GPU: downloadFromBuffer needs a staging pool for an "
                "unmapped source");
        return false;
    }
    // same reasoning as uploadToBuffer(); the readback below happens after
    // submitAndWait() but still inside the scope, so the data is read out
    // before the buffer becomes reusable.
    PoolScope stage_scope(*staging_pool);
    Buffer *stage = staging_pool->get(bytes);
    if (!stage) {
        return false;
    }

    Pass pass(ctx, "download-staging");
    if (!pass.valid()) {
        return false;
    }
    if (!pass.copyBuffer(src, *stage, src_offset, 0, bytes)) {
        return false;
    }
    if (!pass.submitAndWait()) {
        return false;
    }
    pass.reportTimings();

    stage->invalidate(0, bytes);
    std::memcpy(dst, stage->mapped(), bytes);
    return true;
}

bool copyBufferToBuffer(Context &ctx, const Buffer &src, size_t src_offset,
                        Buffer &dst, size_t dst_offset, size_t bytes)
{
    if (!bytes || !src.valid() || !dst.valid() ||
        src_offset + bytes > src.size() || dst_offset + bytes > dst.size()) {
        return false;
    }
    Pass pass(ctx, "buffer-copy");
    if (!pass.valid()) {
        return false;
    }
    if (!pass.copyBuffer(src, dst, src_offset, dst_offset, bytes)) {
        return false;
    }
    if (!pass.submitAndWait()) {
        return false;
    }
    pass.reportTimings();
    return true;
}

} // namespace gpu
} // namespace rtengine
