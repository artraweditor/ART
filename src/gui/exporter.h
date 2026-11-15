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

#include "../engine/rtengine.h"
#include <glibmm/ustring.h>
#include <string>

namespace art { namespace gui {

// Output of a processed image to a file: what ART-cli does after the
// parameters of an image are known, shared with the export command of the
// server mode so that both write the same file.
struct ExportOptions {
    // "jpg", "tif", "png" or the extension of an extra format of
    // ImageIOManager; "jpg" if empty
    std::string format;
    int jpegQuality = 92;
    int jpegSubSamp = 3; // 1 = best compression, 3 = best quality
    // bits per channel; -1 = the default of the format (jpg, png: 8;
    // tif: 16; others: 32)
    int bits = -1;
    bool isFloat = false;
    bool tiffUncompressed = true;
    bool fast = false; // the fast export pipeline (ART-cli -f)
};

// fill in what is left to default (format, bits)
void normalizeExportOptions(ExportOptions &opts);

// merge the profile that goes with the output format (if any) into `params`
void applyExportProfile(art::engine::procparams::ProcParams &params,
                        const ExportOptions &opts);

// Render `job` (which becomes invalid) and write the result to `outfile`.
// `opts` must be normalised. Returns 0 on success; otherwise 1 if the
// processing failed and 2 if the file could not be written. On success
// `w` and `h` are the size of the image.
int runExportJob(art::engine::ProcessingJob *job, const ExportOptions &opts,
                 const Glib::ustring &outfile,
                 art::engine::ProgressListener *pl, int &w, int &h);

}} // namespace art::gui
