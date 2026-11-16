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

#include "../procevents.h"
#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace art { namespace engine { namespace remote {

// The edit history (undo/redo list) and the snapshots ("bookmarks") of one
// open image, with no GUI in it: the rules that the History widget used to
// keep in its Gtk::ListStores (gui/history.cc).
//
// There are two lists and at most one selection among both of them: a history
// entry is selected, or a snapshot is, or neither. Selecting one deselects the
// other. A snapshot being active means that no history entry is selected.
//
// This class only keeps the state. Applying the parameters of a selected entry
// or snapshot to the image is up to the caller (see entryParams()/
// snapshotParams()). Texts are plain, localised by the caller.
//
// `Params` is procparams::ProcParams in the application; the template keeps
// this header (and its tests) free of the engine.
template <class Params>
class BasicHistoryModel {
public:
    struct Entry {
        ProcEvent event;
        std::string text;
        std::string value;
        Params params;
        // see procParamsChanged(); empty for the entries that cannot merge
        std::string mergeKey;
    };
    struct Snapshot {
        std::string name;
        Params params;
    };

    // ---- history ----------------------------------------------------------

    // Record a change of the parameters (the procParamsChanged() of the GUI).
    // `text` is the localised name of the event, `descr` its value.
    // Returns false if the change is not recorded: the events produced by
    // browsing the history itself and by monitor changes, and events with no
    // text at all. As for the others:
    //  - EvPhotoLoaded first clears history and snapshots;
    //  - the entries after the selected one (if any) are dropped;
    //  - if the last remaining entry has the same event it is updated in
    //    place (so that dragging a slider makes one entry), except for
    //    EvProfileChanged, which always makes a new one unless `mergeKey` is
    //    given: then (and only then) an EvProfileChanged entry is updated in
    //    place if it is the selected, last entry and was recorded with the
    //    same key. This lets a client that applies a profile for each step of
    //    a gesture (a slider drag) make one entry per gesture;
    //  - the new or updated entry becomes the selection and no snapshot is
    //    active, except for EvBookmarkSelected: that one only records the
    //    snapshot's parameters at the end of the list and leaves the
    //    selection (the snapshot) alone.
    bool procParamsChanged(const Params &params, ProcEvent ev,
                           const std::string &text, const std::string &descr,
                           const std::string &mergeKey = std::string())
    {
        if (ev == EvHistoryBrowsed || ev == EvMonitorTransform) {
            return false;
        }
        if (text.empty() && descr.empty()) {
            return false;
        }
        if (ev == EvPhotoLoaded) {
            clear();
        }
        if (hsel_ >= 0) {
            entries_.erase(entries_.begin() + hsel_ + 1, entries_.end());
        }
        const bool select = ev != EvBookmarkSelected;
        bool merge;
        if (entries_.empty() || entries_.back().event != ev) {
            merge = false;
        } else if (ev == EvProfileChanged) {
            merge = !mergeKey.empty() && entries_.back().mergeKey == mergeKey &&
                    hsel_ == int(entries_.size()) - 1;
        } else {
            merge = true;
        }
        if (!merge) {
            entries_.push_back(Entry{ev, text, descr, params, mergeKey});
        } else {
            auto &e = entries_.back();
            e.text = text;
            e.value = descr;
            e.event = ev;
            e.params = params;
        }
        if (select) {
            hsel_ = int(entries_.size()) - 1;
            ssel_ = -1;
        }
        return true;
    }

    // forget everything: history, snapshots and selection
    void clear()
    {
        entries_.clear();
        snapshots_.clear();
        hsel_ = ssel_ = -1;
    }

    int size() const { return int(entries_.size()); }
    const Entry &entry(int i) const { return entries_.at(i); }
    const std::vector<Entry> &entries() const { return entries_; }
    // index of the selected entry, -1 if none (e.g. a snapshot is active)
    int selectedEntry() const { return hsel_; }

    // select entry `i` (and deselect the snapshot); false if out of range
    bool selectEntry(int i)
    {
        if (i < 0 || i >= size()) {
            return false;
        }
        hsel_ = i;
        ssel_ = -1;
        return true;
    }

    // The Undo / Redo buttons: one step back / forward from the selection.
    // With no selection (a snapshot is active) both go to the second to last
    // entry, if there is more than one. Return true if the selection changed.
    bool undo()
    {
        if (hsel_ > 0) {
            return selectEntry(hsel_ - 1);
        } else if (hsel_ < 0 && size() > 1) {
            return selectEntry(size() - 2);
        }
        return false;
    }

    bool redo()
    {
        if (hsel_ >= 0) {
            return hsel_ + 1 < size() && selectEntry(hsel_ + 1);
        } else if (size() > 1) {
            return selectEntry(size() - 2);
        }
        return false;
    }

    // the parameters of the last entry (nullptr if the history is empty)
    const Params *lastParams() const
    {
        return entries_.empty() ? nullptr : &entries_.back().params;
    }

