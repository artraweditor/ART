# ART-cli server mode

`ART-cli --server` runs a long-lived, headless process that keeps one image
open and executes commands sent by other `ART-cli --cmd ...` invocations. It is
the scriptable counterpart of the GUI's remote commands (`ART --cmd ...`, which
travel over D-Bus): the same verbs, restricted to those that act on an *active
image*, over a Unix domain socket instead of D-Bus. No GUI, no D-Bus and no
network connection are involved.

Typical uses: driving ART from scripts or from the MCP server
(`tools/mcp`, backend `server`) on a machine without a desktop session, and
repeated previews of one image without paying the start-up and load cost every
time (the processing pipeline and its caches stay alive between commands).

```
$ ART-cli --server &                  # prints "ART-cli server listening on <socket>"
$ ART-cli --cmd open photo.arw
$ ART-cli --cmd apply look.arp
$ ART-cli --cmd get-spot-wb 1500 2000 20 20
$ ART-cli --cmd preview /tmp/p.jpg --scale 0.25
$ ART-cli --cmd snapshot-add before-crop
$ ART-cli --cmd save
$ ART-cli --cmd quit
```

## Command line

| | |
|---|---|
| `ART-cli [--socket PATH] --server [-V] [-q]` | run the server (`-V`: log each command to stderr; `-q`: as for batch mode) |
| `ART-cli [--socket PATH] --cmd VERB [ARGS...]` | send one command; `--cmd` takes everything that follows, so arguments starting with `--` are fine |
| `ART-cli --cmd help` | list the commands (answered locally, no server needed) |

`--cmd` does not load any configuration or engine, so it starts immediately.
Its standard output and standard error are those of the command and its exit
status is 0 on success, 1 on failure (`ART: error: <message>` on stderr), e.g.
`ART: error: no running server (...)` when nobody is listening.
With `ART_CMD_MARK=1` in the environment, every output line is prefixed with
`\x01`, so that a program can tell the reply from unrelated messages
(this is what the MCP server does).

Paths are resolved against the *client's* working directory. Output files
(`preview`) are written by the *server* process.

## Commands

| command | effect |
|---|---|
| `open FILE` | load FILE and make it the active image, replacing the previous one (unsaved edits are dropped); replies when it is loaded. Starting parameters: the `FILE.arp` sidecar if there is one (with its snapshots), otherwise the default profile of the options |
| `save` | write the current parameters and the snapshots to the sidecar; prints its path |
| `get-files` | print the active image (nothing if none) |
| `apply [--append] PROFILE` | apply a (partial) profile; a new history entry |
| `get-params` | the current parameters, as `.arp` text |
| `get-info` | `key: value` lines: size, camera, exposure |
| `get-history` | `index<TAB>*\|-<TAB>text<TAB>value`, `*` = selected |
| `undo [N]`, `redo [N]`, `history-goto INDEX` | move in the history; reply with the new state |
| `get-snapshots` | `*\|-<TAB>name`, `*` = active |
| `snapshot-add [NAME]`, `snapshot-activate NAME`, `snapshot-overwrite NAME`, `snapshot-remove NAME` | manage snapshots |
| `preview OUT [--scale F] [--crop X,Y,W,H]` | render the image (crop in full-image coordinates) to a PNG or JPEG (by extension); prints `path (WxH)` |
| `get-spot-wb X Y W H` | white balance that makes the rectangle neutral (see below) |
| `export OUTFILE [options]` | write the image at full size, as `ART-cli` does (server only; see below) |
| `ping`, `quit` | liveness check; stop the server |

The GUI-only commands (`open-dir`, `get-dir`, `rank`, `label`, `session`) are
not available: the server has no file browser.

Everything above except `open`, `save`, `export`, `ping`, `quit` and the
server's single-image `get-files` is the same code in the GUI (`ART --cmd`) and
in the server: `get-spot-wb` and the others behave identically in both.

### `export OUTFILE [options]`

Renders the active image with its current parameters (unsaved edits included)
through the full output pipeline, exactly as `ART-cli -c -o OUTFILE` would, and
writes it to `OUTFILE`. On success it prints `path (WxH)`. Available in the
server only (and so in libART); the GUI's `--cmd` reports that export is not
available (use `ART-cli` or the batch queue there).

| option | meaning |
|---|---|
| `--format F` | `jpg`, `tif`, `png` or one of the extra formats (`export --list-formats`); by default the extension of `OUTFILE` (`jpeg` and `tiff` are accepted) |
| `--quality N` | jpg only: 0-100, default 92 |
| `--subsampling N` | jpg only: 1 (best compression) to 3 (best quality, the default) |
| `--bits N` | png: 8 (default) or 16; tif: 8, 16 (default) or 32. 32 bits are floating point |
| `--float` | tif: 16-bit floating point (with `--bits 16`) |
| `--compression none\|zip` | tif only; default `none` |
| `--overwrite` | replace an existing `OUTFILE`; without it an existing file is an error |
| `--fast` | the fast export pipeline (`ART-cli -f`) |

