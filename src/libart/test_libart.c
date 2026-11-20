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
 *  Tests of libART's remote backend against fake servers (no ART needed):
 *  ctest -R libart
 */

#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE 1
#define _DARWIN_C_SOURCE 1

#include "ART.h"
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

static int failures = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            ++failures;                                                      \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, \
                    #cond);                                                  \
        }                                                                    \
    } while (0)

static char tmpdir[64];

static void sockpath(char *out, size_t n, const char *name)
{
    snprintf(out, n, "%s/%s", tmpdir, name);
}

/* ---- a fake server ------------------------------------------------------ */

typedef struct {
    int cwd_len;
    char *cwd;
    int mark;
    int nargs;
    char **args;
} request;

static int read_full(int fd, void *buf, size_t n)
{
    char *p = (char *)buf;
    while (n > 0) {
        ssize_t r = read(fd, p, n);
        if (r < 0 && errno == EINTR) {
            continue;
        }
        if (r <= 0) {
            return -1;
        }
        p += r;
        n -= (size_t)r;
    }
    return 0;
}

static void write_full(int fd, const char *p, size_t n)
{
    while (n > 0) {
        ssize_t r = write(fd, p, n);
        if (r < 0 && errno == EINTR) {
            continue;
        }
        if (r <= 0) {
            return;
        }
        p += r;
        n -= (size_t)r;
    }
}

static void put(int fd, const char *s) { write_full(fd, s, strlen(s)); }

static void put_frame(int fd, char kind, const char *data, size_t n)
{
    char head[32];
    snprintf(head, sizeof(head), "%c %zu\n", kind, n);
    put(fd, head);
    write_full(fd, data, n);
}

/* parse "ART1 <len>\n" + payload; returns 0 on success */
static int read_request(int fd, request *rq)
{
    char line[48];
    size_t n = 0, len, off = 0;
    char *payload;
    memset(rq, 0, sizeof(*rq));
    for (;;) {
        char c;
        if (read_full(fd, &c, 1) != 0 || n >= sizeof(line) - 1) {
            return -1;
        }
        if (c == '\n') {
            break;
        }
        line[n++] = c;
    }
    line[n] = '\0';
    if (strncmp(line, "ART1 ", 5) != 0) {
        return -1;
    }
    len = (size_t)strtoull(line + 5, NULL, 10);
    payload = (char *)malloc(len + 1);
    if (read_full(fd, payload, len) != 0) {
        free(payload);
        return -1;
    }
    payload[len] = '\0';
    /* fields: cwd, mark, args... */
    rq->cwd = strdup(payload + off);
    off += strlen(payload + off) + 1;
    rq->mark = payload[off] == '1';
    off += strlen(payload + off) + 1;
    rq->args = (char **)calloc(len + 1, sizeof(char *));
    while (off < len) {
        rq->args[rq->nargs++] = strdup(payload + off);
        off += strlen(payload + off) + 1;
    }
    free(payload);
    return 0;
}

static void free_request(request *rq)
{
    int i;
    free(rq->cwd);
    for (i = 0; i < rq->nargs; ++i) {
        free(rq->args[i]);
    }
    free(rq->args);
}

typedef void (*handler_fn)(int cfd, void *arg);

typedef struct {
    int lfd;
    char path[128];
    handler_fn handler;
    void *arg;
    int nconn;
    pthread_t th;
} fake_server;

static void *fake_main(void *p)
{
    fake_server *s = (fake_server *)p;
    int i;
    for (i = 0; i < s->nconn; ++i) {
        int c = accept(s->lfd, NULL, NULL);
        if (c < 0) {
            break;
        }
        s->handler(c, s->arg);
        close(c);
    }
    return NULL;
}

