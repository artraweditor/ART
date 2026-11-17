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


#include "remotecmd.h"
#include "../engine/procparams.h"
#include "../engine/imgiomanager.h"
#include "../engine/remote/command_dispatch.h"
#include "../utils/pathutils.h"
#include "cachemanager.h"
#include "editorpanel.h"
#include "filebrowser.h"
#include "filecatalog.h"
#include "filepanel.h"
#include "history.h"
#include "options.h"
#include "rtwindow.h"
#include "session.h"
#include "thumbnail.h"
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <glib.h>
#include <iostream>
#include <memory>

namespace art { namespace gui { namespace remotecmd {

namespace {

namespace remote = art::engine::remote;

using Args = remote::Args;
using ReplyPtr = remote::ReplyPtr;
using ClPtr = Glib::RefPtr<Gio::ApplicationCommandLine>;
using remote::abspath;
using remote::parse_int;
using remote::take_flag;

// the output of a command goes to the client of the GApplication command
// line, which is released (ending the client process) when the command is
// complete
class ClSink: public remote::ReplySink {
public:
    explicit ClSink(const ClPtr &cl): cl_(cl) {}

    void out(const std::string &s) override { if (cl_) cl_->print(s); }
    void err(const std::string &s) override { if (cl_) cl_->printerr(s); }
    void finish(int status) override
    {
        if (cl_) {
            cl_->set_exit_status(status);
            cl_.reset();
        }
    }

private:
    ClPtr cl_;
};

// what the GUI-only verbs work with
struct Ctx {
    RTWindow *win;
    ReplyPtr reply;
    std::string cwd;
};

// run `check` periodically in the GUI thread until it returns true (done) or
// the timeout (in seconds) expires, in which case `on_timeout` is called
struct Poll {
    std::function<bool()> check;
    std::function<void()> on_timeout;
    int ticks;
    int max_ticks;
};

void poll_until(std::function<bool()> check, std::function<void()> on_timeout,
                int timeout_s)
{
    auto p = new Poll{std::move(check), std::move(on_timeout), 0, timeout_s * 10};
    gdk_threads_add_timeout(
        100,
        [](gpointer data) -> gboolean {
            auto p = static_cast<Poll *>(data);
            if (p->check()) {
                delete p;
                return FALSE;
            }
            if (++p->ticks >= p->max_ticks) {
                p->on_timeout();
                delete p;
                return FALSE;
            }
            return TRUE;
        },
        p);
}

// the active image: the visible editor tab
class EditorTarget: public remote::CommandTarget {
public:
    explicit EditorTarget(EditorPanel *ep): ep_(ep) {}

