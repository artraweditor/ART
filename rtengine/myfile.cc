/*
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
#include "myfile.h"
#include "utils.h"
#include <cstdarg>
#include <glibmm.h>

// Note: this used to have a memory-mapped variant, guarded by MYFILE_MMAP. No
// build ever defined that macro, and on Windows a live MapViewOfFile() makes
// renaming or deleting the file fail with ERROR_USER_MAPPED_FILE whatever the
// sharing flags on the handle are (issue #398), so the variant is gone: we
// always read the whole file into a heap buffer and close it right away. (The
// remaining #ifdef MYFILE_MMAP blocks in dcraw.cc and fujicompressed.cc are
// dead code for the same reason.)

namespace {

IMFILE *read_whole_file(const char *fname)
{
    FILE *f = rtengine::g_fopen_shared_read(fname);

    if (!f) {
        return nullptr;
    }

    IMFILE *mf = new IMFILE;
    memset(mf, 0, sizeof(*mf));
    fseek(f, 0, SEEK_END);
    mf->size = ftell(f);
    mf->data = new char[mf->size];
    fseek(f, 0, SEEK_SET);
    fread(mf->data, 1, mf->size, f);
    fclose(f);
    mf->pos = 0;
    mf->eof = false;

    return mf;
}

} // namespace

IMFILE *fopen(const char *fname) { return read_whole_file(fname); }

IMFILE *gfopen(const char *fname) { return read_whole_file(fname); }

IMFILE *fopen(unsigned *buf, ssize_t size)
{

    IMFILE *mf = new IMFILE;
    memset(mf, 0, sizeof(*mf));
    mf->size = size;
    mf->data = new char[mf->size];
    memcpy((void *)mf->data, buf, size);
    mf->pos = 0;
    mf->eof = false;
    return mf;
}

void fclose(IMFILE *f)
{
    delete[] f->data;
    delete f;
}

int fscanf(IMFILE *f, const char *s...)
{
    // fscanf not easily wrapped since we have no terminating \0 at end
    // of file data and vsscanf() won't tell us how many characters that
    // were parsed. However, only dcraw.cc code use it and only for "%f" and
    // "%d", so we make a dummy fscanf here just to support dcraw case.
    char buf[51], *endptr = nullptr;
    ssize_t copy_sz = f->size - f->pos;

    if (copy_sz >= static_cast<ssize_t>(sizeof(buf))) {
        copy_sz = sizeof(buf) - 1;
    }

    memcpy(buf, &f->data[f->pos], copy_sz);
    buf[copy_sz] = '\0';
    va_list ap;
    va_start(ap, s);

    if (strcmp(s, "%d") == 0) {
        int i = strtol(buf, &endptr, 10);

        if (endptr == buf) {
            va_end(ap);
            return 0;
        }

        int *pi = va_arg(ap, int *);
        *pi = i;
    } else if (strcmp(s, "%f") == 0) {
        float f = strtof(buf, &endptr);

        if (endptr == buf) {
            va_end(ap);
            return 0;
        }

        float *pf = va_arg(ap, float *);
        *pf = f;
    }

    va_end(ap);
    f->pos += endptr - buf;
    return 1;
}

char *fgets(char *s, ssize_t n, IMFILE *f)
{

    if (f->pos >= f->size) {
        f->eof = true;
        return nullptr;
    }

    ssize_t i = 0;

    do {
        s[i++] = f->data[f->pos++];
    } while (i < n && f->pos < f->size);

    return s;
}

void imfile_set_plistener(IMFILE *f, rtengine::ProgressListener *plistener,
                          double progress_range)
{
    f->plistener = plistener;
    f->progress_range = progress_range;
    f->progress_next = f->size / 10 + 1;
    f->progress_current = 0;
}

void imfile_update_progress(IMFILE *f)
{
    if (!f->plistener || f->progress_current < f->progress_next) {
        return;
    }

    do {
        f->progress_next += f->size / 10 + 1;
    } while (f->progress_next < f->progress_current);

    double p = (double)f->progress_current / f->size;

    if (p > 1.0) {
        /* this can happen if same bytes are read over and over again. Progress
           bar is not intended to be exact, just give some progress indication
           for normal raw file access patterns */
        p = 1.0;
    }

    f->plistener->setProgress(p * f->progress_range);
}
