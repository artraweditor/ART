/** -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright (c) 2020 Alberto Griggio <alberto.griggio@gmail.com>
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

#include <glib/gstdio.h>
#include <glibmm.h>
#include <iostream>

#include "../rtengine/cJSON.h"
#include "../rtengine/settings.h"
#include "wbprovider.h"

namespace rtengine {
extern const Settings *settings;
}

namespace wb_presets {

namespace {

std::map<std::string, std::vector<WBPreset>> load(const Glib::ustring &path)
{
    std::map<std::string, std::vector<WBPreset>> ret;

    Glib::ustring fileName = Glib::build_filename(path, "wbpresets.json");
    if (!Glib::file_test(fileName, Glib::FILE_TEST_EXISTS)) {
        return ret;
    }

    FILE *const f = g_fopen(fileName.c_str(), "r");

    if (rtengine::settings->verbose > 1) {
        std::cout << "trying to load white balance presets from " << fileName
                  << std::flush;
    }

    if (!f) {
        if (rtengine::settings->verbose > 1) {
            std::cout << " FAIL" << std::endl;
        }

        return ret;
    }

    fseek(f, 0, SEEK_END);
    long length = ftell(f);

    if (length <= 0) {
        if (rtengine::settings->verbose > 1) {
            std::cout << " FAIL" << std::endl;
        }

        fclose(f);
        return ret;
    }

    cJSON *root = nullptr;
    {
        std::vector<char> bufvec(length + 1);
        char *buf = &bufvec[0];
        fseek(f, 0, SEEK_SET);
        length = fread(buf, 1, length, f);
        buf[length] = 0;

        fclose(f);
        std::string curloc = setlocale(LC_NUMERIC, NULL);
        setlocale(LC_NUMERIC, "C");
        cJSON_Minify(buf);
        root = cJSON_Parse(buf);
        setlocale(LC_NUMERIC, curloc.c_str());
    }

    if (!root) {
        if (rtengine::settings->verbose > 1) {
            std::cout << " FAIL" << std::endl;
        }

        return ret;
    }

    if (root->type != cJSON_Array) {
        goto parse_error;
    }

    for (int i = 0, n = cJSON_GetArraySize(root); i < n; ++i) {
        cJSON *js = cJSON_GetArrayItem(root, i);
        if (!js) {
            goto parse_error;
        }
        cJSON *ji = cJSON_GetObjectItem(js, "make_model");
        std::string key;

        if (!ji || ji->type != cJSON_String) {
            goto parse_error;
        }

        key = ji->valuestring;

        ji = cJSON_GetObjectItem(js, "presets");

        if (!ji || ji->type != cJSON_Array) {
            goto parse_error;
        }

        for (int j = 0, n = cJSON_GetArraySize(ji); j < n; ++j) {
            cJSON *p = cJSON_GetArrayItem(ji, j);
            if (!p || p->type != cJSON_Object) {
                goto parse_error;
            }

            cJSON *e = cJSON_GetObjectItem(p, "name");
            if (!e || e->type != cJSON_String) {
                goto parse_error;
            }
            std::string name = e->valuestring;

            e = cJSON_GetObjectItem(p, "multipliers");
            if (!e || e->type != cJSON_Array || cJSON_GetArraySize(e) != 3) {
                goto parse_error;
            }

            WBPreset preset(name);
            for (int k = 0; k < 3; ++k) {
                cJSON *v = cJSON_GetArrayItem(e, k);
                if (!v || v->type != cJSON_Number) {
                    goto parse_error;
                }
                preset.mult[k] = v->valuedouble;
            }

            ret[key].emplace_back(std::move(preset));
        }
    }

    cJSON_Delete(root);

    if (rtengine::settings->verbose > 1) {
        std::cout << " OK" << std::endl;
    }

    return ret;

parse_error:

    if (rtengine::settings->verbose) {
        std::cout << " ERROR in parsing " << fileName << std::endl;
    }

    cJSON_Delete(root);
    return ret;
}

std::map<std::string, std::vector<WBPreset>> presets;

} // namespace

void init(const Glib::ustring &baseDir, const Glib::ustring &userSettingsDir)
{
    std::map<Glib::ustring, WBPreset> seen;

    presets = load(baseDir);
    auto user_presets = load(userSettingsDir);
    for (auto &p : user_presets) {
        auto it = presets.find(p.first);
        if (it == presets.end()) {
            presets[p.first] = p.second;
        } else {
            // merge by updating
            seen.clear();
            for (auto &pp : p.second) {
                seen[pp.label] = pp;
            }
            std::vector<WBPreset> tmp;
            for (auto &pp : it->second) {
                auto j = seen.find(pp.label);
                if (j == seen.end()) {
                    tmp.push_back(pp);
                } else {
                    tmp.push_back(j->second);
                }
                seen.erase(j);
            }
            for (auto &pp : p.second) {
                if (seen.find(pp.label) == seen.end()) {
                    tmp.push_back(pp);
                }
            }
            tmp.swap(it->second);
        }
    }
}

const std::map<std::string, std::vector<WBPreset>> &getPresets()
{
    return presets;
}

} // namespace wb_presets
