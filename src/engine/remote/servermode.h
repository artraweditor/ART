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

#include "editsession.h"
#include <iosfwd>
#include <string>
#include <vector>

namespace art { namespace engine { namespace servermode {

// ART-cli --server: a long-running process that keeps one image open and
// executes the commands sent by other ART-cli processes (--cmd) over a Unix
// domain socket. The commands are those of the GUI's `--cmd` that work on the
// active image, plus a few of its own (see printHelp()).

struct Options {
    enum Mode { NONE, SERVER, CLIENT };
    Mode mode = NONE;
    std::string socket;              // "" for the default
    std::vector<std::string> cmd;    // for CLIENT: the verb and its arguments
    // for SERVER: the preferences of the application (not set by parseArgs)
    EditSession::Hooks hooks;
};

// Recognise `[--socket PATH] --server` and `[--socket PATH] --cmd VERB ARGS...`
// (--cmd takes everything that follows). Returns false, leaving `opts` with
// mode NONE, for any other command line; `err` is set if it was meant for the
// server mode but is malformed.
bool parseArgs(int argc, char **argv, Options &opts, std::string &err);

// the command line of `--cmd help`
void printHelp(std::ostream &out);

// Send the command to the server and relay its output and exit status. Needs
// no engine, options or GTK: call it before any initialisation.
int runClient(const Options &opts);

// Run the server until it is told to quit (the `quit` command, SIGINT or
// SIGTERM). The engine must be initialised. Returns the exit code.
int runServer(const Options &opts, int verbose);

}}} // namespace art::engine::servermode
