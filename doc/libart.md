# libART: the server mode as a C library

libART runs the commands of ART's [server mode](server_mode.md) (the ones of
`ART-cli --cmd`: `open`, `apply`, `get-info`, `get-spot-wb`, `preview`, `export`, `get-metadata`, the
history and snapshot commands, ...) from C, and, through a small C ABI, from
Python (a binding is provided) and from any language with a foreign function
interface.

An `ART_engine` executes commands in one of three ways:

| backend | what it does | needs |
|---|---|---|
| **remote** | talks to a running `ART-cli --server` over its Unix socket | nothing but the server (pure C99 code, no dependencies); POSIX only |
| **GUI** | talks to a running ART GUI over D-Bus, as `ART --cmd` does | a running ART (the commands never start one); POSIX only. libART speaks the D-Bus protocol itself (pure C99, no D-Bus or GLib library needed) |
| **in-process** | hosts the server in the calling process: the whole ART engine is inside the library | the data files of ART (`data_dir`); no socket, no other process, so it also works where those are not allowed, e.g. sandboxed apps |

All give the same commands, the same output and the same exit statuses as
`ART-cli --cmd` (the GUI has some more commands, and acts on the image that is
visible in its editor: see `ART --cmd help`); the only difference is that `quit`
is not available in-process (destroy the engine instead).

## The API

The header is `src/libart/ART.h` (C99, usable from C++). It is made to be easy
to bind: only opaque pointers, `int`, `size_t`, `const char *` and one callback
type cross the interface; no structs or enums by value, no varargs, no
thread-local state. All the symbols start with `ART_`, and the shared library
exports nothing else.

```c
#include <ART.h>

ART_engine *e = ART_engine_create(NULL);  // remote, default socket,
if (!e) return 1;                         // output to stdout/stderr
const char *open[] = {"open", "photo.arw", NULL};
const char *spot[] = {"get-spot-wb", "1500", "2000", "20", "20", NULL};
int rc = ART_exec_cmd(e, open);
if (rc == ART_OK) rc = ART_exec_cmd(e, spot);
ART_engine_destroy(e);
```

### Creating an engine

`ART_options` is a builder: create it, set what you need, pass it to
`ART_engine_create()` (which copies it), free it.

```c
ART_options *o = ART_options_new();
ART_options_set_remote(o, "/path/to/server.sock");   // or ART_options_set_gui(...) or ART_options_set_inprocess(...)
ART_options_set_output(o, on_text, my_data);         // command stdout
ART_options_set_error(o, on_text, my_data);          // command stderr
ART_options_set_timeout(o, 30000);                   // remote, gui: milliseconds
ART_engine *e = ART_engine_create(o);     // NULL on failure
if (!e) fprintf(stderr, "%s (%s)\n", ART_options_error(o),
                ART_strerror(ART_options_error_code(o)));
ART_options_free(o);
```