`OUTFILE` is written exactly as named (the extension is not changed to match
the format) and must be in an existing directory; it cannot be the input file.
An option that does not apply to the format is an error. `export --list-formats`
prints the formats this installation can write, one per line (the extra ones
need their encoders, as for `ART-cli -T`).

The result is the same as the one of `ART-cli` for the same parameters (the
image is rendered from the file, not from the editing pipeline, so the raw is
read again). A full-size export takes seconds to minutes, cannot be
interrupted, and keeps the server busy: other clients wait. Set the client's
timeout accordingly (`ART_options_set_timeout` in libART, `ART_MCP_TIMEOUT` for
the MCP server). The edit history and snapshots are not affected.

### `get-spot-wb X Y W H`

The rectangle (top left corner and size, in the coordinates of the processed
image, i.e. the `full_width` x `full_height` of `get-info`; `X,Y,W,H` as one
argument is also accepted) is meant to be neutral, such as a grey card. The
reply:

```
temperature: 3735.61
tint: 1.02108
red_multiplier: 1.67118
green_multiplier: 1
blue_multiplier: 0.524421
```

`temperature` and `tint` are the `Temperature` and `Green` keys of the
`[White Balance]` group (`Setting=CustomTemp`) that make the rectangle
neutral; the multipliers are the equivalent per-channel gains relative to green
(the engine's own scale is arbitrary). The command does not change the image.
The rectangle must lie inside the image and contain at most 1,000,000 pixels.
The measure is the one the GUI's white balance spot tool uses, which is the
case of a square of side 2r+1 centred on a point.

## Behaviour

* **One image, one command at a time.** Commands from different clients are
  executed in the order they arrive; a long one (a large `preview`, an `open`)
  makes the others wait. A command that needs the pipeline waits up to 60 s for
  it to be idle (`timeout waiting for the editor`).
* **Nothing is written implicitly.** Opening another image, `quit`, SIGINT and
  SIGTERM drop unsaved edits; `save` is the only thing that touches the
  sidecar. This differs from the GUI, which saves when an image is closed.
* **`preview` is not an export.** It renders through the preview/detail
  pipeline, so it can differ from a batch export of the same profile
  (measured: a mean difference of about 4 levels out of 255 on one test raw).
  Use `export` when the exact output matters.
* The server does not run the Crop tool's trimming after `apply`; a profile
  whose crop exceeds the image is applied as it is.
* Set `ART_GPU=1` (or `0`) in the server's environment to choose the backend
  explicitly, as for batch runs.

## Socket and security

The socket is, in order of preference, `--socket PATH`, `$ART_SERVER_SOCKET`,
or `server.sock` in a private directory: `$XDG_RUNTIME_DIR/ART` or
`$TMPDIR/ART-$USER` (created `0700`, refused if it is a symlink or belongs to
someone else; made private if it was too open). The socket itself is `0600` and
connections from another user are dropped, so a server is only reachable by the
user who started it. Unix socket paths are limited to about 100 bytes: use
`--socket` if the default is too long.

If the socket file exists, a server that answers means "already running" (the
new one exits with an error); a socket nobody listens on (left by a server
that was killed) is replaced. The socket is removed on a normal exit. There is
no authentication beyond file permissions: anything running as you can drive
the server, including opening and saving files.

On Windows the server mode is not available yet (the transport is behind
`ipc_transport.h`, so another one can be added).

## Protocol

For writing another client. One request per connection.

* Request: `ART1 <len>\n`, then `<len>` bytes (at most 1 MiB): the fields
  `cwd`, `mark` (`0`/`1`), and the verb and arguments, each terminated by a NUL
  byte.
* Response: frames `O <len>\n<bytes>` (stdout), `E <len>\n<bytes>` (stderr),
  ending with `X <status>\n`.

## Source map

| | |
|---|---|
| `src/engine/remote/command_dispatch.*` | verb table and the verbs shared with the GUI, `CommandTarget`/`CommandHost` |
| `src/engine/remote/remote_reply.*` | `Reply`/`ReplySink`: output, error prefix, `\x01` marking |
| `src/engine/remote/ipc_transport.h`, `unix_socket.cc`, `wire.*` | socket transport and framing |
| `src/engine/remote/history_model.h` | history and snapshot rules (GUI-free) |
| `src/gui/editsession.*` | the server's active image (processor, history, snapshots, sidecar) |
| `src/gui/servermode.*` | `--server` / `--cmd` |
| `src/gui/remotecmd.cc` | the GUI's host of the same verbs (D-Bus) |

Tests: `ninja art-history-model-test art-ipc-test && ctest -R "history-model|ipc"`.
