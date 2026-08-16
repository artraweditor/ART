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

#include <cstdio>
#include <glibmm/ustring.h>
#include <type_traits>

namespace rtengine {

// Update a point of a Cairo::Surface by accessing the raw data
void poke255_uc(unsigned char *&dest, unsigned char r, unsigned char g,
                unsigned char b);
// Update a point of a Cairo::Surface by accessing the raw data
void poke01_d(unsigned char *&dest, double r, double g, double b, double a);
// Update a point of a Cairo::Surface by accessing the raw data
/* void poke01_f(unsigned char*& dest, float r, float g, float b); */

void bilinearInterp(const unsigned char *src, int sw, int sh,
                    unsigned char *dst, int dw, int dh);
void nearestInterp(const unsigned char *src, int sw, int sh, unsigned char *dst,
                   int dw, int dh);
void rotate(unsigned char *img, int &w, int &h, int deg);
void hflip(unsigned char *img, int w, int h);
void vflip(unsigned char *img, int w, int h);

template <typename ENUM>
constexpr typename std::underlying_type<ENUM>::type toUnderlying(ENUM value)
{
    return static_cast<typename std::underlying_type<ENUM>::type>(value);
}

// Return lower case extension without the "." or "" if the given name contains
// no "."
Glib::ustring getFileExtension(const Glib::ustring &filename);
// Return true if file has .jpeg or .jpg extension (ignoring case)
bool hasJpegExtension(const Glib::ustring &filename);
// Return true if file has .tiff or .tif extension (ignoring case)
bool hasTiffExtension(const Glib::ustring &filename);
// Return true if file has .png extension (ignoring case)
bool hasPngExtension(const Glib::ustring &filename);

void swab(const void *from, void *to, ssize_t n);

std::string getMD5(const Glib::ustring &fname, bool extended = false);

// Open a source file for binary reading. On Windows this uses CreateFileW with
// FILE_SHARE_DELETE and a non-inheritable handle, so that having the file open
// never prevents it from being renamed or deleted, and the handle is not
// duplicated into child processes (see issue #398). Everywhere else this is
// just g_fopen(fname, "rb").
FILE *g_fopen_shared_read(const Glib::ustring &fname);

// Same as above, but returns a raw file descriptor (-1 on failure). The caller
// owns it and must close() it.
int g_open_shared_read(const Glib::ustring &fname);

std::string get_html_color(int r, int g, int b);

template <class T>
class TempVarSetter {
public:
    explicit TempVarSetter(T &target):
        target_(target),
        oldval_(target)
    {
    }
    
    TempVarSetter(T &target, const T &value):
        target_(target),
        oldval_(target)
    {
        target_ = value;
    }

    ~TempVarSetter()
    {
        target_ = oldval_;
    }
private:
    T &target_;
    T oldval_;
};

} // namespace rtengine

#if __SIZEOF_WCHAR_T__ == 4
Glib::ustring utf32_to_utf8(wchar_t *UTF32Buffer, size_t sizeOfUTF32Buffer);
#endif
