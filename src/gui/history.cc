/* -*- C++ -*-
 *  This file is part of ART.
 *
 *  Copyright (c) 2004-2010 Gabor Horvath <hgabor@rawtherapee.com>
 *  Copyright (c) 2017-2025 Alberto Griggio <alberto.griggio@gmail.com>
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
#include "history.h"
#include "eventmapper.h"
#include "guiutils.h"
#include "../utils/multilangmgr.h"
#include "rtimage.h"

#include <iostream>

namespace art { namespace gui {
using namespace utils;


using namespace art::engine;
using namespace art::engine::procparams;

History::History(bool bookmarkSupport)
    : historyVPaned(nullptr), blistener(nullptr), tpc(nullptr),
      snapshotListener(nullptr), shapshot_update_(false)
{

    blistenerLock =
        false; // sets default that the Before preview will not be locked
    // History List
    // ~~~~~~~~~~~~
    Gtk::ScrolledWindow *hscrollw = Gtk::manage(new Gtk::ScrolledWindow());
    hscrollw->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);

    Gtk::Frame *histFrame = Gtk::manage(new Gtk::Frame(M("HISTORY_LABEL")));
    histFrame->set_label_align(0.025, 0.5);
    histFrame->set_name("HistoryPanel");
    histFrame->add(*hscrollw);

    hTreeView = Gtk::manage(new Gtk::TreeView());
    hscrollw->add(*hTreeView);

    historyModel = Gtk::ListStore::create(historyColumns);
    hTreeView->set_model(historyModel);
    hTreeView->set_headers_visible(false);
    hTreeView->set_hscroll_policy(Gtk::SCROLL_MINIMUM);
    hTreeView->set_vscroll_policy(Gtk::SCROLL_NATURAL);
    hTreeView->set_size_request(80, -1);

    Gtk::CellRendererText *changecrt = Gtk::manage(new Gtk::CellRendererText());
    changecrt->property_ellipsize() = Pango::ELLIPSIZE_END;
    Gtk::CellRendererText *valuecrt = Gtk::manage(new Gtk::CellRendererText());
    valuecrt->property_ellipsize() = Pango::ELLIPSIZE_END;
    Gtk::TreeView::Column *hviewcol =
        Gtk::manage(new Gtk::TreeView::Column(""));
    hviewcol->pack_start(*changecrt, true);
    hviewcol->add_attribute(changecrt->property_text() /*markup ()*/,
                            historyColumns.text);
    hviewcol->set_expand(true);
    hviewcol->set_resizable(true);
    hviewcol->set_fixed_width(35);
    hviewcol->set_min_width(35);
    hviewcol->set_sizing(Gtk::TREE_VIEW_COLUMN_AUTOSIZE);

    Gtk::TreeView::Column *hviewcol2 =
        Gtk::manage(new Gtk::TreeView::Column(""));
    hviewcol2->pack_start(*valuecrt, true);
    hviewcol2->add_attribute(valuecrt->property_text() /*markup ()*/,
                             historyColumns.value);
    hviewcol2->set_expand(true);
    hviewcol2->set_resizable(true);
    hviewcol2->set_fixed_width(35);
    hviewcol2->set_min_width(35);
    hviewcol2->set_sizing(Gtk::TREE_VIEW_COLUMN_AUTOSIZE);
    valuecrt->set_alignment(1.f, 0.f);

    hTreeView->set_has_tooltip(true);
    hTreeView->signal_query_tooltip().connect(
        sigc::mem_fun(*this, &History::on_query_tooltip));
    hTreeView->append_column(*hviewcol);
    hTreeView->append_column(*hviewcol2);

    selchangehist = hTreeView->get_selection()->signal_changed().connect(
        sigc::mem_fun(*this, &History::historySelectionChanged));

    // Bookmark List
    // ~~~~~~~~~~~~~

    Gtk::HBox *ahbox = Gtk::manage(new Gtk::HBox());
    addBookmark = Gtk::manage(new Gtk::Button()); // M("HISTORY_NEWSNAPSHOT")
    setExpandAlignProperties(addBookmark, true, false, Gtk::ALIGN_FILL,
                             Gtk::ALIGN_START);
    // addBookmark->get_style_context()->set_junction_sides(Gtk::JUNCTION_RIGHT);
    addBookmark->get_style_context()->add_class("Left");
    addBookmark->set_tooltip_markup(M("HISTORY_NEWSNAPSHOT_TOOLTIP"));
    Gtk::Image *addimg = Gtk::manage(new RTImage("add-small.svg"));
    addBookmark->set_image(*addimg);
    ahbox->pack_start(*addBookmark);

    delBookmark = Gtk::manage(new Gtk::Button()); // M("HISTORY_DELSNAPSHOT")
    setExpandAlignProperties(delBookmark, true, false, Gtk::ALIGN_FILL,
                             Gtk::ALIGN_START);
    // delBookmark->get_style_context()->set_junction_sides(Gtk::JUNCTION_LEFT);
    delBookmark->get_style_context()->add_class("Right");
    Gtk::Image *delimg = Gtk::manage(new RTImage("remove-small.svg"));
    delBookmark->set_image(*delimg);
    ahbox->pack_start(*delBookmark);

    bscrollw = Gtk::manage(new Gtk::ScrolledWindow());
    //    bscrollw->set_policy (Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    bscrollw->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    bscrollw->set_size_request(-1, 45);

    Gtk::Frame *bmFrame = Gtk::manage(new Gtk::Frame(M("HISTORY_SNAPSHOTS")));
    bmFrame->set_label_align(0.025, 0.5);
    bmFrame->set_name("Snapshots");
    Gtk::VBox *bmBox = Gtk::manage(new Gtk::VBox());
    bmFrame->add(*bmBox);
    bmBox->pack_start(*bscrollw, Gtk::PACK_EXPAND_WIDGET, 4);
    bmBox->pack_end(*ahbox, Gtk::PACK_SHRINK, 4);
    bmBox->set_size_request(-1, 100);

    if (bookmarkSupport) {
        historyVPaned = Gtk::manage(new Gtk::VPaned());
        historyVPaned->pack1(*histFrame, true, true);
        historyVPaned->pack2(*bmFrame, false, false);
        pack_start(*historyVPaned);
    } else {
        pack_start(*histFrame);
    }

    bTreeView = Gtk::manage(new Gtk::TreeView());
    bscrollw->add(*bTreeView);

    bookmarkModel = Gtk::ListStore::create(bookmarkColumns);
    bTreeView->set_model(bookmarkModel);
    bTreeView->set_headers_visible(false);
    bTreeView->append_column_editable(M("HISTORY_SNAPSHOTS"),
                                      bookmarkColumns.text);

    selchangebm = bTreeView->get_selection()->signal_changed().connect(
        sigc::mem_fun(*this, &History::bookmarkSelectionChanged));
    bTreeView->add_events(Gdk::BUTTON_PRESS_MASK);
    bTreeView->signal_button_press_event().connect(
        sigc::mem_fun(*this, &History::onPressEvent), false);

    addBookmark->signal_clicked().connect(
        sigc::mem_fun(*this, &History::addBookmarkPressed));
    delBookmark->signal_clicked().connect(
        sigc::mem_fun(*this, &History::delBookmarkPressed));
    static_cast<Gtk::CellRendererText *>(bTreeView->get_column_cell_renderer(0))
        ->signal_edited()
        .connect(sigc::mem_fun(*this, &History::snapshotNameEdited));
    static_cast<Gtk::CellRendererText *>(bTreeView->get_column_cell_renderer(0))
        ->property_ellipsize() = Pango::ELLIPSIZE_MIDDLE;

    // hTreeView->set_grid_lines (Gtk::TREE_VIEW_GRID_LINES_HORIZONTAL);
    hTreeView->set_grid_lines(Gtk::TREE_VIEW_GRID_LINES_BOTH);
    // hTreeView->signal_size_allocate().connect( sigc::mem_fun(*this,
    // &History::resized) );

    hTreeView->set_enable_search(false);
    bTreeView->set_enable_search(false);

    show_all_children();
}

