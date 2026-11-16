"""Python binding of libART (ctypes, standard library only).

Runs the commands of ART's server mode, either against a running
`ART-cli --server` or in the calling process (the in-process backend), with
the same commands and output as `ART-cli --cmd`. The output of every command
is one JSON document, `{"ok": true, "result": ...}` or `{"ok": false,
"error": "..."}`, parsed for you in Result.reply / .data:

    from artlib import Engine

    with Engine() as art:                       # the default server socket
        art.run("open", "photo.arw", check=True)
        info = art.run("get-info", check=True).data   # a dict
        print(info["full_width"], info["model"])

    # in-process (no server, no socket); data_dir holds ART's data files
    with Engine(inprocess=True, data_dir="/path/to/ART/data") as art:
        r = art.run("get-spot-wb", 1500, 2000, 20, 20)
        print(r.data["temperature"])

`run()` returns a Result. A command that fails (exit status 1 of
`ART-cli --cmd`) gives Result.code == 1, its message in Result.error (and
Result.err) (or raises CommandError with check=True); a failure of the library itself (no
server, connection lost, timeout, ...) raises ArtError.

The library is looked for in `library=`, $ART_LIBRARY, next to this file, and
then in the usual places.
"""

import ctypes
import ctypes.util
import json
import os
import sys
import threading
from typing import NamedTuple, Optional

__all__ = ["Engine", "Result", "ArtError", "CommandError", "ART_OK",
           "ART_ERR_COMMAND"]

ART_OK = 0
ART_ERR_COMMAND = 1

_BACKEND_INPROCESS = 1

_WRITE_FN = ctypes.CFUNCTYPE(None, ctypes.c_void_p, ctypes.c_void_p,
                             ctypes.c_size_t)


class ArtError(Exception):
    """A failure of the library: no server, connection lost, timeout, ..."""

    def __init__(self, code, message):
        super().__init__("%s (code %d)" % (message, code))
        self.code = code
        self.message = message


class CommandError(Exception):
    """The command ran and failed (only with run(check=True))."""

    def __init__(self, args, result):
        msg = result.error or "the command failed"
        super().__init__("%s: %s" % (" ".join(map(str, args)), msg))
        self.result = result


class Result(NamedTuple):
    code: int
    out: str
    err: str

    @property
    def ok(self):
        return self.code == ART_OK

    @property
    def reply(self):
        """The JSON document the command printed, parsed (None if there is
        none: a library failure never gets here)."""
        try:
            return json.loads(self.out)
        except ValueError:
            return None

    @property
    def data(self):
        """The "result" of a successful command (None if it returns no data)."""
        r = self.reply
        return r.get("result") if r else None

    @property
    def error(self):
        """The message of a failed command (None on success)."""
        r = self.reply
        if r and r.get("ok") is False:
            return r.get("error")
        return self.err.strip() or None if self.code else None


def _library_names():
    if sys.platform == "darwin":
        return ["libART.dylib"]
    if sys.platform.startswith("win"):
        return ["ART.dll", "libART.dll"]
    return ["libART.so"]


def _load_library(path=None):
    candidates = []
    if path:
        candidates.append(path)
    if os.environ.get("ART_LIBRARY"):
        candidates.append(os.environ["ART_LIBRARY"])
    here = os.path.dirname(os.path.abspath(__file__))
    for n in _library_names():
        candidates.append(os.path.join(here, n))
    found = ctypes.util.find_library("ART")
    if found:
        candidates.append(found)
    candidates.extend(_library_names())
    last = None
    for c in candidates:
        try:
            return ctypes.CDLL(c)
        except OSError as e:
            last = e
    raise OSError("cannot load libART (set ART_LIBRARY): %s" % last)


_lib = None
_lib_lock = threading.Lock()


def _api(path=None):
    """The library, with the signatures of the functions declared."""
    global _lib
    with _lib_lock:
        if _lib is not None:
            return _lib
        lib = _load_library(path)
        vp, ci, cs, st = ctypes.c_void_p, ctypes.c_int, ctypes.c_char_p, ctypes.c_size_t

        def decl(name, res, *args):
            f = getattr(lib, name)
            f.restype = res
            f.argtypes = list(args)

        decl("ART_options_new", vp)
        decl("ART_options_free", None, vp)
        decl("ART_options_set_remote", ci, vp, cs)
        decl("ART_options_set_inprocess", ci, vp, cs, cs, cs)
        decl("ART_options_set_output", ci, vp, _WRITE_FN, vp)
        decl("ART_options_set_error", ci, vp, _WRITE_FN, vp)
        decl("ART_options_set_cwd", ci, vp, cs)
        decl("ART_options_set_timeout", ci, vp, ci)
        decl("ART_options_set_gpu", ci, vp, cs)
        decl("ART_options_set_verbose", ci, vp, ci)
        decl("ART_options_error", cs, vp)
        decl("ART_options_error_code", ci, vp)
        decl("ART_engine_create", vp, vp)
        decl("ART_engine_destroy", None, vp)
        decl("ART_exec_cmd", ci, vp, ctypes.POINTER(cs))
        decl("ART_engine_error", cs, vp)
        decl("ART_exec_cmd_result", ci, vp, ctypes.POINTER(cs),
             ctypes.POINTER(vp))
        decl("ART_result_out", vp, vp)
        decl("ART_result_out_len", st, vp)
        decl("ART_result_err", vp, vp)
        decl("ART_result_err_len", st, vp)
        decl("ART_result_free", None, vp)
        decl("ART_strerror", cs, ci)
        decl("ART_version", cs)
        decl("ART_abi_version", ci)
        decl("ART_has_backend", ci, ci)
        _lib = lib
        return lib


