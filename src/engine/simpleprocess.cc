/* -*- C++ -*-
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
#include "../utils/multilangmgr.h"
#include "../gui/options.h"
#include "clutstore.h"
#include "colortemp.h"
#include "curves.h"
#include "iccstore.h"
#include "imagesource.h"
#include "improcfun.h"
#include "pipelineprofile.h"
#include "metadata.h"
#include "mytime.h"
#include "processingjob.h"
#include "rawimagesource.h"
#include "rescale.h"
#include "rtengine.h"
#include "threadpool.h"
#include <glibmm.h>

#undef THREAD_PRIORITY_NORMAL

namespace art { namespace engine {
extern const Settings *settings;

namespace {

class ImageProcessor {
public:
    ImageProcessor(ProcessingJob *pjob, int &errorCode, ProgressListener *pl,
                   bool flush)
        : job(static_cast<ProcessingJobImpl *>(pjob)), errorCode(errorCode),
          pl(pl), flush(flush),
          // internal state
          ii(nullptr), imgsrc(nullptr), fw(0), fh(0), scale_factor(1.0), tr(0),
          pp(0, 0, 0, 0, 0), dnstore(), pipeline_scale(1.0), stop(false),
          crop_region(false)
    {
    }

    Imagefloat *operator()()
    {
        if (!job->fast) {
            return normal_pipeline();
        } else {
            return fast_pipeline();
        }
    }

private:
    Imagefloat *normal_pipeline()
    {
        if (settings->verbose) {
            std::cout << "Processing with the normal pipeline" << std::endl;
        }

        if (!stage_init(false)) {
            return nullptr;
        }

        stage_denoise();
        stage_transform();
        return stage_finish(false);
    }

    Imagefloat *fast_pipeline()
    {
        if (!job->pparams.resize.enabled) {
            return normal_pipeline();
        }

        if (settings->verbose) {
            std::cout << "Processing with the fast pipeline" << std::endl;
        }

        // pl = nullptr;

        if (!stage_init(true)) {
            return nullptr;
        }

        stage_early_resize();
        stage_denoise();
        stage_transform();
        return stage_finish(true);
    }

    bool stage_init(bool is_fast)
    {
        errorCode = 0;

        if (pl) {
            pl->setProgressStr("PROGRESSBAR_PROCESSING");
            pl->setProgress(0.0);
        }

        ii = job->initialImage;

        if (!ii) {
            ii = InitialImage::load(job->fname, job->isRaw, &errorCode);

            if (errorCode) {
                delete job;
                return false; // return nullptr;
            }
        }

        procparams::ProcParams &params = job->pparams;

        // acquire image from imagesource
        imgsrc = ii->getImageSource();

        tr = getCoarseBitMask(params.coarse);
        if (imgsrc->getSensorType() == ST_BAYER) {
            if (params.raw.bayersensor.method !=
                RAWParams::BayerSensor::Method::PIXELSHIFT) {
                imgsrc->setBorder(params.raw.bayersensor.border);
            } else {
                imgsrc->setBorder(std::max(params.raw.bayersensor.border, 2));
            }
        } else if (imgsrc->getSensorType() == ST_FUJI_XTRANS) {
            imgsrc->setBorder(params.raw.xtranssensor.border);
        }
        imgsrc->getFullSize(fw, fh, tr);

        // check the crop params
        if (!params.crop.hasGeometry()) {
            params.crop.setDefaultGeometry(fw, fh);
        }

        if (params.crop.x > fw || params.crop.y > fh) {
            // the crop is completely out of the image, so we disable the crop
            params.crop.enabled = false;
            // and we set the values to the defaults
            params.crop.x = 0;
            params.crop.y = 0;
            params.crop.w = fw;
            params.crop.h = fh;
        } else {
            if (params.crop.x < 0) {
                params.crop.x = 0;
            }

            if (params.crop.y < 0) {
                params.crop.y = 0;
            }

            if (params.crop.w <= 0 ||
                (params.crop.x + params.crop.w) > fw) {
                // crop overflow in the width dimension ; we trim it
                params.crop.w = fw - params.crop.x;
            }

            if (params.crop.h <= 0 ||
                (params.crop.y + params.crop.h) > fh) {
                // crop overflow in the height dimension ; we trim it
                params.crop.h = fh - params.crop.y;
            }
        }

        ipf_p.reset(new ImProcFunctions(&params, true));
        ImProcFunctions &ipf = *(ipf_p.get());
        ipf.setPipeline(ImProcFunctions::Pipeline::OUTPUT);
        scale_factor = 1.0;
        if (is_fast) {
            int imw, imh;
            scale_factor = ipf.resizeScale(&params, fw, fh, imw, imh);
            adjust_procparams(scale_factor);
        }

        imgsrc->setCurrentFrame(params.raw.bayersensor.imageNum);

        // set the color temperature
        currWB = ColorTemp();
        if (params.wb.enabled) {
            switch (params.wb.method) {
            case WBParams::CAMERA:
                currWB = imgsrc->getWB();
                break;
            case WBParams::CUSTOM_TEMP:
                currWB = ColorTemp(params.wb.temperature, params.wb.green,
                                   params.wb.equal, "Custom");
                break;
            case WBParams::CUSTOM_MULT_LEGACY:
                currWB = ColorTemp(params.wb.mult[0], params.wb.mult[1],
                                   params.wb.mult[2], 1.0);
                break;
            case WBParams::CUSTOM_MULT: {
                double rm = params.wb.mult[0];
                double gm = params.wb.mult[1];
                double bm = params.wb.mult[2];
                imgsrc->wbCamera2Mul(rm, gm, bm);
                currWB = ColorTemp(rm, gm, bm); //, 1.0);
            } break;
            case WBParams::AUTO:
            default:
                currWB = ColorTemp();
            }
        }

        {
            ART_PROFILE_SCOPE("raw:preprocess");
        imgsrc->preprocess(params.raw, params.lensProf, params.coarse,
                           params.denoise.enabled, currWB);
        }

        if (pl) {
            pl->setProgress(0.20);
        }
        bool autoContrast =
            imgsrc->getSensorType() == ST_BAYER
                ? params.raw.bayersensor.dualDemosaicAutoContrast
                : params.raw.xtranssensor.dualDemosaicAutoContrast;
        double contrastThreshold =
            imgsrc->getSensorType() == ST_BAYER
                ? params.raw.bayersensor.dualDemosaicContrast
                : params.raw.xtranssensor.dualDemosaicContrast;
        {
            ART_PROFILE_SCOPE("raw:demosaic");
            imgsrc->demosaic(params.raw, autoContrast, contrastThreshold);
        }

        if (params.wb.method == WBParams::AUTO) {
            double rm, gm, bm;
            imgsrc->getAutoWBMultipliers(rm, gm, bm);
            if (rm != -1.) {
                if (params.wb.equal == 1) {
                    currWB = ColorTemp(rm, gm, bm);
                } else {
                    currWB = ColorTemp(rm, gm, bm, params.wb.equal);
                }
            } else {
                currWB.useDefaults(params.wb.equal);
            }
        }

        if (pl) {
            pl->setProgress(0.30);
        }
        pp = PreviewProps(0, 0, fw, fh, 1);
        crop_region = setup_crop_region(is_fast);

        if (pl) {
            pl->setProgress(0.40);
        }

        // imgsrc->HLRecovery_Global(params.exposure);

        if (pl) {
            pl->setProgress(0.45);
        }

        if (params.denoise.enabled) {
            {
                ART_PROFILE_SCOPE("denoise:params");
                ipf.denoiseComputeParams(imgsrc, currWB, dnstore, params.denoise);
            }
        }

        if (crop_region) {
            int w, h;
            imgsrc->getSize(pp, w, h);
            img = new Imagefloat(w, h);
        } else {
            img = new Imagefloat(fw, fh);
        }
        {
            ART_PROFILE_SCOPE("raw:getImage");
            imgsrc->getImage(currWB, tr, img, pp, params.exposure, params.raw);
        }
        img->assignColorSpace(params.icm.workingProfile);

        if (pl) {
            pl->setProgress(0.50);
        }

        if (params.toneCurve.histmatching) {
            if (!params.toneCurve.fromHistMatching) {
                imgsrc->getAutoMatchedToneCurve(params.icm,
                                                params.toneCurve.curve,
                                                params.toneCurve.curve2);
            }
        }
        if (params.logenc.enabled && params.logenc.autocompute) {
            ipf.getAutoLog(imgsrc, params.logenc);
        }
        if (params.sharpening.enabled && params.sharpening.deconvAutoRadius) {
            float r = 0.f;
            if (imgsrc->getDeconvAutoRadius(&r)) {
                params.sharpening.deconvradius = r;
            }
        }

        // Spot Removal
        if (params.spot.enabled && !params.spot.entries.empty()) {
            ipf.removeSpots(img, imgsrc, params.spot.entries, pp, currWB,
                            nullptr, tr, nullptr);
        }

        if (flush) {
            imgsrc->flushRawData();
            imgsrc->flushRGB();
        }

        return true;
    }

    // last line of defence against NaN/Inf: report which stage let them in
    void sanitize(const char *where)
    {
        const size_t bad = img->sanitize();
        if (bad && settings->verbose) {
            printf("WARNING: replaced %zu NaN samples after %s\n", bad,
                   where);
        }
    }

    void stage_denoise()
    {
        procparams::ProcParams &params = job->pparams;
        ImProcFunctions &ipf = *(ipf_p.get());

        bool converted = false;
        if (params.filmNegative.colorSpace !=
            FilmNegativeParams::ColorSpace::INPUT) {
            {
                ART_PROFILE_SCOPE("input-profile");
                imgsrc->convertColorSpace(img, params.icm, currWB);
            }
            converted = true;
        }

        if (params.filmNegative.enabled) {
            FilmNegativeParams copy = params.filmNegative;
            ipf.filmNegativeProcess(img, img, copy, params.raw, imgsrc, currWB);
        }

        if (!converted) {
            {
                ART_PROFILE_SCOPE("input-profile");
                imgsrc->convertColorSpace(img, params.icm, currWB);
            }
        }

        sanitize("input");

        if (params.denoise.enabled) {
            {
                ART_PROFILE_SCOPE("denoise");
                ipf.denoise(imgsrc, currWB, img, params.denoise);
            }
        }
    }

    void stage_transform()
    {
        procparams::ProcParams &params = job->pparams;
        ImProcFunctions &ipf = *(ipf_p.get());

        sanitize("denoise");

        LUTu hist16(65536);
        {
            ART_PROFILE_SCOPE("firstAnalysis");
            ipf.firstAnalysis(img, params, hist16);
        }

        stop = ipf.process(ImProcFunctions::Stage::STAGE_0, img);

        // perform transform (excepted resizing)
        if (ipf.needsTransform()) {
            if (params.distortion.enabled && params.distortion.autocompute) {
                params.distortion.amount =
                    ImProcFunctions::getAutoDistor(imgsrc->getFileName(), 400);
            }

            Imagefloat *trImg = nullptr;
            if (crop_region) {
                // straight into the crop
                trImg = new Imagefloat(params.crop.w, params.crop.h, img);
            } else if (ipf.needsLuminanceOnly()) {
                trImg = img;
            } else {
                trImg = new Imagefloat(fw, fh, img);
            }
            {
                ART_PROFILE_SCOPE("transform");
            ipf.transform(img, trImg, crop_region ? params.crop.x : 0,
                          crop_region ? params.crop.y : 0, pp.getX(),
                          pp.getY(), fw, fh, fw, fh, imgsrc->getMetaData(),
                          imgsrc->getRotateDegree(), true);
            }
            if (trImg != img) {
                delete img;
                img = trImg;
            }
        }
    }

    Imagefloat *stage_finish(bool is_fast)
    {
        procparams::ProcParams &params = job->pparams;
        ImProcFunctions &ipf = *(ipf_p.get());

        int cx = 0, cy = 0, cw = img->getWidth(), ch = img->getHeight();
        if (crop_region) {
            // img is the crop already
            cx = params.crop.x;
            cy = params.crop.y;
            ipf.setViewport(cx, cy, fw, fh);
        } else if (params.crop.enabled) {
            int iw = img->getWidth();
            int ih = img->getHeight();

            cx = params.crop.x * scale_factor + 0.5;
            cy = params.crop.y * scale_factor + 0.5;
            cw = std::min(int(params.crop.w * scale_factor + 0.5), iw - cx);
            ch = std::min(int(params.crop.h * scale_factor + 0.5), ih - cy);

            ipf.setViewport(cx, cy, iw, ih);

            Imagefloat *tmpimg = new Imagefloat(cw, ch, img);
#ifdef _OPENMP
#pragma omp parallel for
#endif
            for (int row = 0; row < ch; row++) {
                for (int col = 0; col < cw; col++) {
                    tmpimg->r(row, col) = img->r(row + cy, col + cx);
                    tmpimg->g(row, col) = img->g(row + cy, col + cx);
                    tmpimg->b(row, col) = img->b(row + cy, col + cx);
                }
            }

            delete img;
            img = tmpimg;
        }

        DCPProfile::ApplyState as;
        DCPProfile *dcpProf = imgsrc->getDCP(params.icm, as);

        ipf.setDCPProfile(dcpProf, as);
        stop = stop || ipf.process(ImProcFunctions::Stage::STAGE_1, img);

        if (pl) {
            pl->setProgress(0.55);
        }

        stop = stop || ipf.process(ImProcFunctions::Stage::STAGE_2, img);
        stop = stop || ipf.process(ImProcFunctions::Stage::STAGE_3, img);

        if (pl) {
            pl->setProgress(0.60);
        }

        if (params.resize.enabled) {
            if (!is_fast) {
                int imw, imh;
                double scale = ipf.resizeScale(&params, fw, fh, imw, imh);
                bool allow_upscaling =
                    params.resize.allowUpscaling || params.resize.dataspec == 0;
                if (scale < 1.0 || (scale > 1.0 && allow_upscaling)) {
                    Imagefloat *resized = new Imagefloat(imw, imh, img);
                    { ART_PROFILE_SCOPE("resize");
                      ipf.Lanczos(img, resized, scale); }
                    delete img;
                    img = resized;
                }
            }
        }
        if (params.prsharpening.enabled) {
            ipf.setScale(1);
            ipf.prsharpening(img);
        }

        sanitize("pipeline");

        ART_PROFILE_SCOPE("rgb2out");
        Imagefloat *readyImg = ipf.rgb2out(img, params.icm);

        if (settings->verbose) {
            printf("Output profile_: \"%s\"\n",
                   params.icm.outputProfile.c_str());
        }

        delete img;
        img = nullptr;

        if (pl) {
            pl->setProgress(0.70);
        }

        Exiv2Metadata info(imgsrc->getFileName());
        switch (params.metadata.mode) {
        case MetaDataParams::TUNNEL:
            readyImg->setMetadata(info);
            break;
        case MetaDataParams::EDIT: {
            auto exifpairs = params.metadata.exif;
            if (params.resize.enabled && params.resize.copyPPIToExif) {
                const Glib::ustring val = Glib::ustring::format(params.resize.ppi) + "/1";
                exifpairs["Exif.Image.XResolution"] = val;
                exifpairs["Exif.Image.YResolution"] = val;
                exifpairs["Exif.Image.ResolutionUnit"] = "2";
            }
            info.setExif(exifpairs);
            info.setIptc(params.metadata.iptc);
            if (!(params.metadata.exifKeys.size() == 1 &&
                  params.metadata.exifKeys[0] == "*")) {
                info.setExifKeys(&(params.metadata.exifKeys));
            }
            info.setOutputRating(params,
                                 art::gui::options.thumbnail_rating_mode !=
                                     art::gui::Options::ThumbnailRatingMode::PROCPARAMS);
            readyImg->setMetadata(info);
        }   break;
        default: // case MetaDataParams::STRIP
            // nothing to do
            break;
        }

        // Setting the output curve to readyImg
        if (params.icm.outputProfile != "" &&
            params.icm.outputProfile != ColorManagementParams::NoICMString &&
            params.icm.outputProfile !=
                ColorManagementParams::NoProfileString) {

            cmsHPROFILE jprof = ICCStore::getInstance()->getProfile(
                params.icm.outputProfile); // get outProfile

            if (jprof == nullptr) {
                if (settings->verbose) {
                    printf("\"%s\" ICC output profile not found!\n - use LCMS2 "
                           "substitution\n",
                           params.icm.outputProfile.c_str());
                }
            } else {
                if (settings->verbose) {
                    printf("Using \"%s\" output profile\n",
                           params.icm.outputProfile.c_str());
                }

                ProfileContent pc = ICCStore::getInstance()->getContent(
                    params.icm.outputProfile);
                readyImg->setOutputProfile(pc.getData().c_str(),
                                           pc.getData().size());
            }
        } else if (params.icm.outputProfile ==
                   ColorManagementParams::NoProfileString) {
            cmsHPROFILE wp = ICCStore::getInstance()->workingSpace(
                params.icm.workingProfile);
            ProfileContent wpc(wp);
            readyImg->setOutputProfile(wpc.getData().c_str(),
                                       wpc.getData().size());
        } else {
            // No ICM
            readyImg->setOutputProfile(nullptr, 0);
        }

        if (!job->initialImage) {
            ii->decreaseRef();
        }

        delete job;

        if (pl) {
            pl->setProgress(0.75);
        }

        return readyImg;
    }

    void stage_early_resize()
    {
        procparams::ProcParams &params = job->pparams;
        ImProcFunctions &ipf = *(ipf_p.get());

        if (scale_factor == 1.f) {
            return;
        }

        int imw = fw * scale_factor + 0.5;
        int imh = fh * scale_factor + 0.5;

        // if (params.crop.enabled) {
        //     int cx = params.crop.x;
        //     int cy = params.crop.y;
        //     int cw = params.crop.w;
        //     int ch = params.crop.h;

        //     params.crop.x = cx * scale_factor + 0.5;
        //     params.crop.y = cy * scale_factor + 0.5;
        //     params.crop.w = cw * scale_factor + 0.5;
        //     params.crop.h = ch * scale_factor + 0.5;
        // }

        assert(params.resize.enabled);

        // resize image
        bool allow_upscaling =
            params.resize.allowUpscaling || params.resize.dataspec == 0;
        if (allow_upscaling || (imw <= fw && imh <= fh)) {
            Imagefloat *resized = new Imagefloat(imw, imh, img);
            { ART_PROFILE_SCOPE("resize:early");
              ipf.Lanczos(img, resized, scale_factor); }
            delete img;
            img = resized;
        }

        // params.resize.enabled = false;

        fw = imw;
        fh = imh;
    }

    // With a crop, process only the source region it needs, as
    // Crop::setCropSizes() does. Sets pp to that region. Returns false when
    // the full frame is needed.
    bool setup_crop_region(bool is_fast)
    {
        const procparams::ProcParams &params = job->pparams;
        ImProcFunctions &ipf = *(ipf_p.get());

        if (!params.crop.enabled || is_fast || ipf.stage0NeedsFullImage()) {
            return false;
        }
        // Not crop-local: denoise would change near the edges of the region,
        // and film negative takes its reference medians over the image (older
        // profiles). removeSpots() can work on a region, as in the editor, but
        // that is not tested here yet.
        if (params.denoise.enabled || params.filmNegative.enabled ||
            (params.spot.enabled && !params.spot.entries.empty())) {
            return false;
        }
        // transformLuminanceOnly() and transformLCPCAOnly() expect source and
        // destination to have the same origin; perspective would need the
        // whole frame anyway (the editor widens by 100%)
        if (params.vignetting.enabled || params.perspective.enabled ||
            (params.lensProf.needed() && params.lensProf.useCA)) {
            return false;
        }
        // auto distortion is computed later, in stage_transform()
        if (params.distortion.enabled && params.distortion.autocompute) {
            return false;
        }

        int x1 = params.crop.x;
        int y1 = params.crop.y;
        int x2 = x1 + params.crop.w;
        int y2 = y1 + params.crop.h;

        if (ipf.needsTransform()) {
            constexpr int border = 32; // as Crop::borderRequested
            const int bx1 = std::max(x1 - border, 0);
            const int by1 = std::max(y1 - border, 0);
            const int bx2 = std::min(x2 + border, fw);
            const int by2 = std::min(y2 + border, fh);
            int sx, sy, sw, sh;
            ipf.transCoord(fw, fh, bx1, by1, bx2 - bx1, by2 - by1, sx, sy, sw,
                           sh);
            x1 = sx;
            y1 = sy;
            x2 = sx + sw;
            y2 = sy + sh;
            if (params.lensProf.useDist && params.lensProf.needed()) {
                // as check_need_larger_crop_for_transform() in dcrop.cc
                const int dw = int(fw * 0.15 / 2);
                const int dh = int(fh * 0.15 / 2);
                x1 -= dw;
                y1 -= dh;
                x2 += dw;
                y2 += dh;
            }
            x1 = std::max(x1, 0);
            y1 = std::max(y1, 0);
            x2 = std::min(x2, fw);
            y2 = std::min(y2, fh);
        }

        if (double(x2 - x1) * (y2 - y1) >= double(fw) * fh) {
            return false; // nothing to gain
        }

        pp = PreviewProps(x1, y1, x2 - x1, y2 - y1, 1);
        if (settings->verbose) {
            printf("Processing only the region %d,%d %dx%d of %dx%d needed "
                   "for the crop\n",
                   x1, y1, x2 - x1, y2 - y1, fw, fh);
        }
        return true;
    }

    void adjust_procparams(double scale_factor)
    {
        procparams::ProcParams &params = job->pparams;
        procparams::ProcParams defaultparams;

        ImProcFunctions &ipf = *(ipf_p.get());
        pipeline_scale = 1.0 / scale_factor;
        ipf.setScale(pipeline_scale);

        if (params.raw.xtranssensor.method ==
            procparams::RAWParams::XTransSensor::Method::THREE_PASS) {
            params.raw.xtranssensor.method =
                procparams::RAWParams::XTransSensor::Method::ONE_PASS;
        }

        if (params.raw.bayersensor.method ==
            procparams::RAWParams::BayerSensor::Method::PIXELSHIFT) {
            params.raw.bayersensor.method =
                procparams::RAWParams::BayerSensor::Method::AMAZE;
        }
    }

private:
    ProcessingJobImpl *job;
    int &errorCode;
    ProgressListener *pl;
    bool flush;

    // internal state
    std::unique_ptr<ImProcFunctions> ipf_p;
    InitialImage *ii;
    ImageSource *imgsrc;
    int fw;
    int fh;
    double scale_factor;

    int tr;
    PreviewProps pp;

    ImProcFunctions::DenoiseInfoStore dnstore;

    ColorTemp currWB;
    Imagefloat *img;

    double pipeline_scale;
    bool stop;
    bool crop_region; // pp is the source region of the crop
};

} // namespace

IImagefloat *processImage(ProcessingJob *pjob, int &errorCode,
                          ProgressListener *pl, bool flush)
{
    PipelineTimeReport report("export");
    ImageProcessor proc(pjob, errorCode, pl, flush);
    IImagefloat *res = proc();
    if (pl) {
        pl->pipelineTimes(report.times());
    }
    /* One table per processed image, so a batch run gives a profile per file
     * rather than a single blended one.  No-op unless ART_PROFILE is set. */
    PipelineProfile::report("OUTPUT pipeline");
    return res;
}

void batchProcessingThread(ProcessingJob *job, BatchProcessingListener *bpl)
{

    ProcessingJob *currentJob = job;

    while (currentJob) {
        auto p = bpl->getBatchProfile();
        if (p &&
            static_cast<ProcessingJobImpl *>(currentJob)->use_batch_profile) {
            p->applyTo(static_cast<ProcessingJobImpl *>(currentJob)->pparams);
        }

        int errorCode;
        IImagefloat *img = processImage(currentJob, errorCode, bpl, true);

        if (errorCode) {
            bpl->error(art::utils::M("MAIN_MSG_CANNOTLOAD"));
            currentJob = nullptr;
        } else {
            try {
                currentJob = bpl->imageReady(img);
            } catch (Glib::Exception &ex) {
                bpl->error(ex.what());
                currentJob = nullptr;
            }
        }
    }
}

void startBatchProcessing(ProcessingJob *job, BatchProcessingListener *bpl)
{
    if (bpl) {
        ThreadPool::add_task(
            ThreadPool::Priority::NORMAL,
            sigc::bind(sigc::ptr_fun(batchProcessingThread), job, bpl));
    }
}

}} // namespace art::engine
