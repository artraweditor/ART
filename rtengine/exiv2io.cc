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

#include "exiv2io.h"
#include "utils.h"
#include <glib/gstdio.h>
#include <limits>
#include <memory>

namespace rtengine {

namespace {

constexpr size_t BAD_SIZE = std::numeric_limits<size_t>::max();

// SharedReadIo is read-only and never grows, so all the failure modes exiv2
// can see from it are "this file could not be read"
void throw_io_error()
{
#if EXIV2_TEST_VERSION(0, 28, 0)
    throw Exiv2::Error(Exiv2::ErrorCode::kerFailedToReadImageData);
#elif EXIV2_TEST_VERSION(0, 27, 0)
    throw Exiv2::Error(Exiv2::kerFailedToReadImageData);
#else
    throw Exiv2::Error(1);
#endif
}

} // namespace

SharedReadIo::SharedReadIo(const Glib::ustring &fname)
    : fname_(fname), path_(fname.raw()), fp_(nullptr), mapped_(nullptr),
      mapped_size_(0)
{
#if !EXIV2_TEST_VERSION(0, 28, 0) && defined EXV_UNICODE_PATH
    gunichar2 *w =
        g_utf8_to_utf16(fname.c_str(), -1, nullptr, nullptr, nullptr);
    if (w) {
        wpath_ = reinterpret_cast<const wchar_t *>(w);
        g_free(w);
    }
#endif
}

SharedReadIo::~SharedReadIo() { close(); }

int SharedReadIo::open()
{
    // as with Exiv2::FileIo, open() on an already-open instance reopens it and
    // resets the position to the start
    close();
    fp_ = g_fopen_shared_read(fname_);
    return fp_ ? 0 : 1;
}

int SharedReadIo::close()
{
    munmap();
    if (fp_) {
        ::fclose(fp_);
        fp_ = nullptr;
    }
    return 0;
}

SharedReadIo::count_t SharedReadIo::write(const Exiv2::byte *, count_t)
{
    return 0;
}

SharedReadIo::count_t SharedReadIo::write(Exiv2::BasicIo &) { return 0; }

int SharedReadIo::putb(Exiv2::byte) { return EOF; }

Exiv2::DataBuf SharedReadIo::read(count_t rcount)
{
#if EXIV2_TEST_VERSION(0, 28, 0)
    if (rcount > size()) {
        throw Exiv2::Error(Exiv2::ErrorCode::kerInvalidMalloc);
    }
    Exiv2::DataBuf buf(rcount);
    count_t n = read(buf.data(), buf.size());
    if (n == 0) {
        throw Exiv2::Error(Exiv2::ErrorCode::kerInputDataReadFailed);
    }
    buf.resize(n);
    return buf;
#else
    Exiv2::DataBuf buf(rcount);
    long n = read(buf.pData_, buf.size_);
    buf.size_ = n;
    return buf;
#endif
}

SharedReadIo::count_t SharedReadIo::read(Exiv2::byte *buf, count_t rcount)
{
#if EXIV2_TEST_VERSION(0, 28, 0)
    if (!fp_ || rcount == 0) {
#else
    if (!fp_ || rcount <= 0) {
#endif
        return 0;
    }
    return count_t(::fread(buf, 1, size_t(rcount), fp_));
}

int SharedReadIo::getb()
{
    if (!fp_) {
        return EOF;
    }
    return ::fgetc(fp_);
}

void SharedReadIo::transfer(Exiv2::BasicIo &) { throw_io_error(); }

int SharedReadIo::seek(off_t_ offset, Position pos)
{
    if (!fp_) {
        return 1;
    }

    int whence = SEEK_SET;
    switch (pos) {
    case Exiv2::BasicIo::cur:
        whence = SEEK_CUR;
        break;
    case Exiv2::BasicIo::end:
        whence = SEEK_END;
        break;
    case Exiv2::BasicIo::beg:
    default:
        whence = SEEK_SET;
        break;
    }

#ifdef WIN32
    return ::_fseeki64(fp_, offset, whence);
#else
    return ::fseeko(fp_, offset, whence);
#endif
}

Exiv2::byte *SharedReadIo::mmap(bool isWriteable)
{
    if (isWriteable) {
        // we never hand out a writeable view of a source file
        throw_io_error();
    }

    munmap();

    const size_t sz = size();
    if (sz == BAD_SIZE || !fp_ || seek(0, Exiv2::BasicIo::beg) != 0) {
        throw_io_error();
    }

    // note that this is a plain heap buffer and not a mapped section: a live
    // MapViewOfFile() would make the file un-renameable on Windows whatever
    // its sharing flags are (issue #398). exiv2's own FileIo::mmap() does the
    // same thing on the platforms that have no mmap().
    std::unique_ptr<Exiv2::byte[]> buf(new Exiv2::byte[sz ? sz : 1]);
    if (sz && size_t(read(buf.get(), count_t(sz))) != sz) {
        throw_io_error();
    }

    mapped_size_ = sz;
    mapped_ = buf.release();
    return mapped_;
}

int SharedReadIo::munmap()
{
    delete[] mapped_;
    mapped_ = nullptr;
    mapped_size_ = 0;
    return 0;
}

SharedReadIo::count_t SharedReadIo::tell() const
{
    if (!fp_) {
        return count_t(-1);
    }
#ifdef WIN32
    return count_t(::_ftelli64(fp_));
#else
    return count_t(::ftello(fp_));
#endif
}

size_t SharedReadIo::size() const
{
    GStatBuf st;
    if (::g_stat(fname_.c_str(), &st) != 0) {
        return BAD_SIZE;
    }
    return size_t(st.st_size);
}

bool SharedReadIo::isopen() const { return fp_ != nullptr; }

int SharedReadIo::error() const { return fp_ ? ::ferror(fp_) : 0; }

bool SharedReadIo::eof() const { return fp_ ? ::feof(fp_) != 0 : true; }

BasicIoPtr make_shared_read_io(const Glib::ustring &fname)
{
    BasicIoPtr ret(new SharedReadIo(fname));
    return ret;
}

} // namespace rtengine