static int fake_start(fake_server *s, const char *name, handler_fn h,
                      void *arg, int nconn)
{
    struct sockaddr_un a;
    memset(s, 0, sizeof(*s));
    sockpath(s->path, sizeof(s->path), name);
    unlink(s->path);
    s->lfd = socket(AF_UNIX, SOCK_STREAM, 0);
    memset(&a, 0, sizeof(a));
    a.sun_family = AF_UNIX;
    strcpy(a.sun_path, s->path);
    if (bind(s->lfd, (struct sockaddr *)&a, sizeof(a)) != 0 ||
        listen(s->lfd, 8) != 0) {
        perror("fake server");
        return -1;
    }
    s->handler = h;
    s->arg = arg;
    s->nconn = nconn;
    pthread_create(&s->th, NULL, fake_main, s);
    return 0;
}

static void fake_stop(fake_server *s)
{
    pthread_join(s->th, NULL);
    close(s->lfd);
    unlink(s->path);
}

/* ---- the handlers ---------------------------------------------------------- */

/* echoes the request: "cwd=..;mark=..;args=a|b|c" on stdout, "warn" on stderr */
static void h_echo(int fd, void *arg)
{
    request rq;
    char buf[4096];
    int i;
    size_t n;
    (void)arg;
    if (read_request(fd, &rq) != 0) {
        return;
    }
    n = (size_t)snprintf(buf, sizeof(buf), "cwd=%s;mark=%d;n=%d;args=", rq.cwd,
                         rq.mark, rq.nargs);
    for (i = 0; i < rq.nargs; ++i) {
        n += (size_t)snprintf(buf + n, sizeof(buf) - n, "%s%s", i ? "|" : "",
                              rq.args[i]);
    }
    put_frame(fd, 'O', buf, n);
    put_frame(fd, 'E', "warn\n", 5);
    put(fd, "X 0\n");
    free_request(&rq);
}

static void h_fail(int fd, void *arg)
{
    request rq;
    (void)arg;
    if (read_request(fd, &rq) == 0) {
        put_frame(fd, 'E', "ART: error: boom\n", 17);
        put(fd, "X 1\n");
        free_request(&rq);
    }
}

static void h_partial(int fd, void *arg)
{
    request rq;
    (void)arg;
    if (read_request(fd, &rq) == 0) {
        put(fd, "O 10\nabc"); /* then the connection closes */
        free_request(&rq);
    }
}

static void h_nostatus(int fd, void *arg)
{
    request rq;
    (void)arg;
    if (read_request(fd, &rq) == 0) {
        put_frame(fd, 'O', "all\n", 4); /* but no X frame */
        free_request(&rq);
    }
}

static void h_garbage(int fd, void *arg)
{
    request rq;
    (void)arg;
    if (read_request(fd, &rq) == 0) {
        put(fd, "HELLO WORLD\n");
        free_request(&rq);
    }
}

static void h_bigframe(int fd, void *arg)
{
    request rq;
    (void)arg;
    if (read_request(fd, &rq) == 0) {
        put(fd, "O 999999999999\n");
        free_request(&rq);
    }
}

static void h_badnumber(int fd, void *arg)
{
    request rq;
    (void)arg;
    if (read_request(fd, &rq) == 0) {
        put(fd, "O 12x\n");
        free_request(&rq);
    }
}

static void h_slow(int fd, void *arg)
{
    request rq;
    (void)arg;
    if (read_request(fd, &rq) == 0) {
        usleep(1500 * 1000);
        put(fd, "X 0\n");
        free_request(&rq);
    }
}

static void h_silent_close(int fd, void *arg)
{
    (void)fd;
    (void)arg; /* closes without reading anything */
}

#define MANY 500
static void h_many(int fd, void *arg)
{
    request rq;
    int i;
    char *big = (char *)malloc(1 << 20); /* too big for a thread's stack */
    (void)arg;
    if (!big || read_request(fd, &rq) != 0) {
        free(big);
        return;
    }
    for (i = 0; i < MANY; ++i) {
        char s[16];
        int n = snprintf(s, sizeof(s), "%d\n", i);
        put_frame(fd, 'O', s, (size_t)n);
    }
    memset(big, 'x', (size_t)1 << 20);
    put_frame(fd, 'O', big, (size_t)1 << 20);
    put(fd, "X 0\n");
    free(big);
    free_request(&rq);
}

