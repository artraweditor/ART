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

#include <exiv2/exiv2.hpp>
#include <glibmm.h>
#include <stdio.h>
#include <string>

namespace rtengine {

#if EXIV2_TEST_VERSION(0, 28, 0)
typedef Exiv2::BasicIo::UniquePtr BasicIoPtr;
#else
typedef Exiv2::BasicIo::AutoPtr BasicIoPtr;
#endif

/**
 * A read-only Exiv2::BasicIo that goes through
 * rtengine::g_fopen_shared_read(), i.e. on Windows it opens the file with
 * FILE_SHARE_DELETE and a non-inheritable handle.
 *
 * exiv2's own FileIo opens the file with plain fopen(), which the Windows CRT
 * translates to _SH_DENYNO -- FILE_SHARE_READ | FILE_SHARE_WRITE, but *not*
 * FILE_SHARE_DELETE. Any moment in which an Exiv2::Image keeps its io open
 * therefore blocks renaming and deleting the file, and FileIo::mmap() is worse
 * still: on Windows it creates a real file mapping, which makes rename and
 * delete fail with ERROR_USER_MAPPED_FILE no matter what the sharing flags
 * say. Since ART caches Exiv2::Image objects for the files it has looked at,
 * that is enough to make a file un-renameable for the rest of the session
 * (issue #398).
 *
 * Using this class for every source file we hand to exiv2 makes the whole
 * question moot: our handles never deny a rename or a delete, and mmap() here
 * is a plain heap buffer, not a mapped section.
 *
 * The semantics match FileIo: the file is opened lazily by open(), may be
 * closed and reopened, and mmap() returns a buffer holding the whole file.
 */
class SharedReadIo: public Exiv2::BasicIo {
public:
#if EXIV2_TEST_VERSION(0, 28, 0)
    // read/write counts, tell()
    typedef size_t count_t;
    // seek() offset
    typedef int64_t off_t_;
#else
    typedef long count_t;
#if defined(_MSC_VER)
    typedef int64_t off_t_;
#else
    typedef long off_t_;
#endif
#endif

    explicit SharedReadIo(const Glib::ustring &fname);
    ~SharedReadIo() override;

    int open() override;
    int close() override;

    count_t write(const Exiv2::byte *data, count_t wcount) override;
    count_t write(Exiv2::BasicIo &src) override;
    int putb(Exiv2::byte data) override;

    Exiv2::DataBuf read(count_t rcount) override;
    count_t read(Exiv2::byte *buf, count_t rcount) override;
    int getb() override;

    void transfer(Exiv2::BasicIo &src) override;
    int seek(off_t_ offset, Position pos) override;

    Exiv2::byte *mmap(bool isWriteable = false) override;
    int munmap() override;

    count_t tell() const override;
    size_t size() const override;
    bool isopen() const override;
    int error() const override;
    bool eof() const override;

#if EXIV2_TEST_VERSION(0, 28, 0)
    const std::string &path() const noexcept override { return path_; }
#else
    std::string path() const override { return path_; }
#ifdef EXV_UNICODE_PATH
    std::wstring wpath() const override { return wpath_; }
#endif
#endif

    void populateFakeData() override {}

private:
    SharedReadIo(const SharedReadIo &) = delete;
    SharedReadIo &operator=(const SharedReadIo &) = delete;

    Glib::ustring fname_; // UTF-8, for the glib calls
    std::string path_;    // for exiv2's error messages
#if !EXIV2_TEST_VERSION(0, 28, 0) && defined EXV_UNICODE_PATH
    std::wstring wpath_;
#endif
    FILE *fp_;
    Exiv2::byte *mapped_;
    size_t mapped_size_;
};

/**
 * Returns a SharedReadIo for the given file, wrapped in the smart pointer
 * flavour that this exiv2 version expects. Never null: failure to actually
 * open the file is reported by BasicIo::open(), as with Exiv2::FileIo.
 */
BasicIoPtr make_shared_read_io(const Glib::ustring &fname);

} // namespace rtengine
