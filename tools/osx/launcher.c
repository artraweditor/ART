/* -*- C -*-
 *
 *  This file is part of ART.
 *
 *  Copyright 2026 Alberto Griggio <alberto.griggio@gmail.com>
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
 * Launcher for the macOS ART.app bundle.
 *
 * This program is installed as the CFBundleExecutable of the bundle (i.e. as
 * Contents/MacOS/<name>). It prepares the environment the bundled GTK stack
 * needs and then *execv*s the real binary, Contents/MacOS/.<name>.bin.
 *
 * Using execv rather than fork+wait is what makes opening files from the
 * Finder work: LaunchServices delivers documents to a bundled application as a
 * kAEOpenDocuments Apple Event addressed to the process it launched, not on
 * argv. execv keeps the pid (and the process serial number), so the event
 * reaches the exec'd ART binary, where it is picked up by the
 * "NSApplicationOpenFile" handler installed in rtgui/main.cc. If the real
 * binary ran as a child process instead - as it did with the shell script this
 * launcher replaces - the event would be delivered to the parent and dropped.
 *
 * Therefore, when changing this file:
 *   - never fork or posix_spawn the target, only execv it;
 *   - never link against AppKit and never run a CFRunLoop/NSApplication here;
 *   - keep the work done before the execv to a minimum.
 *
 * The target is derived from argv[0], so the same launcher can be installed
 * under any name.
 */

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/wait.h>

#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <libgen.h>
#include <limits.h>
#include <mach-o/dyld.h>
#include <pwd.h>
#include <spawn.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern char **environ;

/* rtengine/subprocess.cc (get_env) restores the value saved under this prefix
 * when spawning helper programs such as exiftool, so that they do not inherit
 * the bundle-private GTK environment set up below. A saved empty value means
 * "unset in the child", which is exactly what we want for variables that had
 * no value before we overwrote them. */
#define ART_RESTORE_PREFIX "ART_restore_"

static const char *prog = "ART-launcher";

static void die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "%s: ", prog);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(1);
}

static void warn(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "%s: warning: ", prog);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}

/* snprintf that refuses to truncate silently */
static void fmtpath(char *dst, size_t n, const char *fmt, ...)
{
    va_list ap;
    int r;

    va_start(ap, fmt);
    r = vsnprintf(dst, n, fmt, ap);
    va_end(ap);

    if (r < 0 || (size_t)r >= n) {
        die("path too long (limit is %zu bytes): %s", n, fmt);
    }
}

/* -------------------------------------------------------------------------
 * environment
 * ------------------------------------------------------------------------- */

static void save_env(const char *key)
{
    char restore[128];
    const char *cur = getenv(key);
    /* setenv() may reshuffle the environment, so keep our own copy */
    char *old = cur ? strdup(cur) : NULL;

    fmtpath(restore, sizeof(restore), ART_RESTORE_PREFIX "%s", key);
    if (setenv(restore, old ? old : "", 1) != 0) {
        die("cannot set %s: %s", restore, strerror(errno));
    }
    free(old);
}

static void set_env(const char *key, const char *val)
{
    save_env(key);
    if (setenv(key, val, 1) != 0) {
        die("cannot set %s: %s", key, strerror(errno));
    }
}

/* for variables that the helper programs are meant to inherit, and that
 * therefore need no ART_restore_ counterpart */
static void set_env_plain(const char *key, const char *val)
{
    if (setenv(key, val, 1) != 0) {
        die("cannot set %s: %s", key, strerror(errno));
    }
}

/* -------------------------------------------------------------------------
 * running the helper programs needed before startup
 * ------------------------------------------------------------------------- */

static int run(char *const *args, const char *stdout_path)
{
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_t *actions_p = NULL;
    pid_t pid;
    int status = -1;
    int err;

    if (stdout_path) {
        if (posix_spawn_file_actions_init(&actions) != 0) {
            warn("cannot redirect the output of %s", args[0]);
            return -1;
        }
        actions_p = &actions;
        posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, stdout_path,
                                         O_WRONLY | O_CREAT | O_TRUNC, 0600);
    }

    err = posix_spawn(&pid, args[0], actions_p, NULL, args, environ);
    if (actions_p) {
        posix_spawn_file_actions_destroy(actions_p);
    }
    if (err != 0) {
        warn("cannot run %s: %s", args[0], strerror(err));
        return -1;
    }

    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) {
            warn("cannot wait for %s: %s", args[0], strerror(errno));
            return -1;
        }
    }
    return status;
}