/* ---- output collection ----------------------------------------------------- */

typedef struct {
    char text[(1 << 20) + 16384];
    size_t len;
    int calls;
} collector;

static void collect(void *user, const char *data, size_t len)
{
    collector *c = (collector *)user;
    /* the library promises a NUL after the data */
    if (data[len] != '\0') {
        ++failures;
        fprintf(stderr, "callback data not NUL-terminated\n");
    }
    if (c->len + len < sizeof(c->text)) {
        memcpy(c->text + c->len, data, len);
        c->len += len;
        c->text[c->len] = '\0';
    }
    c->calls++;
}

static ART_engine *make_engine(const char *path, collector *out, collector *err,
                               int timeout_ms)
{
    ART_options *o = ART_options_new();
    ART_engine *e = NULL;
    CHECK(o != NULL);
    CHECK(ART_options_set_remote(o, path) == ART_OK);
    if (out) {
        CHECK(ART_options_set_output(o, collect, out) == ART_OK);
    }
    if (err) {
        CHECK(ART_options_set_error(o, collect, err) == ART_OK);
    }
    if (timeout_ms >= 0) {
        CHECK(ART_options_set_timeout(o, timeout_ms) == ART_OK);
    }
    e = ART_engine_create(o);
    CHECK(e != NULL);
    ART_options_free(o);
    return e;
}

/* ---- the tests --------------------------------------------------------------- */

static collector *new_collector(void)
{
    return (collector *)calloc(1, sizeof(collector));
}

static void test_roundtrip_and_args(void)
{
    fake_server s;
    collector *out = new_collector(), *err = new_collector();
    const char *argv[] = {"snapshot-add", "name with spaces", "--odd", "",
                          "line\nbreak", "caf\xc3\xa9", NULL};
    ART_engine *e;

    CHECK(fake_start(&s, "a.sock", h_echo, NULL, 2) == 0);
    e = make_engine(s.path, out, err, -1);
    CHECK(e != NULL);
    CHECK(ART_exec_cmd(e, argv) == ART_OK);
    CHECK(strcmp(ART_engine_error(e), "") == 0);
    /* arguments arrive exactly, and no \x01 marking is requested */
    CHECK(strstr(out->text, ";mark=0;n=6;args=snapshot-add|name with spaces|"
                            "--odd||line\nbreak|caf\xc3\xa9") != NULL);
    CHECK(strcmp(err->text, "warn\n") == 0);

    /* the working directory: that of the process by default ... */
    {
        char cwd[1024];
        char want[1100];
        const char *argv2[] = {"ping", NULL};
        CHECK(getcwd(cwd, sizeof(cwd)) != NULL);
        snprintf(want, sizeof(want), "cwd=%s;", cwd);
        out->len = 0;
        out->text[0] = '\0';
        CHECK(ART_exec_cmd(e, argv2) == ART_OK);
        CHECK(strncmp(out->text, want, strlen(want)) == 0);
    }
    ART_engine_destroy(e);
    fake_stop(&s);
    free(out);
    free(err);
}

static void test_cwd_option(void)
{
    fake_server s;
    collector *out = new_collector();
    ART_options *o = ART_options_new();
    ART_engine *e = NULL;
    const char *argv[] = {"open", "photo.arw", NULL};

    CHECK(fake_start(&s, "cwd.sock", h_echo, NULL, 1) == 0);
    ART_options_set_remote(o, s.path);
    ART_options_set_output(o, collect, out);
    ART_options_set_error(o, collect, new_collector());
    CHECK(ART_options_set_cwd(o, "/some/where") == ART_OK);
    e = ART_engine_create(o);
    CHECK(e != NULL);
    ART_options_free(o);
    CHECK(ART_exec_cmd(e, argv) == ART_OK);
    CHECK(strncmp(out->text, "cwd=/some/where;", 16) == 0);
    ART_engine_destroy(e);
    fake_stop(&s);
    free(out);
}

