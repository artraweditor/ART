/* -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright 2020 Alberto Griggio <alberto.griggio@gmail.com>
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

#include "gainmap.h"
#include "metadata.h"
#include "rawimage.h"

namespace rtengine {

bool RawImage::has_gain_map(std::vector<uint8_t> *out_buf) const
{
    if (!(isBayer() && DNGVERSION())) {
        return false;
    }

#ifndef ART_USE_LIBRAW
    if (RT_OpcodeList2_len <= 0) {
        return false;
    }

    std::vector<uint8_t> buf(RT_OpcodeList2_len);
    fseek(ifp, RT_OpcodeList2_start, SEEK_SET);
    if (fread(&buf[0], 1, RT_OpcodeList2_len, ifp) != RT_OpcodeList2_len) {
        return false;
    }

    if (out_buf) {
        out_buf->swap(buf);
    }
    return true;

#else // ART_USE_LIBRAW

    auto md = Exiv2Metadata(filename);
    md.load();
    auto &exif = md.exifData();
    auto it = exif.findKey(Exiv2::ExifKey("Exif.SubImage1.OpcodeList2"));
    if (it == exif.end()) {
        it = exif.findKey(Exiv2::ExifKey("Exif.Image.OpcodeList2"));
    }
    if (it != exif.end()) {
        if (out_buf) {
            std::vector<Exiv2::byte> buf;
            buf.resize(it->value().size());
            it->value().copy(&buf[0], Exiv2::invalidByteOrder);
            out_buf->resize(buf.size());
            for (size_t i = 0; i < buf.size(); ++i) {
                (*out_buf)[i] = uint8_t(buf[i]);
            }
        }
        return true;
    }
    return false;

#endif // ART_USE_LIBRAW
}

} // namespace rtengine
