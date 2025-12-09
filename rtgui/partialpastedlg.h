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

#include "paramsedited.h"
#include <gtkmm.h>
#include <string>
#include <unordered_map>

class PartialPasteDlg: public Gtk::Dialog {
public:
    PartialPasteDlg(const Glib::ustring &title, Gtk::Window *parent);
    void set_allow_3way(bool yes) { allow_3way_ = yes; }
    ParamsEdited getParamsEdited();

private:
    ParamsEdited pedited_;

    Gtk::CheckButton *everything_;
    sigc::connection everything_conn_;

    struct ButtonInfo {
        sigc::connection conn;
        std::vector<Gtk::CheckButton *> related;
        bool is_master;
        bool *edited;
        unsigned *edited3;
    };
    std::unordered_map<Gtk::CheckButton *, ButtonInfo> buttons_;
    bool allow_3way_;

    void toggled(Gtk::CheckButton *which);
};
