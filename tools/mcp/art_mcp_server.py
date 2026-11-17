#!/usr/bin/env python3
"""MCP server that lets an LLM drive a running ART instance.

It talks to ART through the `ART --cmd ...` remote-command mode (so ART must
already be running; the commands fail otherwise) and needs no third-party
packages: the MCP stdio transport (newline-delimited JSON-RPC 2.0) is
implemented here directly.

Two backends offer the same commands on the active image:
  gui     (default) the running ART GUI, through `ART --cmd` (D-Bus)
  server  a headless `ART-cli --server`, through `ART-cli --cmd` (a Unix
          socket); no GUI and no D-Bus. The file browser, rating, colour label
          and session tools are not available, and `art_save` and `art_export` are.

Environment:
  ART_MCP_BACKEND  gui (default) or server
  ART_BIN   path of the ART GUI executable (default: auto-detected)
  ART_CLI_BIN  path of the ART-cli executable (server backend; default:
            next to ART_BIN, the usual install locations, the build tree, PATH)
  ART_SERVER_SOCKET  socket of the server, if not the default one (server
            backend; ART-cli reads it itself)
  ART_MCP_TIMEOUT  seconds to wait for a command (default 120)

Run `python3 gen_keys.py` after the profile format changes to refresh
known_keys.json (the list of groups/keys accepted in edits).
"""

import base64
import difflib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REF_DIR = os.path.join(HERE, "reference")
SERVER_NAME = "art-mcp"
SERVER_VERSION = "0.1.0"

INSTRUCTIONS = """\
You control the raw photo editor ART (a running instance) through these tools.

Typical workflow for "edit this photo like <description>":
 1. art_status, then art_open_image (or art_open_directory + art_open_image).
 2. art_get_info (image size, camera, ISO, raw or not) and art_get_edit (current
    parameters) -- never guess the current state.
 3. art_reference (no arguments = index; or group="Exposure", ...) to read what
    the relevant tools do and which keys/ranges they accept BEFORE writing values.
 4. art_apply_edit with {"Group": {"Key": value}} (only the keys you want to
    change; everything else is left untouched).
 5. art_preview to LOOK at the result (it returns an image); iterate from 4.
Every change is a History entry: art_get_history lists them, art_undo / art_redo /
art_history_goto move through them. Before trying something risky or when the user
likes a state, save it with art_snapshot (operation "add"); art_get_snapshots lists
them and art_snapshot "activate" returns to one.
Make small, justified changes, one concern at a time, and check the preview.
Edits are applied to the image currently visible in the editor and are
undoable from ART's History panel. They are saved to the .arp sidecar when the
image is closed or the editor tab is left.
art_get_spot_wb measures the white balance of a rectangle that should be neutral
(a grey card, a white wall); set it with art_apply_edit {"White Balance": ...}.
art_get_clut_params lists the parameters of a LUT or CTL script, to fill
Film Simulation's ClutParams.
art_export writes the final full-size file (art_preview is only an approximation).
"""

INSTRUCTIONS_SERVER = """\
You control the raw photo editor ART through a headless ART-cli server: it keeps
one image open (the active image) and every tool acts on it.

Typical workflow for "edit this photo like <description>":
 1. art_status, then art_open_image.
 2. art_get_info (image size, camera, ISO, raw or not) and art_get_edit (current
    parameters) -- never guess the current state.
 3. art_reference (no arguments = index; or group="Exposure", ...) to read what
    the relevant tools do and which keys/ranges they accept BEFORE writing values.
 4. art_apply_edit with {"Group": {"Key": value}} (only the keys you want to
    change; everything else is left untouched).
 5. art_preview to LOOK at the result (it returns an image); iterate from 4.
Every change is a History entry: art_get_history lists them, art_undo / art_redo /
art_history_goto move through them. Before trying something risky or when the user
likes a state, save it with art_snapshot (operation "add"); art_get_snapshots lists
them and art_snapshot "activate" returns to one.
Make small, justified changes, one concern at a time, and check the preview.
Nothing is written to disk until art_save, which writes the edits and the
snapshots to the image's .arp sidecar; opening another image drops unsaved edits.
art_get_spot_wb measures the white balance of a rectangle that should be neutral
(a grey card, a white wall); set it with art_apply_edit {"White Balance": ...}.
art_get_clut_params lists the parameters of a LUT or CTL script, to fill
Film Simulation's ClutParams.
"""


# --------------------------------------------------------------------------
# running ART
# --------------------------------------------------------------------------

class ArtError(Exception):
    pass


IS_WIN = sys.platform.startswith("win")
IS_MAC = sys.platform == "darwin"


def _running_art():
    """Executable of a running ART instance, if any.

    macOS / Linux bundles run the real binary as <dir>/.ART.bin behind a launcher
    script (<dir>/ART) that prepares the environment: use the launcher. On Windows
    the process is ART.exe; it must be the *same install* as the running
    instance (the session bus is per install path)."""
    if IS_WIN:
        try:
            out = subprocess.run(
                ["powershell", "-NoProfile", "-Command",
                 "(Get-Process -Name ART -ErrorAction SilentlyContinue).Path"],
                capture_output=True, text=True, timeout=15).stdout
        except (OSError, subprocess.SubprocessError):
            return None
        for line in out.splitlines():
            if line.strip() and os.path.exists(line.strip()):
                return line.strip()
        return None
    try:
        ps = subprocess.run(["ps", "-axo", "command="], capture_output=True,
                            text=True, timeout=10).stdout
    except (OSError, subprocess.SubprocessError):
        return None
    for line in ps.splitlines():
        m = re.match(r"^(.*?)/\.ART\.bin(\s|$)", line)
        if m and os.access(m.group(1) + "/ART", os.X_OK):
            return m.group(1) + "/ART"
    for line in ps.splitlines():  # no launcher: a plain binary
        m = re.match(r"^(\S*/ART\.app/Contents/MacOS/ART|\S*/src/gui/ART)(\s|$)", line)
        if m and os.access(m.group(1), os.X_OK):
            return m.group(1)
    return None


