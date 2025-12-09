/* -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright 2023 Alberto Griggio <alberto.griggio@gmail.com>
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

#include "LUT3D.h"
#include "linalgebra.h"
#include "opthelper.h"

namespace rtengine {

LUT3D::initializer::~initializer() {}

LUT3D::LUT3D(): input_is_01_(true), dim_(0), dim_minus_one_(0) {}

LUT3D::~LUT3D() {}

void LUT3D::init(int dim, initializer &f, bool input_is_01)
{
    dim_ = dim;
    dim_minus_one_ = dim - 1;
    input_is_01_ = input_is_01;

    lut_.resize(SQR(dim_) * dim_ * 3);
    size_t index = 0;
    float r, g, b;
    for (int i = 0; i < dim_; ++i) {
        for (int j = 0; j < dim_; ++j) {
            for (int k = 0; k < dim_; ++k) {
                f(r, g, b);
                lut_.data[index] = r;
                ++index;
                lut_.data[index] = g;
                ++index;
                lut_.data[index] = b;
                ++index;
            }
        }
    }
}

bool LUT3D::operator()(float &r, float &g, float &b)
{
    if (!input_is_01_) {
        r /= 65535.f;
        g /= 65535.f;
        b /= 65535.f;
    }

    if (!lut_.isEmpty()) {
        apply_tetra(r, g, b);
        return true;
    }
    return false;
}

LUT3D::operator bool() const { return !lut_.isEmpty(); }

// Tetrahedral interpolation, adapted from OpenColorIO
//  https://github.com/AcademySoftwareFoundation/OpenColorIO
//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.
//
namespace {

inline int GetLut3DIndexBlueFast(int indexR, int indexG, int indexB, int dim,
                                 int components)
{
    return components * (indexB + dim * (indexG + dim * indexR));
}

} // namespace

inline void LUT3D::apply_tetra(float &r, float &g, float &b)
{
    const float dimMinusOne = dim_minus_one_;
    const float m_step = dimMinusOne;
    constexpr int m_components = 3;
    const int m_dim = dim_;
    const float *m_optLut = lut_.data;
    float out[3];

    float idx[3];
    idx[0] = r * m_step;
    idx[1] = g * m_step;
    idx[2] = b * m_step;

    // NaNs become 0.
    idx[0] = LIM(idx[0], 0.f, dimMinusOne);
    idx[1] = LIM(idx[1], 0.f, dimMinusOne);
    idx[2] = LIM(idx[2], 0.f, dimMinusOne);

    int indexLow[3];
    indexLow[0] = static_cast<int>(std::floor(idx[0]));
    indexLow[1] = static_cast<int>(std::floor(idx[1]));
    indexLow[2] = static_cast<int>(std::floor(idx[2]));

    int indexHigh[3];
    // When the idx is exactly equal to an index (e.g. 0,1,2...)
    // then the computation of highIdx is wrong. However,
    // the delta is then equal to zero (e.g. idx-lowIdx),
    // so the highIdx has no impact.
    indexHigh[0] = static_cast<int>(std::ceil(idx[0]));
    indexHigh[1] = static_cast<int>(std::ceil(idx[1]));
    indexHigh[2] = static_cast<int>(std::ceil(idx[2]));

    float fx = idx[0] - static_cast<float>(indexLow[0]);
    float fy = idx[1] - static_cast<float>(indexLow[1]);
    float fz = idx[2] - static_cast<float>(indexLow[2]);

    // Compute index into LUT for surrounding corners
    const int n000 = GetLut3DIndexBlueFast(indexLow[0], indexLow[1],
                                           indexLow[2], m_dim, m_components);
    const int n100 = GetLut3DIndexBlueFast(indexHigh[0], indexLow[1],
                                           indexLow[2], m_dim, m_components);
    const int n010 = GetLut3DIndexBlueFast(indexLow[0], indexHigh[1],
                                           indexLow[2], m_dim, m_components);
    const int n001 = GetLut3DIndexBlueFast(indexLow[0], indexLow[1],
                                           indexHigh[2], m_dim, m_components);
    const int n110 = GetLut3DIndexBlueFast(indexHigh[0], indexHigh[1],
                                           indexLow[2], m_dim, m_components);
    const int n101 = GetLut3DIndexBlueFast(indexHigh[0], indexLow[1],
                                           indexHigh[2], m_dim, m_components);
    const int n011 = GetLut3DIndexBlueFast(indexLow[0], indexHigh[1],
                                           indexHigh[2], m_dim, m_components);
    const int n111 = GetLut3DIndexBlueFast(indexHigh[0], indexHigh[1],
                                           indexHigh[2], m_dim, m_components);

    if (fx > fy) {
        if (fy > fz) {
            out[0] = (1 - fx) * m_optLut[n000] + (fx - fy) * m_optLut[n100] +
                     (fy - fz) * m_optLut[n110] + (fz)*m_optLut[n111];

            out[1] = (1 - fx) * m_optLut[n000 + 1] +
                     (fx - fy) * m_optLut[n100 + 1] +
                     (fy - fz) * m_optLut[n110 + 1] + (fz)*m_optLut[n111 + 1];

            out[2] = (1 - fx) * m_optLut[n000 + 2] +
                     (fx - fy) * m_optLut[n100 + 2] +
                     (fy - fz) * m_optLut[n110 + 2] + (fz)*m_optLut[n111 + 2];
        } else if (fx > fz) {
            out[0] = (1 - fx) * m_optLut[n000] + (fx - fz) * m_optLut[n100] +
                     (fz - fy) * m_optLut[n101] + (fy)*m_optLut[n111];

            out[1] = (1 - fx) * m_optLut[n000 + 1] +
                     (fx - fz) * m_optLut[n100 + 1] +
                     (fz - fy) * m_optLut[n101 + 1] + (fy)*m_optLut[n111 + 1];

            out[2] = (1 - fx) * m_optLut[n000 + 2] +
                     (fx - fz) * m_optLut[n100 + 2] +
                     (fz - fy) * m_optLut[n101 + 2] + (fy)*m_optLut[n111 + 2];
        } else {
            out[0] = (1 - fz) * m_optLut[n000] + (fz - fx) * m_optLut[n001] +
                     (fx - fy) * m_optLut[n101] + (fy)*m_optLut[n111];

            out[1] = (1 - fz) * m_optLut[n000 + 1] +
                     (fz - fx) * m_optLut[n001 + 1] +
                     (fx - fy) * m_optLut[n101 + 1] + (fy)*m_optLut[n111 + 1];

            out[2] = (1 - fz) * m_optLut[n000 + 2] +
                     (fz - fx) * m_optLut[n001 + 2] +
                     (fx - fy) * m_optLut[n101 + 2] + (fy)*m_optLut[n111 + 2];
        }
    } else {
        if (fz > fy) {
            out[0] = (1 - fz) * m_optLut[n000] + (fz - fy) * m_optLut[n001] +
                     (fy - fx) * m_optLut[n011] + (fx)*m_optLut[n111];

            out[1] = (1 - fz) * m_optLut[n000 + 1] +
                     (fz - fy) * m_optLut[n001 + 1] +
                     (fy - fx) * m_optLut[n011 + 1] + (fx)*m_optLut[n111 + 1];

            out[2] = (1 - fz) * m_optLut[n000 + 2] +
                     (fz - fy) * m_optLut[n001 + 2] +
                     (fy - fx) * m_optLut[n011 + 2] + (fx)*m_optLut[n111 + 2];
        } else if (fz > fx) {
            out[0] = (1 - fy) * m_optLut[n000] + (fy - fz) * m_optLut[n010] +
                     (fz - fx) * m_optLut[n011] + (fx)*m_optLut[n111];

            out[1] = (1 - fy) * m_optLut[n000 + 1] +
                     (fy - fz) * m_optLut[n010 + 1] +
                     (fz - fx) * m_optLut[n011 + 1] + (fx)*m_optLut[n111 + 1];

            out[2] = (1 - fy) * m_optLut[n000 + 2] +
                     (fy - fz) * m_optLut[n010 + 2] +
                     (fz - fx) * m_optLut[n011 + 2] + (fx)*m_optLut[n111 + 2];
        } else {
            out[0] = (1 - fy) * m_optLut[n000] + (fy - fx) * m_optLut[n010] +
                     (fx - fz) * m_optLut[n110] + (fz)*m_optLut[n111];

            out[1] = (1 - fy) * m_optLut[n000 + 1] +
                     (fy - fx) * m_optLut[n010 + 1] +
                     (fx - fz) * m_optLut[n110 + 1] + (fz)*m_optLut[n111 + 1];

            out[2] = (1 - fy) * m_optLut[n000 + 2] +
                     (fy - fx) * m_optLut[n010 + 2] +
                     (fx - fz) * m_optLut[n110 + 2] + (fz)*m_optLut[n111 + 2];
        }
    }

    r = out[0];
    g = out[1];
    b = out[2];
}

} // namespace rtengine
