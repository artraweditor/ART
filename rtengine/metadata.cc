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

#include <giomm.h>
#include <glib/gstdio.h>
#include <iostream>
#include <mutex>
#include <set>
#include <stdio.h>
#include <unistd.h>

#include "../rtgui/multilangmgr.h"
#include "../rtgui/pathutils.h"
#include "../rtgui/version.h"
#include "cJSON.h"
#include "imagedata.h"
#include "metadata.h"
#include "settings.h"
#include "subprocess.h"

namespace rtengine {

extern const Settings *settings;

std::unique_ptr<Exiv2Metadata::ImageCache> Exiv2Metadata::cache_(nullptr);
std::unique_ptr<Exiv2Metadata::JSONCache> Exiv2Metadata::jsoncache_(nullptr);
std::unique_ptr<Exiftool> Exiv2Metadata::exiftool_(nullptr);

namespace {

#if EXIV2_TEST_VERSION(0, 28, 0)

class Error: public Exiv2::Error {
public:
    Error(const std::string &msg)
        : Exiv2::Error(Exiv2::ErrorCode::kerGeneralError), msg_(msg)
    {
    }
    const char *what() const throw() { return msg_.c_str(); }

private:
    std::string msg_;
};

#else // EXIV2_TEST_VERSION

class Error: public Exiv2::AnyError {
public:
    Error(const std::string &msg): msg_(msg) {}
    const char *what() const throw() { return msg_.c_str(); }
    int code() const throw() { return 0; }

private:
    std::string msg_;
};

#endif // EXIV2_TEST_VERSION

constexpr size_t IMAGE_CACHE_SIZE = 200;

std::unique_ptr<Exiv2::Image> open_exiv2(const Glib::ustring &fname,
                                         bool check_exif)
{
#if defined WIN32 && defined EXV_UNICODE_PATH && !defined ART_WIN32_UCRT
    std::wstring wfname = subprocess::to_wstr(fname);
    auto image = Exiv2::ImageFactory::open(wfname);
#else
    auto image = Exiv2::ImageFactory::open(Glib::filename_from_utf8(fname));
#endif
    image->readMetadata();
    if (!image->good() || (check_exif && image->exifData().empty())) {
#if EXIV2_TEST_VERSION(0, 28, 0)
        auto error_code = Exiv2::ErrorCode::kerErrorMessage;
#elif EXIV2_TEST_VERSION(0, 27, 0)
        auto error_code = Exiv2::kerErrorMessage;
#else
        auto error_code = 1;
#endif
        throw Exiv2::Error(error_code, "exiv2: invalid image");
    }
    std::unique_ptr<Exiv2::Image> ret(image.release());
    return ret;
}

template <class Data, class Key>
void clear_metadata_key(Data &data, const Key &key)
{
    while (true) {
        auto it = data.findKey(key);
        if (it == data.end()) {
            break;
        } else {
            data.erase(it);
        }
    }
}

Glib::ustring exiftool_base_dir;
#ifdef WIN32
const Glib::ustring exiftool_default = "exiftool.exe";
#else
const Glib::ustring exiftool_default = "exiftool";
#endif

Glib::ustring exiftool_config_dir;

const char *exiftool_xmp_config =
    "%Image::ExifTool::UserDefined = (\n"
    "   'Image::ExifTool::XMP::Main' => {\n"
    "       ART => {\n"
    "           SubDirectory => {\n"
    "               TagTable => 'Image::ExifTool::UserDefined::ART',\n"
    "           },\n"
    "       },\n"
    "   },\n"
    ");\n"
    "%Image::ExifTool::UserDefined::ART = (\n"
    "   GROUPS        => { 0 => 'XMP', 1 => 'XMP-ART', 2 => 'Image' },\n"
    "   NAMESPACE     => { 'ART' => 'http://us.pixls.art/ART/1.0/' },\n"
    "   WRITABLE      => 'string',\n"
    "   arp => { Groups => { 2 => 'Other' } },\n"
    ");\n";

} // namespace

class Exiftool {
public:
    Exiftool(): initialized_(false) {}

