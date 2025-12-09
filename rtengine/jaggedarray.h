/* -*- C++ -*-
 *
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2016 Ingo Weyrich <heckflosse67@gmx.de>
 *  Copyright (c) 2016 Adam Reichold <adam.reichold@t-online.de>
 *
 *  RawTherapee is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RawTherapee is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RawTherapee.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <cstring>

#include "array2D.h"

namespace rtengine {

// These emulate a jagged array, but use only 2 allocations instead of 1 + H.

template <typename T> class JaggedArray: private array2D<T> {
public:
    JaggedArray(std::size_t width, std::size_t height, bool init_zero = false)
        : array2D<T>(width, height,
                     ARRAY2D_ALIGNED | (init_zero ? ARRAY2D_CLEAR_DATA : 0))
    {
    }

    operator T **() { return array2D<T>::operator T **(); }

    T *operator[](size_t index) { return array2D<T>::operator[](index); }

    T *operator[](int index) { return operator[](size_t(index)); }
};

} // namespace rtengine
