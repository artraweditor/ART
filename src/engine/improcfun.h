/* -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright (c) 2004-2010 Gabor Horvath <hgabor@rawtherapee.com>
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

#include "LUT.h"
#include "color.h"
#include "coord2d.h"
#include "wavelet.h"
#include "curves.h"
#include "dcp.h"
#include "gamutwarning.h"
#include "ipmonitor.h"
#include "image16.h"
#include "image8.h"
#include "imagefloat.h"
#include "labimage.h"
#include "lcp.h"
#include "masks.h"
#include "pipettebuffer.h"
#include "abortpoint.h"
#include "procparams.h"

#include <atomic>
#include <cstdint>
#include <vector>

namespace art { namespace engine {

extern const Settings *settings;

using namespace procparams;

namespace gpu {
namespace ops {
class DenoisePools;
} // namespace ops
} // namespace gpu

class CLUTApplication;
struct CTLGuideSet;

class ImProcFunctions;

struct ImProcData {
    const ProcParams *params;
    double scale;
    bool multiThread;
    ImProcFunctions *ipf;

    explicit ImProcData(const ProcParams *p = nullptr, double s = 1.0,
                        bool m = true, ImProcFunctions *i = nullptr)
        : params(p), scale(s), multiThread(m), ipf(i)
    {
    }
};

class ImProcFunctions {
public:
    //----------------------------------------------------------------------
    // constructor/destructor and initialization/state manipulation
    //----------------------------------------------------------------------
    ImProcFunctions(const ProcParams *iparams, bool imultiThread = true);
    ~ImProcFunctions();

    void setScale(double iscale);

    gpu::Context *getGPUContext();
    gpu::BufferPool *getGPUPool();

    void updateColorProfiles(const Glib::ustring &monitorProfile,
                             RenderingIntent monitorIntent, bool softProof,
                             GamutCheck gamutCheck);
    void setMonitorTransform(cmsHTRANSFORM xform)
    {
        monitorTransform = xform;
        monitorFast.reset();
    }

    void setDCPProfile(DCPProfile *dcp, const DCPProfile::ApplyState &as)
    {
        dcpProf = dcp;
        dcpApplyState = &as;
    }

    void setPipetteBuffer(PipetteBuffer *pb) { pipetteBuffer = pb; }

    void setProgressListener(ProgressListener *pl, int num_previews);

    /**
     * Make the pipeline abandonable: once *gen differs from `start`,
     * throwIfAborted() throws Cancelled.  Pass nullptr to turn it off again
     * (the default, and what export and the thumbnailer use).  Use
     * AbortScope rather than calling this directly.
     */
    void setAbortToken(const std::atomic<uint64_t> *gen, uint64_t start)
    {
        abort_gen_ = gen;
        abort_start_ = start;
    }
    void throwIfAborted() const
    {
        if (abort_gen_ &&
            abort_gen_->load(std::memory_order_acquire) != abort_start_) {
            throw Cancelled();
        }
    }
    //----------------------------------------------------------------------

    //----------------------------------------------------------------------
    // pipeline management
    //----------------------------------------------------------------------
    enum class Stage { STAGE_0, STAGE_1, STAGE_2, STAGE_3 };
    enum class Pipeline { THUMBNAIL, NAVIGATOR, PREVIEW, OUTPUT };
    void setPipeline(Pipeline pipeline) { cur_pipeline = pipeline; }
    bool process(Stage stage, Imagefloat *img);

    /**
     * The tools of STAGE_1..STAGE_3 as a single ordered list of steps, so
     * that a caller can keep a copy of the image at any step boundary and
     * resume from there (see StageCache).  process(Stage, img) runs all the
     * steps of a stage.  STAGE_0 is not part of the list.
     */
    struct StopState {
        int stage = -1;     // stage of the last step run
        bool prev = false;  // a previous stage asked to stop: skip everything
        bool cur = false;   // a tool of the current stage asked to stop
        bool any() const { return prev || cur; }
    };
    static int numSteps();
    static int stageBegin(Stage stage); // first step of the stage
    static Stage stepStage(int step);
    static const char *stepName(int step);
    /** Copy into `dst` the ProcParams section(s) that the step reads. */
    static void copyStepParams(int step, const ProcParams &src,
                               ProcParams &dst);
    /** Run steps [first, last).  Returns true if the pipeline must stop. */
    bool processSteps(int first, int last, Imagefloat *img, StopState &stop);

    void setViewport(int ox, int oy, int fw, int fh);
    void setOutputHistograms(LUTu *histToneCurve, LUTu *histCCurve,
                             LUTu *histLCurve);
    void setShowSharpeningMask(bool yes);
    //----------------------------------------------------------------------

    //----------------------------------------------------------------------
    // image processing operations
    //----------------------------------------------------------------------
    void firstAnalysis(const Imagefloat *const working,
                       const ProcParams &params, LUTu &vhist16);

    void labAdjustments(Imagefloat *rgb);
    bool doSharpening(Imagefloat *rgb, const SharpeningParams &sharpenParam,
                      bool showMask);
    bool sharpening(Imagefloat *img);
    bool prsharpening(Imagefloat *img);
    void transform(Imagefloat *original, Imagefloat *transformed, int cx,
                   int cy, int sx, int sy, int oW, int oH, int fW, int fH,
                   const FramesMetaData *metadata, int rawRotationDeg,
                   bool highQuality);
    void resize(Imagefloat *src, Imagefloat *dst, float dScale);
    void Lanczos(Imagefloat *src, Imagefloat *dst, float scale);
    void impulsedenoise(Imagefloat *rgb); // Emil's impulse denoise
    bool textureBoost(Imagefloat *rgb);

    struct DenoiseInfoStore {
        DenoiseInfoStore(): pparams() { reset(); }
        float chM;
        float max_r[9];
        float max_b[9];
        float ch_M[9];
        bool valid;
        ProcParams pparams;
        double chrominance;
        double chrominanceRedGreen;
        double chrominanceBlueYellow;

        bool update_pparams(const ProcParams &p);
        void reset();
    };
    void denoiseComputeParams(ImageSource *imgsrc, const ColorTemp &currWB,
                              DenoiseInfoStore &store,
                              procparams::DenoiseParams &dnparams);
    void denoise(ImageSource *imgsrc, const ColorTemp &currWB, Imagefloat *img,
                 const procparams::DenoiseParams &dnparams);

    void defringe(Imagefloat *rgb);
    void dehaze(Imagefloat *rgb);
    void dynamicRangeCompression(Imagefloat *rgb);
    bool localContrast(Imagefloat *rgb);
    bool toneEqualizer(Imagefloat *rgb);
    void softLight(Imagefloat *rgb);
    bool colorCorrection(Imagefloat *rgb);
    void logEncoding(Imagefloat *rgb);
    void filmGrain(Imagefloat *rgb);
    bool guidedSmoothing(Imagefloat *rgb);
    void hslEqualizer(Imagefloat *rgb);
    void channelMixer(Imagefloat *rgb);
    void exposure(Imagefloat *rgb);
    void rgbCurves(Imagefloat *rgb);
    void blackAndWhite(Imagefloat *rgb);
    void toneCurve(Imagefloat *img);
    void saturationVibrance(Imagefloat *img);
    void filmSimulation(Imagefloat *img);
    void fillCTLPipette(CLUTApplication *clut, Imagefloat *img,
                        const CTLGuideSet *guides = nullptr);
    void creativeGradients(Imagefloat *img);
    //----------------------------------------------------------------------

    //----------------------------------------------------------------------
    // Lab/RGB conversion
    //----------------------------------------------------------------------
    void rgb2monitor(Imagefloat *img, Image8 *image, bool bypass_out = false);

    Image8 *rgb2out(Imagefloat *img, int cx, int cy, int cw, int ch,
                    const procparams::ColorManagementParams &icm,
                    bool consider_histogram_settings = true);

    Imagefloat *rgb2out(Imagefloat *img,
                        const procparams::ColorManagementParams &icm);

    void rgb2lab(Imagefloat &src, LabImage &dst,
                 const Glib::ustring &workingSpace);
    void rgb2lab(Imagefloat &src, LabImage &dst)
    {
        rgb2lab(src, dst, params->icm.workingProfile);
    }

    void lab2rgb(const LabImage &src, Imagefloat &dst,
                 const Glib::ustring &workingSpace);
    void lab2rgb(const LabImage &src, Imagefloat &dst)
    {
        lab2rgb(src, dst, params->icm.workingProfile);
    }
    //----------------------------------------------------------------------

    //----------------------------------------------------------------------
    // misc helper functions for image processing ops
    //----------------------------------------------------------------------
    bool needsLuminanceOnly();
    bool needsTransform();
    bool needsPCVignetting();
    bool stage0NeedsFullImage();

    float resizeScale(const ProcParams *params, int fw, int fh, int &imw,
                      int &imh);

    void getAutoLog(ImageSource *imgsrc, procparams::LogEncodingParams &params);

    static double getAutoDistor(const Glib::ustring &fname, int thumb_size);
    bool transCoord(int W, int H, int x, int y, int w, int h, int &xv, int &yv,
                    int &wv, int &hv, double ascaleDef = -1,
                    const LensCorrection *pLCPMap = nullptr);
    bool transCoord(int W, int H, const std::vector<Coord2D> &src,
                    std::vector<Coord2D> &red, std::vector<Coord2D> &green,
                    std::vector<Coord2D> &blue, double ascaleDef = -1,
                    const LensCorrection *pLCPMap = nullptr);
    double getTransformAutoFill(int oW, int oH,
                                const LensCorrection *pLCPMap = nullptr);
    //----------------------------------------------------------------------

    class DeltaEData {
    public:
        bool ok;
        float L;
        float C;
        float H;
        double x;
        double y;
        int size;

        DeltaEData(): ok(false), L(0), C(0), H(0), x(-1), y(-1), size(32) {}
    };
    DeltaEData deltaE;
    int setDeltaEData(art::utils::EditUniqueID id, double x, double y,
                      int size = 32);

    // Spot Removal Tool
    void removeSpots(art::engine::Imagefloat *img, art::engine::ImageSource *imgsrc,
                     const std::vector<procparams::SpotEntry> &entries,
                     const PreviewProps &pp, const art::engine::ColorTemp &currWB,
                     const procparams::ColorManagementParams *cmp, int tr,
                     DenoiseInfoStore *dnstore);

    bool filmNegativeProcess(Imagefloat *input, Imagefloat *output,
                             FilmNegativeParams &fnp,
                             const RAWParams &rawParams,
                             const ImageSource *imgsrc,
                             const ColorTemp &currWB);
    void filmNegativeProcess(art::engine::Imagefloat *input,
                             art::engine::Imagefloat *output,
                             const procparams::FilmNegativeParams &params);

