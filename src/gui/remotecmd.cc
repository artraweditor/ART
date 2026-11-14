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

using Args = std::vector<Glib::ustring>;
using ClPtr = Glib::RefPtr<Gio::ApplicationCommandLine>;

// Completion of a command: `ok` and a message (printed to stdout on success,
// stderr on failure). The command line object is released afterwards.
class Reply {
public:
    explicit Reply(const ClPtr &cl): cl_(cl)
    {
        // programmatic clients (the MCP server) set ART_CMD_MARK=1: every
        // output line is then prefixed with \x01, so that it can be told
        // apart from unrelated messages printed by the client process
        mark_ = atoi(cl->getenv("ART_CMD_MARK").c_str()) != 0;
    }

    void ok(const Glib::ustring &msg = "")
    {
        if (!msg.empty()) {
            print(msg);
        }
        cl_->set_exit_status(0);
        cl_.reset();
    }

    void fail(const Glib::ustring &msg)
    {
        cl_->printerr("ART: error: " + msg + "\n");
        cl_->set_exit_status(1);
        cl_.reset();
    }

    void print(const Glib::ustring &msg)
    {
        if (!mark_) {
            cl_->print(msg + "\n");
            return;
        }
        Glib::ustring out;
        size_t start = 0;
        while (start <= msg.size()) {
            auto end = msg.find('\n', start);
            if (end == Glib::ustring::npos) {
                end = msg.size();
            }
            out += "\x01" + msg.substr(start, end - start) + "\n";
            start = end + 1;
        }
        cl_->print(out);
    }

private:
    ClPtr cl_;
    bool mark_;
};

using ReplyPtr = std::shared_ptr<Reply>;

Glib::ustring abspath(const Glib::ustring &f, const std::string &cwd)
{
    char *p = g_canonicalize_filename(f.c_str(), cwd.empty() ? nullptr : cwd.c_str());
    Glib::ustring ret(p);
    g_free(p);
    return ret;
}

// removes the option `name` from args; true if present
bool take_flag(Args &args, const Glib::ustring &name)
{
    for (auto it = args.begin(); it != args.end(); ++it) {
        if (*it == name) {
            args.erase(it);
            return true;
        }
    }
    return false;
}

// removes the option `name <value>` from args; false if not present, throws
// if the value is missing
bool take_option(Args &args, const Glib::ustring &name, Glib::ustring &value)
{
    for (auto it = args.begin(); it != args.end(); ++it) {
        if (*it == name) {
            auto n = std::next(it);
            if (n == args.end()) {
                throw std::runtime_error("missing value for " + name);
            }
            value = *n;
            args.erase(it, n + 1);
            return true;
        }
    }
    return false;
}

struct Ctx {
    RTWindow *win;
    ReplyPtr reply;
    std::string cwd;
};

bool parse_int(const Glib::ustring &s, int &out)
{
    char *end = nullptr;
    long v = strtol(s.c_str(), &end, 10);
    if (s.empty() || *end) {
        return false;
    }
    out = int(v);
    return true;
}

bool parse_double(const Glib::ustring &s, double &out)
{
    char *end = nullptr;
    out = g_ascii_strtod(s.c_str(), &end);
    return !s.empty() && !*end;
}

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

EditorPanel *current_editor(Ctx &c)
{
    auto v = c.win->getEditorPanels();
    return v.empty() ? nullptr : v[0];
}

