/* -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright 2022 Alberto Griggio <alberto.griggio@gmail.com>
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

#include "options.h"
#include <glibmm.h>
#include <vector>

namespace art {
namespace session {

bool check(const Glib::ustring &fname);
Glib::ustring filename();
Glib::ustring path();
void load(const Glib::ustring &fname);
void save(const Glib::ustring &fname);
void clear();
void add(const std::vector<Glib::ustring> &fnames);
void remove(const std::vector<Glib::ustring> &fnames);
std::vector<Glib::ustring> list();

} // namespace session
} // namespace art
