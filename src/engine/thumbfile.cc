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
#include "thumbfile.h"
#include "../utils/pathutils.h"
#include "image8.h"
#include "imagedata.h"
#include "imgiomanager.h"
#include "procparams.h"
#include "rawimage.h"
#include "rtthumbnail.h"
#include <algorithm>
#include <giomm.h>
#include <memory>

namespace art { namespace engine {

namespace {

namespace pp = procparams;

bool is_raw_file(const Glib::ustring &fname)
{
    // as ART-cli decides whether a file is a raw
    const Glib::ustring ext = utils::getExtension(fname).lowercase();
    return !(ext == "jpg" || ext == "jpeg" || ext == "tif" || ext == "tiff" ||
             ext == "png" || ImageIOManager::getInstance()->canLoad(ext));
}


// the preview embedded in a raw file, reduced (never enlarged) to fit `size`
Image8 *embedded_preview(const Glib::ustring &fname, int size)
{
    std::unique_ptr<RawImage> ri(new RawImage(fname));
    // metadata only: the file is mapped, not read
    if (ri->loadRaw(false, 0, false)) {
        return nullptr;
    }
    // the JPEG can be decoded at 1/2, 1/4 or 1/8 of its size: ask for the
    // smallest result that still has `size` pixels on its longest edge
    int hw = size, hh = size;
    const int tw = ri->get_thumbWidth(), th = ri->get_thumbHeight();
    if (tw > 0 && th > 0) {
        if (tw >= th) {
            hh = std::max(1, size * th / tw);
        } else {
            hw = std::max(1, size * tw / th);
        }
    }
    std::unique_ptr<Image8> img(ri->getThumbnail(hw, hh));
    if (!img || img->getWidth() < 1 || img->getHeight() < 1) {
        return nullptr;
    }
    if (ri->get_rotateDegree() > 0 && ri->thumbNeedsRotation()) {
        img->rotate(ri->get_rotateDegree());
    }
    const int w = img->getWidth(), h = img->getHeight();
    const int m = std::max(w, h);
    if (m > size) {
        const int nw = std::max(1, int(int64_t(w) * size / m));
        const int nh = std::max(1, int(int64_t(h) * size / m));
        return Thumbnail::resizeTo<Image8>(nw, nh, TI_Bilinear, img.get());
    }
    return img.release();
}


// a rendering of the raw or of the image with the params
Image8 *render(const Glib::ustring &fname, bool raw, int size,
               pp::ProcParams params)
{
    eSensorType sensor = ST_NONE;
    int w = size, h = size;
    std::unique_ptr<Thumbnail> tpp;
    if (raw) {
        // the height is what is fixed: the longest edge is at least `size`
        tpp.reset(Thumbnail::loadFromRaw(fname, sensor, w, h, 1,
                                         params.wb.equal, true));
    } else {
        tpp.reset(Thumbnail::loadFromImage(fname, w, h, -1, params.wb.equal));
    }
    if (!tpp) {
        return nullptr;
    }

    // always sRGB, as the previews
    params.icm.outputProfile = "RTv2_sRGB";
    params.icm.outputIntent = RI_RELATIVE;
    params.icm.outputBPC = true;

    // do not enlarge: an image is loaded at the size of the box even if it is
    // smaller (`sc` is the ratio of the original to the loaded one)
    int edge = size;
    if (!raw) {
        int iw, ih;
        double sc = 1.0;
        tpp->getDimensions(iw, ih, sc);
        edge = std::max(1, std::min(size, int(std::max(iw, ih) * sc + 0.5)));
    }
    // the height of the result, for the longest edge to be `edge`
    float ratio = 1.f;
    int rheight = edge;
    if (tpp->getImageWidth(params, edge, ratio) > edge && ratio > 0.f) {
        rheight = std::max(1, int(edge / ratio));
    }

    FramesData md(fname);
    double scale = 1.0;
    IImage8 *res = tpp->processImage(params, sensor, rheight, TI_Bilinear, &md,
                                     scale, false);
    return dynamic_cast<Image8 *>(res);
}

} // namespace


bool makeThumbnailFile(const Glib::ustring &infile,
                       const Glib::ustring &outfile, int size,
                       const Glib::ustring &sidecar, ThumbnailFileResult &res,
                       Glib::ustring &err)
{
    if (size < 1) {
        err = "invalid size";
        return false;
    }
    if (!Glib::file_test(infile, Glib::FILE_TEST_IS_REGULAR)) {
        err = "no such file: " + infile;
        return false;
    }

    pp::ProcParams params;
    res.profile = !sidecar.empty() &&
                  Glib::file_test(sidecar, Glib::FILE_TEST_IS_REGULAR) &&
                  params.load(nullptr, sidecar) == 0;
    if (!res.profile) {
        params.setDefaults();
    }

    const bool raw = is_raw_file(infile);
    std::unique_ptr<Image8> img;
    if (raw && !res.profile) {
        img.reset(embedded_preview(infile, size));
        if (img) {
            res.source = "embedded";
        }
    }
    if (!img) {
        img.reset(render(infile, raw, size, params));
        res.source = raw ? "raw" : "image";
    }
    if (!img) {
        err = "cannot read " + infile;
        return false;
    }

    res.width = img->getWidth();
    res.height = img->getHeight();
    const auto dot = outfile.rfind('.');
    const Glib::ustring e =
        outfile.substr(dot == Glib::ustring::npos ? outfile.size() : dot + 1)
            .lowercase();
    const int rc = (e == "jpg" || e == "jpeg") ? img->saveAsJPEG(outfile, 90)
                                               : img->saveAsPNG(outfile, 8);
    if (rc != 0) {
        err = "cannot write " + outfile;
        return false;
    }
    return true;
}

}} // namespace art::engine
