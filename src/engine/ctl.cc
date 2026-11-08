/** -*- C++ -*-
 *
 *  This file is part of ART
 *
 *  Copyright (c) 2026 Alberto Griggio <alberto.griggio@gmail.com>
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

#ifdef ART_USE_CTL

#include "ctl.h"

#include "color.h"
#include "curves.h"
#include "guidedfilter.h"
#include "iccstore.h"
#include "imagefloat.h"
#include "linalgebra.h"
#include "opthelper.h"
#include "rt_math.h"
#include "settings.h"
#include "utils.h"

#include <algorithm>
#include <fstream>
#include <giomm.h>
#include <iostream>
#include <locale.h>
#include <set>
#include <sstream>
#include <unordered_map>

#include "../utils/multilangmgr.h"
#include "../gui/options.h"
#include "../utils/editid.h"
#include "../utils/pathutils.h"
#include "cJSON.h"

#define BENCHMARK
#include "StopWatch.h"

namespace art { namespace engine {

extern const Settings *settings;

//-----------------------------------------------------------------------------
// script parsing
//-----------------------------------------------------------------------------

namespace {

bool fill_from_json(std::unordered_map<std::string, int> &name2pos,
                    CLUTParamDescriptorList &params, cJSON *root,
                    std::string &name)
{
    if (!cJSON_IsArray(root)) {
        return false;
    }

    auto sz = cJSON_GetArraySize(root);
    if (sz < 2) {
        return false;
    }

    auto n = cJSON_GetArrayItem(root, 0);
    if (!cJSON_IsString(n)) {
        return false;
    }
    name = cJSON_GetStringValue(n);
    auto it = name2pos.find(name);
    if (it == name2pos.end()) {
        return false;
    }
    auto &desc = params[it->second];

    name2pos.erase(it);

    return desc.fill_from_json(root);
}

struct GuideName {
    const char *name;
    CTLGuide guide;
};

// the guides that can be requested in @ART-guides
const GuideName guide_names[] = {
    {"Y", CTLGuide::Y}, {"C", CTLGuide::C}, {"H", CTLGuide::H}};

} // namespace

const char *ctl_guide_name(CTLGuide g)
{
    for (auto &n : guide_names) {
        if (n.guide == g) {
            return n.name;
        }
    }
    return "";
}

bool parse_ctl_guide(const std::string &name, CTLGuide &out)
{
    for (auto &n : guide_names) {
        if (name == n.name) {
            out = n.guide;
            return true;
        }
    }
    return false;
}

namespace {

/**
 * Read the @ART-guides tag of the script, if any. Returns false on error
 * (with a message in verbose mode); found is set to true if the tag is
 * present.
 */
bool get_CTL_guides(const Glib::ustring &filename, std::vector<CTLGuide> &out,
                    bool &found)
{
    out.clear();
    found = false;
    if (!Glib::file_test(filename, Glib::FILE_TEST_EXISTS)) {
        return true;
    }
    int cur_line = 0;
    const auto err = [&](const std::string &msg) -> bool {
        if (settings->verbose) {
            std::cout << filename << ":" << cur_line << ": Error: " << msg
                      << "\n" << std::endl;
        }
        return false;
    };
    auto fn = Glib::filename_from_utf8(filename);
    std::ifstream src(fn.c_str());
    std::string line;
    while (src && std::getline(src, line)) {
        ++cur_line;
        size_t s = 0;
        while (s < line.size() && std::isspace(line[s])) {
            ++s;
        }
        if (s + 1 < line.size() && line[s] == '/' && line[s + 1] == '/') {
            s += 2;
        }
        while (s < line.size() && std::isspace(line[s])) {
            ++s;
        }
        if (line.find("@ART-guides:", s) != s) {
            continue;
        }
        line = line.substr(12 + s);
        if (found) {
            return err("duplicate @ART-guides definition");
        }
        found = true;
        cJSON *root = cJSON_Parse(line.c_str());
        if (!root || !cJSON_IsArray(root)) {
            if (root) {
                cJSON_Delete(root);
            }
            return err("invalid guides definition:\n  " + line);
        }
        const int n = cJSON_GetArraySize(root);
        bool ok = n >= 1 && n <= CTL_MAX_GUIDES;
        if (!ok) {
            err("the number of guides must be between 1 and " +
                std::to_string(CTL_MAX_GUIDES) + ":\n  " + line);
        }
        cJSON *item = nullptr;
        cJSON_ArrayForEach(item, root)
        {
            if (!ok) {
                break;
            }
            CTLGuide g;
            if (!cJSON_IsString(item) ||
                !parse_ctl_guide(cJSON_GetStringValue(item), g)) {
                ok = err("unknown guide in @ART-guides:\n  " + line);
            } else if (std::find(out.begin(), out.end(), g) != out.end()) {
                ok = err("duplicate guide in @ART-guides:\n  " + line);
            } else {
                out.push_back(g);
            }
        }
        cJSON_Delete(root);
        if (!ok) {
            out.clear();
            return false;
        }
    }
    return true;
}

