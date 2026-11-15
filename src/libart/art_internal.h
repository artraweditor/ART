/*
 *  This file is part of ART. GPL-3.0-or-later, see ART.h.
 *
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
