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
 * Per-operator wall-clock profile of the processing pipeline.
 *
 * The point is to answer "which operators actually cost time", because that --
 * not stage order, and not how easy an operator is to port -- is what decides
 * where GPU work is worth doing.  A GPU port that speeds up operators
 * accounting for 5 % of the time is not worth its maintenance cost.
 *
 * Off unless ART_PROFILE is set, and when off a Timer is a load, a branch and
 * nothing else.  Enabling it costs two clock reads per operator, which is
 * noise against operators measured in milliseconds.
 */
#ifndef ART_PIPELINEPROFILE_H
#define ART_PIPELINEPROFILE_H

#include "mytime.h"
#include "gpu/gpu.h"

namespace rtengine {

class PipelineProfile {
public:
    /* True when ART_PROFILE is set to anything other than 0. */
    static bool enabled();

    static void add(const char *name, double ms);

    /* Print the accumulated table, sorted by total time, and reset.  Called
     * once per processed image. */
    static void report(const char *title);

    class Timer {
    public:
        explicit Timer(const char *name):
            name_(name), active_(PipelineProfile::enabled())
        {
            if (active_) {
                t0_.set();
            }
        }

        ~Timer() { stop(); }

        /* Record now instead of at scope exit, for a phase whose extent does
         * not match a C++ block -- the same escape hatch StopWatch::stop()
         * provides.  Idempotent: the destructor will not record a second
         * time. */
        void stop()
        {
            if (active_) {
                active_ = false;
                MyTime t1;
                t1.set();
                PipelineProfile::add(name_, t1.etime(t0_) / 1000.0);
            }
        }

    private:
        Timer(const Timer &);
        Timer &operator=(const Timer &);

        const char *name_;
        MyTime t0_;
        bool active_;
    };
};

/*
 * Coarse CPU/GPU wall-clock split for one pipeline run, printed to stdout
 * when settings->verbose > 0.  Independent of ART_PROFILE above: always on
 * (no env var), never the per-operator table.
 *
 * "GPU time" is the time the CPU thread spent blocked inside
 * Pass::submitAndWait().  That is *not* the same as device compute, and the
 * difference is the point of the breakdown that follows it: a submission's
 * wall also contains submit latency, fence-signal latency, and any staging
 * copy recorded inside the Pass -- which on a discrete GPU is a PCIe
 * transfer.  The "device" figure comes from a GPU timestamp bracketing each
 * command buffer, so "overhead" is what the pipeline pays per submission for
 * reasons other than computing anything.  On a unified-memory device the two
 * are close; on a discrete one they are not, and reporting only the first
 * would attribute all the transfer and round-trip cost to the kernels.
 *
 * "CPU time" is whatever is left of the wall clock -- decoding, demosaicing,
 * tone curves that declined, and (where a device buffer is host-visible) the
 * upload/download memcpys, which are not submissions at all.
 *
 * The allocation counters are here because BufferPool exists to keep
 * vkAllocateMemory off the hot path; a count that scales with the number of
 * transfers means some pool is not being recycled.
 */
class PipelineTimeReport {
public:
    /* title identifies the run in the printed line, e.g. "export", "preview". */
    explicit PipelineTimeReport(const char *title): title_(title)
    {
        gpu::resetStats();
        t0_.set();
    }

    ~PipelineTimeReport();

private:
    PipelineTimeReport(const PipelineTimeReport &);
    PipelineTimeReport &operator=(const PipelineTimeReport &);

    const char *title_;
    MyTime t0_;
};

} // namespace rtengine

#define ART_PROFILE_SCOPE(name)                                                \
    rtengine::PipelineProfile::Timer _art_prof_scope_(name)

/* For a phase that does not coincide with a C++ block: declare it, then call
 * var.stop() where the phase actually ends.  Used by ipdenoise.cc's
 * denoise::RGB_denoise, whose phases are interleaved inside one long
 * function. */
#define ART_PROFILE_SCOPE_NAMED(var, name)                                     \
    rtengine::PipelineProfile::Timer var(name)

#define ART_PIPELINE_TIME_REPORT(title)                                        \
    rtengine::PipelineTimeReport _art_pipeline_time_report_(title)

#endif // ART_PIPELINEPROFILE_H
