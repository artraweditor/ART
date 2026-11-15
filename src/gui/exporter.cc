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

#include "exporter.h"
#include "../engine/iimage.h"
#include "../engine/imgiomanager.h"

namespace art { namespace gui {

void normalizeExportOptions(ExportOptions &opts)
{
    if (opts.format.empty()) {
        opts.format = "jpg";
    }
    if (opts.bits == -1) {
        if (opts.format == "jpg" || opts.format == "png") {
            opts.bits = 8;
        } else if (opts.format == "tif") {
            opts.bits = 16;
        } else {
            opts.bits = 32;
        }
    }
}


void applyExportProfile(art::engine::procparams::ProcParams &params,
                        const ExportOptions &opts)
{
    auto p = art::engine::ImageIOManager::getInstance()->getSaveProfile(
        opts.format);
    if (p) {
        p->applyTo(params);
    }
}


int runExportJob(art::engine::ProcessingJob *job, const ExportOptions &opts,
                 const Glib::ustring &outfile,
                 art::engine::ProgressListener *pl, int &w, int &h)
{
    int errorCode = 0;
    art::engine::IImagefloat *img =
        art::engine::processImage(job, errorCode, pl);
    if (!img) {
        art::engine::ProcessingJob::destroy(job);
        return 1;
    }
    w = img->getWidth();
    h = img->getHeight();

    if (opts.format == "jpg") {
        errorCode = img->saveAsJPEG(outfile, opts.jpegQuality, opts.jpegSubSamp);
    } else if (opts.format == "tif") {
        errorCode = img->saveAsTIFF(outfile, opts.bits, opts.isFloat,
                                    opts.tiffUncompressed);
    } else if (opts.format == "png") {
        errorCode = img->saveAsPNG(outfile, opts.bits);
    } else {
        errorCode = art::engine::ImageIOManager::getInstance()->save(
                        img, opts.format, outfile, nullptr)
                        ? 0
                        : 1;
    }
    img->free();
    return errorCode ? 2 : 0;
}

}} // namespace art::gui