    std::pair<std::string, int> mktemp(const Glib::ustring &fname,
                                       const Glib::ustring &ctx = "")
    {
#ifdef ART_WIN32_UCRT
        std::string bn;
        for (auto c : Glib::path_get_basename(fname)) {
            if (!std::isalnum(c) && c != '_' && c != '-' && c != '.') {
                c = '_';
            }
            bn.push_back(c);
        }
#else
        auto bn = Glib::path_get_basename(fname);
#endif
        std::string templ = Glib::build_filename(
            Glib::get_tmp_dir(),
            Glib::ustring::compose("ART-exiftool%2%3-%1-XXXXXX",
                                   Glib::filename_from_utf8(bn),
                                   (ctx.empty() ? "" : "-"), ctx));
        int fd = Glib::mkstemp(templ);
        return std::make_pair(templ, fd);
    }

    std::unique_ptr<Exiv2::Image> import(const Glib::ustring &fname,
                                         const std::exception &exc)
    {
        auto p = mktemp(fname);
        auto &templ = p.first;
        int fd = p.second;
        if (fd < 0) {
            throw exc;
        }
        Glib::ustring outname = fname_to_utf8(templ) + ".xmp";
        std::vector<Glib::ustring> argv = {"-TagsFromFile", fname,
                                           "-xmp:all<all", outname};
        if (settings->verbose) {
            std::cout << "importing metadata for " << fname << " with exiftool"
                      << std::endl;
        }
        std::string out, err;
        bool ok = exec(argv, &out, &err);
        close(fd);
        g_remove(templ.c_str());
        if (settings->verbose > 1) {
            if (!out.empty()) {
                std::cout << "  exiftool stdout: " << out << std::flush;
            }
            if (!err.empty()) {
                std::cout << "  exiftool stderr: " << err << std::flush;
            }
        }
        if (!ok) {
            if (Glib::file_test(outname, Glib::FILE_TEST_EXISTS)) {
                g_remove(outname.c_str());
            }
            throw exc;
        }
        std::unique_ptr<Exiv2::Image> ret;
        try {
            auto image = open_exiv2(outname, false);
            image->readMetadata();
            auto &exif = image->exifData();
            auto &xmp = image->xmpData();
            const auto set_from = [&](const char *src,
                                      const char *dst) -> void {
                auto dk = Exiv2::ExifKey(dst);
                auto pos = exif.findKey(dk);
                if (pos == exif.end() || !pos->size()) {
                    auto sk = Exiv2::XmpKey(src);
                    auto it = xmp.findKey(sk);
                    if (it != xmp.end() && it->size()) {
                        exif[dst] = it->toString();
                    }
                }
            };
            set_from("Xmp.exifEX.LensModel", "Exif.Photo.LensModel");
            xmp.clear();
            g_remove(outname.c_str());
            ret.reset(image.release());
        } catch (std::exception &e2) {
            if (Glib::file_test(outname, Glib::FILE_TEST_EXISTS)) {
                g_remove(outname.c_str());
            }
            throw exc;
        }
        return ret;
    }

    bool exec(const std::vector<Glib::ustring> &argv, std::string *out,
              std::string *err)
    {
        std::unique_lock<std::mutex> lck(mutex_);

        init();
        if (!p_) {
            return false;
        }

        for (auto &a : argv) {
            if (!p_->write(a.c_str(), a.bytes()) || !p_->write("\n", 1)) {
                return cleanup();
            }
        }
        if (!p_->write("-execute\n", 9)) {
            return cleanup();
        }
        if (!p_->flush()) {
            return cleanup();
        }

        std::string line;
        std::ostringstream buf;
        if (err) {
            *err = "";
        }
        while (true) {
            int c = p_->read();
            if (c == EOF) {
                return cleanup();
#ifdef WIN32
            } else if (c == '\r') {
                continue;
#endif // WIN32
            } else if (c == '\n') {
                if (line == "{ready}") {
                    break;
                } else {
                    if (out) {
                        buf << line << '\n';
                    }
                    line = "";
                }
            } else {
                line.push_back(c);
            }
        }
        if (out) {
            *out = buf.str();
        }

        return true;
    }

