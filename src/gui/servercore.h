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

#include "../engine/remote/command_dispatch.h"
#include "editsession.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace art { namespace gui {

// The commands of the server mode on one active image, independent of how
// they arrive: the socket server (servermode.cc) and the in-process engine of
// libART both execute their commands through one of these.
//
// Not thread safe: one command at a time.
class ServerCore {
public:
    // `help_hint` completes the "missing command" error
    explicit ServerCore(const std::string &help_hint = "try: ART-cli --cmd help");
    ~ServerCore();

    // Execute the command `args` (args[0] is the verb); relative paths are
    // resolved against `cwd`. The output goes to `sink`; it is always
    // finished when this returns. Does not throw.
    void execute(const std::vector<std::string> &args, const std::string &cwd,
                 bool mark,
                 const std::shared_ptr<art::engine::remote::ReplySink> &sink);

    // Called when the `quit` command is executed. Without one, `quit` fails.
    void setQuitHandler(std::function<void()> fn) { on_quit_ = std::move(fn); }

    EditSession &session() { return session_; }

private:
    class Host;
    EditSession session_;
    std::unique_ptr<Host> host_;
    art::engine::remote::Dispatcher dispatcher_;
    std::function<void()> on_quit_;
};

}} // namespace art::gui
