/* -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright (c) 2023-2026 Alberto Griggio <alberto.griggio@gmail.com>
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

#include "cJSON.h"
#include <array>
#include <glibmm/ustring.h>
#include <map>
#include <string>
#include <vector>

namespace art { namespace engine {

enum class CLUTParamType {
    PT_INT,
    PT_FLOAT,
    PT_BOOL,
    PT_CHOICE,
    PT_CURVE,
    PT_FLATCURVE,
    PT_FLATCURVE_PERIODIC,
    // float[2]: hue (radians, in [-PI, PI]) and saturation (in [0, 1])
    PT_HUESAT
};

struct CLUTParamDescriptor {
    std::string name;
    CLUTParamType type;
    double value_min;
    double value_max;
    std::vector<double> value_default;
    std::vector<std::pair<Glib::ustring, int>> choices;
    Glib::ustring gui_name;
    Glib::ustring gui_group;
    double gui_step;
    Glib::ustring gui_tooltip;
    std::vector<std::array<float, 4>> gui_bottom_gradient;
    std::vector<std::array<float, 4>> gui_left_gradient;
    // for curves: the curve_id to pass to ART_fill_pipette (from the
    // @ART-pipette directive of the script), or -1 if the curve has no pipette
    int pipette_id = -1;

    bool fill_from_json(cJSON *root);
};

typedef std::map<std::string, std::vector<double>> CLUTParamValueMap;

class CLUTParamDescriptorList: public std::vector<CLUTParamDescriptor> {
public:
    const std::vector<std::pair<std::string, Glib::ustring>> &
    get_presets() const;
    bool apply_preset(const std::string &key, CLUTParamValueMap &out);
    bool add_preset(const std::string &key, const Glib::ustring &gui_name,
                    const CLUTParamValueMap &value);
    bool add_preset_from_json(cJSON *root);

private:
    std::vector<std::pair<std::string, Glib::ustring>> presets_;
    std::map<std::string, CLUTParamValueMap> presets_map_;
};

}} // namespace art::engine
