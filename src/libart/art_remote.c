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
 *  libART: the remote backend, a client of `ART-cli --server`. The protocol
 *  is described in doc/server_mode.md; one request per connection:
 *
 *    request:  "ART1 <len>\n" + <len> bytes: cwd, a reserved flag ("0") and
 *              the command line, each terminated by a NUL byte
 *    response: frames "O <len>\n<bytes>" (stdout) and "E <len>\n<bytes>"
 *              (stderr), ended by "X <status>\n"
 */

#define _POSIX_C_SOURCE 200809L

#include "art_internal.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
/* Winsock, with AF_UNIX sockets (Windows 10 1803 and later) */
#include <winsock2.h>
#include <afunix.h>
#include <windows.h>

typedef SOCKET sock_t;
typedef int io_ssize;
#define BAD_SOCK INVALID_SOCKET
#define SOCK_CLOSE(s) closesocket(s)
#define SOCK_RECV(s, b, n) recv((s), (b), (int)(n), 0)
#define SOCK_INTR() 0 /* no EINTR on Windows */
#define SOCK_ERRSTR(buf, size) win_errstr((buf), (size), WSAGetLastError())
#define POLLFD WSAPOLLFD
#define SOCK_POLL(p, to) WSAPoll((p), 1, (to))
#else /* POSIX */
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

typedef int sock_t;
typedef ssize_t io_ssize;
#define BAD_SOCK (-1)
#define SOCK_CLOSE(s) close(s)
#define SOCK_RECV(s, b, n) read((s), (b), (n))
#define SOCK_INTR() (errno == EINTR)
#define SOCK_ERRSTR(buf, size) \
    snprintf((buf), (size), "%s", strerror(errno))
#define POLLFD struct pollfd
#define SOCK_POLL(p, to) poll((p), 1, (to))
#endif

#define MAX_REQUEST ((size_t)1 << 20)
#define MAX_FRAME ((size_t)64 << 20)

/* ---- platform helpers ------------------------------------------------------- */

#ifdef _WIN32

static long long now_ms(void) { return (long long)GetTickCount64(); }

static void win_errstr(char *buf, size_t size, int code)
{
    DWORD n = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
        (DWORD)code, 0, buf, (DWORD)size, NULL);
    if (n == 0) {
        snprintf(buf, size, "error %d", code);
        return;
    }
    while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r' ||
                     buf[n - 1] == ' ' || buf[n - 1] == '.')) {
        buf[--n] = '\0';
    }
}

#else

static long long now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

#endif

/* 1: ready, 0: timed out, -1: error. deadline < 0: none. */
static int wait_fd(sock_t fd, short events, long long deadline)
{
    for (;;) {
        POLLFD p;
        int timeout = -1, r;
        if (deadline >= 0) {
            long long left = deadline - now_ms();
            if (left <= 0) {
                return 0;
            }
            timeout = left > 1000000 ? 1000000 : (int)left;
        }
        p.fd = fd;
        p.events = events;
        p.revents = 0;
        r = SOCK_POLL(&p, timeout);
        if (r < 0 && SOCK_INTR()) {
            continue;
        }
        if (r < 0) {
            return -1;
        }
        if (r == 0) {
            if (deadline >= 0 && now_ms() >= deadline) {
                return 0;
            }
            continue;
        }
        return 1; /* readable/writable, or hang-up/error: the I/O reports it */
    }
}

/* ---- buffered reader ----------------------------------------------------- */

typedef struct {
    sock_t fd;
    long long deadline;
    char buf[4096];
    size_t pos, len;
    int timed_out;
} reader;

/* 1: ok, 0: end of stream, -1: error/timeout (r->timed_out) */
static int reader_fill(reader *r)
{
    for (;;) {
        io_ssize n;
        int w = wait_fd(r->fd, POLLIN, r->deadline);
        if (w == 0) {
            r->timed_out = 1;
            return -1;
        }
        if (w < 0) {
            return -1;
        }
        n = SOCK_RECV(r->fd, r->buf, sizeof(r->buf));
        if (n < 0 && SOCK_INTR()) {
            continue;
        }
        if (n < 0) {
            return -1;
        }
        if (n == 0) {
            return 0;
        }
        r->pos = 0;
        r->len = (size_t)n;
        return 1;
    }
}

/* read exactly n bytes: 1 ok, 0 end of stream, -1 error/timeout */
static int reader_read(reader *r, char *dst, size_t n)
{
    while (n > 0) {
        size_t k;
        if (r->pos == r->len) {
            int f = reader_fill(r);
            if (f <= 0) {
                return f;
            }
        }
        k = r->len - r->pos;
        if (k > n) {
            k = n;
        }
        memcpy(dst, r->buf + r->pos, k);
        r->pos += k;
        dst += k;
        n -= k;
    }
    return 1;
}

/* a header line (without its newline, at most 40 bytes): 1 ok, 0 end of
 * stream, -1 error/timeout, -2 too long */
static int reader_line(reader *r, char *line)
{
    size_t n = 0;
    for (;;) {
        char c;
        int f = reader_read(r, &c, 1);
        if (f <= 0) {
            return f;
        }
        if (c == '\n') {
            line[n] = '\0';
            return 1;
        }
        if (n >= 40) {
            return -2;
        }
        line[n++] = c;
    }
}