/* runs "<tool> <files matching pattern>", saving the output to out */
static void run_query_tool(const char *tool, const char *pattern,
                           const char *out)
{
    glob_t g;
    char **args;
    size_t i;

    memset(&g, 0, sizeof(g));
    if (glob(pattern, 0, NULL, &g) != 0 || g.gl_pathc == 0) {
        warn("no files matching %s, skipping %s", pattern, tool);
        globfree(&g);
        return;
    }

    args = calloc(g.gl_pathc + 2, sizeof(char *));
    if (!args) {
        globfree(&g);
        die("out of memory");
    }
    args[0] = (char *)tool;
    for (i = 0; i < g.gl_pathc; ++i) {
        args[i + 1] = g.gl_pathv[i];
    }

    run(args, out);

    free(args);
    globfree(&g);
}

/* -------------------------------------------------------------------------
 * private session bus
 * ------------------------------------------------------------------------- */

static int socket_is_live(const char *path)
{
    struct sockaddr_un addr;
    int fd;
    int ok;

    if (strlen(path) >= sizeof(addr.sun_path)) {
        return 0;
    }

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        return 0;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    ok = (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0);
    close(fd);
    return ok;
}

/* -------------------------------------------------------------------------
 * locating ourselves inside the bundle
 * ------------------------------------------------------------------------- */

/* argv[0] is not reliable (it is whatever the caller passed), so ask dyld */
static void resolve_self(char *self, size_t n)
{
    char raw[PATH_MAX];
    char resolved[PATH_MAX];
    uint32_t size = sizeof(raw);

    if (_NSGetExecutablePath(raw, &size) != 0) {
        die("cannot determine the path of the executable");
    }
    if (!realpath(raw, resolved)) {
        die("cannot resolve %s: %s", raw, strerror(errno));
    }
    fmtpath(self, n, "%s", resolved);
}

/* dirname()/basename() may modify their argument, so always pass a copy */
static void path_dirname(char *dst, size_t n, const char *path)
{
    char copy[PATH_MAX];

    fmtpath(copy, sizeof(copy), "%s", path);
    fmtpath(dst, n, "%s", dirname(copy));
}

static void path_basename(char *dst, size_t n, const char *path)
{
    char copy[PATH_MAX];

    fmtpath(copy, sizeof(copy), "%s", path);
    fmtpath(dst, n, "%s", basename(copy));
}

/* -------------------------------------------------------------------------
 * debug bundles
 * ------------------------------------------------------------------------- */

#ifdef ART_LAUNCHER_DEBUG

/* When the bundle is built with --debug, relax the sanitizer and send the
 * output to $HOME/<name>.log, because an application started from the Finder
 * has nowhere else to write its diagnostics. The redirection is done with
 * dup2() rather than by piping into tee(1), so that no extra process sits
 * between LaunchServices and the exec'd binary; the file descriptors survive
 * the execv. When there is a terminal attached, we leave the output alone. */
static void setup_debug_output(void)
{
    const char *home = getenv("HOME");
    char log[PATH_MAX];

    setenv("ASAN_OPTIONS",
           "detect_container_overflow=0:new_delete_type_mismatch=0:"
           "halt_on_error=0",
           1);

    if (isatty(STDERR_FILENO) || !home || !*home) {
        return;
    }

    fmtpath(log, sizeof(log), "%s/%s.log", home, prog);
    if (!freopen(log, "w", stdout)) {
        warn("cannot write to %s: %s", log, strerror(errno));
        return;
    }
    if (dup2(STDOUT_FILENO, STDERR_FILENO) < 0) {
        warn("cannot redirect the error output: %s", strerror(errno));
    }
}

#endif // ART_LAUNCHER_DEBUG

/* -------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------- */

