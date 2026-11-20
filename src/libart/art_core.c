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
 *  libART: options, engines, results (the part that does not depend on the
 *  backend).
 */

#define _POSIX_C_SOURCE 200809L

#include "art_internal.h"
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#  include <direct.h>
#  include <io.h>
#  include <windows.h>
#  define art_getcwd _getcwd
#  define art_write _write
typedef CRITICAL_SECTION art_mutex;
static void art_mutex_init(art_mutex *m) { InitializeCriticalSection(m); }
static void art_mutex_destroy(art_mutex *m) { DeleteCriticalSection(m); }
static void art_mutex_lock(art_mutex *m) { EnterCriticalSection(m); }
static void art_mutex_unlock(art_mutex *m) { LeaveCriticalSection(m); }
#else
#  include <pthread.h>
#  include <unistd.h>
#  define art_getcwd getcwd
#  define art_write write
typedef pthread_mutex_t art_mutex;
static void art_mutex_init(art_mutex *m) { pthread_mutex_init(m, NULL); }
static void art_mutex_destroy(art_mutex *m) { pthread_mutex_destroy(m); }
static void art_mutex_lock(art_mutex *m) { pthread_mutex_lock(m); }
static void art_mutex_unlock(art_mutex *m) { pthread_mutex_unlock(m); }
#endif

#ifndef ART_LIBART_VERSION
#  define ART_LIBART_VERSION "unknown"
#endif

/* ---- small helpers ----------------------------------------------------- */

void ART_format_error(char *buf, size_t size, const char *fmt, ...)
{
    va_list ap;
    if (!buf || !size) {
        return;
    }
    va_start(ap, fmt);
    vsnprintf(buf, size, fmt, ap);
    va_end(ap);
}


static char *art_strdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char *p = (char *)malloc(n);
    if (p) {
        memcpy(p, s, n);
    }
    return p;
}


/* replace *dst with a copy of s (NULL clears it) */
static int set_str(char **dst, const char *s)
{
    char *p = NULL;
    if (s) {
        p = art_strdup(s);
        if (!p) {
            return ART_ERR_NOMEM;
        }
    }
    free(*dst);
    *dst = p;
    return ART_OK;
}


/* ---- default streams ---------------------------------------------------- */

static void write_stdout(void *user, const char *data, size_t len)
{
    (void)user;
    fwrite(data, 1, len, stdout);
    fflush(stdout);
}

static void write_stderr(void *user, const char *data, size_t len)
{
    (void)user;
    fwrite(data, 1, len, stderr);
    fflush(stderr);
}

static void write_file(void *user, const char *data, size_t len)
{
    FILE *f = (FILE *)user;
    fwrite(data, 1, len, f);
    fflush(f);
}

static void write_fd(void *user, const char *data, size_t len)
{
    int fd = (int)(size_t)user;
    while (len > 0) {
        long r = (long)art_write(fd, data, (unsigned)len);
        if (r < 0 && errno == EINTR) {
            continue;
        }
        if (r <= 0) {
            return;
        }
        data += r;
        len -= (size_t)r;
    }
}


/* ---- options ----------------------------------------------------------- */

ART_options *ART_options_new(void)
{
    ART_options *o = (ART_options *)calloc(1, sizeof(ART_options));
    if (o) {
        o->cfg.backend = ART_BACKEND_REMOTE;
        o->cfg.timeout_ms = -1;
        o->cfg.embed_params = -1;
        o->cfg.out_fn = write_stdout;
        o->cfg.err_fn = write_stderr;
    }
    return o;
}

static void cfg_free(ART_config *c)
{
    free(c->socket_path);
    free(c->data_dir);
    free(c->config_dir);
    free(c->cache_dir);
    free(c->gpu);
    free(c->cwd);
    memset(c, 0, sizeof(*c));
}

void ART_options_free(ART_options *o)
{
    if (o) {
        cfg_free(&o->cfg);
        free(o);
    }
}