    bool getParamsText(Glib::ustring &out) override { return ep_->getParamsText(out); }
    bool getInfoText(Glib::ustring &out) override { return ep_->getInfoText(out); }
    bool getCurrentParams(art::engine::procparams::ProcParams &pp) override
    {
        return ep_->getCurrentParams(pp);
    }
    bool getMetadata(int makernotes, bool edits, remote::json::Value &out,
                     Glib::ustring &err) override
    {
        return ep_->getMetadata(makernotes, edits, out, err);
    }
    bool applyProfile(const art::engine::procparams::PartialProfile &pp,
                      const Glib::ustring &descr) override
    {
        return ep_->applyProfile(pp, descr);
    }
    std::unique_ptr<art::engine::procparams::PartialProfile>
    getDefaultProfile() override
    {
        return ep_->getDefaultProfile();
    }
    // the GUI does not write the file: the image is added to the batch queue
    // (as with the Save as dialog), which processes it and names the file
    void exportImage(const remote::ExportRequest &req,
                     remote::ImageDone done) override
    {
        if (req.overwrite) {
            done(false,
                 "--overwrite is not available in the GUI: the batch queue "
                 "decides what happens to an existing file (see the "
                 "preferences)",
                 0, 0);
            return;
        }
        SaveFormat sf;
        sf.format = req.format;
        sf.jpegQuality = req.quality;
        sf.jpegSubSamp = req.subsampling;
        sf.pngBits = req.format == "png" ? req.bits : 8;
        sf.tiffBits = req.format == "tif" ? req.bits : 16;
        sf.tiffFloat = req.isFloat;
        sf.tiffUncompressed = req.uncompressed;
        sf.saveParams = false;
        Glib::ustring err;
        art::engine::procparams::FilePartialProfile over(nullptr, req.profile,
                                                         false);
        if (!ep_->queueExport(req.outfile, sf, req.fast, err, req.fastSize,
                              req.profile.empty() ? nullptr : &over)) {
            done(false, err, 0, 0);
            return;
        }
        // the queue gives the file the extension of the format
        std::string ext = req.format;
        for (auto &p :
             art::engine::ImageIOManager::getInstance()->getSaveFormats()) {
            if (p.first == req.format) {
                ext = p.second.extension;
            }
        }
        done(true, removeExtension(req.outfile) + "." + ext, -1, -1);
    }
    void renderPreview(const Glib::ustring &outfile, double scale, int cx,
                       int cy, int cw, int ch,
                       std::function<void(bool, const Glib::ustring &, int, int)> done) override
    {
        ep_->renderPreview(outfile, scale, cx, cy, cw, ch, std::move(done));
    }

    bool getFullSize(int &w, int &h) override { return ep_->getFullSize(w, h); }
    bool getSpotWB(int x, int y, int w, int h, remote::SpotWB &out) override
    {
        art::engine::ColorTemp ct;
        if (!ep_->getSpotWB(x, y, w, h, ct)) {
            return false;
        }
        out.temperature = ct.getTemp();
        out.tint = ct.getGreen();
        ct.getMultipliers(out.red, out.green, out.blue);
        return true;
    }

    std::vector<remote::HistoryEntryInfo> getHistoryEntries() override
    {
        std::vector<remote::HistoryEntryInfo> ret;
        for (auto &e : ep_->getHistory()->getHistoryEntries()) {
            ret.push_back({e.text, e.value});
        }
        return ret;
    }
    int getHistoryIndex() override { return ep_->getHistory()->getHistoryIndex(); }
    bool selectHistoryIndex(int index) override
    {
        return ep_->getHistory()->selectHistoryIndex(index);
    }
    std::vector<Glib::ustring> getSnapshotNames() override
    {
        return ep_->getHistory()->getSnapshotNames();
    }
    int getSnapshotIndex() override { return ep_->getHistory()->getSnapshotIndex(); }
    bool activateSnapshot(const Glib::ustring &name) override
    {
        return ep_->getHistory()->activateSnapshot(name);
    }
    bool addSnapshot(const Glib::ustring &name,
                     const art::engine::procparams::ProcParams &params,
                     Glib::ustring &err) override
    {
        return ep_->getHistory()->addSnapshot(name, params, err);
    }
    bool removeSnapshot(const Glib::ustring &name, Glib::ustring &err) override
    {
        return ep_->getHistory()->removeSnapshot(name, err);
    }
    bool overwriteSnapshot(const Glib::ustring &name,
                           const art::engine::procparams::ProcParams &params,
                           Glib::ustring &err) override
    {
        return ep_->getHistory()->overwriteSnapshot(name, params, err);
    }

private:
    EditorPanel *ep_;
};


class GuiHost: public remote::CommandHost {
public:
    explicit GuiHost(RTWindow *w): win(w) {}

    remote::CommandTarget *current() override
    {
        auto v = win->getEditorPanels();
        if (v.empty()) {
            return nullptr;
        }
        target_.reset(new EditorTarget(v[0]));
        return target_.get();
    }

