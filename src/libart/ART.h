/*
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
 * libART: run the commands of ART's server mode (see doc/server_mode.md,
 * "ART-cli --server" and "ART-cli --cmd") from C and from other languages.
 *
 * An ART_engine executes commands either
 *  - by talking to a running `ART-cli --server` over its Unix socket (the
 *    remote backend, always available), or
 *  - in the calling process, which then hosts the server itself (the
 *    in-process backend; check ART_has_backend()): no socket and no other
 *    process are needed, e.g. in sandboxed apps.
 * The commands, their arguments and their output are those of `ART-cli --cmd`.
 *
 * Designed to be bound easily: only opaque pointers, int, size_t, const char *
 * and one callback type cross the interface; there are no structs or enums
 * passed by value, no varargs and no thread-local state. C99.
 *
 * Minimal example:
 *
 *     ART_engine *e = ART_engine_create(NULL);   // remote, default socket,
 *     if (!e) return 1;                          // output to stdout/stderr
 *     const char *open[] = {"open", "photo.arw", NULL};
 *     const char *info[] = {"get-info", NULL};
 *     int rc = ART_exec_cmd(e, open);
 *     if (rc == ART_OK) rc = ART_exec_cmd(e, info);
 *     ART_engine_destroy(e);
 *
 * Threads: calls on one engine are serialised by the library; different
 * engines are independent. A call blocks until the command is complete.
 */

#ifndef ART_LIB_H
#define ART_LIB_H

#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#  if defined(ART_BUILDING_LIB)
#    define ART_API __declspec(dllexport)
#  elif defined(ART_STATIC_LIB)
#    define ART_API
#  else
#    define ART_API __declspec(dllimport)
#  endif
#elif defined(__GNUC__) || defined(__clang__)
#  define ART_API __attribute__((visibility("default")))
#else
#  define ART_API
#endif

/* Bumped when the interface changes incompatibly. */
#define ART_ABI_VERSION 1

typedef struct ART_options ART_options; /* how to create an engine */
typedef struct ART_engine ART_engine;
typedef struct ART_result ART_result; /* the collected output of a command */

/* Receives text printed by a command. `data` is NUL-terminated at data[len]
 * and valid only during the call. It is called on the thread that called
 * ART_exec_cmd(), before that returns, once per chunk of output. It must not
 * call into the same engine. */
typedef void (*ART_write_fn)(void *user, const char *data, size_t len);

/* the backends of ART_has_backend() */
enum { ART_BACKEND_REMOTE = 0, ART_BACKEND_INPROCESS = 1, ART_BACKEND_GUI = 2 };

/* Status codes: plain ints, 0 on success. */
enum {
    ART_OK = 0,
    /* The command ran and failed (like exit status 1 of `ART-cli --cmd`): its
     * "ART: error: ..." line was written to the error stream. */
    ART_ERR_COMMAND = 1,
    /* The rest are failures of the library; the details are in
     * ART_engine_error() (ART_options_error() for ART_engine_create) and are
     * not written to the streams. */
    ART_ERR_INVALID = 2,     /* bad argument */
    ART_ERR_CONNECT = 3,     /* cannot connect to the server, or no GUI running */
    ART_ERR_IO = 4,          /* the connection to the server or bus broke */
    ART_ERR_PROTOCOL = 5,    /* the server does not speak the protocol */
    ART_ERR_TIMEOUT = 6,     /* no answer within the timeout */
    ART_ERR_INIT = 7,        /* the in-process engine cannot be initialised */
    ART_ERR_UNSUPPORTED = 8, /* backend not built in, or not available here */
    ART_ERR_NOMEM = 9,
    ART_ERR_INTERNAL = 10
};

/* ---- options ----------------------------------------------------------
 *
 * Defaults: remote backend, default socket, no timeout, the command's output
 * and error text written to the process's stdout and stderr, relative paths
 * resolved against the current directory of the process.
 * All the setters copy their arguments and return ART_OK, ART_ERR_INVALID
 * (bad argument) or ART_ERR_NOMEM. */

ART_API ART_options *ART_options_new(void); /* NULL if out of memory */
ART_API void ART_options_free(ART_options *opts); /* NULL is ok */

/* Use the remote backend. `socket_path` is that of the server; NULL for the
 * default: $ART_SERVER_SOCKET, else $XDG_RUNTIME_DIR/ART/server.sock, else
 * $TMPDIR/ART-$USER/server.sock (as `ART-cli`). */
ART_API int ART_options_set_remote(ART_options *opts, const char *socket_path);

/* Use the GUI backend: talk to a running ART GUI, as `ART --cmd` does, over
 * D-Bus (no library is needed: libART speaks the protocol itself). A command
 * never starts the GUI: without one running it fails with ART_ERR_CONNECT.
 * `bus_address` is a D-Bus address ("unix:path=..."); NULL for the session
 * bus of ART: $DBUS_SESSION_BUS_ADDRESS, else $XDG_RUNTIME_DIR/bus (Linux),
 * else the private bus of the app bundle, $TMPDIR/ART-$USER/dbus.sock
 * (macOS). The commands are those of the GUI (see doc/libart.md): the verbs
 * of the server mode that act on the active image, and more. POSIX only. */
ART_API int ART_options_set_gui(ART_options *opts, const char *bus_address);