static void test_command_failure(void)
{
    fake_server s;
    collector *out = new_collector(), *err = new_collector();
    const char *argv[] = {"bogus", NULL};
    ART_engine *e;

    CHECK(fake_start(&s, "f.sock", h_fail, NULL, 1) == 0);
    e = make_engine(s.path, out, err, -1);
    CHECK(ART_exec_cmd(e, argv) == ART_ERR_COMMAND);
    CHECK(strcmp(err->text, "ART: error: boom\n") == 0);
    CHECK(out->len == 0);
    /* a command failure is not a library error */
    CHECK(strcmp(ART_engine_error(e), "") == 0);
    ART_engine_destroy(e);
    fake_stop(&s);
    free(out);
    free(err);
}

static void test_result_collection(void)
{
    fake_server s;
    const char *argv[] = {"get-info", NULL};
    ART_engine *e;
    ART_result *r = NULL;
    collector *out = new_collector();

    CHECK(fake_start(&s, "r.sock", h_echo, NULL, 2) == 0);
    /* the stream callbacks are not used for a result */
    e = make_engine(s.path, out, NULL, -1);
    CHECK(ART_exec_cmd_result(e, argv, &r) == ART_OK);
    CHECK(r != NULL);
    CHECK(strstr(ART_result_out(r), "args=get-info") != NULL);
    CHECK(ART_result_out_len(r) == strlen(ART_result_out(r)));
    CHECK(strcmp(ART_result_err(r), "warn\n") == 0);
    CHECK(ART_result_err_len(r) == 5);
    CHECK(out->calls == 0);
    ART_result_free(r);

    /* the same command streamed gives the same text */
    CHECK(ART_exec_cmd(e, argv) == ART_OK);
    CHECK(out->calls == 1 && strstr(out->text, "args=get-info") != NULL);
    ART_engine_destroy(e);
    fake_stop(&s);
    free(out);

    /* accessors tolerate NULL */
    CHECK(strcmp(ART_result_out(NULL), "") == 0 && ART_result_err_len(NULL) == 0);
    ART_result_free(NULL);
}

static void test_failures(void)
{
    const char *argv[] = {"ping", NULL};
    char path[128];
    ART_engine *e;
    fake_server s;
    ART_result *r = NULL;
    int rc;

    /* nobody is there */
    sockpath(path, sizeof(path), "nothing.sock");
    e = make_engine(path, NULL, NULL, -1);
    CHECK(ART_exec_cmd(e, argv) == ART_ERR_CONNECT);
    CHECK(strstr(ART_engine_error(e), "no running server") != NULL);
    CHECK(strstr(ART_engine_error(e), "nothing.sock") != NULL);
    ART_engine_destroy(e);

    /* a path that does not fit */
    {
        char longp[400];
        memset(longp, 'x', sizeof(longp) - 1);
        longp[0] = '/';
        longp[sizeof(longp) - 1] = '\0';
        e = make_engine(longp, NULL, NULL, -1);
        CHECK(ART_exec_cmd(e, argv) == ART_ERR_INVALID);
        CHECK(strstr(ART_engine_error(e), "too long") != NULL);
        ART_engine_destroy(e);
    }

    /* the connection breaks in the middle of a frame / before the status */
    {
        handler_fn hs[] = {h_partial, h_nostatus, h_silent_close};
        int i;
        for (i = 0; i < 3; ++i) {
            collector *out = new_collector();
            CHECK(fake_start(&s, "io.sock", hs[i], NULL, 1) == 0);
            e = make_engine(s.path, out, NULL, -1);
            CHECK(ART_exec_cmd(e, argv) == ART_ERR_IO);
            CHECK(strstr(ART_engine_error(e), "lost") != NULL);
            ART_engine_destroy(e);
            fake_stop(&s);
            free(out);
        }
    }

    /* what was received before the break is still reported in a result */
    CHECK(fake_start(&s, "io2.sock", h_nostatus, NULL, 1) == 0);
    e = make_engine(s.path, NULL, NULL, -1);
    rc = ART_exec_cmd_result(e, argv, &r);
    CHECK(rc == ART_ERR_IO && r != NULL);
    CHECK(strcmp(ART_result_out(r), "all\n") == 0);
    ART_result_free(r);
    ART_engine_destroy(e);
    fake_stop(&s);

    /* a server that does not speak the protocol */
    {
        handler_fn hs[] = {h_garbage, h_bigframe, h_badnumber};
        int i;
        for (i = 0; i < 3; ++i) {
            CHECK(fake_start(&s, "p.sock", hs[i], NULL, 1) == 0);
            e = make_engine(s.path, NULL, NULL, -1);
            CHECK(ART_exec_cmd(e, argv) == ART_ERR_PROTOCOL);
            CHECK(strstr(ART_engine_error(e), "protocol") != NULL);
            ART_engine_destroy(e);
            fake_stop(&s);
        }
    }

    /* the error text goes away with the next successful call */
    CHECK(fake_start(&s, "e.sock", h_silent_close, NULL, 1) == 0);
    e = make_engine(s.path, NULL, NULL, -1);
    CHECK(ART_exec_cmd(e, argv) == ART_ERR_IO);
    CHECK(strcmp(ART_engine_error(e), "") != 0);
    fake_stop(&s);
    CHECK(fake_start(&s, "e.sock", h_echo, NULL, 1) == 0);
    {
        collector *o2 = new_collector();
        ART_options *op = ART_options_new();
        ART_engine *e2 = NULL;
        ART_options_set_remote(op, s.path);
        ART_options_set_output(op, collect, o2);
        ART_options_set_error(op, collect, new_collector());
        e2 = ART_engine_create(op);
        CHECK(e2 != NULL);
        CHECK(ART_exec_cmd(e2, argv) == ART_OK);
        CHECK(strcmp(ART_engine_error(e2), "") == 0);
        ART_engine_destroy(e2);
        ART_options_free(op);
        free(o2);
    }
    ART_engine_destroy(e);
    fake_stop(&s);
}