    // ---- snapshots --------------------------------------------------------

    int snapshotCount() const { return int(snapshots_.size()); }
    const Snapshot &snapshot(int i) const { return snapshots_.at(i); }
    const std::vector<Snapshot> &snapshots() const { return snapshots_; }
    std::vector<std::string> snapshotNames() const
    {
        std::vector<std::string> ret;
        for (auto &s : snapshots_) {
            ret.push_back(s.name);
        }
        return ret;
    }
    // index of the active snapshot, -1 if none
    int activeSnapshot() const { return ssel_; }
    int findSnapshot(const std::string &name) const
    {
        for (size_t i = 0; i < snapshots_.size(); ++i) {
            if (snapshots_[i].name == name) {
                return int(i);
            }
        }
        return -1;
    }

    // Replace all the snapshots (when an image is opened); none is active.
    void setSnapshots(std::vector<Snapshot> snapshots)
    {
        snapshots_ = std::move(snapshots);
        ssel_ = -1;
    }

    // Activate the snapshot and deselect the history entry; false if there is
    // no such snapshot.
    bool activateSnapshot(const std::string &name)
    {
        int i = findSnapshot(name);
        if (i < 0) {
            return false;
        }
        ssel_ = i;
        hsel_ = -1;
        return true;
    }

    // No snapshot is active any more (the history selection is not changed).
    void deselectSnapshot() { ssel_ = -1; }

    // The next/previous snapshot, wrapping around, starting from the first
    // (resp. the first one for "previous") if none is active. Returns the
    // snapshot activated, -1 if there are none.
    int activateNextSnapshot()
    {
        if (snapshots_.empty()) {
            return -1;
        }
        return activateIndex(ssel_ < 0 ? 0 : (ssel_ + 1) % snapshotCount());
    }

    int activatePrevSnapshot()
    {
        if (snapshots_.empty()) {
            return -1;
        }
        return activateIndex(ssel_ < 0 ? 0
                             : ssel_ == 0 ? snapshotCount() - 1
                                          : ssel_ - 1);
    }

    // Add a snapshot at the end. The active history entry and snapshot stay
    // as they are. On failure returns false and sets `err`.
    bool addSnapshot(const std::string &name, const Params &params,
                     std::string &err)
    {
        if (name.empty()) {
            err = "empty snapshot name";
            return false;
        }
        if (findSnapshot(name) >= 0) {
            err = "a snapshot named \"" + name + "\" already exists";
            return false;
        }
        snapshots_.push_back(Snapshot{name, params});
        return true;
    }

    // The snapshot button of the GUI: name it "<prefix> N" with a counter that
    // only goes up (see resetSnapshotCounter()) skipping the names in use, and
    // take the parameters of the selected history entry. Does nothing and
    // returns false if no entry is selected.
    bool addSnapshotFromSelection(const std::string &prefix, std::string &name)
    {
        if (hsel_ < 0) {
            return false;
        }
        do {
            name = prefix + " " + std::to_string(counter_++);
        } while (findSnapshot(name) >= 0);
        snapshots_.push_back(Snapshot{name, entries_[hsel_].params});
        return true;
    }

    void resetSnapshotCounter() { counter_ = 1; }

    // Remove a snapshot. If it was the active one the selection goes back to
    // the last history entry (as the delete button does).
    bool removeSnapshot(const std::string &name, std::string &err)
    {
        int i = findSnapshot(name);
        if (i < 0) {
            err = "no snapshot named \"" + name + "\"";
            return false;
        }
        snapshots_.erase(snapshots_.begin() + i);
        if (i == ssel_) {
            ssel_ = -1;
            if (!entries_.empty()) {
                hsel_ = size() - 1;
            }
        } else if (i < ssel_) {
            --ssel_;
        }
        return true;
    }

    // Replace the parameters of a snapshot; the selection is not touched.
    bool overwriteSnapshot(const std::string &name, const Params &params,
                           std::string &err)
    {
        int i = findSnapshot(name);
        if (i < 0) {
            err = "no snapshot named \"" + name + "\"";
            return false;
        }
        snapshots_[i].params = params;
        return true;
    }

    bool renameSnapshot(const std::string &from, const std::string &to,
                        std::string &err)
    {
        int i = findSnapshot(from);
        if (i < 0) {
            err = "no snapshot named \"" + from + "\"";
            return false;
        }
        if (to.empty()) {
            err = "empty snapshot name";
            return false;
        }
        if (to != from && findSnapshot(to) >= 0) {
            err = "a snapshot named \"" + to + "\" already exists";
            return false;
        }
        snapshots_[i].name = to;
        return true;
    }

private:
    int activateIndex(int i)
    {
        ssel_ = i;
        hsel_ = -1;
        return i;
    }

    std::vector<Entry> entries_;
    std::vector<Snapshot> snapshots_;
    int hsel_ = -1;
    int ssel_ = -1;
    int counter_ = 1;
};

}}} // namespace art::engine::remote
