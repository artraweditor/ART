/* -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright (c) 2004-2010 Gabor Horvath <hgabor@rawtherapee.com>
 *  Copyright (c) 2010 Oliver Duis <www.oliverduis.de>
 *  Copyright (c) 2017-2026 Alberto Griggio <alberto.griggio@gmail.com>
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

#include "../engine/remote/json.h"
#include "../engine/rtengine.h"
#include "batchqueueentry.h"
#include "filepanel.h"
#include "histogrampanel.h"
#include "history.h"
#include "imageareapanel.h"
#include "navigator.h"
#include "profilepanel.h"
#include "progressconnector.h"
#include "saveasdlg.h"
#include "thumbnail.h"
#include "thumbnaillistener.h"
#include "toolpanelcoord.h"
#include <gtkmm.h>
#include <functional>
#include <memory>

namespace art { namespace gui {


class EditorPanel;
class MyProgressBar;

struct EditorPanelIdleHelper {
    EditorPanel *epanel;
    bool destroyed;
    int pending;
};

class RTWindow;
using art::engine::array2D;

class EditorPanel final: public Gtk::VBox,
                         public PParamsChangeListener,
                         public art::engine::ProgressListener,
                         public ThumbnailListener,
                         public HistoryBeforeAfterListener,
                         public art::engine::HistogramListener,
                         public HistogramPanelListener,
                         public art::engine::SizeListener {
public:
    explicit EditorPanel(FilePanel *filePanel = nullptr);
    ~EditorPanel() override;

    void open(Thumbnail *tmb, art::engine::InitialImage *isrc);
    void setAspect();
    void on_realize() override;
    void leftPaneButtonReleased(GdkEventButton *event);
    void rightPaneButtonReleased(GdkEventButton *event);

    void setParent(RTWindow *p);

    void setParentWindow(Gtk::Window *p) { parentWindow = p; }

    void writeOptions();
    void writeToolExpandedStatus(std::vector<int> &tpOpen);

    void showTopPanel(bool show);
    bool isRealized() { return realized; }
    // ProgressListener interface
    void setProgress(double p) override;
    void setProgressStr(const Glib::ustring &str) override;
    void setProgressState(bool inProcessing) override;
    void error(const Glib::ustring &descr) override;
    void pipelineTimes(const art::engine::PipelineTimes &t) override;

    void error(const Glib::ustring &title, const Glib::ustring &descr);
    void displayError(const Glib::ustring &title,
                      const Glib::ustring
                          &descr); // this is called by error in the gtk thread
    void refreshProcessingState(
        bool inProcessing); // this is called by setProcessingState in the gtk
                            // thread

    // PParamsChangeListener interface
    void procParamsChanged(const art::engine::procparams::ProcParams *params,
                           const art::engine::ProcEvent &ev,
                           const Glib::ustring &descr,
                           const ParamsEdited *paramsEdited = nullptr) override;
    void clearParamChanges() override;

    // thumbnaillistener interface
    void procParamsChanged(Thumbnail *thm, int whoChangedIt) override;

    // HistoryBeforeAfterListener
    void historyBeforeAfterChanged(
        const art::engine::procparams::ProcParams &params) override;

    // HistogramListener
    void histogramChanged(
        const art::engine::LUTu &histRed, const art::engine::LUTu &histGreen, const art::engine::LUTu &histBlue,
        const art::engine::LUTu &histLuma, const art::engine::LUTu &histToneCurve, const art::engine::LUTu &histLCurve,
        const art::engine::LUTu &histCCurve, const art::engine::LUTu &histLCAM, const art::engine::LUTu &histCCAM,
        const art::engine::LUTu &histRedRaw, const art::engine::LUTu &histGreenRaw,
        const art::engine::LUTu &histBlueRaw, const art::engine::LUTu &histChroma, const art::engine::LUTu &histLRETI,
        int vectorscopeScale, const array2D<int> &vectorscopeHC,
        const array2D<int> &vectorscopeHS, int waveformScale,
        const array2D<int> &waveformRed, const array2D<int> &waveformGreen,
        const array2D<int> &waveformBlue,
        const array2D<int> &waveformLuma) override;
    void setObservable(art::engine::HistogramObservable *observable) override;
    bool updateHistogram(void) const override;
    bool updateHistogramRaw(void) const override;
    bool updateVectorscopeHC(void) const override;
    bool updateVectorscopeHS(void) const override;
    bool updateWaveform(void) const override;

    // HistogramPanelListener
    void scopeTypeChanged(Options::ScopeType new_type) override;

    // SizeListener
    void sizeChanged(int w, int h, int ow, int oh) override;

    // event handlers
    void info_toggled();
    void hideHistoryActivated();
    void tbRightPanel_1_toggled();
    void tbTopPanel_1_toggled();
    void beforeAfterToggled();
    void tbBeforeLock_toggled();
    void saveAsPressed(GdkEventButton *event);
    void queueImgPressed(GdkEventButton *event);
    void sendToGimpPressed(GdkEventButton *event);
    void openNextEditorImage();
    void openPreviousEditorImage();
    void syncFileBrowser();

    void tbTopPanel_1_visible(bool visible);
    bool CheckSidePanelsVisibility();
    void tbShowHideSidePanels_managestate();
    void toggleSidePanels();
    void toggleSidePanelsZoomFit();

    // Saves the processing parameters of the image being edited (nothing is
    // written if they did not change and the files exist). With async the files
    // are written by a background thread, which is what to use when the caller
    // must not wait for the file system (see Thumbnail::saveProcParams()); the
    // callers that need the files to be there right away must not.
    void saveProfile(bool async = false);
    Glib::ustring getShortName();
    Glib::ustring getFileName();
    bool handleShortcutKey(GdkEventKey *event);
    void bindShortcuts();
    bool keyPressedBefore(GdkEventKey *event);
    bool keyReleased(GdkEventKey *event);
    bool scrollPressed(GdkEventScroll *event);

    bool getIsProcessing() const { return isProcessing || !can_open_now(); }
    void setIsProcessing() { isProcessing = true; }

    void updateProfiles(const Glib::ustring &printerProfile,
                        art::engine::RenderingIntent printerIntent,
                        bool printerBPC);
    void updateTPVScrollbar(bool hide);
    void updateHistogramPosition(int oldPosition, int newPosition);

    void defaultMonitorProfileChanged(const Glib::ustring &profile_name,
                                      bool auto_monitor_profile);

    bool saveImmediately(const Glib::ustring &filename, const SaveFormat &sf);

    // remote-command support (see remotecmd.cc)
    // true if an image is loaded and the pipeline is idle
    bool isReadyForRemote() const { return ipc && !getIsProcessing(); }
    // apply a profile to the open image (as the profile panel does)
    bool applyProfile(const art::engine::procparams::PartialProfile &pp,
                      const Glib::ustring &descr);
    History *getHistory() { return history; }
    // size of the full (processed) image
    bool getFullSize(int &w, int &h);
    // the white balance of the rectangle (x, y, w, h) in full-image
    // coordinates; the parameters are not changed
    bool getSpotWB(int x, int y, int w, int h, art::engine::ColorTemp &out);
    bool getCurrentParams(art::engine::procparams::ProcParams &pp);
    // the default profile of the preferences for this image (see
    // Options::loadDefaultProfile); null if there is no image ready
    std::unique_ptr<art::engine::procparams::PartialProfile>
    getDefaultProfile();
    // the current processing parameters, as the text of an .arp file
    bool getParamsText(Glib::ustring &out);
    // `key: value` lines: file, sizes, camera and exposure metadata
    bool getInfoText(Glib::ustring &out);
    // the metadata as the metadata panel shows it, as a JSON object
    // (makernotes: 1 = with the ExifTool maker notes, 0 = without, -1 = as in
    // the preferences)
    bool getMetadata(int makernotes, bool edits,
                     art::engine::remote::json::Value &out,
                     Glib::ustring &err);
    // Add the image, with its current parameters, to the batch queue, as the
    // "add to queue" button of the Save as dialog does. The queue forces the
    // extension of `outfile` to that of the format, and handles an existing
    // file as its preferences say (a numeric suffix, or replacing it), so the
    // file written can have another name.
    // Returns false (and sets `err`) if the image cannot be queued.
    bool queueExport(const Glib::ustring &outfile, const SaveFormat &sf,
                     bool fast_export, Glib::ustring &err, int fast_size = 0,
                     const art::engine::procparams::PartialProfile *overrides =
                         nullptr);
    // render the window (cx, cy, cw, ch) of the full image (in full-image
    // coordinates; cw <= 0 means the whole image) scaled by `scale` to a file
    // (format by extension). done() is called later, in the GUI thread
    void renderPreview(const Glib::ustring &outfile, double scale, int cx,
                       int cy, int cw, int ch,
                       std::function<void(bool, const Glib::ustring &, int, int)> done);

    Gtk::Paned *catalogPane;

    void cleanup();
    void close();

    void refreshImageAreas();

private:
    BatchQueueEntry *createBatchQueueEntry(
        bool fast_export, bool use_batch_queue_profile,
        const art::engine::procparams::PartialProfile *export_profile,
        int fast_size = 0);
    bool idle_imageSaved(ProgressConnector<int> *pc, art::engine::IImagefloat *img,
                         Glib::ustring fname, SaveFormat sf,
                         art::engine::procparams::ProcParams &pparams);
    bool idle_saveImage(ProgressConnector<art::engine::IImagefloat *> *pc,
                        Glib::ustring fname, SaveFormat sf,
                        art::engine::procparams::ProcParams &pparams);
    bool idle_sendToGimp(ProgressConnector<art::engine::IImagefloat *> *pc,
                         Glib::ustring fname);
    bool idle_sentToGimp(ProgressConnector<int> *pc, art::engine::IImagefloat *img,
                         Glib::ustring filename);
    void histogramProfile_toggled();

    void do_save_image(bool fast_export);
    void do_queue_image(bool fast_export);
    void do_send_to_gimp(bool fast_export);
    bool autosave();

    bool can_open_now() const;
    void update_open_mark(Thumbnail *thm, bool yes);

    Glib::ustring lastSaveAsFileName;
    bool realized;

    MyProgressBar *progressLabel;
    // time of the last main-preview processing run; GUI thread only
    art::engine::PipelineTimes lastTimes_;
    // the progress bar text for the idle state: "Ready", plus lastTimes_
    Glib::ustring readyText() const;
    Gtk::ToggleButton *info;
    Gtk::ToggleButton *hidehp;
    Gtk::ToggleButton *tbShowHideSidePanels;
    Gtk::ToggleButton *tbTopPanel_1;
    Gtk::ToggleButton *tbRightPanel_1;
    Gtk::ToggleButton *tbBeforeLock;
    // bool bAllSidePanelsVisible;
    Gtk::ToggleButton *beforeAfter;
    Gtk::Paned *hpanedl;
    Gtk::Paned *hpanedr;
    Gtk::Image *iHistoryShow, *iHistoryHide;
    Gtk::Image *iTopPanel_1_Show, *iTopPanel_1_Hide;
    Gtk::Image *iRightPanel_1_Show, *iRightPanel_1_Hide;
    Gtk::Image *iShowHideSidePanels;
    Gtk::Image *iShowHideSidePanels_exit;
    Gtk::Image *iBeforeLockON, *iBeforeLockOFF;
    Gtk::Paned *leftbox;
    Gtk::Box *leftsubbox;
    Gtk::Paned *vboxright;
    Gtk::Box *vsubboxright;

    Gtk::Button *queueimg;
    Gtk::Button *saveimgas;
    Gtk::Button *sendtogimp;
    Gtk::Button *navSync;
    Gtk::Button *navNext;
    Gtk::Button *navPrev;

    class ColorManagementToolbar;
    std::unique_ptr<ColorManagementToolbar> colorMgmtToolBar;

    ImageAreaPanel *iareapanel;
    PreviewHandler *previewHandler;
    PreviewHandler *beforePreviewHandler; // for the before-after view
    Navigator *navigator;
    ImageAreaPanel *beforeIarea; // for the before-after view
    Gtk::Box *beforeBox;
    Gtk::Box *afterBox;
    Gtk::Label *beforeLabel;
    Gtk::Label *afterLabel;
    Gtk::Box *beforeAfterBox;
    Gtk::Box *beforeHeaderBox;
    Gtk::Box *afterHeaderBox;
    Gtk::ToggleButton *toggleHistogramProfile;

    Gtk::Frame *ppframe;
    ProfilePanel *profilep;
    History *history;
    HistogramPanel *histogramPanel;
    ToolPanelCoordinator *tpc;
    RTWindow *parent;
    Gtk::Window *parentWindow;
    // SaveAsDialog* saveAsDialog;
    FilePanel *fPanel;

    bool firstProcessingDone;

    Thumbnail *openThm;  // may get invalid on external delete event
    Glib::ustring fname; // must be saved separately

    int selectedFrame;

    art::engine::InitialImage *isrc;
    std::shared_ptr<art::engine::StagedImageProcessor> ipc;
    std::shared_ptr<art::engine::StagedImageProcessor>
        beforeIpc; // for the before-after view

    EditorPanelIdleHelper *epih;

    int err;

    time_t processingStartedTime;

    sigc::connection ShowHideSidePanelsconn;

    bool isProcessing;

    IdleRegister idle_register;

    art::engine::HistogramObservable *histogram_observable;
    Options::ScopeType histogram_scope_type;

    sigc::connection autosave_conn_;

    std::unique_ptr<ToolShortcutsManager> shortcut_mgr_;
};


} } // namespace art::gui
