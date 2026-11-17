/* -*- C++ -*-
 *  This file is part of ART.
 *
 *  Copyright (c) 2004-2010 Gabor Horvath <hgabor@rawtherapee.com>
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

// The shapes of the mouse cursor that tools can ask for. Kept apart from
// cursormanager.h (which needs GTK) so that code that is also built without
// the GUI, such as gui/edit.h, does not depend on GTK.

namespace art { namespace gui {


enum CursorShape {
    CSAddColPicker,
    CSArrow,
    CSCropSelect,
    CSCrosshair,
    CSEmpty,
    CSHandClosed,
    CSHandOpen,
    CSMove,
    CSMove1DH,
    CSMove1DV,
    CSMove2D,
    CSMoveLeft,
    CSMoveRight,
    CSMoveRotate,
    CSPlus,
    CSResizeBottomLeft,
    CSResizeBottomRight,
    CSResizeDiagonal,
    CSResizeHeight,
    CSResizeTopLeft,
    CSResizeTopRight,
    CSResizeWidth,
    CSSpotWB,
    CSStraighten,
    CSUndefined,
    CSWait
};


} } // namespace art::gui
