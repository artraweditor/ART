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
 *  Tests of libART's GUI backend against a fake D-Bus bus with a fake ART GUI
 *  behind it (no ART, no dbus-daemon needed): ctest -R libart-dbus
 *
 *  This file includes art_dbus.c to reuse its message builder and parser; the
 *  wire format itself was checked against GLib's GApplication.
 */

#include "art_dbus.c"

#include <pthread.h>
#include <stdio.h>
#include <sys/stat.h>

#ifdef _WIN32
int main(void) { return 77; }
#else

static int failures = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            ++failures;                                                      \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, \
                    #cond);                                                  \
        }                                                                    \
    } while (0)

enum { M_OK, M_STATUS1, M_ERROR, M_HANG, M_NOOWNER };

typedef struct {
    int mode;
    int listen_fd;
    int seen_args, seen_env, seen_cwd, seen_flags_ok, saw_unknown_error;
    char uid_hex[64];
} fake;

static char tmpdir[64];

static int read_line(conn *c, char *out, size_t n)
{
    size_t i = 0;
    char ch;
    while (conn_read(c, &ch, 1) == 1) {
        if (ch == '\n') {
            if (i && out[i - 1] == '\r') {
                --i;
            }
            out[i] = '\0';
            return 1;
        }
        if (i + 1 < n) {
            out[i++] = ch;
        }
    }
    return 0;
}

static int has(const unsigned char *p, size_t n, const char *needle)
{
    size_t l = strlen(needle), i;
    for (i = 0; i + l <= n; ++i) {
        if (memcmp(p + i, needle, l) == 0) {
            return 1;
        }
    }
    return 0;
}

static void send_to(int fd, const msgspec *spec, const mbuf *body)
{
    mbuf m;
    memset(&m, 0, sizeof(m));
    build_msg(&m, spec, body);
    (void)!write(fd, m.p, m.len);
    free(m.p);
}

static void reply_simple(int fd, const message *in, const char *sig,
                         const mbuf *body)
{
    msgspec r;
    memset(&r, 0, sizeof(r));
    r.type = MSG_RETURN;
    r.serial = 1000;
    r.reply_serial = in->serial;
    r.sig = sig;
    send_to(fd, &r, body);
}

/* a call from the "GUI" (unique name :1.2) to the client (:1.1) */
static void gui_call(int fd, const char *path, const char *member,
                     const char *text, unsigned serial)
{
    msgspec c;
    mbuf b;
    memset(&c, 0, sizeof(c));
    memset(&b, 0, sizeof(b));
    c.type = MSG_CALL;
    c.serial = serial;
    c.path = path;
    c.iface = "org.gtk.private.CommandLine";
    c.member = member;
    c.dest = ":1.1";
    c.sender = ":1.2";
    if (text) {
        c.sig = "s";
        mb_str(&b, text);
    }
    send_to(fd, &c, text ? &b : NULL);
    free(b.p);
}