void History::initHistory()
{

    ConnectionBlocker selBlocker(selchangehist);
    model_.clear();
    historyModel->clear();
    bookmarkModel->clear();
}

// The state (the entries, the snapshots, which of them is selected) is in
// model_; the two list stores are views of it, kept in step by the sync*View()
// functions. The selection of the views is the user's way of changing the
// state, so the selection handlers below pass it on to the model.

void History::syncHistoryView()
{
    const auto &entries = model_.entries();
    const size_t n = entries.size();

    size_t i = 0;
    for (auto row : historyModel->children()) {
        if (i >= n) {
            break;
        }
        const Glib::ustring text(entries[i].text), value(entries[i].value);
        if (Glib::ustring(row[historyColumns.text]) != text) {
            row[historyColumns.text] = text;
        }
        if (Glib::ustring(row[historyColumns.value]) != value) {
            row[historyColumns.value] = value;
        }
        ++i;
    }
    while (historyModel->children().size() > n) {
        Gtk::TreeModel::Path path;
        path.push_back(historyModel->children().size() - 1);
        historyModel->erase(historyModel->get_iter(path));
    }
    while (historyModel->children().size() < n) {
        auto row = *historyModel->append();
        const auto &e = entries[historyModel->children().size() - 1];
        row[historyColumns.text] = Glib::ustring(e.text);
        row[historyColumns.value] = Glib::ustring(e.value);
    }

    Glib::RefPtr<Gtk::TreeSelection> selection = hTreeView->get_selection();
    const int sel = model_.selectedEntry();
    if (sel >= 0) {
        selection->select(historyModel->children()[sel]);
    } else {
        selection->unselect_all();
    }
}

