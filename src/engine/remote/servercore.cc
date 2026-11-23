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

#include "servercore.h"
#include <glibmm.h>
#include <stdexcept>

namespace art { namespace engine {


class ServerCore::Host: public remote::CommandHost {
public:
    Host(EditSession &s, const std::string &m): session(s), mode_(m) {}

    std::string mode() const override { return mode_; }

    remote::CommandTarget *current() override
    {
        return session.isOpen() ? &session : nullptr;
    }

    Glib::ustring paramFileFor(const Glib::ustring &imagefile) override
    {
        return session.paramFileFor(imagefile);
    }

    // commands run one at a time, so waiting here blocks the other callers
    // until the image is ready: they queue up
    void whenReady(const remote::ReplyPtr &reply,
                   std::function<void(remote::CommandTarget &)> fn) override
    {
        if (!session.waitReady(60000)) {
            reply->fail("timeout waiting for the editor");
            return;
        }
        try {
            fn(session);
        } catch (std::exception &e) {
            reply->fail(e.what());
        } catch (Glib::Error &e) {
            reply->fail(e.what());
        }
    }

    EditSession &session;

private:
    std::string mode_;
};


ServerCore::ServerCore(EditSession::Hooks hooks, const std::string &mode,
                       const std::string &help_hint):
    session_(std::move(hooks)), host_(new Host(session_, mode)),
    dispatcher_(help_hint)
{
    using remote::Args;
    using remote::Context;

    remote::registerActiveImageVerbs(dispatcher_);

    EditSession &session = session_;
    dispatcher_.add("open", [&session](Context &c, Args &a) {
        if (a.size() != 1) {
            throw std::runtime_error("usage: open FILE");
        }
        auto f = remote::abspath(a[0], c.cwd);
        if (!Glib::file_test(f, Glib::FILE_TEST_IS_REGULAR)) {
            throw std::runtime_error("no such file: " + f);
        }
        session.open(f);
        if (!session.waitReady(60000)) {
            c.reply->fail("timeout while opening the file");
            return;
        }
        c.reply->ok();
    });
    dispatcher_.add("get-files", [&session](Context &c, Args &a) {
        remote::json::Value r = remote::json::Value::array();
        if (session.isOpen()) {
            r.push(session.fileName());
        }
        c.reply->ok(r);
    });
    dispatcher_.add("save", [&session](Context &c, Args &a) {
        Glib::ustring err;
        if (!session.save(err)) {
            throw std::runtime_error(err);
        }
        remote::json::Value r = remote::json::Value::object();
        r.set("file", session.sidecarFile());
        c.reply->ok(r);
    });
    dispatcher_.add("ping", [](Context &c, Args &a) { c.reply->ok("pong"); });
    dispatcher_.add("quit", [this](Context &c, Args &a) {
        if (!on_quit_) {
            throw std::runtime_error("quit is not available here");
        }
        on_quit_();
        c.reply->ok();
    });
}


ServerCore::~ServerCore() {}


void ServerCore::execute(const std::vector<std::string> &args,
                         const std::string &cwd,
                         const std::shared_ptr<remote::ReplySink> &sink)
{
    auto reply = std::make_shared<remote::Reply>(sink);
    try {
        remote::Args a(args.begin(), args.end());
        dispatcher_.run(*host_, reply, cwd, a);
    } catch (std::exception &e) {
        reply->fail(e.what());
    } catch (...) {
        reply->fail("unexpected error");
    }
    if (!reply->done()) {
        // every command must complete before it returns
        reply->fail("internal error: the command did not complete");
    }
}

}} // namespace art::engine