static void *fake_main(void *arg)
{
    fake *f = (fake *)arg;
    int fd = accept(f->listen_fd, NULL, NULL);
    conn c;
    char line[256];
    message m;
    char err[256];

    if (fd < 0) {
        return NULL;
    }
    memset(&c, 0, sizeof(c));
    c.fd = fd;
    c.deadline = now_ms() + 10000;
    /* "\0AUTH EXTERNAL <hex uid>" */
    {
        char z;
        CHECK(conn_read(&c, &z, 1) == 1 && z == '\0');
    }
    CHECK(read_line(&c, line, sizeof(line)) == 1);
    CHECK(strncmp(line, "AUTH EXTERNAL ", 14) == 0);
    snprintf(f->uid_hex, sizeof(f->uid_hex), "%s", line + 14);
    (void)!write(fd, "OK 00112233445566778899aabbccddeeff\r\n", 37);
    CHECK(read_line(&c, line, sizeof(line)) == 1 && !strncmp(line, "BEGIN", 5));

    while (read_message(&c, &m, err, sizeof(err)) == ART_OK) {
        if (m.type == MSG_CALL && m.member && !strcmp(m.member, "Hello")) {
            mbuf b;
            memset(&b, 0, sizeof(b));
            mb_str(&b, ":1.1");
            reply_simple(fd, &m, "s", &b);
            free(b.p);
        } else if (m.type == MSG_CALL && !strcmp(m.member, "NameHasOwner")) {
            mbuf b;
            const char *name = NULL;
            CHECK(body_string(&m, &name) && !strcmp(name, "us.pixls.art.ART"));
            memset(&b, 0, sizeof(b));
            mb_u32(&b, f->mode == M_NOOWNER ? 0 : 1);
            reply_simple(fd, &m, "b", &b);
            free(b.p);
        } else if (m.type == MSG_CALL && !strcmp(m.member, "CommandLine")) {
            mbuf b;
            message back;
            CHECK(m.path && !strcmp(m.path, "/us/pixls/art/ART"));
            CHECK(m.iface && !strcmp(m.iface, "org.gtk.Application"));
            CHECK(m.sig && !strcmp(m.sig, "oaaya{sv}"));
            CHECK(m.flags & FLAG_NO_AUTO_START);
            f->seen_args = has(m.body, m.bodylen, "get-info") &&
                           has(m.body, m.bodylen, "--cmd") &&
                           has(m.body, m.bodylen, "a b");
            f->seen_env = has(m.body, m.bodylen, "ART_IS_CMD=1") &&
                          has(m.body, m.bodylen, "ART_CMD_START=0");
            f->seen_cwd = has(m.body, m.bodylen, "/some/dir");
            if (f->mode == M_HANG) {
                message_free(&m);
                sleep(2);
                break;
            }
            if (f->mode == M_ERROR) {
                msgspec e;
                memset(&e, 0, sizeof(e));
                memset(&b, 0, sizeof(b));
                e.type = MSG_ERROR;
                e.serial = 1001;
                e.reply_serial = m.serial;
                e.errname = "org.freedesktop.DBus.Error.ServiceUnknown";
                e.sig = "s";
                e.sender = ":1.2";
                mb_str(&b, "gone");
                send_to(fd, &e, &b);
                free(b.p);
                message_free(&m);
                continue;
            }
            /* a call we do not implement gets an error ... */
            gui_call(fd, "/org/gtk/Application/CommandLine", "Foo", NULL, 100);
            if (read_message(&c, &back, err, sizeof(err)) == ART_OK) {
                f->saw_unknown_error =
                    back.type == MSG_ERROR && back.reply_serial == 100 &&
                    back.errname &&
                    strstr(back.errname, "UnknownMethod") != NULL;
                message_free(&back);
            }
            /* ... the output in two pieces, and the error text */
            gui_call(fd, "/org/gtk/Application/CommandLine", "Print",
                     "{\"ok\":true,", 101);
            if (read_message(&c, &back, err, sizeof(err)) == ART_OK) {
                CHECK(back.type == MSG_RETURN && back.reply_serial == 101);
                message_free(&back);
            }
            gui_call(fd, "/org/gtk/Application/CommandLine", "Print",
                     "\"result\":42}\n", 102);
            if (read_message(&c, &back, err, sizeof(err)) == ART_OK) {
                message_free(&back);
            }
            gui_call(fd, "/org/gtk/Application/CommandLine", "PrintError",
                     "ART: error: boom\n", 103);
            if (read_message(&c, &back, err, sizeof(err)) == ART_OK) {
                message_free(&back);
            }
            memset(&b, 0, sizeof(b));
            mb_u32(&b, f->mode == M_STATUS1 ? 1 : 0);
            {
                msgspec r;
                memset(&r, 0, sizeof(r));
                r.type = MSG_RETURN;
                r.serial = 1002;
                r.reply_serial = m.serial;
                r.sig = "i";
                r.sender = ":1.2";
                send_to(fd, &r, &b);
            }
            free(b.p);
        }
        message_free(&m);
    }
    close(fd);
    return NULL;
}

typedef struct {
    char out[256], err[256];
} text;

static void on_out(void *u, const char *d, size_t n)
{
    text *t = (text *)u;
    strncat(t->out, d, n < sizeof(t->out) - strlen(t->out) - 1
                           ? n : sizeof(t->out) - strlen(t->out) - 1);
}

static void on_err(void *u, const char *d, size_t n)
{
    text *t = (text *)u;
    strncat(t->err, d, n < sizeof(t->err) - strlen(t->err) - 1
                           ? n : sizeof(t->err) - strlen(t->err) - 1);
}

