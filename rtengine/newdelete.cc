/** -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright (c) 2021 Alberto Griggio <alberto.griggio@gmail.com>
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

#include <new>
#include <stdlib.h>

void operator delete(void *p) noexcept
{
    if (p) {
        free(p);
    }
}

void operator delete[](void *p) noexcept
{
    if (p) {
        free(p);
    }
}

void *operator new(std::size_t n) noexcept(false)
{
    void *ret = malloc(n);
    if (!ret) {
        throw std::bad_alloc();
    }
    return ret;
}

void *operator new[](std::size_t n) noexcept(false)
{
    void *ret = malloc(n);
    if (!ret) {
        throw std::bad_alloc();
    }
    return ret;
}

void *operator new(std::size_t n, const std::nothrow_t &tag) noexcept
{
    (void)(tag);
    return malloc(n);
}

void *operator new[](std::size_t n, const std::nothrow_t &tag) noexcept
{
    (void)(tag);
    return malloc(n);
}
