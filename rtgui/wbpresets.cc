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

#include "../rtengine/rawimagesource.h"
#include "toolpanelcoord.h"

std::vector<WBPreset> ToolPanelCoordinator::getWBPresets() const
{
    std::vector<WBPreset> ret;
    if (ipc) {
        const rtengine::FramesData *md =
            dynamic_cast<const rtengine::FramesData *>(
                ipc->getInitialImage()->getMetaData());
        rtengine::RawImageSource *src =
            dynamic_cast<rtengine::RawImageSource *>(ipc->getInitialImage());
        if (md && src) {
            std::string key = md->getInternalMakeModel();

            const auto &presets = wb_presets::getPresets();
            auto it = presets.find(key);
            if (it != presets.end()) {
                ret = it->second;
            }
        }
    }
    return ret;
}

void ToolPanelCoordinator::convertWBCam2Mul(double &rm, double &gm, double &bm)
{
    if (ipc) {
        auto src =
            dynamic_cast<rtengine::ImageSource *>(ipc->getInitialImage());
        if (src) {
            src->wbCamera2Mul(rm, gm, bm);
        }
    }
}

void ToolPanelCoordinator::convertWBMul2Cam(double &rm, double &gm, double &bm)
{
    if (ipc) {
        auto src =
            dynamic_cast<rtengine::ImageSource *>(ipc->getInitialImage());
        if (src) {
            src->wbMul2Camera(rm, gm, bm);
        }
    }
}
