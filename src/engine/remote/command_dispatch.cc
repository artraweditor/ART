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

#include "command_dispatch.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <glib.h>
#include <glibmm.h>
#include <stdexcept>

namespace art { namespace engine { namespace remote {

Glib::ustring abspath(const Glib::ustring &f, const std::string &cwd)
{
    char *p = g_canonicalize_filename(f.c_str(), cwd.empty() ? nullptr : cwd.c_str());
    Glib::ustring ret(p);
    g_free(p);
    return ret;
}


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


Dispatcher::Dispatcher(std::string help_hint): help_hint_(std::move(help_hint))
{
}


void Dispatcher::add(const std::string &name, VerbFn fn)
{
    verbs_.push_back({name, std::move(fn)});
}


void Dispatcher::run(CommandHost &host, const ReplyPtr &reply,
                     const std::string &cwd, const Args &args) const
{
    if (args.empty()) {
        reply->fail("missing command (" + help_hint_ + ")");
        return;
    }
    Context c{host, reply, cwd};
    Args a(args.begin() + 1, args.end());
    for (auto &v : verbs_) {
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


namespace {

namespace pp = procparams;

[[noreturn]] void no_image() { throw std::runtime_error("no image is open in the editor"); }

// run fn(target) on the active image once it is ready
void with_target(Context &c, std::function<void(CommandTarget &)> fn)
{
    if (!c.host.current()) {
        no_image();
    }
    c.host.whenReady(c.reply, std::move(fn));
}


void cmd_get_params(Context &c, Args &a)
{
    auto reply = c.reply;
    with_target(c, [reply](CommandTarget &t) {
        Glib::ustring txt;
        if (!t.getParamsText(txt)) {
            throw std::runtime_error("cannot read the parameters");
        }
        reply->ok(txt);
    });
}


void cmd_get_info(Context &c, Args &a)
{
    auto reply = c.reply;
    with_target(c, [reply](CommandTarget &t) {
        Glib::ustring txt;
        if (!t.getInfoText(txt)) {
            throw std::runtime_error("cannot read the image information");
        }
        reply->ok(txt);
    });
}


void cmd_apply(Context &c, Args &a)
{
    const bool append = take_flag(a, "--append");
    if (a.size() != 1) {
        throw std::runtime_error("usage: apply [--append] PROFILE");
    }
    auto prof = abspath(a[0], c.cwd);
    if (!Glib::file_test(prof, Glib::FILE_TEST_IS_REGULAR)) {
        throw std::runtime_error("no such profile: " + prof);
    }
    if (!c.host.current()) {
        no_image();
    }
    pp::FilePartialProfile fp(nullptr, prof, append);
    {
        pp::ProcParams tmp;
        if (!fp.applyTo(tmp)) {
            throw std::runtime_error("cannot load the profile: " + prof);
        }
    }
    auto label = Glib::path_get_basename(prof);
    if (append) {
        label += "+";
    }
    auto reply = c.reply;
    c.host.whenReady(reply, [fp, label, reply](CommandTarget &t) {
        if (!t.applyProfile(fp, label)) {
            throw std::runtime_error("cannot apply the profile");
        }
        reply->ok();
    });
}


void cmd_preview(Context &c, Args &a)
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
    if (!c.host.current()) {
        no_image();
    }
    auto reply = c.reply;
    c.host.whenReady(reply, [=](CommandTarget &t) {
        t.renderPreview(out, scale, cx, cy, cw, ch,
                        [reply](bool ok, const Glib::ustring &msg) {
                            if (ok) {
                                reply->ok(msg);
                            } else {
                                reply->fail(msg);
                            }
                        });
    });
}


void cmd_get_spot_wb(Context &c, Args &a)
{
    // X Y W H, or X,Y,W,H as for preview --crop
    const char *usage = "usage: get-spot-wb X Y W H";
    if (a.size() == 1) {
        a = Glib::Regex::split_simple(",", a[0]);
    }
    int v[4];
    if (a.size() != 4) {
        throw std::runtime_error(usage);
    }
    for (int i = 0; i < 4; ++i) {
        if (!parse_int(a[i], v[i])) {
            throw std::runtime_error(usage);
        }
    }
    const int x = v[0], y = v[1], w = v[2], h = v[3];
    if (w <= 0 || h <= 0) {
        throw std::runtime_error("the rectangle must not be empty");
    }
    // the work (and the memory) is proportional to the area
    constexpr long long MAX_AREA = 1000000;
    if (static_cast<long long>(w) * h > MAX_AREA) {
        throw std::runtime_error("the rectangle is too large (at most 1000000 pixels)");
    }
    if (!c.host.current()) {
        no_image();
    }
    auto reply = c.reply;
    c.host.whenReady(reply, [=](CommandTarget &t) {
        int fw = 0, fh = 0;
        if (!t.getFullSize(fw, fh)) {
            throw std::runtime_error("cannot read the image size");
        }
        if (x < 0 || y < 0 || x + w > fw || y + h > fh) {
            throw std::runtime_error(Glib::ustring::compose(
                "rectangle out of bounds (full image is %1x%2)", fw, fh));
        }
        SpotWB wb;
        if (!t.getSpotWB(x, y, w, h, wb)) {
            throw std::runtime_error("cannot compute the white balance");
        }
        // the engine's multipliers have an arbitrary overall scale (it differs
        // between a measured and a computed white balance): report them
        // relative to the green channel
        if (wb.green > 0) {
            wb.red /= wb.green;
            wb.blue /= wb.green;
            wb.green = 1;
        }
        char buf[256];
        snprintf(buf, sizeof(buf),
                 "temperature: %.6g\ntint: %.6g\nred_multiplier: %.6g\n"
                 "green_multiplier: %.6g\nblue_multiplier: %.6g",
                 wb.temperature, wb.tint, wb.red, wb.green, wb.blue);
        reply->ok(buf);
    });
}


// ---- history and snapshots -------------------------------------------------

bool parse_count(Args &a, int &n)
{
    n = 1;
    if (a.size() > 1 || (a.size() == 1 && (!parse_int(a[0], n) || n < 1))) {
        return false;
    }
    return true;
}


Glib::ustring history_state(CommandTarget &t)
{
    auto v = t.getHistoryEntries();
    int i = t.getHistoryIndex();
    if (i < 0) {
        auto s = t.getSnapshotIndex();
        auto names = t.getSnapshotNames();
        return Glib::ustring::compose(
            "history: %1 entries, none selected; active snapshot: %2",
            v.size(), s >= 0 ? names[s] : Glib::ustring("(none)"));
    }
    return Glib::ustring::compose("history: entry %1 of %2 (%3) selected", i,
                                  v.size() - 1, v[i].text);
}


void cmd_get_history(Context &c, Args &a)
{
    auto reply = c.reply;
    with_target(c, [reply](CommandTarget &t) {
        auto v = t.getHistoryEntries();
        int sel = t.getHistoryIndex();
        for (size_t i = 0; i < v.size(); ++i) {
            reply->print(Glib::ustring::compose(
                "%1\t%2\t%3\t%4", i, int(i) == sel ? "*" : "-", v[i].text,
                v[i].value));
        }
        reply->ok();
    });
}


void undo_redo(Context &c, Args &a, int dir)
{
    int n;
    if (!parse_count(a, n)) {
        throw std::runtime_error(dir < 0 ? "usage: undo [N]" : "usage: redo [N]");
    }
    auto reply = c.reply;
    with_target(c, [reply, n, dir](CommandTarget &t) {
        int size = t.getHistoryEntries().size();
        int idx = t.getHistoryIndex();
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
        t.selectHistoryIndex(target);
        reply->ok(history_state(t));
    });
}


void cmd_history_goto(Context &c, Args &a)
{
    int n;
    if (a.size() != 1 || !parse_int(a[0], n)) {
        throw std::runtime_error("usage: history-goto INDEX");
    }
    auto reply = c.reply;
    with_target(c, [reply, n](CommandTarget &t) {
        if (!t.selectHistoryIndex(n)) {
            throw std::runtime_error(Glib::ustring::compose(
                "no such history entry (valid: 0..%1)",
                int(t.getHistoryEntries().size()) - 1));
        }
        reply->ok(history_state(t));
    });
}


void cmd_get_snapshots(Context &c, Args &a)
{
    auto reply = c.reply;
    with_target(c, [reply](CommandTarget &t) {
        auto names = t.getSnapshotNames();
        int sel = t.getSnapshotIndex();
        for (size_t i = 0; i < names.size(); ++i) {
            reply->print(Glib::ustring::compose("%1\t%2", int(i) == sel ? "*" : "-",
                                                names[i]));
        }
        reply->ok();
    });
}


void cmd_snapshot_activate(Context &c, Args &a)
{
    if (a.size() != 1) {
        throw std::runtime_error("usage: snapshot-activate NAME");
    }
    auto name = a[0];
    auto reply = c.reply;
    with_target(c, [reply, name](CommandTarget &t) {
        if (!t.activateSnapshot(name)) {
            throw std::runtime_error("no snapshot named \"" + name + "\"");
        }
        reply->ok();
    });
}


void cmd_snapshot_add(Context &c, Args &a)
{
    if (a.size() > 1) {
        throw std::runtime_error("usage: snapshot-add [NAME]");
    }
    Glib::ustring name = a.empty() ? Glib::ustring() : a[0];
    auto reply = c.reply;
    with_target(c, [reply, name](CommandTarget &t) {
        auto nm = name;
        if (nm.empty()) {
            auto names = t.getSnapshotNames();
            for (int i = 1;; ++i) {
                nm = Glib::ustring::compose("Snapshot %1", i);
                if (std::find(names.begin(), names.end(), nm) == names.end()) {
                    break;
                }
            }
        }
        pp::ProcParams params;
        if (!t.getCurrentParams(params)) {
            throw std::runtime_error("cannot read the current parameters");
        }
        Glib::ustring err;
        if (!t.addSnapshot(nm, params, err)) {
            throw std::runtime_error(err);
        }
        reply->ok(nm);
    });
}


void cmd_snapshot_remove(Context &c, Args &a)
{
    if (a.size() != 1) {
        throw std::runtime_error("usage: snapshot-remove NAME");
    }
    auto name = a[0];
    auto reply = c.reply;
    with_target(c, [reply, name](CommandTarget &t) {
        Glib::ustring err;
        if (!t.removeSnapshot(name, err)) {
            throw std::runtime_error(err);
        }
        reply->ok();
    });
}


void cmd_snapshot_overwrite(Context &c, Args &a)
{
    if (a.size() != 1) {
        throw std::runtime_error("usage: snapshot-overwrite NAME");
    }
    auto name = a[0];
    auto reply = c.reply;
    with_target(c, [reply, name](CommandTarget &t) {
        pp::ProcParams params;
        if (!t.getCurrentParams(params)) {
            throw std::runtime_error("cannot read the current parameters");
        }
        Glib::ustring err;
        if (!t.overwriteSnapshot(name, params, err)) {
            throw std::runtime_error(err);
        }
        reply->ok();
    });
}

} // namespace


void registerActiveImageVerbs(Dispatcher &d)
{
    d.add("apply", cmd_apply);
    d.add("get-params", cmd_get_params);
    d.add("get-info", cmd_get_info);
    d.add("get-history", cmd_get_history);
    d.add("undo", [](Context &c, Args &a) { undo_redo(c, a, -1); });
    d.add("redo", [](Context &c, Args &a) { undo_redo(c, a, +1); });
    d.add("history-goto", cmd_history_goto);
    d.add("get-snapshots", cmd_get_snapshots);
    d.add("snapshot-activate", cmd_snapshot_activate);
    d.add("snapshot-add", cmd_snapshot_add);
    d.add("snapshot-remove", cmd_snapshot_remove);
    d.add("snapshot-overwrite", cmd_snapshot_overwrite);
    d.add("preview", cmd_preview);
    d.add("get-spot-wb", cmd_get_spot_wb);
}

}}} // namespace art::engine::remote