int main(int argc, char **argv)
{
    char self[PATH_MAX];     /* .../ART.app/Contents/MacOS/ART   */
    char exedir[PATH_MAX];   /* .../ART.app/Contents/MacOS       */
    char contents[PATH_MAX]; /* .../ART.app/Contents             */
    char name[PATH_MAX];     /* ART                              */
    char target[PATH_MAX];   /* .../ART.app/Contents/MacOS/.ART.bin */
    char tmpdir[PATH_MAX];
    char sock[PATH_MAX];
    char buf[PATH_MAX];
    char pattern[PATH_MAX];
    char loaders[PATH_MAX];
    char immodules[PATH_MAX];
    const char *tmp;
    const char *user;
    const char *debug;
    char **args;
    int nargs;
    int i;

    resolve_self(self, sizeof(self));
    path_dirname(exedir, sizeof(exedir), self);
    path_dirname(contents, sizeof(contents), exedir);
    path_basename(name, sizeof(name), self);

    prog = name;

#ifdef ART_LAUNCHER_DEBUG
    setup_debug_output();
#endif

    fmtpath(target, sizeof(target), "%s/.%s.bin", exedir, name);

    /* the per-user directory holding the session bus socket and the module
     * caches; it is shared by all the instances of the application and is
     * intentionally left behind on exit, so that the next launch can reuse it */
    tmp = getenv("TMPDIR");
    if (!tmp || !*tmp) {
        tmp = "/tmp/";
    }
    user = getenv("USER");
    if (!user || !*user) {
        struct passwd *pw = getpwuid(getuid());
        user = (pw && pw->pw_name) ? pw->pw_name : "art";
    }
    fmtpath(tmpdir, sizeof(tmpdir), "%s%s%s-%s", tmp,
            tmp[strlen(tmp) - 1] == '/' ? "" : "/", name, user);

    if (mkdir(tmpdir, 0700) != 0 && errno != EEXIST) {
        die("cannot create %s: %s", tmpdir, strerror(errno));
    }

    fmtpath(sock, sizeof(sock), "%s/dbus.sock", tmpdir);
    fmtpath(loaders, sizeof(loaders), "%s/loader.cache", tmpdir);
    fmtpath(immodules, sizeof(immodules), "%s/gtk.immodules", tmpdir);

    fmtpath(buf, sizeof(buf), "%s/Frameworks", contents);
    set_env("DYLD_LIBRARY_PATH", buf);
    set_env("GDK_PIXBUF_MODULEDIR", buf);

    set_env("GTK_CSD", "0");

    fmtpath(buf, sizeof(buf), "%s/Resources/fonts.conf", contents);
    set_env("FONTCONFIG_FILE", buf);

    fmtpath(buf, sizeof(buf), "%s/Resources/etc/gtk-3.0", contents);
    set_env("GTK_PATH", buf);

    fmtpath(buf, sizeof(buf), "%s/Resources/share/glib-2.0/schemas", contents);
    set_env("GSETTINGS_SCHEMA_DIR", buf);

    fmtpath(buf, sizeof(buf), "%s/Resources/share", contents);
    set_env("XDG_DATA_DIRS", buf);

    set_env("GDK_PIXBUF_MODULE_FILE", loaders);
    set_env("GTK_IM_MODULE_FILE", immodules);

    fmtpath(buf, sizeof(buf), "unix:path=%s", sock);
    set_env("DBUS_SESSION_BUS_ADDRESS", buf);

    /* not set by us, but it must not leak into the helper programs either */
    save_env("GIO_MODULE_DIR");

    set_env_plain("GDK_RENDERING", "similar");

    fmtpath(buf, sizeof(buf), "%s/Resources/exiftool", contents);
    set_env_plain("ART_EXIFTOOL_BASE_DIR", buf);

    /* start the private session bus if it is not running already, and refresh
     * the module caches while we are at it */
    if (!socket_is_live(sock)) {
        char daemon[PATH_MAX];
        char config[PATH_MAX];
        char address[PATH_MAX];
        char *dbus_args[6];

        unlink(sock);

        fmtpath(daemon, sizeof(daemon), "%s/Resources/dbus-daemon", contents);
        fmtpath(config, sizeof(config),
                "--config-file=%s/Resources/dbus-1/session.conf", contents);
        fmtpath(address, sizeof(address), "--address=unix:path=%s", sock);

        dbus_args[0] = daemon;
        dbus_args[1] = (char *)"--fork";
        dbus_args[2] = (char *)"--print-pid";
        dbus_args[3] = config;
        dbus_args[4] = address;
        dbus_args[5] = NULL;
        run(dbus_args, "/dev/null");

        fmtpath(buf, sizeof(buf), "%s/Resources/gdk-pixbuf-query-loaders",
                contents);
        fmtpath(pattern, sizeof(pattern),
                "%s/Frameworks/libpixbufloader*svg.so", contents);
        run_query_tool(buf, pattern, loaders);

        fmtpath(buf, sizeof(buf), "%s/Resources/gtk-query-immodules-3.0",
                contents);
        fmtpath(pattern, sizeof(pattern), "%s/Frameworks/im-*.so", contents);
        run_query_tool(buf, pattern, immodules);
    }

    /* argv[0] is replaced by the real binary, and the process serial number
     * that LaunchServices may pass is dropped: the exec'd program gets it back
     * from the Apple Event machinery anyway, and it is not a valid ART option */
    args = calloc(argc + 1, sizeof(char *));
    if (!args) {
        die("out of memory");
    }
    nargs = 0;
    args[nargs++] = target;
    for (i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "-psn_", 5) != 0) {
            args[nargs++] = argv[i];
        }
    }
    args[nargs] = NULL;

    debug = getenv("ART_DEBUG");
    if (debug && atoi(debug)) {
        fprintf(stderr, "%s - running:", prog);
        for (i = 0; i < nargs; ++i) {
            fprintf(stderr, " %s", args[i]);
        }
        fputc('\n', stderr);
    }

    execv(target, args);

    die("cannot execute %s: %s", target, strerror(errno));
    return 1;
}
