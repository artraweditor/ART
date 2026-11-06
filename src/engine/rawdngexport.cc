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
 *  along with ART.  If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * Raw preprocessing only: run the pipeline up to (and optionally including)
 * demosaicing and save the result as a DNG.
 */

#include "../utils/version.h"
#include "colortemp.h"
#include "imagesource.h"
#include "lensexif.h"
#include "metadata.h"
#include "procparams.h"
#include "rawimagesource.h"
#include "rtengine.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <tiff.h>
#include <tiffio.h>

namespace art { namespace engine {

using namespace procparams;

namespace {

using OpcodeLists = std::array<std::vector<unsigned char>, 3>;

constexpr unsigned OPCODE_GAINMAP = 9;
constexpr unsigned OPCODE_FIXVIGNETTERADIAL = 3;

// the DNG opcode lists of the input, if any (in the SubIFD of the raw data or
// in the IFD0). They are read as raw bytes
OpcodeLists read_opcodes(const Exiv2Metadata &md)
{
    OpcodeLists ret;
    for (const auto &e : md.exifData()) {
        const int n = int(e.tag()) - 51008; // OpcodeList1, 2, 3 (51008, 9, 22)
        const int idx = e.tag() == 51022 ? 2 : (n == 0 || n == 1 ? n : -1);
        if (idx < 0 || !ret[idx].empty() || e.count() == 0 ||
            !(e.groupName().compare(0, 8, "SubImage") == 0 ||
              e.groupName() == "Image")) {
            continue;
        }
        std::vector<Exiv2::byte> buf(e.value().size());
        e.value().copy(&buf[0], Exiv2::invalidByteOrder);
        ret[idx].assign(buf.begin(), buf.end());
    }
    return ret;
}

// remove from an opcode list the opcodes with the given ids
void drop_opcodes(std::vector<unsigned char> &list,
                  const std::vector<unsigned> &ids)
{
    const auto get32 = [&](size_t i) -> uint32_t {
        return (uint32_t(list[i]) << 24) | (uint32_t(list[i + 1]) << 16) |
               (uint32_t(list[i + 2]) << 8) | uint32_t(list[i + 3]);
    };
    if (list.size() < 4) {
        return;
    }
    const uint32_t n = get32(0);
    std::vector<unsigned char> out(4);
    uint32_t kept = 0;
    size_t idx = 4;
    for (uint32_t i = 0; i < n; ++i) {
        if (idx + 16 > list.size()) {
            return; // malformed: leave it alone
        }
        const uint32_t id = get32(idx);
        const size_t end = idx + 16 + get32(idx + 12);
        if (end > list.size()) {
            return;
        }
        if (std::find(ids.begin(), ids.end(), id) == ids.end()) {
            out.insert(out.end(), list.begin() + idx, list.begin() + end);
            ++kept;
        }
        idx = end;
    }
    for (int i = 0; i < 4; ++i) {
        out[i] = (kept >> (8 * (3 - i))) & 0xff;
    }
    list = kept ? out : std::vector<unsigned char>();
}

bool write_dng(const RawImageSource::PreprocessedRaw &raw,
               const RawDNGOptions &opts, const std::string &make,
               const std::string &model,
               const OpcodeLists &opcodes,
               const Glib::ustring &fname, ProgressListener *pl)
{
    const int W = raw.width;
    const int H = raw.height;
    const int spp = raw.demosaiced ? 3 : 1;
    const bool is_float = opts.bits == 32;

    TIFF *out = TIFFOpen(fname.c_str(), "w");
    if (!out) {
        return false;
    }

    // clip level. The "R G1 B G2" values of the raw are all 65535 when no WB
    // is baked in; otherwise only the channel with the largest multiplier
    // saturates there. A DNG can store one clip level per sample (linear
    // DNG), but readers (LibRaw, so ART) fold the differences into the white
    // balance and then scale the data wrongly, so a single level is used for
    // both kinds of DNG: the lowest (no colour cast, but the highlights of the
    // other channels are lost) or, with --dng-clip=max, the highest
    const double mn =
        std::min({raw.clip[0], raw.clip[1], raw.clip[2], raw.clip[3]});
    const double mx =
        std::max({raw.clip[0], raw.clip[1], raw.clip[2], raw.clip[3]});
    const double cfa_wl = std::min(opts.clip_max ? mx : mn, 65535.0);

    const unsigned char dng_version[4] = {1, 4, 0, 0};
    const unsigned char dng_backward[4] = {1, 1, 0, 0};

    TIFFSetField(out, TIFFTAG_SUBFILETYPE, 0);
    TIFFSetField(out, TIFFTAG_SOFTWARE, RTNAME " " RTVERSION);
    if (!make.empty()) {
        TIFFSetField(out, TIFFTAG_MAKE, make.c_str());
    }
    if (!model.empty()) {
        TIFFSetField(out, TIFFTAG_MODEL, model.c_str());
    }
    TIFFSetField(out, TIFFTAG_IMAGEWIDTH, W);
    TIFFSetField(out, TIFFTAG_IMAGELENGTH, H);
    int orientation = 1;
    switch (raw.rotate) {
    case 90: orientation = 6; break;
    case 180: orientation = 3; break;
    case 270: orientation = 8; break;
    default: break;
    }
    TIFFSetField(out, TIFFTAG_ORIENTATION, orientation);
    TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, spp);
    TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, opts.bits);
    TIFFSetField(out, TIFFTAG_SAMPLEFORMAT,
                 is_float ? SAMPLEFORMAT_IEEEFP : SAMPLEFORMAT_UINT);
    TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(out, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, 1);
    TIFFSetField(out, TIFFTAG_PHOTOMETRIC,
                 raw.demosaiced ? 34892 /* LinearRaw */ : PHOTOMETRIC_CFA);

