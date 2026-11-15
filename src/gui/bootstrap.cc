/* -*- C++ -*-
 *
 *  This file is part of ART.
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

#include "bootstrap.h"
#include "options.h"
#include <glib.h>
#include <glib/gstdio.h>
#include <glibmm/fileutils.h>
#include <iostream>
#include <tiffio.h>

namespace art { namespace gui { namespace bootstrap {

int init(bool quickstart, int verbose, std::string *error,
         const std::function<void(const char *)> &phase)
{
    if (phase) {
        phase("main");
    }
    try {
        Options::load(quickstart, verbose);
    } catch (Options::Error &e) {
        if (error) {
            *error = e.get_msg();
        } else {
            std::cerr << std::endl << "Error:" << e.get_msg() << std::endl;
        }
        return -2;
    }
    if (phase) {
        phase("options loaded");
    }

    if (options.is_defProfRawMissing()) {
        options.defProfRaw = Options::DEFPROFILE_RAW;
        std::cerr << std::endl
                  << "The default profile for raw photos could not be found or "
                     "is not set."
                  << std::endl
                  << "Please check your profiles' directory, it may be missing "
                     "or damaged."
                  << std::endl
                  << "\"" << Options::DEFPROFILE_RAW
                  << "\" will be used instead." << std::endl
                  << std::endl;
    }
    if (options.is_bundledDefProfRawMissing()) {
        std::cerr << std::endl
                  << "The bundled profile \"" << options.defProfRaw
                  << "\" could not be found!" << std::endl
                  << "Your installation could be damaged." << std::endl
                  << "Default internal values will be used instead."
                  << std::endl
                  << std::endl;
        options.defProfRaw = Options::DEFPROFILE_INTERNAL;
    }

    if (options.is_defProfImgMissing()) {
        options.defProfImg = Options::DEFPROFILE_IMG;
        std::cerr << std::endl
                  << "The default profile for non-raw photos could not be "
                     "found or is not set."
                  << std::endl
                  << "Please check your profiles' directory, it may be missing "
                     "or damaged."
                  << std::endl
                  << "\"" << Options::DEFPROFILE_IMG
                  << "\" will be used instead." << std::endl
                  << std::endl;
    }
    if (options.is_bundledDefProfImgMissing()) {
        std::cerr << std::endl
                  << "The bundled profile " << options.defProfImg
                  << " could not be found!" << std::endl
                  << "Your installation could be damaged." << std::endl
                  << "Default internal values will be used instead."
                  << std::endl
                  << std::endl;
        options.defProfImg = Options::DEFPROFILE_INTERNAL;
    }

    TIFFSetWarningHandler(nullptr); // avoid annoying message boxes

#ifndef WIN32

    // Move the old path to the new one if the new does not exist
    if (Glib::file_test(Glib::build_filename(options.user_config_dir, "cache"),
                        Glib::FILE_TEST_IS_DIR) &&
        !Glib::file_test(options.cacheBaseDir, Glib::FILE_TEST_IS_DIR)) {
        if (g_rename(
                Glib::build_filename(options.user_config_dir, "cache").c_str(),
                options.cacheBaseDir.c_str()) == -1) {
            std::cout << "g_rename "
                      << Glib::build_filename(options.user_config_dir, "cache")
                             .c_str()
                      << " => " << options.cacheBaseDir.c_str() << " failed."
                      << std::endl;
        }
    }

#endif

    return 0;
}

}}} // namespace art::gui::bootstrap
