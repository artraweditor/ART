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

#include "../engine/procparams.h"
#include "../engine/remote/command_dispatch.h"
#include "../engine/remote/history_model.h"
#include "../engine/rtengine.h"
#include <glibmm/ustring.h>
#include <memory>
#include <string>

namespace art { namespace gui {

// The "active image" of the ART-cli server: a headless counterpart of what
// EditorPanel + History + ToolPanelCoordinator do for the image open in the
// GUI. It owns the image processor, the edit history and the snapshots, and
// implements the commands' view of an image (CommandTarget).
//
// Not thread safe: all calls must come from one thread (the server's command
// executor). The engine's own threads only touch the processor.
//
// Differences from the GUI that are deliberate:
//  - the sidecar (<file>.arp, with its snapshots) is read when the image is
//    opened, and only written by save(): nothing is ever saved implicitly;
//  - no cache directory is consulted, no Crop tool trims an overflowing crop
//    after a profile is applied.
class EditSession final: public art::engine::remote::CommandTarget {
public:
    EditSession();
    ~EditSession() override;

    // Load `filename` (an absolute path) and make it the active image,
    // replacing the previous one (unsaved edits are dropped). Starts the
    // first processing run in the background. Throws std::runtime_error.
    void open(const Glib::ustring &filename);
    // forget the active image (unsaved edits are dropped)
    void close();

    bool isOpen() const { return bool(ipc_); }
    const Glib::ustring &fileName() const { return fname_; }
    // true if an image is loaded and the processing pipeline is idle
    bool isReady() const;
    // wait (polling) until isReady(), at most `timeout_ms`; false if it timed
    // out or no image is open
    bool waitReady(int timeout_ms) const;
    // the parameters or snapshots changed since the image was opened/saved
    bool isDirty() const { return dirty_; }

    // Write the master parameters and the snapshots to the sidecar file
    // (replacing it). On failure returns false and sets `err`.
    bool save(Glib::ustring &err);
    // path of the sidecar of the active image
    const Glib::ustring &sidecarFile() const { return sidecar_; }

    // ---- CommandTarget ----------------------------------------------------
    bool getParamsText(Glib::ustring &out) override;
    bool getInfoText(Glib::ustring &out) override;
    bool getCurrentParams(art::engine::procparams::ProcParams &pp) override;
    bool getMetadata(int makernotes, bool edits,
                     art::engine::remote::json::Value &out,
                     Glib::ustring &err) override;
    bool applyProfile(const art::engine::procparams::PartialProfile &pp,
                      const Glib::ustring &descr) override;
    std::unique_ptr<art::engine::procparams::PartialProfile>
    getDefaultProfile() override;
    // synchronous: done() is called before this returns (after waiting for
    // the render, 60 s at most)
    void renderPreview(
        const Glib::ustring &outfile, double scale, int cx, int cy, int cw,
        int ch, std::function<void(bool, const Glib::ustring &, int, int)> done) override;

    bool getFullSize(int &w, int &h) override;
    // synchronous: done() is called before this returns. The image is
    // rendered from the file (not from the editing pipeline), as ART-cli
    // does, with the current parameters, including unsaved edits
    void exportImage(
        const art::engine::remote::ExportRequest &req,
        std::function<void(bool, const Glib::ustring &, int, int)> done) override;
    bool getSpotWB(int x, int y, int w, int h,
                   art::engine::remote::SpotWB &out) override;
    std::vector<art::engine::remote::HistoryEntryInfo>
    getHistoryEntries() override;
    int getHistoryIndex() override;
    bool selectHistoryIndex(int index) override;
    std::vector<Glib::ustring> getSnapshotNames() override;
    int getSnapshotIndex() override;
    bool activateSnapshot(const Glib::ustring &name) override;
    bool addSnapshot(const Glib::ustring &name,
                     const art::engine::procparams::ProcParams &params,
                     Glib::ustring &err) override;
    bool removeSnapshot(const Glib::ustring &name, Glib::ustring &err) override;
    bool overwriteSnapshot(const Glib::ustring &name,
                           const art::engine::procparams::ProcParams &params,
                           Glib::ustring &err) override;

private:
    using Model =
        art::engine::remote::BasicHistoryModel<art::engine::procparams::ProcParams>;

    // merge `pp` into the parameters of the processor and start a processing
    // run (ToolPanelCoordinator::profileChange); returns the new parameters
    art::engine::procparams::ProcParams
    changeParams(const art::engine::procparams::PartialProfile &pp,
                 art::engine::ProcEvent ev);
    // add the change to the history
    void record(const art::engine::procparams::ProcParams &params,
                art::engine::ProcEvent ev, const Glib::ustring &descr);

    std::shared_ptr<art::engine::StagedImageProcessor> ipc_;
    Model model_;
    Glib::ustring fname_;
    Glib::ustring sidecar_;
    bool is_raw_;
    bool dirty_;
};

}} // namespace art::gui
