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

std::string failureDocument(const Glib::ustring &msg)
{
    json::Value v = json::Value::object();
    v.set("ok", false).set("error", msg);
    return v.dump() + "\n";
}


Reply::Reply(std::shared_ptr<ReplySink> sink):
    sink_(std::move(sink)), done_(false)
{
}


void Reply::ok()
{
    if (done_) {
        return;
    }
    done_ = true;
    sink_->out("{\"ok\":true}\n");
    sink_->finish(0);
}


void Reply::ok(const json::Value &result)
{
    if (done_) {
        return;
    }
    done_ = true;
    json::Value v = json::Value::object();
    v.set("ok", true).set("result", result);
    sink_->out(v.dump() + "\n");
    sink_->finish(0);
}


void Reply::fail(const Glib::ustring &msg)
{
    if (done_) {
        return;
    }
    done_ = true;
    sink_->out(failureDocument(msg));
    sink_->err("ART: error: " + msg.raw() + "\n");
    sink_->finish(1);
}

}}} // namespace art::engine::remote
