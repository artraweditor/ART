/* -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright (c) 2023-2026 Alberto Griggio <alberto.griggio@gmail.com>
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

namespace art { namespace engine {

LUT3D::initializer::~initializer() {}

LUT3D::LUT3D(): input_is_01_(true), dim_(0), dim_minus_one_(0) {}

LUT3D::~LUT3D() {}

void LUT3D::init(int dim, initializer &f, bool input_is_01)
{
    dim_ = dim;
    dim_minus_one_ = dim - 1;
    input_is_01_ = input_is_01;

#ifdef ART_SIMD
    // one float of padding so the vectorized tetrahedral interpolation can
    // always do a 4-wide unaligned load starting at the last LUT entry's
    // red component (the loaded 4th lane, belonging to the next entry, is
    // never used)
    lut_.resize(SQR(dim_) * dim_ * 3 + 1);
#else
    lut_.resize(SQR(dim_) * dim_ * 3);
#endif
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

bool LUT3D::operator()(float &r, float &g, float &b) const
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

#ifdef ART_SIMD

namespace {

// Accumulates w0*lut[c0] + w1*lut[c1] + w2*lut[c2] + w3*lut[c3] for the
// r/g/b triple stored contiguously at each corner, in a single 4-wide
// (r, g, b, unused) vector instead of one scalar FMA chain per channel.
inline vfloat interp_corners(const float *lut, float w0, int c0, float w1,
                             int c1, float w2, int c2, float w3, int c3)
{
    vfloat out = vmulf(F2V(w0), LVFU(lut[c0]));
    out = vmlaf(F2V(w1), LVFU(lut[c1]), out);
    out = vmlaf(F2V(w2), LVFU(lut[c2]), out);
    out = vmlaf(F2V(w3), LVFU(lut[c3]), out);
    return out;
}

} // namespace

#endif // ART_SIMD

namespace {

// Order two axes by decreasing fractional part, carrying their strides.
inline void order_axes(float &fa, int &sa, float &fb, int &sb)
{
    const bool swap = fa < fb;
    const float f = swap ? fb : fa;
    const int s = swap ? sb : sa;
    fb = swap ? fa : fb;
    sb = swap ? sa : sb;
    fa = f;
    sa = s;
}

} // namespace

inline void LUT3D::apply_tetra(float &r, float &g, float &b) const
{
    const float dimMinusOne = dim_minus_one_;
    constexpr int m_components = 3;
    const int m_dim = dim_;
    const float *m_optLut = lut_.data;

    // NaNs become 0.
    const float idx[3] = {LIM(r * dimMinusOne, 0.f, dimMinusOne),
                          LIM(g * dimMinusOne, 0.f, dimMinusOne),
                          LIM(b * dimMinusOne, 0.f, dimMinusOne)};

    // idx is not negative: truncating is flooring
    const int indexLow[3] = {static_cast<int>(idx[0]), static_cast<int>(idx[1]),
                             static_cast<int>(idx[2])};

    float fx = idx[0] - static_cast<float>(indexLow[0]);
    float fy = idx[1] - static_cast<float>(indexLow[1]);
    float fz = idx[2] - static_cast<float>(indexLow[2]);

    // Step to the next node along each axis, none if the coordinate is on a
    // node (as ceil() gave). A positive fraction implies the next node exists.
    int sx = fx > 0.f ? GetLut3DIndexBlueFast(1, 0, 0, m_dim, m_components) : 0;
    int sy = fy > 0.f ? GetLut3DIndexBlueFast(0, 1, 0, m_dim, m_components) : 0;
    int sz = fz > 0.f ? GetLut3DIndexBlueFast(0, 0, 1, m_dim, m_components) : 0;

    const int n000 = GetLut3DIndexBlueFast(indexLow[0], indexLow[1],
                                           indexLow[2], m_dim, m_components);
    const int n111 = n000 + sx + sy + sz;

    // The tetrahedron has the corners n000, n111 and the two on the path
    // from one to the other that steps along the axes by decreasing fraction
    // (this is what the six cases of the usual formulation spell out). Sort
    // the three axes with a network of three compare-and-swaps.
    order_axes(fx, sx, fy, sy);
    order_axes(fy, sy, fz, sz);
    order_axes(fx, sx, fy, sy);

    const int n1 = n000 + sx;
    const int n2 = n1 + sy;

#ifdef ART_SIMD
    vfloat outv = interp_corners(m_optLut, 1 - fx, n000, fx - fy, n1, fy - fz,
                                 n2, fz, n111);

    float out[4] ALIGNED16;
    STVFU(out[0], outv);
#else
    float out[3];

    for (int c = 0; c < 3; ++c) {
        out[c] = (1 - fx) * m_optLut[n000 + c] + (fx - fy) * m_optLut[n1 + c] +
                 (fy - fz) * m_optLut[n2 + c] + (fz)*m_optLut[n111 + c];
    }
#endif // ART_SIMD

    r = out[0];
    g = out[1];
    b = out[2];
}

}} // namespace art::engine