static void test_timeout(void)
{
    fake_server s;
    const char *argv[] = {"open", "x", NULL};
    ART_engine *e;

    CHECK(fake_start(&s, "t.sock", h_slow, NULL, 1) == 0);
    e = make_engine(s.path, NULL, NULL, 200);
    CHECK(ART_exec_cmd(e, argv) == ART_ERR_TIMEOUT);
    CHECK(strstr(ART_engine_error(e), "200 ms") != NULL);
    ART_engine_destroy(e);
    fake_stop(&s);
}

static void test_streaming(void)
{
    fake_server s;
    collector *out = new_collector();
    const char *argv[] = {"get-history", NULL};
    ART_engine *e;
    char expect[4096];
    size_t n = 0;
    int i;

    CHECK(fake_start(&s, "m.sock", h_many, NULL, 1) == 0);
    e = make_engine(s.path, out, NULL, -1);
    CHECK(ART_exec_cmd(e, argv) == ART_OK);
    CHECK(out->calls == MANY + 1); /* one call per frame, in order */
    for (i = 0; i < 5; ++i) {
        n += (size_t)snprintf(expect + n, sizeof(expect) - n, "%d\n", i);
    }
    CHECK(strncmp(out->text, expect, n) == 0);
    CHECK(out->len > (size_t)(1 << 20)); /* the big frame arrived whole */
    ART_engine_destroy(e);
    fake_stop(&s);
    free(out);
}

