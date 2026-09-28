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

// Pad a 3x3 matrix into a 3x4 row-major array matching std430's mat3-as-three-vec4s layout.
#pragma once

namespace rtengine {
namespace gpu {

inline void fillRows(float dst[3][4], const float src[3][3])
{
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            dst[i][j] = src[i][j];
        }
        dst[i][3] = 0.f;
    }
}


inline void fillMat3(float m[3][4], const float *rows)
{
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            m[r][c] = rows[r * 3 + c];
        }
        m[r][3] = 0.f;
    }
}



} // namespace gpu
} // namespace rtengine
