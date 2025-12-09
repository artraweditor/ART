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

#include "image8.h"
#include <cairomm/cairomm.h>
#include <gtkmm.h>
#include <memory>

namespace rtengine {

/** @brief Get a quick preview image out of a raw or standard file
 *
 * This class reads the full size preview image (at least the biggest one
 * available) from the raw file, or the fast demosaiced version if no suitable
 * embedded preview is found.
 *
 * For standard image, it simply read it with fast conversion for 32 bits images
 */
class PreviewImage {
public:
    PreviewImage(const Glib::ustring &fname, const Glib::ustring &ext,
                 int width = -1, int height = -1, bool enable_cms = false,
                 bool compute_histogram = false);
    ~PreviewImage();

    Cairo::RefPtr<Cairo::ImageSurface> getImage();
    void getHistogram(LUTu &r, LUTu &g, LUTu &b);

private:
    void load();
    Image8 *load_raw(const Glib::ustring &fname, int width, int height);
    Image8 *load_raw_preview(const Glib::ustring &fname, int width, int height);
    Image8 *load_img(const Glib::ustring &fname, int width, int height);
    void render(bool enable_cms);
    void get_histogram(Image8 *img);

    Glib::ustring fname_;
    Glib::ustring ext_;
    int width_;
    int height_;
    bool enable_cms_;
    bool compute_histogram_;
    bool loaded_;

    std::unique_ptr<Image8> img_;
    Cairo::RefPtr<Cairo::ImageSurface> previewImage;
    std::array<LUTu, 3> hist_;
    cmsHPROFILE imgprof_;
};

} // namespace rtengine
