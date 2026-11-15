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

// Unit tests for the history/snapshot model (no engine, no GUI needed):
// ctest -R history-model

#include "history_model.h"

#include <iostream>

using namespace art::engine;
using namespace art::engine::remote;

namespace {

int failures = 0;

#define CHECK(cond)                                                           \
    do {                                                                      \
        if (!(cond)) {                                                        \
            ++failures;                                                       \
            std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: "    \
                      << #cond << std::endl;                                  \
        }                                                                     \
    } while (0)

// the parameters are just an int here
using Model = BasicHistoryModel<int>;

// events used below (any codes would do)
const ProcEvent EvA = ProcEvent(static_cast<int>(EvLEnabled));
const ProcEvent EvB = ProcEvent(static_cast<int>(EvLEnabled) - 1);

Model loaded()
{
    Model m;
    CHECK(m.procParamsChanged(0, EvPhotoLoaded, "Loaded", ""));
    return m;
}

void test_ignored_events()
{
    Model m = loaded();
    CHECK(!m.procParamsChanged(1, EvHistoryBrowsed, "x", "y"));
    CHECK(!m.procParamsChanged(1, EvMonitorTransform, "x", "y"));
    CHECK(!m.procParamsChanged(1, EvA, "", ""));
    CHECK(m.size() == 1);
    // an event with only a description is recorded
    CHECK(m.procParamsChanged(1, EvA, "", "d"));
    CHECK(m.size() == 2);
}

void test_coalescing()
{
    Model m = loaded();
    m.procParamsChanged(1, EvA, "A", "1");
    m.procParamsChanged(2, EvA, "A", "2"); // same event: updated in place
    CHECK(m.size() == 2);
    CHECK(m.entry(1).params == 2 && m.entry(1).value == "2");
    m.procParamsChanged(3, EvB, "B", "3"); // different event: new entry
    CHECK(m.size() == 3);
    m.procParamsChanged(4, EvProfileChanged, "P", "p");
    m.procParamsChanged(5, EvProfileChanged, "P", "p"); // never coalesced
    CHECK(m.size() == 5);
    CHECK(m.selectedEntry() == 4);
    CHECK(*m.lastParams() == 5);
}

void test_truncate_after_selection()
{
    Model m = loaded();
    m.procParamsChanged(1, EvA, "A", "");
    m.procParamsChanged(2, EvB, "B", "");
    m.procParamsChanged(3, EvProfileChanged, "P", "");
    CHECK(m.selectEntry(1));
    // a new change drops what came after the selected entry
    m.procParamsChanged(9, EvB, "B", "");
    CHECK(m.size() == 3);
    CHECK(m.entry(2).params == 9);
    CHECK(m.selectedEntry() == 2);
    // ... and coalesces with the new last entry if it has the same event:
    // entry 1 is EvA, so a change of EvA after selecting it updates it
    CHECK(m.selectEntry(1));
    m.procParamsChanged(7, EvA, "A", "");
    CHECK(m.size() == 2);
    CHECK(m.entry(1).params == 7);
}

void test_selection_and_undo_redo()
{
    Model m = loaded();
    m.procParamsChanged(1, EvA, "A", "");
    m.procParamsChanged(2, EvB, "B", "");
    CHECK(m.selectedEntry() == 2);
    CHECK(!m.selectEntry(3) && !m.selectEntry(-1));
    CHECK(m.selectedEntry() == 2);
    CHECK(m.undo() && m.selectedEntry() == 1);
    CHECK(m.undo() && m.selectedEntry() == 0);
    CHECK(!m.undo() && m.selectedEntry() == 0);
    CHECK(m.redo() && m.redo() && m.selectedEntry() == 2);
    CHECK(!m.redo() && m.selectedEntry() == 2);

    // with a snapshot active both go to the second to last entry
    std::string err;
    CHECK(m.addSnapshot("s", 5, err));
    CHECK(m.activateSnapshot("s"));
    CHECK(m.selectedEntry() == -1 && m.activeSnapshot() == 0);
    CHECK(m.undo() && m.selectedEntry() == 1 && m.activeSnapshot() == -1);
    CHECK(m.activateSnapshot("s"));
    CHECK(m.redo() && m.selectedEntry() == 1);
}

void test_snapshots()
{
    Model m = loaded();
    m.procParamsChanged(1, EvA, "A", "");
    std::string err;
    CHECK(!m.addSnapshot("", 0, err) && err == "empty snapshot name");
    CHECK(m.addSnapshot("one", 10, err));
    CHECK(m.addSnapshot("two", 20, err));
    CHECK(!m.addSnapshot("one", 0, err));
    CHECK(err == "a snapshot named \"one\" already exists");
    // adding does not change the selection
    CHECK(m.selectedEntry() == 1 && m.activeSnapshot() == -1);

    CHECK(m.activateSnapshot("two") && m.activeSnapshot() == 1);
    CHECK(!m.activateSnapshot("nope"));
    CHECK(m.activeSnapshot() == 1);

    // activating a snapshot records its parameters as an unselected entry
    // at the end, and a second activation updates that entry in place
    m.procParamsChanged(m.snapshot(1).params, EvBookmarkSelected, "Snap", "two");
    CHECK(m.size() == 3 && m.selectedEntry() == -1 && m.activeSnapshot() == 1);
    CHECK(m.activateSnapshot("one"));
    m.procParamsChanged(m.snapshot(0).params, EvBookmarkSelected, "Snap", "one");
    CHECK(m.size() == 3 && m.entry(2).params == 10);
    CHECK(m.selectedEntry() == -1 && m.activeSnapshot() == 0);

    // a real change deselects the snapshot
    m.procParamsChanged(5, EvB, "B", "");
    CHECK(m.activeSnapshot() == -1 && m.selectedEntry() == m.size() - 1);

    CHECK(m.overwriteSnapshot("one", 11, err) && m.snapshot(0).params == 11);
    CHECK(!m.overwriteSnapshot("nope", 0, err));
}

void test_remove_snapshot()
{
    Model m = loaded();
    m.procParamsChanged(1, EvA, "A", "");
    m.procParamsChanged(2, EvB, "B", "");
    std::string err;
    m.addSnapshot("a", 1, err);
    m.addSnapshot("b", 2, err);
    m.addSnapshot("c", 3, err);

    // removing one before the active snapshot keeps the active one
    m.activateSnapshot("c");
    CHECK(m.removeSnapshot("a", err));
    CHECK(m.activeSnapshot() == 1 && m.snapshot(1).name == "c");
    // removing one after it doesn't move it
    CHECK(m.removeSnapshot("b", err) && m.activeSnapshot() == 0);
    // removing the active one goes back to the last history entry
    CHECK(m.removeSnapshot("c", err));
    CHECK(m.activeSnapshot() == -1 && m.selectedEntry() == 2);
    CHECK(!m.removeSnapshot("c", err) && err == "no snapshot named \"c\"");
}

void test_auto_name_and_rename()
{
    Model m = loaded();
    std::string err, name;
    // needs a selected entry
    Model empty;
    CHECK(!empty.addSnapshotFromSelection("Snapshot", name));
    m.procParamsChanged(4, EvA, "A", "");
    CHECK(m.addSnapshotFromSelection("Snapshot", name) && name == "Snapshot 1");
    CHECK(m.snapshot(0).params == 4);
    // the counter goes up, skipping names in use
    CHECK(m.addSnapshot("Snapshot 2", 0, err));
    CHECK(m.addSnapshotFromSelection("Snapshot", name) && name == "Snapshot 3");
    // it does not go back after a removal ...
    CHECK(m.removeSnapshot("Snapshot 3", err));
    CHECK(m.addSnapshotFromSelection("Snapshot", name) && name == "Snapshot 4");
    // ... unless reset
    m.resetSnapshotCounter();
    CHECK(m.addSnapshotFromSelection("Snapshot", name) && name == "Snapshot 3");

    CHECK(m.renameSnapshot("Snapshot 1", "first", err));
    CHECK(m.findSnapshot("first") == 0 && m.findSnapshot("Snapshot 1") < 0);
    CHECK(!m.renameSnapshot("first", "Snapshot 2", err));
    CHECK(!m.renameSnapshot("first", "", err));
    CHECK(!m.renameSnapshot("nope", "x", err));
    CHECK(m.renameSnapshot("first", "first", err)); // to itself: fine
}

void test_deselect_snapshot()
{
    Model m = loaded();
    std::string err;
    m.addSnapshot("a", 0, err);
    CHECK(m.activateSnapshot("a") && m.selectedEntry() == -1);
    m.deselectSnapshot();
    // nothing is selected now, and the history selection is not restored
    CHECK(m.activeSnapshot() == -1 && m.selectedEntry() == -1);
    CHECK(m.selectEntry(0) && m.activeSnapshot() == -1);
}

void test_next_prev_snapshot()
{
    Model m = loaded();
    CHECK(m.activateNextSnapshot() == -1 && m.activatePrevSnapshot() == -1);
    std::string err;
    m.addSnapshot("a", 0, err);
    m.addSnapshot("b", 0, err);
    m.addSnapshot("c", 0, err);
    CHECK(m.activateNextSnapshot() == 0);
    CHECK(m.activateNextSnapshot() == 1);
    CHECK(m.activateNextSnapshot() == 2);
    CHECK(m.activateNextSnapshot() == 0); // wraps
    CHECK(m.activatePrevSnapshot() == 2); // wraps
    CHECK(m.activatePrevSnapshot() == 1);
    CHECK(m.selectedEntry() == -1);
}

void test_photo_loaded_clears()
{
    Model m = loaded();
    m.procParamsChanged(1, EvA, "A", "");
    std::string err;
    m.addSnapshot("s", 1, err);
    m.activateSnapshot("s");
    m.procParamsChanged(0, EvPhotoLoaded, "Loaded", "");
    CHECK(m.size() == 1 && m.snapshotCount() == 0);
    CHECK(m.selectedEntry() == 0 && m.activeSnapshot() == -1);

    // setSnapshots replaces the list and deselects
    std::vector<Model::Snapshot> v;
    v.push_back({"x", 1});
    v.push_back({"y", 2});
    m.setSnapshots(v);
    CHECK(m.snapshotNames() == (std::vector<std::string>{"x", "y"}));
    CHECK(m.activeSnapshot() == -1 && m.selectedEntry() == 0);
}

} // namespace

int main()
{
    test_ignored_events();
    test_coalescing();
    test_truncate_after_selection();
    test_selection_and_undo_redo();
    test_snapshots();
    test_remove_snapshot();
    test_auto_name_and_rename();
    test_deselect_snapshot();
    test_next_prev_snapshot();
    test_photo_loaded_clears();
    if (failures) {
        std::cerr << failures << " check(s) failed" << std::endl;
        return 1;
    }
    std::cout << "history model: all checks passed" << std::endl;
    return 0;
}