// run fn(editor) as soon as the visible editor has finished loading/processing
// (waits up to 60 s); exceptions thrown by fn are reported to the client
void when_ready(Ctx &c, std::function<void(EditorPanel *)> fn)
{
    auto reply = c.reply;
    auto win = c.win;
    const auto attempt = [reply, win, fn]() -> bool {
        auto v = win->getEditorPanels();
        if (v.empty() || !v[0]->isReadyForRemote()) {
            return false;
        }
        try {
            fn(v[0]);
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
    c.reply->ok(c.win->fpanel->fileCatalog->lastSelectedDir());
}

void cmd_get_params(Ctx &c, Args &a)
{
    if (!current_editor(c)) {
        throw std::runtime_error("no image is open in the editor");
    }
    auto reply = c.reply;
    when_ready(c, [reply](EditorPanel *ep) {
        Glib::ustring txt;
        if (!ep->getParamsText(txt)) {
            throw std::runtime_error("cannot read the parameters");
        }
        reply->ok(txt);
    });
}

void cmd_get_info(Ctx &c, Args &a)
{
    if (!current_editor(c)) {
        throw std::runtime_error("no image is open in the editor");
    }
    auto reply = c.reply;
    when_ready(c, [reply](EditorPanel *ep) {
        Glib::ustring txt;
        if (!ep->getInfoText(txt)) {
            throw std::runtime_error("cannot read the image information");
        }
        reply->ok(txt);
    });
}

void cmd_get_files(Ctx &c, Args &a)
{
    for (auto ep : c.win->getEditorPanels()) {
        c.reply->print(ep->getFileName());
    }
    c.reply->ok();
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

void cmd_apply(Ctx &c, Args &a)
{
    const bool append = take_flag(a, "--append");
    if (a.size() != 1) {
        throw std::runtime_error("usage: apply [--append] PROFILE");
    }
    auto prof = abspath(a[0], c.cwd);
    if (!Glib::file_test(prof, Glib::FILE_TEST_IS_REGULAR)) {
        throw std::runtime_error("no such profile: " + prof);
    }
    if (!current_editor(c)) {
        throw std::runtime_error("no image is open in the editor");
    }
    art::engine::procparams::FilePartialProfile pp(nullptr, prof, append);
    {
        art::engine::procparams::ProcParams tmp;
        if (!pp.applyTo(tmp)) {
            throw std::runtime_error("cannot load the profile: " + prof);
        }
    }
    auto label = Glib::path_get_basename(prof);
    if (append) {
        label += "+";
    }
    auto reply = c.reply;
    when_ready(c, [pp, label, reply](EditorPanel *ep) {
        if (!ep->applyProfile(pp, label)) {
            throw std::runtime_error("cannot apply the profile");
        }
        reply->ok();
    });
}

void cmd_preview(Ctx &c, Args &a)
{
    Glib::ustring sv, cv;
    double scale = 1.0;
    int cx = 0, cy = 0, cw = 0, ch = 0;
    if (take_option(a, "--scale", sv) && (!parse_double(sv, scale) || scale <= 0)) {
        throw std::runtime_error("invalid --scale: " + sv);
    }
    if (take_option(a, "--crop", cv)) {
        std::vector<Glib::ustring> parts = Glib::Regex::split_simple(",", cv);
        if (parts.size() != 4 || !parse_int(parts[0], cx) ||
            !parse_int(parts[1], cy) || !parse_int(parts[2], cw) ||
            !parse_int(parts[3], ch) || cw <= 0 || ch <= 0) {
            throw std::runtime_error("invalid --crop (expected X,Y,W,H): " + cv);
        }
    }
    if (a.size() != 1) {
        throw std::runtime_error("usage: preview OUTFILE [--scale F] [--crop X,Y,W,H]");
    }
    auto out = abspath(a[0], c.cwd);
    if (!current_editor(c)) {
        throw std::runtime_error("no image is open in the editor");
    }
    auto reply = c.reply;
    when_ready(c, [=](EditorPanel *ep) {
        ep->renderPreview(out, scale, cx, cy, cw, ch,
                          [reply](bool ok, const Glib::ustring &msg) {
                              if (ok) {
                                  reply->ok(msg);
                              } else {
                                  reply->fail(msg);
                              }
                          });
    });
}


// ---- history and snapshots -------------------------------------------------

// run fn(history, editor) on the visible editor once it is ready
void with_history(Ctx &c, std::function<void(History *, EditorPanel *)> fn)
{
    if (!current_editor(c)) {
        throw std::runtime_error("no image is open in the editor");
    }
    when_ready(c, [fn](EditorPanel *ep) { fn(ep->getHistory(), ep); });
}

bool parse_count(Args &a, int &n)
{
    n = 1;
    if (a.size() > 1 || (a.size() == 1 && (!parse_int(a[0], n) || n < 1))) {
        return false;
    }
    return true;
}

Glib::ustring history_state(History *h)
{
    auto v = h->getHistoryEntries();
    int i = h->getHistoryIndex();
    if (i < 0) {
        auto s = h->getSnapshotIndex();
        auto names = h->getSnapshotNames();
        return Glib::ustring::compose(
            "history: %1 entries, none selected; active snapshot: %2",
            v.size(), s >= 0 ? names[s] : Glib::ustring("(none)"));
    }
    return Glib::ustring::compose("history: entry %1 of %2 (%3) selected", i,
                                  v.size() - 1, v[i].text);
}

void cmd_get_history(Ctx &c, Args &a)
{
    auto reply = c.reply;
    with_history(c, [reply](History *h, EditorPanel *) {
        auto v = h->getHistoryEntries();
        int sel = h->getHistoryIndex();
        for (size_t i = 0; i < v.size(); ++i) {
            reply->print(Glib::ustring::compose(
                "%1\t%2\t%3\t%4", i, int(i) == sel ? "*" : "-", v[i].text,
                v[i].value));
        }
        reply->ok();
    });
}

void undo_redo(Ctx &c, Args &a, int dir)
{
    int n;
    if (!parse_count(a, n)) {
        throw std::runtime_error(dir < 0 ? "usage: undo [N]" : "usage: redo [N]");
    }
    auto reply = c.reply;
    with_history(c, [reply, n, dir](History *h, EditorPanel *) {
        int size = h->getHistoryEntries().size();
        int idx = h->getHistoryIndex();
        if (idx < 0) {
            if (dir > 0) {
                throw std::runtime_error(
                    "no history entry is selected (a snapshot is active): use "
                    "history-goto");
            }
            idx = size - 1; // as the Undo button: from the latest entry
        }
        int target = idx + dir * n;
        if (target < 0 || target >= size) {
            throw std::runtime_error(dir < 0 ? "nothing to undo"
                                             : "nothing to redo");
        }
        h->selectHistoryIndex(target);
        reply->ok(history_state(h));
    });
}

void cmd_undo(Ctx &c, Args &a) { undo_redo(c, a, -1); }
void cmd_redo(Ctx &c, Args &a) { undo_redo(c, a, +1); }

void cmd_history_goto(Ctx &c, Args &a)
{
    int n;
    if (a.size() != 1 || !parse_int(a[0], n)) {
        throw std::runtime_error("usage: history-goto INDEX");
    }
    auto reply = c.reply;
    with_history(c, [reply, n](History *h, EditorPanel *) {
        if (!h->selectHistoryIndex(n)) {
            throw std::runtime_error(Glib::ustring::compose(
                "no such history entry (valid: 0..%1)",
                int(h->getHistoryEntries().size()) - 1));
        }
        reply->ok(history_state(h));
    });
}

void cmd_get_snapshots(Ctx &c, Args &a)
{
    auto reply = c.reply;
    with_history(c, [reply](History *h, EditorPanel *) {
        auto names = h->getSnapshotNames();
        int sel = h->getSnapshotIndex();
        for (size_t i = 0; i < names.size(); ++i) {
            reply->print(Glib::ustring::compose("%1\t%2", int(i) == sel ? "*" : "-",
                                                names[i]));
        }
        reply->ok();
    });
}

void cmd_snapshot_activate(Ctx &c, Args &a)
{
    if (a.size() != 1) {
        throw std::runtime_error("usage: snapshot-activate NAME");
    }
    auto name = a[0];
    auto reply = c.reply;
    with_history(c, [reply, name](History *h, EditorPanel *) {
        if (!h->activateSnapshot(name)) {
            throw std::runtime_error("no snapshot named \"" + name + "\"");
        }
        reply->ok();
    });
}

void cmd_snapshot_add(Ctx &c, Args &a)
{
    if (a.size() > 1) {
        throw std::runtime_error("usage: snapshot-add [NAME]");
    }
    Glib::ustring name = a.empty() ? Glib::ustring() : a[0];
    auto reply = c.reply;
    with_history(c, [reply, name](History *h, EditorPanel *ep) {
        auto nm = name;
        if (nm.empty()) {
            auto names = h->getSnapshotNames();
            for (int i = 1;; ++i) {
                nm = Glib::ustring::compose("Snapshot %1", i);
                if (std::find(names.begin(), names.end(), nm) == names.end()) {
                    break;
                }
            }
        }
        art::engine::procparams::ProcParams pp;
        if (!ep->getCurrentParams(pp)) {
            throw std::runtime_error("cannot read the current parameters");
        }
        Glib::ustring err;
        if (!h->addSnapshot(nm, pp, err)) {
            throw std::runtime_error(err);
        }
        reply->ok(nm);
    });
}

void cmd_snapshot_remove(Ctx &c, Args &a)
{
    if (a.size() != 1) {
        throw std::runtime_error("usage: snapshot-remove NAME");
    }
    auto name = a[0];
    auto reply = c.reply;
    with_history(c, [reply, name](History *h, EditorPanel *) {
        Glib::ustring err;
        if (!h->removeSnapshot(name, err)) {
            throw std::runtime_error(err);
        }
        reply->ok();
    });
}

void cmd_snapshot_overwrite(Ctx &c, Args &a)
{
    if (a.size() != 1) {
        throw std::runtime_error("usage: snapshot-overwrite NAME");
    }
    auto name = a[0];
    auto reply = c.reply;
    with_history(c, [reply, name](History *h, EditorPanel *ep) {
        art::engine::procparams::ProcParams pp;
        if (!ep->getCurrentParams(pp)) {
            throw std::runtime_error("cannot read the current parameters");
        }
        Glib::ustring err;
        if (!h->overwriteSnapshot(name, pp, err)) {
            throw std::runtime_error(err);
        }
        reply->ok();
    });
}

struct Verb {
    const char *name;
    void (*fn)(Ctx &, Args &);
};

const Verb verbs[] = {
    {"open-dir", cmd_open_dir},
    {"get-dir", cmd_get_dir},
    {"rank", [](Ctx &c, Args &a) { rate(c, a, true); }},
    {"label", [](Ctx &c, Args &a) { rate(c, a, false); }},
    {"session", cmd_session},
    {"open", cmd_open},
    {"apply", cmd_apply},
    {"get-files", cmd_get_files},
    {"get-params", cmd_get_params},
    {"get-info", cmd_get_info},
    {"get-history", cmd_get_history},
    {"undo", cmd_undo},
    {"redo", cmd_redo},
    {"history-goto", cmd_history_goto},
    {"get-snapshots", cmd_get_snapshots},
    {"snapshot-activate", cmd_snapshot_activate},
    {"snapshot-add", cmd_snapshot_add},
    {"snapshot-remove", cmd_snapshot_remove},
    {"snapshot-overwrite", cmd_snapshot_overwrite},
    {"preview", cmd_preview},
};

} // namespace


void run(RTWindow *win, const ClPtr &cl, const Args &args)
{
    auto reply = std::make_shared<Reply>(cl);
    if (args.empty()) {
        reply->fail("missing command (try: ART --cmd help)");
        return;
    }
    Ctx c{win, reply, cl->get_cwd()};
    Args a(args.begin() + 1, args.end());
    for (auto &v : verbs) {
        if (args[0] == v.name) {
            try {
                v.fn(c, a);
            } catch (std::exception &e) {
                reply->fail(e.what());
            } catch (Glib::Error &e) {
                reply->fail(e.what());
            }
            return;
        }
    }
    reply->fail("unknown command: " + args[0]);
}


} } } // namespace art::gui::remotecmd