def _candidates():
    repo = os.path.abspath(os.path.join(HERE, "..", ".."))
    if IS_WIN:
        for var in ("ProgramFiles", "ProgramFiles(x86)", "LOCALAPPDATA"):
            base = os.environ.get(var)
            if base:
                yield os.path.join(base, "ART", "ART.exe")
                yield os.path.join(base, "Programs", "ART", "ART.exe")
        return
    yield os.path.expanduser("~/Applications/ART.app/Contents/MacOS/ART")
    yield "/Applications/ART.app/Contents/MacOS/ART"
    for d in ("/opt/ART", "/usr/local/ART", os.path.expanduser("~/ART"),
              os.path.expanduser("~/.local/ART")):
        yield os.path.join(d, "ART")
    yield os.path.join(repo, "build", "gpu", "Release", "ART.app", "Contents", "MacOS", "ART")


def find_art():
    """ART_BIN, else the running instance, else an installed copy, else the
    build tree (the least likely to have a bus), else PATH."""
    cand = os.environ.get("ART_BIN")
    if cand:
        return cand
    running = _running_art()
    if running:
        return running
    for p in _candidates():
        if os.access(p, os.X_OK):
            return p
    w = shutil.which("ART")
    if w:
        return w
    raise ArtError("cannot find the ART executable; set ART_BIN")


def backend():
    b = os.environ.get("ART_MCP_BACKEND", "gui").strip().lower() or "gui"
    if b not in ("gui", "server"):
        raise ArtError("ART_MCP_BACKEND must be 'gui' or 'server', not %r" % b)
    return b


def _cli_candidates():
    repo = os.path.abspath(os.path.join(HERE, "..", ".."))
    exe = "ART-cli.exe" if IS_WIN else "ART-cli"
    if IS_WIN:
        for var in ("ProgramFiles", "ProgramFiles(x86)", "LOCALAPPDATA"):
            base = os.environ.get(var)
            if base:
                yield os.path.join(base, "ART", exe)
                yield os.path.join(base, "Programs", "ART", exe)
        return
    yield os.path.expanduser("~/Applications/ART.app/Contents/MacOS/" + exe)
    yield "/Applications/ART.app/Contents/MacOS/" + exe
    for d in ("/opt/ART", "/usr/local/ART", os.path.expanduser("~/ART"),
              os.path.expanduser("~/.local/ART")):
        yield os.path.join(d, exe)
    yield os.path.join(repo, "build", "gpu", "Release", "ART.app", "Contents", "MacOS", exe)


def find_cli():
    """ART_CLI_BIN, else the ART-cli next to ART_BIN, else an installed copy,
    else the build tree, else PATH."""
    exe = "ART-cli.exe" if IS_WIN else "ART-cli"
    cand = os.environ.get("ART_CLI_BIN")
    if cand:
        return cand
    art = os.environ.get("ART_BIN")
    if art:
        p = os.path.join(os.path.dirname(art), exe)
        if os.access(p, os.X_OK):
            return p
    for p in _cli_candidates():
        if os.access(p, os.X_OK):
            return p
    w = shutil.which("ART-cli")
    if w:
        return w
    raise ArtError("cannot find the ART-cli executable; set ART_CLI_BIN")


def require_gui(what):
    if backend() != "gui":
        raise ArtError("%s needs the ART GUI (ART_MCP_BACKEND=gui)" % what)


def _is_bundle_launcher(path):
    """The macOS bundle launcher exports its own DBUS_SESSION_BUS_ADDRESS."""
    return IS_MAC and os.path.exists(os.path.join(os.path.dirname(path), ".ART.bin"))


def bus_address(art):
    """D-Bus address to give `ART --cmd`, or None to leave the environment alone.

    * Windows: nothing to do (GLib autolaunches a per-install bus via gdbus.exe).
    * macOS: the app launcher sets it; a bare binary falls back to the launcher's
      per-user socket <TMPDIR>/ART-<user>/dbus.sock.
    * Linux: the desktop session's bus (DBUS_SESSION_BUS_ADDRESS, normally
      inherited); if missing, the systemd default $XDG_RUNTIME_DIR/bus.
    """
    if IS_WIN or os.environ.get("DBUS_SESSION_BUS_ADDRESS") or _is_bundle_launcher(art):
        return None
    if IS_MAC:
        try:
            import getpass
            user = os.environ.get("USER") or getpass.getuser()
        except Exception:
            user = ""
        for d in (user_tmpdir() or tempfile.gettempdir(), "/tmp"):
            sock = os.path.join(d, "ART-%s" % user, "dbus.sock")
            if os.path.exists(sock):
                return "unix:path=" + sock
        return None
    rt = os.environ.get("XDG_RUNTIME_DIR") or "/run/user/%d" % os.getuid()
    sock = os.path.join(rt, "bus")
    return "unix:path=" + sock if os.path.exists(sock) else None


