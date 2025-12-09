/* -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright 2022 Alberto Griggio <alberto.griggio@gmail.com>
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

#include "session.h"
#include <algorithm>
#include <giomm.h>
#include <glib/gstdio.h>
#include <iostream>
#include <sstream>
#include <unordered_set>

namespace art {
namespace session {

namespace {

std::unordered_set<std::string> get_file_list(const Glib::ustring &fname)
{
    std::unordered_set<std::string> res;

    if (Glib::file_test(fname, Glib::FILE_TEST_EXISTS)) {
        auto dir = Gio::File::create_for_path(fname);
        auto src = dir->read();
        std::stringstream sbuf;
        char buf[4097];
        while (true) {
            auto n = src->read(buf, 4096);
            if (!n) {
                break;
            } else {
                buf[n] = 0;
                sbuf << buf;
            }
        }
        sbuf.seekg(0);
        std::string line;
        while (src && std::getline(sbuf, line)) {
            Glib::ustring name = line;
            // handle crlf files
            if (name.size() && name[name.size() - 1] == '\r') {
                name.resize(name.size() - 1);
            }
            if (Glib::file_test(name, Glib::FILE_TEST_EXISTS)) {
                res.insert(name);
            }
        }
    }

    return res;
}

void save(const Glib::ustring &fname,
          const std::unordered_set<std::string> &names)
{
    std::vector<std::string> l(names.begin(), names.end());
    std::sort(l.begin(), l.end());
    FILE *out = g_fopen(fname.c_str(), "wb");
    for (auto &s : l) {
        fputs(s.c_str(), out);
        fputs("\n", out);
    }
    fclose(out);
}

} // namespace

bool check(const Glib::ustring &fname)
{
    return fname == Options::SESSION_PATH;
}

Glib::ustring filename()
{
    return Glib::build_filename(options.user_config_dir, "session");
}

void load(const Glib::ustring &fname)
{
    auto fn = filename();
    auto cur = get_file_list(fname);
    save(fn, cur);
}

void save(const Glib::ustring &fname)
{
    auto fn = filename();
    auto cur = get_file_list(fn);
    save(fname, cur);
}

void clear()
{
    std::unordered_set<std::string> empty;
    save(filename(), empty);
}

void add(const std::vector<Glib::ustring> &fnames)
{
    auto fn = filename();
    auto prev = get_file_list(fn);
    for (auto &n : fnames) {
        prev.insert(n);
    }
    save(fn, prev);
}

void remove(const std::vector<Glib::ustring> &fnames)
{
    auto fn = filename();
    auto prev = get_file_list(fn);
    for (auto &n : fnames) {
        prev.erase(n);
    }
    save(fn, prev);
}

std::vector<Glib::ustring> list()
{
    auto prev = get_file_list(filename());
    std::vector<Glib::ustring> ret(prev.begin(), prev.end());
    std::sort(ret.begin(), ret.end());
    return ret;
}

Glib::ustring path() { return Options::SESSION_PATH; }

} // namespace session
} // namespace art
