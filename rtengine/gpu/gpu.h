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
 * Public facade of the Vulkan compute backend; see doc/gpu_pipeline.md.
 * C++11 and Vulkan-free so ordinary rtengine TUs can include it.
 *
 * Nothing here ever throws or aborts: a false/null return means "run the
 * CPU path instead."
 */
#ifndef ART_GPU_GPU_H
#define ART_GPU_GPU_H

#include "../noncopyable.h"

#include <glibmm/ustring.h>
#include <string>
#include <vector>

namespace rtengine {

class Imagefloat;

namespace gpu {

struct DeviceInfo {
    enum Kind { DISCRETE, INTEGRATED, VIRTUAL, SOFTWARE, OTHER };

    DeviceInfo():
        index(-1), kind(OTHER), device_local_bytes(0), api_version(0),
        host_visible_device_local(false), unified_memory(false)
    {
    }

    int index;
    std::string name;
    std::string driver;
    Kind kind;
    unsigned long long device_local_bytes;
    unsigned int api_version;
    bool host_visible_device_local;
    bool unified_memory;
};

/* Safe to call before init(). */
std::vector<DeviceInfo> enumerateDevices();

/* Called once from rtengine::init(); opening the device itself is lazy.
 * device_preference: "" or "off" disables; "auto" picks the best
 * non-software device; a decimal integer selects by index; anything else
 * matches a device-name substring case-insensitively.
 *
 * ART_GPU/ART_VULKAN_DEVICE/ART_VULKAN_ALLOW_SOFTWARE/ART_VULKAN_LIBRARY/
 * ART_GPU_DISABLE_OPS environment variables override the arguments, so ART
 * and ART-cli behave identically. Never fatal. */
bool init(const Glib::ustring &user_settings_dir,
          const Glib::ustring &device_preference, bool allow_software);

/* Must run before the thread pool is gone and before FFTW teardown. */
void cleanup();

bool available();

/* One line for --version / the About dialog / a bug report. */
std::string description();

bool opEnabled(const char *op);

/* True when `op` (an ImProcFunctions::apply stage-tool name) has a full GPU
 * implementation usable right now; the apply() funnel uses this to decide
 * whether to force a CPU sync before running the tool. Conservative by
 * construction: unknown/partially-ported names return false.
 *
 * No tool currently qualifies -- the GPU work that survives is all *inside*
 * a tool, which uploads at entry and syncs back before it returns. This is
 * the extension point for a tool that one day runs end to end on the device;
 * see doc/gpu_pipeline.md §3. */
bool hasFullGPUStage(const char *op);

/* Cumulative time inside Pass::submitAndWait(), across every Pass on every
 * thread since the last resetStats(). Milliseconds; 0 if unused.
 * Process-wide, not per-caller -- fine for a single active run, which is
 * the only case this is used for. */
double GPUTimeMs();

/* Counters for one pipeline run, reset together by resetStats().
 *
 * submit_wall_ms and device_ms are deliberately separate.  The first is what
 * the CPU thread spent blocked in submitAndWait(); the second is what the
 * device actually spent executing, from a timestamp bracketing the whole
 * command buffer.  On a unified-memory device the two are close, which is why
 * the older single number could be described as "GPU time".  On a discrete
 * GPU they are not: the difference is submit latency, fence-signal latency and
 * any PCIe staging copy recorded inside the Pass, none of which is compute.
 * Reporting only the first attributes all of that to the kernels.
 *
 * allocations counts vkAllocateMemory calls and live_allocations the ones not
 * yet freed.  Both matter because BufferPool exists precisely to keep the
 * first near zero in steady state; a count that grows with the number of
 * transfers means a pool is not being recycled. */
struct Stats {
    Stats():
        submit_wall_ms(0.0), device_ms(0.0), submissions(0), allocations(0),
        live_allocations(0)
    {
    }

    double submit_wall_ms;
    double device_ms;
    unsigned long submissions;
    unsigned long allocations;
    unsigned long live_allocations;
};

Stats stats();

void resetStats();

/* Called by Pass::submitAndWait(); not meant for ordinary rtengine code.
 * device_ms is 0 when the device cannot do timestamps. */
void addSubmission(double wall_ms, double device_ms);

/* Called by Context::allocateBuffer()/Buffer::reset(); likewise internal. */
void addAllocation();
void removeAllocation();

/* Emit a message once per distinct text, at settings->verbose level. */
void logOnce(const std::string &msg);


class Context;
class BufferPool;

namespace ops {

// Hand the image back to the CPU and return false; this is how an operator declines.
bool decline(Imagefloat *img);

} // namespace ops

} // namespace gpu
} // namespace rtengine

#endif // ART_GPU_GPU_H
