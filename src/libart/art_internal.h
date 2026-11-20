/*
 *
 *  This file is part of ART.
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

/*
 *  Internals shared by the files of libART (not installed).
 */

#ifndef ART_INTERNAL_H
#define ART_INTERNAL_H

#include "ART.h"
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* where a command's text goes while it runs */
typedef struct ART_sink {
    ART_write_fn out;
    void *out_user;
    ART_write_fn err;
    void *err_user;
} ART_sink;

typedef struct ART_config { /* what an engine keeps of the options */
    int backend;
    char *socket_path; /* remote */
    char *data_dir, *config_dir, *cache_dir, *gpu; /* in-process */
    char *cwd;
    int timeout_ms;
    int verbose;
    int embed_params; /* in-process: embed the processing parameters in exported files; -1: as the options file says */
    ART_write_fn out_fn;
    void *out_user;
    ART_write_fn err_fn;
    void *err_user;
} ART_config;

struct ART_options {
    ART_config cfg;
    char error[512];
    int error_code;
};

/* A backend. exec() runs one command, reporting its text to the sink, and
 * returns an ART_* code; for the library's own failures it fills `err`
 * (size `errsize`). cwd is never NULL. */
typedef struct ART_backend_ops {
    int (*exec)(void *state, const ART_config *cfg, const char *cwd, int argc,
                const char *const *argv, const ART_sink *sink, char *err,
                size_t errsize);
    void (*destroy)(void *state);
} ART_backend_ops;

/* the remote backend (art_remote.c) */
int ART_remote_create(const ART_config *cfg, const ART_backend_ops **ops,
                      void **state, char *err, size_t errsize);

/* the in-process backend (art_embed.cc); a stub returning ART_ERR_UNSUPPORTED
 * when it is not built in */
int ART_embed_create(const ART_config *cfg, const ART_backend_ops **ops,
                     void **state, char *err, size_t errsize);
int ART_embed_available(void);

void ART_format_error(char *buf, size_t size, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* ART_INTERNAL_H */