def user_tmpdir():
    """The per-user temp dir ($TMPDIR in a normal login session). GUI clients such as
    Claude Desktop start MCP servers without it, but the app launcher keeps its D-Bus
    socket in <TMPDIR>/ART-<user>/, so the child needs the same value ART was
    started with."""
    t = os.environ.get("TMPDIR")
    if t:
        return t
    if not IS_MAC:
        return None
    try:
        t = os.confstr("CS_DARWIN_USER_TEMP_DIR")
    except (AttributeError, ValueError, OSError):
        t = ""
    if not t:
        try:
            t = subprocess.run(["/usr/bin/getconf", "DARWIN_USER_TEMP_DIR"],
                               capture_output=True, text=True, timeout=5).stdout.strip()
        except (OSError, subprocess.SubprocessError):
            t = ""
    return t or None


def run_cmd(*args, timeout=None):
    """Run `ART --cmd args...`. Every command prints one JSON document on
    stdout, {"ok": true, "result": ...} or {"ok": false, "error": "..."}:
    return its "result" (None if there is none), or raise ArtError."""
    timeout = timeout or float(os.environ.get("ART_MCP_TIMEOUT", "120"))
    env = dict(os.environ)
    if IS_MAC and not env.get("TMPDIR") and user_tmpdir():
        # also where the server looks for its default socket
        env["TMPDIR"] = user_tmpdir()
    if backend() == "server":
        art = find_cli()  # no D-Bus: ART-cli talks to the server's socket
    else:
        art = find_art()
        addr = bus_address(art)
        if addr:
            env["DBUS_SESSION_BUS_ADDRESS"] = addr
    try:
        p = subprocess.run([art, "--cmd"] + [str(a) for a in args],
                           capture_output=True, timeout=timeout,
                           env=env, cwd=tempfile.gettempdir())
    except subprocess.TimeoutExpired:
        raise ArtError("timeout waiting for ART")
    except OSError as e:
        raise ArtError("cannot run %s: %s" % (art, e))
    stdout = p.stdout.decode("utf-8", "replace")
    try:
        reply = json.loads(stdout)
    except ValueError:
        # not a reply of the command mode: the exit status and stderr tell more
        err = [l for l in p.stderr.decode("utf-8", "replace").splitlines()
               if l.startswith("ART: error:")]
        raise ArtError((err[-1].replace("ART: error: ", "") if err else
                        "ART command failed (exit %d)" % p.returncode))
    if not isinstance(reply, dict) or reply.get("ok") is not True:
        raise ArtError(reply.get("error") or "ART command failed"
                       if isinstance(reply, dict) else "unexpected reply of ART")
    return reply.get("result")


# --------------------------------------------------------------------------
# profile (.arp) writing and validation
# --------------------------------------------------------------------------

def _load_keys():
    try:
        with open(os.path.join(HERE, "known_keys.json")) as f:
            return json.load(f)
    except OSError:
        return {"groups": {}, "dynamic": []}


KNOWN = _load_keys()
# the sidecar groups an LLM must not write (not image edits)
FORBIDDEN_GROUPS = {"Version"}


def _norm(s):
    return re.sub(r"[\s_\-]+", "", s).lower()


def resolve_group(name):
    groups = KNOWN["groups"]
    if name in groups:
        return name
    by_norm = {_norm(g): g for g in groups}
    if _norm(name) in by_norm:
        return by_norm[_norm(name)]
    if not groups:
        return name  # no key list available: do not validate
    close = difflib.get_close_matches(name, list(groups), n=4, cutoff=0.5)
    raise ArtError("unknown group [%s]%s" % (
        name, "; did you mean: " + ", ".join(close) if close else
        " (see art_reference for the list of groups)"))


def resolve_key(group, key, notes):
    keys = KNOWN["groups"].get(group)
    if keys is None or key in keys:
        return key
    by_norm = {_norm(k): k for k in keys}
    if _norm(key) in by_norm:
        return by_norm[_norm(key)]
    if group in KNOWN["dynamic"]:
        note = ("[%s]: keys that are not fixed ones (e.g. %s) were passed through "
                "unchecked, because this group has indexed per-region keys; "
                "double-check their spelling against art_reference" % (group, key))
        if not any(n.startswith("[%s]: keys that" % group) for n in notes):
            notes.append(note)
        return key
    close = difflib.get_close_matches(key, keys, n=4, cutoff=0.5)
    raise ArtError("unknown key %s in [%s]%s" % (
        key, group, "; did you mean: " + ", ".join(close) if close else
        "; valid keys: " + ", ".join(keys)))


def fmt_value(v):
    if isinstance(v, bool):
        return "true" if v else "false"
    if isinstance(v, int):
        return str(v)
    if isinstance(v, float):
        return repr(round(v, 6))
    if isinstance(v, (list, tuple)):
        return ";".join(fmt_value(x) for x in v) + ";"
    if isinstance(v, str):
        if "\n" in v:
            raise ArtError("values cannot contain newlines")
        return v
    raise ArtError("unsupported value %r" % (v,))


def edit_to_arp(edit):
    """{'Group': {'Key': value}} -> (arp text, notes)."""
    if not isinstance(edit, dict) or not edit:
        raise ArtError("`edit` must be a non-empty object {group: {key: value}}")
    notes, lines = [], []
    for g, kv in edit.items():
        group = resolve_group(g)
        if group in FORBIDDEN_GROUPS:
            raise ArtError("group [%s] cannot be edited" % group)
        if not isinstance(kv, dict) or not kv:
            raise ArtError("[%s] must map to a non-empty object of keys" % g)
        items = {}
        for k, v in kv.items():
            items[resolve_key(group, k, notes)] = v
        if ("Enabled" in KNOWN["groups"].get(group, []) and "Enabled" not in items):
            items = dict([("Enabled", True)] + list(items.items()))
            notes.append("[%s]: added Enabled=true" % group)
        lines.append("[%s]" % group)
        for k, v in items.items():
            lines.append("%s=%s" % (k, fmt_value(v)))
        lines.append("")
    return "\n".join(lines), notes


