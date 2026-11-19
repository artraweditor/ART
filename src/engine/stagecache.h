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
#pragma once

#include "improcfun.h"
#include "procparams.h"
#include <vector>

namespace art { namespace engine {

/**
 * Keeps the intermediate images of the STAGE_1..STAGE_3 pipeline of the
 * preview (and of the detail crop), so that an update only has to redo the
 * part of the pipeline that follows the first change.
 *
 * The pipeline is the list of steps of ImProcFunctions (one per tool).  The
 * cache holds one image per stage (a "cut"): after STAGE_1, after STAGE_2
 * and at the end of the pipeline, so an update resumes from the start of the
 * stage that contains the first step that has to be redone.
 *
 * Which cut an update resumes from is decided by the refresh bits ("todo",
 * which say from which stage on the pipeline is dirty) refined by comparing
 * the ProcParams of the previous update with the current ones, tool by tool.
 */
class StageCache {
public:
    StageCache();
    ~StageCache();
    StageCache(const StageCache &) = delete;
    StageCache &operator=(const StageCache &) = delete;

    /** The positions (number of steps already run) of the cut points. */
    static std::vector<int> stageCuts();

    int size() const { return bufs_.size(); }
    /** (Re)allocate all the images and forget their content. */
    void allocate(int w, int h);
    void free();
    /** Forget the content of all images (without freeing them). */
    void invalidate();
    void assignColorSpace(const Glib::ustring &profile);

    /** The image at the end of the pipeline. */
    Imagefloat *final() const { return bufs_.back(); }
    /** True if the last run asked the pipeline to stop. */
    bool stopped() const { return states_.back().any(); }

    /**
     * Bring final() up to date.
     *
     * @param todo the refresh bits of the update
     * @param src the input of the pipeline
     * @param prev_stop true if a step before the pipeline asked to stop
     * @param refine false to resume only at stage boundaries, as decided by
     *        `todo`, ignoring the params (for when a tool has side outputs
     *        that must not be skipped, e.g. an active pipette buffer)
     * @return true if the pipeline must stop
     */
    bool run(ImProcFunctions &ipf, const procparams::ProcParams &params,
             int todo, const Imagefloat *src, bool prev_stop,
             bool refine = true);

    /** The first step redone by the last run() (numSteps() if none). */
    int firstStep() const { return first_step_; }

private:
    int firstDirtyStep(int todo) const;
    int findStart(const procparams::ProcParams &params, int s0, bool refine,
                  int if_unchanged) const;
    void verify(ImProcFunctions &ipf, const procparams::ProcParams &params,
                const Imagefloat *src, bool prev_stop);

    std::vector<int> cuts_;
    std::vector<Imagefloat *> bufs_;
    std::vector<ImProcFunctions::StopState> states_;
    std::vector<bool> valid_;
    procparams::ProcParams last_;
    bool have_last_;
    int first_step_;
    int verify_;
};

/**
 * Narrow the refresh bits of an update of the detail crop that precede the
 * stages (M_INIT, M_LINDENOISE, M_SPOT, M_HDR, M_TRANSFORM, M_BLURMAP) by
 * comparing the params of its last update with the current ones: if only
 * tools of the stages, tools of these phases or sections that nothing reads
 * changed, only the phases that depend on the latter are kept.  Anything that
 * is not understood (including the bits that say that the image source
 * changed) returns `todo` unchanged.  The other bits are left alone: they are
 * for StageCache::run().
 *
 * Can only be used if nothing but the params can have changed what the early
 * phases read.
 */
int refineEarlyTodo(const procparams::ProcParams &last,
                    const procparams::ProcParams &cur, int todo);

}} // namespace art::engine
