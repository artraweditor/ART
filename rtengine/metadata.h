/* -*- C++ -*-
 *
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2019 Alberto Griggio <alberto.griggio@gmail.com>
 *
 *  RawTherapee is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RawTherapee is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RawTherapee.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "cache.h"
#include "procparams.h"
#include <exiv2/exiv2.hpp>
#include <glibmm.h>
#include <memory>
#include <unordered_set>

namespace rtengine {

class ProgressListener;
class Exiftool;

long exiv2_to_long(const Exiv2::Metadatum &d);

class Exiv2Metadata {
public:
    Exiv2Metadata();
    explicit Exiv2Metadata(const Glib::ustring &path);
    Exiv2Metadata(const Glib::ustring &path, bool merge_xmp_sidecar);

    void load() const;

    Exiv2::ExifData &exifData()
    {
        return image_.get() ? image_->exifData() : exif_data_;
    }
    const Exiv2::ExifData &exifData() const
    {
        return const_cast<Exiv2Metadata *>(this)->exifData();
    }

    Exiv2::IptcData &iptcData()
    {
        return image_.get() ? image_->iptcData() : iptc_data_;
    }
    const Exiv2::IptcData &iptcData() const
    {
        return const_cast<Exiv2Metadata *>(this)->iptcData();
    }

    Exiv2::XmpData &xmpData()
    {
        return image_.get() ? image_->xmpData() : xmp_data_;
    }
    const Exiv2::XmpData &xmpData() const
    {
        return const_cast<Exiv2Metadata *>(this)->xmpData();
    }

    const Glib::ustring &filename() const { return src_; }
    const rtengine::procparams::ExifPairs &exif() const { return exif_; }
    const rtengine::procparams::IPTCPairs &iptc() const { return iptc_; }
    void setExif(const rtengine::procparams::ExifPairs &exif) { exif_ = exif; }
    void setIptc(const rtengine::procparams::IPTCPairs &iptc) { iptc_ = iptc; }

    void saveToImage(ProgressListener *pl, const Glib::ustring &path,
                     bool preserve_all_tags) const;
    void saveToXmp(const Glib::ustring &path) const;

    void setOutputRating(const rtengine::procparams::ProcParams &pparams,
                         bool from_xmp_sidecar);

    void setExifKeys(const std::vector<std::string> *keys);

    void getDimensions(int &w, int &h) const;
    std::unordered_map<std::string, std::string> getMakernotes() const;

    Exiv2::ExifData getOutputExifData() const;

    static Glib::ustring xmpSidecarPath(const Glib::ustring &path);
    static Exiv2::XmpData getXmpSidecar(const Glib::ustring &path);

    static void init(const Glib::ustring &base_dir,
                     const Glib::ustring &user_dir);
    static void cleanup();

    static void embedProcParamsData(const Glib::ustring &fname,
                                    const std::string &data);

private:
    static std::unordered_map<std::string, std::string>
    getExiftoolMakernotes(const Glib::ustring &path);
    void do_merge_xmp(Exiv2::Image *dst, bool keep_all) const;
    void import_exif_pairs(Exiv2::ExifData &out) const;
    void import_iptc_pairs(Exiv2::IptcData &out) const;
    void remove_unwanted(Exiv2::ExifData &dst) const;

    Glib::ustring src_;
    bool merge_xmp_;
    mutable std::shared_ptr<Exiv2::Image> image_;
    rtengine::procparams::ExifPairs exif_;
    rtengine::procparams::IPTCPairs iptc_;
    Exiv2::ExifData exif_data_;
    Exiv2::IptcData iptc_data_;
    Exiv2::XmpData xmp_data_;
    int rating_;

    std::shared_ptr<std::unordered_set<std::string>> exif_keys_;

    struct CacheVal {
        std::shared_ptr<Exiv2::Image> image;
        Glib::TimeVal image_mtime;
        Glib::TimeVal xmp_mtime;
        bool use_xmp;
        CacheVal(): image(nullptr), image_mtime(), xmp_mtime(), use_xmp(false)
        {
        }
    };
    // typedef std::pair<std::shared_ptr<Exiv2::Image>, Glib::TimeVal> CacheVal;
    typedef Cache<Glib::ustring, CacheVal> ImageCache;
    static std::unique_ptr<ImageCache> cache_;
    typedef std::pair<std::unordered_map<std::string, std::string>,
                      Glib::TimeVal>
        JSONCacheVal;
    typedef Cache<Glib::ustring, JSONCacheVal> JSONCache;
    static std::unique_ptr<JSONCache> jsoncache_;

    static std::unique_ptr<Exiftool> exiftool_;
};

} // namespace rtengine