def validate_arp(text):
    """Check the group/key names of a hand-written .arp fragment."""
    notes, group = [], None
    for n, line in enumerate(text.splitlines(), 1):
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        m = re.match(r"^\[(.+)\]$", line)
        if m:
            group = resolve_group(m.group(1))
            if group in FORBIDDEN_GROUPS:
                raise ArtError("line %d: group [%s] cannot be edited" % (n, group))
            continue
        if "=" not in line or group is None:
            raise ArtError("line %d: expected [Group] or Key=Value" % n)
        resolve_key(group, line.split("=", 1)[0].strip(), notes)
    return notes


def apply_arp_text(text, append):
    fd, path = tempfile.mkstemp(suffix=".arp", prefix="art_mcp_")
    try:
        with os.fdopen(fd, "w") as f:
            f.write(text)
        args = ["apply"] + (["--append"] if append else []) + [path]
        run_cmd(*args)
    finally:
        try:
            os.unlink(path)
        except OSError:
            pass


# --------------------------------------------------------------------------
# reference documentation
# --------------------------------------------------------------------------

def _ref_files():
    try:
        return sorted(f for f in os.listdir(REF_DIR)
                      if f.endswith(".md") and f != "FORMAT.md")
    except OSError:
        return []


def _sections():
    """[(file, heading, body)] for every `## ` / `# ` section of the reference."""
    out = []
    for fn in _ref_files():
        with open(os.path.join(REF_DIR, fn), encoding="utf-8") as f:
            text = f.read()
        parts = re.split(r"(?m)^(#{1,2} .+)$", text)
        head = parts[0]
        for i in range(1, len(parts), 2):
            out.append((fn, parts[i].lstrip("# ").strip(), parts[i] + parts[i + 1]))
        if head.strip():
            out.append((fn, fn, head))
    return out


def reference(group=None, topic=None):
    secs = _sections()
    if not secs:
        raise ArtError("reference documentation not found in " + REF_DIR)
    if not group and not topic:
        titles = "\n".join("- %s  (%s)" % (h, fn) for fn, h, _ in secs)
        idx = [b for fn, h, b in secs if "tool index" in h.lower()]
        return ((idx[0] + "\n\n") if idx else "") + \
            "All reference sections (pass group=<name> or topic=<text>):\n" + titles
    q = _norm(group or topic)
    hits = []
    if group:
        # the exact `[Group]` heading first; "Crop" must not match "PCVignette (post-crop)"
        pat = re.compile(r"\[\s*%s\s*\]" % re.escape(group.strip("[] ")), re.I)
        hits = [(fn, h, b) for fn, h, b in secs if pat.search(h)]
        if not hits:
            hits = [(fn, h, b) for fn, h, b in secs if _norm(h) == q or
                    _norm(h.split("--")[0].split("\u2014")[0]) == q]
    if not hits:
        hits = [(fn, h, b) for fn, h, b in secs if q in _norm(h)]
    if not hits and group:
        # a group name may appear in a heading as `[Name]`; also try bodies
        hits = [(fn, h, b) for fn, h, b in secs if q in _norm(b[:400])]
    if not hits:
        raise ArtError("nothing found for %r; call art_reference with no arguments "
                       "for the index" % (group or topic))
    return "\n\n".join(b.strip() for _, _, b in hits)


# --------------------------------------------------------------------------
# tools
# --------------------------------------------------------------------------

def _kv_text(d):
    """`key: value` lines of a JSON object."""
    def one(v):
        return str(v).lower() if isinstance(v, bool) else str(v)
    return "\n".join("%s: %s" % (k, one(v)) for k, v in d.items())


def _history_state(r):
    """One line about the history after undo/redo/history-goto."""
    if r["selected"] < 0:
        return "history: %d entries, none selected; active snapshot: %s" % (
            r["count"], r["snapshot"] or "(none)")
    return "history: entry %d of %d (%s) selected" % (
        r["selected"], r["count"] - 1, r["text"])


def t_status_server(a):
    out = []
    try:
        cli = find_cli()
    except ArtError as e:
        return [text("ART-cli not found: %s" % e)]
    out.append("Backend: ART-cli server (executable %s, socket %s)" % (
        cli, os.environ.get("ART_SERVER_SOCKET") or "default"))
    try:
        run_cmd("ping", timeout=20)
        files = run_cmd("get-files", timeout=20)
        out.append("The server is running. " + (
            "Active image: %s" % files[0] if files else "No image is open."))
    except ArtError as e:
        out.append("The server is NOT reachable: %s" % e)
        out.append("Start it with `ART-cli --server` (in the same environment, so that "
                   "it uses the same socket).")
    return [text("\n".join(out))]


def t_status(a):
    if backend() == "server":
        return t_status_server(a)
    art = find_art()
    out = ["ART executable: %s (D-Bus: %s)" % (
        art, bus_address(art) or os.environ.get("DBUS_SESSION_BUS_ADDRESS") or
        "set by the app launcher" if _is_bundle_launcher(art) else
        "handled by GLib" if IS_WIN else "none found")]
    try:
        d = run_cmd("get-dir", timeout=20)["directory"]
        out.append("ART is running. Directory in the file browser: %s" % (d or "(none)"))
        files = run_cmd("get-files", timeout=20)
        out.append("Open in the editor (visible first):\n  " + "\n  ".join(files)
                   if files else "Nothing is open in the editor.")
    except ArtError as e:
        out.append("ART is NOT reachable: %s" % e)
        if IS_MAC:
            out.append("(TMPDIR used: %s)" % (user_tmpdir() or "unset"))
        out.append("Start the ART GUI first (the same installation as ART_BIN). It needs a "
                   "D-Bus session bus: the macOS app and Windows build start one "
                   "themselves; on Linux the desktop session provides it.")
    return [text("\n".join(out))]


