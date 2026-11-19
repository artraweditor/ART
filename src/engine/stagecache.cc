/* -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright (c) 2017-2026 Alberto Griggio <alberto.griggio@gmail.com>
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
#include "stagecache.h"
#include "refreshmap.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

namespace art { namespace engine {

namespace {

using IPF = ImProcFunctions;
using Stage = IPF::Stage;

constexpr int PIPELINE_BITS = M_RGBCURVE | M_LUMACURVE | M_LUMINANCE | M_COLOR;
// the bits of an update that do not change the input of the pipeline.
// M_AUTOEXP only recomputes (and stores in the params, which are compared) the
// values of the "auto" settings, and the preview sets it on every update when
// it always renders at high quality
constexpr int EARLY_BITS =
    M_INIT | M_LINDENOISE | M_SPOT | M_HDR | M_TRANSFORM | M_BLURMAP;
constexpr int NOT_INPUT_BITS =
    PIPELINE_BITS | M_MONITOR | M_CROP | M_VOID | M_MINUPDATE | M_AUTOEXP;

// the sections of the params that no tool reads
void copyNeutral(const procparams::ProcParams &src, procparams::ProcParams &dst)
{
    dst.rank = src.rank;
    dst.colorlabel = src.colorlabel;
    dst.inTrash = src.inTrash;
    dst.appVersion = src.appVersion;
    dst.ppVersion = src.ppVersion;
    dst.metadata = src.metadata;
}


bool envFlag(const char *name)
{
    const char *v = getenv(name);
    return v && *v && strcmp(v, "0");
}


// a linked mask can use the output of any tool before the one that links it
bool linkedMasksUsed(const procparams::ProcParams &params)
{
    for (auto p : params.get_maskable()) {
        for (auto &m : p->get_masks()) {
            if (m.enabled && m.linkedMask.enabled) {
                return true;
            }
        }
    }
    return false;
}

} // namespace


// one image per stage: after STAGE_1, after STAGE_2 and at the end
std::vector<int> StageCache::stageCuts()
{
    return {IPF::stageBegin(Stage::STAGE_2), IPF::stageBegin(Stage::STAGE_3),
            IPF::numSteps()};
}


StageCache::StageCache():
    cuts_(stageCuts()),
    bufs_(cuts_.size(), nullptr),
    states_(cuts_.size()),
    valid_(cuts_.size(), false),
    have_last_(false),
    first_step_(IPF::numSteps()),
    verify_(envFlag("ART_STAGE_CACHE_VERIFY") ? 1 : 0)
{
}


StageCache::~StageCache()
{
    free();
}


void StageCache::allocate(int w, int h)
{
    for (auto &b : bufs_) {
        if (!b) {
            b = new Imagefloat();
        }
        b->allocate(w, h);
    }
    invalidate();
}


void StageCache::free()
{
    for (auto &b : bufs_) {
        delete b;
        b = nullptr;
    }
    invalidate();
}


void StageCache::invalidate()
{
    std::fill(valid_.begin(), valid_.end(), false);
    have_last_ = false;
}


void StageCache::assignColorSpace(const Glib::ustring &profile)
{
    for (auto b : bufs_) {
        if (b) {
            b->assignColorSpace(profile);
        }
    }
}


int StageCache::firstDirtyStep(int todo) const
{
    if (todo & M_RGBCURVE) {
        return 0;
    } else if (todo & M_LUMACURVE) {
        return IPF::stageBegin(Stage::STAGE_2);
    } else if (todo & (M_LUMINANCE | M_COLOR)) {
        return IPF::stageBegin(Stage::STAGE_3);
    }
    return IPF::numSteps();
}


/* The first step that has to be redone, given that nothing before `s0` has
 * changed.  Anything that is not understood is an answer of s0, except that
 * if the params are the same as the last time `if_unchanged` is returned. */