def _enc(s):
    return None if s is None else os.fsencode(s)


def _text(b):
    return b.decode("utf-8", "replace")


class Engine:
    """A connection to a server, or an in-process engine.

    socket      path of the server's socket (default: the usual one)
    inprocess   True to run the engine in this process instead
    data_dir, config_dir, cache_dir, gpu    for inprocess (see art.h)
    cwd         base of relative paths in commands (default: the cwd)
    timeout     seconds, remote only (default: none)
    verbose     diagnostics of the in-process engine (debugging; they go to the
                process's stdout/stderr; 0, the default, prints nothing)
    on_output, on_error    called with each chunk of text (str) as the
                command prints it; the text is also in the Result
    library     path of libART (see the module docstring)
    """

    def __init__(self, *, socket=None, inprocess=False, data_dir=None,
                 config_dir=None, cache_dir=None, gpu=None, cwd=None,
                 timeout=None, verbose=0, on_output=None, on_error=None,
                 library=None):
        self._api = _api(library)
        self._engine = None
        self._lock = threading.Lock()
        self._on_output = on_output
        self._on_error = on_error
        self._cur_out = None  # the chunks of the command being run
        self._cur_err = None

        # the callbacks must stay referenced as long as the engine exists
        self._cb_out = _WRITE_FN(self._write_out)
        self._cb_err = _WRITE_FN(self._write_err)

        a = self._api
        opts = a.ART_options_new()
        if not opts:
            raise MemoryError("ART_options_new")
        try:
            self._check(a.ART_options_set_output(opts, self._cb_out, None))
            self._check(a.ART_options_set_error(opts, self._cb_err, None))
            if inprocess:
                self._check(a.ART_options_set_inprocess(
                    opts, _enc(data_dir), _enc(config_dir), _enc(cache_dir)))
            else:
                self._check(a.ART_options_set_remote(opts, _enc(socket)))
            if cwd is not None:
                self._check(a.ART_options_set_cwd(opts, _enc(cwd)))
            if timeout is not None:
                self._check(a.ART_options_set_timeout(opts, int(timeout * 1000)))
            if gpu is not None:
                self._check(a.ART_options_set_gpu(opts, _enc(gpu)))
            self._check(a.ART_options_set_verbose(opts, int(verbose)))
            handle = a.ART_engine_create(opts)
            if not handle:
                rc = a.ART_options_error_code(opts) or 10  # ART_ERR_INTERNAL
                msg = a.ART_options_error(opts) or a.ART_strerror(rc)
                raise ArtError(rc, _text(msg))
            self._engine = ctypes.c_void_p(handle)
        finally:
            a.ART_options_free(opts)

    # -- the callbacks of the library: called on the thread that runs a command

    def _write(self, data, length, chunks, callback):
        text = _text(ctypes.string_at(data, length))
        if chunks is not None:
            chunks.append(text)
        if callback is not None:
            callback(text)

    def _write_out(self, user, data, length):
        self._write(data, length, self._cur_out, self._on_output)

    def _write_err(self, user, data, length):
        self._write(data, length, self._cur_err, self._on_error)

    def _check(self, rc):
        if rc != ART_OK:
            raise ArtError(rc, _text(self._api.ART_strerror(rc)))

    # -- commands

    def run(self, *args, check=False):
        """Run a command: run("open", "photo.arw"). The arguments can be
        anything with a str() (numbers, paths). Returns a Result."""
        if self._engine is None:
            raise ArtError(2, "the engine is closed")
        if not args:
            raise ValueError("no command given")
        argv = [os.fsencode(os.fspath(a) if isinstance(a, os.PathLike) else str(a))
                for a in args]
        for a in argv:
            if b"\0" in a:
                raise ValueError("an argument contains a NUL")
        carr = (ctypes.c_char_p * (len(argv) + 1))(*argv, None)
        with self._lock:
            self._cur_out, self._cur_err = [], []
            try:
                rc = self._api.ART_exec_cmd(self._engine, carr)
                out, err = "".join(self._cur_out), "".join(self._cur_err)
            finally:
                self._cur_out = self._cur_err = None
            if rc not in (ART_OK, ART_ERR_COMMAND):
                msg = self._api.ART_engine_error(self._engine)
                raise ArtError(rc, _text(msg) or _text(self._api.ART_strerror(rc)))
        result = Result(rc, out, err)
        if check and not result.ok:
            raise CommandError(args, result)
        return result

    def close(self):
        """Destroy the engine (an in-process engine drops unsaved edits)."""
        if self._engine is not None:
            self._api.ART_engine_destroy(self._engine)
            self._engine = None

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()
        return False

    def __del__(self):
        try:
            self.close()
        except Exception:
            pass


def version():
    """The version of the loaded library."""
    return _text(_api().ART_version())


def has_inprocess_backend():
    return bool(_api().ART_has_backend(_BACKEND_INPROCESS))
