/** -*- C++ -*-
 *
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2020 Alberto Griggio <alberto.griggio@gmail.com>
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

#include <gtkmm.h>

class DateEntry: public Gtk::HBox {
public:
    DateEntry();
    ~DateEntry();

    Glib::Date get_date() const { return date_; }
    void set_date(const Glib::Date &date);

    sigc::signal<void> signal_date_changed() { return sig_date_changed_; }

private:
    void on_button(const GdkEventButton *evt);
    bool on_date_selected(const GdkEventButton *evt);
    void on_enter();

    Gtk::Button *button_;
    Gtk::Entry *entry_;
    Gtk::Calendar *calendar_;
    Gtk::Dialog *dialog_;
    Glib::Date date_;

    sigc::signal<void> sig_date_changed_;
};
