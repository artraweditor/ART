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

#include "../procparams.h"
#include "remote_reply.h"
#include <functional>
#include <glibmm/ustring.h>
#include <memory>
#include <string>
#include <vector>

namespace art { namespace engine { namespace remote {

using Args = std::vector<Glib::ustring>;
using ReplyPtr = std::shared_ptr<Reply>;

struct HistoryEntryInfo {
    Glib::ustring text;
    Glib::ustring value;
};

// a white balance: temperature and tint ("green"), and the equivalent
// multipliers of the red, green and blue channels (in any overall scale)
struct SpotWB {
    double temperature = 0;
    double tint = 0;
    double red = 0;
    double green = 0;
    double blue = 0;
};

// A validated `export` command: the output format and its options, with the
// defaults of the format filled in
struct ExportRequest {
    Glib::ustring outfile;   // absolute
    std::string format;      // "jpg", "tif", "png" or an extra format
    int quality = 92;        // jpg
    int subsampling = 3;     // jpg: 1 = best compression, 3 = best quality
    int bits = 8;            // per channel
    bool isFloat = false;    // tif
    bool uncompressed = true; // tif
    bool fast = false;       // the fast export pipeline
    bool overwrite = false;  // replace an existing file
};

// Completion of a command that writes an image: on success `msg` is the file
// written and `w` x `h` its size (-1 x -1 if the image is only queued for
// export: it is not written yet); on failure `msg` is the error message
using ImageDone =
    std::function<void(bool ok, const Glib::ustring &msg, int w, int h)>;

// The active image, as seen by the commands that operate on it. Implemented by
// the GUI (an adaptor over EditorPanel/History) and by the ART-cli server.
class CommandTarget {
public:
    virtual ~CommandTarget() = default;

    // the current processing parameters, as the text of an .arp file
    virtual bool getParamsText(Glib::ustring &out) = 0;
    // `key: value` lines: file, sizes, camera and exposure metadata
    virtual bool getInfoText(Glib::ustring &out) = 0;
    virtual bool getCurrentParams(procparams::ProcParams &pp) = 0;
    // the metadata of the image as a JSON object (see metadata_json.h);
    // `makernotes`: 1 = include the ExifTool maker notes, 0 = not, -1 = as the
    // preferences say; `edits`: apply the metadata edits of the parameters.
    // On failure sets `err`
    virtual bool getMetadata(int makernotes, bool edits, json::Value &out,
                             Glib::ustring &err) = 0;
    // the default profile of the preferences for this image (the one for raw
    // or for non-raw images, possibly the dynamic profile); null if it cannot
    // be loaded
    virtual std::unique_ptr<procparams::PartialProfile> getDefaultProfile() = 0;
    // apply a profile as a new (undoable) history entry
    virtual bool applyProfile(const procparams::PartialProfile &pp,
                              const Glib::ustring &descr) = 0;
    // render the window (cx, cy, cw, ch) of the full image (full-image
    // coordinates; cw <= 0 means the whole image) scaled by `scale` to a file
    // (format by extension). done() may be called later
    virtual void renderPreview(const Glib::ustring &outfile, double scale,
                               int cx, int cy, int cw, int ch,
                               ImageDone done) = 0;

    // size of the full (processed) image, the reference of the coordinates of
    // getSpotWB() and renderPreview()
    virtual bool getFullSize(int &w, int &h) = 0;

    // Render the image with the current parameters at full size, as ART-cli
    // would, and write it as the request says. done() receives the outcome
    // and the message to report; it may be called later. Not available by
    // default.
    virtual void exportImage(const ExportRequest &req, ImageDone done)
    {
        (void)req;
        done(false, "export is not available in this mode", 0, 0);
    }
    // The white balance that makes the rectangle (x, y, w, h), in full-image
    // coordinates, neutral. The current parameters are not changed.
    virtual bool getSpotWB(int x, int y, int w, int h, SpotWB &out) = 0;

    virtual std::vector<HistoryEntryInfo> getHistoryEntries() = 0;
    // index of the selected history row, -1 if none (e.g. a snapshot is active)
    virtual int getHistoryIndex() = 0;
    virtual bool selectHistoryIndex(int index) = 0;
    virtual std::vector<Glib::ustring> getSnapshotNames() = 0;
    // index of the active snapshot, -1 if none
    virtual int getSnapshotIndex() = 0;
    virtual bool activateSnapshot(const Glib::ustring &name) = 0;
    // these three return false and set `err` on failure
    virtual bool addSnapshot(const Glib::ustring &name,
                             const procparams::ProcParams &params,
                             Glib::ustring &err) = 0;
    virtual bool removeSnapshot(const Glib::ustring &name,
                                Glib::ustring &err) = 0;
    virtual bool overwriteSnapshot(const Glib::ustring &name,
                                   const procparams::ProcParams &params,
                                   Glib::ustring &err) = 0;
};


// What the shared verbs need from whoever hosts them.
class CommandHost {
public:
    virtual ~CommandHost() = default;
    // the active image, nullptr if none is open
    virtual CommandTarget *current() = 0;
    // Run fn(target) as soon as the active image has finished loading and
    // processing; give up after 60 s, reporting "timeout waiting for the
    // editor" through `reply`. Exceptions thrown by fn must be reported
    // through `reply` (as Reply::fail) by the host.
    virtual void whenReady(const ReplyPtr &reply,
                           std::function<void(CommandTarget &)> fn) = 0;
};


struct Context {
    CommandHost &host;
    ReplyPtr reply;
    std::string cwd; // of the client, for relative paths
};

using VerbFn = std::function<void(Context &, Args &)>;


// Verb table. A verb reports its outcome through `reply` (possibly later, for
// asynchronous commands); an exception thrown synchronously is reported as a
// failure.
class Dispatcher {
public:
    // `help_hint` completes the "missing command" error
    explicit Dispatcher(std::string help_hint = "try: ART --cmd help");

    void add(const std::string &name, VerbFn fn);
    void run(CommandHost &host, const ReplyPtr &reply, const std::string &cwd,
             const Args &args) const;

private:
    struct Verb {
        std::string name;
        VerbFn fn;
    };
    std::string help_hint_;
    std::vector<Verb> verbs_;
};

// the commands that operate on the active image: apply, get-params, get-info,
// get-history, undo, redo, history-goto, get-snapshots, snapshot-add,
// snapshot-activate, snapshot-overwrite, snapshot-remove, preview, get-spot-wb
void registerActiveImageVerbs(Dispatcher &d);


// helpers for verb implementations

// `f` made absolute (and normalised) against `cwd`
Glib::ustring abspath(const Glib::ustring &f, const std::string &cwd);
// removes the option `name` from args; true if present
bool take_flag(Args &args, const Glib::ustring &name);
// removes the option `name <value>` from args; false if not present, throws
// if the value is missing
bool take_option(Args &args, const Glib::ustring &name, Glib::ustring &value);
bool parse_int(const Glib::ustring &s, int &out);
bool parse_double(const Glib::ustring &s, double &out);

}}} // namespace art::engine::remote