    TIFFSetField(out, TIFFTAG_DNGVERSION, dng_version);
    TIFFSetField(out, TIFFTAG_DNGBACKWARDVERSION, dng_backward);
    std::string unique = make.empty() ? model : (make + " " + model);
    if (!unique.empty()) {
        TIFFSetField(out, TIFFTAG_UNIQUECAMERAMODEL, unique.c_str());
    }

    if (!raw.demosaiced) {
        const uint16_t dim[2] = {uint16_t(raw.cfa_size),
                                 uint16_t(raw.cfa_size)};
        unsigned char pat[36];
        for (int i = 0; i < raw.cfa_size; ++i) {
            for (int j = 0; j < raw.cfa_size; ++j) {
                pat[i * raw.cfa_size + j] = (unsigned char)raw.cfa[i][j];
            }
        }
        TIFFSetField(out, TIFFTAG_CFAREPEATPATTERNDIM, dim);
        TIFFSetField(out, TIFFTAG_CFAPATTERN, raw.cfa_size * raw.cfa_size,
                     pat);
        // CFAPlaneColor is the default (R, G, B)
    }

    {
        // not known to libtiff
        static const TIFFFieldInfo opcode_info[] = {
            {TIFFTAG_OPCODELIST1, TIFF_VARIABLE2, TIFF_VARIABLE2,
             TIFF_UNDEFINED, FIELD_CUSTOM, 1, 1,
             const_cast<char *>("OpcodeList1")},
            {TIFFTAG_OPCODELIST2, TIFF_VARIABLE2, TIFF_VARIABLE2,
             TIFF_UNDEFINED, FIELD_CUSTOM, 1, 1,
             const_cast<char *>("OpcodeList2")},
            {TIFFTAG_OPCODELIST3, TIFF_VARIABLE2, TIFF_VARIABLE2,
             TIFF_UNDEFINED, FIELD_CUSTOM, 1, 1,
             const_cast<char *>("OpcodeList3")}};
        TIFFMergeFieldInfo(out, opcode_info, 3);
        const ttag_t tags[3] = {TIFFTAG_OPCODELIST1, TIFFTAG_OPCODELIST2,
                                TIFFTAG_OPCODELIST3};
        for (int i = 0; i < 3; ++i) {
            if (!opcodes[i].empty()) {
                TIFFSetField(out, tags[i], uint32_t(opcodes[i].size()),
                             opcodes[i].data());
            }
        }
    }