/* ---- sending ----------------------------------------------------------------- */

static int send_all(sock_t fd, const char *p, size_t n, long long deadline,
                    int *timed_out)
{
#if defined(_WIN32)
    const int flags = 0;
#elif defined(MSG_NOSIGNAL)
    const int flags = MSG_NOSIGNAL;
#else
    const int flags = 0;
#endif
    while (n > 0) {
        io_ssize r;
        int w = wait_fd(fd, POLLOUT, deadline);
        if (w == 0) {
            *timed_out = 1;
            return -1;
        }
        if (w < 0) {
            return -1;
        }
#ifdef _WIN32
        r = send(fd, p, n > 0x100000 ? 0x100000 : (int)n, flags);
#else
        r = send(fd, p, n, flags);
#endif
        if (r < 0 && SOCK_INTR()) {
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

/* ---- the socket ----------------------------------------------------------- */

/* the socket path of the server: the option, $ART_SERVER_SOCKET, or the
 * default one (see ART-cli); looked up only, nothing is created */
static int socket_path(const ART_config *cfg, char *out, size_t size)
{
    const char *env;
    const char *xdg;
    int n;

    if (cfg->socket_path && *cfg->socket_path) {
        n = snprintf(out, size, "%s", cfg->socket_path);
    } else if ((env = getenv("ART_SERVER_SOCKET")) && *env) {
        n = snprintf(out, size, "%s", env);
#ifdef _WIN32
    } else if ((xdg = getenv("TEMP")) || (xdg = getenv("TMP"))) {
        /* the same layout as the POSIX default: <temp>\ART-<user>\server.sock */
        const char *user = getenv("USERNAME");
        n = snprintf(out, size, "%s\\ART-%s\\server.sock", xdg,
                     (user && *user) ? user : "user");
    } else {
        n = snprintf(out, size, "C:\\Windows\\Temp\\ART\\server.sock");
    }
    (void)env;
    return n > 0 && (size_t)n < size;
}
#else
    } else if ((xdg = getenv("XDG_RUNTIME_DIR")) && xdg[0] == '/') {
        n = snprintf(out, size, "%s/ART/server.sock", xdg);
    } else {
        const char *tmp = getenv("TMPDIR");
        const char *user = getenv("USER");
        char tmpbuf[1024], userbuf[64];
        size_t l;
        snprintf(tmpbuf, sizeof(tmpbuf), "%s", (tmp && *tmp) ? tmp : "/tmp");
        l = strlen(tmpbuf);
        while (l > 1 && tmpbuf[l - 1] == '/') {
            tmpbuf[--l] = '\0';
        }
        if (user && *user) {
            snprintf(userbuf, sizeof(userbuf), "%s", user);
        } else {
            snprintf(userbuf, sizeof(userbuf), "%u", (unsigned)geteuid());
        }
        n = snprintf(out, size, "%s/ART-%s/server.sock", tmpbuf, userbuf);
    }
    return n > 0 && (size_t)n < size;
}
#endif

typedef struct {
    int unused;
} remote_state;

static int remote_exec(void *state, const ART_config *cfg, const char *cwd,
                       int argc, const char *const *argv, const ART_sink *sink,
                       char *err, size_t errsize)
{
    char path[1024];
    struct sockaddr_un addr;
    sock_t fd = BAD_SOCK;
    char msg[256];
    int rc = ART_ERR_INTERNAL, i, timed_out = 0;
    long long deadline = cfg->timeout_ms >= 0 ? now_ms() + cfg->timeout_ms : -1;
    size_t paylen = strlen(cwd) + 1 + 2, off = 0;
    char *req = NULL;
    char head[40];
    int headlen;
    reader rd;

    (void)state;

    if (!socket_path(cfg, path, sizeof(path))) {
        ART_format_error(err, errsize, "the socket path is empty or too long");
        return ART_ERR_INVALID;
    }
    if (strlen(path) >= sizeof(addr.sun_path)) {
        ART_format_error(err, errsize,
                         "socket path too long (at most %d bytes): %s",
                         (int)sizeof(addr.sun_path) - 1, path);
        return ART_ERR_INVALID;
    }

    for (i = 0; i < argc; ++i) {
        paylen += strlen(argv[i]) + 1;
        if (paylen > MAX_REQUEST) {
            ART_format_error(err, errsize, "the command is too long");
            return ART_ERR_INVALID;
        }
    }
    /* header + payload in one buffer */
    headlen = snprintf(head, sizeof(head), "ART1 %zu\n", paylen);
    req = (char *)malloc((size_t)headlen + paylen);
    if (!req) {
        return ART_ERR_NOMEM;
    }
    memcpy(req, head, (size_t)headlen);
    off = (size_t)headlen;
    memcpy(req + off, cwd, strlen(cwd) + 1);
    off += strlen(cwd) + 1;
    req[off++] = '0'; /* reserved */
    req[off++] = '\0';
    for (i = 0; i < argc; ++i) {
        size_t l = strlen(argv[i]) + 1;
        memcpy(req + off, argv[i], l);
        off += l;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    memcpy(addr.sun_path, path, strlen(path));
    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == BAD_SOCK) {
        SOCK_ERRSTR(msg, sizeof(msg));
        ART_format_error(err, errsize, "cannot create a socket: %s", msg);
        free(req);
        return ART_ERR_INTERNAL;
    }
#ifdef _WIN32
    SetHandleInformation((HANDLE)fd, HANDLE_FLAG_INHERIT, 0);
#else
#ifdef SO_NOSIGPIPE
    {
        int one = 1;
        setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
    }
#endif
    {
        int fl = fcntl(fd, F_GETFD);
        if (fl >= 0) {
            fcntl(fd, F_SETFD, fl | FD_CLOEXEC);
        }
    }
#endif
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        SOCK_ERRSTR(msg, sizeof(msg));
        ART_format_error(err, errsize, "no running server (%s: %s)", path, msg);
        SOCK_CLOSE(fd);
        free(req);
        return ART_ERR_CONNECT;
    }

    if (send_all(fd, req, off, deadline, &timed_out) != 0) {
        if (timed_out) {
            ART_format_error(err, errsize, "timeout sending the command");
            rc = ART_ERR_TIMEOUT;
        } else {
            ART_format_error(err, errsize,
                             "cannot send the command to the server");
            rc = ART_ERR_IO;
        }
        goto done;
    }

    memset(&rd, 0, sizeof(rd));
    rd.fd = fd;
    rd.deadline = deadline;
    for (;;) {
        char line[48];
        int f = reader_line(&rd, line);
        char kind;
        size_t n;
        const char *p;

        if (f == -1 && rd.timed_out) {
            ART_format_error(err, errsize,
                             "no answer from the server within %d ms",
                             cfg->timeout_ms);
            rc = ART_ERR_TIMEOUT;
            goto done;
        }
        if (f <= 0 && f != -2) {
            ART_format_error(err, errsize,
                             "the connection to the server was lost");
            rc = ART_ERR_IO;
            goto done;
        }
        kind = line[0];
        p = line + 1;
        if (f == -2 || (kind != 'O' && kind != 'E' && kind != 'X')) {
            ART_format_error(err, errsize, "protocol error: bad frame header");
            rc = ART_ERR_PROTOCOL;
            goto done;
        }
        /* "<kind> <digits>" */
        if (*p != ' ' || !p[1] || strlen(p + 1) > 12 ||
            strspn(p + 1, "0123456789") != strlen(p + 1)) {
            ART_format_error(err, errsize, "protocol error: bad frame header");
            rc = ART_ERR_PROTOCOL;
            goto done;
        }
        n = (size_t)strtoull(p + 1, NULL, 10);
        if (kind == 'X') {
            rc = n == 0 ? ART_OK : ART_ERR_COMMAND;
            goto done;
        }
        if (n > MAX_FRAME) {
            ART_format_error(err, errsize, "protocol error: frame too large");
            rc = ART_ERR_PROTOCOL;
            goto done;
        }
        if (n > 0) {
            char *data = (char *)malloc(n + 1);
            int g;
            if (!data) {
                rc = ART_ERR_NOMEM;
                goto done;
            }
            g = reader_read(&rd, data, n);
            if (g != 1) {
                free(data);
                if (g == -1 && rd.timed_out) {
                    ART_format_error(err, errsize,
                                     "no answer from the server within %d ms",
                                     cfg->timeout_ms);
                    rc = ART_ERR_TIMEOUT;
                } else {
                    ART_format_error(err, errsize,
                                     "the connection to the server was lost");
                    rc = ART_ERR_IO;
                }
                goto done;
            }
            data[n] = '\0';
            if (kind == 'O' && sink->out) {
                sink->out(sink->out_user, data, n);
            } else if (kind == 'E' && sink->err) {
                sink->err(sink->err_user, data, n);
            }
            free(data);
        }
    }

done:
    SOCK_CLOSE(fd);
    free(req);
    return rc;
}

static void remote_destroy(void *state)
{
#ifdef _WIN32
    if (state) {
        WSACleanup(); /* pairs with the WSAStartup in ART_remote_create */
    }
#endif
    free(state);
}

static const ART_backend_ops remote_ops = {remote_exec, remote_destroy};

int ART_remote_create(const ART_config *cfg, const ART_backend_ops **ops,
                      void **state, char *err, size_t errsize)
{
    remote_state *s;
    (void)cfg;
#ifdef _WIN32
    {
        WSADATA wsa;
        int rc = WSAStartup(MAKEWORD(2, 2), &wsa);
        if (rc != 0) {
            ART_format_error(err, errsize, "cannot initialize Winsock (%d)", rc);
            return ART_ERR_INTERNAL;
        }
    }
#else
    (void)err;
    (void)errsize;
#endif
    s = (remote_state *)calloc(1, sizeof(*s));
    if (!s) {
#ifdef _WIN32
        WSACleanup();
#endif
        return ART_ERR_NOMEM;
    }
    *ops = &remote_ops;
    *state = s;
    return ART_OK;
}