    // run fn(editor) as soon as the visible editor has finished
    // loading/processing (waits up to 60 s); exceptions thrown by fn are
    // reported to the client
    void whenReady(const ReplyPtr &reply,
                   std::function<void(remote::CommandTarget &)> fn) override
    {
        auto win = this->win;
        const auto attempt = [reply, win, fn]() -> bool {
            auto v = win->getEditorPanels();
            if (v.empty() || !v[0]->isReadyForRemote()) {
                return false;
            }
            try {
                EditorTarget t(v[0]);
                fn(t);
            } catch (std::exception &e) {
                reply->fail(e.what());
            } catch (Glib::Error &e) {
                reply->fail(e.what());
            }
            return true;
        };
        if (!attempt()) {
            poll_until(attempt,
                       [reply]() { reply->fail("timeout waiting for the editor"); },
                       60);
        }
    }

    RTWindow *win;

private:
    std::unique_ptr<EditorTarget> target_;
};

void show_main(Ctx &c)
{
    c.win->SetMainCurrent();
    c.win->present();
}

// ---------------------------------------------------------------------------

void cmd_open_dir(Ctx &c, Args &a)
{
    if (a.size() != 1) {
        throw std::runtime_error("usage: open-dir DIR");
    }
    auto dir = abspath(a[0], c.cwd);
    if (!Glib::file_test(dir, Glib::FILE_TEST_IS_DIR)) {
        throw std::runtime_error("not a directory: " + dir);
    }
    c.win->fpanel->dirBrowser->open(dir, "");
    show_main(c);
    c.reply->ok();
}

void cmd_get_dir(Ctx &c, Args &a)
{
    remote::json::Value r = remote::json::Value::object();
    r.set("directory", c.win->fpanel->fileCatalog->lastSelectedDir());
    c.reply->ok(r);
}

void cmd_get_files(Ctx &c, Args &a)
{
    remote::json::Value r = remote::json::Value::array();
    for (auto ep : c.win->getEditorPanels()) {
        r.push(ep->getFileName());
    }
    c.reply->ok(r);
}

void rate(Ctx &c, Args &a, bool is_rank)
{
    int n = 0;
    if (a.size() < 2 || !parse_int(a[0], n) || n < 0 || n > 5) {
        throw std::runtime_error(
            Glib::ustring::compose("usage: %1 0..5 FILE...",
                                   is_rank ? "rank" : "label"));
    }
    std::vector<Glib::ustring> files;
    for (size_t i = 1; i < a.size(); ++i) {
        auto f = abspath(a[i], c.cwd);
        if (!Glib::file_test(f, Glib::FILE_TEST_IS_REGULAR)) {
            throw std::runtime_error("no such file: " + f);
        }
        files.push_back(f);
    }
    std::vector<Glib::ustring> rest;
    auto fb = c.win->fpanel->fileCatalog->fileBrowser;
    if (is_rank) {
        fb->setRankForFiles(files, n, rest);
    } else {
        fb->setColorLabelForFiles(files, n, rest);
    }
    // files not shown in the browser: go through the thumbnail cache
    const bool need_pp = options.thumbnail_rating_mode ==
                         Options::ThumbnailRatingMode::PROCPARAMS;
    for (auto &f : rest) {
        Thumbnail *t = cacheMgr->getEntry(f);
        if (!t) {
            throw std::runtime_error("cannot read the image: " + f);
        }
        if (need_pp) {
            delete t->createProcParamsForUpdate(false, false, true);
        }
        if (is_rank) {
            t->setRank(n);
        } else {
            t->setColorLabel(n);
        }
        t->updateCache();
        t->decreaseRef();
    }
    c.reply->ok();
}

void cmd_session(Ctx &c, Args &a)
{
    const bool show = take_flag(a, "--show");
    if (a.empty()) {
        throw std::runtime_error("usage: session add|remove|clear|load|save|open|show ...");
    }
    const auto op = a[0];
    std::vector<Glib::ustring> files;
    for (size_t i = 1; i < a.size(); ++i) {
        files.push_back(abspath(a[i], c.cwd));
    }
    bool do_show = show;
    Glib::ustring to_open;
    if (op == "add") {
        session::add(files);
    } else if (op == "remove") {
        session::remove(files);
    } else if (op == "clear") {
        session::clear();
    } else if (op == "load") {
        session::load(files.empty() ? session::filename() + ".last" : files[0]);
    } else if (op == "save") {
        if (files.size() != 1) {
            throw std::runtime_error("usage: session save FILE");
        }
        session::save(files[0]);
    } else if (op == "show") {
        do_show = true;
    } else if (op == "open") {
        if (files.empty()) {
            throw std::runtime_error("usage: session open FILE...");
        }
        session::add(files);
        to_open = files[0];
        do_show = true;
    } else {
        throw std::runtime_error("unknown session operation: " + op);
    }

    if (do_show) {
        auto fc = c.win->fpanel->fileCatalog;
        if (!session::check(fc->lastSelectedDir())) {
            c.win->fpanel->dirBrowser->open(session::path(), "");
        }
        show_main(c);
        if (!to_open.empty()) {
            // wait until the session view is populated, then open the file
            auto reply = c.reply;
            auto win = c.win;
            poll_until(
                [to_open, reply, win]() {
                    for (auto ep : win->getEditorPanels()) {
                        if (ep->getFileName() == to_open &&
                            ep->isReadyForRemote()) {
                            reply->ok();
                            return true;
                        }
                    }
                    return false;
                },
                [reply]() { reply->fail("timeout while opening the file"); }, 60);
            fc->openFile(to_open);
            return;
        }
    }
    c.reply->ok();
}

void cmd_open(Ctx &c, Args &a)
{
    if (a.size() != 1) {
        throw std::runtime_error("usage: open FILE");
    }
    auto f = abspath(a[0], c.cwd);
    if (!Glib::file_test(f, Glib::FILE_TEST_IS_REGULAR)) {
        throw std::runtime_error("no such file: " + f);
    }
    auto reply = c.reply;
    auto win = c.win;
    // dirBrowser->open() takes the file name relative to the directory
    win->fpanel->dirBrowser->open(Glib::path_get_dirname(f),
                                  Glib::path_get_basename(f));
    poll_until(
        [reply, win, f]() {
            auto v = win->getEditorPanels();
            if (!v.empty() && v[0]->getFileName() == f &&
                v[0]->isReadyForRemote()) {
                reply->ok();
                return true;
            }
            return false;
        },
        [reply]() { reply->fail("timeout while opening the file"); }, 60);
}

// GUI-only verbs get the window; the shared ones are registered below
remote::VerbFn gui_verb(void (*fn)(Ctx &, Args &))
{
    return [fn](remote::Context &c, Args &a) {
        Ctx gc{static_cast<GuiHost &>(c.host).win, c.reply, c.cwd};
        fn(gc, a);
    };
}

const remote::Dispatcher &dispatcher()
{
    static const remote::Dispatcher d = []() {
        remote::Dispatcher d;
        d.add("open-dir", gui_verb(cmd_open_dir));
        d.add("get-dir", gui_verb(cmd_get_dir));
        d.add("rank", gui_verb([](Ctx &c, Args &a) { rate(c, a, true); }));
        d.add("label", gui_verb([](Ctx &c, Args &a) { rate(c, a, false); }));
        d.add("session", gui_verb(cmd_session));
        d.add("open", gui_verb(cmd_open));
        d.add("get-files", gui_verb(cmd_get_files));
        remote::registerActiveImageVerbs(d);
        return d;
    }();
    return d;
}

} // namespace


void run(RTWindow *win, const ClPtr &cl, const Args &args)
{
    auto reply = std::make_shared<remote::Reply>(std::make_shared<ClSink>(cl));
    GuiHost host(win);
    dispatcher().run(host, reply, cl->get_cwd(), args);
}


} } } // namespace art::gui::remotecmd
