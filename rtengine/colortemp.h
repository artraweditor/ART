/* -*- C++ -*-
 *
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2004-2010 Gabor Horvath <hgabor@rawtherapee.com>
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
#ifndef _COLORTEMP_
#define _COLORTEMP_

#include <array>
#include <cmath>
#include <map>
#include <string>

namespace rtengine {

constexpr double MINTEMP = 1500.0;
constexpr double MAXTEMP = 60000.0;
constexpr double MINGREEN = 0.02;
constexpr double MAXGREEN = 10.0;
constexpr double MINEQUAL = 0.8;
constexpr double MAXEQUAL = 1.5;
constexpr double INITIALBLACKBODY = 4000.0;
constexpr double MAX_WB_MUL = 20.0;

class ColorTemp {
public:
    ColorTemp();
    explicit ColorTemp(double e);
    ColorTemp(double t, double g, double e, const std::string &m);
    ColorTemp(double mulr, double mulg, double mulb, double e);
    ColorTemp(double mulr, double mulg, double mulb);

    void update(const double rmul, const double gmul, const double bmul,
                const double equal);

    void useDefaults(const double equal);

    bool clipped() const { return clipped_; }

    inline double getTemp() const { return temp; }

    inline double getGreen() const { return green; }

    inline double getEqual() const { return equal; }

    void getMultipliers(double &mulr, double &mulg, double &mulb) const;

    void mul2temp(const double rmul, const double gmul, const double bmul,
                  const double equal, double &temp, double &green) const;

    bool operator==(const ColorTemp &other) const;
    bool operator!=(const ColorTemp &other) const;

private:
    void clip(double &temp, double &green) const;
    void clip(double &temp, double &green, double &equal) const;
    void temp2mul(double temp, double green, double equal, double &rmul,
                  double &gmul, double &bmul) const;

    enum Mode { TEMP_TINT, MULTIPLIERS };
    Mode mode_;
    double temp;
    double green;
    double equal;
    std::array<double, 3> mult_;
    mutable bool clipped_;
};

} // namespace rtengine
#endif
