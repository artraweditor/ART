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

#include "ipc_transport.h"
#include <functional>
#include <string>
#include <vector>

namespace art { namespace engine { namespace remote { namespace wire {

// The protocol between `ART-cli --cmd` and `ART-cli --server`, one request per
// connection.
//
// Request:  "ART1 <len>\n" followed by <len> bytes: the fields `cwd`, a
//           reserved flag ("0" or "1"; it used to ask for marked output and
//           is now ignored; clients send "0") and then the command line (verb
//           and arguments), each terminated by a NUL byte. Arguments are therefore exact: they can
//           hold spaces, newlines, or start with "--".
// Response: a sequence of frames, "O <len>\n<bytes>" (for the client's
//           stdout: the JSON reply document of the command, one line) and
//           "E <len>\n<bytes>" (stderr), ended by "X <status>\n" (the exit
//           status of the client).

struct Request {
    std::string cwd;
    std::vector<std::string> args; // args[0] is the verb
};

constexpr size_t MAX_REQUEST = 1 << 20;
constexpr size_t MAX_FRAME = 64 << 20;

bool writeRequest(ipc::Connection &c, const Request &r);
// false and `err` if nothing valid arrived (the peer is gone, or it does not
// speak this protocol)
bool readRequest(ipc::Connection &c, Request &r, std::string &err);

// kind is 'O' or 'E'
bool writeFrame(ipc::Connection &c, char kind, const std::string &data);
bool writeStatus(ipc::Connection &c, int status);

// Read the response, calling onData('O' or 'E', text) for each frame, until
// the status frame. False and `err` if the stream ends or breaks before.
bool readResponse(ipc::Connection &c,
                  const std::function<void(char, const std::string &)> &onData,
                  int &status, std::string &err);

}}}} // namespace art::engine::remote::wire