void History::syncBookmarkView()
{
    const auto &snaps = model_.snapshots();
    const size_t n = snaps.size();

    size_t i = 0;
    for (auto row : bookmarkModel->children()) {
        if (i >= n) {
            break;
        }
        const Glib::ustring name(snaps[i].name);
        if (Glib::ustring(row[bookmarkColumns.text]) != name) {
            row[bookmarkColumns.text] = name;
        }
        ++i;
    }
    while (bookmarkModel->children().size() > n) {
        Gtk::TreeModel::Path path;
        path.push_back(bookmarkModel->children().size() - 1);
        bookmarkModel->erase(bookmarkModel->get_iter(path));
    }
    while (bookmarkModel->children().size() < n) {
        auto row = *bookmarkModel->append();
        row[bookmarkColumns.text] =
            Glib::ustring(snaps[bookmarkModel->children().size() - 1].name);
    }
}

void History::historySelectionChanged()
{

    Glib::RefPtr<Gtk::TreeSelection> selection = hTreeView->get_selection();
    Gtk::TreeModel::iterator iter = selection->get_selected();

    if (iter) {
        const int idx = historyModel->get_path(iter)[0];
        if (idx < 0 || idx >= model_.size()) {
            return;
        }
        // this deselects the active snapshot
        model_.selectEntry(idx);
        bTreeView->get_selection()->unselect_all();

        const auto &entry = model_.entry(idx);
        if (tpc) {
            FullPartialProfile pp(entry.params);
            tpc->profileChange(&pp, EvHistoryBrowsed, Glib::ustring(entry.text),
                               nullptr);
        }

        if (blistener && !blistenerLock) {
            blistener->historyBeforeAfterChanged(entry.params);
        }
    }
}

void History::bookmarkSelectionChanged()
{

    Glib::RefPtr<Gtk::TreeSelection> selection = bTreeView->get_selection();
    Gtk::TreeModel::iterator iter = selection->get_selected();

    if (iter) {
        const int idx = bookmarkModel->get_path(iter)[0];
        if (idx < 0 || idx >= model_.snapshotCount()) {
            return;
        }

        if (shapshot_update_) {
            shapshot_update_ = false;
            if (confirmBookmarkUpdate()) {
                // the snapshot takes the parameters of the selected history
                // entry, or of the last one if none is selected
                int h = model_.selectedEntry();
                if (h < 0) {
                    h = model_.size() - 1;
                }
                if (h >= 0) {
                    std::string err;
                    model_.overwriteSnapshot(model_.snapshot(idx).name,
                                             model_.entry(h).params, err);

                    if (snapshotListener) {
                        snapshotListener->snapshotsChanged(getSnapshots());
                    }
                }
            }
        } else {
            hTreeView->get_selection()->unselect_all();

            // this deselects the history entry
            const Glib::ustring name(model_.snapshot(idx).name);
            model_.activateSnapshot(name.raw());
            if (tpc) {
                FullPartialProfile pp(model_.snapshot(idx).params);
                tpc->profileChange(&pp, EvBookmarkSelected, name, nullptr);
            }
        }
    }
}

