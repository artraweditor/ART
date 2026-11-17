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

#include "editsession.h"
#include "../engine/image8.h"
#include "../engine/imgiomanager.h"
#include "../engine/improccoordinator.h"
#include "../engine/profilestore.h"
#include "../engine/remote/metadata_json.h"
#include "../engine/refreshmap.h"
#include "../engine/threadpool.h"
#include "../utils/multilangmgr.h"
#include "../utils/pathutils.h"
#include "eventmapper.h"
#include "exporter.h"
#include "fastexport.h"
#include "options.h"
#include <algorithm>
#include <glib/gstdio.h>
#include <chrono>
#include <cmath>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace art { namespace gui {

namespace pp = art::engine::procparams;
namespace eng = art::engine;

namespace {

// receives the rendered window of a preview
class PreviewCatcher: public eng::DetailedCropListener {
public:
    PreviewCatcher(int cx, int cy, int cw, int ch, int skip):
        cx_(cx), cy_(cy), cw_(cw), ch_(ch), skip_(skip), ready_(false),
        w_(0), h_(0)
    {
    }

    void getWindow(int &cx, int &cy, int &cw, int &ch, int &skip) override
    {
        cx = cx_;
        cy = cy_;
        cw = cw_;
        ch = ch_;
        skip = skip_;
    }

    void setDetailedCrop(eng::IImage8 *img, eng::IImage8 *imgtrue,
                         const pp::ColorManagementParams &cmp,
                         const pp::CropParams &cp, int cx, int cy, int cw,
                         int ch, int skip) override
    {
        // called from a worker thread: copy the (non-shared) data
        if (cx != cx_ || cy != cy_ || cw != cw_ || ch != ch_ ||
            skip != skip_ || !imgtrue) {
            return;
        }
        std::lock_guard<std::mutex> lck(mtx_);
        w_ = imgtrue->getWidth();
        h_ = imgtrue->getHeight();
        data_.assign(imgtrue->getData(), imgtrue->getData() + 3 * w_ * h_);
        ready_ = true;
    }

    bool take(std::vector<unsigned char> &data, int &w, int &h)
    {
        std::lock_guard<std::mutex> lck(mtx_);
        if (!ready_) {
            return false;
        }
        data.swap(data_);
        w = w_;
        h = h_;
        return true;
    }

private:
    int cx_, cy_, cw_, ch_, skip_;
    std::mutex mtx_;
    bool ready_;
    int w_, h_;
    std::vector<unsigned char> data_;
};

} // namespace


EditSession::EditSession(): is_raw_(false), dirty_(false) {}

EditSession::~EditSession() { close(); }


void EditSession::close()
{
    if (ipc_) {
        ipc_->stopProcessing();
        // the processor releases the initial image
        ipc_.reset();
    }
    model_.clear();
    fname_.clear();
    sidecar_.clear();
    dirty_ = false;
}


void EditSession::open(const Glib::ustring &filename)
{
    close();

    // as ART-cli decides whether a file is a raw
    is_raw_ = true;
    const Glib::ustring ext = utils::getExtension(filename).lowercase();
    if (ext == "jpg" || ext == "jpeg" || ext == "tif" || ext == "tiff" ||
        ext == "png" || eng::ImageIOManager::getInstance()->canLoad(ext)) {
        is_raw_ = false;
    }

    int err = 0;
    eng::InitialImage *ii =
        eng::InitialImage::load(filename, is_raw_, &err, nullptr);
    if (!ii) {
        throw std::runtime_error("cannot load the image: " + filename);
    }

    try {
        ipc_.reset(eng::StagedImageProcessor::create(ii));
        ipc_->setPreviewScale(10); // as the editor does
        fname_ = filename;
        sidecar_ = options.getParamFile(filename);

        // the sidecar, if there is one, else the default profile
        pp::ProcParamsWithSnapshots saved;
        const bool have_sidecar =
            Glib::file_test(sidecar_, Glib::FILE_TEST_EXISTS) &&
            saved.load(nullptr, sidecar_) == 0;
        std::unique_ptr<pp::PartialProfile> start;
        if (have_sidecar) {
            start.reset(new pp::FullPartialProfile(saved.master));
        } else {
            start = options.loadDefaultProfile(is_raw_, ii->getMetaData());
        }

        auto params = changeParams(*start, eng::EvPhotoLoaded);
        record(params, eng::EvPhotoLoaded,
               Glib::ustring(have_sidecar ? Glib::path_get_basename(sidecar_)
                                          : std::string("(default)")));
        // history.setSnapshots(): after the EvPhotoLoaded entry, which clears
        std::vector<Model::Snapshot> snaps;
        if (have_sidecar) {
            for (auto &s : saved.snapshots) {
                snaps.push_back({s.first.raw(), s.second});
            }
        }
        model_.setSnapshots(std::move(snaps));
        model_.resetSnapshotCounter();
        dirty_ = false;
    } catch (...) {
        close();
        throw;
    }
}


