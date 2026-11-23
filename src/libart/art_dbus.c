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
 *  libART: the GUI backend, a client of a running ART GUI, i.e. what
 *  `ART --cmd ...` does, without GLib: a minimal D-Bus client (the wire
 *  protocol over a Unix socket, EXTERNAL authentication) that makes the
 *  `CommandLine` call of GApplication's `org.gtk.Application` interface on
 *  the name `us.pixls.art.ART`.
 *
 *  GApplication runs a remote command line like this: the caller passes the
 *  path of an object it exports, `org.gtk.private.CommandLine`; the primary
 *  instance calls `Print(s)` / `PrintError(s)` on it for the output of the
 *  command, then replies to `CommandLine` with the exit status. The GUI
 *  tells a command from a plain command line by `ART_IS_CMD=1` in the
 *  environment of the platform data, and refuses to start a new instance
 *  for one when `ART_CMD_START=0` (see RTApplication::on_command_line).
 *
 *  Only what that needs is implemented: no file descriptor passing, no
 *  signals, no name registration. POSIX only.
 */

#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE 1
#define _DARWIN_C_SOURCE 1

#include "art_internal.h"
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32

int ART_dbus_create(const ART_config *cfg, const ART_backend_ops **ops,
                    void **state, char *err, size_t errsize)
{
    (void)cfg;
    (void)ops;
    (void)state;
    ART_format_error(err, errsize,
                     "the GUI backend is not available on this platform");
    return ART_ERR_UNSUPPORTED;
}

int ART_dbus_available(void) { return 0; }

#else /* POSIX */

#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#define BUS_NAME "us.pixls.art.ART"
#define OBJECT_PATH "/us/pixls/art/ART"
#define APP_IFACE "org.gtk.Application"
/* the object we say we export (the primary calls Print() on it) */
#define CALLBACK_PATH "/org/gtk/Application/CommandLine"

#define MAX_MESSAGE ((size_t)64 << 20) /* the D-Bus limit is 128 MiB */
#define MAX_REQUEST ((size_t)1 << 20)

/* message types and flags */
enum { MSG_CALL = 1, MSG_RETURN = 2, MSG_ERROR = 3, MSG_SIGNAL = 4 };
enum { FLAG_NO_REPLY = 0x1, FLAG_NO_AUTO_START = 0x2 };

/* ---- time and I/O ----------------------------------------------------------- */

