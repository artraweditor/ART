/** -*- C++ -*-
 *
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2018 Alberto Griggio <alberto.griggio@gmail.com>
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

#include "../rtengine/procparams.h"
#include "edit.h"
#include <gtkmm.h>

using rteMaskShape = rtengine::procparams::AreaMask::Shape;
using rteMaskRect = rtengine::procparams::AreaMask::Rectangle;
using rteMaskPoly = rtengine::procparams::AreaMask::Polygon;

class AreaMask: public EditSubscriber {
public:
    enum class DraggedElement { NONE, POINT, ROUNDNESS, SEGMENT, WHOLE };

    AreaMask();
    ~AreaMask();

    CursorShape getCursor(int objectID) override;
    bool mouseOver(int modifierKey) override;
    bool button1Pressed(int modifierKey) override;
    bool button1Released() override;
    bool drag1(int modifierKey) override;
    bool button3Pressed(int modifierKey) override;
    bool pick3(bool picked) override;
    bool scroll(int modifierKey, GdkScrollDirection direction, double deltaX,
                double deltaY, bool &propagateEvent) override;

    size_t getPolygonSize();
    void setPolygon(const std::vector<rteMaskPoly::Knot> &new_poly);
    std::vector<rteMaskPoly::Knot> getPolygon();
    void clearPolygon();
    void setGeometryType(rteMaskShape::Type newType);
    rteMaskShape::Type getGeometryType();
    void deleteGeometry();
    void createRectangleGeometry();
    void createPolygonGeometry();
    void createGradientGeometry();
    void updateGeometry(const int fullWidth = -1, const int fullHeight = -1);

protected:
    int last_object_;
    double dragged_point_old_angle_;
    double dragged_point_adjuster_angle_;
    double dragged_feather_offset_;
    rtengine::Coord dragged_center_;
    double center_x_;
    double center_y_;
    double width_;
    double height_;
    double strength_start_;
    double strength_end_;
    double feather_;
    double angle_;

    int top_id_;
    int bottom_id_;
    int left_id_;
    int right_id_;
    int rotate_w_id_;
    int rotate_h_id_;
    int center_id_;

    // Visible (and MouseOver) geometry for Polygon
    Line *insertion_line;            // [0]    visible
    PolyLine *curve;                 // [1]    visible
    PolyLine *cage;                  // [2]    visible
    std::vector<Line *> segments_MO; // [3, n]           hoverable
    Circle *sel_knot;                // [n+1]  visible / hoverable
    Circle *sel_knot_bg_;
    Circle *prev_knot; // [n+2]  visible / hoverable
    Circle *next_knot; // [n+3]  visible / hoverable

    int hovered_line_id_;            // range identical to poly_knots_
    int sel_poly_knot_id_;           // range identical to poly_knots_
    int prev_poly_knot_id_;          // range identical to poly_knots_
    int next_poly_knot_id_;          // range identical to poly_knots_
    DraggedElement dragged_element_; // true if adjusting the Roundness value
    std::vector<rtengine::CoordD>
        dragged_points_; // copy of initial points for dragging and bounds
                         // handling

    // Gradient geometry IDs
    int h_line_id_;
    int v_line_id_;
    int feather_line1_id_;
    int feather_line2_id_;
    int center_circle_id_;

private:
    void setPolylineSize(size_t newSize);
    void initHoverGeometry();

    std::vector<rteMaskPoly::Knot> poly_knots_;
    rteMaskShape::Type geomType;
};
