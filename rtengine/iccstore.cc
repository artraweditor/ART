/*
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2004-2010 Gabor Horvath <hgabor@rawtherapee.com>
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
#include <cstring>

#include <glib/gstdio.h>
#include <glibmm.h>

#ifdef WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

#include <iostream>

#include "iccstore.h"

#include "iccmatrices.h"
#include "procparams.h"

#include "../rtgui/options.h"
#include "../rtgui/threadutils.h"
#include "lcms2_plugin.h"
#ifdef ART_LCMS2_FAST_FLOAT
#include <lcms2_fast_float.h>
#endif // ART_LCMS2_FAST_FLOAT

#include "cJSON.h"
#include "color.h"
#include "linalgebra.h"

#define inkc_constant 0x696E6B43

namespace rtengine {

extern const Settings *settings;

namespace {

// Not recursive
void loadProfiles(const Glib::ustring &dirName,
                  std::map<Glib::ustring, cmsHPROFILE> *profiles,
                  std::map<Glib::ustring, ProfileContent> *profileContents,
                  std::map<Glib::ustring, Glib::ustring> *profileNames,
                  bool nameUpper)
{
    if (dirName.empty()) {
        return;
    }

    try {
        Glib::Dir dir(dirName);

        for (Glib::DirIterator entry = dir.begin(); entry != dir.end();
             ++entry) {
            const Glib::ustring fileName = *entry;

            if (fileName.size() < 4) {
                continue;
            }

            const Glib::ustring extension = getFileExtension(fileName);

            if (extension != "icc" && extension != "icm") {
                continue;
            }

            const Glib::ustring filePath =
                Glib::build_filename(dirName, fileName);

            if (!Glib::file_test(filePath, Glib::FILE_TEST_IS_REGULAR)) {
                continue;
            }

            Glib::ustring name = fileName.substr(0, fileName.size() - 4);

            if (nameUpper) {
                name = name.uppercase();
            }

            if (profiles) {
                const ProfileContent content(filePath);
                const cmsHPROFILE profile = content.toProfile();

                if (profile) {
                    profiles->emplace(name, profile);

                    if (profileContents) {
                        profileContents->emplace(name, content);
                    }
                }
            }

            if (profileNames) {
                profileNames->emplace(name, filePath);
            }
        }
    } catch (Glib::Exception &) {
    }
}

// Version dedicated to single profile load when loadAll==false (cli version
// "-q" mode)
bool loadProfile(const Glib::ustring &profile, const Glib::ustring &dirName,
                 std::map<Glib::ustring, cmsHPROFILE> *profiles,
                 std::map<Glib::ustring, ProfileContent> *profileContents)
{
    if (dirName.empty() || profiles == nullptr) {
        return false;
    }

    try {
        Glib::Dir dir(dirName);

        for (Glib::DirIterator entry = dir.begin(); entry != dir.end();
             ++entry) {
            const Glib::ustring fileName = *entry;

            if (fileName.size() < 4) {
                continue;
            }

            const Glib::ustring extension = getFileExtension(fileName);

            if (extension != "icc" && extension != "icm") {
                continue;
            }

            const Glib::ustring filePath =
                Glib::build_filename(dirName, fileName);

            if (!Glib::file_test(filePath, Glib::FILE_TEST_IS_REGULAR)) {
                continue;
            }

            const Glib::ustring name = fileName.substr(0, fileName.size() - 4);

            if (name == profile) {
                const ProfileContent content(filePath);
                const cmsHPROFILE profile = content.toProfile();

                if (profile) {
                    profiles->emplace(name, profile);

                    if (profileContents) {
                        profileContents->emplace(name, content);
                    }

                    return true;
                }
            }
        }
    } catch (Glib::Exception &) {
    }

    return false;
}

void getSupportedIntent(cmsHPROFILE profile, cmsUInt32Number intent,
                        cmsUInt32Number direction, uint8_t &result)
{
    if (cmsIsIntentSupported(profile, intent, direction)) {
        result |= 1 << intent;
    }
}

uint8_t getSupportedIntents(cmsHPROFILE profile, cmsUInt32Number direction)
{
    if (!profile) {
        return 0;
    }

    uint8_t result = 0;

    getSupportedIntent(profile, INTENT_PERCEPTUAL, direction, result);
    getSupportedIntent(profile, INTENT_RELATIVE_COLORIMETRIC, direction,
                       result);
    getSupportedIntent(profile, INTENT_SATURATION, direction, result);
    getSupportedIntent(profile, INTENT_ABSOLUTE_COLORIMETRIC, direction,
                       result);

    return result;
}

cmsHPROFILE createXYZProfile()
{
    float mat[3][3] = {{1.0, 0, 0}, {0, 1.0, 0}, {0, 0, 1.0}};
    return ICCStore::createFromMatrix(mat, false, "XYZ");
}

const float (*wprofiles[])[3] = {
    xyz_sRGB, xyz_adobe, xyz_prophoto,
    // xyz_widegamut, xyz_bruce, xyz_beta, xyz_best,
    xyz_rec2020, xyz_ACESp0, xyz_ACESp1};

const float (*iwprofiles[])[3] = {
    sRGB_xyz, adobe_xyz, prophoto_xyz,
    // widegamut_xyz, bruce_xyz, beta_xyz, best_xyz,
    rec2020_xyz, ACESp0_xyz, ACESp1_xyz};

const char *wpnames[] = {"sRGB", "Adobe RGB", "ProPhoto",
                         // "WideGamut", "BruceRGB", "Beta RGB", "BestRGB",
                         "Rec2020", "ACESp0", "ACESp1"};

//-----------------------------------------------------------------------------
// helper functions to fix V2 profiles TRCs, used in
// ProfileContent::toProfile()
// see https://github.com/Beep6581/RawTherapee/issues/5026
// -----------------------------------------------------------------------------
bool is_ART_profile(cmsHPROFILE profile)
{
    const cmsMLU *mlu = static_cast<const cmsMLU *>(
        cmsReadTag(profile, cmsSigDeviceMfgDescTag));
    if (!mlu) {
        return false;
    }
    cmsUInt32Number sz = cmsMLUgetASCII(mlu, "en", "US", nullptr, 0);
    if (!sz) {
        return false;
    }
    std::vector<char> buf(sz);
    cmsMLUgetASCII(mlu, "en", "US", &buf[0], sz);
    buf.back() = 0; // sanity
    return strcmp(&buf[0], "ART") == 0;
}

bool get_ART_gamma_slope(cmsHPROFILE profile, double &gammatag,
                         double &slopetag)
{
    const cmsMLU *modelDescMLU = static_cast<const cmsMLU *>(
        cmsReadTag(profile, cmsSigDeviceModelDescTag));
    if (modelDescMLU) {
        cmsUInt32Number count =
            cmsMLUgetWide(modelDescMLU, "en", "US", nullptr, 0);
        if (count) {
            std::vector<wchar_t> vbuf(count);
            wchar_t *buffer = &vbuf[0];
            count = cmsMLUgetWide(modelDescMLU, "en", "US", buffer, count);
            Glib::ustring modelDesc;
#if __SIZEOF_WCHAR_T__ == 2
            char *cModelDesc = g_utf16_to_utf8(
                (unsigned short int *)buffer, -1, nullptr, nullptr,
                nullptr); // convert to utf-8 in a buffer allocated by glib
            if (cModelDesc) {
                modelDesc.assign(cModelDesc);
                g_free(cModelDesc);
            }
#else
            modelDesc = utf32_to_utf8(buffer, count);
#endif
            if (!modelDesc.empty()) {
                try {
                    std::size_t pos = modelDesc.find("#us/pixls/ART#");
                    std::size_t posmid = modelDesc.find(":");
                    std::size_t posend = modelDesc.find("!");
                    if (pos == std::string::npos ||
                        posmid == std::string::npos ||
                        posend == std::string::npos) {
                        return false;
                    }
                    pos += 13;
                    std::string strgamma =
                        modelDesc.substr(pos + 1, (posmid - pos));
                    gammatag = std::stod(strgamma.c_str());
                    std::string strslope =
                        modelDesc.substr(posmid + 1, (posend - posmid));
                    slopetag = std::stod(strslope.c_str());
                    return true;
                } catch (std::invalid_argument &) {
                    return false;
                }
            }
        }
    }
    return false;
}

} // namespace

ProfileContent::ProfileContent() = default;

ProfileContent::ProfileContent(const Glib::ustring &fileName)
{
    FILE *const f = g_fopen(fileName.c_str(), "rb");

    if (!f) {
        return;
    }

    fseek(f, 0, SEEK_END);
    long length = ftell(f);

    if (length > 0) {
        char *d = new char[length + 1];
        fseek(f, 0, SEEK_SET);
        length = fread(d, 1, length, f);
        d[length] = 0;
        data.assign(d, length);
        delete[] d;
    } else {
        data.clear();
    }

    fclose(f);
}

ProfileContent::ProfileContent(cmsHPROFILE hProfile)
{
    if (hProfile != nullptr) {
        cmsUInt32Number bytesNeeded = 0;
        cmsSaveProfileToMem(hProfile, nullptr, &bytesNeeded);

        if (bytesNeeded > 0) {
            char *d = new char[bytesNeeded + 1];
            cmsSaveProfileToMem(hProfile, d, &bytesNeeded);
            data.assign(d, bytesNeeded);
            delete[] d;
        }
    }
}

cmsHPROFILE ProfileContent::toProfile() const
{
    cmsHPROFILE profile = nullptr;
    if (!data.empty()) {
        profile = cmsOpenProfileFromMem(data.c_str(), data.size());
    }
    return profile;
}

const std::string &ProfileContent::getData() const { return data; }

class ICCStore::Implementation {
    using ProfileMap = std::map<Glib::ustring, cmsHPROFILE>;
    using MatrixMap = std::map<Glib::ustring, TMatrix>;
    using ContentMap = std::map<Glib::ustring, ProfileContent>;
    using NameMap = std::map<Glib::ustring, Glib::ustring>;

    static constexpr const char *DEFAULT_WORKING_SPACE = "Rec2020";

public:
    Implementation()
        : loadAll(true), xyz(createXYZProfile()), srgb(cmsCreate_sRGBProfile()),
          thumb_monitor_xform_(nullptr),
          monitor_profile_hash_("000000000000000000000000000000000")
    {
        // cmsErrorAction(LCMS_ERROR_SHOW);

        constexpr int N = sizeof(wpnames) / sizeof(wpnames[0]);

        for (int i = 0; i < N; ++i) {
            wProfiles[wpnames[i]] = createFromMatrix(wprofiles[i]);
            // wProfilesGamma[wpnames[i]] = createFromMatrix(wprofiles[i],
            // true);
            wMatrices[wpnames[i]] = wprofiles[i];
            iwMatrices[wpnames[i]] = iwprofiles[i];
        }
    }

    ~Implementation()
    {
        if (thumb_monitor_xform_) {
            cmsDeleteTransform(thumb_monitor_xform_);
        }

        for (auto &p : wProfiles) {
            if (p.second) {
                cmsCloseProfile(p.second);
            }
        }

        // for (auto &p : wProfilesGamma) {
        //     if (p.second) {
        //         cmsCloseProfile(p.second);
        //     }
        // }
        for (auto &p : fileProfiles) {
            if (p.second) {
                cmsCloseProfile(p.second);
            }
        }

        if (srgb) {
            cmsCloseProfile(srgb);
        }

        if (xyz) {
            cmsCloseProfile(xyz);
        }
    }

    void init(const Glib::ustring &usrICCDir, const Glib::ustring &rtICCDir,
              bool loadAll)
    {
        // Reads all profiles from the given profiles dir

        MyMutex::MyLock lock(mutex);

        this->loadAll = loadAll;

        // RawTherapee's profiles take precedence if a user's profile of the
        // same name exists
        profilesDir = Glib::build_filename(rtICCDir, "output");
        userICCDir = usrICCDir;
        fileProfiles.clear();
        fileProfileContents.clear();

        if (loadAll) {
            loadProfiles(profilesDir, &fileProfiles, &fileProfileContents,
                         nullptr, false);
            loadProfiles(userICCDir, &fileProfiles, &fileProfileContents,
                         nullptr, false);
        }

        // Input profiles
        // Load these to different areas, since the short name(e.g. "NIKON D700"
        // may overlap between system/user and RT dir)
        stdProfilesDir = Glib::build_filename(rtICCDir, "input");
        fileCamProfiles.clear();
        fileCamProfilesFileNames.clear();

        if (loadAll) {
            loadProfiles(stdProfilesDir, nullptr, nullptr,
                         &fileCamProfilesFileNames, true);
            Glib::ustring user_input_icc_dir = Glib::build_filename(
                options.user_config_dir, "iccprofiles", "input");
            loadProfiles(user_input_icc_dir, nullptr, nullptr,
                         &fileCamProfilesFileNames, true);
        }

        defaultMonitorProfile = settings->monitorProfile;

        loadWorkingSpaces(rtICCDir);
        loadWorkingSpaces(userICCDir);

        // initialize the alarm colours for lcms gamut checking -- we use bright
        // green
        cmsUInt16Number cms_alarm_codes[cmsMAXCHANNELS] = {0, 65535, 65535};
        cmsSetAlarmCodes(cms_alarm_codes);

        update_thumbnail_monitor_transform();
    }

    cmsHPROFILE workingSpace(const Glib::ustring &name) const
    {
        const ProfileMap::const_iterator r = wProfiles.find(name);

        return r != wProfiles.end()
                   ? r->second
                   : wProfiles.find(DEFAULT_WORKING_SPACE)->second;
    }

    TMatrix workingSpaceMatrix(const Glib::ustring &name) const
    {
        const MatrixMap::const_iterator r = wMatrices.find(name);

        return r != wMatrices.end()
                   ? r->second
                   : wMatrices.find(DEFAULT_WORKING_SPACE)->second;
    }

    TMatrix workingSpaceInverseMatrix(const Glib::ustring &name) const
    {

        const MatrixMap::const_iterator r = iwMatrices.find(name);

        return r != iwMatrices.end()
                   ? r->second
                   : iwMatrices.find(DEFAULT_WORKING_SPACE)->second;
    }

    bool outputProfileExist(const Glib::ustring &name) const
    {
        MyMutex::MyLock lock(mutex);
        return fileProfiles.find(name) != fileProfiles.end();
    }

    cmsHPROFILE getProfile(const Glib::ustring &name)
    {
        MyMutex::MyLock lock(mutex);
        return getProfile_unlocked(name);
    }

    static std::string getProfileTag(cmsHPROFILE profile, cmsTagSignature tag)
    {
        const cmsMLU *mlu =
            static_cast<const cmsMLU *>(cmsReadTag(profile, tag));
        if (!mlu) {
            return "";
        }
        cmsUInt32Number sz = cmsMLUgetASCII(mlu, "en", "US", nullptr, 0);
        if (!sz) {
            return "";
        }
        std::vector<char> buf(sz);
        cmsMLUgetASCII(mlu, "en", "US", &buf[0], sz);
        buf.back() = 0; // sanity
        return std::string(&buf[0]);
    }

    cmsHPROFILE getCameraProfile(const Glib::ustring &name)
    {
        const Glib::ustring nameUpper = name.uppercase();

        MyMutex::MyLock lock(mutex);

        const ProfileMap::const_iterator r = fileCamProfiles.find(nameUpper);

        // Return profile from store
        if (r != fileCamProfiles.end()) {
            return r->second;
        } else if (!loadAll) {
            // Directory not scanned, so looking and adding now...
            if (!loadProfile(name, profilesDir, &fileProfiles,
                             &fileProfileContents)) {
                loadProfile(name, userICCDir, &fileProfiles,
                            &fileProfileContents);
            }

            const ProfileMap::const_iterator r = fileProfiles.find(name);

            if (r != fileProfiles.end()) {
                return r->second;
            }
        }

        // Profile is not yet in store
        const NameMap::const_iterator f =
            fileCamProfilesFileNames.find(nameUpper);

        // Profile does not exist
        if (f == fileCamProfilesFileNames.end()) {
            return nullptr;
        }

        // But there exists one --> load it
        const ProfileContent content(f->second);
        const cmsHPROFILE profile = content.toProfile();

        if (profile) {
            fileCamProfiles.emplace(f->first, profile);
        }

        // Profile invalid or stored now --> remove entry from
        // fileCamProfilesFileNames
        fileCamProfilesFileNames.erase(f);
        return profile;
    }

    ProfileContent getContent(const Glib::ustring &name) const
    {
        MyMutex::MyLock lock(mutex);

        const ContentMap::const_iterator r = fileProfileContents.find(name);

        return r != fileProfileContents.end() ? r->second : ProfileContent();
    }

    cmsHPROFILE getXYZProfile() const { return xyz; }

    cmsHPROFILE getsRGBProfile() const { return srgb; }

    std::vector<Glib::ustring> doGetProfiles(const ProfileMap &profiles,
                                             ProfileType type) const
    {
        std::vector<Glib::ustring> res;

        for (const auto &profile : profiles) {
            if ((type == ICCStore::ProfileType::MONITOR &&
                 cmsGetDeviceClass(profile.second) == cmsSigDisplayClass &&
                 cmsGetColorSpace(profile.second) == cmsSigRgbData) ||
                (type == ICCStore::ProfileType::PRINTER &&
                 cmsGetDeviceClass(profile.second) == cmsSigOutputClass) ||
                (type == ICCStore::ProfileType::OUTPUT &&
                 (cmsGetDeviceClass(profile.second) == cmsSigDisplayClass ||
                  cmsGetDeviceClass(profile.second) == cmsSigInputClass ||
                  cmsGetDeviceClass(profile.second) == cmsSigOutputClass) &&
                 cmsGetColorSpace(profile.second) == cmsSigRgbData)) {
                res.push_back(profile.first);
            }
        }

        return res;
    }

    std::vector<Glib::ustring> getProfiles(ProfileType type) const
    {
        MyMutex::MyLock lock(mutex);

        return doGetProfiles(fileProfiles, type);
    }

    std::vector<Glib::ustring> getProfilesFromDir(const Glib::ustring &dirName,
                                                  ProfileType type) const
    {
        ProfileMap profiles;

        MyMutex::MyLock lock(mutex);

        loadProfiles(dirName, &profiles, nullptr, nullptr, false);

        auto ret = doGetProfiles(profiles, type);
        for (auto &p : profiles) {
            if (fileProfiles.find(p.first) == fileProfiles.end()) {
                fileProfiles.insert(p);
            }
        }
        return ret;
    }

    std::uint8_t getInputIntents(cmsHPROFILE profile)
    {
        MyMutex::MyLock lock(mutex);

        return getSupportedIntents(profile, LCMS_USED_AS_INPUT);
    }

    std::uint8_t getOutputIntents(cmsHPROFILE profile)
    {
        MyMutex::MyLock lock(mutex);

        return getSupportedIntents(profile, LCMS_USED_AS_OUTPUT);
    }

    std::uint8_t getProofIntents(cmsHPROFILE profile)
    {
        MyMutex::MyLock lock(mutex);

        return getSupportedIntents(profile, LCMS_USED_AS_PROOF);
    }

    std::uint8_t getInputIntents(const Glib::ustring &name)
    {
        return getInputIntents(getProfile(name));
    }

    std::uint8_t getOutputIntents(const Glib::ustring &name)
    {
        return getOutputIntents(getProfile(name));
    }

    std::uint8_t getProofIntents(const Glib::ustring &name)
    {
        return getProofIntents(getProfile(name));
    }

    Glib::ustring getDefaultMonitorProfileName() const
    {
        return defaultMonitorProfile;
    }

    void setDefaultMonitorProfileName(const Glib::ustring &name)
    {
        MyMutex::MyLock lock(mutex);
        defaultMonitorProfile = name;
        update_thumbnail_monitor_transform();
    }

    std::vector<Glib::ustring> getWorkingProfiles()
    {
        std::vector<Glib::ustring> res;

        // for (unsigned int i = 0; i < sizeof(wpnames) / sizeof(wpnames[0]);
        // i++) {
        //     res.push_back(wpnames[i]);
        // }
        for (const auto &p : wProfiles) {
            res.push_back(p.first);
        }

        return res;
    }

    cmsHTRANSFORM getThumbnailMonitorTransform()
    {
        return thumb_monitor_xform_;
    }

    const std::string &getThumbnailMonitorHash() const
    {
        return monitor_profile_hash_;
    }

    bool getProfileMatrix(const Glib::ustring &name, Mat33<float> &out)
    {
        auto prof = getProfile(name);

        if (!prof) {
            if (name == procparams::ColorManagementParams::NoICMString) {
                auto m = workingSpaceMatrix("sRGB");
                for (int i = 0; i < 3; ++i) {
                    for (int j = 0; j < 3; ++j) {
                        out[i][j] = m[i][j];
                    }
                }
                return true;
            }
            return false;
        }

        return ICCStore::getProfileMatrix(prof, out);
    }

    cmsHPROFILE getActiveMonitorProfile() const
    {
        MyMutex::MyLock lock(mutex);
        return getActiveMonitorProfile_unlocked();
    }

    cmsHPROFILE
    getStdMonitorProfile(rtengine::Settings::StdMonitorProfile name) const
    {
        auto self = const_cast<Implementation *>(this);
        switch (name) {
        case rtengine::Settings::StdMonitorProfile::SRGB:
            return self->getProfile_unlocked("RTv4_sRGB");
        case rtengine::Settings::StdMonitorProfile::DISPLAY_P3:
            return self->getProfile_unlocked("RTv4_DisplayP3");
        case rtengine::Settings::StdMonitorProfile::ADOBE_RGB:
            return self->getProfile_unlocked("RTv4_Medium");
        default:
            return nullptr;
        }
    }

private:
    inline cmsHPROFILE getActiveMonitorProfile_unlocked() const
    {
        cmsHPROFILE monitor = nullptr;
        if (settings->color_mgmt_mode !=
            Settings::ColorManagementMode::APPLICATION) {
            monitor = getStdMonitorProfile(settings->os_monitor_profile);
        } else {
            monitor = const_cast<Implementation *>(this)->getProfile_unlocked(
                defaultMonitorProfile);
        }
        return monitor;
    }

    void update_thumbnail_monitor_transform()
    {
        if (thumb_monitor_xform_) {
            cmsDeleteTransform(thumb_monitor_xform_);
        }
        thumb_monitor_xform_ = nullptr;

        auto monitor = getActiveMonitorProfile_unlocked();
        if (monitor) {
            monitor_profile_hash_ = Glib::Checksum::compute_checksum(
                Glib::Checksum::CHECKSUM_MD5,
                ProfileContent(monitor).getData());
            switch (settings->monitorIntent) {
            case RI_PERCEPTUAL:
                monitor_profile_hash_.push_back('0');
                break;
            case RI_RELATIVE:
                monitor_profile_hash_.push_back('1');
                break;
            case RI_SATURATION:
                monitor_profile_hash_.push_back('2');
                break;
            case RI_ABSOLUTE:
                monitor_profile_hash_.push_back('3');
                break;
            default:
                monitor_profile_hash_.push_back('0');
                break;
            }

            cmsHPROFILE iprof = cmsCreateLab4Profile(nullptr);
            cmsUInt32Number flags = cmsFLAGS_NOOPTIMIZE | cmsFLAGS_NOCACHE;
            thumb_monitor_xform_ =
                cmsCreateTransform(iprof, TYPE_Lab_FLT, monitor, TYPE_RGB_FLT,
                                   settings->monitorIntent, flags);
            cmsCloseProfile(iprof);
        } else {
            monitor_profile_hash_ = "000000000000000000000000000000000";
        }
    }

    cmsHPROFILE getProfile_unlocked(const Glib::ustring &name)
    {
        const ProfileMap::const_iterator r = fileProfiles.find(name);

        if (r != fileProfiles.end()) {
            return r->second;
        }

        if (!name.compare(0, 5, "file:")) {
            const ProfileContent content(name.substr(5));
            const cmsHPROFILE profile = content.toProfile();

            if (profile) {
                fileProfiles.emplace(name, profile);
                fileProfileContents.emplace(name, content);

                return profile;
            }
        } else if (!loadAll) {
            // Look for a standard profile
            if (!loadProfile(name, profilesDir, &fileProfiles,
                             &fileProfileContents)) {
                loadProfile(name, userICCDir, &fileProfiles,
                            &fileProfileContents);
            }

            const ProfileMap::const_iterator r = fileProfiles.find(name);

            if (r != fileProfiles.end()) {
                return r->second;
            }
        }

        return nullptr;
    }

    using CVector = Vec3<float>;
    using CMatrix = Mat33<float>;
    struct PMatrix {
        float matrix[3][3];
        PMatrix(): matrix{} {}
        explicit PMatrix(const CMatrix &m) { set(m); }

        CMatrix toMatrix() const
        {
            CMatrix ret;

            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    ret[i][j] = matrix[i][j];
                }
            }

            return ret;
        }

        void set(const CMatrix &m)
        {
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    matrix[i][j] = m[i][j];
                }
            }
        }
    };

    bool computeWorkingSpaceMatrix(const Glib::ustring &path,
                                   const Glib::ustring &filename, PMatrix &out)
    {
        Glib::ustring fullpath = filename;

        if (!Glib::path_is_absolute(fullpath)) {
            fullpath = Glib::build_filename(path, filename);
        }

        ProfileContent content(fullpath);
        cmsHPROFILE prof = content.toProfile();

        if (!prof) {
            return false;
        }

        if (cmsGetColorSpace(prof) != cmsSigRgbData) {
            cmsCloseProfile(prof);
            return false;
        }

        if (!cmsIsMatrixShaper(prof)) {
            cmsCloseProfile(prof);
            return false;
        }

        cmsCIEXYZ *red = static_cast<cmsCIEXYZ *>(
            cmsReadTag(prof, cmsSigRedMatrixColumnTag));
        cmsCIEXYZ *green = static_cast<cmsCIEXYZ *>(
            cmsReadTag(prof, cmsSigGreenMatrixColumnTag));
        cmsCIEXYZ *blue = static_cast<cmsCIEXYZ *>(
            cmsReadTag(prof, cmsSigBlueMatrixColumnTag));

        if (!red || !green || !blue) {
            cmsCloseProfile(prof);
            return false;
        }

        CMatrix m(float(red->X), float(green->X), float(blue->X), float(red->Y),
                  float(green->Y), float(blue->Y), float(red->Z),
                  float(green->Z), float(blue->Z));
        m[1][0] = red->Y;
        m[1][1] = green->Y;
        m[1][2] = blue->Y;
        m[2][0] = red->Z;
        m[2][1] = green->Z;
        m[2][2] = blue->Z;
        out.set(m);

        cmsCloseProfile(prof);
        return true;
    }

    bool loadWorkingSpaces(const Glib::ustring &path)
    {
        Glib::ustring fileName =
            Glib::build_filename(path, "workingspaces.json");
        FILE *const f = g_fopen(fileName.c_str(), "r");

        if (settings->verbose) {
            std::cout << "trying to load extra working spaces from " << fileName
                      << std::flush;
        }

        if (!f) {
            if (settings->verbose) {
                std::cout << " FAIL" << std::endl;
            }

            return false;
        }

        fseek(f, 0, SEEK_END);
        long length = ftell(f);

        if (length <= 0) {
            if (settings->verbose) {
                std::cout << " FAIL" << std::endl;
            }

            fclose(f);
            return false;
        }

        cJSON *root = nullptr;
        {
            std::vector<char> bufvec(length + 1);
            char *buf = &bufvec[0];
            fseek(f, 0, SEEK_SET);
            length = fread(buf, 1, length, f);
            buf[length] = 0;

            fclose(f);

            cJSON_Minify(buf);
            root = cJSON_Parse(buf);
        }

        if (!root) {
            if (settings->verbose) {
                std::cout << " FAIL" << std::endl;
            }

            return false;
        }

        cJSON *js = cJSON_GetObjectItem(root, "working_spaces");

        if (!js) {
            goto parse_error;
        }

        for (js = js->child; js != nullptr; js = js->next) {
            cJSON *ji = cJSON_GetObjectItem(js, "name");
            std::unique_ptr<PMatrix> m(new PMatrix);
            std::string name;

            if (!ji || ji->type != cJSON_String) {
                goto parse_error;
            }

            name = ji->valuestring;

            if (wProfiles.find(name) != wProfiles.end()) {
                continue; // already there -- ignore
            }

            bool found_matrix = false;

            ji = cJSON_GetObjectItem(js, "matrix");

            if (ji) {
                if (ji->type != cJSON_Array) {
                    goto parse_error;
                }

                ji = ji->child;

                for (int i = 0; i < 3; ++i) {
                    for (int j = 0; j < 3; ++j, ji = ji->next) {
                        if (!ji || ji->type != cJSON_Number) {
                            goto parse_error;
                        }

                        m->matrix[i][j] = ji->valuedouble;
                    }
                }

                if (ji) {
                    goto parse_error;
                }

                found_matrix = true;
            } else {
                ji = cJSON_GetObjectItem(js, "file");

                if (!ji || ji->type != cJSON_String) {
                    goto parse_error;
                }

                found_matrix =
                    computeWorkingSpaceMatrix(path, ji->valuestring, *m);
            }

            if (!found_matrix) {
                if (settings->verbose) {
                    std::cout
                        << "Could not find suitable matrix for working space: "
                        << name << std::endl;
                }

                continue;
            }

            pMatrices.emplace_back(std::move(m));
            TMatrix w = pMatrices.back()->matrix;

            CMatrix b;

            if (!inverse(pMatrices.back()->toMatrix(), b)) {
                if (settings->verbose) {
                    std::cout << "Matrix for working space: " << name
                              << " is not invertible, skipping" << std::endl;
                }

                pMatrices.pop_back();
            } else {
                wMatrices[name] = w;
                pMatrices.emplace_back(new PMatrix(b));
                TMatrix iw = pMatrices.back()->matrix;
                iwMatrices[name] = iw;
                wProfiles[name] = createFromMatrix(w);

                if (settings->verbose) {
                    std::cout << "Added working space: " << name << std::endl;
                    std::cout << "  matrix: [";

                    for (int i = 0; i < 3; ++i) {
                        std::cout << " [";

                        for (int j = 0; j < 3; ++j) {
                            std::cout << " " << w[i][j];
                        }

                        std::cout << "]";
                    }

                    std::cout << " ]" << std::endl;
                }
            }
        }

        cJSON_Delete(root);

        if (settings->verbose) {
            std::cout << " OK" << std::endl;
        }

        return true;

    parse_error:

        if (settings->verbose) {
            std::cout << " ERROR in parsing " << fileName << std::endl;
        }

        cJSON_Delete(root);
        return false;
    }

    ProfileMap wProfiles;
    // ProfileMap wProfilesGamma;
    MatrixMap wMatrices;
    MatrixMap iwMatrices;

    std::vector<std::unique_ptr<PMatrix>> pMatrices;

    // These contain profiles from user/system directory(supplied on init)
    Glib::ustring profilesDir;
    Glib::ustring userICCDir;
    mutable ProfileMap fileProfiles;
    ContentMap fileProfileContents;

    // These contain standard profiles from RT. Keys are all in uppercase.
    Glib::ustring stdProfilesDir;
    NameMap fileCamProfilesFileNames;
    ProfileMap fileCamProfiles;

    Glib::ustring defaultMonitorProfile;

    bool loadAll;

    const cmsHPROFILE xyz;
    const cmsHPROFILE srgb;

    mutable MyMutex mutex;

    cmsHTRANSFORM thumb_monitor_xform_;
    std::string monitor_profile_hash_;
};

ICCStore *ICCStore::getInstance()
{
    static ICCStore instance;
    return &instance;
}

void ICCStore::init(const Glib::ustring &usrICCDir,
                    const Glib::ustring &stdICCDir, bool loadAll)
{
#ifdef ART_LCMS2_FAST_FLOAT
    cmsPlugin(cmsFastFloatExtensions());
#endif
    implementation->init(usrICCDir, stdICCDir, loadAll);
}

cmsHPROFILE ICCStore::workingSpace(const Glib::ustring &name) const
{
    return implementation->workingSpace(name);
}

// cmsHPROFILE ICCStore::workingSpaceGamma(const Glib::ustring& name) const
// {
//     return implementation->workingSpaceGamma(name);
// }

TMatrix ICCStore::workingSpaceMatrix(const Glib::ustring &name) const
{
    return implementation->workingSpaceMatrix(name);
}

TMatrix ICCStore::workingSpaceInverseMatrix(const Glib::ustring &name) const
{
    return implementation->workingSpaceInverseMatrix(name);
}

bool ICCStore::outputProfileExist(const Glib::ustring &name) const
{
    return implementation->outputProfileExist(name);
}

cmsHPROFILE ICCStore::getProfile(const Glib::ustring &name) const
{
    return implementation->getProfile(name);
}

std::string ICCStore::getProfileTag(cmsHPROFILE profile, cmsTagSignature tag)
{
    return Implementation::getProfileTag(profile, tag);
}

cmsHPROFILE ICCStore::getCameraProfile(const Glib::ustring &name) const
{
    return implementation->getCameraProfile(name);
}

ProfileContent ICCStore::getContent(const Glib::ustring &name) const
{
    return implementation->getContent(name);
}

Glib::ustring ICCStore::getDefaultMonitorProfileName() const
{
    return implementation->getDefaultMonitorProfileName();
}

void ICCStore::setDefaultMonitorProfileName(const Glib::ustring &name)
{
    implementation->setDefaultMonitorProfileName(name);
}

cmsHPROFILE ICCStore::getXYZProfile() const
{
    return implementation->getXYZProfile();
}

cmsHPROFILE ICCStore::getsRGBProfile() const
{
    return implementation->getsRGBProfile();
}

std::vector<Glib::ustring> ICCStore::getProfiles(ProfileType type) const
{
    return implementation->getProfiles(type);
}

std::vector<Glib::ustring>
ICCStore::getProfilesFromDir(const Glib::ustring &dirName,
                             ProfileType type) const
{
    return implementation->getProfilesFromDir(dirName, type);
}

std::uint8_t ICCStore::getInputIntents(cmsHPROFILE profile) const
{
    return implementation->getInputIntents(profile);
}

std::uint8_t ICCStore::getOutputIntents(cmsHPROFILE profile) const
{
    return implementation->getOutputIntents(profile);
}

std::uint8_t ICCStore::getProofIntents(cmsHPROFILE profile) const
{
    return implementation->getProofIntents(profile);
}

std::uint8_t ICCStore::getInputIntents(const Glib::ustring &name) const
{
    return implementation->getInputIntents(name);
}

std::uint8_t ICCStore::getOutputIntents(const Glib::ustring &name) const
{
    return implementation->getOutputIntents(name);
}

std::uint8_t ICCStore::getProofIntents(const Glib::ustring &name) const
{
    return implementation->getProofIntents(name);
}

cmsHTRANSFORM ICCStore::getThumbnailMonitorTransform()
{
    return implementation->getThumbnailMonitorTransform();
}

const std::string &ICCStore::getThumbnailMonitorHash() const
{
    return implementation->getThumbnailMonitorHash();
}

bool ICCStore::getProfileMatrix(const Glib::ustring &name, Mat33<float> &out)
{
    return implementation->getProfileMatrix(name, out);
}

cmsHPROFILE
ICCStore::getStdMonitorProfile(Settings::StdMonitorProfile name) const
{
    return implementation->getStdMonitorProfile(name);
}

cmsHPROFILE ICCStore::getActiveMonitorProfile() const
{
    return implementation->getActiveMonitorProfile();
}

bool ICCStore::getProfileMatrix(cmsHPROFILE prof, Mat33<float> &out)
{
    if (cmsGetColorSpace(prof) == cmsSigRgbData && cmsIsMatrixShaper(prof) &&
        !cmsIsCLUT(prof, RI_PERCEPTUAL, LCMS_USED_AS_OUTPUT)) {

        cmsCIEXYZ *red = static_cast<cmsCIEXYZ *>(
            cmsReadTag(prof, cmsSigRedMatrixColumnTag));
        cmsCIEXYZ *green = static_cast<cmsCIEXYZ *>(
            cmsReadTag(prof, cmsSigGreenMatrixColumnTag));
        cmsCIEXYZ *blue = static_cast<cmsCIEXYZ *>(
            cmsReadTag(prof, cmsSigBlueMatrixColumnTag));

        if (red && green && blue) {
            out[0][0] = red->X;
            out[0][1] = green->X;
            out[0][2] = blue->X;
            out[1][0] = red->Y;
            out[1][1] = green->Y;
            out[1][2] = blue->Y;
            out[2][0] = red->Z;
            out[2][1] = green->Z;
            out[2][2] = blue->Z;
            return true;
        }
    }

    return false;
}

bool ICCStore::getProfileParametricTRC(cmsHPROFILE prof, float &out_gamma,
                                       float &out_slope)
{
    if (is_ART_profile(prof)) {
        double g = 0, s = 0;
        if (get_ART_gamma_slope(prof, g, s)) {
            out_gamma = g;
            out_slope = s;
            return true;
        }
    }
    return false;
}

ICCStore::ICCStore(): implementation(new Implementation) {}

ICCStore::~ICCStore() = default;

std::vector<Glib::ustring> ICCStore::getWorkingProfiles()
{
    return implementation->getWorkingProfiles();
}

// WARNING: the caller must lock lcmsMutex
cmsHPROFILE ICCStore::makeStdGammaProfile(cmsHPROFILE iprof)
{
    // forgive me for the messy code, quick hack to change gamma of an ICC
    // profile to the RT standard gamma
    if (!iprof) {
        return nullptr;
    }

    cmsUInt32Number bytesNeeded = 0;
    cmsSaveProfileToMem(iprof, nullptr, &bytesNeeded);

    if (bytesNeeded == 0) {
        return nullptr;
    }

    uint8_t *data = new uint8_t[bytesNeeded + 1];
    cmsSaveProfileToMem(iprof, data, &bytesNeeded);
    const uint8_t *p = &data[128]; // skip 128 byte header
    uint32_t tag_count;
    memcpy(&tag_count, p, 4);
    p += 4;
    tag_count = ntohl(tag_count);

    struct icctag {
        uint32_t sig;
        uint32_t offset;
        uint32_t size;
    } tags[tag_count];

    constexpr uint32_t gamma = 0x239;
    constexpr int gamma_size = 14;
    int data_size = (gamma_size + 3) & ~3;

    for (uint32_t i = 0; i < tag_count; i++) {
        memcpy(&tags[i], p, 12);
        tags[i].sig = ntohl(tags[i].sig);
        tags[i].offset = ntohl(tags[i].offset);
        tags[i].size = ntohl(tags[i].size);
        p += 12;

        if (tags[i].sig != 0x62545243 && // bTRC
            tags[i].sig != 0x67545243 && // gTRC
            tags[i].sig != 0x72545243 && // rTRC
            tags[i].sig != 0x6B545243) { // kTRC
            data_size += (tags[i].size + 3) & ~3;
        }
    }

    uint32_t sz = 128 + 4 + tag_count * 12 + data_size;
    uint8_t *nd = new uint8_t[sz];
    memset(nd, 0, sz);
    memcpy(nd, data, 128 + 4);
    sz = htonl(sz);
    memcpy(nd, &sz, 4);
    uint32_t offset = 128 + 4 + tag_count * 12;
    uint32_t gamma_offset = 0;

    for (uint32_t i = 0; i < tag_count; i++) {
        struct icctag tag;
        tag.sig = htonl(tags[i].sig);

        if (tags[i].sig == 0x62545243 || // bTRC
            tags[i].sig == 0x67545243 || // gTRC
            tags[i].sig == 0x72545243 || // rTRC
            tags[i].sig == 0x6B545243) { // kTRC
            if (gamma_offset == 0) {
                gamma_offset = offset;
                uint32_t pcurve[] = {htonl(0x63757276), htonl(0),
                                     htonl(/*gamma_size == 12 ? 0U : */ 1U)};
                memcpy(&nd[offset], pcurve, 12);

                // if (gamma_size == 14) {
                uint16_t gm = htons(gamma);
                memcpy(&nd[offset + 12], &gm, 2);
                //}

                offset += (gamma_size + 3) & ~3;
            }

            tag.offset = htonl(gamma_offset);
            tag.size = htonl(gamma_size);
        } else {
            tag.offset = htonl(offset);
            tag.size = htonl(tags[i].size);
            memcpy(&nd[offset], &data[tags[i].offset], tags[i].size);
            offset += (tags[i].size + 3) & ~3;
        }

        memcpy(&nd[128 + 4 + i * 12], &tag, 12);
    }

    cmsHPROFILE oprof = cmsOpenProfileFromMem(nd, ntohl(sz));
    delete[] nd;
    delete[] data;
    return oprof;
}

