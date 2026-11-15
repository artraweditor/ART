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

#include <cstddef>
#include <memory>
#include <string>

namespace art { namespace engine { namespace remote { namespace ipc {

// A connected stream (one client talking to the server). The transport is
// deliberately tiny so that another one (e.g. named pipes on Windows) can be
// added behind it.
class Connection {
public:
    virtual ~Connection() = default;
    // read exactly `n` bytes; false on end of stream, error or timeout
    virtual bool read(void *buf, size_t n) = 0;
    // write all `n` bytes; false on error (e.g. the peer is gone)
    virtual bool write(const void *buf, size_t n) = 0;
};

class Listener {
public:
    virtual ~Listener() = default;
    // Wait up to `timeout_ms` (-1: forever) for a client. Returns nullptr if
    // none came, or if the wait was interrupted by a signal. Clients that are
    // not the same user as the server are refused. A request must arrive
    // within a few seconds of the connection, or reading it fails.
    virtual std::unique_ptr<Connection> accept(int timeout_ms) = 0;
};

// The socket the server listens on by default: $ART_SERVER_SOCKET if set,
// else server.sock in a private (0700, owned by the user) directory,
// $XDG_RUNTIME_DIR/ART or $TMPDIR/ART-$USER, which is created if needed.
// Returns "" and sets `err` if no safe place can be found, or on a platform
// without a transport.
std::string defaultSocketPath(std::string &err);

// Connect to a server; nullptr and `err` on failure.
std::unique_ptr<Connection> connect(const std::string &path, std::string &err);

// Start listening on `path` (a Unix domain socket, created with mode 0600 and
// removed again when the listener is destroyed). If something is there
// already: a server that answers means failure ("already running"); a socket
// that nobody listens on (left by a crashed server) is replaced; anything
// else is an error. nullptr and `err` on failure.
std::unique_ptr<Listener> listen(const std::string &path, std::string &err);

}}}} // namespace art::engine::remote::ipc
