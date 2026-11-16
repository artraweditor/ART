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
Its exit status is 0 on success and 1 on failure.

Paths are resolved against the *client's* working directory. Output files
(`preview`, `export`) are written by the *server* process.

## Replies

**Standard output carries one thing: the reply of the command, as a single
line of JSON.** Nothing else is ever written to it (`ART-cli --cmd` and
`ART --cmd` load no configuration and print no diagnostics there), so a
script can parse all of it:

```
{"ok":true}                        success, nothing to return
{"ok":true,"result":<value>}       success
{"ok":false,"error":"message"}     failure
```

A failed command (and a failure of the client itself, such as
`no running server (...)` when nobody is listening) also prints
`ART: error: <message>` on standard error and exits with status 1; standard
error is free for such human-oriented messages. The only exception is
`--cmd help`, which prints the help text on standard output.

```
$ ART-cli --cmd get-spot-wb 1500 2000 20 20
{"ok":true,"result":{"temperature":3735.6147766113281,"tint":1.0210771229425262,"red_multiplier":1.6711792696011167,"green_multiplier":1,"blue_multiplier":0.524420945538329}}
$ ART-cli --cmd get-info | jq .result.full_width
3124
```

The `result` of each command is listed below. Strings are UTF-8, numbers are
JSON numbers (`null` stands for a missing value).

## Commands

| command | effect |
|---|---|
| `open FILE` | load FILE and make it the active image, replacing the previous one (unsaved edits are dropped); replies when it is loaded. Starting parameters: the `FILE.arp` sidecar if there is one (with its snapshots), otherwise the default profile of the options |
| `save` | write the current parameters and the snapshots to the sidecar; result `{"file": <sidecar path>}` |
| `get-files` | the active image: an array of paths (empty if none) |
| `apply [--append] [--merge KEY] PROFILE` | apply a (partial) profile; a new history entry, or with `--merge`, the same one as the previous `apply --merge` with the same KEY (see below) |
| `apply :neutral`, `apply :default` | apply the neutral profile, or the default profile of the preferences (see below) |
| `get-params` | the current parameters: `{"params": <.arp text>}` |
| `get-metadata [--makernotes\|--no-makernotes] [--edits\|--no-edits]` | the metadata as a JSON object (see below) |
| `get-info` | an object: `file`, `is_raw` (boolean), `source_width`, `source_height`, `full_width`, `full_height`, `make`, `model`, `lens`, `iso`, `f_number`, `focal_length_mm`, `shutter_s`, `exposure_compensation`, `orientation` |
| `get-history` | `{"selected": <index, -1 if none>, "entries": [{"index", "selected", "text", "value"}, ...]}` |
| `undo [N]`, `redo [N]`, `history-goto INDEX` | move in the history; result: the new state `{"selected", "count", "text", "snapshot"}` (`text` and `snapshot` are `null` when there is none) |
| `get-snapshots` | an array of `{"name", "active"}` |
| `snapshot-add [NAME]` | result `{"name": <the name given or generated>}` |
| `snapshot-activate NAME`, `snapshot-overwrite NAME`, `snapshot-remove NAME` | manage snapshots |
| `preview OUT [--scale F] [--crop X,Y,W,H]` | render the image (crop in full-image coordinates) to a PNG or JPEG (by extension); result `{"file", "width", "height"}` |
| `get-spot-wb X Y W H` | white balance that makes the rectangle neutral (see below) |
| `export OUTFILE [options]` | write the image at full size, as `ART-cli` does (in the GUI: queue it; see below) |
| `ping`, `quit` | liveness check (result `"pong"`); stop the server |

The GUI-only commands (`open-dir`, `get-dir` (result `{"directory"}`), `rank`,
`label`, `session`) are not available: the server has no file browser. In the
GUI, `get-files` lists all the open images, the visible one first.

Everything above except `open`, `save`, `export`, `ping`, `quit` and the
server's single-image `get-files` is the same code in the GUI (`ART --cmd`) and
in the server: `get-spot-wb` and the others behave identically in both.

### `apply --merge KEY`

For a client that applies a profile at every step of a gesture (a slider drag),
which would otherwise fill the history with one entry per step. Consecutive
`apply --merge KEY PROFILE` commands with the same non-empty KEY update one
history entry, provided it is still the selected, last entry (so an `undo`, a
history jump, an `apply` without `--merge` or another KEY starts a new entry).
Use a new KEY (e.g. `exposure#17`) for each gesture, so that each is one undo
step. The profiles are still applied one by one, so the final parameters are
those of a sequence of plain `apply` commands. Not valid with `:neutral` and
`:default`. The GUI (`ART --cmd`) accepts the option but records one entry per
command, as without it.

### `apply :neutral` and `apply :default`

Two special names stand for profiles that are not files, as in the profile
panel of the GUI. A file with such a name can still be applied as `./:neutral`.

* `:neutral` is the internal neutral profile: every tool at its default value.
* `:default` is the default profile for the image set in the preferences: the one
  for raw images or the one for other images, depending on the image. It can
  be a profile file, the neutral profile, or the *dynamic* profile, whose rules
  are evaluated on the metadata of the image.