int StageCache::findStart(const procparams::ProcParams &params, int s0,
                          bool refine, int if_unchanged) const
{
    if (!refine || !have_last_) {
        return s0;
    }
    const int N = IPF::numSteps();

    // a change in a section that no step reads can affect any of them
    procparams::ProcParams tmp(params);
    for (int i = 0; i < N; ++i) {
        IPF::copyStepParams(i, last_, tmp);
    }
    copyNeutral(last_, tmp);
    if (tmp != last_) {
        return s0;
    }

    tmp = last_;
    int f = N;
    for (int i = s0; i < N; ++i) {
        IPF::copyStepParams(i, params, tmp);
        if (tmp != last_) {
            f = i;
            break;
        }
    }
    if (f >= N) {
        // nothing changed that we can see
        return if_unchanged;
    }
    // masks can be linked to the output of earlier tools of the same stage
    // (the manager is reset where a stage begins)
    if (linkedMasksUsed(params)) {
        f = std::max(s0, IPF::stageBegin(IPF::stepStage(f)));
    }
    return f;
}


bool StageCache::run(ImProcFunctions &ipf, const procparams::ProcParams &params,
                     int todo, const Imagefloat *src, bool prev_stop,
                     bool refine)
{
    const int N = IPF::numSteps();
    const int s0 = firstDirtyStep(todo);
    first_step_ = N;
    // an update that was abandoned halfway leaves its images unfinished
    const bool unfinished =
        std::find(valid_.begin(), valid_.end(), false) != valid_.end();
    if (s0 >= N && !unfinished) {
        return stopped();
    }

    int f = s0;
    if (s0 >= N) {
        // only finish what is left, unless the params say that more changed
        f = findStart(params, 0, refine, N);
    } else if (todo & ~NOT_INPUT_BITS) {
        // the input changed: nothing can be reused
        invalidate();
        f = 0;
    } else {
        f = findStart(params, s0, refine, s0);
    }
    last_ = params;
    have_last_ = true;

    int base = -1; // the cut to resume from; -1 for the input
    for (size_t j = 0; j < cuts_.size() && cuts_[j] <= f && valid_[j]; ++j) {
        base = j;
    }
    for (size_t j = base + 1; j < cuts_.size(); ++j) {
        valid_[j] = false; // being recomputed
    }

    ImProcFunctions::StopState st;
    const Imagefloat *cur = src;
    int first = 0;
    if (base < 0) {
        st.prev = prev_stop;
    } else {
        st = states_[base];
        cur = bufs_[base];
        first = cuts_[base];
    }
    first_step_ = first;

    if (envFlag("ART_STAGE_CACHE_TRACE")) {
        fprintf(stderr,
                "StageCache(%d): todo=0x%x, dirty from step %d, redo from "
                "step %d (%s), resume from %s\n",
                int(cuts_.size()), todo, s0, first,
                first < N ? IPF::stepName(first) : "-",
                base < 0 ? "the input"
                         : (std::string("the cut at ") +
                            std::to_string(cuts_[base])).c_str());
    }

    for (size_t j = base + 1; j < cuts_.size(); ++j) {
        cur->copyTo(bufs_[j]);
        ipf.processSteps(first, cuts_[j], bufs_[j], st);
        states_[j] = st;
        valid_[j] = true;
        cur = bufs_[j];
        first = cuts_[j];
    }

    if (verify_) {
        verify(ipf, params, src, prev_stop);
    }
    return stopped();
}


/* Debug aid (ART_STAGE_CACHE_VERIFY=1): redo the whole pipeline from the
 * input and compare it with what the cache produced. */
