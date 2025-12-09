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

#pragma once

namespace rtengine {

struct Coord;
struct CoordD;
struct PolarCoord;

// Do not confuse with Coord2D, this one is used by the UI with type Int
struct Coord {
    int x = 0;
    int y = 0;

    Coord() = default;
    Coord(const int x, const int y);
    Coord(const Coord &other) = default;
    explicit Coord(const PolarCoord &other);

    Coord &operator=(const Coord &other) = default;
    Coord &operator=(const PolarCoord &other);

    void get(int &x, int &y) const;
    void set(const int x, const int y);

    bool clip(const int width, const int height);

    Coord &operator+=(const Coord &other);
    Coord &operator+=(const CoordD &other);
    Coord &operator-=(const Coord &other);
    Coord &operator-=(const CoordD &other);
    Coord &operator*=(const double scale);
    bool operator<(const Coord &rhs) const;
    bool operator>(const Coord &rhs) const;
    bool operator<=(const Coord &rhs) const;
    bool operator>=(const Coord &rhs) const;
};

bool operator==(const Coord &lhs, const Coord &rhs);
bool operator!=(const Coord &lhs, const Coord &rhs);

const Coord operator+(const Coord &lhs, const Coord &rhs);
const Coord operator-(const Coord &lhs, const Coord &rhs);
const Coord operator*(const Coord &lhs, const double rhs);
const Coord operator*(const double lhs, const Coord &rhs);

// Do not confuse with Coord2D, this one is used by the UI with type Double.
struct CoordD {
    double x = 0;
    double y = 0;

    CoordD() = default;
    CoordD(const double x, const double y);
    CoordD(const CoordD &other) = default;
    explicit CoordD(const PolarCoord &other);

    CoordD &operator=(const CoordD &other) = default;
    CoordD &operator=(const PolarCoord &other);

    void get(double &x, double &y) const;
    void set(const double x, const double y);

    bool clip(const int width, const int height);
    double getLength();

    CoordD &operator+=(const CoordD &other);
    CoordD &operator+=(const Coord &other);
    CoordD &operator-=(const CoordD &other);
    CoordD &operator-=(const Coord &other);
    CoordD &operator*=(const double scale);
    bool operator<(const CoordD &rhs) const;
    bool operator>(const CoordD &rhs) const;
    bool operator<=(const CoordD &rhs) const;
    bool operator>=(const CoordD &rhs) const;
};

bool operator==(const CoordD &lhs, const CoordD &rhs);
bool operator!=(const CoordD &lhs, const CoordD &rhs);

const CoordD operator+(const CoordD &lhs, const CoordD &rhs);
const CoordD operator-(const CoordD &lhs, const CoordD &rhs);
const CoordD operator*(const CoordD &lhs, const double rhs);
const CoordD operator*(const double lhs, const CoordD &rhs);

struct PolarCoord {
    double radius = 0.0;
    double angle = 0.0;

    PolarCoord() = default;
    PolarCoord(const double radius, const double angle);
    PolarCoord(const PolarCoord &other) = default;
    explicit PolarCoord(const Coord &other);
    explicit PolarCoord(const CoordD &other);

    PolarCoord &operator=(const PolarCoord &other) = default;
    PolarCoord &operator=(const Coord &other);
    PolarCoord &operator=(const CoordD &other);

    void get(double &radius, double &angle) const;
    void set(const double radius, const double angle);

