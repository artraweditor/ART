/* -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright (c) 2026 Alberto Griggio <alberto.griggio@gmail.com>
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
 * Optional fast path for converting float RGB through an input ICC profile
 * to a linear matrix/shaper output profile.
 *
 * Evaluates supported curve, float CLUT and matrix stages in blocks, with
 * float results between stages. Powers other than 2 and 3 use powf() (pow()
 * for an input that is not a float) and CLUT interpolation uses LUT3D, so
 * results are not bit-identical to lcms2.
 * Curves given as tables are evaluated by cmsEvalToneCurveFloat(), which
 * rounds to 16 bits like the reference (from lcms2 2.16; older versions do
 * not use them).
 *
 * Only the relative colorimetric intent is handled.
 *
 * create() validates the individual supported stages and the complete
 * transform against lcms2 on probe inputs. This is a compatibility check, not
 * a proven error bound.
 */
#pragma once

#include <memory>

#include <lcms2.h>

namespace art { namespace engine {

class ICCFloatPipeline {
public:
    /*
     * Returns nullptr if the profile combination or the intent (only
     * INTENT_RELATIVE_COLORIMETRIC) is unsupported or validation against
     * reference fails. The caller must then use reference instead.
     *
     * Validation requires, for each channel,
     *   abs(candidate - reference) <= 2e-6 * max(1, abs(reference))
     * and, to keep dark pixels accurate in proportion to their size,
     *   abs(candidate - reference) <= 1e-5 * (largest abs(reference) of the
     *                                         pixel) + 1e-12
     * Any non-finite value is a failure.
     *
     * reference must be the TYPE_RGB_FLT transform from in to out for this
     * intent, created with cmsFLAGS_NOOPTIMIZE.
     *
     * The caller must hold the profile lock during create(). The returned
     * object may borrow curve data from in and must be destroyed before in.
     *
     * ART_ICC_FAST=0 disables this path.
     */
    static std::unique_ptr<ICCFloatPipeline> create(cmsHPROFILE in,
                                                    cmsHPROFILE out,
                                                    cmsUInt32Number intent,
                                                    cmsHTRANSFORM reference);

    /* Converts W (>= 0) RGB triplets in place, like
     * cmsDoTransform(reference, ...). */
    void apply(float *rgb, int W) const;

    ~ICCFloatPipeline();

private:
    struct Impl;

    ICCFloatPipeline();

    std::unique_ptr<Impl> impl_;
};

}} // namespace art::engine