/* Use the in-process backend. `data_dir` is the directory with ART's data
 * files (profiles, luts, ...), `config_dir` the user configuration
 * directory, `cache_dir` the cache directory; any of them can be NULL for the
 * default. The engine of a process is initialised once, by the first
 * in-process engine: the directories of later ones are ignored. */
ART_API int ART_options_set_inprocess(ART_options *opts, const char *data_dir,
                                      const char *config_dir,
                                      const char *cache_dir);

/* Where the text printed by the commands goes. Without these, the process's
 * stdout and stderr. */
ART_API int ART_options_set_output(ART_options *opts, ART_write_fn fn,
                                   void *user);
ART_API int ART_options_set_error(ART_options *opts, ART_write_fn fn,
                                  void *user);
/* ... or a stdio stream or a file descriptor (neither is ever closed by the
 * library). */
ART_API int ART_options_set_output_file(ART_options *opts, FILE *f);
ART_API int ART_options_set_error_file(ART_options *opts, FILE *f);
ART_API int ART_options_set_output_fd(ART_options *opts, int fd);
ART_API int ART_options_set_error_fd(ART_options *opts, int fd);

/* Directory against which relative paths in commands are resolved (the
 * server resolves them for its own working directory otherwise). NULL: the
 * current directory of the process at the time of each call. */
ART_API int ART_options_set_cwd(ART_options *opts, const char *dir);

/* Remote only: give up on a command after `ms` milliseconds (the whole
 * command, connection included) with ART_ERR_TIMEOUT; -1 for no limit (the
 * default). Some commands legitimately take long (opening a large raw). */
ART_API int ART_options_set_timeout(ART_options *opts, int ms);

/* In-process only: the GPU use, "auto", "off", "force" or a device name, as
 * the ART_GPU environment variable; NULL for the setting of the options
 * file. */
ART_API int ART_options_set_gpu(ART_options *opts, const char *mode);

/* In-process only: whether the files made by `export` get the processing
 * parameters (the sidecar's content) embedded in their metadata, as ART's
 * "embed the parameters in the metadata of the output" preference; without
 * this call, the setting of the options file. */
ART_API int ART_options_set_embed_params(ART_options *opts, int on);

/* In-process only: verbosity of the engine's own diagnostics (debugging
 * aid). The engine writes them directly to the process's stdout and stderr,
 * not to the streams of the engine: at the default, 0, the engine prints
 * nothing, so leave it there in a program that uses its own stdout. */
ART_API int ART_options_set_verbose(ART_options *opts, int level);

/* Why the last ART_engine_create() with these options failed: the message
 * ("" if it did not fail), valid until the next call that uses `opts`, and
 * the code (ART_OK if it did not fail). */
ART_API const char *ART_options_error(const ART_options *opts);
ART_API int ART_options_error_code(const ART_options *opts);

/* ---- engines ---------------------------------------------------------- */

/* Create an engine; `opts` can be NULL for the defaults. The options are
 * copied: they can be freed or changed afterwards (the only thing written to
 * `opts` is the failure, see ART_options_error() and
 * ART_options_error_code()). Returns NULL on failure (with `opts` NULL, only
 * if out of memory). */
ART_API ART_engine *ART_engine_create(ART_options *opts);

/* NULL is ok. Not to be called during a call on the same engine. For the
 * in-process backend the image that is open is closed, dropping the changes
 * that were not saved with the `save` command. */
ART_API void ART_engine_destroy(ART_engine *engine);

/* Execute a command: argv[0] is the verb, the others its arguments, the
 * array ends with a NULL pointer (arguments are exact: they can contain
 * spaces or start with "--"). Output goes to the streams of the engine: the
 * output stream gets the reply of the command, one line of JSON
 * ({"ok":true,"result":...} or {"ok":false,"error":"..."}), the error stream
 * the "ART: error: ..." line of a failed command.
 * Returns ART_OK, ART_ERR_COMMAND if the command failed, or another code
 * (see above). */
ART_API int ART_exec_cmd(ART_engine *engine, const char *const *argv);

/* The message of the last failure of the library on this engine, "" if the
 * last call did not fail that way. Valid until the next call on the engine. */
ART_API const char *ART_engine_error(const ART_engine *engine);

/* ---- collecting the output ---------------------------------------------
 *
 * For callers that prefer not to write callbacks: like ART_exec_cmd(), but
 * the text the command printed is collected in a result instead of going to
 * the streams. On every return code (but ART_ERR_INVALID and ART_ERR_NOMEM)
 * *result is set, with whatever was received. Free it with
 * ART_result_free(). */
ART_API int ART_exec_cmd_result(ART_engine *engine, const char *const *argv,
                                ART_result **result);
/* NUL-terminated strings, never NULL; the lengths exclude the NUL. */
ART_API const char *ART_result_out(const ART_result *result);
ART_API size_t ART_result_out_len(const ART_result *result);
ART_API const char *ART_result_err(const ART_result *result);
ART_API size_t ART_result_err_len(const ART_result *result);
ART_API void ART_result_free(ART_result *result); /* NULL is ok */

/* ---- miscellaneous ---------------------------------------------------- */

ART_API const char *ART_strerror(int code); /* short description, static */
ART_API const char *ART_version(void);      /* version of the library */
ART_API int ART_abi_version(void);          /* ART_ABI_VERSION it was built with */
/* 1 if the backend (ART_BACKEND_*) is available in this build, else 0. */
ART_API int ART_has_backend(int backend);

#ifdef __cplusplus
}
#endif

#endif /* ART_LIB_H */