cmsHPROFILE ICCStore::createFromMatrix(const float matrix[3][3], bool gamma,
                                       const Glib::ustring &name)
{

    static const unsigned phead[] = {
        1024, 0, 0x2100000,  0x6d6e7472, 0x52474220, 0x58595a20, 0,
        0,    0, 0x61637370, 0,          0,          0,          0,
        0,    0, 0,          0xf6d6,     0x10000,    0xd32d};
    unsigned pbody[] = {10,         0x63707274, 0,  36, /* cprt */
                        0x64657363, 0,          40,     /* desc */
                        0x77747074, 0,          20,     /* wtpt */
                        0x626b7074, 0,          20,     /* bkpt */
                        0x72545243, 0,          14,     /* rTRC */
                        0x67545243, 0,          14,     /* gTRC */
                        0x62545243, 0,          14,     /* bTRC */
                        0x7258595a, 0,          20,     /* rXYZ */
                        0x6758595a, 0,          20,     /* gXYZ */
                        0x6258595a, 0,          20}; /* bXYZ */
    static const unsigned pwhite[] = {0xf351, 0x10000, 0x116cc}; // D65
    // static const unsigned pwhite[] = { 0xf6d6, 0x10000, 0xd340 };//D50

    // 0x63757276 : curveType, 0 : reserved, 1 : entries(1=gamma, 0=identity),
    // 0x1000000=1.0
    unsigned pcurve[] = {0x63757276, 0, 0, 0x1000000};
    //    unsigned pcurve[] = { 0x63757276, 0, 1, 0x1000000 };

    if (gamma) {
        pcurve[2] = 1;
        // pcurve[3] = 0x1f00000;// pcurve for gamma BT709 : g=2.22 s=4.5
        // normalize gamma in RT, default(Emil's choice = sRGB)
        pcurve[3] = 0x2390000; // pcurve for gamma sRGB : g:2.4 s=12.92310

    } else {
        // lcms2 up to 2.4 has a bug with linear gamma causing precision
        // loss(banding) of floating point data when a normal icc encoding of
        // linear gamma is used
        //(i e 0 table entries), but by encoding a gamma curve which is 1.0 the
        // floating point path is taken within lcms2 so no precision loss occurs
        // and gamma is still 1.0.
        pcurve[2] = 1;
        pcurve[3] = 0x1000000; // pcurve for gamma 1
    }

    // constructing profile header
    unsigned *oprof = new unsigned[phead[0] / sizeof(unsigned)];
    memset(oprof, 0, phead[0]);
    memcpy(oprof, phead, sizeof(phead));

    oprof[0] = 132 + 12 * pbody[0];

    // constructing tag directory(pointers inside the file), and types
    // 0x74657874 : text
    // 0x64657363 : description tag
    for (unsigned int i = 0; i < pbody[0]; i++) {
        oprof[oprof[0] / 4] =
            i ? (i > 1 ? 0x58595a20 : 0x64657363) : 0x74657874;
        pbody[i * 3 + 2] = oprof[0];
        oprof[0] += (pbody[i * 3 + 3] + 3) & -4;
    }

    memcpy(oprof + 32, pbody, sizeof(pbody));

    // wtpt
    memcpy((char *)oprof + pbody[8] + 8, pwhite, sizeof(pwhite));

    // r/g/b TRC
    for (int i = 4; i < 7; i++) {
        memcpy((char *)oprof + pbody[i * 3 + 2], pcurve, sizeof(pcurve));
    }

    // r/g/b XYZ
    //    pseudoinverse((double(*)[3]) out_rgb[output_color-1], inverse, 3);
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) {
            oprof[pbody[j * 3 + 23] / 4 + i + 2] = matrix[i][j] * 0x10000 + 0.5;
            //      for (num = k=0; k < 3; k++)
            //        num += xyzd50_srgb[i][k] * inverse[j][k];
        }

    // convert to network byte order
    for (unsigned int i = 0; i < phead[0] / 4; i++) {
        oprof[i] = htonl(oprof[i]);
    }

    // cprt
    strcpy((char *)oprof + pbody[2] + 8, "--rawtherapee profile--");

    // desc
    oprof[pbody[5] / 4 + 2] = name.size() + 1;
    strcpy((char *)oprof + pbody[5] + 12, name.c_str());

    cmsHPROFILE p = cmsOpenProfileFromMem(oprof, ntohl(oprof[0]));
    delete[] oprof;
    return p;
}

cmsHPROFILE ICCStore::createFromMatrix(const double matrix[3][3], bool gamma,
                                       const Glib::ustring &name)
{
    float fmatrix[3][3];
    to_float_matrix(matrix, fmatrix);
    return createFromMatrix(fmatrix, gamma, name);
}

} // namespace rtengine
