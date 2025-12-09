/* -*- C++ -*-
 *
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2004-2010 Gabor Horvath <hgabor@rawtherapee.com>
 *
 *  RawTherapee is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RawTherapee is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RawTherapee.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "../rtengine/rtengine.h"
#include "blackwhite.h"
#include "cacorrection.h"
#include "chmixer.h"
#include "coarsepanel.h"
#include "crop.h"
#include "defringe.h"
#include "denoise.h"
#include "distortion.h"
#include "exposure.h"
#include "gradient.h"
#include "icmpanel.h"
#include "imageareatoollistener.h"
#include "impulsedenoise.h"
#include "labcurve.h"
#include "lensgeom.h"
#include "lensgeomlistener.h"
#include "lensprofile.h"
#include "metadatapanel.h"
#include "pcvignette.h"
#include "perspective.h"
#include "pparamschangelistener.h"
#include "profilechangelistener.h"
#include "resize.h"
#include "rotate.h"
#include "saturation.h"
#include "sharpening.h"
#include "textureboost.h"
#include "tonecurve.h"
#include "toneequalizer.h"
#include "toolbar.h"
#include "toolpanel.h"
#include "vignetting.h"
#include "whitebalance.h"
#include <gtkmm.h>
#include <vector>
// #include "dirpyrequalizer.h"
#include "../rtengine/noncopyable.h"
#include "bayerpreprocess.h"
#include "bayerprocess.h"
#include "bayerrawexposure.h"
#include "colorcorrection.h"
#include "darkframe.h"
#include "dehaze.h"
#include "fattaltonemap.h"
#include "filmnegative.h"
#include "filmsimulation.h"
#include "flatfield.h"
#include "grain.h"
#include "guiutils.h"
#include "hslequalizer.h"
#include "localcontrast.h"
#include "logencoding.h"
#include "preprocess.h"
#include "prsharpening.h"
#include "rawcacorrection.h"
#include "rawexposure.h"
#include "rgbcurves.h"
#include "sensorbayer.h"
#include "sensorxtrans.h"
#include "smoothing.h"
#include "softlight.h"
#include "spot.h"
#include "xtransprocess.h"
#include "xtransrawexposure.h"

class ImageEditorCoordinator;

class ToolPanelCoordinator: public ToolPanelListener,
                            public ToolBarListener,
                            public ProfileChangeListener,
                            public WBProvider,
                            public DFProvider,
                            public FFProvider,
                            public LensGeomListener,
                            public SpotWBListener,
                            public CropPanelListener,
                            public PerspCorrectionPanelListener,
                            public ICMPanelListener,
                            public ImageAreaToolListener,
                            public rtengine::ImageTypeListener,
                            public rtengine::AutoExpListener,
                            public FilmNegProvider,
                            public AreaDrawListenerProvider,
                            public DeltaEColorProvider,
                            public rtengine::NonCopyable {
protected:
    WhiteBalance *whitebalance;
    Vignetting *vignetting;
    Gradient *gradient;
    PCVignette *pcvignette;
    GeometryPanel *geompanel;
    LensPanel *lenspanel;
    LensProfilePanel *lensProf;
    Rotate *rotate;
    Distortion *distortion;
    PerspCorrection *perspective;
    CACorrection *cacorrection;
    ChMixer *chmixer;
    BlackWhite *blackwhite;
    HSLEqualizer *hsl;
    Resize *resize;
    PrSharpening *prsharpening;
    ICMPanel *icm;
    Crop *crop;
    Exposure *exposure;
    Saturation *saturation;
    ToneCurve *toneCurve;
    ToneEqualizer *toneEqualizer;
    LocalContrast *localContrast;
    Spot *spot;
    Defringe *defringe;
    ImpulseDenoise *impulsedenoise;
    Denoise *denoise;
    TextureBoost *textureBoost;
    Sharpening *sharpening;
    LabCurve *lcurve;
    RGBCurves *rgbcurves;
    SoftLight *softlight;
    Dehaze *dehaze;
    FilmGrain *grain;
    FilmSimulation *filmSimulation;
    SensorBayer *sensorbayer;
    SensorXTrans *sensorxtrans;
    BayerProcess *bayerprocess;
    XTransProcess *xtransprocess;
    BayerPreProcess *bayerpreprocess;
    PreProcess *preprocess;
    DarkFrame *darkframe;
    FlatField *flatfield;
    RAWCACorr *rawcacorrection;
    RAWExposure *rawexposure;
    BayerRAWExposure *bayerrawexposure;
    XTransRAWExposure *xtransrawexposure;
    FattalToneMapping *fattal;
    LogEncoding *logenc;
    MetaDataPanel *metadata;
    Smoothing *smoothing;
    ColorCorrection *colorcorrection;
    FilmNegative *filmNegative;

    std::vector<PParamsChangeListener *> paramcListeners;

    rtengine::StagedImageProcessor *ipc;

    std::vector<ToolPanel *> toolPanels;
    std::vector<FoldableToolPanel *> favorites;
    ToolVBox *favoritePanel;
    ToolVBox *exposurePanel;
    ToolVBox *detailsPanel;
    ToolVBox *colorPanel;
    ToolVBox *transformPanel;
    ToolVBox *rawPanel;
    // ToolVBox* advancedPanel;
    ToolVBox *localPanel;
    ToolVBox *effectsPanel;
    ToolBar *toolBar;

    TextOrIcon *toiF;
    TextOrIcon *toiE;
    TextOrIcon *toiD;
    TextOrIcon *toiC;
    TextOrIcon *toiT;
    TextOrIcon *toiR;
    TextOrIcon *toiM;
    TextOrIcon *toiW;
    TextOrIcon *toiL;
    TextOrIcon *toiFx;

    Gtk::Image *imgPanelEnd[8];
    Gtk::VBox *vbPanelEnd[8];

    Gtk::ScrolledWindow *favoritePanelSW;
    Gtk::ScrolledWindow *exposurePanelSW;
    Gtk::ScrolledWindow *detailsPanelSW;
    Gtk::ScrolledWindow *colorPanelSW;
    Gtk::ScrolledWindow *transformPanelSW;
    Gtk::ScrolledWindow *rawPanelSW;
    // Gtk::ScrolledWindow* advancedPanelSW;
    Gtk::ScrolledWindow *localPanelSW;
    Gtk::ScrolledWindow *effectsPanelSW;

    std::vector<MyExpander *> expList;

    bool hasChanged;

    void addPanel(Gtk::Box *where, FoldableToolPanel *panel, int level = 1);
    void foldThemAll(GdkEventButton *event);
    void updateVScrollbars(bool hide);
    void addfavoritePanel(Gtk::Box *where, FoldableToolPanel *panel,
                          int level = 1);

private:
    EditDataProvider *editDataProvider;

public:
    CoarsePanel *coarse;
    Gtk::Notebook *toolPanelNotebook;

    ToolPanelCoordinator(bool batch = false);
    ~ToolPanelCoordinator() override;

    bool getChangedState() { return hasChanged; }
    void updateCurveBackgroundHistogram(
        const LUTu &histToneCurve, const LUTu &histLCurve,
        const LUTu &histCCurve, const LUTu &histLCAM, const LUTu &histCCAM,
        const LUTu &histRed, const LUTu &histGreen, const LUTu &histBlue,
        const LUTu &histLuma, const LUTu &histLRETI);
    void foldAllButOne(Gtk::Box *parent, FoldableToolPanel *openedSection);

    // multiple listeners can be added that are notified on changes (typical:
    // profile panel and the history)
    void addPParamsChangeListener(PParamsChangeListener *pp)
    {
        paramcListeners.push_back(pp);
    }

    // toolpanellistener interface
    void refreshPreview(const rtengine::ProcEvent &event) override;
    void panelChanged(const rtengine::ProcEvent &event,
                      const Glib::ustring &descr) override;
    void setTweakOperator(rtengine::TweakOperator *tOperator) override;
    void unsetTweakOperator(rtengine::TweakOperator *tOperator) override;

    void imageTypeChanged(bool isRaw, bool isBayer, bool isXtrans,
                          bool isMono = false) override;

    //    void autoContrastChanged (double autoContrast);
    // profilechangelistener interface
    void profileChange(const rtengine::procparams::PartialProfile *nparams,
                       const rtengine::ProcEvent &event,
                       const Glib::ustring &descr,
                       const ParamsEdited *paramsEdited = nullptr,
                       bool fromLastSave = false) override;
    void
    setDefaults(const rtengine::procparams::ProcParams *defparams) override;

    // DirSelectionListener interface
    void dirSelected(const Glib::ustring &dirname,
                     const Glib::ustring &openfile);

    // to support the GUI:
    CropGUIListener *
    getCropGUIListener(); // through the CropGUIListener the editor area can
                          // notify the "crop" ToolPanel when the crop selection
                          // changes

    // init the toolpanelcoordinator with an image & close it
    void initImage(rtengine::StagedImageProcessor *ipc_, bool israw);
    void closeImage();

    // update the "expanded" state of the Tools
    void updateToolState();
    void openAllTools();
    void closeAllTools();
    // read/write the "expanded" state of the expanders & read/write the crop
    // panel settings (ratio, guide type, etc.)
    void readOptions();
    void writeOptions();
    void writeToolExpandedStatus(std::vector<int> &tpOpen);

    // wbprovider interface
    void getAutoWB(rtengine::ColorTemp &out, double equal) override
    {
        if (ipc) {
            ipc->getAutoWB(out, equal);
        }
    }
    void getCamWB(rtengine::ColorTemp &out) override
    {
        if (ipc) {
            ipc->getCamWB(out);
        }
    }

    std::vector<WBPreset> getWBPresets() const override;
    void convertWBCam2Mul(double &rm, double &gm, double &bm) override;
    void convertWBMul2Cam(double &rm, double &gm, double &bm) override;

    // DFProvider interface
    rtengine::RawImage *getDF() override;

    // FFProvider interface
    rtengine::RawImage *getFF() override;
    Glib::ustring GetCurrentImageFilePath() override;
    bool hasEmbeddedFF() override;

    // FilmNegProvider interface
    bool getFilmNegativeSpot(rtengine::Coord spot, int spotSize, RGB &refInput,
                             RGB &refOutput) override;

    // rotatelistener interface
    void straightenRequested() override;
    void autoCropRequested() override;
    double autoDistorRequested() override;
    void autoPerspectiveRequested(
        bool horiz, bool vert, double &angle, double &horizontal,
        double &vertical, double &shear,
        const std::vector<rtengine::ControlLine> *lines = nullptr) override;
    void updateTransformPreviewRequested(rtengine::ProcEvent event,
                                         bool render_perspective) override;

    // spotwblistener interface
    void spotWBRequested(int size) override;

    // croppanellistener interface
    void cropSelectRequested() override;
    // void cropResetRequested() override;
    void cropEnableChanged(bool enabled) override;

    // PerspCorrectionPanelListener interface
    void controlLineEditModeChanged(bool active) override;

    // icmpanellistener interface
    void saveInputICCReference(const Glib::ustring &fname,
                               bool apply_wb) override;

    // imageareatoollistener interface
    void spotWBselected(int x, int y, Thumbnail *thm = nullptr) override;
    void sharpMaskSelected(bool sharpMask) override;
    int getSpotWBRectSize() const override;
    void cropSelectionReady() override;
    void rotateSelectionReady(double rotate_deg,
                              Thumbnail *thm = nullptr) override;
    ToolBar *getToolBar() const override;
    CropGUIListener *startCropEditing(Thumbnail *thm = nullptr) override;

    void updateTPVScrollbar(bool hide);
    bool handleShortcutKey(GdkEventKey *event);

    // ToolBarListener interface
    void toolSelected(ToolMode tool) override;
    void toolDeselected(ToolMode tool) override;
    void editModeSwitchedOff() override;

    void setEditProvider(EditDataProvider *provider);

    // AutoExpListener interface
    void autoExpChanged(double expcomp, int bright, int contr, int black,
                        int hlcompr, int hlcomprthresh, bool hlrecons) override;
    void
    autoMatchedToneCurveChanged(const std::vector<double> &curve,
                                const std::vector<double> &curve2) override;

    void setAreaDrawListener(AreaDrawListener *listener) override;

    // DeltaEColorProvider interface
    bool getDeltaELCH(EditUniqueID id, rtengine::Coord pos, float &L, float &C,
                      float &H) override;

    void setProgressListener(rtengine::ProgressListener *pl);

    void setToolShortcutManager(ToolShortcutManager *mgr);

private:
    IdleRegister idle_register;
};
