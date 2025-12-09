/* -*- C++ -*-
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2017 Alberto Griggio
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

#include "dynamicprofile.h"
#include "metadata.h"

#include <glibmm/regex.h>
#include <stdlib.h>

using namespace rtengine;
using namespace rtengine::procparams;

namespace {

const int ISO_MAX = 512000;
const double FNUMBER_MAX = 100.0;
const double FOCALLEN_MAX = 10000.0;
const double SHUTTERSPEED_MAX = 1000.0;
const double EXPCOMP_MIN = -20.0;
const double EXPCOMP_MAX = 20.0;

} // namespace

DynamicProfileRules dynamicProfileRules;

bool DynamicProfileRule::Optional::operator()(const Glib::ustring &val) const
{
    if (!enabled) {
        return true;
    }

    if (value.find("re:") == 0) {
        // this is a regexp
        return Glib::Regex::match_simple(value.substr(3), val,
                                         Glib::REGEX_CASELESS);
    } else {
        // normal string comparison
        return value.casefold() == val.casefold();
    }
}

bool DynamicProfileRule::CustomMetadata::operator()(
    const FramesMetaData *m) const
{
    if (!enabled || value.empty()) {
        return true;
    }

    try {
        rtengine::Exiv2Metadata meta(m->getFileName());
        std::unordered_map<std::string, std::string> mn;
        bool mn_loaded = false;
        meta.load();
        auto &exif = meta.exifData();
        Glib::ustring found;
        for (auto &p : value) {
            if (p.first.find("ExifTool.MakerNotes.") == 0) {
                if (!mn_loaded) {
                    mn_loaded = true;
                    mn = meta.getMakernotes();
                }
                auto it = mn.find(p.first.substr(20));
                if (it == mn.end()) {
                    return false;
                }
                found = it->second;
            } else {
                auto pos = exif.findKey(Exiv2::ExifKey(p.first));
                if (pos == exif.end()) {
                    return false;
                }
                found = pos->print(&exif);
                if (!found.validate()) {
                    return false;
                }
            }
            auto &val = p.second;
            if (val.find("re:") == 0) {
                if (!Glib::Regex::match_simple(val.substr(3), found,
                                               Glib::REGEX_CASELESS)) {
                    return false;
                }
            } else {
                if (Glib::ustring(val).casefold() != found.casefold()) {
                    return false;
                }
            }
        }
        return true;
    } catch (std::exception &exc) {
        return false;
    }
}

DynamicProfileRule::DynamicProfileRule()
    : serial_number(0), iso(0, ISO_MAX), fnumber(0, FNUMBER_MAX),
      focallen(0, FOCALLEN_MAX), shutterspeed(0, SHUTTERSPEED_MAX),
      expcomp(EXPCOMP_MIN, EXPCOMP_MAX), customdata()
{
}

bool DynamicProfileRule::operator<(const DynamicProfileRule &other) const
{
    return serial_number < other.serial_number;
}

bool DynamicProfileRule::matches(const rtengine::FramesMetaData *im) const
{
    return (iso(im->getISOSpeed()) && fnumber(im->getFNumber()) &&
            focallen(im->getFocalLen()) &&
            shutterspeed(im->getShutterSpeed()) && expcomp(im->getExpComp()) &&
            camera(im->getCamera()) && lens(im->getLens()) &&
            imagetype(im->isRAW() ? "raw" : "nonraw") &&
            filetype(getFileExtension(im->getFileName())) &&
            software(im->getSoftware()) && customdata(im));
}

namespace {

void get_int_range(DynamicProfileRule::Range<int> &dest,
                   const Glib::KeyFile &kf, const Glib::ustring &group,
                   const Glib::ustring &key)
{
    try {
        int min = kf.get_integer(group, key + "_min");
        int max = kf.get_integer(group, key + "_max");

        if (min <= max) {
            dest.min = min;
            dest.max = max;
        }
    } catch (Glib::KeyFileError &e) {
    }
}

void get_double_range(DynamicProfileRule::Range<double> &dest,
                      const Glib::KeyFile &kf, const Glib::ustring &group,
                      const Glib::ustring &key)
{
    try {
        double min = kf.get_double(group, key + "_min");
        double max = kf.get_double(group, key + "_max");

        if (min <= max) {
            dest.min = min;
            dest.max = max;
        }
    } catch (Glib::KeyFileError &e) {
    }
}

void get_optional(DynamicProfileRule::Optional &dest, const Glib::KeyFile &kf,
                  const Glib::ustring &group, const Glib::ustring &key)
{
    try {
        bool e = kf.get_boolean(group, key + "_enabled");

        if (e) {
            Glib::ustring s = kf.get_string(group, key + "_value");
            dest.enabled = e;
            dest.value = s;
        }
    } catch (Glib::KeyFileError &) {
    }
}

void get_customdata(DynamicProfileRule::CustomMetadata &dest,
                    const Glib::KeyFile &kf, const Glib::ustring &group,
                    const Glib::ustring &key)
{
    try {
        bool e = kf.get_boolean(group, key + "_enabled");

        if (e) {
            Glib::ArrayHandle<Glib::ustring> ar =
                kf.get_string_list(group, key + "_value");
            dest.enabled = e;
            dest.value.clear();
            for (const auto &s : ar) {
                auto p = s.find("=");
                if (p != Glib::ustring::npos) {
                    dest.value.emplace_back(s.substr(0, p), s.substr(p + 1));
                } else {
                    dest.value.emplace_back(s, "");
                }
            }
        }
    } catch (Glib::KeyFileError &) {
    }
}

void set_int_range(Glib::KeyFile &kf, const Glib::ustring &group,
                   const Glib::ustring &key,
                   const DynamicProfileRule::Range<int> &val)
{
    kf.set_integer(group, key + "_min", val.min);
    kf.set_integer(group, key + "_max", val.max);
}

void set_double_range(Glib::KeyFile &kf, const Glib::ustring &group,
                      const Glib::ustring &key,
                      const DynamicProfileRule::Range<double> &val)
{
    kf.set_double(group, key + "_min", val.min);
    kf.set_double(group, key + "_max", val.max);
}

void set_optional(Glib::KeyFile &kf, const Glib::ustring &group,
                  const Glib::ustring &key,
                  const DynamicProfileRule::Optional &val)
{
    kf.set_boolean(group, key + "_enabled", val.enabled);
    kf.set_string(group, key + "_value", val.value);
}

void set_customdata(Glib::KeyFile &kf, const Glib::ustring &group,
                    const Glib::ustring &key,
                    const DynamicProfileRule::CustomMetadata &val)
{
    kf.set_boolean(group, key + "_enabled", val.enabled);
    std::vector<std::string> tmp;
    for (auto &p : val.value) {
        tmp.push_back(p.first + "=" + p.second);
    }
    Glib::ArrayHandle<Glib::ustring> arr = tmp;
    kf.set_string_list(group, key + "_value", arr);
}

} // namespace

bool DynamicProfileRules::loadRules(bool force_builtins)
{
    dynamicRules.clear();
    Glib::KeyFile kf;

    Glib::ustring rules_file =
        Glib::build_filename(options.user_config_dir, "dynamicprofile.cfg");
    if (force_builtins ||
        !Glib::file_test(rules_file, Glib::FILE_TEST_EXISTS)) {
        rules_file = builtin_rules_file_;
    }

    try {
        if (!kf.load_from_file(rules_file)) {
            return false;
        }
    } catch (Glib::Error &e) {
        return false;
    }

    if (options.rtSettings.verbose > 1) {
        printf("loading dynamic profiles...\n");
    }

    auto groups = kf.get_groups();

    for (auto group : groups) {
        // groups are of the form "rule N", where N is a positive integer
        if (group.find("rule ") != 0) {
            return false;
        }

        std::istringstream buf(group.c_str() + 5);
        int serial = 0;

        if (!(buf >> serial) || !buf.eof()) {
            return false;
        }

        if (options.rtSettings.verbose > 1) {
            printf(" loading rule %d\n", serial);
        }

        dynamicRules.emplace_back(DynamicProfileRule());
        DynamicProfileRule &rule = dynamicRules.back();
        rule.serial_number = serial;
        get_int_range(rule.iso, kf, group, "iso");
        get_double_range(rule.fnumber, kf, group, "fnumber");
        get_double_range(rule.focallen, kf, group, "focallen");
        get_double_range(rule.shutterspeed, kf, group, "shutterspeed");
        get_double_range(rule.expcomp, kf, group, "expcomp");
        get_optional(rule.camera, kf, group, "camera");
        get_optional(rule.lens, kf, group, "lens");
        get_optional(rule.imagetype, kf, group, "imagetype");
        get_optional(rule.filetype, kf, group, "filetype");
        get_optional(rule.software, kf, group, "software");
        get_customdata(rule.customdata, kf, group, "customdata");

        try {
            rule.profilepath = kf.get_string(group, "profilepath");
        } catch (Glib::KeyFileError &) {
            dynamicRules.pop_back();
        }
    }

    std::sort(dynamicRules.begin(), dynamicRules.end());
    rulesLoaded = true;
    return true;
}

bool DynamicProfileRules::storeRules()
{
    if (options.rtSettings.verbose > 1) {
        printf("saving dynamic profiles...\n");
    }

    Glib::KeyFile kf;

    for (auto &rule : dynamicRules) {
        std::ostringstream buf;
        buf << "rule " << rule.serial_number;
        Glib::ustring group = buf.str();
        set_int_range(kf, group, "iso", rule.iso);
        set_double_range(kf, group, "fnumber", rule.fnumber);
        set_double_range(kf, group, "focallen", rule.focallen);
        set_double_range(kf, group, "shutterspeed", rule.shutterspeed);
        set_double_range(kf, group, "expcomp", rule.expcomp);
        set_optional(kf, group, "camera", rule.camera);
        set_optional(kf, group, "lens", rule.lens);
        set_optional(kf, group, "imagetype", rule.imagetype);
        set_optional(kf, group, "filetype", rule.filetype);
        set_optional(kf, group, "software", rule.software);
        set_customdata(kf, group, "customdata", rule.customdata);
        kf.set_string(group, "profilepath", rule.profilepath);
    }

    Glib::ustring fname =
        Glib::build_filename(options.user_config_dir, "dynamicprofile.cfg");
    if (Glib::file_test(fname, Glib::FILE_TEST_EXISTS)) {
        try {
            Glib::KeyFile old;
            old.load_from_file(fname);
            old.save_to_file(fname + ".bak");
        } catch (std::exception &e) {
        }
    }
    return kf.save_to_file(fname);
}

const std::vector<DynamicProfileRule> &DynamicProfileRules::getRules()
{
    if (!rulesLoaded) {
        loadRules();
    }

    return dynamicRules;
}

void DynamicProfileRules::setRules(const std::vector<DynamicProfileRule> &r)
{
    dynamicRules = r;
}

void DynamicProfileRules::init(const Glib::ustring &base_dir)
{
    builtin_rules_file_ = Glib::build_filename(base_dir, "dynamicprofile.cfg");
}

Glib::ustring DynamicProfileRules::builtin_rules_file_("");