| function | |
|---|---|
| `ART_options_set_remote(o, socket)` | remote backend; `socket` NULL for the default (`$ART_SERVER_SOCKET`, else `$XDG_RUNTIME_DIR/ART/server.sock`, else `$TMPDIR/ART-$USER/server.sock`, as `ART-cli`) |
| `ART_options_set_gui(o, bus)` | GUI backend: the commands go to the running ART GUI, which owns the D-Bus name `us.pixls.art.ART`. `bus` is a D-Bus address (`unix:path=...`, or `unix:abstract=...` on Linux); NULL for the session bus of ART: `$DBUS_SESSION_BUS_ADDRESS`, else `$XDG_RUNTIME_DIR/bus` (Linux), else the private bus the app bundle's launcher starts, `$TMPDIR/ART-$USER/dbus.sock` (macOS; the same places `tools/mcp` looks in). Fails with `ART_ERR_CONNECT` if there is no bus or no GUI on it: a command never starts the GUI. Relative paths are resolved against the client's directory (`ART_options_set_cwd()`), but files written by `preview` and `export` are written by the GUI process |
| `ART_options_set_inprocess(o, data_dir, config_dir, cache_dir)` | in-process backend. `data_dir` holds ART's data files (profiles, luts, lensfun, ...; or set `ART_DATA_DIR`); the other two can be NULL for the defaults. The engine of a process is initialised once, by the first in-process engine: the directories of later ones are ignored |
| `ART_options_set_output(o, fn, user)`, `ART_options_set_error(o, fn, user)` | where the text printed by the commands goes (see below); by default the process's stdout and stderr |
| `ART_options_set_output_file(o, FILE *)`, `..._error_file`, `..._output_fd(o, fd)`, `..._error_fd` | the same for a stdio stream or a file descriptor (never closed by the library) |
| `ART_options_set_cwd(o, dir)` | base of relative paths in commands (default: the current directory at the time of each call) |
| `ART_options_set_timeout(o, ms)` | remote, gui: give up after `ms` (-1 = never, the default). An in-process command cannot be interrupted |
| `ART_options_set_gpu(o, mode)` | in-process: `"auto"`, `"off"`, `"force"` or a device name, as `ART_GPU` (default: from ART's options file) |
| `ART_options_set_verbose(o, level)` | in-process: the engine's own diagnostics (a debugging aid), written by the engine to the process's stdout and stderr; the default, 0, prints nothing |

### Running commands

```c
int ART_exec_cmd(ART_engine *e, const char *const *argv);
```

`argv[0]` is the command and the others its arguments; the array ends with a
`NULL`. Arguments are exact: they can contain spaces, or start with `--`. The
call blocks until the command is done.

What the command prints goes to the output and error streams of the engine,
through the callbacks. As for `ART-cli --cmd`, the output stream receives
exactly one thing, the reply of the command as one line of JSON
(`{"ok":true,"result":...}` or `{"ok":false,"error":"..."}`, see
[server_mode.md](server_mode.md#replies)); the error stream gets the
`ART: error: ...` line of a failed command. Nothing else is written to either
(the in-process engine's own diagnostics are not, see below):

```c
typedef void (*ART_write_fn)(void *user, const char *data, size_t len);
```

`data` is NUL-terminated at `data[len]` and valid only during the call. The
callback is called on the thread that called `ART_exec_cmd()`, before it
returns, once per chunk of output, and must not call into the same engine.

**Return codes.** 0 is success. `ART_ERR_COMMAND` (1) means that the command
ran and failed, like exit status 1 of `ART-cli --cmd`: its `ART: error: ...`
line has gone to the error stream. Any other value is a failure of the
library, with its message in `ART_engine_error(e)` (not on the streams):

| code | meaning |
|---|---|
| `ART_ERR_INVALID` (2) | bad argument (NULL, no command, ...) |
| `ART_ERR_CONNECT` (3) | no server on the socket; for the GUI backend, no bus or no GUI running |
| `ART_ERR_IO` (4) | the connection broke |
| `ART_ERR_PROTOCOL` (5) | the other end does not speak the protocol |
| `ART_ERR_TIMEOUT` (6) | no answer within the timeout |
| `ART_ERR_INIT` (7) | the in-process engine could not be initialised |
| `ART_ERR_UNSUPPORTED` (8) | backend not built in, or not available on this platform |
| `ART_ERR_NOMEM` (9), `ART_ERR_INTERNAL` (10) | |

`ART_strerror(code)` gives a short description of a code.

**Collecting the output.** For callers that would rather not write callbacks,
`ART_exec_cmd_result(e, argv, &result)` runs the command like `ART_exec_cmd`
but collects the text in a result instead of sending it to the streams:
`ART_result_out(r)`, `ART_result_err(r)` (NUL-terminated, never NULL, with
`..._len` variants), `ART_result_free(r)`. The result is set on every return
code except `ART_ERR_INVALID` and `ART_ERR_NOMEM`, with whatever was received.

### Threads

Calls on one engine are serialised by the library, so an engine can be shared
by several threads (a command from another thread waits for the current
one). Different engines are independent; in-process engines share the one
engine of the process but each has its own open image. Do not destroy an
engine while a call on it is running.

### Other functions

`ART_version()`, `ART_abi_version()` (compare with `ART_ABI_VERSION`),
`ART_has_backend(ART_BACKEND_REMOTE | ART_BACKEND_INPROCESS)`.

## Building

libART is part of the CMake build (`-DART_BUILD_LIBART=ON`, the default; with
`-DART_BUILD_GUI=OFF -DART_BUILD_CLI=OFF` it is the only thing built, and GTK is
not needed):

| target | what |
|---|---|
| `art-lib` | the shared library `libART.so` / `libART.dylib` / `ART.dll`; with `ART_LIBART_EMBED=ON` (the default) it contains the in-process backend, so it links the ART engine and its dependencies |
| `art-lib-static` | `libART.a`, remote and GUI backends only (its users would have to link the engine themselves for the other one); define `ART_STATIC_LIB` when using it on Windows |
| `art-lib-cmd` | `art_cmd`, an example: runs one command like `ART-cli --cmd`, with `--socket`, `--gui [BUS]`, `--inprocess --data DIR ...` |

With `-DART_LIBART_EMBED=OFF` the library is just `art_core.c`, `art_remote.c`
and `art_dbus.c`: plain C99, no dependencies, usable on its own (copy the five
files `ART.h`, `art_internal.h`, `art_core.c`, `art_remote.c`, `art_dbus.c`).
`ninja install` installs the library, its header, the documentation and a
pkg-config file (see below); `-DART_INSTALL_LIBART=OFF` leaves them out.

The remote and GUI backends are POSIX only for now (the server is); on Windows
they report `ART_ERR_UNSUPPORTED`.

## The GUI backend

What `ART --cmd VERB ARGS...` does, without GLib: libART connects to the
D-Bus session bus (Unix sockets, `EXTERNAL` authentication), checks that
`us.pixls.art.ART` has an owner, and makes the `CommandLine` call of
GApplication's `org.gtk.Application` interface with `{"ART", "--cmd", verb,
args...}`, the working directory, and `ART_IS_CMD=1`, `ART_CMD_START=0` as the
environment (the GUI tells commands from other command lines by it, and never
starts a new instance for one). The GUI answers by calling `Print` and
`PrintError` on an object that libART exports for the call, which give the two
streams, and then replies with the exit status. One connection per command.

Only that is implemented: no file descriptor passing, no `launchd:` or `tcp:`
bus addresses, no autolaunch of a bus (the Windows case of `tools/mcp`).
Nothing is needed at build time, so the backend is always built (on POSIX).

## The in-process backend

* It runs the very code of `ART-cli --server` (`ServerCore`, `src/engine/remote/servercore.*`),
  with the output going to the callbacks instead of a socket, and without any
  socket, `fork` or thread of its own: everything runs on the calling thread
  (the engine's worker threads never call the callbacks).
* The first in-process engine initialises the engine of the process (options,
  thread pool, GPU context, ...); the last one to be destroyed cleans it up.
  Creating another one afterwards works.
* The engine prints some messages directly on the process's stdout and stderr
  when verbose (`ART_options_set_verbose`); nothing at level 0, which is what
  keeps the process's stdout free for the program.
* Nothing is saved unless the `save` command is run; destroying the engine
  drops unsaved edits.
* Porting ART's own dependencies to a platform (GLib, exiv2, LibRaw, lensfun,
  OpenMP, Vulkan through MoltenVK on iOS, ...) is a separate matter. In
  particular ART runs `exiftool` as a child process for some metadata, which
  platforms that cannot start processes would need to make optional (not
  checked). The library itself adds no such requirement.

## Installation and use

In the bundles (macOS, Windows and Linux; the `tools/*/bundle_ART.py` scripts
copy what `ninja install` puts there and add the libraries that libART needs)
everything is under one relocatable directory, `libART`:

| | Linux, Windows | macOS |
|---|---|---|
| root | `libART/` | `ART.app/Contents/Resources/libART/` |
| library | `libART/lib/libART.so`, `libART/lib/libART.dll` (+ `libART.dll.a`) | `lib/libART.dylib` |
| header | `libART/include/ART.h` | `include/ART.h` |
| pkg-config file | `libART/lib/pkgconfig/ART.pc` | `lib/pkgconfig/ART.pc` |
| documentation | `libART/doc/` (this file and `server_mode.md`) | `doc/` |

For a system install (no bundle) they go to the usual places of the prefix:
`lib/`, `include/`, `share/doc/ART/` and `lib/pkgconfig/`.

The `.pc` file finds its directory by itself (`${pcfiledir}`), so the tree can
be moved. To compile and link a program:

    export PKG_CONFIG_PATH=/path/to/ART/libART/lib/pkgconfig
    cc prog.c $(pkg-config --cflags --libs ART) -o prog       # #include <ART.h>

(it also adds an rpath to the library directory on Linux and macOS).

At run time, the library needs the libraries of ART's bundle when it contains the
in-process backend:

* **Linux:** nothing to do: the library looks for them in the `lib/` directory
  of the bundle (`$ORIGIN/../../lib`).
* **macOS:** the libraries in `Contents/Frameworks` are found through
  `DYLD_LIBRARY_PATH`, as the launchers of `ART` and `ART-cli` do:
  `DYLD_LIBRARY_PATH=/path/to/ART.app/Contents/Frameworks ./prog`. (Their own
  install names point to the build machine's prefix, so the bundle is not
  relocatable without it.)
* **Windows:** the DLLs it depends on are in the bundle's main directory, next
  to `ART.exe`, not next to `libART.dll`: add that directory to `PATH`, or to
  the DLL search path of the program (`SetDllDirectory`, `AddDllDirectory`).

The remote and GUI backends alone have no such requirement.

## Language binding

The Python binding is a thin wrapper (`src/libart/bindings/python/`), meant to
be copied or adapted. It looks for the library in a constructor argument,
`$ART_LIBRARY`, next to the module and in the usual places.

### Getting the final image: `export`

`preview` is a quick, downscaled approximation. To write the real output of the
edited image (the file `ART-cli` would produce) use `export`; it works with both
backends, and the file is written by the process that runs the engine (the
server, for the remote backend):

```c
const char *ex[] = {"export", "/tmp/out.jpg", "--quality", "95", "--overwrite", NULL};
rc = ART_exec_cmd(e, ex);        // prints "/tmp/out.jpg (WxH)" on success
```

Other formats and options (`--format`, `--bits`, `--float`, `--compression`,
...) are described in [server_mode.md](server_mode.md#export-outfile-options);
`export --list-formats` lists what the installation can write. An export takes
seconds to minutes and cannot be interrupted; with the remote backend set a
sufficient `ART_options_set_timeout()` (the default is none).

### Python (`bindings/python/artlib.py`, ctypes, standard library only)

```python
from artlib import Engine

with Engine() as art:                       # default server socket
    art.run("open", "photo.arw", check=True)
    info = art.run("get-info", check=True).data       # parsed "result": a dict
    r = art.run("get-spot-wb", 1500, 2000, 20, 20)    # r.code, r.out, r.err, r.ok
    print(r.data["temperature"])                      # r.reply is the whole document

with Engine(inprocess=True, data_dir="/path/to/data", gpu="off") as art:
    ...
```

`run()` returns a `Result` (`code`, `out`, `err`, `ok`, and the parsed JSON as
`reply`, `data` and `error`); a command that fails
has `code == 1` (or raises `CommandError` with `check=True`); a failure of the
library raises `ArtError`. `on_output=`/`on_error=` callbacks receive the text
as it is printed.

## Tests

* `ninja art-libart-test art-libart-header-test && ctest -R libart`: the remote
  backend against fake servers (partial reads, hang-ups, bad protocol, timeouts,
  argument exactness, threads, ...), and the header compiled as C++.
* `ninja art-libart-dbus-test && ctest -R libart-dbus`: the GUI backend against
  a fake bus and GUI (authentication, the call, the output in pieces, status,
  errors, timeout). The wire format was also checked once against GLib's own
  `GApplication` (a small program with ART's flags and application id, on a
  private `dbus-daemon`).
* In-process, on a real image (the test is skipped unless the variables are set):

      ART_TEST_DATA_DIR=.../ART.app/Contents/Resources ART_TEST_IMAGE=photo.arw \
      ART_TEST_PROFILE=photo.arw.arp ctest -R libart-embed

  (add `DYLD_LIBRARY_PATH` / `LD_LIBRARY_PATH` if the engine's libraries are not
  found). It edits a copy of the image.
* The Python binding: `bindings/python/test_artlib.py` (`remote` needs a running
  server, `gui` a running ART GUI; see the top of the file).