    // levels: black is always 0
    const double data_scale = is_float ? 1.0 / 65535.0 : 1.0;
    if (is_float) {
        // float DNG samples are nominally in 0..1
        uint32_t one = 1;
        TIFFSetField(out, TIFFTAG_WHITELEVEL, 1, &one);
    } else {
        const uint32_t w1 = uint32_t(std::lround(cfa_wl));
        const uint32_t w[3] = {w1, w1, w1};
        TIFFSetField(out, TIFFTAG_WHITELEVEL, spp, w);
    }

    // colour: raw.cam_xyz maps XYZ to the camera balanced with pre_mul (d).
    // The data is multiplied by the gains g (baked) or not at all (uniform),
    // so the matrix has to be adjusted. The neutral is the true camera white,
    // 1/wb, in the space of the data; the baked gains can differ from wb
    // (see RawImageSource::getPreprocessedRaw)
    float cm[9];
    float neutral[3];
    for (int c = 0; c < 3; ++c) {
        const double s = (raw.uniform ? 1.0 : raw.gain[c]) / raw.pre_mul[c];
        for (int k = 0; k < 3; ++k) {
            cm[3 * c + k] = float(s * raw.cam_xyz[c][k]);
        }
        neutral[c] = (raw.uniform ? 1.0 : raw.gain[c]) / raw.wb[c];
    }
    const float ng = neutral[1];
    for (int c = 0; c < 3; ++c) {
        neutral[c] /= ng;
    }
    TIFFSetField(out, TIFFTAG_COLORMATRIX1, 9, cm);
    TIFFSetField(out, TIFFTAG_ASSHOTNEUTRAL, 3, neutral);
    TIFFSetField(out, TIFFTAG_CALIBRATIONILLUMINANT1, 21); // D65

    const size_t row_samples = size_t(W) * spp;
    std::vector<uint16_t> row16(is_float ? 0 : row_samples);
    std::vector<float> rowf(is_float ? row_samples : 0);
    bool ok = true;
    for (int y = 0; y < H && ok; ++y) {
        const float *src = &raw.data[size_t(y) * row_samples];
        void *buf;
        if (is_float) {
            for (size_t i = 0; i < row_samples; ++i) {
                rowf[i] = src[i] * data_scale;
            }
            buf = rowf.data();
        } else {
            for (size_t i = 0; i < row_samples; ++i) {
                const double lim = cfa_wl;
                row16[i] = uint16_t(
                    std::lround(std::min<double>(std::max(src[i], 0.f), lim)));
            }
            buf = row16.data();
        }
        ok = TIFFWriteScanline(out, buf, y, 0) >= 0;
        if (pl && !(y & 255)) {
            pl->setProgress(0.5 + 0.5 * double(y) / H);
        }
    }
    TIFFClose(out);
    return ok;
}

} // namespace


