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

#include "dirtreeview.h"
#include <iostream>

DirTreeView::DirTreeView()
{
    auto commands = UserCommandStore::getInstance()->getAllCommands();

    pmenu = nullptr;
    for (auto &cmd : commands)
    {
        if (cmd.filetype == UserCommand::DIRECTORY)
        {
            if (!pmenu) pmenu = new Gtk::Menu();
            auto item = Gtk::make_managed<Gtk::MenuItem>(cmd.label, true);
            item->signal_activate().connect(sigc::bind(sigc::mem_fun(*this, &DirTreeView::on_menu_item_activate), item));
            pmenu->append(*item);
            menu_commands.emplace_back();
            menu_commands.back().first.reset(item);
            menu_commands.back().second = cmd;
        }
    }

    if (pmenu)
    {
        pmenu->accelerate(*this);
        pmenu->show_all();
    }
}

DirTreeView::~DirTreeView()
{
    delete pmenu;
}

DirTreeView::type_signal_menu_item_activated DirTreeView::signal_menu_item_activated()
{
    return sig_menu_item_activated;
}

bool DirTreeView::on_button_press_event(GdkEventButton* button_event)
{
    bool ret = false;

    ret = TreeView::on_button_press_event(button_event);
    if ((pmenu) && (button_event->type == GDK_BUTTON_PRESS) && (button_event->button == 3))
    {
        pmenu->popup_at_pointer((GdkEvent*)button_event);
    }

    return ret;
}

void DirTreeView::on_menu_item_activate(Gtk::MenuItem* m)
{
    for (size_t i = 0; i < menu_commands.size(); ++i)
    {
        if (m == menu_commands[i].first.get())
        {
            const auto &cmd = menu_commands[i].second;
            sig_menu_item_activated.emit(cmd);
            return;
        }
    }
}