    bool embed_procparams(const Glib::ustring &fname, const std::string &data)
    {
        Glib::ustring cfg =
            Glib::build_filename(exiftool_config_dir, "ART-exiftool.config");
        if (!Glib::file_test(cfg, Glib::FILE_TEST_EXISTS)) {
            FILE *f = g_fopen(cfg.c_str(), "w");
            if (!f) {
                return false;
            }
            bool err = (fputs(exiftool_xmp_config, f) == EOF);
            fclose(f);
            if (err) {
                return false;
            }
        }

        std::vector<Glib::ustring> argv = {
            "-config", cfg, "-overwrite_original", "-Arp=" + data, fname};
        if (settings->verbose) {
            std::cout << "embedding params for " << fname << " with exiftool"
                      << std::endl;
        }
        std::string out, err;
        bool ok = exec(argv, &out, &err);
        return ok;
    }

    void shutdown()
    {
        if (p_) {
            p_->write("-stay_open\n0\n", 13);
            p_->flush();
        }
        cleanup();
    }

private:
    Glib::ustring get_bin()
    {
        Glib::ustring exiftool = settings->exiftool_path;
        if (exiftool == exiftool_default) {
            Glib::ustring e = Glib::build_filename(exiftool_base_dir, exiftool);
            if (Glib::file_test(e, Glib::FILE_TEST_EXISTS)) {
                exiftool = e;
            }
        }
        return exiftool;
    }

    bool cleanup()
    {
        p_.reset(nullptr);
        return false;
    }

    void init()
    {
        if (!initialized_) {
            initialized_ = true;
            auto e = get_bin();
            if (e.empty()) {
                p_.reset(nullptr);
                if (settings->verbose) {
                    std::cout << "exiftool disabled or not found" << std::endl;
                }
            } else {
                if (settings->verbose) {
                    std::cout << "starting exiftool... " << std::flush;
                }
                std::vector<Glib::ustring> argv = {
                    e,      "-stay_open",       "true",     "-@",
                    "-",    "-common_args",     "-charset", "filename=utf8",
                    "-api", "WindowsLongPath=0"};
                try {
                    p_ = subprocess::popen("", argv, true, true, true);
                    if (settings->verbose) {
                        std::cout << (p_ ? "OK" : "ERROR!") << std::endl;
                    }
                } catch (subprocess::error &exc) {
                    p_.reset(nullptr);
                    if (settings->verbose) {
                        std::cout << "ERROR: " << exc.what() << std::endl;
                    }
                }
            }
        }
    }

