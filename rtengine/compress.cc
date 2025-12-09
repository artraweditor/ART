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

#include "compress.h"
#include <giomm.h>
#include <glib/gstdio.h>
#include <glibmm.h>

namespace rtengine {

std::vector<uint8_t> compress(const std::string &src, int level)
{
    auto s = Gio::MemoryOutputStream::create(nullptr, 0, g_realloc, g_free);
    auto c =
        Gio::ZlibCompressor::create(Gio::ZLIB_COMPRESSOR_FORMAT_RAW, level);
    std::vector<uint8_t> res;
    {
        auto stream = Gio::ConverterOutputStream::create(s, c);
        stream->set_close_base_stream(true);
        gsize n;
        if (!stream->write_all(src, n)) {
            return res;
        }
    }
    char *data = static_cast<char *>(s->get_data());
    for (size_t i = 0, n = s->get_data_size(); i < n; ++i) {
        res.push_back(data[i]);
    }
    return res;
}

std::string decompress(const std::vector<uint8_t> &src)
{
    auto s = Gio::MemoryOutputStream::create(nullptr, 0, g_realloc, g_free);
    auto c = Gio::ZlibDecompressor::create(Gio::ZLIB_COMPRESSOR_FORMAT_RAW);
    std::vector<char> res;
    {
        auto stream = Gio::ConverterOutputStream::create(s, c);
        stream->set_close_base_stream(true);
        constexpr gsize chunk = 512;
        size_t i = 0;
        while (i < src.size()) {
            gsize count = std::min(src.size() - i, chunk);
            auto n = stream->write(&(src[i]), count);
            if (n < 0) {
                return "";
            } else if (n == 0) {
                break;
            }
            i += n;
        }
    }
    char *data = static_cast<char *>(s->get_data());
    for (size_t i = 0, n = s->get_data_size(); i < n; ++i) {
        res.push_back(data[i]);
    }
    res.push_back(0);
    return std::string(&(res[0]));
}

namespace {

bool convert_to(const std::string &src_fname, const std::string &dest_fname,
                bool compress)
{
    bool ok = true;
    auto f = Gio::File::create_for_path(dest_fname);
    auto s = f->append_to();
    Glib::RefPtr<Gio::Converter> c;
    if (compress) {
        c = Gio::ZlibCompressor::create(Gio::ZLIB_COMPRESSOR_FORMAT_GZIP, -1);
    } else {
        c = Gio::ZlibDecompressor::create(Gio::ZLIB_COMPRESSOR_FORMAT_GZIP);
    }
    if (c) {
        auto stream = Gio::ConverterOutputStream::create(s, c);
        stream->set_close_base_stream(true);
        constexpr gsize chunk = 512;
        char buffer[chunk];
        FILE *src = g_fopen(src_fname.c_str(), "rb");
        if (!src) {
            ok = false;
        } else {
            try {
                size_t n = 0;
                while ((n = fread(buffer, 1, chunk, src)) > 0) {
                    stream->write(buffer, n);
                }
            } catch (...) {
                ok = false;
            }
            fclose(src);
        }
    } else {
        ok = false;
    }

    return ok;
}

} // namespace

bool decompress_to(const std::string &src_fname, const std::string &dest_fname)
{
    return convert_to(src_fname, dest_fname, false);
}

bool compress_to(const std::string &src_fname, const std::string &dest_fname)
{
    return convert_to(src_fname, dest_fname, true);
}

} // namespace rtengine