/**
 * ART-compatible CTL scripts can contain parameters as additional uniform
 * input parameters to ART_main. The supported parameter types are "bool",
 * "int", "float" and arrays of floats. Each such parameter must come with an
 * associated ART parameter definition in the CTL script. ART parameter
 * definitions are special comment lines of the following form:
 *
 *  // @ART-param: <param-def>
 *
 * where <param-def> is an array in JSON format, whose content is described in
 * the comment of CLUTParamDescriptor::fill_from_json() (in clutparams.cc)
 *
 * If default values are not given in the ART parameter definition, they are
 * taken from the definition of the ART_main function. If no default is given,
 * zero is used.
 *
 * Example:
 *
 * // @ART-param: ["param_float", "A float slider", -1.0, 1.0, 0.5, 0.1]
 * // @ART-param: ["param_int", "An int slider", -10, 10]
 * // @ART-param: ["param_bool", "A checkbox", true]
 * // @ART-param: ["param_choice", "A combo box", ["Option A", "Option B"], 1,
 * "Select between option A (value 0) and option B (value 1)"]
 *
 * ART_main can optionally take up to 3 guides: varying float inputs
 * following r, g and b (before the output arguments):
 *
 *  // @ART-guides: ["Y", "C", "H"]
 *  void ART_main(varying float r, varying float g, varying float b,
 *                varying float guide_y, varying float guide_c,
 *                varying float guide_h, output varying float rout, ...)
 *
 * A guide is a spatially smoothed (edge-aware) quantity, computed from the
 * input of the tool (i.e. before any of its regions is applied), that gives a
 * script some information about the neighbourhood of the pixel. The
 * @ART-guides tag lists, in the order of the arguments, how each guide is
 * computed (no duplicates are allowed):
 *
 *  - "Y": the luminance of the input image, computed as in the "Tone
 *         Equalizer" tool (regularization 4). It is a linear luminance,
 *         normalised like the tone equalizer does: 0.18 is middle grey.
 *  - "C": a chroma-like quantity: the smoothed value of the "cz" output of
 *         Color::rgb2jzczhz (i.e. the chroma in the JzAzBz colour space),
 *         multiplied by 15, so that it is 0 for neutral colours and close to
 *         1 for strongly saturated ones
 *  - "H": a hue-like quantity: the smoothed value of the "hz" output of
 *         Color::rgb2jzczhz (the hue angle in the JzAzBz colour space, in
 *         radians, in the range (-pi, pi]). Note that it is an angle, so the
 *         differences between hues must be wrapped by the script.
 *
 * If the tag is missing, a script with exactly one guide gets "Y" (so
 * @ART-guides is only needed for other guides, or for more than one). The
 * number of strings in the tag must match the number of guides taken by
 * ART_main.
 *
 * The guides are not in the colour space of the script, but in the working
 * profile. Scripts taking a guide are never baked into a 3D LUT, so any
 * @ART-lut annotation is ignored. They can only be used by tools that
 * provide guides (Color/Tone Correction, Film Simulation). If ART_main takes
 * guides, so must ART_fill_pipette, in the same positions (before curve_id).
 *
 * Curve parameters can support pipettes (picking a point of the curve from a
 * pixel of the preview). A script declares which curves do with
 *
 *  // @ART-pipette: ["curve_a", "curve_b"]
 *
 * (curve_id is the index in this list), and provides a second entry point
 *
 *  void ART_fill_pipette(varying float r, varying float g, varying float b,
 *                        [varying float guide, ...] (the same as ART_main)
 *                        output varying float rout, output varying float gout,
 *                        output varying float bout, int curve_id,
 *                        <optional parameters, as for ART_main>)
 *
 * which receives the same input as ART_main and writes, in the *_out
 * variables, the position (in [0, 1]) in the curve curve_id that corresponds
 * to the pixel (-1 for an unused channel).
 *
 * void ART_main(varying float r, varying float g, varying float b,
 *               output varying float or, output varying float og, output
 * varying float ob, float param_float, int param_int, bool param_bool, int
 * param_choice)
 * {
 *    // ...
 * }
 */
