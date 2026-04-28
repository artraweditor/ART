/* -*- C++ -*-
 *
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2018 Jean-Christophe FRISCH <natureh.510@gmail.com>
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

#include <gtkmm/image.h>

/**
 * @brief A master class for derived class of Gtk::Image in order to handle
 * theme-related icon sets.
 */
class RTScalable {
    static int global_display_scale_;
    static Gtk::TextDirection direction_; // cached value for text-direction
    
protected:
    static Cairo::RefPtr<Cairo::ImageSurface>
    loadImage(const Glib::ustring &fname, int scale=0);
    static Gtk::TextDirection getDirection();

public:
    static void init(Gtk::Window *window);
    static int getGlobalDisplayScale();
    static int getDisplayScale(const Gtk::Widget *w);

    static void setDisplayScale(const Cairo::RefPtr<Cairo::Surface> &surface,
                                int scale);
    static int getDisplayScale(const Cairo::RefPtr<Cairo::Surface> &surface);
};