bool EditSession::isReady() const
{
    return ipc_ &&
           !static_cast<const eng::ImProcCoordinator *>(ipc_.get())->is_running();
}


bool EditSession::waitReady(int timeout_ms) const
{
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeout_ms);
    while (ipc_) {
        if (isReady()) {
            return true;
        }
        if (std::chrono::steady_clock::now() >= deadline) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return false;
}


bool EditSession::save(Glib::ustring &err)
{
    if (!ipc_) {
        err = "no image is open";
        return false;
    }
    pp::ProcParamsWithSnapshots out;
    ipc_->getParams(&out.master);
    for (auto &s : model_.snapshots()) {
        out.snapshots.emplace_back(s.name, s.params);
    }
    if (out.save(nullptr, sidecar_) != 0) {
        err = "cannot write " + sidecar_;
        return false;
    }
    dirty_ = false;
    return true;
}


pp::ProcParams EditSession::changeParams(const pp::PartialProfile &nparams,
                                         eng::ProcEvent ev)
{
    // see ToolPanelCoordinator::profileChange
    pp::ProcParams *params = ipc_->beginUpdateParams();
    pp::ProcParams merged(*params);

    // reset IPTC values when switching procparams from the history
    if (ev == eng::EvHistoryBrowsed) {
        merged.metadata.iptc.clear();
        merged.metadata.exif.clear();
    }
    nparams.applyTo(merged);

    // do not rerender the raw if nothing that affects it has changed
    bool filter_raw_refresh = false;
    if (ev != eng::EvPhotoLoaded) {
        filter_raw_refresh = (params->raw == merged.raw) &&
                             (params->lensProf == merged.lensProf) &&
                             (params->filmNegative == merged.filmNegative) &&
                             (params->wb == merged.wb);
    }
    *params = merged;

    if (filter_raw_refresh) {
        ipc_->endUpdateParams(eng::RefreshMapper::getInstance()->getAction(ev) &
                              eng::ALLNORAW);
    } else {
        ipc_->endUpdateParams(ev);
    }
    return merged;
}


void EditSession::record(const pp::ProcParams &params, eng::ProcEvent ev,
                         const Glib::ustring &descr, const std::string &merge_key)
{
    const Glib::ustring text =
        M(ProcEventMapper::getInstance()->getHistoryMsg(ev));
    model_.procParamsChanged(params, ev, text.raw(), descr.raw(), merge_key);
}


bool EditSession::getParamsText(Glib::ustring &out)
{
    if (!ipc_) {
        return false;
    }
    pp::ProcParams params;
    ipc_->getParams(&params);
    pp::KeyFile kf;
    if (params.save(nullptr, kf, nullptr, "") != 0) {
        return false;
    }
    out = kf.to_data();
    return true;
}


bool EditSession::getInfoText(Glib::ustring &out)
{
    if (!ipc_) {
        return false;
    }
    auto md = ipc_->getInitialImage()->getMetaData();
    int sw = 0, sh = 0;
    ipc_->getInitialImage()->getImageSource()->getFullSize(sw, sh);
    out = Glib::ustring::compose("file: %1\n", fname_);
    out += Glib::ustring::compose("is_raw: %1\n", is_raw_ ? "true" : "false");
    out += Glib::ustring::compose("source_width: %1\nsource_height: %2\n", sw, sh);
    out += Glib::ustring::compose("full_width: %1\nfull_height: %2\n",
                                  ipc_->getFullWidth(), ipc_->getFullHeight());
    if (md) {
        out += Glib::ustring::compose("make: %1\nmodel: %2\nlens: %3\n",
                                      md->getMake(), md->getModel(),
                                      md->getLens());
        out += Glib::ustring::compose(
            "iso: %1\nf_number: %2\nfocal_length_mm: %3\nshutter_s: %4\n"
            "exposure_compensation: %5\norientation: %6\n",
            md->getISOSpeed(), md->getFNumber(), md->getFocalLen(),
            md->getShutterSpeed(), md->getExpComp(), md->getOrientation());
    }
    return true;
}


bool EditSession::getCurrentParams(pp::ProcParams &params)
{
    if (!ipc_) {
        return false;
    }
    ipc_->getParams(&params);
    return true;
}


bool EditSession::getMetadata(int makernotes, bool edits,
                              art::engine::remote::json::Value &out,
                              Glib::ustring &err)
{
    if (!ipc_) {
        err = "no image is open in the editor";
        return false;
    }
    pp::ProcParams params;
    ipc_->getParams(&params);
    try {
        out = art::engine::remote::metadataToJson(
            fname_, ipc_->getInitialImage()->getMetaData(), params, edits,
            makernotes < 0 ? options.show_exiftool_makernotes : makernotes > 0);
    } catch (std::exception &e) {
        err = e.what();
        return false;
    }
    return true;
}


