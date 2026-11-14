/* -*- C++ -*-
 *
 *  This file is part of ART.
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

#include <giomm.h>
#include <glibmm.h>
#include <vector>

namespace art { namespace gui {

class RTWindow;

namespace remotecmd {

// Execute the remote command `args` (args[0] is the verb) in the running
// instance. Must be called from the GUI thread. Output goes to the client's
// stdout/stderr; the exit status is set on `cl`. Commands that complete
// asynchronously keep a reference to `cl` until they are done (the client
// waits for that).
void run(RTWindow *win, const Glib::RefPtr<Gio::ApplicationCommandLine> &cl,
         const std::vector<Glib::ustring> &args);

} // namespace remotecmd

} } // namespace art::gui
