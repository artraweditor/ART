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

#include "json.h"
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

// The reply document of a failed command (what Reply::fail() sends to stdout),
// for the clients to use when the command could not even be sent
std::string failureDocument(const Glib::ustring &msg);

// Completion of a command. Every command ends with exactly one call of ok()
// or fail(), which writes one JSON document, on a single line, to stdout:
//
//   {"ok":true}                      success, no data
//   {"ok":true,"result":<value>}     success
//   {"ok":false,"error":"message"}   failure
//
// A failure also writes an "ART: error: message" line to stderr, and sets exit
// status 1. Nothing else is ever written to stdout. Only the first
// ok()/fail() takes effect.
class Reply {
public:
    explicit Reply(std::shared_ptr<ReplySink> sink);

    void ok();
    void ok(const json::Value &result);
    void fail(const Glib::ustring &msg);
    // ok() or fail() has been called
    bool done() const { return done_; }

private:
    std::shared_ptr<ReplySink> sink_;
    bool done_;
};

}}} // namespace art::engine::remote
