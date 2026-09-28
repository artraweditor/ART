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

#include "pipelineprofile.h"

#include "settings.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace rtengine {

extern const Settings *settings;

namespace {

struct Entry {
    Entry(): ms(0.0), calls(0) {}
    double ms;
    long calls;
};

std::map<std::string, Entry> entries_;
std::mutex mutex_;

} // namespace

bool PipelineProfile::enabled()
{
    static const bool v = []() {
        const char *e = std::getenv("ART_PROFILE");
        return e && *e && std::strcmp(e, "0") != 0;
    }();
    return v;
}

void PipelineProfile::add(const char *name, double ms)
{
    if (!name) {
        return;
    }
    /* Several threads can be inside the pipeline at once (crop updaters, the
     * batch queue, thumbnailers), so the accumulation is locked.  Contention is
     * irrelevant: one lock per operator, not per pixel. */
    std::lock_guard<std::mutex> lock(mutex_);
    Entry &e = entries_[name];
    e.ms += ms;
    ++e.calls;
}

void PipelineProfile::report(const char *title)
{
    if (!enabled()) {
        return;
    }

    std::vector<std::pair<std::string, Entry> > v;
    double total = 0.0;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (std::map<std::string, Entry>::const_iterator it = entries_.begin();
             it != entries_.end(); ++it) {
            v.push_back(*it);
            total += it->second.ms;
        }
        entries_.clear();
    }
    if (v.empty()) {
        return;
    }

    std::sort(v.begin(), v.end(),
              [](const std::pair<std::string, Entry> &a,
                 const std::pair<std::string, Entry> &b) {
                  return a.second.ms > b.second.ms;
              });

    std::cerr << "\n=== pipeline profile: " << (title ? title : "") << " ===\n";
    std::cerr << std::left << std::setw(28) << "operator" << std::right
              << std::setw(11) << "ms" << std::setw(8) << "%"
              << std::setw(9) << "cum %" << std::setw(8) << "calls" << "\n";
    std::cerr << std::string(64, '-') << "\n";

    double cum = 0.0;
    for (size_t i = 0; i < v.size(); ++i) {
        const double pct = total > 0.0 ? v[i].second.ms / total * 100.0 : 0.0;
        cum += pct;
        std::cerr << std::left << std::setw(28) << v[i].first << std::right
                  << std::fixed << std::setprecision(2) << std::setw(11)
                  << v[i].second.ms << std::setprecision(1) << std::setw(8)
                  << pct << std::setw(9) << cum << std::setw(8)
                  << v[i].second.calls << "\n";
    }
    std::cerr << std::string(64, '-') << "\n";
    std::cerr << std::left << std::setw(28) << "TOTAL (sum of scopes)"
              << std::right << std::fixed << std::setprecision(2)
              << std::setw(11) << total << "\n\n";
}

PipelineTimeReport::~PipelineTimeReport()
{
    if (!settings || settings->verbose <= 0) {
        return;
    }
    MyTime t1;
    t1.set();
    const double wall = t1.etime(t0_) / 1000.0;
    const gpu::Stats st = gpu::stats();
    const double gpu_ms = st.submit_wall_ms;
    std::cout << "pipeline (" << (title_ ? title_ : "") << "): " << std::fixed
              << std::setprecision(2) << wall << " ms wall, " << gpu_ms
              << " ms GPU, " << (wall - gpu_ms) << " ms CPU" << std::endl;
    if (st.submissions) {
        std::cout << "  GPU: " << st.submissions << " submissions, "
                  << st.device_ms << " ms device, "
                  << (gpu_ms - st.device_ms) << " ms overhead" << std::endl;
    }
    if (st.allocations || st.live_allocations) {
        std::cout << "  GPU: " << st.allocations
                  << " allocations this run, " << st.live_allocations
                  << " live" << std::endl;
    }
}

} // namespace rtengine
