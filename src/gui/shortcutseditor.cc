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

#include "shortcutseditor.h"
#include "guiutils.h"
#include "shortcutsmanager.h"
#include "systemshortcuts.h"
#include "../utils/multilangmgr.h"

#include <algorithm>
#include <initializer_list>

namespace art { namespace gui {

using utils::M;

namespace {

Glib::ustring prettify(const std::string &component)
{
    Glib::ustring res(component);
    for (auto it = res.begin(); it != res.end(); ++it) {
        if (*it == '_') {
            res.replace(it, std::next(it), " ");
        }
    }
    if (!res.empty()) {
        res.replace(res.begin(), std::next(res.begin()),
                    res.substr(0, 1).uppercase());
    }
    return res;
}


std::vector<std::string> split(const std::string &id)
{
    std::vector<std::string> res;
    size_t b = 0;
    while (true) {
        size_t e = id.find('.', b);
        res.push_back(id.substr(b, e == std::string::npos ? e : e - b));
        if (e == std::string::npos) {
            return res;
        }
        b = e + 1;
    }
}

} // namespace


ShortcutsEditor::ShortcutsEditor():
    Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4),
    work_(ShortcutsManager::getInstance()->table()),
    base_(ShortcutsManager::getInstance()->baselineTable())
{
    saved_ = work_.dumpOverrides(base_);
    set_border_width(4);

    search_.set_placeholder_text(M("PREFERENCES_SHORTCUTS_SEARCH"));
    pack_start(search_, Gtk::PACK_SHRINK);

    model_ = Gtk::TreeStore::create(columns_);
    // groups and actions are sorted alphabetically on their labels
    model_->set_sort_func(
        columns_.name,
        [this](const Gtk::TreeIter &a, const Gtk::TreeIter &b) {
            const Glib::ustring la = (*a)[columns_.name];
            const Glib::ustring lb = (*b)[columns_.name];
            return la.casefold_collate_key().compare(lb.casefold_collate_key());
        });
    model_->set_sort_column(columns_.name, Gtk::SORT_ASCENDING);
    filter_ = Gtk::TreeModelFilter::create(model_);
    filter_->set_visible_func(
        sigc::mem_fun(*this, &ShortcutsEditor::rowVisible));
    tree_.set_model(filter_);
    tree_.set_tooltip_column(columns_.tooltip.index());
    tree_.set_enable_search(false); // the search box does it
    setTreeViewCssProvider(&tree_);

    auto *name_col = Gtk::manage(new Gtk::TreeViewColumn(
        M("PREFERENCES_SHORTCUTS_ACTION")));
    auto *name_cell = Gtk::manage(new Gtk::CellRendererText());
    name_col->pack_start(*name_cell);
    name_col->add_attribute(name_cell->property_text(), columns_.name);
    name_col->add_attribute(name_cell->property_weight(), columns_.weight);
    name_col->set_expand(true);
    tree_.append_column(*name_col);

    auto *keys_col = Gtk::manage(new Gtk::TreeViewColumn(
        M("PREFERENCES_SHORTCUTS_KEYS")));
    auto *keys_cell = Gtk::manage(new Gtk::CellRendererText());
    keys_col->pack_start(*keys_cell);
    keys_col->add_attribute(keys_cell->property_text(), columns_.keys);
    keys_col->add_attribute(keys_cell->property_weight(), columns_.weight);
    tree_.append_column(*keys_col);

    auto *sw = Gtk::manage(new Gtk::ScrolledWindow());
    sw->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    sw->set_shadow_type(Gtk::SHADOW_IN);
    sw->add(tree_);
    pack_start(*sw, Gtk::PACK_EXPAND_WIDGET);

    auto *buttons = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
    change_ = Gtk::manage(new Gtk::Button(M("PREFERENCES_SHORTCUTS_CHANGE")));
    add_ = Gtk::manage(new Gtk::Button(M("PREFERENCES_SHORTCUTS_ADD")));
    disable_ = Gtk::manage(new Gtk::Button(M("GENERAL_DISABLE")));
    reset_ = Gtk::manage(new Gtk::Button(M("GENERAL_RESET")));
    auto *reset_all =
        Gtk::manage(new Gtk::Button(M("PREFERENCES_SHORTCUTS_RESETALL")));
    for (auto b : {change_, add_, disable_, reset_}) {
        buttons->pack_start(*b, Gtk::PACK_SHRINK);
    }
    buttons->pack_end(*reset_all, Gtk::PACK_SHRINK);
    pack_start(*buttons, Gtk::PACK_SHRINK);

    // which modifier "Primary" is, and which one the mouse actions use
    {
        const auto row = [&](const char *label, const char *tip,
                             Gtk::RadioButton &ctrl, Gtk::RadioButton &os,
                             bool is_os, sigc::connection &conn,
                             void (ShortcutsEditor::*changed)()) {
            auto *box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
            auto *l = Gtk::manage(new Gtk::Label(M(label) + ":"));
            ctrl.set_label(work_.osKeyOrCtrlName(false));
            os.set_label(work_.osKeyOrCtrlName(true));
            os.join_group(ctrl);
            (is_os ? os : ctrl).set_active(true);
            for (Gtk::Widget *w : std::initializer_list<Gtk::Widget *>{
                     l, &ctrl, &os}) {
                w->set_tooltip_text(M(tip));
            }
            box->pack_start(*l, Gtk::PACK_SHRINK);
            box->pack_start(ctrl, Gtk::PACK_SHRINK);
            box->pack_start(os, Gtk::PACK_SHRINK);
            pack_start(*box, Gtk::PACK_SHRINK);
            // a change toggles both buttons: the state of one says it all
            conn = os.signal_toggled().connect(
                sigc::mem_fun(*this, changed));
        };
        row("PREFERENCES_SHORTCUTS_PRIMARY", "PREFERENCES_SHORTCUTS_PRIMARY_TIP",
            primary_ctrl_, primary_os_, work_.primaryIsOsKey(), primary_conn_,
            &ShortcutsEditor::primaryModifierChanged);
        row("PREFERENCES_SHORTCUTS_MOUSEMOD", "PREFERENCES_SHORTCUTS_MOUSEMOD_TIP",
            mouse_ctrl_, mouse_os_, work_.mouseIsOsKey(), mouse_conn_,
            &ShortcutsEditor::mouseModifierChanged);
    }

    auto *note = Gtk::manage(new Gtk::Label(M("PREFERENCES_SHORTCUTS_RESTART")));
    note->set_xalign(0);
    note->set_line_wrap(true);
    pack_start(*note, Gtk::PACK_SHRINK);

    change_->signal_clicked().connect(
        sigc::bind(sigc::mem_fun(*this, &ShortcutsEditor::changePressed),
                   false));
    add_->signal_clicked().connect(
        sigc::bind(sigc::mem_fun(*this, &ShortcutsEditor::changePressed),
                   true));
    disable_->signal_clicked().connect(
        sigc::mem_fun(*this, &ShortcutsEditor::disablePressed));
    reset_->signal_clicked().connect(
        sigc::mem_fun(*this, &ShortcutsEditor::resetPressed));
    reset_all->signal_clicked().connect(
        sigc::mem_fun(*this, &ShortcutsEditor::resetAllPressed));
    search_.signal_search_changed().connect(
        sigc::mem_fun(*this, &ShortcutsEditor::searchChanged));
    // Esc clears the search (and only closes the dialog if it is empty)
    search_.signal_key_press_event().connect(
        [this](GdkEventKey *ev) {
            if (ev->keyval == GDK_KEY_Escape &&
                !(ev->state & gtk_accelerator_get_default_mod_mask()) &&
                !search_.get_text().empty()) {
                search_.set_text("");
                searchChanged(); // now, not after the entry's delay
                return true;
            }
            return false;
        },
        false);
    tree_.get_selection()->signal_changed().connect(
        sigc::mem_fun(*this, &ShortcutsEditor::selectionChanged));
    tree_.signal_row_activated().connect(
        sigc::mem_fun(*this, &ShortcutsEditor::rowActivated));

    populate();
    selectionChanged();
}


// The row of a node of the hierarchy ("editor", "editor.zoom_fit"), created
// together with its parents; the model sorts them alphabetically on their
// labels.
Gtk::TreeIter ShortcutsEditor::node(const std::string &path)
{
    auto it = nodes_.find(path);
    if (it != nodes_.end()) {
        return it->second;
    }
    const size_t dot = path.rfind('.');
    Gtk::TreeIter row;
    if (dot == std::string::npos) {
        row = model_->append();
        (*row)[columns_.name] = prettify(path);
    } else {
        row = model_->append(node(path.substr(0, dot))->children());
        (*row)[columns_.name] = prettify(path.substr(dot + 1));
    }
    (*row)[columns_.weight] = Pango::WEIGHT_NORMAL;
    nodes_[path] = row;
    return row;
}


// Text that a search looks into: for an action, its name, id and keys; for a
// group, its name
bool ShortcutsEditor::rowMatches(const Gtk::TreeModel::const_iterator &row) const
{
    Glib::ustring text = (*row)[columns_.name];
    const std::string id = (*row)[columns_.id];
    if (!id.empty()) {
        text += " " + displayName(id) + " " + id + " " +
                Glib::ustring((*row)[columns_.keys]);
    }
    return text.lowercase().find(query_) != Glib::ustring::npos;
}


// A row is shown if it matches, if a group above it matches (the whole group
// is then shown), or if something below it does
bool ShortcutsEditor::rowVisible(
    const Gtk::TreeModel::const_iterator &row) const
{
    if (query_.empty() || rowMatches(row)) {
        return true;
    }
    for (auto p = row->parent(); p; p = p->parent()) {
        if (rowMatches(p)) {
            return true;
        }
    }
    for (auto &c : row->children()) {
        if (rowVisible(c)) {
            return true;
        }
    }
    return false;
}


void ShortcutsEditor::searchChanged()
{
    const std::string selected = selectedAction();
    query_ = search_.get_text().lowercase();
    filter_->refilter();
    if (!query_.empty()) {
        tree_.expand_all();
    } else {
        tree_.collapse_all();
    }
    // keep the selected action, if it is still shown (it is when the search is
    // cleared): open its groups, select it and bring it into view
    if (!selected.empty()) {
        auto it = nodes_.find(selected);
        if (it != nodes_.end()) {
            const auto path = filter_->convert_child_path_to_path(
                model_->get_path(it->second));
            if (!path.empty()) {
                tree_.expand_to_path(path);
                tree_.get_selection()->select(path);
                tree_.scroll_to_row(path, 0.5);
            }
        }
    }
    selectionChanged();
}


// After Primary became the OS key (Cmd): some of the shortcuts that use it may
// be taken by the system (Cmd+F2, Cmd+, ...). Ask what to do with each of
// them. Returns false if the user wants to give up the change.
bool ShortcutsEditor::resolveSystemClashes()
{
    for (auto &id : work_.ids()) {
        for (const auto &key : work_.keys(id)) {
            if (!(key.mods & shortcut_mod::Primary)) {
                continue; // not affected by the change
            }
            while (isSystemShortcut(key, work_.masks())) {
                Glib::ustring text = Glib::ustring::compose(
                    M("PREFERENCES_SHORTCUTS_SYSCLASH"),
                    Glib::Markup::escape_text(
                        formatShortcutKey(key, work_.masks(), true)),
                    Glib::Markup::escape_text(displayName(id)));
                Glib::ustring question;
                const auto sep = text.find("\n\n");
                if (sep != Glib::ustring::npos) {
                    question = text.substr(sep + 2);
                    text = text.substr(0, sep);
                }
                Gtk::MessageDialog msg(getToplevelWindow(this), text, true,
                                       Gtk::MESSAGE_QUESTION,
                                       Gtk::BUTTONS_NONE, true);
                msg.set_secondary_text(question);
                msg.add_button(M("PREFERENCES_SHORTCUTS_BACKTOCTRL"),
                               Gtk::RESPONSE_CANCEL);
                msg.add_button(M("GENERAL_DISABLE"), Gtk::RESPONSE_REJECT);
                msg.add_button(M("PREFERENCES_SHORTCUTS_CHOOSE"),
                               Gtk::RESPONSE_APPLY);
                msg.add_button(M("PREFERENCES_SHORTCUTS_KEEP"),
                               Gtk::RESPONSE_ACCEPT);
                const int resp = msg.run();
                msg.hide();
                if (resp == Gtk::RESPONSE_ACCEPT) {
                    break;
                }
                auto without = work_.keys(id);
                without.erase(std::remove(without.begin(), without.end(), key),
                              without.end());
                if (resp == Gtk::RESPONSE_REJECT) {
                    work_.setKeys(id, without);
                    break;
                }
                if (resp == Gtk::RESPONSE_APPLY) {
                    ShortcutKey nk;
                    if (recordKey(nk)) {
                        const auto before = work_.keys(id);
                        work_.setKeys(id, without);
                        if (assign(id, nk, true)) {
                            break;
                        }
                        work_.setKeys(id, before); // gave up: ask again
                    }
                    continue;
                }
                return false; // go back to Ctrl
            }
        }
    }
    return true;
}


void ShortcutsEditor::primaryModifierChanged()
{
    const bool os = primary_os_.get_active();
    if (os) {
        const ShortcutTable saved = work_;
        work_.setPrimaryIsOsKey(true);
        if (!resolveSystemClashes()) {
            work_ = saved;
            syncModifierButtons();
        }
    } else {
        work_.setPrimaryIsOsKey(false);
    }
    refresh(); // the keys are shown with it
    selectionChanged();
}


void ShortcutsEditor::mouseModifierChanged()
{
    work_.setMouseIsOsKey(mouse_os_.get_active());
}


// after the buttons were set from a table (reset all)
void ShortcutsEditor::syncModifierButtons()
{
    primary_conn_.block();
    mouse_conn_.block();
    (work_.primaryIsOsKey() ? primary_os_ : primary_ctrl_).set_active(true);
    (work_.mouseIsOsKey() ? mouse_os_ : mouse_ctrl_).set_active(true);
    primary_conn_.unblock();
    mouse_conn_.unblock();
}


void ShortcutsEditor::populate()
{
    for (auto &id : work_.ids()) {
        auto row = node(id);
        (*row)[columns_.id] = id;
        auto action = work_.find(id);
        (*row)[columns_.tooltip] =
            Glib::Markup::escape_text(id + "  [" + action->context + "]");
        refreshRow(row);
    }
}


Glib::ustring ShortcutsEditor::displayKeys(const std::string &id) const
{
    Glib::ustring res;
    for (auto &k : work_.keys(id)) {
        res += (res.empty() ? "" : ", ") +
               Glib::ustring(formatShortcutKey(k, work_.masks(), true));
    }
    return res.empty() ? M("GENERAL_DISABLED") : res;
}


Glib::ustring ShortcutsEditor::displayName(const std::string &id)
{
    Glib::ustring res;
    for (auto &c : split(id)) {
        res += (res.empty() ? "" : " / ") + prettify(c);
    }
    return res;
}


void ShortcutsEditor::refreshRow(const Gtk::TreeIter &row)
{
    const std::string id = (*row)[columns_.id];
    if (id.empty()) {
        return;
    }
    (*row)[columns_.keys] = displayKeys(id);
    (*row)[columns_.weight] =
        work_.keys(id) != base_.keys(id) ? Pango::WEIGHT_BOLD
                                         : Pango::WEIGHT_NORMAL;
}


void ShortcutsEditor::refresh()
{
    // an assignment can also take a key away from another action
    for (auto &n : nodes_) {
        refreshRow(n.second);
    }
    if (!query_.empty()) {
        filter_->refilter(); // a search also looks at the keys
    }
}


std::string ShortcutsEditor::selectedAction()
{
    if (auto row = tree_.get_selection()->get_selected()) {
        return (*row)[columns_.id];
    }
    return "";
}


void ShortcutsEditor::selectionChanged()
{
    const std::string id = selectedAction();
    const bool action = !id.empty();
    change_->set_sensitive(action);
    add_->set_sensitive(action);
    disable_->set_sensitive(action && !work_.keys(id).empty());
    reset_->set_sensitive(action && work_.keys(id) != base_.keys(id));
}


void ShortcutsEditor::rowActivated(const Gtk::TreeModel::Path &path,
                                   Gtk::TreeViewColumn *)
{
    if (selectedAction().empty()) {
        if (tree_.row_expanded(path)) {
            tree_.collapse_row(path);
        } else {
            tree_.expand_row(path, false);
        }
    } else {
        changePressed(false);
    }
}


// Ask for a key press. Esc (or the Cancel button) gives up.
bool ShortcutsEditor::recordKey(ShortcutKey &out)
{
    Gtk::Dialog dlg(M("PREFERENCES_SHORTCUTS_PRESS"), getToplevelWindow(this),
                    true);
    auto *label = Gtk::manage(new Gtk::Label(
        M("PREFERENCES_SHORTCUTS_PRESS") + "\n\n" +
        M("PREFERENCES_SHORTCUTS_ESC")));
    label->set_justify(Gtk::JUSTIFY_CENTER);
    label->set_margin_top(24);
    label->set_margin_bottom(24);
    label->set_margin_start(24);
    label->set_margin_end(24);
    dlg.get_content_area()->pack_start(*label);
    dlg.add_button(M("GENERAL_CANCEL"), Gtk::RESPONSE_CANCEL);

    bool got = false;
    const auto &masks = work_.masks();
    dlg.signal_key_press_event().connect(
        [&](GdkEventKey *ev) {
            // a modifier alone is not a shortcut: wait for the key
            if ((ev->keyval >= GDK_KEY_Shift_L &&
                 ev->keyval <= GDK_KEY_Hyper_R) ||
                ev->keyval == GDK_KEY_ISO_Level3_Shift) {
                return true;
            }
            if (ev->keyval == GDK_KEY_Escape &&
                !(ev->state & gtk_accelerator_get_default_mod_mask())) {
                dlg.response(Gtk::RESPONSE_CANCEL);
                return true;
            }
            // a modifier that cannot be part of a shortcut (the OS key while
            // it is not the Primary one): wait for another key
            if (masks.normalize(ev->state) & shortcut_mod::Other) {
                return true;
            }
            out = shortcutFromEvent(masks, getKeyval(ev, false), ev->keyval,
                                    ev->state);
            got = out.valid();
            dlg.response(got ? Gtk::RESPONSE_OK : Gtk::RESPONSE_CANCEL);
            return true;
        },
        false); // before the dialog's own handling (Tab, mnemonics, ...)
    dlg.show_all_children();
    dlg.run();
    return got;
}


bool ShortcutsEditor::assign(const std::string &id, const ShortcutKey &key,
                             bool add)
{
    // the actions that would clash for sure (same context and phase) and the
    // ones of another context that can be reached by the same key press
    std::vector<std::string> others;
    const std::string same = work_.findConflict(id, key);
    if (!same.empty()) {
        others.push_back(same);
    }
    const bool hard = !others.empty();
    for (auto &o : work_.findOverlaps(id, key)) {
        others.push_back(o);
    }

    // macOS may take the key before ART sees it
    const bool system = isSystemShortcut(key, work_.masks());
    const Glib::ustring keyname = Glib::Markup::escape_text(
        formatShortcutKey(key, work_.masks(), true));

    if (!others.empty() || system) {
        Glib::ustring names;
        for (auto &o : others) {
            names += (names.empty() ? "" : ", ") +
                     Glib::Markup::escape_text(displayName(o));
        }
        // the strings are Pango markup: the first paragraph is the statement
        // (shown in bold by the dialog), the rest the question
        Glib::ustring text = Glib::ustring::compose(
            M(others.empty() ? "PREFERENCES_SHORTCUTS_SYSTEM"
                             : hard ? "PREFERENCES_SHORTCUTS_CONFLICT"
                                    : "PREFERENCES_SHORTCUTS_OVERLAP"),
            keyname, names);
        Glib::ustring question;
        const auto sep = text.find("\n\n");
        if (sep != Glib::ustring::npos) {
            question = text.substr(sep + 2);
            text = text.substr(0, sep);
        }
        if (system && !others.empty()) {
            question += "\n\n" + Glib::ustring::compose(
                                    M("PREFERENCES_SHORTCUTS_SYSTEMNOTE"),
                                    keyname);
        }
        Gtk::MessageDialog msg(getToplevelWindow(this), text, true,
                               Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE, true);
        if (!question.empty()) {
            msg.set_secondary_text(question);
        }
        msg.add_button(M("GENERAL_CANCEL"), Gtk::RESPONSE_CANCEL);
        if (!hard) {
            msg.add_button(M(others.empty()
                                 ? "PREFERENCES_SHORTCUTS_ASSIGNANYWAY"
                                 : "PREFERENCES_SHORTCUTS_KEEPBOTH"),
                           Gtk::RESPONSE_ACCEPT);
        }
        if (!others.empty()) {
            msg.add_button(M("PREFERENCES_SHORTCUTS_REASSIGN"),
                           Gtk::RESPONSE_OK);
        }
        const int resp = msg.run();
        if (resp != Gtk::RESPONSE_OK && resp != Gtk::RESPONSE_ACCEPT) {
            return false;
        }
        if (resp == Gtk::RESPONSE_OK) {
            // the other actions lose the key (and are disabled if it was
            // their last)
            for (auto &other : others) {
                auto keys = work_.keys(other);
                keys.erase(std::remove(keys.begin(), keys.end(), key),
                           keys.end());
                work_.setKeys(other, keys);
            }
        }
    }
    auto keys = add ? work_.keys(id) : std::vector<ShortcutKey>();
    keys.push_back(key);
    work_.setKeys(id, keys);
    return true;
}


void ShortcutsEditor::changePressed(bool add)
{
    const std::string id = selectedAction();
    ShortcutKey key;
    if (!id.empty() && recordKey(key)) {
        assign(id, key, add);
        refresh();
        selectionChanged();
    }
}


void ShortcutsEditor::disablePressed()
{
    const std::string id = selectedAction();
    if (!id.empty()) {
        work_.setKeys(id, {});
        refresh();
        selectionChanged();
    }
}


void ShortcutsEditor::resetPressed()
{
    const std::string id = selectedAction();
    if (!id.empty()) {
        // the baseline cannot clash with itself, but another action may have
        // been given its key since: take it back from there
        for (auto &k : base_.keys(id)) {
            const std::string other = work_.findConflict(id, k);
            if (!other.empty()) {
                auto keys = work_.keys(other);
                keys.erase(std::remove(keys.begin(), keys.end(), k),
                           keys.end());
                work_.setKeys(other, keys);
            }
        }
        work_.setKeys(id, base_.keys(id));
        refresh();
        selectionChanged();
    }
}


void ShortcutsEditor::resetAllPressed()
{
    work_.setPrimaryIsOsKey(base_.primaryIsOsKey());
    work_.setMouseIsOsKey(base_.mouseIsOsKey());
    for (auto &id : work_.ids()) {
        work_.setKeys(id, base_.keys(id));
    }
    syncModifierButtons();
    refresh();
    selectionChanged();
}


bool ShortcutsEditor::save(std::string &err)
{
    const std::string text = work_.dumpOverrides(base_);
    if (text == saved_) {
        return true; // nothing changed: leave the user's file alone
    }
    if (ShortcutsManager::getInstance()->saveUserFile(work_, err)) {
        saved_ = text;
        return true;
    }
    return false;
}

} } // namespace art::gui
