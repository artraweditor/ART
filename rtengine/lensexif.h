/* -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright (c) 2020 Alberto Griggio <alberto.griggio@gmail.com>
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

#include "lcp.h"
#include "procparams.h"
#include "rtengine.h"
#include <memory>

namespace rtengine {

class ExifLensCorrection: public LensCorrection {
public:
    static bool ok(const FramesMetaData *meta);
    ExifLensCorrection(const FramesMetaData *meta, int width, int height,
                       const CoarseTransformParams &coarse, int rawRotationDeg);
    bool ok() const;

    void correctDistortion(double &x, double &y, int cx, int cy,
                           double scale) const override;
    bool isCACorrectionAvailable() const override;
    void correctCA(double &x, double &y, int cx, int cy,
                   int channel) const override;
    void processVignette(int width, int height, float **rawData) const override;
    void processVignette3Channels(int width, int height,
                                  float **rawData) const override
    {
    }

    class CorrectionData {
    public:
        virtual ~CorrectionData() = default;
        virtual void get_coeffs(std::vector<float> &knots,
                                std::vector<float> &dist,
                                std::vector<float> &vig,
                                std::array<std::vector<float>, 3> &ca,
                                bool &is_dng) const = 0;
        virtual bool has_dist() const = 0;
        virtual bool has_ca() const = 0;
        virtual bool has_vign() const = 0;
    };

private:
    std::unique_ptr<CorrectionData> data_;
    std::vector<float> knots_;
    std::vector<float> dist_;
    std::vector<float> vig_;
    std::array<std::vector<float>, 3> ca_;
    bool is_dng_;
    bool swap_xy_;
    float w2_;
    float h2_;
    float r_;
};

} // namespace rtengine
