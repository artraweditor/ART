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

#include <glibmm/ustring.h>
#include <string>

namespace art { namespace engine {

struct ThumbnailFileResult {
    int width = 0;
    int height = 0;
    // where the pixels come from: "embedded" (the preview of a raw file),
    // "raw" (a quick rendering of the raw) or "image" (a non-raw file)
    std::string source;
    // true if the profile was applied
    bool profile = false;
};

/**
 * Write to `outfile` (PNG or JPEG, by extension; sRGB) a thumbnail of
 * `infile` whose longest edge is at most `size` pixels: smaller if the best
 * source is smaller (it is never enlarged).
 *
 * If `sidecar` is not empty and exists, its profile is applied (as the
 * thumbnails of the file browser do: a quick rendering of the raw, or the
 * image reduced, then processed). Otherwise, for a raw file the preview
 * embedded in it is used, which is the fastest way, and if it has none the
 * raw is rendered quickly with the neutral profile; a non-raw file is just
 * reduced.
 *
 * @return false on failure, setting `err`
 */
bool makeThumbnailFile(const Glib::ustring &infile,
                       const Glib::ustring &outfile, int size,
                       const Glib::ustring &sidecar, ThumbnailFileResult &res,
                       Glib::ustring &err);

}} // namespace art::engine
