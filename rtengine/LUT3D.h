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
#pragma once

#include "alignedbuffer.h"
#include "rt_math.h"
#include <vector>

namespace rtengine {

class LUT3D {
public:
    class initializer {
    public:
        virtual ~initializer();
        virtual void operator()(float &r, float &g, float &b) = 0;
    };

    LUT3D();
    ~LUT3D();

    void init(int dim, initializer &f, bool input_is_01 = true);
    bool operator()(float &r, float &g, float &b);

    int dimension() const { return dim_; }
    operator bool() const;

private:
    void apply_tetra(float &r, float &g, float &b);

    bool input_is_01_;
    int dim_;
    float dim_minus_one_;
    AlignedBuffer<float> lut_;
};

} // namespace rtengine
