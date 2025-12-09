/* -*- C++ -*-
 *
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
#pragma once

#include "../rtengine/rtengine.h"
#include <array>
#include <glibmm.h>
#include <map>
#include <vector>

struct WBPreset {
    Glib::ustring label;
    std::array<double, 3> mult;

    WBPreset(const Glib::ustring &l = "", const std::array<double, 3> &m = {})
        : label(l), mult(m)
    {
    }
};

class WBProvider {
public:
    virtual ~WBProvider() {}
    virtual void getAutoWB(rtengine::ColorTemp &out, double equal) {}
    virtual void getCamWB(rtengine::ColorTemp &out) {}
    virtual void spotWBRequested(int size) {}

    virtual std::vector<WBPreset> getWBPresets() const
    {
        return std::vector<WBPreset>();
    }
    virtual void convertWBCam2Mul(double &rm, double &gm, double &bm) {}
    virtual void convertWBMul2Cam(double &rm, double &gm, double &bm) {}
};

namespace wb_presets {

void init(const Glib::ustring &baseDir, const Glib::ustring &userSettingsDir);

const std::map<std::string, std::vector<WBPreset>> &getPresets();

} // namespace wb_presets