Both are complete profiles: they **replace all the parameters** of the image
(including the metadata edits and the general settings stored in them, such
as the colour label), exactly as choosing them in the profile panel does, as one history
entry named `Neutral` or `Default`. `--append` is an error with them.

### `export OUTFILE [options]`

Renders the active image with its current parameters (unsaved edits included)
through the full output pipeline, exactly as `ART-cli -c -o OUTFILE` would, and
writes it to `OUTFILE`. The result is `{"file", "width", "height", "format"}`.
In the GUI (`ART --cmd export`) the image is instead **added to the batch
queue** with these settings, as with the "add to queue" button of the Save as
dialog, and the result is `{"file", "format", "queued": true}`. Limitations
of `export` in the GUI, compared with the server:

* **Nothing is written when the command returns.** It only queues the image, so
  the reply has no `width` and `height`, and neither the completion nor a
  failure of the export is reported to the command: a processing or saving
  error is shown in the GUI, as for any queued image.
* **The queue has to run.** The image is processed when the queue is started: by
  its auto-start switch, or by the user.
* **File name.** The queue gives `OUTFILE` the extension of the format (`file` is
  the name with that extension) and creates its folder if needed. It treats an
  existing file as its preferences say: by default it adds a numeric suffix
  (`name-1.jpg`), with the "overwrite output file" preference it replaces it.
  The command cannot choose, so `--overwrite` is an error. The reported name is
  the one asked for, not necessarily the final one.
* **The parameters are those of the moment of the command**: later edits do not
  change the queued export. As for Save as, the queue's batch profile is not
  applied, and no `.arp` file is saved next to the output.
* It acts on the visible editor tab, like the other commands.

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
returns the formats this installation can write, as an array of strings (the
extra ones need their encoders, as for `ART-cli -T`).

The result is the same as the one of `ART-cli` for the same parameters (the
image is rendered from the file, not from the editing pipeline, so the raw is
read again). A full-size export takes seconds to minutes, cannot be
interrupted, and keeps the server busy: other clients wait. Set the client's
timeout accordingly (`ART_options_set_timeout` in libART, `ART_MCP_TIMEOUT` for
the MCP server). The edit history and snapshots are not affected.

### `get-metadata [--makernotes | --no-makernotes] [--edits | --no-edits]`

Returns the metadata of the active image as one flat JSON object, like the
metadata panel of the GUI shows it. The keys are tag names:

* `Exif.<Group>.<Tag>`: the tags read by exiv2, with the basic tags filled in
  and, unless `--no-edits` is given, the changes made by the current parameters
  (the metadata edits of the `.arp`: the Exif edits, and the PPI of the Resize
  tool when it is copied to the Exif data) applied. **Values keep their type**: integer
  types are integers, rationals and floating point values are numbers (`null`
  if a rational has a zero denominator; a rational such as `16/1` is
  printed as `16`), text is a string, a tag with several values is an array,
  and an "undefined" value is a string if it is printable text (`"0232"`) and
  an array of bytes otherwise. Binary blobs (non-text values of 256 bytes or
  more) are left out;
* `ExifTool.MakerNotes.<Name>`: the maker notes read by ExifTool, **always
  strings** (the way ExifTool prints them).

The maker notes are included if the preferences ask for them (the panel's
option), unless `--makernotes` or `--no-makernotes` says otherwise. Getting
them runs ExifTool, which takes longer (and needs ExifTool to be installed);
`--no-makernotes` is the quick form. The edits are included by default, as in
the panel; with `--no-edits` the tags are those of the file as it is (plus the
basic tags), which is what to use to read the original metadata. Keys are
sorted.

```
{"ok":true,"result":{"Exif.Image.Make":"SONY","Exif.Photo.ISOSpeedRatings":100,
 "Exif.Photo.FNumber":2.8,"Exif.Photo.ComponentsConfiguration":[1,2,3,0],
 "ExifTool.MakerNotes.LensMount":"E-mount", ...}}
```

### `get-spot-wb X Y W H`

The rectangle (top left corner and size, in the coordinates of the processed
image, i.e. the `full_width` x `full_height` of `get-info`; `X,Y,W,H` as one
argument is also accepted) is meant to be neutral, such as a grey card. The
result is an object:

```
{"temperature":3735.6147766113281,"tint":1.0210771229425262,
 "red_multiplier":1.6711792696011167,"green_multiplier":1,
 "blue_multiplier":0.524420945538329}
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
| `src/engine/remote/remote_reply.*`, `json.h` | `Reply`/`ReplySink`: the JSON reply document and the error line; `json::Value`, a wrapper over cJSON |
| `src/engine/remote/ipc_transport.h`, `unix_socket.cc`, `wire.*` | socket transport and framing |
| `src/engine/remote/history_model.h` | history and snapshot rules (GUI-free) |
| `src/gui/editsession.*` | the server's active image (processor, history, snapshots, sidecar) |
| `src/gui/servermode.*` | `--server` / `--cmd` |
| `src/gui/remotecmd.cc` | the GUI's host of the same verbs (D-Bus) |

Tests: `ninja art-history-model-test art-ipc-test && ctest -R "history-model|ipc"`.