bool get_CTL_params(const Glib::ustring &filename,
                    std::shared_ptr<Ctl::Interpreter> intp,
                    Ctl::FunctionCallPtr func, CLUTParamDescriptorList &out,
                    Glib::ustring &colorspace, int &lut_dim,
                    const std::vector<CTLGuide> &guides)
{
    setlocale(LC_NUMERIC, "C");

    out.clear();
    std::unordered_map<std::string, int> name2pos;

    colorspace = "";
    const bool uses_guide = !guides.empty();
    if (uses_guide) {
        lut_dim = -1;
    }

    static std::unordered_map<std::string, std::string> profilemap = {
        {"aces2065-1", "ACESp0"},  {"acescg", "ACESp1"},
        {"rec2020", "Rec2020"},    {"prophoto", "ProPhoto"},
        {"rec709", "sRGB"},        {"srgb", "sRGB"},
        {"adobergb", "Adobe RGB"}, {"adobe", "Adobe RGB"}};

    int cur_line = 0;

    const auto err = [&](const std::string &msg) -> bool {
        if (settings->verbose) {
            std::cout << filename << ":";
            if (cur_line > 0) {
                std::cout << cur_line << ":";
            }
            std::cout << " Error: " << msg << "\n" << std::endl;
        }
        return false;
    };

    for (size_t i = 3 + guides.size(), n = func->numInputArgs(); i < n; ++i) {
        auto a = func->inputArg(i);
        if (a->isVarying()) {
            return err("parameter " + a->name() + " is varying");
        }
        CLUTParamType tp = CLUTParamType::PT_INT;
        switch (a->type()->cDataType()) {
        case Ctl::BoolTypeEnum:
            tp = CLUTParamType::PT_BOOL;
            break;
        case Ctl::IntTypeEnum:
            tp = a->type().cast<Ctl::BoolType>() ? CLUTParamType::PT_BOOL
                                                 : CLUTParamType::PT_INT;
            break;
        case Ctl::FloatTypeEnum:
            tp = CLUTParamType::PT_FLOAT;
            break;
        case Ctl::ArrayTypeEnum:
            if (a->type()
                    .cast<Ctl::ArrayType>()
                    ->elementType()
                    .cast<Ctl::FloatType>()) {
                tp = CLUTParamType::PT_CURVE;
                break;
            } else {
                // fall back to error
            }
        default:
            return err("parameter " + a->name() + " is of unsupported type");
        }

        std::string name = a->name();
        name2pos[name] = out.size();

        out.emplace_back();
        auto &desc = out.back();
        desc.name = name;
        desc.type = tp;

        desc.value_min = 0;
        desc.value_max = 1;
        desc.value_default = {0};

        if (a->hasDefaultValue()) {
            switch (tp) {
            case CLUTParamType::PT_BOOL:
                desc.value_default[0] = *reinterpret_cast<bool *>(a->data());
                break;
            case CLUTParamType::PT_FLOAT:
                desc.value_default[0] = *reinterpret_cast<float *>(a->data());
                break;
            case CLUTParamType::PT_INT:
                desc.value_default[0] = *reinterpret_cast<int *>(a->data());
            default:
                break;
            }
        }
    }

    std::unordered_map<std::string, int> order;
    std::vector<std::string> pipette_curves;
    int pipette_line = 0;

    if (Glib::file_test(filename, Glib::FILE_TEST_EXISTS)) {
        auto fn = Glib::filename_from_utf8(filename);
        std::ifstream src(fn.c_str());
        std::string line;
        std::string name;
        while (src && std::getline(src, line)) {
            ++cur_line;
            size_t s = 0;
            while (s < line.size() && std::isspace(line[s])) {
                ++s;
            }
            if (s + 1 < line.size() && line[s] == '/' && line[s + 1] == '/') {
                s += 2;
            }
            while (s < line.size() && std::isspace(line[s])) {
                ++s;
            }
            if (line.find("@ART-param:", s) == s) {
                line = line.substr(11 + s);
                cJSON *root = cJSON_Parse(line.c_str());
                if (!root) {
                    return err("bad parameter definition:\n  " + line);
                }
                bool ok = fill_from_json(name2pos, out, root, name);
                cJSON_Delete(root);
                if (!ok) {
                    return err("bad parameter definition:\n  " + line);
                }
                order[name] = cur_line;
            } else if (line.find("@ART-colorspace:", s) == s) {
                line = line.substr(16 + s);
                cJSON *root = cJSON_Parse(line.c_str());
                if (!root || !cJSON_IsString(root)) {
                    return err("invalid colorspace definition:\n  " + line);
                }
                std::string name =
                    Glib::ustring(cJSON_GetStringValue(root)).casefold();
                cJSON_Delete(root);
                auto it = profilemap.find(name);
                if (it != profilemap.end()) {
                    colorspace = it->second;
                } else {
                    return err("invalid colorspace definition:\n  " + line);
                }
            } else if (line.find("@ART-lut:", s) == s) {
                line = line.substr(9 + s);
                cJSON *root = cJSON_Parse(line.c_str());
                if (!root || !cJSON_IsNumber(root)) {
                    return err("invalid lut definition:\n  " + line);
                }
                double v = root->valuedouble;
                cJSON_Delete(root);
                lut_dim = int(v);
                if (lut_dim < -1 || v != lut_dim) {
                    return err("invalid lut definition:\n  " + line);
                }
                if (uses_guide) {
                    if (settings->verbose) {
                        std::cout << filename << ":" << cur_line
                                  << ": Warning: @ART-lut is ignored in "
                                     "scripts taking a guide"
                                  << std::endl;
                    }
                    lut_dim = -1;
                }
            } else if (line.find("@ART-pipette:", s) == s) {
                line = line.substr(13 + s);
                cJSON *root = cJSON_Parse(line.c_str());
                if (!root || !cJSON_IsArray(root)) {
                    if (root) {
                        cJSON_Delete(root);
                    }
                    return err("invalid pipette definition:\n  " + line);
                }
                pipette_curves.clear();
                pipette_line = cur_line;
                bool ok = true;
                cJSON *item = nullptr;
                cJSON_ArrayForEach(item, root)
                {
                    if (cJSON_IsString(item)) {
                        pipette_curves.push_back(cJSON_GetStringValue(item));
                    } else {
                        ok = false;
                    }
                }
                cJSON_Delete(root);
                if (!ok) {
                    return err("invalid pipette definition:\n  " + line);
                }
            } else if (line.find("@ART-preset:", s) == s) {
                line = line.substr(12 + s);
                cJSON *root = cJSON_Parse(line.c_str());
                if (!root) {
                    return err("invalid preset definition:\n  " + line);
                }
                bool ok = out.add_preset_from_json(root);
                cJSON_Delete(root);
                if (!ok) {
                    return err("invalid preset definition:\n  " + line);
                }
            }
        }
    } else {
        return err("file reading error");
    }

    cur_line = 0;
    if (!name2pos.empty() && !out.empty()) {
        std::string msg =
            "the following parameter definitions are missing:\n  ";
        const char *sep = "";
        for (auto &p : name2pos) {
            msg += sep + p.first;
            sep = ", ";
        }
        return err(msg);
    }

    if (pipette_curves.size() > art::utils::EUID_CTL_CURVE_MASK + 1) {
        cur_line = pipette_line;
        return err("too many curves in the pipette definition");
    }
    for (size_t i = 0; i < pipette_curves.size(); ++i) {
        auto it = std::find_if(out.begin(), out.end(),
                               [&](const CLUTParamDescriptor &d) {
                                   return d.name == pipette_curves[i];
                               });
        cur_line = pipette_line;
        if (it == out.end()) {
            return err("pipette defined for unknown parameter " +
                       pipette_curves[i]);
        } else if (it->type != CLUTParamType::PT_CURVE &&
                   it->type != CLUTParamType::PT_FLATCURVE &&
                   it->type != CLUTParamType::PT_FLATCURVE_PERIODIC) {
            return err("pipette defined for non-curve parameter " +
                       pipette_curves[i]);
        } else if (it->pipette_id >= 0) {
            return err("duplicate pipette for parameter " + pipette_curves[i]);
        }
        it->pipette_id = int(i);
    }
    cur_line = 0;

    const auto lt =[&](const CLUTParamDescriptor &a,
                        const CLUTParamDescriptor &b) -> bool {
        return order[a.name] < order[b.name];
    };
    std::sort(out.begin(), out.end(), lt);

    return true;
}

/**
 * Check that the script, if it declares pipettes, also provides a well-formed
 * ART_fill_pipette. If it does not, the pipettes are disabled (by the caller),
 * but this is not an error: the script is still usable.
 */
