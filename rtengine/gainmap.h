/* -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright 2020 Alberto Griggio <alberto.griggio@gmail.com>
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

#include <stdint.h>
#include <vector>

namespace rtengine {

struct GainMap {
    uint32_t top;
    uint32_t left;
    uint32_t bottom;
    uint32_t right;
    uint32_t plane;
    uint32_t planes;
    uint32_t row_pitch;
    uint32_t col_pitch;
    uint32_t map_points_v;
    uint32_t map_points_h;
    double map_spacing_v;
    double map_spacing_h;
    double map_origin_v;
    double map_origin_h;
    uint32_t map_planes;
    std::vector<float> map_gain;

    GainMap() = default;

    std::string to_str() const;
    static std::vector<GainMap> read(const std::vector<uint8_t> &buf);
};

} // namespace rtengine