    std::unique_ptr<subprocess::SubprocessInfo> p_;
    std::mutex mutex_;
    bool initialized_;
};

//-----------------------------------------------------------------------------
// Exiv2Metadata
//-----------------------------------------------------------------------------

Exiv2Metadata::Exiv2Metadata()
    : src_(""), merge_xmp_(false), image_(nullptr), rating_(0)
{
}

Exiv2Metadata::Exiv2Metadata(const Glib::ustring &path)
    : src_(path), merge_xmp_(settings->metadata_xmp_sync !=
                             Settings::MetadataXmpSync::NONE),
      image_(nullptr), rating_(0)
{
}

Exiv2Metadata::Exiv2Metadata(const Glib::ustring &path, bool merge_xmp_sidecar)
    : src_(path), merge_xmp_(merge_xmp_sidecar), image_(nullptr), rating_(0)
{
}

void Exiv2Metadata::load() const
{
    if (!src_.empty() && !image_.get() &&
        Glib::file_test(src_.c_str(), Glib::FILE_TEST_EXISTS)) {
        CacheVal val;
        auto finfo = Gio::File::create_for_path(src_)->query_info(
            G_FILE_ATTRIBUTE_TIME_MODIFIED);
        Glib::TimeVal xmp_mtime(0, 0);
        if (merge_xmp_) {
            auto xmpname = xmpSidecarPath(src_);
            if (Glib::file_test(xmpname.c_str(), Glib::FILE_TEST_EXISTS)) {
                xmp_mtime = Gio::File::create_for_path(xmpname)
                                ->query_info(G_FILE_ATTRIBUTE_TIME_MODIFIED)
                                ->modification_time();
            }
        }

        if (cache_ && cache_->get(src_, val) &&
            val.image_mtime >= finfo->modification_time() &&
            val.use_xmp == merge_xmp_ && val.xmp_mtime >= xmp_mtime) {
            // if (settings->verbose) {
            //     std::cout << "Metadata for " << src_ << " found in cache" <<
            //     std::endl;
            // }
            image_ = val.image;
        } else {
            try {
                // throw Error("test exiftool");
                auto img = open_exiv2(src_, true);
                image_.reset(img.release());
            } catch (std::exception &exc) {
                auto img = exiftool_->import(src_, exc);
                image_.reset(img.release());
            }
            if (merge_xmp_) {
                do_merge_xmp(image_.get(), false);
            }
            if (cache_) {
                val.image = image_;
                val.image_mtime = finfo->modification_time();
                val.xmp_mtime = xmp_mtime;
                val.use_xmp = merge_xmp_;
                cache_->set(src_, val);
            }
        }
    }
}

void Exiv2Metadata::do_merge_xmp(Exiv2::Image *dst, bool keep_all) const
{
    try {
        auto xmp = getXmpSidecar(src_);
        Exiv2::ExifData exif;
        Exiv2::IptcData iptc;
        Exiv2::copyXmpToIptc(xmp, iptc);
        Exiv2::moveXmpToExif(xmp, exif);
        std::unordered_map<std::string, std::unordered_set<std::string>> seen;

        if (!keep_all) {
            remove_unwanted(exif);
        }

        for (auto &datum : exif) {
            dst->exifData()[datum.key()] = datum;
        }
        for (auto &datum : iptc) {
            auto &s = seen[datum.key()];
            if (s.empty()) {
                clear_metadata_key(dst->iptcData(),
                                   Exiv2::IptcKey(datum.key()));
                dst->iptcData()[datum.key()] = datum;
                s.insert(datum.toString());
            } else if (s.insert(datum.toString()).second) {
                dst->iptcData().add(datum);
            }
        }
        seen.clear();
        for (auto &datum : xmp) {
            auto &s = seen[datum.key()];
            if (s.empty()) {
                clear_metadata_key(dst->xmpData(), Exiv2::XmpKey(datum.key()));
                dst->xmpData()[datum.key()] = datum;
                s.insert(datum.toString());
            } else if (s.insert(datum.toString()).second) {
                dst->xmpData().add(datum);
            }
        }
    } catch (std::exception &exc) {
        if (settings->verbose) {
            std::cerr << "Error loading metadata from XMP sidecar: "
                      << exc.what() << std::endl;
        }
    }
}

void Exiv2Metadata::saveToImage(ProgressListener *pl, const Glib::ustring &path,
                                bool preserve_all_tags) const
{
    auto dst = open_exiv2(path, false);
    if (image_.get()) {
        dst->setIptcData(image_->iptcData());
        dst->setXmpData(image_->xmpData());
        if (merge_xmp_) {
            do_merge_xmp(dst.get(), preserve_all_tags);
        }
        auto srcexif = image_->exifData();
        if (!preserve_all_tags) {
            remove_unwanted(srcexif);
        }
        // dst->setExifData(srcexif);
        for (auto &tag : srcexif) {
            if (tag.count() > 0) {
                dst->exifData()[tag.key()] = tag;
            }
        }
    } else {
        dst->setExifData(exif_data_);
        dst->setIptcData(iptc_data_);
        dst->setXmpData(xmp_data_);
    }

    if (!exif_keys_ ||
        exif_keys_->find("Exif.Image.Software") == exif_keys_->end()) {
        dst->exifData()["Exif.Image.Software"] = RTNAME " " RTVERSION;
    }
    if (!exif_keys_ ||
        exif_keys_->find("Exif.Image.DateTime") == exif_keys_->end()) {
        dst->exifData()["Exif.Image.DateTime"] =
            Glib::DateTime::create_now_local().format("%Y:%m:%d %H:%M:%S");
    }

    if (rating_ != 0) {
        if (!preserve_all_tags ||
            dst->exifData().findKey(Exiv2::ExifKey("Exif.Image.Rating")) ==
                dst->exifData().end()) {
            dst->exifData()["Exif.Image.Rating"] =
                static_cast<unsigned short>(LIM(rating_, 0, 5));
        }
        if (!preserve_all_tags ||
            dst->xmpData().findKey(Exiv2::XmpKey("Xmp.xmp.Rating")) ==
                dst->xmpData().end()) {
            dst->xmpData()["Xmp.xmp.Rating"] = std::to_string(rating_);
        }
    }
    import_exif_pairs(dst->exifData());
    import_iptc_pairs(dst->iptcData());
    bool xmp_tried = false;
    bool iptc_tried = false;
    for (int i = 0; i < 3; ++i) {
        try {
            dst->writeMetadata();
            return;
        } catch (Exiv2::Error &exc) {
            if (int(exc.code()) == 37) {
                std::string msg = exc.what();
                if (pl) {
                    pl->error(Glib::ustring::compose(M("METADATA_SAVE_ERROR"),
                                                     path, "WARNING: " + msg));
                }
                if (msg.find("XMP") != std::string::npos &&
                    !dst->xmpData().empty()) {
                    dst->xmpData().clear();
                    if (!xmp_tried && merge_xmp_) {
                        do_merge_xmp(dst.get(), preserve_all_tags);
                        xmp_tried = true;
                    }
                } else if (msg.find("IPTC") != std::string::npos &&
                           !dst->iptcData().empty()) {
                    dst->iptcData().clear();
                    if (!iptc_tried) {
                        import_iptc_pairs(dst->iptcData());
                        iptc_tried = true;
                    }
                }
            } else {
                throw exc;
            }
        }
    }
}

void Exiv2Metadata::remove_unwanted(Exiv2::ExifData &dst) const
{
    Exiv2::ExifThumb thumb(dst);
    thumb.erase();

    static const std::set<std::string> badtags = {
        "Exif.Image.Orientation",
        "Exif.Image2.JPEGInterchangeFormat",
        "Exif.Image2.JPEGInterchangeFormatLength",
        "Exif.Image.NewSubfileType",
        "Exif.Image.SubfileType",
        "Exif.Image.ImageWidth",
        "Exif.Image.ImageLength",
        "Exif.Image.BitsPerSample",
        "Exif.Image.Compression",
        "Exif.Image.PhotometricInterpretation",
        "Exif.Image.Thresholding",
        "Exif.Image.CellWidth",
        "Exif.Image.CellLength",
        "Exif.Image.FillOrder",
        "Exif.Image.StripOffsets",
        "Exif.Image.Orientation",
        "Exif.Image.SamplesPerPixel",
        "Exif.Image.RowsPerStrip",
        "Exif.Image.StripByteCounts",
        "Exif.Image.XResolution",
        "Exif.Image.YResolution",
        "Exif.Image.PlanarConfiguration",
        "Exif.Image.GrayResponseUnit",
        "Exif.Image.GrayResponseCurve",
        "Exif.Image.T4Options",
        "Exif.Image.T6Options",
        "Exif.Image.ResolutionUnit",
        "Exif.Image.PageNumber",
        "Exif.Image.Predictor",
        "Exif.Image.TileWidth",
        "Exif.Image.TileLength",
        "Exif.Image.TileOffsets",
        "Exif.Image.TileByteCounts",
        "Exif.Image.SubIFDs",
        "Exif.Image.ExtraSamples",
        "Exif.Image.SampleFormat",
        "Exif.Image.SMinSampleValue",
        "Exif.Image.SMaxSampleValue",
        "Exif.Image.Indexed",
        "Exif.Image.JPEGTables",
        "Exif.Image.OPIProxy",
        "Exif.Image.JPEGProc",
        "Exif.Image.JPEGInterchangeFormat",
        "Exif.Image.JPEGInterchangeFormatLength",
        "Exif.Image.JPEGRestartInterval",
        "Exif.Image.JPEGLosslessPredictors",
        "Exif.Image.JPEGPointTransforms",
        "Exif.Image.JPEGQTables",
        "Exif.Image.JPEGDCTables",
        "Exif.Image.JPEGACTables",
        "Exif.Image.TIFFEPStandardID",
        "Exif.Image.DNGVersion",
        "Exif.Image.DNGBackwardVersion",
        "Exif.Image.DNGPrivateData",
        "Exif.Image.OriginalRawFileData",
        "Exif.Image.SubTileBlockSize",
        "Exif.Image.RowInterleaveFactor",
        "Exif.Photo.ComponentsConfiguration",
        "Exif.Photo.CompressedBitsPerPixel"};

    static const std::vector<std::string> badpatterns = {"Exif.SubImage"};

    if (exif_keys_ && !src_.empty()) {
        try {
            FramesData fd(src_);
            fd.fillBasicTags(dst);
        } catch (std::exception &exc) {
            std::cout << "Error reading metadata from " << src_ << std::endl;
        }
    }

    for (auto it = dst.begin(); it != dst.end();) {
        int relevant =
            exif_keys_
                ? (exif_keys_->find(it->key()) != exif_keys_->end() ? 1 : 0)
                : -1;
        if (badtags.find(it->key()) != badtags.end() && relevant != 1) {
            it = dst.erase(it);
        } else if (relevant == 0) {
            it = dst.erase(it);
        } else {
            bool found = false;
            for (auto &p : badpatterns) {
                if (it->key().find(p) == 0) {
                    it = dst.erase(it);
                    found = true;
                    break;
                }
            }
            if (!found) {
                ++it;
            }
        }
    }
}

void Exiv2Metadata::import_exif_pairs(Exiv2::ExifData &out) const
{
    for (auto &p : exif_) {
        try {
            out[p.first] = p.second;
        } catch (std::exception &exc) {
            if (settings->verbose) {
                std::cout << "Error setting " << p.first << " to " << p.second
                          << ": " << exc.what() << std::endl;
            }
        }
    }
}

void Exiv2Metadata::import_iptc_pairs(Exiv2::IptcData &out) const
{
    for (auto &p : iptc_) {
        try {
            auto &v = p.second;
            if (v.size() >= 1) {
                clear_metadata_key(out, Exiv2::IptcKey(p.first));
                Exiv2::Iptcdatum d(Exiv2::IptcKey(p.first));
                d.setValue(v[0]);
                out[p.first] = d;
                for (size_t j = 1; j < v.size(); ++j) {
                    d.setValue(v[j]);
                    out.add(d);
                }
            }
        } catch (std::exception &exc) {
            if (settings->verbose) {
                std::cout << "Error setting " << p.first << ": " << exc.what()
                          << std::endl;
            }
        }
    }
}

void Exiv2Metadata::saveToXmp(const Glib::ustring &path) const
{
    Exiv2::XmpData xmp;
    Exiv2::copyExifToXmp(exifData(), xmp);
    Exiv2::copyIptcToXmp(iptcData(), xmp);
    for (auto &datum : xmpData()) {
        xmp[datum.key()] = datum;
    }
    Exiv2::ExifData exif;
    Exiv2::IptcData iptc;
    import_exif_pairs(exif);
    import_iptc_pairs(iptc);
    Exiv2::copyExifToXmp(exif, xmp);
    Exiv2::copyIptcToXmp(iptc, xmp);

    std::string data;
    bool err = false;
    if (Exiv2::XmpParser::encode(data, xmp,
                                 Exiv2::XmpParser::omitPacketWrapper |
                                     Exiv2::XmpParser::useCompactFormat) != 0) {
        err = true;
    } else {
        FILE *out = g_fopen(path.c_str(), "wb");
        if (!out || fputs(data.c_str(), out) == EOF) {
            err = true;
        }
        if (out) {
            fclose(out);
        }
    }

    if (err) {
        throw Error("error saving XMP sidecar " + path);
    }
}

void Exiv2Metadata::setExifKeys(const std::vector<std::string> *keys)
{
    exif_keys_.reset();
    if (keys) {
        exif_keys_ = std::make_shared<std::unordered_set<std::string>>();
        exif_keys_->insert(keys->begin(), keys->end());
    }
}

void Exiv2Metadata::getDimensions(int &w, int &h) const
{
    if (image_) {
        if (dynamic_cast<const Exiv2::XmpSidecar *>(image_.get())) {
            auto &exif = image_->exifData();
            auto itw = exif.findKey(Exiv2::ExifKey("Exif.Image.ImageWidth"));
            auto ith = exif.findKey(Exiv2::ExifKey("Exif.Image.ImageLength"));
            if (itw != exif.end() && ith != exif.end()) {
                w = exiv2_to_long(*itw);
                h = exiv2_to_long(*ith);
            } else {
                w = h = -1;
            }
        } else {
            w = image_->pixelWidth();
            h = image_->pixelHeight();
        }
    } else {
        w = h = -1;
    }
}

Glib::ustring Exiv2Metadata::xmpSidecarPath(const Glib::ustring &path)
{
    Glib::ustring fn = path;
    if (settings->xmp_sidecar_style == Settings::XmpSidecarStyle::STD) {
        fn = removeExtension(fn);
    }
    return fn + ".xmp";
}

Exiv2::XmpData Exiv2Metadata::getXmpSidecar(const Glib::ustring &path)
{
    Exiv2::XmpData ret;
    auto fname = xmpSidecarPath(path);
    if (Glib::file_test(fname, Glib::FILE_TEST_EXISTS)) {
        auto image = open_exiv2(fname, false);
        ret = image->xmpData();
    }
    return ret;
}

void Exiv2Metadata::init(const Glib::ustring &base_dir,
                         const Glib::ustring &user_dir)
{
    cache_.reset(new ImageCache(IMAGE_CACHE_SIZE));
    jsoncache_.reset(new JSONCache(IMAGE_CACHE_SIZE));
    const gchar *exiftool_base_dir_env = g_getenv("ART_EXIFTOOL_BASE_DIR");
    if (exiftool_base_dir_env) {
        exiftool_base_dir = exiftool_base_dir_env;
    } else {
        exiftool_base_dir = base_dir;
    }
    exiftool_config_dir = user_dir;
    exiftool_.reset(new Exiftool());

    Exiv2::XmpParser::initialize();
    Exiv2::XmpProperties::registerNs("us/pixls/ART/", "ART");
#ifdef EXV_ENABLE_BMFF
    Exiv2::enableBMFF(true);
#endif
}

void Exiv2Metadata::cleanup()
{
    Exiv2::XmpParser::terminate();
    if (exiftool_) {
        exiftool_->shutdown();
    }
}

void Exiv2Metadata::embedProcParamsData(const Glib::ustring &fname,
                                        const std::string &data)
{
    try {
        auto img = open_exiv2(fname, false);
        img->xmpData()["Xmp.ART.arp"] = data;
        img->writeMetadata();
    } catch (std::exception &exc) {
        if (!exiftool_->embed_procparams(fname, data)) {
            throw exc;
        }
    }
}

std::unordered_map<std::string, std::string>
Exiv2Metadata::getExiftoolMakernotes(const Glib::ustring &fname)
{
    if (fname.empty()) {
        return {};
    }

    JSONCacheVal val;
    Glib::RefPtr<Gio::FileInfo> finfo;
    try {
        finfo = Gio::File::create_for_path(fname)->query_info(
            G_FILE_ATTRIBUTE_TIME_MODIFIED);
    } catch (Glib::Error &exc) {
        if (settings->verbose) {
            std::cout << "Error querying the modification time for " << fname
                      << ": " << exc.what() << std::endl;
        }
    }
    if (jsoncache_ && finfo && jsoncache_->get(fname, val) &&
        val.second >= finfo->modification_time()) {
        if (settings->verbose > 1) {
            std::cout << "retrieving exiftool makernotes from cache for: "
                      << fname << std::endl;
        }
        return val.first;
    }

    std::unordered_map<std::string, std::string> ret;

    auto p = exiftool_->mktemp(fname, "json");
    auto &templ = p.first;
    int fd = p.second;
    if (fd < 0) {
        if (settings->verbose) {
            std::cout << "ERROR generating temporary file: "
                      << Glib::path_get_basename(fname) << std::endl;
        }
        return ret;
    }
    Glib::ustring outname = fname_to_utf8(templ);

    std::vector<Glib::ustring> argv = {
        "-json", "-MakerNotes:all", "-RAF:all", "-PanasonicRaw:all",
        "-w+",   "%0f" + outname,   fname};
    std::string out, err;
    if (!exiftool_->exec(argv, &out, &err)) {
        if (settings->verbose) {
            std::cout << "ERROR executing exiftool with args:";
            for (auto &a : argv) {
                std::cout << " " << a;
            }
            std::cout << std::endl;
            std::cout << "output:\n" << out << "\nerror:\n" << err << std::endl;
        }
        if (Glib::file_test(outname, Glib::FILE_TEST_EXISTS)) {
            g_remove(outname.c_str());
        }
        return ret;
    } else if (settings->verbose > 1) {
        std::cout << "exiftool exec with args:";
        for (auto &a : argv) {
            std::cout << " " << a;
        }
        std::cout << std::endl;
        std::cout << "output:\n" << out << "\nerror:\n" << err << std::endl;
    }

    cJSON *root = nullptr;
    close(fd);

    FILE *src = g_fopen(outname.c_str(), "rb");
    if (src) {
        std::ostringstream data;
        int c;
        while (true) {
            c = fgetc(src);
            if (c != EOF) {
                data << static_cast<unsigned char>(c);
            } else {
                break;
            }
        }
        fclose(src);
        std::string s = data.str();
        root = cJSON_Parse(s.c_str());
    }
    if (Glib::file_test(outname, Glib::FILE_TEST_EXISTS)) {
        g_remove(outname.c_str());
    }

    if (!root) {
        return ret;
    }

    const auto tostr = [](double d) -> std::string {
        if (d == int(d)) {
            return std::to_string(int(d));
        } else {
            auto s = std::to_string(d);
            auto p = s.rfind('.');
            if (p != std::string::npos) {
                while (s.back() == '0') {
                    s.pop_back();
                }
                if (s.back() == '.') {
                    s.pop_back();
                }
            }
            return s;
        }
    };

    if (cJSON_IsArray(root) && cJSON_GetArraySize(root) == 1) {
        cJSON *obj = cJSON_GetArrayItem(root, 0);
        if (obj && cJSON_IsObject(obj)) {
            for (cJSON *e = obj->child; e != nullptr; e = e->next) {
                if (e->type & cJSON_String) {
                    ret[e->string] = e->valuestring;
                } else if (e->type & cJSON_Number) {
                    ret[e->string] = tostr(e->valuedouble);
                } else if (e->type & cJSON_True) {
                    ret[e->string] = "true";
                } else if (e->type & cJSON_False) {
                    ret[e->string] = "false";
                }
            }
        }
    }

    cJSON_Delete(root);
    ret.erase("SourceFile");

    if (jsoncache_ && finfo) {
        jsoncache_->set(fname, JSONCacheVal(ret, finfo->modification_time()));
    }

    return ret;
}

std::unordered_map<std::string, std::string>
Exiv2Metadata::getMakernotes() const
{
    return getExiftoolMakernotes(src_);
}

Exiv2::ExifData Exiv2Metadata::getOutputExifData() const
{
    Exiv2::ExifData exif = exifData();
    try {
        auto xmp = getXmpSidecar(src_);
        Exiv2::moveXmpToExif(xmp, exif);
    } catch (std::exception &exc) {
        if (settings->verbose) {
            std::cerr << "Error loading metadata from XMP sidecar: "
                      << exc.what() << std::endl;
        }
    }
    remove_unwanted(exif);
    import_exif_pairs(exif);
    for (auto it = exif.begin(); it != exif.end();) {
        if (it->count() > 0) {
            ++it;
        } else {
            it = exif.erase(it);
        }
    }
    return exif;
}

void Exiv2Metadata::setOutputRating(
    const rtengine::procparams::ProcParams &pparams, bool from_xmp_sidecar)
{
    if (from_xmp_sidecar) {
        auto xmp = getXmpSidecar(src_);
        auto it = xmp.findKey(Exiv2::XmpKey("Xmp.xmp.Rating"));
        if (it != xmp.end()) {
            rating_ = exiv2_to_long(*it);
        }
    } else {
        rating_ = pparams.inTrash ? -1 : pparams.rank;
    }
}

long exiv2_to_long(const Exiv2::Metadatum &d)
{
#if EXIV2_TEST_VERSION(0, 28, 0)
    return d.toInt64();
#else
    return d.toLong();
#endif
}

} // namespace rtengine