int processRawToDNG(InitialImage *ii, const ProcParams &params,
                    const RawDNGOptions &opts, const Glib::ustring &outfile,
                    ProgressListener *pl)
{
    ImageSource *imgsrc = ii->getImageSource();
    auto *rs = dynamic_cast<RawImageSource *>(imgsrc);
    const bool xtrans = imgsrc->getSensorType() == ST_FUJI_XTRANS;
    if (!rs || (imgsrc->getSensorType() != ST_BAYER && !xtrans)) {
        std::cerr << "raw preprocessing to DNG is supported only for Bayer "
                     "and X-Trans raw files"
                  << std::endl;
        return 1;
    }

    if (pl) {
        pl->setProgressStr("PROGRESSBAR_PROCESSING");
        pl->setProgress(0.0);
    }

    const RAWParams &rp = params.raw;
    if (xtrans) {
        imgsrc->setBorder(rp.xtranssensor.border);
    } else if (rp.bayersensor.method !=
               RAWParams::BayerSensor::Method::PIXELSHIFT) {
        imgsrc->setBorder(rp.bayersensor.border);
    } else {
        imgsrc->setBorder(std::max(rp.bayersensor.border, 2));
    }
    imgsrc->setCurrentFrame(rp.bayersensor.imageNum);

    // same WB selection as in the normal pipeline (simpleprocess.cc)
    ColorTemp currWB;
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
            currWB = ColorTemp(rm, gm, bm);
        } break;
        case WBParams::AUTO:
        default:
            currWB = ColorTemp();
        }
    }

    rs->setUniformScale(opts.neutral);
    imgsrc->preprocess(rp, params.lensProf, params.coarse,
                       params.denoise.enabled, currWB);
    if (pl) {
        pl->setProgress(0.25);
    }
    if (opts.demosaic) {
        const bool autoContrast =
            xtrans ? rp.xtranssensor.dualDemosaicAutoContrast
                   : rp.bayersensor.dualDemosaicAutoContrast;
        double contrastThreshold = xtrans
                                       ? rp.xtranssensor.dualDemosaicContrast
                                       : rp.bayersensor.dualDemosaicContrast;
        imgsrc->demosaic(rp, autoContrast, contrastThreshold);
    }
    if (pl) {
        pl->setProgress(0.5);
    }

    RawImageSource::PreprocessedRaw raw;
    if (!rs->getPreprocessedRaw(raw, opts.demosaic)) {
        std::cerr << "cannot retrieve the preprocessed raw data" << std::endl;
        return 2;
    }

    if (!opts.neutral && opts.bits == 32) {
        const double mn = std::min({raw.clip[0], raw.clip[1], raw.clip[2]});
        if (mn < 65535.0 * 0.999) {
            std::cerr << "warning: float DNG with white balance baked in: "
                         "per-channel clipping levels cannot be stored; "
                         "highlights may get a colour cast (use "
                         "--dng-wb=neutral or --dng-bits=16)"
                      << std::endl;
        }
    }

    const FramesMetaData *md = ii->getMetaData();
    const std::string make = md ? md->getMake() : "";
    const std::string model = md ? md->getModel() : "";
    // the vignetting correction is part of the preprocessing: don't describe
    // it again if it has already been applied to the data
    const bool vign_applied =
        params.lensProf.useVign &&
        params.lensProf.lcMode != LensProfParams::LcMode::NONE;

    OpcodeLists opcodes;
    Exiv2Metadata meta(imgsrc->getFileName());
    bool has_meta = true;
    try {
        meta.load();
    } catch (std::exception &exc) {
        std::cerr << "warning: cannot read the metadata: " << exc.what()
                  << std::endl;
        has_meta = false;
    }
    if (md && md->isDNG()) {
        // keep the opcodes of the input, except for the corrections that
        // have been applied to the data by the preprocessing
        if (has_meta) {
            opcodes = read_opcodes(meta);
        }
        if (rp.ff_embedded) { // gain maps
            drop_opcodes(opcodes[1], {OPCODE_GAINMAP});
        }
        if (vign_applied) {
            drop_opcodes(opcodes[2], {OPCODE_FIXVIGNETTERADIAL});
        }
    } else if (opts.lens_opcodes && md) {
        ExifLensCorrection lc(md, raw.width, raw.height, CoarseTransformParams(),
                              -1);
        if (!lc.getDNGOpcodes(opcodes[2], raw.demosaiced ? 3 : 1,
                              !vign_applied)) {
            std::cerr << "note: no embedded lens correction to convert to DNG "
                         "opcodes"
                      << std::endl;
        }
    }
    if (!write_dng(raw, opts, make, model, opcodes, outfile, pl)) {
        std::cerr << "cannot write " << outfile << std::endl;
        return 3;
    }
    // copy the metadata (exposure, ISO, lens, ...), as the later processing
    // depends on it (e.g. the lens correction). The opcodes have already
    // been written
    if (has_meta) {
        try {
            auto &exif = meta.exifData();
            for (auto it = exif.begin(); it != exif.end();) {
                // drop the DNG-specific tags of the source (WhiteLevel,
                // BlackLevel, ActiveArea, ColorMatrix, AsShotNeutral,
                // illuminants, opcodes, ...): write_dng has already written
                // the correct ones for the preprocessed data, and saveToImage
                // would overwrite them. Keep only the lens info.
                if (it->groupName() == "Image" && it->tag() >= 50706 &&
                    it->tag() <= 52000 && it->tag() != 50736) {
                    it = exif.erase(it);
                } else {
                    ++it;
                }
            }
            meta.saveToImage(pl, outfile, false);
        } catch (std::exception &exc) {
            std::cerr << "warning: cannot copy the metadata: " << exc.what()
                      << std::endl;
        }
    }
    if (pl) {
        pl->setProgress(1.0);
    }
    return 0;
}

}} // namespace art::engine