static int cfg_copy(ART_config *dst, const ART_config *src)
{
    *dst = *src;
    dst->socket_path = dst->data_dir = dst->config_dir = dst->cache_dir = NULL;
    dst->gpu = dst->cwd = NULL;
    if (set_str(&dst->socket_path, src->socket_path) ||
        set_str(&dst->data_dir, src->data_dir) ||
        set_str(&dst->config_dir, src->config_dir) ||
        set_str(&dst->cache_dir, src->cache_dir) ||
        set_str(&dst->gpu, src->gpu) || set_str(&dst->cwd, src->cwd)) {
        cfg_free(dst);
        return ART_ERR_NOMEM;
    }
    return ART_OK;
}

#define CHECK_OPTS(o) \
    if (!(o)) {       \
        return ART_ERR_INVALID; \
    }

int ART_options_set_remote(ART_options *o, const char *socket_path)
{
    CHECK_OPTS(o);
    o->cfg.backend = ART_BACKEND_REMOTE;
    return set_str(&o->cfg.socket_path, socket_path);
}

int ART_options_set_inprocess(ART_options *o, const char *data_dir,
                              const char *config_dir, const char *cache_dir)
{
    int rc;
    CHECK_OPTS(o);
    o->cfg.backend = ART_BACKEND_INPROCESS;
    if ((rc = set_str(&o->cfg.data_dir, data_dir)) ||
        (rc = set_str(&o->cfg.config_dir, config_dir)) ||
        (rc = set_str(&o->cfg.cache_dir, cache_dir))) {
        return rc;
    }
    return ART_OK;
}

int ART_options_set_output(ART_options *o, ART_write_fn fn, void *user)
{
    CHECK_OPTS(o);
    o->cfg.out_fn = fn ? fn : write_stdout;
    o->cfg.out_user = fn ? user : NULL;
    return ART_OK;
}

int ART_options_set_error(ART_options *o, ART_write_fn fn, void *user)
{
    CHECK_OPTS(o);
    o->cfg.err_fn = fn ? fn : write_stderr;
    o->cfg.err_user = fn ? user : NULL;
    return ART_OK;
}

int ART_options_set_output_file(ART_options *o, FILE *f)
{
    CHECK_OPTS(o);
    return ART_options_set_output(o, f ? write_file : NULL, f);
}

int ART_options_set_error_file(ART_options *o, FILE *f)
{
    CHECK_OPTS(o);
    return ART_options_set_error(o, f ? write_file : NULL, f);
}

int ART_options_set_output_fd(ART_options *o, int fd)
{
    CHECK_OPTS(o);
    if (fd < 0) {
        return ART_ERR_INVALID;
    }
    return ART_options_set_output(o, write_fd, (void *)(size_t)fd);
}

int ART_options_set_error_fd(ART_options *o, int fd)
{
    CHECK_OPTS(o);
    if (fd < 0) {
        return ART_ERR_INVALID;
    }
    return ART_options_set_error(o, write_fd, (void *)(size_t)fd);
}

int ART_options_set_cwd(ART_options *o, const char *dir)
{
    CHECK_OPTS(o);
    return set_str(&o->cfg.cwd, dir);
}

int ART_options_set_timeout(ART_options *o, int ms)
{
    CHECK_OPTS(o);
    if (ms < -1) {
        return ART_ERR_INVALID;
    }
    o->cfg.timeout_ms = ms;
    return ART_OK;
}

int ART_options_set_gpu(ART_options *o, const char *mode)
{
    CHECK_OPTS(o);
    return set_str(&o->cfg.gpu, mode);
}

int ART_options_set_embed_params(ART_options *o, int on)
{
    CHECK_OPTS(o);
    o->cfg.embed_params = on ? 1 : 0;
    return ART_OK;
}

int ART_options_set_verbose(ART_options *o, int level)
{
    CHECK_OPTS(o);
    o->cfg.verbose = level < 0 ? 0 : level;
    return ART_OK;
}

const char *ART_options_error(const ART_options *o)
{
    return o ? o->error : "";
}

int ART_options_error_code(const ART_options *o)
{
    return o ? o->error_code : ART_OK;
}


/* ---- engines ----------------------------------------------------------- */

struct ART_engine {
    ART_config cfg;
    const ART_backend_ops *ops;
    void *state;
    art_mutex mu;
    char error[512];
};

