/* -*- C++ -*-
 *
 *  This file is part of RawTherapee.
 *
 *  Copyright 2020 Jean-Christophe FRISCH <natureh.510@gmail.com>
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

#include "procparams.h"
#include "rtengine.h"

namespace rtengine {
namespace procparams {

// Created in own file to speed up build process during elaboration

#define POLY_ALGO 2

#if POLY_ALGO == 1

std::vector<CoordD>
AreaMask::Polygon::get_tessellation(std::vector<Knot> &knots)
{
    // TODO: Find a way to have the appropriate number of points for each
    // subcurve
    constexpr int nbrPoints = 10; // Number of points per curve ;
    constexpr double increment = 1. / double(nbrPoints - 1);

    std::vector<CoordD> poly;
    if (knots.size() < 3) {
        return poly;
    }

    //--------------------  Create list of Line & Bezier curves
    //-------------------

    // NB : A corner starts in the middle of segment(N) and end in the middle of
    // segment(N+1)
    //      The the first point of a corner == the last point of the previous
    //      corner, so we skip the very first point of each corner when building
    //      'poly'

    CoordD cornerKnots[3];
    CoordD middleStart;
    CoordD middleEnd;
    CoordD currStart;
    CoordD currEnd;

    for (size_t i = 0; i < knots.size(); ++i) {
        if (i == 0) {
            size_t a = knots.size() - 1;
            cornerKnots[0].set(knots[a].x, knots[a].y);
            cornerKnots[1].set(knots[i].x, knots[i].y);
            cornerKnots[2].set(knots[i + 1].x, knots[i + 1].y);
        } else if (i == knots.size() - 1) {
            cornerKnots[0].set(knots[i - 1].x, knots[i - 1].y);
            cornerKnots[1].set(knots[i].x, knots[i].y);
            cornerKnots[2].set(knots[0].x, knots[0].y);
        } else {
            cornerKnots[0].set(knots[i - 1].x, knots[i - 1].y);
            cornerKnots[1].set(knots[i].x, knots[i].y);
            cornerKnots[2].set(knots[i + 1].x, knots[i + 1].y);
        }
        middleStart = (cornerKnots[0] + cornerKnots[1]) * 0.5;
        middleEnd = (cornerKnots[1] + cornerKnots[2]) * 0.5;

        if (knots[i].roundness == 0.) {
            poly.push_back(cornerKnots[1]);
            poly.push_back(middleEnd);
        } else {
            // Start/End of curves are necessary
            double r = (100. - knots[i].roundness) / 100.;
            currStart = (cornerKnots[1] - middleStart) * r + middleStart;
            currEnd = (cornerKnots[1] - middleEnd) * r + middleEnd;

            /*  First point is skipped
            poly.push_back(currStart);
            */

            for (int k = 1; k < (nbrPoints - 1); k++) {
                double t = k * increment;
                double t2 = t * t;
                double tr = 1. - t;
                double tr2 = tr * tr;
                double tr2t = tr * 2 * t;

                // adding a point to the polyline
                CoordD point(tr2 * currStart.x + tr2t * cornerKnots[1].x +
                                 t2 * currEnd.x,
                             tr2 * currStart.y + tr2t * cornerKnots[1].y +
                                 t2 * currEnd.y);
                poly.push_back(point);
            }

            // adding the last point of the sub-curve
            poly.push_back(currEnd);

            // adding the remaining segment
            poly.push_back(middleEnd);
        }
    }

    return poly;
}

#endif

#if POLY_ALGO == 2