bool check_CTL_pipette(const Glib::ustring &filename,
                       std::shared_ptr<Ctl::Interpreter> intp,
                       const CLUTParamDescriptorList &params,
                       const std::vector<CTLGuide> &guides)
{
    bool wanted = false;
    for (auto &p : params) {
        wanted = wanted || p.pipette_id >= 0;
    }
    if (!wanted) {
        return true;
    }

    const auto warn = [&](const std::string &msg) -> bool {
        if (settings->verbose) {
            std::cout << filename << ": Warning: " << msg
                      << "; pipettes disabled" << std::endl;
        }
        return false;
    };

    Ctl::FunctionCallPtr f;
    try {
        f = intp->newFunctionCall("ART_fill_pipette");
    } catch (std::exception &) {
        return warn("@ART-pipette given but no ART_fill_pipette function");
    }
    const int id_pos = 3 + int(guides.size());
    if (f->numInputArgs() < id_pos + 1 || f->numOutputArgs() != 3) {
        return warn("wrong number of arguments to ART_fill_pipette");
    }
    for (int i = 0; i < 3; ++i) {
        for (auto a : {f->inputArg(i), f->outputArg(i)}) {
            if (!a->type().cast<Ctl::FloatType>() || !a->isVarying()) {
                return warn("bad argument type in ART_fill_pipette");
            }
        }
    }
    for (size_t k = 0; k < guides.size(); ++k) {
        auto g = f->inputArg(3 + k);
        if (!g->type().cast<Ctl::FloatType>() || !g->isVarying()) {
            return warn("ART_fill_pipette must take the same varying float "
                        "guides as ART_main, after its first 3 inputs");
        }
    }
    auto id = f->inputArg(id_pos);
    if (id->isVarying() || id->type()->cDataType() != Ctl::IntTypeEnum ||
        id->type().cast<Ctl::BoolType>()) {
        return warn("curve_id of ART_fill_pipette must be a uniform int");
    }
    for (size_t i = id_pos + 1, n = f->numInputArgs(); i < n; ++i) {
        if (f->inputArg(i)->isVarying()) {
            return warn("parameter " + f->inputArg(i)->name() +
                        " of ART_fill_pipette is varying");
        }
    }
    return true;
}

} // namespace

namespace {

inline float CTL_shaper_func(float a, bool inv)
{
    constexpr float m1 = 2610.0 / 16384.0;
    constexpr float m2 = 2523.0 / 32.0;
    constexpr float c1 = 107.0 / 128.0;
    constexpr float c2 = 2413.0 / 128.0;
    constexpr float c3 = 2392.0 / 128.0;
    constexpr float scale = 100.0;

    if (a <= 0.f) {
        return 0.f;
    }

    if (!inv) {
        a /= scale;
        float aa = pow_F(a, m1);
        return pow_F((c1 + c2 * aa) / (1.f + c3 * aa), m2);
    } else {
        float p = pow_F(a, 1.f / m2);
        float aa = std::max(p - c1, 0.f) / (c2 - c3 * p);
        return pow_F(aa, 1.f / m1) * scale;
    }
}

struct ShaperLUTs {
    LUTf fwd;
    LUTf inv;

    ShaperLUTs()
    {
        fwd(65536);
        inv(65536);
        for (int i = 0; i < 65536; ++i) {
            float x = float(i) / 65535.f;
            fwd[i] = CTL_shaper_func(x, false);
            inv[i] = CTL_shaper_func(x, true);
        }
    }
};

} // namespace

//-----------------------------------------------------------------------------
// CTLStore
//-----------------------------------------------------------------------------

CTLStore &CTLStore::getInstance()
{
    static CTLStore instance;
    return instance;
}

CTLStore::CTLStore()
    : cache_(art::gui::options.clutCacheSize * 4),
      lut_cache_(art::gui::options.clutCacheSize * 4)
{
}

void CTLStore::clearCache()
{
    art::utils::MyMutex::MyLock lock(mutex_);

    cache_.clear();

    art::utils::MyMutex::MyLock lut_lock(lut_mutex_);
    lut_cache_.clear();
}

std::shared_ptr<const LUT3D> CTLStore::findLut3d(const std::string &key,
                                                 int min_dim) const
{
    std::shared_ptr<const LUT3D> lut;
    if (lut_cache_.get(key, lut) && lut && lut->dimension() >= min_dim) {
        if (settings->verbose > 1) {
            std::cout << "CTL lut cache hit: dimension " << lut->dimension()
                      << " for " << min_dim << " requested" << std::endl;
        }
        return lut;
    }
    if (settings->verbose > 1) {
        std::cout << "CTL lut cache miss: dimension " << min_dim << std::endl;
    }
    return nullptr;
}

void CTLStore::storeLut3d(const std::string &key,
                          std::shared_ptr<const LUT3D> lut)
{
    if (!lut) {
        return;
    }
    art::utils::MyMutex::MyLock lock(lut_mutex_);
    std::shared_ptr<const LUT3D> old;
    if (lut_cache_.get(key, old) && old &&
        old->dimension() >= lut->dimension()) {
        return;
    }
    lut_cache_.set(key, lut);
    if (settings->verbose > 1) {
        std::cout << "CTL lut cache store: dimension " << lut->dimension()
                  << std::endl;
    }
}

float CTLStore::shaper(float a, bool inv)
{
    static ShaperLUTs luts;
    if (a >= 0.f && a <= 1.f) {
        return inv ? luts.inv[a * 65535.f] : luts.fwd[a * 65535.f];
    }
    return CTL_shaper_func(a, inv);
}

