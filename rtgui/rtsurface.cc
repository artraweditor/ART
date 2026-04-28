/*
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2018 Jean-Christophe FRISCH <natureh.510@gmail.com>
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

#include <iostream>

#include "options.h"
#include "rtsurface.h"

namespace {

std::map<std::string, Cairo::RefPtr<Cairo::ImageSurface>> surfaceCache;

}


RTSurface::RTSurface(): RTScalable()
{
    Cairo::RefPtr<Cairo::ImageSurface> imgSurf(
        new Cairo::ImageSurface(nullptr, false));
    surface = imgSurf;
}

RTSurface::RTSurface(const RTSurface &other): RTScalable()
{
    surface = other.surface;
}

RTSurface::RTSurface(Glib::ustring fileName, Glib::ustring rtlFileName)
    : RTScalable()
{
    Cairo::RefPtr<Cairo::ImageSurface> imgSurf(
        new Cairo::ImageSurface(nullptr, false));
    surface = imgSurf;
//     setImage(fileName, rtlFileName);
// }

// void RTSurface::setImage(Glib::ustring fileName, Glib::ustring rtlFileName)
// {
    Glib::ustring imageName;

    if (!rtlFileName.empty() && getDirection() == Gtk::TEXT_DIR_RTL) {
        imageName = rtlFileName;
    } else {
        imageName = fileName;
    }

    changeImage(imageName);
}


void RTSurface::changeImage(Glib::ustring imageName)
{
    auto iterator = surfaceCache.find(imageName);

    if (iterator == surfaceCache.end()) {
        // for now, we hardcode the scale to 2 or more
        int scale = std::max(2, RTScalable::getGlobalDisplayScale());
        surface = loadImage(imageName, scale);
        iterator = surfaceCache.emplace(imageName, surface).first;
    }

    surface = iterator->second;
}

int RTSurface::getWidth() const
{
    if (surface) {
        return surface->get_width() / RTScalable::getDisplayScale(surface); //RTSurface::getDisplayScale();
    } else {
        return -1;
    }
}

int RTSurface::getHeight() const
{
    if (surface) {
        return surface->get_height() / RTScalable::getDisplayScale(surface);//RTSurface::getDisplayScale();
    } else {
        return -1;
    }
}

bool RTSurface::hasSurface() const { return surface ? true : false; }
