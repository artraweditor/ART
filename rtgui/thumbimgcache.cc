/* -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright 2023 Alberto Griggio <alberto.griggio@gmail.com>
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

#include "thumbimgcache.h"
#include "../rtengine/image8.h"
#include "options.h"
#include <glib/gstdio.h>
#include <iostream>

extern Options options;

namespace art {
namespace thumbimgcache {

/******************************************************************************
 * file format:
 *
 * "ART\n" header
 * monitor hash
 * size of the procparams
 * compressed procparams
 * width
 * height
 * image data
 ******************************************************************************/
rtengine::IImage8 *load(const Glib::ustring &cache_fname,
                        const rtengine::procparams::ProcParams &pparams, int h)
{
    if (!options.thumb_cache_processed) {
        return nullptr;
    }

    Glib::ustring fname = cache_fname + ".artt";

    if (!Glib::file_test(fname, Glib::FILE_TEST_EXISTS)) {
        return nullptr;
    }

    FILE *f = g_fopen(fname.c_str(), "rb");

    if (!f) {
        return nullptr;
    }

    // header
    char buffer[64];
    fgets(buffer, 5, f);
    if (strcmp(buffer, "ART\n") != 0) {
        return nullptr;
    }

    // monitor hash
    if (fread(buffer, sizeof(char), 33, f) < 33) {
        return nullptr;
    }
    buffer[33] = '\0';
    if (strcmp(buffer, rtengine::ICCStore::getInstance()
                           ->getThumbnailMonitorHash()
                           .c_str()) != 0) {
        fclose(f);
        return nullptr;
    }

    // size of the profile data
    guint32 profsz = 0;
    if (fread(&profsz, 1, sizeof(guint32), f) < sizeof(guint32)) {
        fclose(f);
        return nullptr;
    }

    rtengine::procparams::ProcParams imgparams;
    {
        std::vector<uint8_t> profzdata(profsz);
        if (fread(&profzdata[0], sizeof(uint8_t), profsz, f) < profsz) {
            fclose(f);
            return nullptr;
        }
        std::string profdata = rtengine::decompress(profzdata);
        if (!imgparams.from_data(profdata.c_str())) {
            fclose(f);
            return nullptr;
        }
    }
    if (imgparams != pparams) {
        fclose(f);
        return nullptr;
    }

    guint32 width = 0, height = 0;

    if (fread(&width, 1, sizeof(guint32), f) < sizeof(guint32)) {
        fclose(f);
        return nullptr;
    }

    if (fread(&height, 1, sizeof(guint32), f) < sizeof(guint32)) {
        fclose(f);
        return nullptr;
    }

    if (std::min(width, height) <= 0) {
        fclose(f);
        return nullptr;
    }

    if (guint32(h) != height) {
        fclose(f);
        return nullptr;
    }

    rtengine::Image8 *image = new rtengine::Image8(width, height);
    image->readData(f);
    fclose(f);

    // if (guint32(h) < height) {
    //     int w = int(float(width) * float(guint32(h) / height));
    //     rtengine::Image8 *resized = new rtengine::Image8(w, h);
    //     image->resizeImgTo<>(w, h, rtengine::TI_Nearest, resized);
    //     delete image;
    //     image = resized;
    // }

    if (options.rtSettings.verbose > 1) {
        std::cout << "read from cache: " << fname << " " << width << "x"
                  << height << std::endl;
    }

    return image;
}

bool store(const Glib::ustring &cache_fname,
           const rtengine::procparams::ProcParams &pparams,
           rtengine::IImage8 *img)
{
    if (!options.thumb_cache_processed) {
        return false;
    }

    Glib::ustring fname = cache_fname + ".artt";
    FILE *f = g_fopen(fname.c_str(), "wb");

    if (!f) {
        return false;
    }

    fputs("ART\n", f);
    fputs(rtengine::ICCStore::getInstance()->getThumbnailMonitorHash().c_str(),
          f);
    std::vector<uint8_t> profzdata = rtengine::compress(pparams.to_data(), 1);
    guint32 profsz = guint32(profzdata.size());
    fwrite(&profsz, sizeof(guint32), 1, f);
    fwrite(&profzdata[0], sizeof(uint8_t), profsz, f);

    guint32 w = guint32(img->getWidth());
    guint32 h = guint32(img->getHeight());
    fwrite(&w, sizeof(guint32), 1, f);
    fwrite(&h, sizeof(guint32), 1, f);

    img->writeData(f);

    fclose(f);

    if (options.rtSettings.verbose > 1) {
        std::cout << "saved in cache: " << fname << " " << w << "x" << h
                  << std::endl;
    }

    return true;
}

} // namespace thumbimgcache
} // namespace art
