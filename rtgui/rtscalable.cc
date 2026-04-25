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

#include "rtscalable.h"
#include "options.h"
#include "pathutils.h"
#include <glib/gstdio.h>
#include <glibmm.h>
#include <glibmm/regex.h>
#include <iostream>
#include <librsvg/rsvg.h>
#include <unordered_map>

double RTScalable::dpi_ = 0.;
int RTScalable::pseudo_hidpi_scale_ = 0;
int RTScalable::global_display_scale_ = 1;

extern Options options;
extern unsigned char initialGdkScale;
extern float fontScale;
Gtk::TextDirection RTScalable::direction_ = Gtk::TextDirection::TEXT_DIR_NONE;

double RTScalable::getDPI() { return dpi_; }

double RTScalable::getTweakedDPI() { return dpi_ * fontScale; }

int RTScalable::getPseudoHiDPIScale() { return pseudo_hidpi_scale_; }

int RTScalable::getGlobalDisplayScale()
{
    return global_display_scale_;
}

int RTScalable::getDisplayScale(const Gtk::Widget *w)
{
    if (options.pseudoHiDPISupport) {
        return 1;
    } else {
        return w ? w->get_scale_factor() : global_display_scale_;
    }
}


Gtk::TextDirection RTScalable::getDirection() { return direction_; }

void RTScalable::init(Gtk::Window *window)
{
    direction_ = window->get_direction();

    if (!options.pseudoHiDPISupport) {
        pseudo_hidpi_scale_ = 1;
        dpi_ = baseDPI;
        global_display_scale_ = window->get_scale_factor();
    } else {
        double newDPI = window->get_screen()->get_resolution();
        int newScale = std::max((int)initialGdkScale, window->get_scale_factor());
        global_display_scale_ = 1;
        pseudo_hidpi_scale_ = newScale;
        // HOMBRE: On windows, if scale = 2, the dpi is non significant, i.e.
        // should be considered = 192 ; don't know for linux/macos
        dpi_ = newDPI;
        if (pseudo_hidpi_scale_ == 1) {
            if (dpi_ >= baseHiDPI) {
                pseudo_hidpi_scale_ = 2;
            }
        } else if (pseudo_hidpi_scale_ == 2) {
            if (dpi_ < baseHiDPI) {
                dpi_ *= 2.;
            }
        }
    }
}

namespace {

std::map<std::pair<std::string, int>, Cairo::RefPtr<Cairo::ImageSurface>> image_cache;

Cairo::RefPtr<Cairo::ImageSurface> cache_put(const Glib::ustring &fname, int scale, Cairo::RefPtr<Cairo::ImageSurface> surf)
{
    image_cache[std::make_pair(fname, scale)] = surf;
    return surf;
}

Cairo::RefPtr<Cairo::ImageSurface> cache_get(const Glib::ustring &fname, int scale)
{
    Cairo::RefPtr<Cairo::ImageSurface> res;
    auto it = image_cache.find(std::make_pair(fname, scale));
    if (it != image_cache.end()) {
        res = it->second;
    }
    return res;
}

} // namespace

Cairo::RefPtr<Cairo::ImageSurface>
RTScalable::loadImage(const Glib::ustring &fname, double dpi, int scale)
{
    Glib::ustring imagesFolder =
        Glib::build_filename(options.ART_base_dir, "images");

    if (!scale) {
        scale = getGlobalDisplayScale();
    }

    auto res = cache_get(fname, scale);
    if (res) {
        return res;
    }
    auto path = Glib::build_filename(imagesFolder, fname);

    std::string svgFile;

    if (getExtension(fname) == "png") {
        return cache_put(fname, scale, Cairo::ImageSurface::create_from_png(path));
    }

    // -------------------- Loading the SVG file --------------------

    try {
        svgFile = Glib::file_get_contents(path);
    } catch (Glib::FileError &err) {
        std::cerr << "ERROR: " << err.what() << std::endl;
        Cairo::RefPtr<Cairo::ImageSurface> surf =
            Cairo::ImageSurface::create(Cairo::FORMAT_RGB24, 10, 10);
        return surf;
    }

    // -------------------- Updating the the magic color --------------------
    // Magic color       : #2a7fff

    std::string updatedSVG = Glib::Regex::create("#2a7fff")->replace(
        svgFile, 0, options.svg_color, Glib::RegexMatchFlags());

    // -------------------- Creating the rsvg handle --------------------

    GError **error = nullptr;
    RsvgHandle *handle = rsvg_handle_new_from_data(
        (unsigned const char *)updatedSVG.c_str(), updatedSVG.length(), error);

    if (handle == nullptr) {
        std::cerr << "ERROR: Can't use the provided data for \"" << fname
                  << "\" to create a RsvgHandle:" << std::endl
                  << Glib::ustring((*error)->message) << std::endl;
        Cairo::RefPtr<Cairo::ImageSurface> surf =
            Cairo::ImageSurface::create(Cairo::FORMAT_RGB24, 10, 10);
        return surf;
    }

    // -------------------- Drawing the image to a Cairo::ImageSurface
    // --------------------

    RsvgDimensionData dim;
    rsvg_handle_get_dimensions(handle, &dim);
    double r = dpi / baseDPI * scale;
    Cairo::RefPtr<Cairo::ImageSurface> surf = Cairo::ImageSurface::create(
        Cairo::FORMAT_ARGB32, (int)(dim.width * r + 0.499),
        (int)(dim.height * r + 0.499));
    Cairo::RefPtr<Cairo::Context> c = Cairo::Context::create(surf);
    c->set_source_rgba(0., 0., 0., 0.);
    c->set_operator(Cairo::OPERATOR_CLEAR);
    c->paint();
    c->set_operator(Cairo::OPERATOR_OVER);
    c->scale(r, r);
    rsvg_handle_render_cairo(handle, c->cobj());
    rsvg_handle_free(handle);

    setDisplayScale(surf, scale);

    return cache_put(fname, scale, surf);
}

void RTScalable::setDisplayScale(const Cairo::RefPtr<Cairo::Surface> &surface,
                                int scale)
{
    cairo_surface_t *cobj = surface->cobj();
    cairo_surface_set_device_scale(cobj, scale, scale);
}


int RTScalable::getDisplayScale(const Cairo::RefPtr<Cairo::Surface> &surface)
{
    cairo_surface_t *cobj = surface->cobj();
    double scale = 1;
    cairo_surface_get_device_scale(cobj, &scale, &scale);
    return scale;
}