std::pair<std::shared_ptr<Ctl::Interpreter>, std::vector<Ctl::FunctionCallPtr>>
CTLStore::getLut(const Glib::ustring &filename, int num_threads,
                 int &chunk_size, CLUTParamDescriptorList &params,
                 Glib::ustring &colorspace, int &lut_dim,
                 std::string *md5_out,
                 std::vector<CTLGuide> *guides_out) const
{
    art::utils::MyMutex::MyLock lock(mutex_);

    lut_dim = 0;
    if (guides_out) {
        guides_out->clear();
    }

    CacheEntry result;
    std::vector<Ctl::FunctionCallPtr> retval;
    std::shared_ptr<Ctl::Interpreter> intp;

    const Glib::ustring full_filename =
        !Glib::path_is_absolute(filename)
            ? Glib::ustring(Glib::build_filename(art::gui::options.clutsDir, filename))
            : filename;
    if (!Glib::file_test(full_filename, Glib::FILE_TEST_IS_REGULAR) ||
        getFileExtension(full_filename) != "ctl") {
        return std::make_pair(intp, retval);
    }
    const auto md5 = getMD5(full_filename, true);
    if (md5_out) {
        *md5_out = md5;
    }

    const auto err = [&](const char *msg) {
        if (settings->verbose) {
            std::cout << "Error in CTL script from " << full_filename << ": "
                      << msg << std::endl;
        }
        retval.clear();
        intp.reset();
        return std::make_pair(intp, retval);
    };

    try {
        std::string key = Glib::filename_from_utf8(full_filename);
        bool found = cache_.get(key, result);
        if (!found || result.md5 != md5) {
            if (settings->verbose > 1) {
                std::cout << "CTL cache miss: " << full_filename << std::endl;
            }
            StopWatch load_CTL("CTL script load", true);

            intp = std::make_shared<Ctl::SimdInterpreter>();
            intp->setMaxInstCount(10 * 10000000);
            std::vector<std::string> module_paths{
                Glib::path_get_dirname(full_filename),
                Glib::build_filename(art::gui::options.user_config_dir, "ctlscripts"),
                Glib::build_filename(art::gui::options.ART_base_dir, "ctlscripts")};
            auto pth = intp->modulePaths();
            module_paths.insert(module_paths.end(), pth.begin(), pth.end());
            intp->setModulePaths(module_paths);
            intp->loadFile(
                full_filename,
                art::utils::removeExtension(Glib::path_get_basename(full_filename)));

            auto f = intp->newFunctionCall("ART_main");
            if (f->numInputArgs() < 3) {
                return err("wrong number of input arguments to ART_main");
            } else {
                for (int i = 0; i < 3; ++i) {
                    auto a = f->inputArg(i);
                    if (!a->type().cast<Ctl::FloatType>() || !a->isVarying()) {
                        return err("bad input arg type");
                    }
                }
            }
            if (f->numOutputArgs() != 3) {
                return err("wrong number of output arguments");
            } else {
                for (int i = 0; i < 3; ++i) {
                    auto a = f->outputArg(i);
                    if (!a->type().cast<Ctl::FloatType>() || !a->isVarying()) {
                        return err("bad output arg type");
                    }
                }
            }

            // the varying inputs after r, g, b are the guides
            int num_guides = 0;
            while (3 + num_guides < int(f->numInputArgs()) &&
                   f->inputArg(3 + num_guides)->isVarying()) {
                if (!f->inputArg(3 + num_guides)
                         ->type()
                         .cast<Ctl::FloatType>()) {
                    return err("the guides (inputs after the first 3) must be "
                               "floats");
                }
                ++num_guides;
            }
            if (num_guides > CTL_MAX_GUIDES) {
                return err("too many guides");
            }

            std::vector<CTLGuide> guides;
            bool guides_tag = false;
            if (!get_CTL_guides(full_filename, guides, guides_tag)) {
                return err("error in parsing @ART-guides");
            }
            if (guides_tag) {
                if (int(guides.size()) != num_guides) {
                    return err(
                        ("@ART-guides lists " + std::to_string(guides.size()) +
                         " guides, but ART_main takes " +
                         std::to_string(num_guides))
                            .c_str());
                }
            } else if (num_guides == 1) {
                guides.push_back(CTLGuide::Y);
            } else if (num_guides > 1) {
                return err("ART_main takes more than one guide: use "
                           "@ART-guides to say which");
            }

            if (!get_CTL_params(full_filename, intp, f, params, colorspace,
                                lut_dim, guides)) {
                params.clear();
                return err("error in parsing CTL parameters");
            }

            if (!check_CTL_pipette(full_filename, intp, params, guides)) {
                for (auto &p : params) {
                    p.pipette_id = -1;
                }
            }

            result.intp = intp;
            result.md5 = md5;
            result.params = params;
            result.colorspace = colorspace;
            result.lut_dim = lut_dim;
            result.guides = guides;
            cache_.set(key, result);
        } else {
            intp = result.intp;
            params = result.params;
            colorspace = result.colorspace;
            lut_dim = result.lut_dim;
        }
        if (guides_out) {
            *guides_out = result.guides;
        }
        if (intp) {
            for (int i = 0; i < num_threads; ++i) {
                retval.push_back(intp->newFunctionCall("ART_main"));
            }
            chunk_size = intp->maxSamples();
        }
    } catch (std::exception &exc) {
        return err(exc.what());
    }

    return std::make_pair(intp, retval);
}

