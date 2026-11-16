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
#include "../rtengine.h"
#include "json.h"
#include <glibmm/ustring.h>

namespace art { namespace engine { namespace remote {

// The metadata of an image as the metadata panel of the GUI shows it, as a
// JSON object whose keys are the tag names:
//
//  - the Exif tags read by exiv2 ("Exif.Photo.ISOSpeedRatings", ...), with
//    the basic tags filled in from `idata` and, if `edits`, the edits and
//    overrides of `params` applied (the Exif edits and the PPI of the Resize
//    tool, which the .arp of the image holds), as the panel does. Values keep their type: integers
//    are JSON integers, rationals and floating point values are numbers
//    (null for a rational with a zero denominator), text is a string, and a
//    tag with several values is an array. "undefined" values are a string if
//    they are printable text, otherwise an array of bytes. Binary blobs
//    (non-text values of 256 bytes or more) are left out;
//  - if `makernotes`, the maker notes read by ExifTool
//    ("ExifTool.MakerNotes.<name>"): always strings.
//
// `idata` can be null. Throws std::runtime_error if the file cannot be read.
json::Value metadataToJson(const Glib::ustring &filename,
                           const FramesMetaData *idata,
                           const procparams::ProcParams &params, bool edits,
                           bool makernotes);

}}} // namespace art::engine::remote