void History::procParamsChanged(const ProcParams *params, const ProcEvent &ev,
                                const Glib::ustring &descr,
                                const ParamsEdited *paramsEdited)
{
    // to prevent recursion, we filter out the events triggered by the history
    // and events that should not be registered
    if (ev == EvHistoryBrowsed || ev == EvMonitorTransform) {
        return;
    }

    // construct formatted list content
    Glib::ustring text = M(ProcEventMapper::getInstance()->getHistoryMsg(ev));

    if (text.empty() && descr.empty()) {
        return;
    }

    selchangehist.block(true);
    selchangebm.block(true);

    // the model drops the entries after the selection, and either updates the
    // last entry (same event) or adds a new one; EvPhotoLoaded starts afresh
    if (model_.procParamsChanged(*params, ev, text.raw(), descr.raw())) {
        syncHistoryView();
        if (ev == EvPhotoLoaded) {
            syncBookmarkView();
        }

        if (ev != EvBookmarkSelected) {
            bTreeView->get_selection()->unselect_all();
        }

        if (blistener && !blistenerLock) {
            blistener->historyBeforeAfterChanged(*model_.lastParams());
        }
    }

    Glib::RefPtr<Gtk::TreeSelection> selection = hTreeView->get_selection();
    if (!selection->get_selected_rows().empty()) {
        std::vector<Gtk::TreeModel::Path> selp = selection->get_selected_rows();
        hTreeView->scroll_to_row(*selp.begin());
    }

    selchangehist.block(false);
    selchangebm.block(false);
}

void History::clearParamChanges() { initHistory(); }

// show the snapshot just added to the model, selecting it and starting to
// edit its name
void History::showNewBookmark()
{
    syncBookmarkView();
    Gtk::TreeModel::Path path;
    path.push_back(bookmarkModel->children().size() - 1);
    bTreeView->set_cursor(path, *bTreeView->get_column(0), true);
}

void History::addBookmarkWithText(Glib::ustring text)
{
    // the snapshot has the parameters of the selected history entry
    const int h = model_.selectedEntry();
    if (h < 0) {
        return;
    }

    std::string err;
    if (model_.addSnapshot(text.raw(), model_.entry(h).params, err)) {
        showNewBookmark();
    }
}

void History::addBookmarkPressed()
{
    std::string name;
    if (model_.addSnapshotFromSelection(M("HISTORY_SNAPSHOT").raw(), name)) {
        showNewBookmark();

        if (snapshotListener) {
            snapshotListener->snapshotsChanged(getSnapshots());
        }
    }
}

void History::delBookmarkPressed()
{

    // lookup the selected item in the bookmark
    Glib::RefPtr<Gtk::TreeSelection> selection = bTreeView->get_selection();
    Gtk::TreeModel::iterator iter = selection->get_selected();

    if (!iter) {
        return;
    }

    // remove selected bookmark
    std::string err;
    model_.removeSnapshot(
        model_.snapshot(bookmarkModel->get_path(iter)[0]).name, err);
    bookmarkModel->erase(iter);
    // select last item in history
    const int size = model_.size();
    if (size > 0) {
        hTreeView->get_selection()->select(historyModel->children()[size - 1]);
    }

    if (snapshotListener) {
        snapshotListener->snapshotsChanged(getSnapshots());
    }
}

void History::undo()
{

    Glib::RefPtr<Gtk::TreeSelection> selection = hTreeView->get_selection();
    Gtk::TreeModel::iterator iter = selection->get_selected();

    if (iter && iter != historyModel->children().begin()) {
        selection->select(--iter);
    } else if (!iter) {
        int size = historyModel->children().size();

        if (size > 1) {
            selection->select(historyModel->children().operator[](size - 2));
        }
    }
}

void History::redo()
{

    Glib::RefPtr<Gtk::TreeSelection> selection = hTreeView->get_selection();
    Gtk::TreeModel::iterator iter = selection->get_selected();

    if (iter) {
        ++iter;

        if (iter != historyModel->children().end()) {
            selection->select(iter);
        }
    } else {
        int size = historyModel->children().size();

        if (size > 1) {
            selection->select(historyModel->children().operator[](size - 2));
        }
    }
}

/*
void History::resized (Gtk::Allocation& req)
{
}
*/

bool History::getBeforeAfterParams(art::engine::procparams::ProcParams &params)
{
    if (model_.size() == 0 || !blistener) {
        return false;
    }

    params = *model_.lastParams(); // the last entry
    return true;
}

