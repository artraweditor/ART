/* -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright (c) 2026 Alberto Griggio <alberto.griggio@gmail.com>
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
#include <vector>

namespace rtengine {

// A process that has a file open
struct FileInUseInfo {
    Glib::ustring app_name; // as reported by the OS, e.g. "exiftool.exe"
    unsigned long pid;
    bool is_self; // true if this is the ART process itself

    FileInUseInfo(): pid(0), is_self(false) {}
};

/**
 * Returns the processes that currently have `fname` open, so that a failed
 * rename or delete can tell the user *who* is in the way instead of just
 * "permission denied" (issue #398).
 *
 * Implemented with the Windows Restart Manager, which is the same mechanism
 * Explorer uses for its "the file is open in ..." dialog. Always returns an
 * empty list on the other platforms, where an open file never blocks a rename
 * or an unlink in the first place.
 */
std::vector<FileInUseInfo> get_file_holders(const Glib::ustring &fname);

// Formats the result of get_file_holders() as "exiftool.exe (7124), ART (3300)".
// Empty if `holders` is empty.
Glib::ustring format_file_holders(const std::vector<FileInUseInfo> &holders);

/**
 * Returns the paths of all the files this process currently has open. Only
 * meant for -v diagnostics: if get_file_holders() ever names ART itself, this
 * says which of our own handles is the one to blame. Empty on non-Windows.
 */
std::vector<Glib::ustring> get_own_open_files();

} // namespace rtengine