void StageCache::verify(ImProcFunctions &ipf,
                        const procparams::ProcParams &params,
                        const Imagefloat *src, bool prev_stop)
{
    const Imagefloat *out = final();
    std::unique_ptr<Imagefloat> ref(
        new Imagefloat(out->getWidth(), out->getHeight()));
    src->copyTo(ref.get());
    ImProcFunctions::StopState st;
    st.prev = prev_stop;
    ipf.processSteps(0, IPF::numSteps(), ref.get(), st);

    ref->syncCpu();
    out->syncCpu();
    if (ref->mode() != out->mode()) {
        fprintf(stderr, "StageCache VERIFY: image modes differ\n");
        return;
    }
    const int w = out->getWidth(), h = out->getHeight();
    double maxdiff = 0, sum = 0;
    size_t bad = 0;
    int bx = 0, by = 0;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float d = std::max({std::abs(ref->r(y, x) - out->r(y, x)),
                                std::abs(ref->g(y, x) - out->g(y, x)),
                                std::abs(ref->b(y, x) - out->b(y, x))});
            sum += d;
            if (!(d <= 4.f)) { // also catches NaNs
                ++bad;
            }
            if (!(d <= maxdiff)) {
                maxdiff = d;
                bx = x;
                by = y;
            }
        }
    }
    fprintf(stderr,
            "StageCache VERIFY (n=%d, redo from step %d): %s: max diff %g at "
            "(%d, %d), mean %g, %zu samples > 4\n",
            int(cuts_.size()), first_step_, bad ? "MISMATCH" : "ok", maxdiff,
            bx, by, sum / (double(w) * h), bad);
}


/* The phases before the stages of the detail crop (see Crop::updateImpl) form
 * a chain: each one works on the output of the previous one, and some of them
 * in place:
 *
 *   M_INIT       load the crop from the image source (reads raw, wb, coarse,
 *                icm, filmNegative, the highlight reconstruction of exposure)
 *   M_LINDENOISE copy of the above, denoised (denoise, expcomp)
 *   M_SPOT       copy of the above with the spots removed (spot; the
 *                denoiseInfoStore of the denoise, if enabled)
 *   M_HDR        dehaze/drcompression, in place on the output of the last of
 *                the previous two that made a copy
 *   M_TRANSFORM  geometric transform of the above (lensProf, perspective,
 *                rotate, distortion, commonTrans, coarse, crop)
 *
 * An in-place phase cannot be redone without redoing the one that produced
 * its input, hence the closures below.  The sections of the params that no
 * phase and no step reads are "neutral".  If any other section changed (so
 * that we do not know which phases are affected) the refresh bits are left
 * alone. */
int refineEarlyTodo(const procparams::ProcParams &last,
                    const procparams::ProcParams &cur, int todo)
{
    if (!(todo & EARLY_BITS) ||
        (todo & (M_RAW | M_PREPROC | M_WHITEBALANCE | M_HIGHQUAL))) {
        return todo;
    }

    procparams::ProcParams tmp(cur);
    for (int i = 0, n = IPF::numSteps(); i < n; ++i) {
        IPF::copyStepParams(i, last, tmp);
    }
    copyNeutral(last, tmp);
    tmp.denoise = last.denoise;
    tmp.spot = last.spot;
    tmp.dehaze = last.dehaze;
    tmp.drcompression = last.drcompression;
    // (exposure was copied with the steps: its early part is checked below)
    if (tmp != last) {
        return todo;
    }

    const bool exposure_early =
        cur.exposure.enabled != last.exposure.enabled ||
        cur.exposure.expcomp != last.exposure.expcomp ||
        cur.exposure.hrmode != last.exposure.hrmode ||
        cur.exposure.hrblur != last.exposure.hrblur;
    constexpr int DENOISE = M_LINDENOISE | M_SPOT | M_HDR;
    int need = 0;
    if (exposure_early || cur.denoise != last.denoise) {
        need |= DENOISE;
    }
    // (spot removal copies the denoised crop, and uses what the denoise
    // computed if the latter is enabled)
    // (the preview of the processor only does it if there are spots)
    const bool spot_copy = cur.spot.enabled && last.spot.enabled &&
                           !cur.spot.entries.empty() &&
                           !last.spot.entries.empty() &&
                           !cur.denoise.enabled;
    if (cur.spot != last.spot) {
        need |= spot_copy ? (M_SPOT | M_HDR) : DENOISE;
    }
    if (cur.dehaze != last.dehaze || cur.drcompression != last.drcompression) {
        need |= spot_copy ? (M_SPOT | M_HDR) : DENOISE;
    }
    if (need) {
        need |= M_TRANSFORM;
    }
    return (todo & ~EARLY_BITS) | need;
}

}} // namespace art::engine