static int engine_create(ART_options *opts, ART_engine **out)
{
    ART_engine *e;
    ART_options defaults;
    const ART_options *o = opts;
    int rc;
    char err[512] = "";

    if (!out) {
        return ART_ERR_INVALID;
    }
    *out = NULL;
    if (!o) {
        memset(&defaults, 0, sizeof(defaults));
        defaults.cfg.backend = ART_BACKEND_REMOTE;
        defaults.cfg.timeout_ms = -1;
        defaults.cfg.out_fn = write_stdout;
        defaults.cfg.err_fn = write_stderr;
        o = &defaults;
    } else {
        opts->error[0] = '\0';
    }

    e = (ART_engine *)calloc(1, sizeof(ART_engine));
    if (!e) {
        return ART_ERR_NOMEM;
    }
    if ((rc = cfg_copy(&e->cfg, &o->cfg)) != ART_OK) {
        free(e);
        return rc;
    }
    if (e->cfg.backend == ART_BACKEND_REMOTE) {
        rc = ART_remote_create(&e->cfg, &e->ops, &e->state, err, sizeof(err));
    } else if (e->cfg.backend == ART_BACKEND_INPROCESS) {
        rc = ART_embed_create(&e->cfg, &e->ops, &e->state, err, sizeof(err));
    } else {
        ART_format_error(err, sizeof(err), "unknown backend %d",
                         e->cfg.backend);
        rc = ART_ERR_INVALID;
    }
    if (rc != ART_OK) {
        if (opts) {
            snprintf(opts->error, sizeof(opts->error), "%s", err);
        }
        cfg_free(&e->cfg);
        free(e);
        return rc;
    }
    art_mutex_init(&e->mu);
    *out = e;
    return ART_OK;
}

ART_engine *ART_engine_create(ART_options *opts)
{
    ART_engine *e = NULL;
    int rc = engine_create(opts, &e);
    if (opts) {
        opts->error_code = rc;
    }
    return rc == ART_OK ? e : NULL;
}

void ART_engine_destroy(ART_engine *e)
{
    if (!e) {
        return;
    }
    if (e->ops && e->ops->destroy) {
        e->ops->destroy(e->state);
    }
    art_mutex_destroy(&e->mu);
    cfg_free(&e->cfg);
    free(e);
}

const char *ART_engine_error(const ART_engine *e)
{
    return e ? e->error : "";
}

#define MAX_ARGS 100000

static int exec_locked(ART_engine *e, const char *const *argv,
                       const ART_sink *sink)
{
    int argc = 0, rc;
    char cwdbuf[4096];
    const char *cwd;

    e->error[0] = '\0';
    if (!argv || !argv[0]) {
        ART_format_error(e->error, sizeof(e->error), "no command given");
        return ART_ERR_INVALID;
    }
    while (argv[argc]) {
        if (++argc > MAX_ARGS) {
            ART_format_error(e->error, sizeof(e->error),
                             "too many arguments (or argv is not NULL-terminated)");
            return ART_ERR_INVALID;
        }
    }
    if (e->cfg.cwd) {
        cwd = e->cfg.cwd;
    } else if (art_getcwd(cwdbuf, sizeof(cwdbuf))) {
        cwd = cwdbuf;
    } else {
        cwd = "";
    }
    rc = e->ops->exec(e->state, &e->cfg, cwd, argc, argv, sink, e->error,
                      sizeof(e->error));
    if (rc == ART_ERR_COMMAND || rc == ART_OK) {
        e->error[0] = '\0';
    }
    return rc;
}

static int exec_engine(ART_engine *e, const char *const *argv,
                       const ART_sink *sink)
{
    int rc;
    if (!e) {
        return ART_ERR_INVALID;
    }
    art_mutex_lock(&e->mu);
    rc = exec_locked(e, argv, sink);
    art_mutex_unlock(&e->mu);
    return rc;
}

int ART_exec_cmd(ART_engine *e, const char *const *argv)
{
    ART_sink sink;
    if (!e) {
        return ART_ERR_INVALID;
    }
    sink.out = e->cfg.out_fn;
    sink.out_user = e->cfg.out_user;
    sink.err = e->cfg.err_fn;
    sink.err_user = e->cfg.err_user;
    return exec_engine(e, argv, &sink);
}