bool History::on_query_tooltip(int x, int y, bool keyboard_tooltip,
                               const Glib::RefPtr<Gtk::Tooltip> &tooltip)
{
    bool displayTooltip = false;

    Gtk::TreeModel::Path path;
    int x2 = -1;
    int y2 = -1;
    hTreeView->convert_widget_to_bin_window_coords(x, y, x2, y2);
    bool hasPath = hTreeView->get_path_at_pos(x2, y2, path);

    if (hasPath) {
        if (path && !path.empty()) {
            Gtk::TreeModel::iterator iter = historyModel->get_iter(path);
            if (iter) {
                Glib::ustring text, val;
                iter->get_value<Glib::ustring>(0, text);
                iter->get_value<Glib::ustring>(1, val);

                /*
                 *
                 *
                 * Why this doesn't work ???
                 *
                 *
                 *
                Gtk::Label *left = Gtk::manage (new Gtk::Label(param+" :"));
                Gtk::Label *right = Gtk::manage (new Gtk::Label(val));
                right->set_justify(Gtk::JUSTIFY_LEFT);
                Gtk::HBox *hbox = Gtk::manage (new Gtk::HBox());
                hbox->set_spacing(5);
                hbox->pack_start(*left, Gtk::PACK_SHRINK, 0);
                hbox->pack_start(*right, Gtk::PACK_SHRINK, 0);
                tooltip->set_custom(*hbox);
                */

                tooltip->set_text(text + " : " + val);
                displayTooltip = true;
            }
        }
    }
    return displayTooltip;
}

void History::setPParamsSnapshotListener(PParamsSnapshotListener *l)
{
    snapshotListener = l;
}

void History::setSnapshots(
    const std::vector<
        std::pair<Glib::ustring, art::engine::procparams::ProcParams>> &snapshots)
{
    std::vector<Model::Snapshot> v;
    for (auto &p : snapshots) {
        v.push_back(Model::Snapshot{p.first.raw(), p.second});
    }
    model_.setSnapshots(std::move(v)); // none is active
    bookmarkModel->clear();
    syncBookmarkView();
}

std::vector<std::pair<Glib::ustring, art::engine::procparams::ProcParams>>
History::getSnapshots()
{
    std::vector<std::pair<Glib::ustring, art::engine::procparams::ProcParams>> ret;
    for (const auto &s : model_.snapshots()) {
        ret.push_back(std::make_pair(Glib::ustring(s.name), s.params));
    }
    return ret;
}

std::vector<History::Entry> History::getHistoryEntries()
{
    std::vector<Entry> ret;
    for (const auto &e : model_.entries()) {
        Entry r;
        r.text = e.text;
        r.value = e.value;
        ret.push_back(r);
    }
    return ret;
}

int History::getHistoryIndex() { return model_.selectedEntry(); }

bool History::selectHistoryIndex(int index)
{
    if (index < 0 || index >= model_.size()) {
        return false;
    }
    hTreeView->get_selection()->select(historyModel->children()[index]);
    return true;
}

std::vector<Glib::ustring> History::getSnapshotNames()
{
    std::vector<Glib::ustring> ret;
    for (const auto &n : model_.snapshotNames()) {
        ret.push_back(n);
    }
    return ret;
}

int History::getSnapshotIndex() { return model_.activeSnapshot(); }

bool History::activateSnapshot(const Glib::ustring &name)
{
    const int i = model_.findSnapshot(name.raw());
    if (i < 0) {
        return false;
    }
    shapshot_update_ = false;
    bTreeView->get_selection()->select(bookmarkModel->children()[i]);
    return true;
}

bool History::addSnapshot(const Glib::ustring &name,
                          const art::engine::procparams::ProcParams &params,
                          Glib::ustring &err)
{
    std::string e;
    if (!model_.addSnapshot(name.raw(), params, e)) {
        err = e;
        return false;
    }
    // not selected: the active history row stays as it is
    ConnectionBlocker block_sel(selchangebm);
    syncBookmarkView();
    if (snapshotListener) {
        snapshotListener->snapshotsChanged(getSnapshots());
    }
    return true;
}

bool History::removeSnapshot(const Glib::ustring &name, Glib::ustring &err)
{
    const int i = model_.findSnapshot(name.raw());
    if (i < 0) {
        err = "no snapshot named \"" + name + "\"";
        return false;
    }
    auto it = bookmarkModel->children()[i];
    const bool was_active = bTreeView->get_selection()->is_selected(it);
    std::string e;
    model_.removeSnapshot(name.raw(), e);
    bookmarkModel->erase(it);
    if (was_active) {
        // as the delete button does: back to the last history entry
        const int size = model_.size();
        if (size > 0) {
            hTreeView->get_selection()->select(historyModel->children()[size - 1]);
        }
    }
    if (snapshotListener) {
        snapshotListener->snapshotsChanged(getSnapshots());
    }
    return true;
}

