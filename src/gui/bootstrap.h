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

#include "../engine/remote/editsession.h"
#include <functional>
#include <string>

namespace art { namespace gui { namespace bootstrap {

// Load the options (and with them initialise the engine: Options::load ends
// with art::engine::init) and apply the fallbacks for a missing default
// profile. `options.ART_base_dir` must be set already. The environment
// variables ART_SETTINGS and ART_CACHE select the configuration and cache
// directories, as for the programs.
//
// Returns 0, or -2 if the options cannot be loaded: the message is then
// stored in *error, or printed on stderr if `error` is null. `phase`, if
// given, is told of the milestones (used for the ART_PROFILE timings).
// The preferences of the application, as the active image of the server mode
// (art::engine::EditSession) needs them. `options` must be loaded.
art::engine::EditSession::Hooks editSessionHooks();

int init(bool quickstart, int verbose, std::string *error = nullptr,
         const std::function<void(const char *)> &phase = nullptr);

}}} // namespace art::gui::bootstrap
