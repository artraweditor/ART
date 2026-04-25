/*
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2004-2010 Gabor Horvath <hgabor@rawtherapee.com>
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

#include "rtimage.h"

#include <iostream>

#include "options.h"

namespace {

std::map<std::string, Glib::RefPtr<Gdk::Pixbuf>> pixbufCache;
std::map<std::pair<std::string, int>, Cairo::RefPtr<Cairo::ImageSurface>> surfaceCache;

} // namespace

RTImage::RTImage(): path_("") {}

RTImage::RTImage(RTImage &other): path_("")
{
    pixbuf = other.pixbuf;
    surface = other.surface;
    path_ = other.path_;
    if (pixbuf) {
        set(pixbuf);
    } else if (surface) {
        set(surface);
    }
}

RTImage::RTImage(const Glib::ustring &fileName,
                 const Glib::ustring &rtlFileName)
    : Gtk::Image()
{
    setImage(fileName, rtlFileName);
    property_scale_factor().signal_changed().connect(sigc::mem_fun(*this, &RTImage::updateScale));
}

RTImage::RTImage(Glib::RefPtr<Gdk::Pixbuf> &pbuf)
{
    if (surface) {
        surface.clear();
    }
    if (pbuf) {
        set(pbuf);
        this->pixbuf = pbuf;
    }
    path_ = "";
}

RTImage::RTImage(Cairo::RefPtr<Cairo::ImageSurface> &surf)
{
    if (pixbuf) {
        pixbuf.clear();
    }
    if (surf) {
        set(surf);
        surface = surf;
    }
    path_ = "";
}

RTImage::RTImage(Glib::RefPtr<RTImage> &other)
{
    if (other) {
        if (other->get_surface()) {
            surface = other->get_surface();
            set(surface);
        } else {
            pixbuf = other->get_pixbuf();
            set(pixbuf);
        }
        path_ = other->path_;
        property_scale_factor().signal_changed().connect(sigc::mem_fun(*this, &RTImage::updateScale));
    } else {
        path_ = "";
    }
}

void RTImage::setImage(const Glib::ustring &fileName,
                       const Glib::ustring &rtlFileName)
{
    Glib::ustring imageName;

    if (!rtlFileName.empty() && getDirection() == Gtk::TEXT_DIR_RTL) {
        imageName = rtlFileName;
    } else {
        imageName = fileName;
    }

    changeImage(imageName);
}

void RTImage::changeImage(const Glib::ustring &imageName)
{
    clear();

    path_ = imageName;

    if (imageName.empty()) {
        return;
    }

    if (pixbuf) {
        auto iterator = pixbufCache.find(imageName);
        assert(iterator != pixbufCache.end());
        pixbuf = iterator->second;
        set(iterator->second);
    } else { // if no Pixbuf is set, we update or create a Cairo::ImageSurface
        int scale = RTScalable::getDisplayScale(this);
        auto key = std::make_pair(imageName, scale);
        auto iterator = surfaceCache.find(key);
        if (iterator == surfaceCache.end()) {
            auto surf = createImgSurfFromFile(imageName, scale);
            iterator = surfaceCache.emplace(key, surf).first;
        }
        surface = iterator->second;
        set(iterator->second);
    }
}

Cairo::RefPtr<Cairo::ImageSurface> RTImage::get_surface() { return surface; }

int RTImage::get_width()
{
    if (surface) {
        return surface->get_width();
    }
    if (pixbuf) {
        return pixbuf->get_width();
    }
    return -1;
}

int RTImage::get_height()
{
    if (surface) {
        return surface->get_height();
    }
    if (pixbuf) {
        return pixbuf->get_height();
    }
    return -1;
}


Glib::RefPtr<Gdk::Pixbuf>
RTImage::createPixbufFromFile(const Glib::ustring &fileName, int scale)
{
    Cairo::RefPtr<Cairo::ImageSurface> imgSurf =
        createImgSurfFromFile(fileName, scale);
    int s = scale > 0 ? scale : RTScalable::getGlobalDisplayScale();
    auto res = Gdk::Pixbuf::create(imgSurf, 0, 0, imgSurf->get_width(),
                                   imgSurf->get_height());
    if (s > 1) {
        res = res->scale_simple(imgSurf->get_width() / s,
                                imgSurf->get_height() / s, Gdk::INTERP_TILES);
    }
    return res;
}

Cairo::RefPtr<Cairo::ImageSurface>
RTImage::createImgSurfFromFile(const Glib::ustring &fileName, int scale)
{
    Cairo::RefPtr<Cairo::ImageSurface> surf;

    try {
        surf = loadImage(fileName, getTweakedDPI(), scale);
    } catch (const Glib::Exception &exception) {
        if (options.rtSettings.verbose) {
            std::cerr << "Failed to load image \"" << fileName
                      << "\": " << exception.what() << std::endl;
        }
    }

    return surf;
}


void RTImage::updateScale()
{
    if (!path_.empty()) {
        changeImage(path_);
        queue_draw();
    }
}