bool CTLStore::getDisplayName(const Glib::ustring &filename,
                              Glib::ustring &name, int &order)
{
    if (Glib::file_test(filename, Glib::FILE_TEST_EXISTS)) {
        auto fn = Glib::filename_from_utf8(filename);
        std::ifstream src(fn.c_str());
        std::string line;
        bool found = false;
        order = -1;
        while (src && std::getline(src, line)) {
            size_t s = 0;
            while (s < line.size() && std::isspace(line[s])) {
                ++s;
            }
            if (s + 1 < line.size() && line[s] == '/' &&
                line[s + 1] == '/') {
                s += 2;
            } else {
                continue;
            }
            while (s < line.size() && std::isspace(line[s])) {
                ++s;
            }
            if (line.find("@ART-label:", s) == s) {
                line = line.substr(11 + s);
                cJSON *root = cJSON_Parse(line.c_str());
                if (root) {
                    if (cJSON_IsString(root)) {
                        name = cJSON_GetStringValue(root);
                        if (!name.empty() && name[0] == '$') {
                            auto pos = name.find(';');
                            if (pos != Glib::ustring::npos) {
                                auto key = name.substr(1, pos - 1);
                                auto dflt = name.substr(pos + 1);
                                auto res = art::utils::M(key);
                                name = (res == key) ? dflt : res;
                            } else {
                                name = art::utils::M(name.c_str() + 1);
                            }
                        }
                        found = !name.empty();
                    }
                    cJSON_Delete(root);
                }
                if (order >= 0) {
                    break;
                }
            } else if (line.find("@ART-order:", s) == s) {
                line = line.substr(11 + s);
                cJSON *root = cJSON_Parse(line.c_str());
                if (root) {
                    if (cJSON_IsNumber(root)) {
                        order = root->valueint;
                    }
                    cJSON_Delete(root);
                }
                if (found) {
                    break;
                }
            }
        }
        if (found) {
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
// CTLApplication
//-----------------------------------------------------------------------------

CTLApplication::CTLApplication()
    : chunk_size_(0), lut_dim_(0)
{
}

bool CTLApplication::init(const Glib::ustring &filename, int num_threads,
                          Glib::ustring &colorspace)
{
    filename_ = filename;
    lut_dim_ = 0;
    guides_.clear();
    auto res = CTLStore::getInstance().getLut(filename_, num_threads,
                                              chunk_size_, params_, colorspace,
                                              lut_dim_, &md5_, &guides_);
    auto intp = res.first;
    auto &func = res.second;
    if (func.empty()) {
        return false;
    }
    intp_ = intp;
    func_ = std::move(func);
    pipette_func_.clear();
    bool pipette = false;
    for (auto &p : params_) {
        pipette = pipette || p.pipette_id >= 0;
    }
    if (pipette) {
        // the script was validated by getLut
        for (size_t i = 0; i < func_.size(); ++i) {
            pipette_func_.push_back(intp->newFunctionCall("ART_fill_pipette"));
        }
    }
    return true;
}

bool CTLApplication::set_params(const CLUTParamValueMap &values, int max_dim)
{
    try {
        for (size_t i = 0; i < params_.size(); ++i) {
            auto &desc = params_[i];
            auto it = values.find(desc.name);
            if (it == values.end()) {
                std::cout << "WARNING: no value for " << desc.name << std::endl;
            }
            auto vv = it != values.end() ? it->second : desc.value_default;
            auto v = vv[0];
            const auto find_arg = [&](const std::vector<Ctl::FunctionCallPtr> &funcs) {
                for (size_t j = 0, n = funcs[0]->numInputArgs(); j < n; ++j) {
                    if (funcs[0]->inputArg(j)->name() == desc.name) {
                        return int(j);
                    }
                }
                return -1;
            };
            const int arg = find_arg(func_);
            if (arg < 0) {
                if (settings->verbose) {
                    std::cout << "Error: no parameter " << desc.name
                              << " in LUT " << filename_ << std::endl;
                }
                return false;
            }
            std::unique_ptr<Curve> curve;
            if (desc.type == CLUTParamType::PT_CURVE) {
                curve.reset(new DiagonalCurve(vv));
            } else if (desc.type == CLUTParamType::PT_FLATCURVE ||
                       desc.type == CLUTParamType::PT_FLATCURVE_PERIODIC) {
                curve.reset(new FlatCurve(
                    vv, desc.type == CLUTParamType::PT_FLATCURVE_PERIODIC));
            }
            const auto set_arg = [&](std::vector<Ctl::FunctionCallPtr> &funcs,
                                     int arg) {
                for (auto f : funcs) {
                    auto d = f->inputArg(arg)->data();
                    switch (desc.type) {
                    case CLUTParamType::PT_BOOL:
                        *reinterpret_cast<bool *>(d) = v;
                        break;
                    case CLUTParamType::PT_FLOAT:
                        *reinterpret_cast<float *>(d) = v;
                        break;
                    case CLUTParamType::PT_CURVE:
                    case CLUTParamType::PT_FLATCURVE:
                    case CLUTParamType::PT_FLATCURVE_PERIODIC: {
                        Ctl::ArrayTypePtr atp =
                            f->inputArg(arg)->type().cast<Ctl::ArrayType>();
                        size_t n = atp->size();
                        for (size_t j = 0; j < n; ++j) {
                            double x = double(j) / double(n - 1);
                            *(reinterpret_cast<float *>(d) + j) =
                                curve->getVal(x);
                        }
                    } break;
                    case CLUTParamType::PT_INT:
                    case CLUTParamType::PT_CHOICE:
                    default:
                        *reinterpret_cast<int *>(d) = v;
                        break;
                    }
                }
            };
            set_arg(func_, arg);
            if (!pipette_func_.empty()) {
                // parameters unknown to ART_fill_pipette are simply not set
                const int parg = find_arg(pipette_func_);
                if (parg >= 0) {
                    auto ta = func_[0]->inputArg(arg)->type();
                    auto tb = pipette_func_[0]->inputArg(parg)->type();
                    bool same = ta->cDataType() == tb->cDataType();
                    if (same && ta->cDataType() == Ctl::ArrayTypeEnum) {
                        same = ta.cast<Ctl::ArrayType>()->size() ==
                               tb.cast<Ctl::ArrayType>()->size();
                    }
                    if (same) {
                        set_arg(pipette_func_, parg);
                    } else if (settings->verbose) {
                        std::cout << "Warning: parameter " << desc.name
                                  << " has different types in ART_main and "
                                     "ART_fill_pipette in LUT "
                                  << filename_ << std::endl;
                    }
                }
            }
        }
        if (settings->verbose) {
            std::set<std::string> valid;
            for (auto &p : params_) {
                valid.insert(p.name);
            }
            for (auto &p : values) {
                if (valid.find(p.first) == valid.end()) {
                    std::cout << "Warning: invalid parameter " << p.first
                              << " for LUT " << filename_ << std::endl;
                }
            }
        }
    } catch (std::exception &exc) {
        if (settings->verbose) {
            std::cout << "Error in setting parameters for LUT "
                      << filename_ << ": " << exc.what() << std::endl;
        }
        return false;
    } catch (...) {
        if (settings->verbose) {
            std::cout << "UNKNOWN Error in setting parameters for LUT "
                      << filename_ << std::endl;
        }
        return false;
    }

    // a script taking a guide depends on more than the colour of the pixel,
    // so it can't be baked into a 3D LUT (lut_dim_ is -1 in this case)
    int dim = uses_guide() ? -1 : lut_dim_;
    if (dim >= 0 && settings->ctl_scripts_fast_preview && max_dim != 0) {
        dim = !dim ? max_dim : std::min(dim, max_dim);
    }
    if (dim > 0) {
        // a cached LUT bigger than requested is fine, see CTLStore::findLut3d
        auto &store = CTLStore::getInstance();
        const auto key = lut_key(values);
        lut_ = store.findLut3d(key, dim);
        if (!lut_) {
            lut_ = compute_lut(dim);
            store.storeLut3d(key, lut_);
        }
    } else {
        lut_.reset();
    }

    return true;
}

namespace {

class CTLLutInitializer: public LUT3D::initializer {
public:
    CTLLutInitializer(const std::vector<float> *rgb): rgb_(rgb), i_(0) {}

    void operator()(float &r, float &g, float &b) override
    {
        r = rgb_[0][i_];
        g = rgb_[1][i_];
        b = rgb_[2][i_];
        ++i_;
    }

private:
    const std::vector<float> *rgb_;
    int i_;
};

} // namespace

std::shared_ptr<const LUT3D> CTLApplication::compute_lut(int dim)
{
    BENCHFUN
    if (settings->verbose > 1) {
        std::cout << "computing CTL lut for: " << filename_
                  << " of dimension: " << dim << std::endl;
    }

    std::vector<float> rgb[3];

    int sz = SQR(dim) * dim;
    for (int i = 0; i < 3; ++i) {
        rgb[i].reserve(sz);
    }

    for (int i = 0; i < dim; ++i) {
        float r = float(i) / (dim - 1);
        for (int j = 0; j < dim; ++j) {
            float g = float(j) / (dim - 1);
            for (int k = 0; k < dim; ++k) {
                float b = float(k) / (dim - 1);
                rgb[0].push_back(CTLStore::shaper(r, true));
                rgb[1].push_back(CTLStore::shaper(g, true));
                rgb[2].push_back(CTLStore::shaper(b, true));
            }
        }
    }

    auto func = func_[0];

    for (int x = 0; x < sz; x += chunk_size_) {
        const auto n = (x + chunk_size_ < sz ? chunk_size_ : sz - x);
        for (int i = 0; i < 3; ++i) {
            memcpy(func->inputArg(i)->data(), &(rgb[i][x]), sizeof(float) * n);
        }
        func->callFunction(n);
        for (int i = 0; i < 3; ++i) {
            memcpy(&(rgb[i][x]), func->outputArg(i)->data(), sizeof(float) * n);
        }
    }

    CTLLutInitializer f(rgb);
    auto lut = std::make_shared<LUT3D>();
    lut->init(dim, f);
    return lut;
}

std::string CTLApplication::lut_key(const CLUTParamValueMap &values) const
{
    // everything the output of the script depends on, but the dimension of
    // the LUT: the script, and the value of each of its parameters (all the
    // elements, as curves are arrays), falling back to the defaults like
    // set_params does
    std::ostringstream buf;
    buf.imbue(std::locale::classic());
    buf.precision(17);
    buf << filename_ << '\n' << md5_;
    for (const auto &p : params_) {
        auto it = values.find(p.name);
        const auto &vv = (it != values.end()) ? it->second : p.value_default;
        buf << '\n' << p.name << ':';
        for (auto v : vv) {
            buf << ' ' << v;
        }
    }
    return Glib::Checksum::compute_checksum(Glib::Checksum::CHECKSUM_SHA256,
                                            buf.str());
}

namespace {

bool check_guide_rows(const std::vector<CTLGuide> &guides,
                      const float *const *guide_rows,
                      const Glib::ustring &filename)
{
    for (auto g : guides) {
        if (!guide_rows || !guide_rows[int(g)]) {
            if (settings->verbose) {
                std::cout << "Error: guide " << ctl_guide_name(g)
                          << " not given to CTL script " << filename
                          << std::endl;
            }
            return false;
        }
    }
    return true;
}

} // namespace

bool CTLApplication::apply(int thread_id, int W, float *r, float *g, float *b,
                           const float *const *guide_rows)
{
    if (!check_guide_rows(guides_, guide_rows, filename_)) {
        return false;
    }
    auto func = func_[thread_id];
    float *ch[3] = {r, g, b};

    if (lut_) {
        for (int x = 0; x < W; ++x) {
            float rr = CTLStore::shaper(r[x], false);
            float gg = CTLStore::shaper(g[x], false);
            float bb = CTLStore::shaper(b[x], false);
            (*lut_)(rr, gg, bb);
            r[x] = rr;
            g[x] = gg;
            b[x] = bb;
        }
    } else {
        for (int x = 0; x < W; x += chunk_size_) {
            const auto n = (x + chunk_size_ < W ? chunk_size_ : W - x);
            for (int i = 0; i < 3; ++i) {
                memcpy(func->inputArg(i)->data(), ch[i] + x, sizeof(float) * n);
            }
            for (size_t k = 0; k < guides_.size(); ++k) {
                memcpy(func->inputArg(3 + k)->data(),
                       guide_rows[int(guides_[k])] + x, sizeof(float) * n);
            }
            func->callFunction(n);
            for (int i = 0; i < 3; ++i) {
                memcpy(ch[i] + x, func->outputArg(i)->data(),
                       sizeof(float) * n);
            }
        }
    }
    return true;
}

bool CTLApplication::fill_pipette(int thread_id, int curve_id, int W,
                                  const float *const in[3],
                                  float *const out[3],
                                  const float *const *guide_rows)
{
    if (!can_pipette(thread_id) ||
        !check_guide_rows(guides_, guide_rows, filename_)) {
        return false;
    }
    try {
        auto func = pipette_func_[thread_id];
        // same input as ART_main, but never through the baked 3D LUT: the
        // output is not a colour
        *reinterpret_cast<int *>(
            func->inputArg(3 + guides_.size())->data()) = curve_id;
        for (int x = 0; x < W; x += chunk_size_) {
            const auto n = (x + chunk_size_ < W ? chunk_size_ : W - x);
            for (int i = 0; i < 3; ++i) {
                memcpy(func->inputArg(i)->data(), in[i] + x,
                       sizeof(float) * n);
            }
            for (size_t k = 0; k < guides_.size(); ++k) {
                memcpy(func->inputArg(3 + k)->data(),
                       guide_rows[int(guides_[k])] + x, sizeof(float) * n);
            }
            func->callFunction(n);
            for (int i = 0; i < 3; ++i) {
                memcpy(out[i] + x, func->outputArg(i)->data(),
                       sizeof(float) * n);
            }
        }
        return true;
    } catch (std::exception &exc) {
        if (settings->verbose) {
            std::cout << "Error in computing the pipette of CTL LUT "
                      << filename_ << ": " << exc.what() << std::endl;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
// guides
//-----------------------------------------------------------------------------

namespace {

// parameters of the smoothing of the "C" and "H" guides
constexpr int CH_RADIUS = 8; // in pixels at scale 1
constexpr float CH_EPSILON = 1e-3f;
// cz is multiplied by this before filtering, so that it is roughly in [0, 1]
// (like in the tone curve), and the same epsilon fits
constexpr float CH_SCALE = 50.f;
// the "C" guide is cz multiplied by this, so that strongly saturated colours
// are near 1 (neutral greys are 0, skin tones are around 0.1-0.2), which makes
// it easier to use in scripts
constexpr float CH_OUT_SCALE = 15.f;

void compute_Y_guide(Imagefloat *img, TMatrix ws, double scale, bool multithread,
                     array2D<float> &guide, bool spatial)
{
    // the luminance is the Y channel of the YUV mode (in 0..65535)
    const int W = img->getWidth();
    const int H = img->getHeight();
    guide(W, H, ARRAY2D_ALIGNED);

    if (img->mode() == Imagefloat::Mode::YUV) {
#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                guide[y][x] = LIM(img->g(y, x) / 65535.f, 1e-5f, 32.f);
            }
        }
    } else {
#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                guide[y][x] = LIM(Color::rgbLuminance(img->r(y, x), img->g(y, x),
                                                      img->b(y, x), ws) /
                                      65535.f,
                                  1e-5f, 32.f);
            }
        }
    }

    // the same guide as the tone equalizer at regularization 4. This is the
    // only place to change if a different algorithm is wanted
    if (spatial) {
        constexpr int regularization = 4;
        toneEqGuide(guide, regularization, scale, multithread);
    }
}

/**
 * The "C" and "H" guides: the chroma and the hue (cz and hz of
 * Color::rgb2jzczhz) of the image, smoothed with an edge-aware filter.
 * The hue is an angle, so it is not smoothed directly: the cartesian
 * components (az, bz) are, which gives a chroma-weighted average of the hues
 * without problems at the wrap point. All the channels are filtered with a
 * guided filter that uses the chroma itself as its guide, so that they share
 * the same edges.
 */
void compute_CH_guides(Imagefloat *img, TMatrix ws, double scale,
                       bool multithread, bool want_c, bool want_h,
                       array2D<float> &C, array2D<float> &Hz, bool spatial)
{
    const int W = img->getWidth();
    const int H = img->getHeight();
    const bool yuv = img->mode() == Imagefloat::Mode::YUV;

    array2D<float> az(W, H, ARRAY2D_ALIGNED);
    array2D<float> bz(W, H, ARRAY2D_ALIGNED);
    array2D<float> cz(W, H, ARRAY2D_ALIGNED);

#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            float R, G, B;
            if (yuv) {
                Color::yuv2rgb(img->g(y, x), img->b(y, x), img->r(y, x), R, G, B,
                               ws);
            } else {
                R = img->r(y, x);
                G = img->g(y, x);
                B = img->b(y, x);
            }
            float Jz, a, b;
            Color::rgb2jzazbz(std::max(R, 0.f) / 65535.f,
                              std::max(G, 0.f) / 65535.f,
                              std::max(B, 0.f) / 65535.f, Jz, a, b, ws);
            if (!(Jz > 1e-7f)) {
                // the hue is undefined (also catches NaNs)
                a = b = 0.f;
            }
            a *= CH_SCALE;
            b *= CH_SCALE;
            az[y][x] = a;
            bz[y][x] = b;
            cz[y][x] = std::sqrt(a * a + b * b);
        }
    }

    const int radius = std::max(1, int(CH_RADIUS / scale + 0.5));
    array2D<float> tmp(W, H, ARRAY2D_ALIGNED);

    if (want_h) {
        array2D<float> bs(W, H, ARRAY2D_ALIGNED);
        if (spatial) {
            guidedFilter(cz, az, tmp, radius, CH_EPSILON, multithread);
            guidedFilter(cz, bz, bs, radius, CH_EPSILON, multithread);
        } else {
            for (int y = 0; y < H; ++y) {
                for (int x = 0; x < W; ++x) {
                    tmp[y][x] = az[y][x];
                    bs[y][x] = bz[y][x];
                }
            }
        }
        Hz(W, H, ARRAY2D_ALIGNED);
#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                float c, h;
                Color::jzazbz2jzch(tmp[y][x], bs[y][x], c, h);
                Hz[y][x] = h;
            }
        }
    }
    if (want_c) {
        if (spatial) {
            guidedFilter(cz, cz, tmp, radius, CH_EPSILON, multithread);
        } else {
            for (int y = 0; y < H; ++y) {
                for (int x = 0; x < W; ++x) {
                    tmp[y][x] = cz[y][x];
                }
            }
        }
        C(W, H, ARRAY2D_ALIGNED);
#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                C[y][x] = std::max(tmp[y][x], 0.f) * (CH_OUT_SCALE / CH_SCALE);
            }
        }
    }
}

} // namespace

void compute_ctl_guides(Imagefloat *img, unsigned kinds_mask,
                        const Glib::ustring &working_profile, double scale,
                        bool multithread, CTLGuideSet &out, bool spatial)
{
    TMatrix ws = ICCStore::getInstance()->workingSpaceMatrix(working_profile);
    for (int i = 0; i < CTL_MAX_GUIDES; ++i) {
        out.present[i] = false;
    }

    if (kinds_mask & ctl_guide_bit(CTLGuide::Y)) {
        compute_Y_guide(img, ws, scale, multithread, out.plane[int(CTLGuide::Y)],
                         spatial);
        out.present[int(CTLGuide::Y)] = true;
    }
    const bool want_c = kinds_mask & ctl_guide_bit(CTLGuide::C);
    const bool want_h = kinds_mask & ctl_guide_bit(CTLGuide::H);
    if (want_c || want_h) {
        compute_CH_guides(img, ws, scale, multithread, want_c, want_h,
                          out.plane[int(CTLGuide::C)],
                          out.plane[int(CTLGuide::H)], spatial);
        out.present[int(CTLGuide::C)] = want_c;
        out.present[int(CTLGuide::H)] = want_h;
    }
}

}} // namespace art::engine

#endif // ART_USE_CTL