static void test_invalid_arguments(void)
{
    ART_engine *e;
    ART_result *r = (ART_result *)1;
    const char *empty[] = {NULL};
    const char *ok[] = {"ping", NULL};
    char path[128];

    sockpath(path, sizeof(path), "unused.sock");
    e = make_engine(path, NULL, NULL, -1);
    CHECK(ART_exec_cmd(NULL, ok) == ART_ERR_INVALID);
    CHECK(ART_exec_cmd(e, NULL) == ART_ERR_INVALID);
    CHECK(strstr(ART_engine_error(e), "no command") != NULL);
    CHECK(ART_exec_cmd(e, empty) == ART_ERR_INVALID);
    CHECK(ART_exec_cmd_result(e, NULL, &r) == ART_ERR_INVALID && r == NULL);
    CHECK(ART_exec_cmd_result(NULL, ok, &r) == ART_ERR_INVALID);
    CHECK(ART_exec_cmd_result(e, ok, NULL) == ART_ERR_INVALID);
    ART_engine_destroy(e);
    ART_engine_destroy(NULL);
    CHECK(strcmp(ART_engine_error(NULL), "") == 0);

    /* options */
    CHECK(ART_options_set_remote(NULL, "x") == ART_ERR_INVALID);
    {
        ART_options *o = ART_options_new();
        CHECK(ART_options_set_timeout(o, -2) == ART_ERR_INVALID);
        CHECK(ART_options_set_output_fd(o, -1) == ART_ERR_INVALID);
        CHECK(ART_options_set_timeout(o, -1) == ART_OK);
        CHECK(ART_options_set_remote(o, NULL) == ART_OK);
        ART_options_free(o);
        ART_options_free(NULL);
    }
    {
        ART_engine *none = ART_engine_create(NULL); /* the defaults */
        CHECK(none != NULL);
        ART_engine_destroy(none);
    }
}

static void test_file_and_fd_streams(void)
{
    fake_server s;
    ART_options *o = ART_options_new();
    ART_engine *e = NULL;
    const char *argv[] = {"ping", NULL};
    FILE *fo = tmpfile();
    int pfd[2];
    char buf[256];
    ssize_t n;

    CHECK(fake_start(&s, "s.sock", h_echo, NULL, 1) == 0);
    CHECK(fo != NULL && pipe(pfd) == 0);
    ART_options_set_remote(o, s.path);
    CHECK(ART_options_set_output_file(o, fo) == ART_OK);
    CHECK(ART_options_set_error_fd(o, pfd[1]) == ART_OK);
    e = ART_engine_create(o);
    CHECK(e != NULL);
    ART_options_free(o);
    CHECK(ART_exec_cmd(e, argv) == ART_OK);
    ART_engine_destroy(e);
    fake_stop(&s);

    rewind(fo);
    n = (ssize_t)fread(buf, 1, sizeof(buf) - 1, fo);
    buf[n > 0 ? n : 0] = '\0';
    CHECK(strstr(buf, "args=ping") != NULL);
    fclose(fo);
    n = read(pfd[0], buf, sizeof(buf) - 1);
    buf[n > 0 ? n : 0] = '\0';
    CHECK(strcmp(buf, "warn\n") == 0);
    close(pfd[0]);
    close(pfd[1]);
}

static void test_default_socket_path(void)
{
    fake_server s;
    ART_engine *e = NULL;
    collector *out = new_collector();
    ART_options *o = ART_options_new();
    const char *argv[] = {"ping", NULL};

    /* $ART_SERVER_SOCKET, then $XDG_RUNTIME_DIR/ART/server.sock */
    CHECK(fake_start(&s, "env.sock", h_echo, NULL, 1) == 0);
    setenv("ART_SERVER_SOCKET", s.path, 1);
    ART_options_set_output(o, collect, out);
    ART_options_set_error(o, collect, new_collector());
    e = ART_engine_create(o);
    CHECK(e != NULL);
    CHECK(ART_exec_cmd(e, argv) == ART_OK);
    ART_engine_destroy(e);
    fake_stop(&s);
    unsetenv("ART_SERVER_SOCKET");
    CHECK(strstr(out->text, "args=ping") != NULL);

    {
        char dir[160], sock[200];
        snprintf(dir, sizeof(dir), "%s/ART", tmpdir);
        CHECK(mkdir(dir, 0700) == 0);
        snprintf(sock, sizeof(sock), "%s/server.sock", dir);
        setenv("XDG_RUNTIME_DIR", tmpdir, 1);
        CHECK(fake_start(&s, "ART/server.sock", h_echo, NULL, 1) == 0);
        out->len = 0;
        out->text[0] = '\0';
        e = ART_engine_create(o);
    CHECK(e != NULL);
        CHECK(ART_exec_cmd(e, argv) == ART_OK);
        CHECK(strstr(out->text, "args=ping") != NULL);
        ART_engine_destroy(e);
        fake_stop(&s);
        unsetenv("XDG_RUNTIME_DIR");
        rmdir(dir);
    }
    ART_options_free(o);
    free(out);
}