def t_open_directory(a):
    require_gui("art_open_directory")
    run_cmd("open-dir", a["path"])
    return [text("Opened %s in the file browser." % a["path"])]


def t_get_directory(a):
    require_gui("art_get_directory")
    return [text(run_cmd("get-dir")["directory"] or "(none)")]


def t_open_image(a):
    run_cmd("open", a["path"])
    return [text("Opened %s in the editor." % a["path"])]


def t_list_open(a):
    r = run_cmd("get-files")
    return [text("\n".join(r) or "No image is open in the editor.")]


def t_info(a):
    return [text(_kv_text(run_cmd("get-info")))]


def t_get_edit(a):
    arp = run_cmd("get-params")["params"]
    groups = a.get("groups")
    if groups:
        want = {_norm(g) for g in groups}
        keep, cur = [], None
        for line in arp.splitlines():
            m = re.match(r"^\[(.+)\]$", line.strip())
            if m:
                cur = _norm(m.group(1)) in want
            if cur:
                keep.append(line)
        arp = "\n".join(keep) if keep else "(none of the requested groups is present)"
    return [text(arp)]


def t_apply_edit(a):
    append = bool(a.get("append", False))
    notes = []
    if a.get("arp"):
        notes += validate_arp(a["arp"])
        body = a["arp"]
    else:
        body, notes = edit_to_arp(a.get("edit"))
    apply_arp_text(body, append)
    msg = "Applied (%s mode):\n%s" % ("append" if append else "overwrite", body.strip())
    if notes:
        msg += "\n\nNotes:\n- " + "\n- ".join(notes)
    msg += "\n\nCall art_preview to check the result."
    return [text(msg)]


def t_apply_profile(a):
    run_cmd(*(["apply"] + (["--append"] if a.get("append") else []) + [a["path"]]))
    return [text("Applied profile %s." % a["path"])]


def t_preview(a):
    fmt = a.get("format", "jpeg")
    ext = "png" if fmt == "png" else "jpg"
    crop = a.get("crop")
    scale = a.get("scale")
    if scale is None:
        info = run_cmd("get-info")
        try:
            w, h = float(info["full_width"]), float(info["full_height"])
        except (KeyError, ValueError, TypeError):
            w = h = 4000.0
        if crop:
            w, h = float(crop["w"]), float(crop["h"])
        scale = min(1.0, float(a.get("max_size", 1280)) / max(w, h))
    args = ["preview", None, "--scale", "%.6g" % scale]
    if crop:
        args += ["--crop", "%d,%d,%d,%d" % (crop["x"], crop["y"], crop["w"], crop["h"])]
    fd, path = tempfile.mkstemp(suffix="." + ext, prefix="art_mcp_prev_")
    os.close(fd)
    try:
        args[1] = path
        msg = run_cmd(*args)
        with open(path, "rb") as f:
            data = f.read()
    finally:
        try:
            os.unlink(path)
        except OSError:
            pass
    dims = "%dx%d" % (msg["width"], msg["height"]) if msg else ""
    return [text("Preview %s%s, scale %.4g." % (dims,
                                                 " of the crop" if crop else "", scale)),
            {"type": "image", "data": base64.b64encode(data).decode(),
             "mimeType": "image/png" if ext == "png" else "image/jpeg"}]


def t_spot_wb(a):
    r = run_cmd("get-spot-wb", a["x"], a["y"], a["w"], a["h"])
    try:
        temp, tint = "%.6g" % r["temperature"], "%.6g" % r["tint"]
        mul = ["%.6g" % r[k] for k in ("red_multiplier", "green_multiplier",
                                        "blue_multiplier")]
    except (KeyError, TypeError):
        raise ArtError("unexpected reply of get-spot-wb")
    return [text("White balance that makes the rectangle (x=%d, y=%d, w=%d, h=%d) neutral:\n"
                 "  Temperature: %s\n  Tint (Green): %s\n"
                 "  channel multipliers (red, green, blue; green = 1): %s\n"
                 "To use it: art_apply_edit {\"White Balance\": {\"Setting\": \"CustomTemp\", "
                 "\"Temperature\": %d, \"Green\": %s}}"
                 % (a["x"], a["y"], a["w"], a["h"], temp, tint, ", ".join(mul),
                    int(round(float(temp))), tint))]


def t_get_clut_params(a):
    path = a["path"]
    if not os.path.isabs(path):
        raise ArtError("path must be absolute: %s" % path)
    r = run_cmd("get-clut-params", path)
    if not isinstance(r, dict) or "params" not in r:
        raise ArtError("unexpected reply of get-clut-params")
    if not r["params"] and not r.get("presets"):
        return [text("%s has no parameters (a plain LUT, or a file ART cannot read)." % path)]
    return [text("Parameters of %s. Set them in the profile as "
                 "{\"Film Simulation\": {\"ClutParams\": \"name=v1,v2;name2=v\"}} (see "
                 "art_reference); `default` is the list of values of a parameter, and a "
                 "preset gives the values it sets.\n%s"
                 % (path, json.dumps(r, ensure_ascii=False, indent=1)))]


def t_save(a):
    return [text("Saved the edits and snapshots to %s." % run_cmd("save")["file"])]


