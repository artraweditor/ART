/* -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright (c) 2004-2010 Gabor Horvath <hgabor@rawtherapee.com>
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

#include <glibmm.h>
#include <string>

namespace art { namespace utils {


Glib::ustring removeExtension(const Glib::ustring &filename);
Glib::ustring getExtension(const Glib::ustring &filename);
Glib::ustring fname_to_utf8(const std::string &fname);
Glib::ustring getExecutablePath(const char *argv0);
bool isSubdir(const Glib::ustring &parent, const Glib::ustring &child);

/** Follow the chain of symbolic links starting at `path` (also a dangling
 * one), and return the path of the file that a write to `path` is meant to
 * reach. Returns `path` itself if it is not a symlink. */
std::string resolveSymlinks(const std::string &path);

/** Atomically (temp file + rename) replace the contents of `path`, like
 * Glib::file_set_contents(), but if `path` is a symlink the file it points to
 * is replaced and the link is left in place.
 * Throws Glib::FileError. */
void writeFileFollowingSymlinks(const std::string &path,
                                const std::string &contents);


} } // namespace art::utils
