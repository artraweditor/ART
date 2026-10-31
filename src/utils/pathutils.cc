/* -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright (c) 2004-2017 the RawTherapee developers
 *  Copyright (c) 2022-2026 Alberto Griggio <alberto.griggio@gmail.com>
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

#include "pathutils.h"
#include <giomm/file.h>

#ifndef WIN32
#include <glib.h>
#include <glib/gstdio.h>
#include <glibmm/fileutils.h>
#include <glibmm/threads.h>
#include <unistd.h>
#else // WIN32
#include <windows.h>
#include "conio.h"
#include <glibmm/thread.h>
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif // __APPLE__

namespace art { namespace utils {


Glib::ustring removeExtension(const Glib::ustring &filename)
{

    Glib::ustring bname = Glib::path_get_basename(filename);
    size_t lastdot = bname.find_last_of('.');
    size_t lastwhitespace = bname.find_last_of(" \t\f\v\n\r");

    if (lastdot != bname.npos &&
        (lastwhitespace == bname.npos || lastdot > lastwhitespace)) {
        return filename.substr(0, filename.size() - (bname.size() - lastdot));
    } else {
        return filename;
    }
}

Glib::ustring getExtension(const Glib::ustring &filename)
{

    Glib::ustring bname = Glib::path_get_basename(filename);
    size_t lastdot = bname.find_last_of('.');
    size_t lastwhitespace = bname.find_last_of(" \t\f\v\n\r");

    if (lastdot != bname.npos &&
        (lastwhitespace == bname.npos || lastdot > lastwhitespace)) {
        return filename.substr(filename.size() - (bname.size() - lastdot) + 1,
                               filename.npos);
    } else {
        return "";
    }
}

// For an unknown reason, Glib::filename_to_utf8 doesn't work on reliably
// Windows, so we're using Glib::filename_to_utf8 for Linux/Apple and
// Glib::locale_to_utf8 for Windows.
Glib::ustring fname_to_utf8(const std::string &fname)
{
#ifdef WIN32
    try {
        return Glib::locale_to_utf8(fname);
    } catch (Glib::Error &) {
        return Glib::convert_with_fallback(fname, "UTF-8", "ISO-8859-1", "?");
    }

#else

    return Glib::filename_to_utf8(fname);

#endif
}

Glib::ustring getExecutablePath(const char *argv0)
{
    char exname[512] = {0};

#ifdef WIN32
    WCHAR exnameU[512] = {0};
    GetModuleFileNameW(NULL, exnameU, 511);
    WideCharToMultiByte(CP_UTF8, 0, exnameU, -1, exname, 511, 0, 0);

#elif defined __APPLE__
    uint32_t bufsz = 512;

    if (_NSGetExecutablePath(exname, &bufsz) != 0) {
        g_strlcpy(exname, argv0, 512);
    }
    {
        gchar *a = g_canonicalize_filename(exname, NULL);
        g_strlcpy(exname, a, 512);
        g_free(a);
    }

#else

    if (readlink("/proc/self/exe", exname, 511) < 0) {
        g_strlcpy(exname, argv0, 512);
        gchar *a = g_canonicalize_filename(exname, NULL);
        g_strlcpy(exname, a, 512);
        g_free(a);
    }

#endif

    Glib::ustring exePath = Glib::path_get_dirname(exname);
    return exePath;
}


bool isSubdir(const Glib::ustring &parent, const Glib::ustring &child)
{
    auto p = Gio::File::create_for_path(parent);
    auto c = Gio::File::create_for_path(child);
    return c->has_prefix(p);
}


std::string resolveSymlinks(const std::string &path)
{
    std::string cur = path;
    // the limit guards against loops
    for (int i = 0; i < 40; ++i) {
        GError *err = nullptr;
        gchar *link = g_file_read_link(cur.c_str(), &err);
        if (!link) {
            // not a symlink (or not there)
            g_clear_error(&err);
            return cur;
        }
        std::string target = link;
        g_free(link);
        if (!Glib::path_is_absolute(target)) {
            target = Glib::build_filename(Glib::path_get_dirname(cur), target);
        }
        cur = target;
    }
    return path;
}


void writeFileFollowingSymlinks(const std::string &path,
                                const std::string &contents)
{
    Glib::file_set_contents(resolveSymlinks(path), contents);
}


} } // namespace art::utils