private:
    cmsHPROFILE monitor;
    cmsHTRANSFORM monitorTransform;
    // set only when it is equivalent to monitorTransform (see ipmonitor.h)
    std::unique_ptr<MonitorMatrixTransform> monitorFast;
    std::unique_ptr<GamutWarning> gamutWarning;

    const ProcParams *params;
    double scale;
    bool multiThread;
    Pipeline cur_pipeline;

    DCPProfile *dcpProf;
    const DCPProfile::ApplyState *dcpApplyState;

    PipetteBuffer *pipetteBuffer;

    double lumimul[3];

    int offset_x;
    int offset_y;
    int full_width;
    int full_height;

    LUTu *histToneCurve;
    LUTu *histCCurve;
    LUTu *histLCurve;

    bool show_sharpening_mask;

    const std::atomic<uint64_t> *abort_gen_ = nullptr;
    uint64_t abort_start_ = 0;

    ProgressListener *plistener;
    int progress_step;
    int progress_end;

    LinkedMaskManager linked_mask_mgr_;

    gpu::BufferPool *gpuPool_;

private:
    void transformLuminanceOnly(Imagefloat *original, Imagefloat *transformed,
                                int cx, int cy, int oW, int oH, int fW, int fH,
                                bool creative);
    void transformGeneral(bool highQuality, Imagefloat *original,
                          Imagefloat *transformed, int cx, int cy, int sx,
                          int sy, int oW, int oH, int fW, int fH,
                          const LensCorrection *pLCPMap);
    void transformLCPCAOnly(Imagefloat *original, Imagefloat *transformed,
                            int cx, int cy, const LensCorrection *pLCPMap);

    void expcomp(Imagefloat *rgb, const procparams::ExposureParams *expparams);

    bool needsCA();
    bool needsDistortion();
    bool needsRotation();
    bool needsPerspective();
    bool needsGradient();
    bool needsVignetting();
    bool needsLCP();
    bool needsLensfun();
    bool needsDCPProfile();

    struct StepDesc;
    static const std::vector<StepDesc> &steps();

    template <class Ret, class Method>
    Ret apply(const char *name, Method op, Imagefloat *img, bool can_skip_sync);
};

/// Makes `ipf` and this thread's abortPoint() abandon the work in flight once
/// *gen moves.
class AbortScope {
public:
    AbortScope(ImProcFunctions &ipf, const std::atomic<uint64_t> *gen):
        ipf_(ipf), start_(gen ? gen->load(std::memory_order_acquire) : 0),
        probe_(gen, start_), prev_(abortProbeSlot())
    {
        ipf_.setAbortToken(gen, start_);
        abortProbeSlot() = gen ? &probe_ : nullptr;
    }
    ~AbortScope()
    {
        abortProbeSlot() = prev_;
        ipf_.setAbortToken(nullptr, 0);
    }
    AbortScope(const AbortScope &) = delete;
    AbortScope &operator=(const AbortScope &) = delete;

private:
    ImProcFunctions &ipf_;
    const uint64_t start_; // one snapshot for both, declared before probe_
    AbortProbe probe_;
    const AbortProbe *prev_;
};

}} // namespace art::engine