std::unique_ptr<pp::PartialProfile> EditSession::getDefaultProfile()
{
    if (!ipc_) {
        return nullptr;
    }
    return options.loadDefaultProfile(is_raw_,
                                      ipc_->getInitialImage()->getMetaData());
}


bool EditSession::applyProfile(const pp::PartialProfile &profile,
                               const Glib::ustring &descr)
{
    if (!isReady()) {
        return false;
    }
    auto params = changeParams(profile, eng::EvProfileChanged);
    record(params, eng::EvProfileChanged, descr);
    dirty_ = true;
    return true;
}


bool EditSession::applyProfileMerged(const pp::PartialProfile &profile,
                                     const Glib::ustring &descr,
                                     const std::string &merge_key)
{
    if (!isReady()) {
        return false;
    }
    auto params = changeParams(profile, eng::EvProfileChanged);
    record(params, eng::EvProfileChanged, descr, merge_key);
    dirty_ = true;
    return true;
}


void EditSession::renderPreview(
    const Glib::ustring &outfile, double scale, int cx, int cy, int cw, int ch,
    std::function<void(bool, const Glib::ustring &, int, int)> done)
{
    if (!isReady()) {
        done(false, "no image ready in the editor", 0, 0);
        return;
    }
    const int fw = ipc_->getFullWidth();
    const int fh = ipc_->getFullHeight();
    if (cw <= 0 || ch <= 0) {
        cx = cy = 0;
        cw = fw;
        ch = fh;
    }
    if (cx < 0 || cy < 0 || cx + cw > fw || cy + ch > fh) {
        done(false, Glib::ustring::compose(
                        "crop out of bounds (full image is %1x%2)", fw, fh), 0, 0);
        return;
    }
    if (!(scale > 0)) {
        done(false, "invalid scale", 0, 0);
        return;
    }

    // the engine can only downscale by an integer factor: use the largest one
    // not exceeding 1/scale, and resample the result to the exact size
    const int skip = scale >= 1.0 ? 1 : std::max(1, int(std::floor(1.0 / scale)));

    auto ipc = ipc_; // keep the processor alive for the worker
    eng::DetailedCrop *crop = ipc->createCrop(nullptr, true);
    PreviewCatcher catcher(cx, cy, cw, ch, skip);
    crop->setListener(&catcher);
    if (crop->tryUpdate()) {
        eng::ThreadPool::add_task(eng::ThreadPool::Priority::HIGH,
                                  [ipc, crop]() { crop->fullUpdate(); });
    }

    std::vector<unsigned char> buf;
    int w = 0, h = 0;
    bool got = false;
    for (int ticks = 0; ticks < 3000 && !got; ++ticks) { // 60 s
        got = catcher.take(buf, w, h);
        if (!got) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
    crop->setListener(nullptr);
    crop->destroy();
    if (!got) {
        done(false, "timeout while rendering the preview", 0, 0);
        return;
    }

    // the engine output covers cw/skip pixels; bring it to the requested scale
    const int ow = std::max(1, int(std::lround(cw * scale)));
    const int oh = std::max(1, int(std::lround(ch * scale)));
    eng::Image8 img(w, h);
    std::memcpy(img.data, buf.data(), size_t(3) * w * h);
    std::unique_ptr<eng::Image8> scaled;
    eng::Image8 *out = &img;
    if (ow != w || oh != h) {
        scaled.reset(new eng::Image8(ow, oh));
        img.resizeImgTo(ow, oh, eng::TI_Bilinear, scaled.get());
        out = scaled.get();
    }
    const auto dot = outfile.rfind('.');
    const Glib::ustring e =
        outfile.substr(dot == Glib::ustring::npos ? outfile.size() : dot + 1)
            .lowercase();
    const int rc = (e == "jpg" || e == "jpeg") ? out->saveAsJPEG(outfile, 90)
                                               : out->saveAsPNG(outfile, 8);
    if (rc != 0) {
        done(false, "cannot write " + outfile, 0, 0);
        return;
    }
    done(true, outfile, ow, oh);
}


void EditSession::exportImage(
    const art::engine::remote::ExportRequest &req,
    std::function<void(bool, const Glib::ustring &, int, int)> done)
{
    if (!ipc_) {
        done(false, "no image is open in the editor", 0, 0);
        return;
    }
    if (req.outfile == fname_) {
        done(false, "cannot overwrite the input file: " + req.outfile, 0, 0);
        return;
    }
    if (!req.overwrite && Glib::file_test(req.outfile, Glib::FILE_TEST_EXISTS)) {
        done(false, req.outfile + " already exists (use --overwrite)", 0, 0);
        return;
    }

    // fail before the (long) rendering if the file cannot be written
    const Glib::ustring dir = Glib::path_get_dirname(req.outfile);
    if (!Glib::file_test(dir, Glib::FILE_TEST_IS_DIR)) {
        done(false, "no such directory: " + dir, 0, 0);
        return;
    }

    ExportOptions opts;
    opts.format = req.format;
    opts.jpegQuality = req.quality;
    opts.jpegSubSamp = req.subsampling;
    opts.bits = req.bits;
    opts.isFloat = req.isFloat;
    opts.tiffUncompressed = req.uncompressed;
    opts.fast = req.fast;
    normalizeExportOptions(opts);

    pp::ProcParams params;
    ipc_->getParams(&params);
    applyExportProfile(params, opts);

    auto job = create_processing_job(fname_, is_raw_, params, opts.fast,
                                       req.fastSize);
    if (!job) {
        done(false, "impossible to create the processing job", 0, 0);
        return;
    }
    int w = 0, h = 0;
    const int rc = runExportJob(job, opts, req.outfile, nullptr, w, h);
    if (rc == 1) {
        done(false, "failure in processing " + fname_, 0, 0);
    } else if (rc != 0) {
        // do not leave a truncated file behind
        g_unlink(req.outfile.c_str());
        done(false, "failure in saving to " + req.outfile, 0, 0);
    } else {
        done(true, req.outfile, w, h);
    }
}


bool EditSession::getFullSize(int &w, int &h)
{
    if (!ipc_) {
        return false;
    }
    w = ipc_->getFullWidth();
    h = ipc_->getFullHeight();
    return true;
}


bool EditSession::getSpotWB(int x, int y, int w, int h,
                            art::engine::remote::SpotWB &out)
{
    if (!ipc_) {
        return false;
    }
    eng::ColorTemp ct;
    ipc_->getSpotWBRect(x, y, w, h, ct);
    out.temperature = ct.getTemp();
    out.tint = ct.getGreen();
    ct.getMultipliers(out.red, out.green, out.blue);
    return true;
}


std::vector<art::engine::remote::HistoryEntryInfo> EditSession::getHistoryEntries()
{
    std::vector<art::engine::remote::HistoryEntryInfo> ret;
    for (auto &e : model_.entries()) {
        ret.push_back({e.text, e.value});
    }
    return ret;
}


int EditSession::getHistoryIndex() { return model_.selectedEntry(); }


bool EditSession::selectHistoryIndex(int index)
{
    if (!ipc_ || !model_.selectEntry(index)) {
        return false;
    }
    changeParams(pp::FullPartialProfile(model_.entry(index).params),
                 eng::EvHistoryBrowsed);
    dirty_ = true;
    return true;
}


std::vector<Glib::ustring> EditSession::getSnapshotNames()
{
    std::vector<Glib::ustring> ret;
    for (auto &n : model_.snapshotNames()) {
        ret.push_back(n);
    }
    return ret;
}


int EditSession::getSnapshotIndex() { return model_.activeSnapshot(); }


bool EditSession::activateSnapshot(const Glib::ustring &name)
{
    if (!ipc_ || !model_.activateSnapshot(name.raw())) {
        return false;
    }
    auto params = changeParams(
        pp::FullPartialProfile(model_.snapshot(model_.activeSnapshot()).params),
        eng::EvBookmarkSelected);
    record(params, eng::EvBookmarkSelected, name);
    dirty_ = true;
    return true;
}


bool EditSession::addSnapshot(const Glib::ustring &name,
                              const pp::ProcParams &params, Glib::ustring &err)
{
    std::string e;
    if (!model_.addSnapshot(name.raw(), params, e)) {
        err = e;
        return false;
    }
    dirty_ = true;
    return true;
}


bool EditSession::removeSnapshot(const Glib::ustring &name, Glib::ustring &err)
{
    const bool was_active = model_.findSnapshot(name.raw()) >= 0 &&
                            model_.findSnapshot(name.raw()) ==
                                model_.activeSnapshot();
    std::string e;
    if (!model_.removeSnapshot(name.raw(), e)) {
        err = e;
        return false;
    }
    if (was_active && model_.selectedEntry() >= 0) {
        // back to the last history entry (as the delete button does)
        changeParams(
            pp::FullPartialProfile(model_.entry(model_.selectedEntry()).params),
            eng::EvHistoryBrowsed);
    }
    dirty_ = true;
    return true;
}


bool EditSession::overwriteSnapshot(const Glib::ustring &name,
                                    const pp::ProcParams &params,
                                    Glib::ustring &err)
{
    std::string e;
    if (!model_.overwriteSnapshot(name.raw(), params, e)) {
        err = e;
        return false;
    }
    dirty_ = true;
    return true;
}

}} // namespace art::gui