/* run one command against a fresh fake bus in the given mode */
static int scenario(int mode, int timeout_ms, text *t, fake *f, char *engerr,
                    size_t engerrsize)
{
    char path[128], addr[160];
    struct sockaddr_un sa;
    pthread_t th;
    ART_options *o;
    ART_engine *e;
    int rc;
    const char *argv[] = {"get-info", "a b", NULL};

    memset(t, 0, sizeof(*t));
    memset(f, 0, sizeof(*f));
    f->mode = mode;
    snprintf(path, sizeof(path), "%s/bus", tmpdir);
    unlink(path);
    f->listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    memset(&sa, 0, sizeof(sa));
    sa.sun_family = AF_UNIX;
    snprintf(sa.sun_path, sizeof(sa.sun_path), "%s", path);
    CHECK(bind(f->listen_fd, (struct sockaddr *)&sa, sizeof(sa)) == 0);
    CHECK(listen(f->listen_fd, 1) == 0);
    pthread_create(&th, NULL, fake_main, f);

    snprintf(addr, sizeof(addr), "unix:path=%s", path);
    o = ART_options_new();
    CHECK(ART_options_set_gui(o, addr) == ART_OK);
    ART_options_set_output(o, on_out, t);
    ART_options_set_error(o, on_err, t);
    ART_options_set_cwd(o, "/some/dir");
    if (timeout_ms >= 0) {
        ART_options_set_timeout(o, timeout_ms);
    }
    e = ART_engine_create(o);
    CHECK(e != NULL);
    ART_options_free(o);
    rc = ART_exec_cmd(e, argv);
    snprintf(engerr, engerrsize, "%s", ART_engine_error(e));
    ART_engine_destroy(e);
    pthread_join(th, NULL);
    close(f->listen_fd);
    unlink(path);
    return rc;
}

int main(void)
{
    text t;
    fake f;
    char eng[512];
    int rc;
    char uid[32], hex[64];
    size_t i, o = 0;

    signal(SIGPIPE, SIG_IGN);
    snprintf(tmpdir, sizeof(tmpdir), "/tmp/artdbus.XXXXXX");
    if (!mkdtemp(tmpdir)) {
        perror("mkdtemp");
        return 77;
    }
    CHECK(ART_has_backend(ART_BACKEND_GUI) == 1);

    rc = scenario(M_OK, -1, &t, &f, eng, sizeof(eng));
    CHECK(rc == ART_OK);
    CHECK(!strcmp(t.out, "{\"ok\":true,\"result\":42}\n"));
    CHECK(!strcmp(t.err, "ART: error: boom\n"));
    CHECK(f.seen_args && f.seen_env && f.seen_cwd);
    CHECK(f.saw_unknown_error);
    snprintf(uid, sizeof(uid), "%u", (unsigned)geteuid());
    for (i = 0; uid[i]; ++i) {
        o += (size_t)snprintf(hex + o, sizeof(hex) - o, "%02x", (unsigned char)uid[i]);
    }
    CHECK(!strcmp(f.uid_hex, hex));

    rc = scenario(M_STATUS1, -1, &t, &f, eng, sizeof(eng));
    CHECK(rc == ART_ERR_COMMAND);
    CHECK(eng[0] == '\0');
    CHECK(strstr(t.err, "boom") != NULL);

    rc = scenario(M_NOOWNER, -1, &t, &f, eng, sizeof(eng));
    CHECK(rc == ART_ERR_CONNECT);
    CHECK(strstr(eng, "no running ART GUI") != NULL);
    CHECK(t.out[0] == '\0');

    rc = scenario(M_ERROR, -1, &t, &f, eng, sizeof(eng));
    CHECK(rc == ART_ERR_CONNECT);
    CHECK(strstr(eng, "gone") != NULL);

    rc = scenario(M_HANG, 300, &t, &f, eng, sizeof(eng));
    CHECK(rc == ART_ERR_TIMEOUT);

    {   /* no bus at all */
        ART_options *op = ART_options_new();
        ART_engine *e;
        const char *argv[] = {"get-info", NULL};
        ART_options_set_gui(op, "unix:path=/nonexistent/bus");
        e = ART_engine_create(op);
        ART_options_free(op);
        CHECK(e != NULL);
        CHECK(ART_exec_cmd(e, argv) == ART_ERR_CONNECT);
        CHECK(strstr(ART_engine_error(e), "/nonexistent/bus") != NULL);
        ART_engine_destroy(e);
    }

    rmdir(tmpdir);
    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    printf("libart GUI backend: ok\n");
    return 0;
}
#endif
