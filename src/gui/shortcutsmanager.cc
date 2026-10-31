/** -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright (c) 2026 Alberto Griggio <alberto.griggio@gmail.com>
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

#include "shortcutsmanager.h"
#include "guiutils.h"
#include "../utils/multilangmgr.h"
#include "../utils/pathutils.h"
#include "options.h"
#include "rtwindow.h"
#include "shortcutdefs.h"
#include "toolpanel.h"

#include <cstdio>
#include <cstdlib>
#include <glib/gstdio.h>
#include <glibmm.h>
#include <map>
#include <set>
#include <vector>

namespace art { namespace gui {

namespace {

const char *const USER_FILE = "shortcuts.json";
const char *const REFERENCE_FILE = "shortcuts_reference.json";

void writeIfDifferent(const std::string &path, const std::string &text)
{
    if (FILE *f = g_fopen(path.c_str(), "rb")) {
        std::string cur;
        char buf[4096];
        size_t n;
        while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
            cur.append(buf, n);
        }
        fclose(f);
        if (cur == text) {
            return;
        }
    }
    if (FILE *f = g_fopen(path.c_str(), "wb")) {
        fwrite(text.data(), 1, text.size(), f);
        fclose(f);
    }
}

} // namespace


ShortcutsManager::ShortcutsManager():
    table_(new ShortcutTable(ModifierMasks())),
    debug_(std::getenv("ART_SHORTCUTS_DEBUG") != nullptr)
{
}


ShortcutsManager *ShortcutsManager::getInstance()
{
    static ShortcutsManager instance;
    return &instance;
}


void ShortcutsManager::init(const Glib::ustring &system_dir,
                            const Glib::ustring &user_config_dir)
{
    table_.reset(new ShortcutTable(ModifierMasks::native()));
    defineDefaultShortcuts(*table_);

    // for "%shortcut(action.id)%" in the translated strings (tooltips, ...):
    // they are markup, hence the escaping
    utils::langMgr.setShortcutLabeler([this](const std::string &id) {
        if (!table_->contains(id)) {
            fprintf(stderr, "shortcuts: unknown action '%s' in a string\n",
                    id.c_str());
            return Glib::ustring("?");
        }
        auto l = table_->label(id);
        return l.empty() ? Glib::ustring("-") : Glib::Markup::escape_text(l);
    });

    // two layers on top of the built-in defaults: the shortcuts.json installed
    // with ART, then the user's one. Problems are reported with the name of
    // the file they were found in.
    system_dir_ = system_dir;
    user_dir_ = user_config_dir;
    std::vector<std::string> loaded;
    for (const auto &dir : {system_dir, user_config_dir}) {
        if (dir.empty()) {
            continue;
        }
        const std::string file = Glib::build_filename(dir, USER_FILE);
        if (table_->loadFile(file, false)) {
            loaded.push_back(file);
        }
        for (auto &w : table_->warnings()) {
            fprintf(stderr, "%s (%s)\n", w.c_str(), file.c_str());
        }
        table_->clearWarnings();
    }

    // "%mouse_primary%" in the strings: "Ctrl-click" or "Cmd-click"
    utils::langMgr.setMousePrimaryModifierName(table_->mouseModifierName());

    if (!user_config_dir.empty()) {
        // something to copy from: all the actions with their default keys,
        // kept up to date with the version of ART that is running
        writeIfDifferent(Glib::build_filename(user_config_dir, REFERENCE_FILE),
                         table_->dumpJson(true, true));
    }

    if (debug_) {
        for (auto &c : table_->conflicts()) {
            fprintf(stderr, "shortcuts: conflict: %s\n", c.c_str());
        }
        std::string from = "defaults only";
        for (size_t i = 0; i < loaded.size(); ++i) {
            from = (i ? from + ", " : "customised from ") + loaded[i];
        }
        fprintf(stderr, "shortcuts: %zu actions, %s\n", table_->ids().size(),
                from.c_str());
        fprintf(stderr, "shortcuts: mouse modifier: %s (mask 0x%x), save: %s\n",
                table_->mouseModifierName().c_str(), table_->mousePrimaryMask(),
                table_->label("editor.save").c_str());
    }
}


ShortcutTable ShortcutsManager::baselineTable() const
{
    ShortcutTable t(table_->masks());
    defineDefaultShortcuts(t);
    if (!system_dir_.empty()) {
        t.loadFile(Glib::build_filename(system_dir_, USER_FILE), false);
    }
    return t;
}


bool ShortcutsManager::saveUserFile(const ShortcutTable &edited,
                                    std::string &err) const
{
    if (user_dir_.empty()) {
        err = "no configuration folder";
        return false;
    }
    const std::string text = edited.dumpOverrides(baselineTable());
    const std::string path = Glib::build_filename(user_dir_, USER_FILE);
    const std::string backup = path + ".bak";

    if (Glib::file_test(path, Glib::FILE_TEST_IS_REGULAR) &&
        !Glib::file_test(backup, Glib::FILE_TEST_EXISTS)) {
        try {
            art::utils::writeFileFollowingSymlinks(
                backup, Glib::file_get_contents(path));
        } catch (Glib::Error &e) {
            err = "cannot back up " + path + ": " + e.what();
            return false;
        }
    }
    try {
        // atomic: a crash must not leave half a file
        art::utils::writeFileFollowingSymlinks(path, text);
    } catch (Glib::Error &e) {
        err = "cannot write " + path + ": " + e.what();
        return false;
    }
    return true;
}


void ShortcutsManager::bind(const void *owner, const std::string &id,
                            Handler handler, Guard enabled)
{
    if (!table_->contains(id)) {
        fprintf(stderr, "shortcuts: binding to unknown action '%s'\n",
                id.c_str());
        return;
    }
    bindings_[std::make_pair(owner, id)] = {std::move(handler),
                                            std::move(enabled)};
}


void ShortcutsManager::unbindOwner(const void *owner)
{
    for (auto it = bindings_.begin(); it != bindings_.end();) {
        if (it->first.first == owner) {
            it = bindings_.erase(it);
        } else {
            ++it;
        }
    }
}


std::vector<ShortcutKey> ShortcutsManager::candidates(GdkEventKey *event) const
{
    return shortcutCandidates(table_->masks(), getKeyval(event, false),
                              event->keyval, event->state);
}


bool ShortcutsManager::matches(GdkEventKey *event, ShortcutPhase phase,
                               const std::string &context) const
{
    return !table_->lookup(candidates(event), context, phase).empty();
}


bool ShortcutsManager::matchesKey(GdkEventKey *event, ShortcutPhase phase,
                                   const std::string &context) const
{
    return table_->usesKeyval(getKeyval(event, false), context, phase) ||
           table_->usesKeyval(event->keyval, context, phase);
}


bool ShortcutsManager::dispatch(GdkEventKey *event, ShortcutPhase phase,
                                const std::vector<ShortcutScope> &chain)
{
    const auto &masks = table_->masks();
    const auto cands = candidates(event);

    for (auto &scope : chain) {
        for (auto action : table_->lookup(cands, scope.context, phase)) {
            auto it = bindings_.find(std::make_pair(scope.owner, action->id));
            if (it == bindings_.end()) {
                continue;
            }
            auto &b = it->second;
            if (b.enabled && !b.enabled()) {
                continue;
            }
            // the handler may bind/unbind: work on a copy
            Handler h = b.handler;
            if (h(event)) {
                if (debug_) {
                    fprintf(stderr, "shortcuts: %s -> %s [%s]\n",
                            formatShortcutKey(cands[0], masks).c_str(),
                            action->id.c_str(), scope.context.c_str());
                }
                return true;
            }
        }
    }
    // a bare Shift/Ctrl/Alt/Cmd press is never a shortcut: keep it out of the log
    const bool modifier_only =
        (event->keyval >= GDK_KEY_Shift_L && event->keyval <= GDK_KEY_Hyper_R) ||
        event->keyval == GDK_KEY_ISO_Level3_Shift;
    // (the before phase sees every key first: only the final verdict is news)
    if (debug_ && !modifier_only && phase == ShortcutPhase::After) {
        fprintf(stderr, "shortcuts: %s: not handled (%s)\n",
                formatShortcutKey(cands[0], masks).c_str(),
                phase == ShortcutPhase::Before ? "before" : "after");
    }
    return false;
}


void ShortcutsManager::addAccelerator(
    Gtk::MenuItem &item, const Glib::RefPtr<Gtk::AccelGroup> &group,
    const std::string &id) const
{
    auto &keys = table_->keys(id);
    if (keys.empty()) {
        return;
    }
    guint keyval;
    GdkModifierType mods;
    shortcutToAccelerator(keys[0], table_->masks(), keyval, mods);
    item.add_accelerator("activate", group, keyval,
                         static_cast<Gdk::ModifierType>(mods),
                         Gtk::ACCEL_VISIBLE);
}


std::vector<std::string> ShortcutsManager::unboundActions() const
{
    std::set<std::string> bound;
    for (auto &b : bindings_) {
        bound.insert(b.first.second);
    }
    std::vector<std::string> res;
    for (auto &id : table_->ids()) {
        if (!bound.count(id)) {
            res.push_back(id);
        }
    }
    return res;
}


void ShortcutsManager::reportUnbound() const
{
    if (!debug_) {
        return;
    }
    for (auto &id : unboundActions()) {
        fprintf(stderr, "shortcuts: never bound: %s\n", id.c_str());
    }
}


Glib::ustring ShortcutsManager::label(const std::string &id) const
{
    return table_->label(id);
}


//-----------------------------------------------------------------------------
// ToolShortcutsManager
//-----------------------------------------------------------------------------

using namespace utils;


extern Options options;

namespace {

enum {
    DIRECTION_ZERO = 0,
    DIRECTION_DOWN = -1,
    DIRECTION_UP = 1,
    DIRECTION_INITIAL = 2
};

} // namespace


ToolShortcutsManager::ToolShortcutsManager(RTWindow *parent)
    : parent_(parent), cur_tool_(nullptr), cur_adjuster_(nullptr)
{
    auto sm = ShortcutsManager::getInstance();
    sm->bind(this, "tool.help", [this](GdkEventKey *) {
        showHelp();
        return true;
    });
    // +/-/reset while a key is held (the dispatch is only asked then)
    const auto value = [this, sm](const char *id, int direction) {
        sm->bind(this, id, [this, direction](GdkEventKey *) {
            doit(direction);
            return true;
        });
    };
    value("tool.value_up", DIRECTION_UP);
    value("tool.value_down", DIRECTION_DOWN);
    value("tool.value_zero", DIRECTION_ZERO);
    value("tool.value_initial", DIRECTION_INITIAL);
}

ToolShortcutsManager::~ToolShortcutsManager()
{
    ShortcutsManager::getInstance()->unbindOwner(this);
    if (conn_.connected()) {
        conn_.disconnect();
    }
}

void ToolShortcutsManager::reset(bool clear)
{
    cur_action_.clear();
    cur_tool_ = nullptr;
    cur_adjuster_ = nullptr;
    if (conn_.connected()) {
        conn_.disconnect();
    }
    if (clear) {
        action_map_.clear();
    }
}

void ToolShortcutsManager::addShortcut(const std::string &action_id,
                                      FoldableToolPanel *tool,
                                      Adjuster *adjuster)
{
    if (!tool || !adjuster) {
        return; // a slider that this tool did not create
    }
    action_map_[action_id] = std::make_pair(tool, adjuster);

    // pressing the key arms the adjuster (and key repeat keeps it so)
    ShortcutsManager::getInstance()->bind(
        this, action_id, [this, action_id](GdkEventKey *) {
            auto it = action_map_.find(action_id);
            if (it == action_map_.end()) {
                return false; // removed by reset(true)
            }
            if (cur_action_ != action_id) {
                cur_action_ = action_id;
                cur_tool_ = it->second.first;
                cur_adjuster_ = it->second.second;
            }
            return true;
        });
}

namespace {

bool has_modifiers(GdkEventKey *event)
{
    return event->state &
           (ShortcutsManager::getInstance()->primaryMask() |
            GDK_CONTROL_MASK | GDK_MOD1_MASK);
}

} // namespace

bool ToolShortcutsManager::keyPressed(GdkEventKey *event)
{
    auto sm = ShortcutsManager::getInstance();

    if (shouldHandleScroll() &&
        sm->dispatch(event, ShortcutPhase::After, {{"tool-value", this}})) {
        return true;
    }

    if (sm->dispatch(event, ShortcutPhase::After, {{"tool", this}})) {
        return true;
    }

    // any other key (without Ctrl/Alt/Cmd) lets go of the adjuster
    if (has_modifiers(event)) {
        return false;
    }
    return keyReleased(event);
}

bool ToolShortcutsManager::keyReleased(GdkEventKey *event)
{
    // the release of +/-/reset keys while an adjuster is held is not ours to
    // act on, but must not let go of it
    if (shouldHandleScroll() &&
        ShortcutsManager::getInstance()->matches(event, ShortcutPhase::After,
                                                 "tool-value")) {
        return true;
    }

    cur_action_.clear();
    cur_tool_ = nullptr;
    cur_adjuster_ = nullptr;

    return false;
}

bool ToolShortcutsManager::scrollPressed(GdkEventScroll *event)
{
    if (shouldHandleScroll()) {
        if (conn_.connected()) {
            conn_.disconnect();
        }

        int dir =
            event->direction == GDK_SCROLL_DOWN ? DIRECTION_DOWN : DIRECTION_UP;
        doit(dir, options.adjuster_shortcut_scrollwheel_factor);
        return true;
    }
    return false;
}

bool ToolShortcutsManager::shouldHandleScroll() const
{
    return (!cur_action_.empty() && cur_tool_ != nullptr &&
            cur_adjuster_ != nullptr);
}

void ToolShortcutsManager::doit(int direction, int speed)
{
    cur_tool_->disableListener();
    cur_tool_->setEnabled(true);
    if (direction == DIRECTION_ZERO) {
        cur_adjuster_->resetValue(false);
    } else if (direction == DIRECTION_INITIAL) {
        cur_adjuster_->resetValue(true);
    } else {
        cur_adjuster_->setValue(cur_adjuster_->getValue() +
                                direction * cur_adjuster_->getStepValue() *
                                    speed);
    }
    Glib::ustring msg = cur_tool_->getUILabel() + ": " +
                        cur_adjuster_->getLabel() + " = " +
                        cur_adjuster_->getTextValue();
    parent_->showInfo(msg, 0.0);

    FoldableToolPanel *tool = cur_tool_;
    Adjuster *adjuster = cur_adjuster_;
    const auto doit = [tool, adjuster]() -> bool {
        tool->enableListener();
        adjuster->forceNotifyListener();
        return false;
    };
    conn_ = Glib::signal_timeout().connect(sigc::slot<bool>(doit),
                                           options.adjusterMaxDelay);
}

void ToolShortcutsManager::showHelp()
{
    Glib::ustring msg = M("TOOL_SHORTCUT_HELP_HEADER");
    std::map<Glib::ustring, std::vector<Glib::ustring>> helpmap;
    for (auto &p : action_map_) {
        auto key = ShortcutsManager::getInstance()->label(p.first);
        if (key.empty()) {
            continue; // disabled
        }
        auto header = p.second.first->getUILabel();
        auto value = key + ": " + p.second.second->getLabel();
        helpmap[header].push_back(value);
    }

    Glib::ustring pad(10, ' ');
    for (auto &p : helpmap) {
        msg += "\n";
        msg += p.first;
        Glib::ustring headpad(p.first.size() + 2, ' ');
        auto &v = p.second;
        std::sort(v.begin(), v.end());
        for (size_t i = 0; i < v.size(); ++i) {
            msg += "\n" + pad + v[i];
        }
    }

    parent_->showInfo(msg, options.error_message_duration);
}

} } // namespace art::gui