static long long now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/* 1: ready, 0: timed out, -1: error. deadline < 0: none. */
static int wait_fd(int fd, short events, long long deadline)
{
    for (;;) {
        struct pollfd p;
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
        r = poll(&p, 1, timeout);
        if (r < 0 && errno == EINTR) {
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
        return 1;
    }
}

typedef struct {
    int fd;
    long long deadline;
    unsigned char buf[8192];
    size_t pos, len;
    int timed_out;
} conn;

/* 1: ok, 0: end of stream, -1: error or timeout (c->timed_out) */
static int conn_fill(conn *c)
{
    for (;;) {
        ssize_t n;
        int w = wait_fd(c->fd, POLLIN, c->deadline);
        if (w == 0) {
            c->timed_out = 1;
            return -1;
        }
        if (w < 0) {
            return -1;
        }
        n = read(c->fd, c->buf, sizeof(c->buf));
        if (n < 0 && errno == EINTR) {
            continue;
        }
        if (n < 0) {
            return -1;
        }
        if (n == 0) {
            return 0;
        }
        c->pos = 0;
        c->len = (size_t)n;
        return 1;
    }
}

static int conn_read(conn *c, void *dst, size_t n)
{
    unsigned char *d = (unsigned char *)dst;
    while (n > 0) {
        size_t k;
        if (c->pos == c->len) {
            int f = conn_fill(c);
            if (f <= 0) {
                return f;
            }
        }
        k = c->len - c->pos;
        if (k > n) {
            k = n;
        }
        memcpy(d, c->buf + c->pos, k);
        c->pos += k;
        d += k;
        n -= k;
    }
    return 1;
}

static int conn_write(conn *c, const void *data, size_t n)
{
    const char *p = (const char *)data;
#ifdef MSG_NOSIGNAL
    const int flags = MSG_NOSIGNAL;
#else
    const int flags = 0;
#endif
    while (n > 0) {
        ssize_t r;
        int w = wait_fd(c->fd, POLLOUT, c->deadline);
        if (w == 0) {
            c->timed_out = 1;
            return -1;
        }
        if (w < 0) {
            return -1;
        }
        r = send(c->fd, p, n, flags);
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

/* ---- marshalling (little endian, which is what we send) ------------------ */

typedef struct {
    unsigned char *p;
    size_t len, cap;
    int oom;
} mbuf;

static void mb_reserve(mbuf *b, size_t n)
{
    if (b->oom || b->len + n <= b->cap) {
        return;
    }
    {
        size_t cap = b->cap ? b->cap : 256;
        unsigned char *p;
        while (cap < b->len + n) {
            cap *= 2;
        }
        p = (unsigned char *)realloc(b->p, cap);
        if (!p) {
            b->oom = 1;
            return;
        }
        b->p = p;
        b->cap = cap;
    }
}

static void mb_put(mbuf *b, const void *d, size_t n)
{
    mb_reserve(b, n);
    if (!b->oom) {
        if (n) {
            memcpy(b->p + b->len, d, n);
        }
        b->len += n;
    }
}

static void mb_u8(mbuf *b, unsigned v)
{
    unsigned char c = (unsigned char)v;
    mb_put(b, &c, 1);
}

static void mb_align(mbuf *b, size_t a)
{
    size_t pad = (a - b->len % a) % a;
    mb_reserve(b, pad);
    if (!b->oom) {
        memset(b->p + b->len, 0, pad);
        b->len += pad;
    }
}

static void mb_u32(mbuf *b, unsigned v)
{
    unsigned char c[4];
    mb_align(b, 4);
    c[0] = (unsigned char)v;
    c[1] = (unsigned char)(v >> 8);
    c[2] = (unsigned char)(v >> 16);
    c[3] = (unsigned char)(v >> 24);
    mb_put(b, c, 4);
}

static void mb_patch_u32(mbuf *b, size_t pos, unsigned v)
{
    if (!b->oom) {
        b->p[pos] = (unsigned char)v;
        b->p[pos + 1] = (unsigned char)(v >> 8);
        b->p[pos + 2] = (unsigned char)(v >> 16);
        b->p[pos + 3] = (unsigned char)(v >> 24);
    }
}

/* STRING and OBJECT_PATH: u32 length, the bytes, a NUL */
static void mb_str(mbuf *b, const char *s)
{
    size_t n = strlen(s);
    mb_u32(b, (unsigned)n);
    mb_put(b, s, n + 1);
}

/* SIGNATURE: u8 length, the bytes, a NUL */
static void mb_sig(mbuf *b, const char *s)
{
    size_t n = strlen(s);
    mb_u8(b, (unsigned)n);
    mb_put(b, s, n + 1);
}

/* "ay" holding a C string, NUL included (as g_variant_new_bytestring) */
static void mb_bytestring(mbuf *b, const char *s)
{
    size_t n = strlen(s) + 1;
    mb_u32(b, (unsigned)n);
    mb_put(b, s, n);
}

/* start of an array whose elements need `elem_align`: the length is patched
 * by mb_array_end; returns the position of the length */
static size_t mb_array_begin(mbuf *b, size_t elem_align, size_t *start)
{
    size_t lenpos;
    mb_u32(b, 0);
    lenpos = b->len - 4;
    mb_align(b, elem_align); /* the padding is not part of the length */
    *start = b->len;
    return lenpos;
}

static void mb_array_end(mbuf *b, size_t lenpos, size_t start)
{
    mb_patch_u32(b, lenpos, (unsigned)(b->len - start));
}

typedef struct {
    int type, flags;
    unsigned serial, reply_serial; /* reply_serial 0: none */
    const char *path, *iface, *member, *dest, *errname, *sig;
    const char *sender; /* only a bus sets it: for the tests' fake bus */
} msgspec;

static void field_str(mbuf *m, unsigned code, char type, const char *v)
{
    char t[2];
    t[0] = type;
    t[1] = '\0';
    mb_align(m, 8);
    mb_u8(m, code);
    mb_sig(m, t);
    mb_str(m, v);
}

/* a complete message: header, fields, body (a buffer that starts at offset 0
 * of an 8-aligned body, so its own alignment is right) */
static void build_msg(mbuf *m, const msgspec *s, const mbuf *body)
{
    size_t fieldspos;
    mb_u8(m, 'l');
    mb_u8(m, (unsigned)s->type);
    mb_u8(m, (unsigned)s->flags);
    mb_u8(m, 1);
    mb_u32(m, (unsigned)(body ? body->len : 0));
    mb_u32(m, s->serial);
    mb_u32(m, 0); /* the length of the fields, patched below */
    fieldspos = m->len;
    if (s->path) {
        field_str(m, 1, 'o', s->path);
    }
    if (s->iface) {
        field_str(m, 2, 's', s->iface);
    }
    if (s->member) {
        field_str(m, 3, 's', s->member);
    }
    if (s->errname) {
        field_str(m, 4, 's', s->errname);
    }
    if (s->reply_serial) {
        mb_align(m, 8);
        mb_u8(m, 5);
        mb_sig(m, "u");
        mb_u32(m, s->reply_serial);
    }
    if (s->dest) {
        field_str(m, 6, 's', s->dest);
    }
    if (s->sender) {
        field_str(m, 7, 's', s->sender);
    }
    if (s->sig && *s->sig) {
        mb_align(m, 8);
        mb_u8(m, 8);
        mb_sig(m, "g");
        mb_sig(m, s->sig);
    }
    mb_patch_u32(m, fieldspos - 4, (unsigned)(m->len - fieldspos));
    mb_align(m, 8);
    if (body) {
        mb_put(m, body->p, body->len);
    }
}

/* ---- parsing -------------------------------------------------------------- */

typedef struct {
    unsigned char *raw; /* the whole message; the strings point into it */
    int type, flags, big;
    unsigned serial, reply_serial;
    const char *path, *iface, *member, *errname, *sender, *sig;
    const unsigned char *body;
    size_t bodylen;
} message;

static void message_free(message *m)
{
    free(m->raw);
    memset(m, 0, sizeof(*m));
}

static unsigned rd_u32(const unsigned char *p, int big)
{
    if (big) {
        return ((unsigned)p[0] << 24) | ((unsigned)p[1] << 16) |
               ((unsigned)p[2] << 8) | p[3];
    }
    return ((unsigned)p[3] << 24) | ((unsigned)p[2] << 16) |
           ((unsigned)p[1] << 8) | p[0];
}

/* a string-like value at *pos (STRING, OBJECT_PATH: u32 length; SIGNATURE: u8
 * length), NUL-terminated on the wire; 0 if malformed */
static int rd_string(const unsigned char *raw, size_t end, size_t *pos,
                     int big, int is_sig, const char **out)
{
    size_t p = *pos, n;
    if (is_sig) {
        if (p >= end) {
            return 0;
        }
        n = raw[p++];
    } else {
        p = (p + 3) & ~(size_t)3;
        if (p + 4 > end) {
            return 0;
        }
        n = rd_u32(raw + p, big);
        p += 4;
    }
    if (n >= end || p + n + 1 > end || raw[p + n] != 0) {
        return 0;
    }
    *out = (const char *)raw + p;
    *pos = p + n + 1;
    return 1;
}

static int parse_fields(message *m, size_t fields_len)
{
    size_t pos = 16, end = 16 + fields_len;
    while (pos < end) {
        unsigned code;
        const char *sig;
        pos = (pos + 7) & ~(size_t)7;
        if (pos >= end) {
            break;
        }
        code = m->raw[pos++];
        if (!rd_string(m->raw, end, &pos, m->big, 1, &sig)) {
            return 0;
        }
        if (sig[0] == 's' || sig[0] == 'o' || sig[0] == 'g') {
            const char *v;
            if (sig[1] != '\0' ||
                !rd_string(m->raw, end, &pos, m->big, sig[0] == 'g', &v)) {
                return 0;
            }
            switch (code) {
            case 1: m->path = v; break;
            case 2: m->iface = v; break;
            case 3: m->member = v; break;
            case 4: m->errname = v; break;
            case 7: m->sender = v; break;
            case 8: m->sig = v; break;
            default: break;
            }
        } else if (sig[0] == 'u' && sig[1] == '\0') {
            unsigned v;
            pos = (pos + 3) & ~(size_t)3;
            if (pos + 4 > end) {
                return 0;
            }
            v = rd_u32(m->raw + pos, m->big);
            pos += 4;
            if (code == 5) {
                m->reply_serial = v;
            }
        } else {
            return 0; /* a field we do not know the layout of */
        }
    }
    return 1;
}

/* 0: ok, else an ART_ERR_* code (message in err) */
static int read_message(conn *c, message *m, char *err, size_t errsize)
{
    unsigned char head[16];
    size_t fields_len, padded, body_len, total;
    int f;

    memset(m, 0, sizeof(*m));
    f = conn_read(c, head, sizeof(head));
    if (f != 1) {
        if (f == -1 && c->timed_out) {
            ART_format_error(err, errsize, "no answer from ART within the timeout");
            return ART_ERR_TIMEOUT;
        }
        ART_format_error(err, errsize, "the connection to the bus was lost");
        return ART_ERR_IO;
    }
    if ((head[0] != 'l' && head[0] != 'B') || head[3] != 1) {
        ART_format_error(err, errsize, "protocol error: bad message header");
        return ART_ERR_PROTOCOL;
    }
    m->big = head[0] == 'B';
    m->type = head[1];
    m->flags = head[2];
    body_len = rd_u32(head + 4, m->big);
    m->serial = rd_u32(head + 8, m->big);
    fields_len = rd_u32(head + 12, m->big);
    padded = (fields_len + 7) & ~(size_t)7;
    if (fields_len > MAX_MESSAGE || body_len > MAX_MESSAGE) {
        ART_format_error(err, errsize, "protocol error: message too large");
        return ART_ERR_PROTOCOL;
    }
    total = 16 + padded + body_len;
    m->raw = (unsigned char *)malloc(total + 1);
    if (!m->raw) {
        return ART_ERR_NOMEM;
    }
    memcpy(m->raw, head, sizeof(head));
    f = conn_read(c, m->raw + 16, total - 16);
    if (f != 1) {
        message_free(m);
        if (f == -1 && c->timed_out) {
            ART_format_error(err, errsize, "no answer from ART within the timeout");
            return ART_ERR_TIMEOUT;
        }
        ART_format_error(err, errsize, "the connection to the bus was lost");
        return ART_ERR_IO;
    }
    m->raw[total] = '\0';
    if (!parse_fields(m, fields_len)) {
        message_free(m);
        ART_format_error(err, errsize, "protocol error: bad message fields");
        return ART_ERR_PROTOCOL;
    }
    m->body = m->raw + 16 + padded;
    m->bodylen = body_len;
    return ART_OK;
}

/* the first value of a message, if it is a string or an int32; 0 if not */
static int body_string(const message *m, const char **s)
{
    size_t pos = 0;
    if (!m->sig || (m->sig[0] != 's' && m->sig[0] != 'o') || m->sig[1]) {
        return 0;
    }
    return rd_string(m->body, m->bodylen, &pos, m->big, 0, s);
}

static int body_u32(const message *m, char type, unsigned *v)
{
    if (!m->sig || m->sig[0] != type || m->sig[1] || m->bodylen < 4) {
        return 0;
    }
    *v = rd_u32(m->body, m->big);
    return 1;
}

/* ---- the connection to the bus ----------------------------------------------- */

typedef struct {
    int unused;
} dbus_state;

static int hexval(int c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

/* the value of `key` in "k=v,k=v" (unescaping %XX), 1 if present */
static int addr_value(const char *params, const char *key, char *out,
                      size_t size)
{
    size_t klen = strlen(key);
    const char *p = params;
    while (*p) {
        const char *end = strchr(p, ',');
        size_t n = end ? (size_t)(end - p) : strlen(p);
        if (n > klen && p[klen] == '=' && strncmp(p, key, klen) == 0) {
            size_t i = klen + 1, o = 0;
            while (i < n) {
                int h1, h2;
                if (p[i] == '%' && i + 2 < n &&
                    (h1 = hexval(p[i + 1])) >= 0 &&
                    (h2 = hexval(p[i + 2])) >= 0) {
                    if (o + 1 >= size) {
                        return 0;
                    }
                    out[o++] = (char)(h1 * 16 + h2);
                    i += 3;
                } else {
                    if (o + 1 >= size) {
                        return 0;
                    }
                    out[o++] = p[i++];
                }
            }
            out[o] = '\0';
            return 1;
        }
        if (!end) {
            break;
        }
        p = end + 1;
    }
    return 0;
}

static void close_fd(int fd)
{
    int saved = errno;
    close(fd);
    errno = saved;
}

/* connect to one address ("unix:path=...", "unix:abstract=..."), returning
 * the descriptor, or -1 with `why` filled */
static int connect_one(const char *addr, char *why, size_t whysize)
{
    struct sockaddr_un sa;
    char path[1024];
    socklen_t salen;
    int fd, is_abstract = 0;
    const char *colon = strchr(addr, ':');

    if (!colon || (size_t)(colon - addr) != 4 || strncmp(addr, "unix", 4)) {
        ART_format_error(why, whysize, "unsupported D-Bus transport in \"%s\"",
                         addr);
        return -1;
    }
    if (addr_value(colon + 1, "path", path, sizeof(path))) {
        /* a file */
    } else if (addr_value(colon + 1, "abstract", path, sizeof(path))) {
#ifdef __linux__
        is_abstract = 1;
#else
        ART_format_error(why, whysize,
                         "abstract sockets are not supported here (%s)", addr);
        return -1;
#endif
    } else {
        ART_format_error(why, whysize, "unsupported D-Bus address \"%s\"", addr);
        return -1;
    }
    memset(&sa, 0, sizeof(sa));
    sa.sun_family = AF_UNIX;
    if (strlen(path) + (is_abstract ? 1 : 0) >= sizeof(sa.sun_path)) {
        ART_format_error(why, whysize, "the bus socket path is too long: %s",
                         path);
        return -1;
    }
    if (is_abstract) {
        memcpy(sa.sun_path + 1, path, strlen(path));
        salen = (socklen_t)(offsetof(struct sockaddr_un, sun_path) + 1 +
                            strlen(path));
    } else {
        memcpy(sa.sun_path, path, strlen(path));
        salen = (socklen_t)sizeof(sa);
    }
    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        ART_format_error(why, whysize, "cannot create a socket: %s",
                         strerror(errno));
        return -1;
    }
    {
        int fl = fcntl(fd, F_GETFD);
        if (fl >= 0) {
            fcntl(fd, F_SETFD, fl | FD_CLOEXEC);
        }
    }
#ifdef SO_NOSIGPIPE
    {
        int one = 1;
        setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
    }
#endif
    if (connect(fd, (struct sockaddr *)&sa, salen) != 0) {
        ART_format_error(why, whysize, "%s: %s", path, strerror(errno));
        close_fd(fd);
        return -1;
    }
    return fd;
}

/* %XX-escape a path for use as a value of an address */
static void append_escaped(char *dst, size_t size, const char *path)
{
    static const char hex[] = "0123456789abcdef";
    size_t o = strlen(dst);
    for (; *path && o + 4 < size; ++path) {
        unsigned char c = (unsigned char)*path;
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') || strchr("/-_.\\", c)) {
            dst[o++] = (char)c;
        } else {
            dst[o++] = '%';
            dst[o++] = hex[c >> 4];
            dst[o++] = hex[c & 15];
        }
    }
    dst[o] = '\0';
}

static void append_unix_path(char *dst, size_t size, const char *path)
{
    size_t o = strlen(dst);
    if (o && o + 1 < size) {
        dst[o++] = ';';
        dst[o] = '\0';
    }
    snprintf(dst + o, size - o, "unix:path=");
    append_escaped(dst, size, path);
}

/* the bus to use: the option, $DBUS_SESSION_BUS_ADDRESS, else the places
 * where ART's own bus is (the same as tools/mcp):
 *  - Linux: the session bus of systemd, $XDG_RUNTIME_DIR/bus
 *  - macOS: the one the app launcher starts, <TMPDIR>/ART-<user>/dbus.sock
 * as a list of addresses separated by ';' */
static void bus_addresses(const ART_config *cfg, char *out, size_t size)
{
    const char *env;
    out[0] = '\0';
    if (cfg->bus_address && *cfg->bus_address) {
        snprintf(out, size, "%s", cfg->bus_address);
        return;
    }
    env = getenv("DBUS_SESSION_BUS_ADDRESS");
    if (env && *env && strcmp(env, "disabled:") != 0) {
        snprintf(out, size, "%s", env);
        return;
    }
#ifdef __APPLE__
    {
        const char *user = getenv("USER");
        const char *tmp = getenv("TMPDIR");
        char path[1024], usrbuf[32], ctmp[1024];
        if (!user || !*user) {
            snprintf(usrbuf, sizeof(usrbuf), "%u", (unsigned)geteuid());
            user = usrbuf;
        }
        if (tmp && *tmp) {
            snprintf(path, sizeof(path), "%s/ART-%s/dbus.sock", tmp, user);
            append_unix_path(out, size, path);
        }
        /* a client started without TMPDIR (a GUI program, say) */
        if (confstr(_CS_DARWIN_USER_TEMP_DIR, ctmp, sizeof(ctmp)) > 0) {
            snprintf(path, sizeof(path), "%s/ART-%s/dbus.sock", ctmp, user);
            append_unix_path(out, size, path);
        }
        snprintf(path, sizeof(path), "/tmp/ART-%s/dbus.sock", user);
        append_unix_path(out, size, path);
    }
#else
    {
        const char *rt = getenv("XDG_RUNTIME_DIR");
        char path[1024];
        if (rt && rt[0] == '/') {
            snprintf(path, sizeof(path), "%s/bus", rt);
        } else {
            snprintf(path, sizeof(path), "/run/user/%u/bus",
                     (unsigned)geteuid());
        }
        append_unix_path(out, size, path);
    }
#endif
}

/* connect to the first address of the list that works */
static int connect_bus(const char *list, char *err, size_t errsize)
{
    char why[512] = "";
    char one[1100];
    const char *p = list;
    int tried = 0;
    while (*p) {
        const char *end = strchr(p, ';');
        size_t n = end ? (size_t)(end - p) : strlen(p);
        if (n > 0 && n < sizeof(one)) {
            int fd;
            memcpy(one, p, n);
            one[n] = '\0';
            ++tried;
            fd = connect_one(one, why, sizeof(why));
            if (fd >= 0) {
                return fd;
            }
        }
        if (!end) {
            break;
        }
        p = end + 1;
    }
    if (!tried) {
        ART_format_error(err, errsize,
                         "no D-Bus session bus (set DBUS_SESSION_BUS_ADDRESS)");
    } else {
        ART_format_error(err, errsize, "cannot connect to the D-Bus bus (%s)",
                         why);
    }
    return -1;
}

/* EXTERNAL authentication: 0 ok, else an ART_ERR_* code */
static int authenticate(conn *c, char *err, size_t errsize)
{
    char uid[32], line[256], hex[80];
    static const char digits[] = "0123456789abcdef";
    size_t i, n = 0;
    int o = 0;

    snprintf(uid, sizeof(uid), "%u", (unsigned)geteuid());
    for (i = 0; uid[i]; ++i) {
        hex[o++] = digits[(unsigned char)uid[i] >> 4];
        hex[o++] = digits[(unsigned char)uid[i] & 15];
    }
    hex[o] = '\0';
    /* a NUL byte starts the protocol */
    {
        char first[128];
        int l = snprintf(first + 1, sizeof(first) - 1, "AUTH EXTERNAL %s\r\n", hex);
        first[0] = '\0';
        if (conn_write(c, first, (size_t)l + 1) != 0) {
            ART_format_error(err, errsize, "cannot talk to the bus");
            return ART_ERR_IO;
        }
    }
    for (;;) {
        char ch;
        int f = conn_read(c, &ch, 1);
        if (f != 1) {
            ART_format_error(err, errsize,
                             f == -1 && c->timed_out
                                 ? "the bus did not answer"
                                 : "the bus closed the connection (authentication refused?)");
            return f == -1 && c->timed_out ? ART_ERR_TIMEOUT : ART_ERR_CONNECT;
        }
        if (ch == '\n') {
            break;
        }
        if (n + 1 < sizeof(line)) {
            line[n++] = ch;
        }
    }
    line[n] = '\0';
    if (strncmp(line, "OK", 2) != 0) {
        ART_format_error(err, errsize, "the bus refused the authentication: %s",
                         line);
        return ART_ERR_CONNECT;
    }
    if (conn_write(c, "BEGIN\r\n", 7) != 0) {
        ART_format_error(err, errsize, "cannot talk to the bus");
        return ART_ERR_IO;
    }
    return ART_OK;
}

/* ---- conversations --------------------------------------------------------- */

typedef struct {
    conn c;
    unsigned next_serial;
} session;

static int send_msg(session *s, const msgspec *spec, const mbuf *body,
                    char *err, size_t errsize)
{
    mbuf m;
    int rc = ART_OK;
    memset(&m, 0, sizeof(m));
    build_msg(&m, spec, body);
    if (m.oom) {
        free(m.p);
        return ART_ERR_NOMEM;
    }
    if (conn_write(&s->c, m.p, m.len) != 0) {
        if (s->c.timed_out) {
            ART_format_error(err, errsize, "timeout sending the command");
            rc = ART_ERR_TIMEOUT;
        } else {
            ART_format_error(err, errsize, "cannot send to the bus");
            rc = ART_ERR_IO;
        }
    }
    free(m.p);
    return rc;
}

/* answer a call that was made to us */
static int answer_call(session *s, const message *in, int known, char *err,
                       size_t errsize)
{
    msgspec spec;
    mbuf body;
    int rc;
    if ((in->flags & FLAG_NO_REPLY) || !in->sender) {
        return ART_OK;
    }
    memset(&spec, 0, sizeof(spec));
    memset(&body, 0, sizeof(body));
    spec.flags = FLAG_NO_REPLY;
    spec.serial = s->next_serial++;
    spec.reply_serial = in->serial;
    spec.dest = in->sender;
    if (known) {
        spec.type = MSG_RETURN;
        rc = send_msg(s, &spec, NULL, err, errsize);
    } else {
        spec.type = MSG_ERROR;
        spec.errname = "org.freedesktop.DBus.Error.UnknownMethod";
        spec.sig = "s";
        mb_str(&body, "not implemented by libART");
        if (body.oom) {
            free(body.p);
            return ART_ERR_NOMEM;
        }
        rc = send_msg(s, &spec, &body, err, errsize);
    }
    free(body.p);
    return rc;
}

/* read messages until the reply to `serial` (a return or an error), handing
 * the Print()/PrintError() calls of the other side to `sink` (they are
 * refused when it is NULL). 0 on success, with the reply in *reply. */
static int await_reply(session *s, unsigned serial, const ART_sink *sink,
                       message *reply, char *err, size_t errsize)
{
    for (;;) {
        message m;
        int rc = read_message(&s->c, &m, err, errsize);
        if (rc != ART_OK) {
            return rc;
        }
        if ((m.type == MSG_RETURN || m.type == MSG_ERROR) &&
            m.reply_serial == serial) {
            *reply = m;
            return ART_OK;
        }
        if (m.type == MSG_CALL) {
            const char *text = NULL;
            int known = 0;
            if (sink && m.member && m.iface &&
                strcmp(m.iface, "org.gtk.private.CommandLine") == 0 &&
                (strcmp(m.member, "Print") == 0 ||
                 strcmp(m.member, "PrintError") == 0) &&
                body_string(&m, &text)) {
                known = 1;
                if (m.member[5] == '\0') {
                    if (sink->out) {
                        sink->out(sink->out_user, text, strlen(text));
                    }
                } else if (sink->err) {
                    sink->err(sink->err_user, text, strlen(text));
                }
            }
            rc = answer_call(s, &m, known, err, errsize);
            message_free(&m);
            if (rc != ART_OK) {
                return rc;
            }
            continue;
        }
        message_free(&m); /* signals, replies to nothing we asked */
    }
}

/* describe an error reply */
static void describe_error(const message *m, char *err, size_t errsize)
{
    const char *text = NULL;
    if (body_string(m, &text) && *text) {
        ART_format_error(err, errsize, "%s: %s",
                         m->errname ? m->errname : "D-Bus error", text);
    } else {
        ART_format_error(err, errsize, "%s",
                         m->errname ? m->errname : "D-Bus error");
    }
}

static int call_bus(session *s, const char *member, const mbuf *body,
                    const char *sig, message *reply, char *err, size_t errsize)
{
    msgspec spec;
    unsigned serial = s->next_serial++;
    int rc;
    memset(&spec, 0, sizeof(spec));
    spec.type = MSG_CALL;
    spec.serial = serial;
    spec.path = "/org/freedesktop/DBus";
    spec.iface = "org.freedesktop.DBus";
    spec.member = member;
    spec.dest = "org.freedesktop.DBus";
    spec.sig = sig;
    rc = send_msg(s, &spec, body, err, errsize);
    if (rc != ART_OK) {
        return rc;
    }
    rc = await_reply(s, serial, NULL, reply, err, errsize);
    if (rc == ART_OK && reply->type == MSG_ERROR) {
        describe_error(reply, err, errsize);
        message_free(reply);
        return ART_ERR_IO;
    }
    return rc;
}

/* the arguments of CommandLine(): the object that gets the output, the
 * command line the way `ART --cmd` passes it ({base dir, "--cmd", verb,
 * arguments...}) and the platform data GApplication adds: the working
 * directory and (the GUI asks for it) the environment */
static void command_line_body(mbuf *b, const char *cwd, int argc,
                              const char *const *argv)
{
    size_t lenpos, start, dictlen, dictstart, i;

    mb_str(b, CALLBACK_PATH);

    lenpos = mb_array_begin(b, 4, &start); /* aay */
    mb_bytestring(b, "ART");
    mb_bytestring(b, "--cmd");
    for (i = 0; i < (size_t)argc; ++i) {
        mb_bytestring(b, argv[i]);
    }
    mb_array_end(b, lenpos, start);

    dictlen = mb_array_begin(b, 8, &dictstart); /* a{sv} */
    mb_align(b, 8);
    mb_str(b, "cwd");
    mb_sig(b, "ay");
    mb_bytestring(b, cwd);
    mb_align(b, 8);
    mb_str(b, "environ");
    mb_sig(b, "aay");
    lenpos = mb_array_begin(b, 4, &start);
    mb_bytestring(b, "ART_IS_CMD=1");
    mb_bytestring(b, "ART_CMD_START=0"); /* never start a GUI for a command */
    mb_array_end(b, lenpos, start);
    mb_array_end(b, dictlen, dictstart);
}

static int gui_exec(void *state, const ART_config *cfg, const char *cwd,
                    int argc, const char *const *argv, const ART_sink *sink,
                    char *err, size_t errsize)
{
    char addrs[4096];
    session s;
    message reply;
    mbuf body;
    msgspec spec;
    size_t paylen = strlen(cwd);
    unsigned serial, owned = 0, status = 0;
    int rc, i, fd;

    (void)state;
    for (i = 0; i < argc; ++i) {
        paylen += strlen(argv[i]) + 1;
        if (paylen > MAX_REQUEST) {
            ART_format_error(err, errsize, "the command is too long");
            return ART_ERR_INVALID;
        }
    }

    bus_addresses(cfg, addrs, sizeof(addrs));
    fd = connect_bus(addrs, err, errsize);
    if (fd < 0) {
        return ART_ERR_CONNECT;
    }
    memset(&s, 0, sizeof(s));
    s.c.fd = fd;
    s.c.deadline = cfg->timeout_ms >= 0 ? now_ms() + cfg->timeout_ms : -1;
    s.next_serial = 1;

    rc = authenticate(&s.c, err, errsize);
    if (rc != ART_OK) {
        goto done;
    }

    /* Hello: the bus does not talk to us before it */
    rc = call_bus(&s, "Hello", NULL, NULL, &reply, err, errsize);
    if (rc != ART_OK) {
        goto done;
    }
    message_free(&reply);

    memset(&body, 0, sizeof(body));
    mb_str(&body, BUS_NAME);
    rc = call_bus(&s, "NameHasOwner", &body, "s", &reply, err, errsize);
    free(body.p);
    if (rc != ART_OK) {
        goto done;
    }
    if (!body_u32(&reply, 'b', &owned)) {
        message_free(&reply);
        ART_format_error(err, errsize, "protocol error: bad reply of the bus");
        rc = ART_ERR_PROTOCOL;
        goto done;
    }
    message_free(&reply);
    if (!owned) {
        ART_format_error(err, errsize,
                         "no running ART GUI (nobody owns %s on the bus %s)",
                         BUS_NAME, addrs);
        rc = ART_ERR_CONNECT;
        goto done;
    }

    memset(&body, 0, sizeof(body));
    command_line_body(&body, cwd, argc, argv);
    if (body.oom) {
        free(body.p);
        rc = ART_ERR_NOMEM;
        goto done;
    }
    memset(&spec, 0, sizeof(spec));
    serial = s.next_serial++;
    spec.type = MSG_CALL;
    spec.flags = FLAG_NO_AUTO_START;
    spec.serial = serial;
    spec.path = OBJECT_PATH;
    spec.iface = APP_IFACE;
    spec.member = "CommandLine";
    spec.dest = BUS_NAME;
    spec.sig = "oaaya{sv}";
    rc = send_msg(&s, &spec, &body, err, errsize);
    free(body.p);
    if (rc != ART_OK) {
        goto done;
    }
    rc = await_reply(&s, serial, sink, &reply, err, errsize);
    if (rc != ART_OK) {
        goto done;
    }
    if (reply.type == MSG_ERROR) {
        describe_error(&reply, err, errsize);
        rc = (reply.errname && strstr(reply.errname, "ServiceUnknown"))
                 ? ART_ERR_CONNECT
                 : ART_ERR_IO;
        message_free(&reply);
        goto done;
    }
    if (!body_u32(&reply, 'i', &status)) {
        ART_format_error(err, errsize,
                         "protocol error: bad reply to the command");
        message_free(&reply);
        rc = ART_ERR_PROTOCOL;
        goto done;
    }
    message_free(&reply);
    rc = status == 0 ? ART_OK : ART_ERR_COMMAND;

done:
    close_fd(fd);
    return rc;
}

static void gui_destroy(void *state) { free(state); }

static const ART_backend_ops gui_ops = {gui_exec, gui_destroy};

int ART_dbus_create(const ART_config *cfg, const ART_backend_ops **ops,
                    void **state, char *err, size_t errsize)
{
    dbus_state *s;
    (void)cfg;
    (void)err;
    (void)errsize;
    s = (dbus_state *)calloc(1, sizeof(*s));
    if (!s) {
        return ART_ERR_NOMEM;
    }
    *ops = &gui_ops;
    *state = s;
    return ART_OK;
}

int ART_dbus_available(void) { return 1; }

#endif /* POSIX */
