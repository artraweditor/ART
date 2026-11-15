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

#include <glibmm/ustring.h>
#include <memory>
#include <string>

namespace art { namespace engine { namespace remote {

// Where the output of one remote command goes. Implemented by each transport
// (the GUI's GApplication command line, the ART-cli server's socket).
class ReplySink {
public:
    virtual ~ReplySink() = default;
    // text for the client's stdout / stderr (already newline-terminated)
    virtual void out(const std::string &s) = 0;
    virtual void err(const std::string &s) = 0;
    // the command is complete: set the client's exit status and release it
    virtual void finish(int status) = 0;
};

// Completion of a command: `ok` and a message (printed to stdout on success),
// or `fail` (an "ART: error:" line on stderr, exit status 1). Only the first
// ok()/fail() takes effect.
class Reply {
public:
    // `mark`: prefix every output line with \x01, so that a programmatic
    // client (the MCP server, ART_CMD_MARK=1) can tell real output from
    // unrelated messages printed by the client process
    Reply(std::shared_ptr<ReplySink> sink, bool mark);

    void ok(const Glib::ustring &msg = "");
    void fail(const Glib::ustring &msg);
    void print(const Glib::ustring &msg);
    // ok() or fail() has been called
    bool done() const { return done_; }

private:
    std::shared_ptr<ReplySink> sink_;
    bool mark_;
    bool done_;
};

}}} // namespace art::engine::remote
