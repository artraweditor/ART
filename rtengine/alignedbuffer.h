/* -*- C++ -*-
 *
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2004-2012 Gabor Horvath <hgabor@rawtherapee.com>, Oliver Duis
 * <oduis@oliverduis.de>
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

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <utility>

namespace rtengine {

// Aligned buffer that should be faster
template <class T> class AlignedBuffer {

private:
    void *real;
    char alignment;
    size_t allocatedSize;
    int unitSize;

public:
    T *data;

    /** @brief Allocate aligned memory
     * @param size Number of elements of size T to allocate, i.e. allocated size
     * will be sizeof(T)*size ; set it to 0 if you want to defer the allocation
     * @param align Expressed in bytes; SSE instructions need 128 bits
     * alignment, which mean 16 bytes, which is the default value align=0 means
     * unaligned
     */
    AlignedBuffer(size_t size = 0, size_t align = 16)
        : real(nullptr), alignment(align), allocatedSize(0), unitSize(0),
          data(nullptr)
    {
        if (size) {
            resize(size);
        }
    }

    ~AlignedBuffer()
    {
        if (real) {
            free(real);
        }
    }

    /** @brief Return true if there's no memory allocated
     */
    bool isEmpty() const { return allocatedSize == 0; }

    /** @brief Allocate the "size" amount of elements of "structSize" length
     * each
     * @param size number of elements to allocate
     * @param structSize if non null, will let you override the default struct's
     * size (unit: byte)
     * @return True is everything went fine, including freeing memory when
     * size==0, false if the allocation failed
     */
    bool resize(size_t size, int structSize = 0)
    {
        if (size == 0) {
            if (real) {
                free(real);
            }
            real = nullptr;
            data = nullptr;
            allocatedSize = 0;
            unitSize = 0;
            return true;
        }

        size_t elemsz = structSize ? structSize : sizeof(T);
        size_t amount = size * elemsz;
        if (amount != allocatedSize) {
            unitSize = elemsz;
            allocatedSize = amount;
            size_t space = amount + alignment;
            real = realloc(real, space);
            void *p = real;
            if (!p || (alignment && !std::align(alignment, amount, p, space))) {
                if (real) {
                    free(real);
                    real = nullptr;
                    data = nullptr;
                    allocatedSize = 0;
                    unitSize = 0;
                    return false;
                }
            }
            data = static_cast<T *>(p);
        }

        return true;
    }

    void swap(AlignedBuffer<T> &other)
    {
        std::swap(real, other.real);
        std::swap(alignment, other.alignment);
        std::swap(allocatedSize, other.allocatedSize);
        std::swap(data, other.data);
    }

    unsigned int getSize() const
    {
        return unitSize ? allocatedSize / unitSize : 0;
    }
};

} // namespace rtengine
