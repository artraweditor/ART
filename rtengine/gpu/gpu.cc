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

#include "gpu.h"

#include "../imagefloat.h"
#include "../settings.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include <set>
#include <sstream>

#ifdef ART_USE_VULKAN
#include "vk_context.h"
#include "vk_pipeline.h"
#endif

namespace rtengine {

extern const Settings *settings;

namespace gpu {

namespace {

std::mutex log_mutex_;
std::set<std::string> logged_;

bool degraded_ = false;
std::string status_;
std::set<std::string> disabled_ops_;
bool wanted_ = false;

std::mutex stats_mutex_;
Stats stats_;

void parseDisabledOps()
{
    const char *v = std::getenv("ART_GPU_DISABLE_OPS");
    if (!v || !*v) {
        return;
    }
    std::string s(v);
    size_t pos = 0;
    while (pos <= s.size()) {
        const size_t comma = s.find(',', pos);
        const std::string tok =
            s.substr(pos, comma == std::string::npos ? std::string::npos
                                                     : comma - pos);
        if (!tok.empty()) {
            disabled_ops_.insert(tok);
        }
        if (comma == std::string::npos) {
            break;
        }
        pos = comma + 1;
    }
}

} // namespace

void logOnce(const std::string &msg)
{
    {
        std::lock_guard<std::mutex> lock(log_mutex_);
        if (!logged_.insert(msg).second) {
            return;
        }
    }
    // never via ProgressListener: that reaches the GUI status bar on every keystroke
    std::cerr << msg << std::endl;
}

bool opEnabled(const char *op)
{
    if (disabled_ops_.empty() || !op) {
        return true;
    }
    return disabled_ops_.find(op) == disabled_ops_.end();
}

bool hasFullGPUStage(const char *op)
{
#ifdef ART_USE_VULKAN
    if (!op || !available()) {
        return false;
    }
    /* See doc/gpu_pipeline.md §3: an explicit list, not one derived from the
     * SPIR-V registry, because a kernel existing isn't sufficient -- a tool
     * only belongs here if *every* path from its apply() entry point to the
     * kernel is free of direct CPU-plane access.
     *
     * The list is empty on purpose. The tools that once qualified (exposure,
     * saturationVibrance, toneEqualizer) had their GPU paths removed once the
     * measurements showed they didn't pay for the transfers; what is left of
     * the backend accelerates work *inside* a tool (denoise, smoothing,
     * wavelets, NL-means), and each of those syncs the image back to the CPU
     * before returning. Add a name here only together with that audit. */
    static const char *const ported[] = {
        nullptr,
    };
    for (int i = 0; ported[i]; ++i) {
        if (std::strcmp(ported[i], op) == 0) {
            return opEnabled(op);
        }
    }
#else
    (void)op;
#endif
    return false;
}

double GPUTimeMs()
{
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_.submit_wall_ms;
}

Stats stats()
{
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void resetStats()
{
    std::lock_guard<std::mutex> lock(stats_mutex_);
    /* live_allocations survives: it is a property of the device, not of the
     * run being measured, and buffers outlive a single pipeline pass by
     * design (BufferPool, ImageResidency). */
    const unsigned long live = stats_.live_allocations;
    stats_ = Stats();
    stats_.live_allocations = live;
}

void addSubmission(double wall_ms, double device_ms)
{
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.submit_wall_ms += wall_ms;
    stats_.device_ms += device_ms;
    ++stats_.submissions;
}

void addAllocation()
{
    std::lock_guard<std::mutex> lock(stats_mutex_);
    ++stats_.allocations;
    ++stats_.live_allocations;
}

void removeAllocation()
{
    std::lock_guard<std::mutex> lock(stats_mutex_);
    if (stats_.live_allocations) {
        --stats_.live_allocations;
    }
}

#ifdef ART_USE_VULKAN

std::vector<DeviceInfo> enumerateDevices() { return Context::enumerate(); }

bool init(const Glib::ustring &user_settings_dir,
          const Glib::ustring &device_preference, bool allow_software)
{
    parseDisabledOps();
    Context::configure(user_settings_dir, device_preference, allow_software);

    // recorded so a later failure reports as degraded, not "off by choice"
    const char *art_gpu = std::getenv("ART_GPU");
    const std::string pref = art_gpu && *art_gpu
                                 ? std::string(art_gpu)
                                 : std::string(device_preference.raw());
    wanted_ = !(pref.empty() || pref == "off" || pref == "0");
    return wanted_;
}

bool available()
{
    Context *c = Context::get();
    if (!c) {
        if (wanted_ && !degraded_) {
            degraded_ = true;
            status_ = contextError().empty()
                          ? std::string("no usable Vulkan device")
                          : contextError();
            std::cerr << "GPU: unavailable (" << status_
                      << "); using the CPU pipeline" << std::endl;
        }
        return false;
    }
    if (c->deviceLost()) {
        return false;
    }
    return true;
}

std::string description()
{
    Context *c = Context::get();
    if (!c) {
        return status_.empty() ? std::string("not in use") : ("unavailable: " + status_);
    }
    return c->description();
}

void cleanup()
{
    if (Context *c = Context::get()) { // pipelines reference the device, so they go first
        clearPipelineCache(*c);
        c->flushPipelineCacheToDisk();
    }
    Context::destroy();
    unloadVulkanLibrary();
}

#else // !ART_USE_VULKAN

std::vector<DeviceInfo> enumerateDevices() { return std::vector<DeviceInfo>(); }

bool init(const Glib::ustring &, const Glib::ustring &, bool) { return false; }

bool available() { return false; }

std::string description() { return "not compiled in"; }

void cleanup() {}

#endif // ART_USE_VULKAN


namespace ops {

bool decline(Imagefloat *img)
{
    if (img) {
        img->syncCpuForWrite();
    }
    return false;
}

} // namespace ops

} // namespace gpu
} // namespace rtengine
