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

#include "remote_reply.h"

namespace art { namespace engine { namespace remote {

Reply::Reply(std::shared_ptr<ReplySink> sink, bool mark):
    sink_(std::move(sink)), mark_(mark), done_(false)
{
}


void Reply::ok(const Glib::ustring &msg)
{
    if (done_) {
        return;
    }
    if (!msg.empty()) {
        print(msg);
    }
    done_ = true;
    sink_->finish(0);
}


void Reply::fail(const Glib::ustring &msg)
{
    if (done_) {
        return;
    }
    done_ = true;
    sink_->err("ART: error: " + msg.raw() + "\n");
    sink_->finish(1);
}


void Reply::print(const Glib::ustring &msg)
{
    if (!mark_) {
        sink_->out(msg.raw() + "\n");
        return;
    }
    const std::string &m = msg.raw();
    std::string out;
    size_t start = 0;
    while (start <= m.size()) {
        auto end = m.find('\n', start);
        if (end == std::string::npos) {
            end = m.size();
        }
        out += "\x01" + m.substr(start, end - start) + "\n";
        start = end + 1;
    }
    sink_->out(out);
}

}}} // namespace art::engine::remote