    PolarCoord &operator+=(const PolarCoord &other);
    PolarCoord &operator-=(const PolarCoord &other);
    PolarCoord &operator*=(const double scale);
};

bool operator==(const PolarCoord &lhs, const PolarCoord &rhs);
bool operator!=(const PolarCoord &lhs, const PolarCoord &rhs);

const PolarCoord operator+(const PolarCoord &lhs, const PolarCoord &rhs);
const PolarCoord operator-(const PolarCoord &lhs, const PolarCoord &rhs);
const PolarCoord operator*(const PolarCoord &lhs, const double rhs);
const PolarCoord operator*(const double lhs, const PolarCoord &rhs);

inline Coord::Coord(const int x, const int y): x(x), y(y) {}

inline Coord::Coord(const PolarCoord &other) { *this = other; }

inline void Coord::get(int &x, int &y) const
{
    x = this->x;
    y = this->y;
}

inline void Coord::set(const int x, const int y)
{
    this->x = x;
    this->y = y;
}

inline Coord &Coord::operator+=(const Coord &other)
{
    x += other.x;
    y += other.y;
    return *this;
}

inline Coord &Coord::operator+=(const CoordD &other)
{
    x += int(other.x + 0.5);
    y += int(other.y + 0.5);
    return *this;
}

inline Coord &Coord::operator-=(const Coord &other)
{
    x -= other.x;
    y -= other.y;
    return *this;
}

inline Coord &Coord::operator-=(const CoordD &other)
{
    x -= int(other.x + 0.5);
    y -= int(other.y + 0.5);
    return *this;
}

inline Coord &Coord::operator*=(const double scale)
{
    x *= scale;
    y *= scale;
    return *this;
}

inline bool Coord::operator<(const Coord &rhs) const
{
    return x < rhs.x && y < rhs.y;
}

inline bool Coord::operator>(const Coord &rhs) const
{
    return x > rhs.x && y > rhs.y;
}

inline bool Coord::operator<=(const Coord &rhs) const
{
    return x <= rhs.x && y <= rhs.y;
}

inline bool Coord::operator>=(const Coord &rhs) const
{
    return x >= rhs.x && y >= rhs.y;
}

inline bool operator==(const Coord &lhs, const Coord &rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

inline bool operator!=(const Coord &lhs, const Coord &rhs)
{
    return !(lhs == rhs);
}

inline const Coord operator+(const Coord &lhs, const Coord &rhs)
{
    return Coord(lhs) += rhs;
}

inline const Coord operator-(const Coord &lhs, const Coord &rhs)
{
    return Coord(lhs) -= rhs;
}

inline const Coord operator*(const Coord &lhs, const double rhs)
{
    return Coord(lhs) *= rhs;
}

inline const Coord operator*(const double lhs, const Coord &rhs)
{
    return Coord(rhs) *= lhs;
}

inline CoordD::CoordD(const double x, const double y): x(x), y(y) {}

inline CoordD::CoordD(const PolarCoord &other) { *this = other; }

inline void CoordD::get(double &x, double &y) const
{
    x = this->x;
    y = this->y;
}

inline void CoordD::set(const double x, const double y)
{
    this->x = x;
    this->y = y;
}

inline CoordD &CoordD::operator+=(const CoordD &other)
{
    x += other.x;
    y += other.y;
    return *this;
}

inline CoordD &CoordD::operator+=(const Coord &other)
{
    x += double(other.x);
    y += double(other.y);
    return *this;
}

inline CoordD &CoordD::operator-=(const CoordD &other)
{
    x -= other.x;
    y -= other.y;
    return *this;
}

inline CoordD &CoordD::operator-=(const Coord &other)
{
    x -= double(other.x);
    y -= double(other.y);
    return *this;
}

inline CoordD &CoordD::operator*=(const double scale)
{
    x *= scale;
    y *= scale;
    return *this;
}

inline bool CoordD::operator<(const CoordD &rhs) const
{
    return x < rhs.x && y < rhs.y;
}

inline bool CoordD::operator>(const CoordD &rhs) const
{
    return x > rhs.x && y > rhs.y;
}

inline bool CoordD::operator<=(const CoordD &rhs) const
{
    return x <= rhs.x && y <= rhs.y;
}

inline bool CoordD::operator>=(const CoordD &rhs) const
{
    return x >= rhs.x && y >= rhs.y;
}

inline bool operator==(const CoordD &lhs, const CoordD &rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

inline bool operator!=(const CoordD &lhs, const CoordD &rhs)
{
    return !(lhs == rhs);
}

inline const CoordD operator+(const CoordD &lhs, const CoordD &rhs)
{
    return CoordD(lhs) += rhs;
}

inline const CoordD operator-(const CoordD &lhs, const CoordD &rhs)
{
    return CoordD(lhs) -= rhs;
}

inline const CoordD operator*(const CoordD &lhs, const double rhs)
{
    return CoordD(lhs) *= rhs;
}

inline const CoordD operator*(const double lhs, const CoordD &rhs)
{
    return CoordD(rhs) *= lhs;
}

inline PolarCoord::PolarCoord(const double radius, const double angle)
    : radius(radius), angle(angle)
{
}

inline PolarCoord::PolarCoord(const Coord &other) { *this = other; }

inline PolarCoord::PolarCoord(const CoordD &other) { *this = other; }

inline void PolarCoord::get(double &radius, double &angle) const
{
    radius = this->radius;
    angle = this->angle;
}

inline void PolarCoord::set(const double radius, const double angle)
{
    this->radius = radius;
    this->angle = angle;
}

inline PolarCoord &PolarCoord::operator+=(const PolarCoord &other)
{
    *this = Coord(*this) + Coord(other);
    return *this;
}

inline PolarCoord &PolarCoord::operator-=(const PolarCoord &other)
{
    *this = Coord(*this) - Coord(other);
    return *this;
}

inline PolarCoord &PolarCoord::operator*=(const double scale)
{
    radius *= scale;
    return *this;
}

inline bool operator==(const PolarCoord &lhs, const PolarCoord &rhs)
{
    return lhs.radius == rhs.radius && lhs.angle == rhs.angle;
}

inline bool operator!=(const PolarCoord &lhs, const PolarCoord &rhs)
{
    return !(lhs == rhs);
}

inline const PolarCoord operator+(const PolarCoord &lhs, const PolarCoord &rhs)
{
    return PolarCoord(lhs) += rhs;
}

inline const PolarCoord operator-(const PolarCoord &lhs, const PolarCoord &rhs)
{
    return PolarCoord(lhs) -= rhs;
}

inline const PolarCoord operator*(const PolarCoord &lhs, const double rhs)
{
    return PolarCoord(lhs) *= rhs;
}

inline const PolarCoord operator*(const double lhs, const PolarCoord &rhs)
{
    return PolarCoord(rhs) *= lhs;
}

} // namespace rtengine