def t_export(a):
    path = a["path"]
    if not os.path.isabs(path):
        raise ArtError("path must be absolute: %s" % path)
    args = ["export", path]
    for key, opt in (("format", "--format"), ("quality", "--quality"),
                     ("subsampling", "--subsampling"), ("bits", "--bits"),
                     ("compression", "--compression")):
        if a.get(key) is not None:
            args += [opt, a[key]]
    for key in ("float", "overwrite"):
        if a.get(key):
            args.append("--" + key)
    if a.get("fast_size") is not None:
        args += ["--fast", a["fast_size"]]  # implies fast
    elif a.get("fast"):
        args.append("--fast")
    # a full-size export is slow: wait at least ten minutes
    timeout = max(600.0, float(os.environ.get("ART_MCP_TIMEOUT", "120")))
    r = run_cmd(*args, timeout=timeout)
    return [text("Exported %s (%dx%d)" % (r["file"], r["width"], r["height"]))]


def t_rating(a):
    require_gui("art_set_rating")
    run_cmd("rank", a["rating"], *a["files"])
    return [text("Set the rating of %d file(s) to %d." % (len(a["files"]), a["rating"]))]


def t_label(a):
    require_gui("art_set_color_label")
    run_cmd("label", a["label"], *a["files"])
    names = ["none", "red", "yellow", "green", "blue", "purple"]
    return [text("Set the colour label of %d file(s) to %s." % (len(a["files"]), names[a["label"]]))]


def t_session(a):
    require_gui("art_session")
    op = a["operation"]
    files = a.get("files") or []
    run_cmd("session", op, *files)
    return [text("Session %s done." % op)]



def t_get_history(a):
    rows = []
    for e in run_cmd("get-history")["entries"]:
        rows.append("%s %d: %s%s" % ("->" if e["selected"] else "  ", e["index"], e["text"],
                                     " (%s)" % e["value"] if e["value"] else ""))
    if not rows:
        return [text("The history is empty.")]
    return [text("History, oldest first ('->' = current state; no arrow = a snapshot is "
                 "active):\n" + "\n".join(rows))]


def t_undo(a):
    return [text(_history_state(run_cmd("undo", int(a.get("steps", 1)))))]


def t_redo(a):
    return [text(_history_state(run_cmd("redo", int(a.get("steps", 1)))))]


def t_history_goto(a):
    return [text(_history_state(run_cmd("history-goto", a["index"])))]


def t_get_snapshots(a):
    names = ["%s %s" % ("->" if sn["active"] else "  ", sn["name"])
             for sn in run_cmd("get-snapshots")]
    return [text("Snapshots ('->' = active):\n" + "\n".join(names) if names
                 else "There are no snapshots.")]


def t_snapshot(a):
    op, name = a["operation"], a.get("name")
    if op == "add":
        r = run_cmd(*(["snapshot-add"] + ([name] if name else [])))["name"]
        return [text("Saved the current state as snapshot \"%s\"." % r)]
    if not name:
        raise ArtError("`name` is required for %s" % op)
    run_cmd("snapshot-" + op, name)
    done = {"activate": "Activated", "overwrite": "Overwrote (with the current state)",
            "remove": "Removed"}[op]
    return [text("%s snapshot \"%s\"." % (done, name))]


def t_reference(a):
    r = reference(a.get("group"), a.get("topic"))
    if a.get("group"):
        r += ("\n\n(Related topics: art_reference(topic=\"curves\") for the curve text "
              "format, topic=\"mask\" for local-edit masks and regions, "
              "topic=\"pipeline order\", topic=\"recipes\".)")
    return [text(r)]


def text(s):
    return {"type": "text", "text": s}


def _obj(props, required=()):
    return {"type": "object", "properties": props, "required": list(required),
            "additionalProperties": False}


_files = {"type": "array", "items": {"type": "string"}, "minItems": 1,
          "description": "Absolute paths of image files."}

