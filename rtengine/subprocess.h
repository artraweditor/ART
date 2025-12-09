/* -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright (c) 2020 Alberto Griggio <alberto.griggio@gmail.com>
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

#include "noncopyable.h"
#include <exception>
#include <glibmm.h>
#include <sstream>
#include <string>

namespace rtengine {
namespace subprocess {

class error: public std::exception {
public:
    error() {}
    error(const error &other): buf_() { buf_ << other.buf_.str(); }

    template <class T> error &operator<<(const T &t)
    {
        buf_ << t;
        return *this;
    }

    const char *what() const throw()
    {
        msg_ = buf_.str();
        return msg_.c_str();
    }

private:
    std::ostringstream buf_;
    mutable std::string msg_;
};

std::wstring to_wstr(const Glib::ustring &s);

#ifdef WIN32
std::wstring quote(const std::wstring &s);
#endif // WIN32

std::vector<Glib::ustring> split_command_line(const Glib::ustring &cmdl);

void exec_sync(const Glib::ustring &workdir,
               const std::vector<Glib::ustring> &argv, bool search_in_path,
               std::string *out, std::string *err);

std::vector<std::string> get_env();

class SubprocessInfo: public NonCopyable {
public:
    explicit SubprocessInfo(uintptr_t impl): impl_(impl) {}
    ~SubprocessInfo();

    int read();
    bool write(const char *s, size_t n);
    bool flush();

    bool live() const;
    int wait();
    void kill();

    int id() const;

private:
    uintptr_t impl_;
};

std::unique_ptr<SubprocessInfo> popen(const Glib::ustring &workdir,
                                      const std::vector<Glib::ustring> &argv,
                                      bool search_in_path, bool pipe_in,
                                      bool pipe_out);

} // namespace subprocess
} // namespace rtengine