/* ---- collecting the output ------------------------------------------------ */

typedef struct {
    char *p;
    size_t len, cap;
} ART_buf;

struct ART_result {
    ART_buf out, err;
    int nomem;
};

static void buf_append(ART_result *r, ART_buf *b, const char *data, size_t n)
{
    if (r->nomem) {
        return;
    }
    if (b->len + n + 1 > b->cap) {
        size_t cap = b->cap ? b->cap : 256;
        char *p;
        while (cap < b->len + n + 1) {
            cap *= 2;
        }
        p = (char *)realloc(b->p, cap);
        if (!p) {
            r->nomem = 1;
            return;
        }
        b->p = p;
        b->cap = cap;
    }
    memcpy(b->p + b->len, data, n);
    b->len += n;
    b->p[b->len] = '\0';
}

static void result_out(void *user, const char *data, size_t len)
{
    ART_result *r = (ART_result *)user;
    buf_append(r, &r->out, data, len);
}

static void result_err(void *user, const char *data, size_t len)
{
    ART_result *r = (ART_result *)user;
    buf_append(r, &r->err, data, len);
}

int ART_exec_cmd_result(ART_engine *e, const char *const *argv,
                        ART_result **result)
{
    ART_sink sink;
    ART_result *r;
    int rc;

    if (!result) {
        return ART_ERR_INVALID;
    }
    *result = NULL;
    if (!e) {
        return ART_ERR_INVALID;
    }
    r = (ART_result *)calloc(1, sizeof(ART_result));
    if (!r) {
        return ART_ERR_NOMEM;
    }
    sink.out = result_out;
    sink.out_user = r;
    sink.err = result_err;
    sink.err_user = r;
    rc = exec_engine(e, argv, &sink);
    if (rc == ART_ERR_INVALID) {
        ART_result_free(r);
        return rc;
    }
    if (r->nomem) {
        ART_result_free(r);
        return ART_ERR_NOMEM;
    }
    *result = r;
    return rc;
}

const char *ART_result_out(const ART_result *r) { return r && r->out.p ? r->out.p : ""; }
size_t ART_result_out_len(const ART_result *r) { return r ? r->out.len : 0; }
const char *ART_result_err(const ART_result *r) { return r && r->err.p ? r->err.p : ""; }
size_t ART_result_err_len(const ART_result *r) { return r ? r->err.len : 0; }

void ART_result_free(ART_result *r)
{
    if (r) {
        free(r->out.p);
        free(r->err.p);
        free(r);
    }
}


/* ---- miscellaneous ---------------------------------------------------- */

const char *ART_strerror(int code)
{
    switch (code) {
    case ART_OK: return "success";
    case ART_ERR_COMMAND: return "the command failed";
    case ART_ERR_INVALID: return "invalid argument";
    case ART_ERR_CONNECT: return "cannot connect to the server";
    case ART_ERR_IO: return "connection error";
    case ART_ERR_PROTOCOL: return "protocol error";
    case ART_ERR_TIMEOUT: return "timeout";
    case ART_ERR_INIT: return "cannot initialise the engine";
    case ART_ERR_UNSUPPORTED: return "not supported";
    case ART_ERR_NOMEM: return "out of memory";
    case ART_ERR_INTERNAL: return "internal error";
    default: return "unknown error";
    }
}

const char *ART_version(void) { return ART_LIBART_VERSION; }

int ART_abi_version(void) { return ART_ABI_VERSION; }

int ART_has_backend(int backend)
{
    if (backend == ART_BACKEND_REMOTE) {
#ifdef _WIN32
        return 0;
#else
        return 1;
#endif
    }
    if (backend == ART_BACKEND_INPROCESS) {
        return ART_embed_available();
    }
    return 0;
}


#ifndef ART_LIBART_EMBED
/* the in-process backend is not built in */
int ART_embed_available(void) { return 0; }

int ART_embed_create(const ART_config *cfg, const ART_backend_ops **ops,
                     void **state, char *err, size_t errsize)
{
    (void)cfg;
    (void)ops;
    (void)state;
    ART_format_error(err, errsize,
                     "this build of libART has no in-process backend");
    return ART_ERR_UNSUPPORTED;
}
#endif
