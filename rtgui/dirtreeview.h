/* -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright 2025 Balázs Terényi
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

#include "extprog.h"
#include <gtkmm.h>

class DirTreeView: public Gtk::TreeView {
public:
    DirTreeView();
    virtual ~DirTreeView();

    typedef sigc::signal<void, const UserCommand &>
        type_signal_menu_item_activated;
    type_signal_menu_item_activated signal_menu_item_activated();

protected:
    bool on_button_press_event(GdkEventButton *button_event) override;
    void on_menu_item_activate(Gtk::MenuItem *m);

    type_signal_menu_item_activated sig_menu_item_activated;
    Gtk::Menu *pmenu;
    std::vector<std::pair<std::unique_ptr<Gtk::MenuItem>, UserCommand>>
        menu_commands;
};