static void test_misc(void)
{
    ART_options *o;
    ART_engine *e = NULL;

    CHECK(ART_abi_version() == ART_ABI_VERSION);
    CHECK(strlen(ART_version()) > 0);
    CHECK(ART_has_backend(ART_BACKEND_REMOTE) == 1);
    CHECK(ART_has_backend(12345) == 0);
    CHECK(strcmp(ART_strerror(ART_OK), "success") == 0);
    CHECK(strlen(ART_strerror(ART_ERR_TIMEOUT)) > 0);
    CHECK(strlen(ART_strerror(999)) > 0);

    /* the in-process backend is not built in the test library */
    if (!ART_has_backend(ART_BACKEND_INPROCESS)) {
        o = ART_options_new();
        CHECK(ART_options_set_inprocess(o, NULL, NULL, NULL) == ART_OK);
        e = ART_engine_create(o);
        CHECK(e == NULL);
        CHECK(ART_options_error_code(o) == ART_ERR_UNSUPPORTED);
        CHECK(strstr(ART_options_error(o), "in-process") != NULL);
        /* a later success clears the error */
        CHECK(ART_options_set_remote(o, "/tmp/x.sock") == ART_OK);
        e = ART_engine_create(o);
        CHECK(e != NULL && ART_options_error_code(o) == ART_OK &&
              strcmp(ART_options_error(o), "") == 0);
        ART_engine_destroy(e);
        e = NULL;
        ART_options_free(o);
    }
}

typedef struct {
    ART_engine *e;
    int ok;
} worker;

static void *worker_main(void *p)
{
    worker *w = (worker *)p;
    const char *argv[] = {"ping", NULL};
    int i;
    for (i = 0; i < 20; ++i) {
        if (ART_exec_cmd(w->e, argv) != ART_OK) {
            w->ok = 0;
        }
    }
    return NULL;
}

static void test_threads(void)
{
    /* calls on one engine from several threads are serialised */
    fake_server s;
    ART_engine *e;
    pthread_t th[4];
    worker w[4];
    collector *out = new_collector();
    int i;

    CHECK(fake_start(&s, "th.sock", h_echo, NULL, 80) == 0);
    e = make_engine(s.path, out, new_collector(), -1);
    for (i = 0; i < 4; ++i) {
        w[i].e = e;
        w[i].ok = 1;
        pthread_create(&th[i], NULL, worker_main, &w[i]);
    }
    for (i = 0; i < 4; ++i) {
        pthread_join(th[i], NULL);
        CHECK(w[i].ok);
    }
    CHECK(out->calls == 80);
    ART_engine_destroy(e);
    fake_stop(&s);
    free(out);
}

int main(void)
{
    char tmpl[] = "/tmp/art-lib-XXXXXX";
    char cmd[128];

    /* the fake servers write to clients that have gone */
    signal(SIGPIPE, SIG_IGN);
    if (!mkdtemp(tmpl)) {
        perror("mkdtemp");
        return 2;
    }
    snprintf(tmpdir, sizeof(tmpdir), "%s", tmpl);

    test_roundtrip_and_args();
    test_cwd_option();
    test_command_failure();
    test_result_collection();
    test_failures();
    test_timeout();
    test_streaming();
    test_invalid_arguments();
    test_file_and_fd_streams();
    test_default_socket_path();
    test_misc();
    test_threads();

    snprintf(cmd, sizeof(cmd), "rm -rf '%s'", tmpdir);
    if (system(cmd) != 0) {
        fprintf(stderr, "could not remove %s\n", tmpdir);
    }
    if (failures) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    printf("libART: all checks passed\n");
    return 0;
}