std::vector<CoordD>
AreaMask::Polygon::get_tessellation(std::vector<Knot> &knots)
{
    std::vector<CoordD> poly;
    if (knots.size() < 3) {
        return poly;
    }

    //--------------------  Create list of Line & Bezier curves
    //-------------------

    // NB : A corner starts in the middle of segment(N) and end in the middle of
    // segment(N+1)
    //      The the first point of a corner == the last point of the previous
    //      corner, so we skip the very first point of each corner when building
    //      'poly'

    CoordD cornerKnots[3];
    CoordD prevEnd;
    CoordD currStart;
    CoordD currEnd;
    CoordD nextStart;

    for (size_t i = 0; i < knots.size(); ++i) {
        double prevRoundness;
        double nextRoundness;

        if (i == 0) {
            size_t a = knots.size() - 1;
            cornerKnots[0].set(knots[a].x, knots[a].y);
            cornerKnots[1].set(knots[i].x, knots[i].y);
            cornerKnots[2].set(knots[i + 1].x, knots[i + 1].y);
            prevRoundness = knots[a].roundness / 100.;
            nextRoundness = knots[i + 1].roundness / 100.;
        } else if (i == knots.size() - 1) {
            cornerKnots[0].set(knots[i - 1].x, knots[i - 1].y);
            cornerKnots[1].set(knots[i].x, knots[i].y);
            cornerKnots[2].set(knots[0].x, knots[0].y);
            prevRoundness = knots[i - 1].roundness / 100.;
            nextRoundness = knots[0].roundness / 100.;
        } else {
            cornerKnots[0].set(knots[i - 1].x, knots[i - 1].y);
            cornerKnots[1].set(knots[i].x, knots[i].y);
            cornerKnots[2].set(knots[i + 1].x, knots[i + 1].y);
            prevRoundness = knots[i - 1].roundness / 100.;
            nextRoundness = knots[i + 1].roundness / 100.;
        }

        double roundness = knots[i].roundness / 100.;

        if (prevRoundness == 0. && roundness == 0.) {
            poly.push_back(cornerKnots[1]);
            continue;
        }

        prevEnd =
            (cornerKnots[1] - cornerKnots[0]) * prevRoundness + cornerKnots[0];
        nextStart =
            (cornerKnots[1] - cornerKnots[2]) * nextRoundness + cornerKnots[2];
        currStart =
            (cornerKnots[0] - cornerKnots[1]) * roundness + cornerKnots[1];
        currEnd =
            (cornerKnots[2] - cornerKnots[1]) * roundness + cornerKnots[1];

        if ((prevRoundness + roundness) > 1.) {
            // prev knot roundness and current knot roundness are overlaping
            currStart = (prevEnd + currStart) * 0.5;
        } else {
            // prev knot roundness and current knot roundness are not
            // overlaping, adding the conection line
            poly.push_back(currStart);
        }

        if ((nextRoundness + roundness) > 1.) {
            // next knot roundness and current knot roundness are overlaping
            currEnd = (currEnd + nextStart) * 0.5;
        } else {
            // next knot roundness and current knot roundness are not
            // overlaping, adding the conection line
            // poly.push_back(nextStart);
        }

        // evaluating the required number of points for a smooth shape
        // based on the length of the local "cage" in pixels
        CoordD dist1(cornerKnots[1] - currStart);
        CoordD dist2(cornerKnots[1] - currEnd);

        int nbrPoints = rtengine::max<int>(
            int((dist1.getLength() + dist2.getLength()) / 10.),
            5); // one segment for 5 px ;
        double increment = 1. / double(nbrPoints - 1);

        for (int k = 1; k < (nbrPoints - 1); k++) {
            double t = k * increment;
            double t2 = t * t;
            double tr = 1. - t;
            double tr2 = tr * tr;
            double tr2t = tr * 2 * t;

            // adding a point to the polyline
            CoordD point(
                tr2 * currStart.x + tr2t * cornerKnots[1].x + t2 * currEnd.x,
                tr2 * currStart.y + tr2t * cornerKnots[1].y + t2 * currEnd.y);
            poly.push_back(point);
        }

        // adding the last point of the sub-curve
        poly.push_back(currEnd);
    }

    return poly;
}

#endif

} // namespace procparams
} // namespace rtengine
