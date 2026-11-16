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

#include "metadata_json.h"
#include "../imagedata.h"
#include "../metadata.h"
#include <cmath>
#include <exiv2/exiv2.hpp>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace art { namespace engine { namespace remote {

namespace {

json::Value rational_value(const Exiv2::Rational &r)
{
    if (r.second == 0) {
        return json::Value();
    }
    return json::Value(double(r.first) / double(r.second));
}


json::Value rational_value(const Exiv2::URational &r)
{
    if (r.second == 0) {
        return json::Value();
    }
    return json::Value(double(r.first) / double(r.second));
}


// the value `n` of a numeric datum, with the type of the datum
json::Value numeric_value(const Exiv2::Exifdatum &d, size_t n)
{
    switch (d.typeId()) {
    case Exiv2::unsignedRational: {
        auto v = dynamic_cast<const Exiv2::ValueType<Exiv2::URational> *>(
            &d.value());
        if (v && n < v->value_.size()) {
            return rational_value(v->value_[n]);
        }
        return rational_value(d.toRational(n));
    }
    case Exiv2::signedRational:
        return rational_value(d.toRational(n));
    case Exiv2::tiffFloat: {
        auto v = dynamic_cast<const Exiv2::ValueType<float> *>(&d.value());
        if (v && n < v->value_.size()) {
            return json::Value(double(v->value_[n]));
        }
        return json::Value(double(d.toFloat(n)));
    }
    case Exiv2::tiffDouble: {
        auto v = dynamic_cast<const Exiv2::ValueType<double> *>(&d.value());
        if (v && n < v->value_.size()) {
            return json::Value(v->value_[n]);
        }
        return json::Value(double(d.toFloat(n)));
    }
    default:
        return json::Value(static_cast<long long>(d.toInt64(n)));
    }
}


// the value of a datum with its type, or false if it is not to be shown
bool datum_value(const Exiv2::Exifdatum &d, json::Value &out)
{
    const auto t = d.typeId();
    if (t != Exiv2::asciiString && d.size() >= 256) {
        return false; // a binary blob
    }
    switch (t) {
    case Exiv2::asciiString:
    case Exiv2::string:
    case Exiv2::date:
    case Exiv2::time:
        out = json::Value(d.toString());
        return true;
    case Exiv2::comment: {
        auto c = dynamic_cast<const Exiv2::CommentValue *>(&d.value());
        out = json::Value(c ? c->comment() : d.toString());
        return true;
    }
    case Exiv2::undefined: {
        std::vector<Exiv2::byte> buf(d.size());
        if (d.size() && d.copy(buf.data(), Exiv2::littleEndian) != d.size()) {
            return false;
        }
        size_t len = buf.size();
        while (len > 0 && buf[len - 1] == 0) {
            --len;
        }
        bool text = len > 0;
        for (size_t i = 0; i < len; ++i) {
            text = text && buf[i] >= 0x20 && buf[i] < 0x7f;
        }
        if (text) {
            out = json::Value(std::string(buf.begin(), buf.begin() + len));
        } else {
            out = json::Value::array();
            for (auto b : buf) {
                out.push(int(b));
            }
        }
        return true;
    }
    case Exiv2::unsignedByte:
    case Exiv2::signedByte:
    case Exiv2::unsignedShort:
    case Exiv2::signedShort:
    case Exiv2::unsignedLong:
    case Exiv2::signedLong:
    case Exiv2::unsignedLongLong:
    case Exiv2::signedLongLong:
    case Exiv2::tiffIfd:
    case Exiv2::tiffIfd8:
    case Exiv2::unsignedRational:
    case Exiv2::signedRational:
    case Exiv2::tiffFloat:
    case Exiv2::tiffDouble: {
        const size_t n = d.count();
        if (n == 1) {
            out = numeric_value(d, 0);
        } else {
            out = json::Value::array();
            for (size_t i = 0; i < n; ++i) {
                out.push(numeric_value(d, i));
            }
        }
        return true;
    }
    default:
        out = json::Value(d.toString());
        return true;
    }
}

} // namespace


json::Value metadataToJson(const Glib::ustring &filename,
                           const FramesMetaData *idata,
                           const procparams::ProcParams &params, bool edits,
                           bool makernotes)
{
    if (filename.empty()) {
        throw std::runtime_error("no file");
    }
    Exiv2::ExifData exif;
    Exiv2Metadata meta(filename);
    try {
        meta.load();
        exif = meta.exifData();
    } catch (std::exception &e) {
        throw std::runtime_error(std::string("cannot read the metadata: ") +
                                 e.what());
    }

    if (auto fd = dynamic_cast<const FramesData *>(idata)) {
        fd->fillBasicTags(exif);
    }

    // the edits of the parameters, then the overrides, as the metadata panel
    procparams::ExifPairs overrides;
    if (edits && params.resize.enabled && params.resize.copyPPIToExif) {
        const Glib::ustring val = Glib::ustring::format(params.resize.ppi) + "/1";
        overrides["Exif.Image.XResolution"] = val;
        overrides["Exif.Image.YResolution"] = val;
        overrides["Exif.Image.ResolutionUnit"] = "2";
    }
    const procparams::ExifPairs *lists[2] = {&params.metadata.exif, &overrides};
    for (auto l : lists) {
        if (!edits) {
            break;
        }
        for (auto &p : *l) {
            try {
                exif[p.first].setValue(p.second);
            } catch (std::exception &) {
                // an invalid edit is ignored (the panel reports it)
            }
        }
    }

    std::map<std::string, const Exiv2::Exifdatum *> sorted;
    for (auto &d : exif) {
        sorted[d.key()] = &d;
    }
    json::Value ret = json::Value::object();
    for (auto &p : sorted) {
        json::Value v;
        try {
            if (datum_value(*p.second, v)) {
                ret.set(p.first, v);
            }
        } catch (std::exception &) {
            // a value that cannot be read is left out
        }
    }

    if (makernotes) {
        auto mn = meta.getMakernotes();
        std::map<std::string, std::string> sorted_mn(mn.begin(), mn.end());
        for (auto &p : sorted_mn) {
            ret.set("ExifTool.MakerNotes." + p.first, p.second);
        }
    }
    return ret;
}

}}} // namespace art::engine::remote
