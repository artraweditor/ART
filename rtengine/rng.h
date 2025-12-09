/* -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright 2025 Alberto Griggio <alberto.griggio@gmail.com>
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

#include "rt_math.h"
#include "sleef.h"
#include "sleefsseavx.h"
#include <inttypes.h>
#include <limits>
#include <math.h>

namespace rtengine {

class RandomNumberGenerator {
public:
    RandomNumberGenerator(uint32_t seed): seed_(seed) { assert(seed_); }

    uint32_t
    randint(uint32_t upper_bound = std::numeric_limits<uint32_t>::max())
    {
        seed_ = next();
        return uint32_t(seed_ >> 16U) % upper_bound;
    }

    float randfloat()
    {
        uint32_t ub = std::numeric_limits<int>::max();
        return float(randint(ub)) / float(ub);
    }

private:
    uint64_t next() const { return ((seed_ * a_) + c_) & mask_; }

    static constexpr uint64_t a_ = 25214903917ULL;
    static constexpr uint64_t c_ = 11U;
    static constexpr uint64_t mask_ = ~(2ULL << 48);
    uint64_t seed_;
};

// see https://en.wikipedia.org/wiki/Marsaglia_polar_method
class NormalDistribution {
public:
    explicit NormalDistribution(float mean = 0, float std_dev = 1)
        : mean_(mean), std_dev_(std_dev), spare_(0), has_spare_(false)
    {
    }

    float operator()(RandomNumberGenerator &rng)
    {
        if (has_spare_) {
            has_spare_ = false;
            return spare_ * std_dev_ + mean_;
        } else {
            double u, v, s;
            do {
                u = rng.randfloat() * 2.f - 1.f;
                v = rng.randfloat() * 2.f - 1.f;
                s = u * u + v * v;
            } while (s >= 1.f || s == 0.f);
            s = sqrtf(-2.f * xlogf(s) / s);
            spare_ = v * s;
            has_spare_ = true;
            return mean_ + std_dev_ * u * s;
        }
    }

private:
    const float mean_;
    const float std_dev_;
    float spare_;
    bool has_spare_;
};

} // namespace rtengine