bool History::overwriteSnapshot(
    const Glib::ustring &name, const art::engine::procparams::ProcParams &params,
    Glib::ustring &err)
{
    std::string e;
    if (!model_.overwriteSnapshot(name.raw(), params, e)) {
        err = e;
        return false;
    }
    if (snapshotListener) {
        snapshotListener->snapshotsChanged(getSnapshots());
    }
    return true;
}

// the name column is editable: the store has the new text already
void History::snapshotNameEdited(const Glib::ustring &path,
                                 const Glib::ustring &new_text)
{
    const int i = atoi(path.c_str());
    if (i >= 0 && i < model_.snapshotCount()) {
        const std::string old = model_.snapshot(i).name;
        std::string err;
        if (!model_.renameSnapshot(old, new_text.raw(), err)) {
            // not a usable name (empty, or that of another snapshot)
            auto row = *bookmarkModel->get_iter(path);
            row[bookmarkColumns.text] = Glib::ustring(old);
        }
    }
    if (snapshotListener) {
        snapshotListener->snapshotsChanged(getSnapshots());
    }
}

void History::enableSnapshots(bool yes) { bTreeView->set_sensitive(yes); }

bool History::onPressEvent(GdkEventButton *event)
{
    shapshot_update_ = event->button == 3;
    if (shapshot_update_) {
        ConnectionBlocker block_sel(selchangebm);
        bTreeView->get_selection()->unselect_all();
        model_.deselectSnapshot();
    }

    return false;
}

bool History::confirmBookmarkUpdate()
{
    Gtk::Popover p(*bTreeView);
    Gtk::HBox hb;
    Gtk::VBox vb;
    p.set_border_width(16);
    p.add(vb);
    Gtk::Label lbl(M("HISTORY_SNAPSHOT_CONFIRM_UPDATE"));
    Gtk::Label space("");
    vb.pack_start(lbl);
    hb.pack_start(space, Gtk::PACK_EXPAND_WIDGET);
    Gtk::Button ok(M("GENERAL_OK"));
    Gtk::Button cancel(M("GENERAL_CANCEL"));
    hb.pack_start(ok);
    hb.pack_start(cancel);
    vb.pack_start(hb);

    int result = 0;

    ok.signal_clicked().connect(sigc::slot<void>([&]() {
        result = 1;
        p.hide();
    }));
    cancel.signal_clicked().connect(sigc::slot<void>([&]() {
        result = -1;
        p.hide();
    }));
    p.signal_closed().connect(sigc::slot<void>([&]() {
        if (!result)
            result = -1;
    }));

    p.show_all_children();
    p.set_modal(true);
    p.show();
    // p.popup();

    while (result == 0) {
        gtk_main_iteration();
    }

    return result == 1;
}

void History::setBeforeAfterLock(bool yes)
{
    blistenerLock = yes;
    if (blistener && !blistenerLock) {
        const int h = model_.selectedEntry();
        if (h >= 0) {
            blistener->historyBeforeAfterChanged(model_.entry(h).params);
        }
    }
}

void History::activateNextSnapshot()
{
    auto children = bookmarkModel->children();
    if (children.size() == 0) {
        return;
    }

    Glib::RefPtr<Gtk::TreeSelection> selection = bTreeView->get_selection();
    Gtk::TreeModel::iterator iter = selection->get_selected();

    if (!iter) {
        selection->select(children.begin());
        return;
    }
    ++iter;
    if (iter == children.end()) {
        selection->select(children.begin());
    } else {
        selection->select(iter);
    }
}

void History::activatePrevSnapshot()
{
    auto children = bookmarkModel->children();
    if (children.size() == 0) {
        return;
    }

    Glib::RefPtr<Gtk::TreeSelection> selection = bTreeView->get_selection();
    Gtk::TreeModel::iterator iter = selection->get_selected();

    Gtk::TreeModel::iterator last = children.begin();
    for (int i = 1; i < children.size(); ++i) {
        ++last;
    }

    if (!iter) {
        selection->select(children.begin());
    } else if (iter == children.begin()) {
        selection->select(last);
    } else {
        --iter;
        selection->select(iter);
    }
}


} } // namespace art::gui