TOOLS = [
    ("art_status", "Check that ART is running and reachable; show the open directory and "
     "the images open in the editor. Call this first.", _obj({}), t_status),
    ("art_reference", "Read the documentation of ART's editing tools: what each tool does, "
     "every parameter (name, type, default, range, effect), pitfalls and examples. "
     "With no arguments returns the index of all tools and sections. Pass group (an .arp "
     "group name such as 'Exposure', 'ToneCurve', 'Denoise', 'ColorCorrection', 'Crop') for "
     "that tool's section, or topic for a free-text search of section titles "
     "(e.g. 'curves', 'masks', 'pipeline order', 'recipes'). Read it before writing edits.",
     _obj({"group": {"type": "string"}, "topic": {"type": "string"}}), t_reference),
    ("art_open_directory", "Show a folder in ART's file browser.",
     _obj({"path": {"type": "string"}}, ["path"]), t_open_directory),
    ("art_get_directory", "Path of the folder currently shown in the file browser "
     "(':session:' if the session view is active).", _obj({}), t_get_directory),
    ("art_open_image", "Open an image in the editor (waits until it is loaded). It becomes "
     "the image that edits and previews act on.",
     _obj({"path": {"type": "string"}}, ["path"]), t_open_image),
    ("art_list_open_images", "Paths of the images open in the editor; the first one is the "
     "visible tab (the one edits apply to).", _obj({}), t_list_open),
    ("art_get_info", "Size, camera, lens, ISO, aperture, shutter, focal length and raw/non-raw "
     "of the image visible in the editor. full_width/full_height are the coordinates for "
     "Crop and for preview crops.", _obj({}), t_info),
    ("art_get_edit", "The current processing parameters (.arp text) of the visible image: "
     "read this before editing. Pass groups to get only some groups (the full output is long).",
     _obj({"groups": {"type": "array", "items": {"type": "string"}}}), t_get_edit),
    ("art_apply_edit", "Change processing parameters of the visible image. Give `edit` = "
     "{\"Group\": {\"Key\": value, ...}, ...} using the group and key names from "
     "art_reference; only the given keys change. Booleans are true/false, numbers are "
     "numbers, curves and lists are strings exactly as documented. Tools with an Enabled key "
     "are enabled automatically unless you set Enabled yourself. For per-region/mask data "
     "that does not fit that shape, give `arp` (raw .arp text) instead. append=false replaces "
     "the region lists of the local-editing tools (ColorCorrection, Smoothing, Local Contrast, "
     "TextureBoost), append=true adds to them.",
     _obj({"edit": {"type": "object", "additionalProperties": {"type": "object"}},
           "arp": {"type": "string"}, "append": {"type": "boolean", "default": False}}),
     t_apply_edit),
    ("art_apply_profile_file", "Apply a profile (.arp) file from disk to the visible image.",
     _obj({"path": {"type": "string"}, "append": {"type": "boolean", "default": False}},
          ["path"]), t_apply_profile),
    ("art_preview", "Render the visible image with its current edits and return it as an "
     "image. Optionally a crop (x, y, w, h in full-image pixels, see art_get_info) to inspect "
     "details, and either scale or max_size (longest edge, default 1280).",
     _obj({"scale": {"type": "number", "exclusiveMinimum": 0},
           "max_size": {"type": "integer", "minimum": 64},
           "crop": _obj({"x": {"type": "integer", "minimum": 0},
                         "y": {"type": "integer", "minimum": 0},
                         "w": {"type": "integer", "minimum": 1},
                         "h": {"type": "integer", "minimum": 1}}, ["x", "y", "w", "h"]),
           "format": {"type": "string", "enum": ["jpeg", "png"], "default": "jpeg"}}),
     t_preview),
    ("art_get_spot_wb", "Measure the white balance of a rectangle of the visible image that "
     "should be neutral (grey card, white wall; x, y, w, h in full-image pixels, see "
     "art_get_info; at most 1000000 pixels). Returns the temperature and tint that make it "
     "neutral and the equivalent channel multipliers. It does not change the image: apply the "
     "result with art_apply_edit if wanted.",
     _obj({"x": {"type": "integer", "minimum": 0}, "y": {"type": "integer", "minimum": 0},
           "w": {"type": "integer", "minimum": 1}, "h": {"type": "integer", "minimum": 1}},
          ["x", "y", "w", "h"]), t_spot_wb),
    ("art_get_clut_params", "List the parameters of a LUT or CTL script (the controls that "
     "Film Simulation shows for it): name, type (int, float, bool, choice, curve, ...), "
     "label, group, tooltip, min, max, step, default values and the choices of a choice "
     "parameter, plus the script's presets. A plain LUT (CLF, Hald, cube) has none. path is "
     "absolute. It does not need an image and does not change anything.",
     _obj({"path": {"type": "string"}}, ["path"]), t_get_clut_params),
    ("art_save", "Write the edits and snapshots of the active image to its .arp sidecar "
     "(server backend only: ART-cli never saves by itself).", _obj({}), t_save),
    ("art_export", "Write the visible image with its current edits at full size to an image "
     "file, exactly as ART's batch export would (unlike art_preview, which is a quick "
     "approximation). Server backend only. Slow: seconds to minutes. path is absolute; the "
     "format is that of its extension unless format is given (jpg, tif, png, ...). Options: "
     "quality (jpg, 0-100, default 92), subsampling (jpg, 1-3), bits (png 8|16; tif 8|16|32), "
     "float (tif, 16-bit floating point), compression (tif: none|zip). An existing file is "
     "not replaced unless overwrite is true. fast uses the quick export pipeline, which "
     "scales the image down to the size set in ART's preferences; fast_size (pixels) implies "
     "fast and sets the longest edge of the result instead, e.g. 1600 for a web copy.",
     _obj({"path": {"type": "string"}, "format": {"type": "string"},
           "quality": {"type": "integer", "minimum": 0, "maximum": 100},
           "subsampling": {"type": "integer", "minimum": 1, "maximum": 3},
           "bits": {"type": "integer", "enum": [8, 16, 32]},
           "float": {"type": "boolean"},
           "compression": {"type": "string", "enum": ["none", "zip"]},
           "overwrite": {"type": "boolean", "default": False},
           "fast": {"type": "boolean", "default": False},
           "fast_size": {"type": "integer", "minimum": 1, "maximum": 100000}},
          ["path"]), t_export),
    ("art_get_history", "The edit history of the visible image, oldest first, with the "
     "current entry marked. Each entry is one change (e.g. an Exposure adjustment).",
     _obj({}), t_get_history),
    ("art_undo", "Undo the last edit (or `steps` edits) of the visible image by moving back "
     "in the history. Later entries stay available for art_redo until a new edit is made.",
     _obj({"steps": {"type": "integer", "minimum": 1, "default": 1}}), t_undo),
    ("art_redo", "Redo `steps` (default 1) undone edits.",
     _obj({"steps": {"type": "integer", "minimum": 1, "default": 1}}), t_redo),
    ("art_history_goto", "Jump to a history entry by index (see art_get_history).",
     _obj({"index": {"type": "integer", "minimum": 0}}, ["index"]), t_history_goto),
    ("art_get_snapshots", "The named snapshots (saved edit states) of the visible image; "
     "the active one is marked.", _obj({}), t_get_snapshots),
    ("art_snapshot", "Manage snapshots of the visible image. add: save the CURRENT state "
     "as a new snapshot (name optional, auto-numbered if omitted); activate: switch the "
     "image to a snapshot; overwrite: replace a snapshot with the CURRENT state; remove: "
     "delete a snapshot. Snapshots are stored in the .arp sidecar.",
     _obj({"operation": {"type": "string", "enum": ["add", "activate", "overwrite", "remove"]},
           "name": {"type": "string"}}, ["operation"]), t_snapshot),
    ("art_set_rating", "Set the star rating (0-5) of image files.",
     _obj({"files": _files, "rating": {"type": "integer", "minimum": 0, "maximum": 5}},
          ["files", "rating"]), t_rating),
    ("art_set_color_label", "Set the colour label of image files: 0 none, 1 red, 2 yellow, "
     "3 green, 4 blue, 5 purple.",
     _obj({"files": _files, "label": {"type": "integer", "minimum": 0, "maximum": 5}},
          ["files", "label"]), t_label),
    ("art_session", "Operate on ART's session (a virtual collection of files): add, remove, "
     "clear, show (switch the browser to it), open (add and open the first file in the "
     "editor), load/save (from/to a file; load without files restores the last session).",
     _obj({"operation": {"type": "string",
                         "enum": ["add", "remove", "clear", "show", "open", "load", "save"]},
           "files": {"type": "array", "items": {"type": "string"}}}, ["operation"]),
     t_session),
]
TOOL_MAP = {n: (d, s, f) for n, d, s, f in TOOLS}
# tools of one backend only
GUI_ONLY = {"art_open_directory", "art_get_directory", "art_set_rating",
            "art_set_color_label", "art_session"}
SERVER_ONLY = {"art_save", "art_export"}


def tools_for_backend():
    try:
        b = backend()
    except ArtError:
        b = "gui"
    hidden = GUI_ONLY if b == "server" else SERVER_ONLY
    return [t for t in TOOLS if t[0] not in hidden]


# --------------------------------------------------------------------------
# MCP / JSON-RPC
# --------------------------------------------------------------------------

PROTOCOLS = ["2025-06-18", "2025-03-26", "2024-11-05"]


def handle(msg):
    method = msg.get("method")
    params = msg.get("params") or {}
    if method == "initialize":
        want = params.get("protocolVersion")
        return {"protocolVersion": want if want in PROTOCOLS else PROTOCOLS[0],
                "capabilities": {"tools": {}, "resources": {}},
                "serverInfo": {"name": SERVER_NAME, "version": SERVER_VERSION},
                "instructions": (INSTRUCTIONS_SERVER
                                 if os.environ.get("ART_MCP_BACKEND", "gui").strip().lower()
                                 == "server" else INSTRUCTIONS)}
    if method == "ping":
        return {}
    if method == "tools/list":
        return {"tools": [{"name": n, "description": d, "inputSchema": s}
                          for n, d, s, _ in tools_for_backend()]}
    if method == "tools/call":
        name = params.get("name")
        if name not in TOOL_MAP or name not in [t[0] for t in tools_for_backend()]:
            raise RpcError(-32602, "unknown tool " + str(name))
        args = params.get("arguments") or {}
        schema, fn = TOOL_MAP[name][1], TOOL_MAP[name][2]
        for r in schema.get("required", []):
            if r not in args:
                return {"content": [text("missing argument: " + r)], "isError": True}
        try:
            return {"content": fn(args), "isError": False}
        except ArtError as e:
            return {"content": [text("Error: %s" % e)], "isError": True}
        except Exception as e:  # keep the server alive
            return {"content": [text("Internal error: %r" % (e,))], "isError": True}
    if method == "resources/list":
        return {"resources": [
            {"uri": "art://reference/" + f, "name": f, "mimeType": "text/markdown",
             "description": "ART tool and parameter reference"} for f in _ref_files()]}
    if method == "resources/read":
        uri = params.get("uri", "")
        fn = uri.replace("art://reference/", "")
        if fn not in _ref_files():
            raise RpcError(-32002, "unknown resource " + uri)
        with open(os.path.join(REF_DIR, fn), encoding="utf-8") as f:
            return {"contents": [{"uri": uri, "mimeType": "text/markdown", "text": f.read()}]}
    if method and method.startswith("notifications/"):
        return None
    raise RpcError(-32601, "method not found: %s" % method)


class RpcError(Exception):
    def __init__(self, code, message):
        super().__init__(message)
        self.code, self.message = code, message


def main():
    if "--check" in sys.argv:
        print("tools:", ", ".join(TOOL_MAP))
        print("known groups:", len(KNOWN["groups"]), " reference sections:", len(_sections()))
        return
    for stream, kw in ((sys.stdin, {"encoding": "utf-8"}),
                       (sys.stdout, {"encoding": "utf-8", "newline": "\n"})):
        try:
            stream.reconfigure(**kw)
        except (AttributeError, ValueError):
            pass
    out = sys.stdout
    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue
        try:
            msg = json.loads(line)
        except ValueError:
            continue
        if not isinstance(msg, dict):
            continue
        is_request = "id" in msg and "method" in msg
        try:
            result = handle(msg)
        except RpcError as e:
            if is_request:
                out.write(json.dumps({"jsonrpc": "2.0", "id": msg["id"],
                                      "error": {"code": e.code, "message": e.message}}) + "\n")
                out.flush()
            continue
        if is_request:
            out.write(json.dumps({"jsonrpc": "2.0", "id": msg["id"], "result": result}) + "\n")
            out.flush()


if __name__ == "__main__":
    main()
