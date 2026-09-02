# PROJECT KNOWLEDGE BASE

**Scope:** ART core repository and its external agentic AI pipeline.

## OVERVIEW

ART is a cross-platform RAW image editor, GPL-3 fork of RawTherapee (C++11/C11, GTK3/gtkmm, CMake ≥3.9). The repository also contains an external agentic pipeline around ART: LangGraph + RAG + `.arp` profile generation + ART-cli rendering.

**Execution authority for the pipeline:** `docs/ART_agentic_wbs.md` (v2.5, 2026-09-02; 12주, 9/1–11/21, 169h 계획). Scope is Master Q&A/RAG and FiveK correction recommendation only. Feedback summarization and translation are out of scope.

## STRUCTURE

```
AAART/
├── rtengine/            # processing engine (demosaic, color, tone, noise) — vendored jpeg_ijg/, klt/
├── rtgui/               # GTKmm GUI + ART-cli headless binary (main.cc, main-cli.cc)
├── rtdata/              # runtime data: iccprofiles, .arp profiles, luts, themes, languages, options
├── doc/                 # manpage only (doc/manpage/ART.1)
├── docs/                # strategy doc + agentic execution WBS
├── cmake/               # find-modules (FindKLT, FindMacIntegration, FindUnalignedMalloc, FindX87Math)
├── scripts/             # agentic pipeline build, RAW sample, corpus collection, integration test scripts
├── data/                # pipeline sample RAW/profile and corpus data
├── tools/               # dev utilities: build-art, bundle_ART.py, generate* scripts, wb/icc tools
├── licenses/            # third-party licenses (SLEEF, libiomp5, DroidSansMono, jdatasrc)
├── venv/                # Python 3.14 venv for agentic scripts — NOT git-tracked
├── .worktrees/          # local worktrees — gitignored
└── .serena/ .omo/ .codegraph/ .agents/   # local agent tooling state — NOT part of the codebase
```

## WHERE TO LOOK

| Task | Location | Notes |
|---|---|---|
| ART-cli (headless processing) | `rtgui/main-cli.cc` | entry `main()`; flags in `rtgui/printhelp.h` |
| GUI app | `rtgui/main.cc` | GTKmm entry point |
| Processing pipeline | `rtengine/improccoordinator.cc`, `rtengine/improcfun.cc` | full pipeline orchestration and core processing |
| RAW decoding | `rtengine/dcraw.c` | vendored dcraw — do not modernize |
| Build targets | `rtgui/CMakeLists.txt` | `art` + `art-cli` executables, `rtengine` library |
| CI | `.github/workflows/` | Linux/macOS/Windows builds and source tarball |
| Release tooling | `tools/generateSourceTarball`, `tools/generateReleaseInfo` | invoked by `source.yml` |
| CLI flag reference | `rtgui/printhelp.h` | authoritative; `doc/manpage/ART.1` is stale |
| Pipeline scope and schedule | `docs/ART_agentic_wbs.md` | current authority |
| Pipeline architecture and detailed tasks | `tasks/plan.md`, `tasks/todo.md` | dependency graph and task cards |
| `.arp` keys for generation | `docs/arp_schema.md` + `rtgui/ppversion.h` | format/version coupling |
| Pipeline corpus | `data/rawpedia/`, `data/issues/` | RawPedia and ART GitHub sources |

## CODE MAP

| Symbol | Type | Location | Role |
|---|---|---|---|
| `main` | fn | `rtgui/main.cc` | GUI entry |
| `main` | fn | `rtgui/main-cli.cc` | batch processing, `--make-icc`, `--check-lut` |
| `process` | fn | `rtengine/improccoordinator.cc` | full pipeline orchestration |
| `process` | fn | `rtengine/improcfun.cc` | core image processing |
| `ART_print_help` | fn | `rtgui/printhelp.h` | authoritative CLI flag reference |
| `rtengine` | lib | `rtengine/CMakeLists.txt` | shared engine library |
| `art` / `art-cli` | exe | `rtgui/CMakeLists.txt` | output names `ART` / `ART-cli` |
| `guidedFilter` | fn | `rtengine/guidedfilter.cc` | edge-preserving filter |

## CONVENTIONS

- **C++11 / C11** — set globally via `-std=c++11`/`-std=c11` in CMake flags, not per target. GCC ≥4.9 required.
- **Formatting:** `.clang-format` = LLVM base, IndentWidth 4, ColumnLimit 80, WebKit braces, no tabs, no short single-line if/loops.
- **Build defaults:** `CMAKE_BUILD_TYPE` forced to Release when empty; `-DNDEBUG` in release, `-D_DEBUG` in debug. ccache auto-enabled if present.
- **Options (defaults):** `OPTION_OMP`=ON, `ENABLE_MIMALLOC`=ON, `ENABLE_LIBRAW`=ON, `ENABLE_OCIO`=ON, `ENABLE_CTL`=OFF, `ENABLE_SIMDE`=OFF, `ENABLE_LCMS_FAST_FLOAT`=ON, `WITH_LTO`/`WITH_SAN`/`WITH_PROF`/`BUILD_SHARED`=OFF.
- **Pipeline Python:** use the root Python 3.14 `venv/`; do not add `requirements.txt`. Scripts use argparse, docstrings, and a `main()` guard; Bash scripts use `set -e`.
- **Pipeline dependencies:** install only when a consuming task needs them and record the installation then. Do not predeclare a package list, model, configuration loader, or file layout.
- **Version coupling:** bump `PPVERSION` in `rtgui/ppversion.h` whenever the `.arp` format or tool behavior changes.

## ANTI-PATTERNS

- **NEVER build with `-ffast-math`** — it introduces artifacts; `rtgui/main.cc` and `rtgui/main-cli.cc` reject it.
- **No x86 intrinsics in `rtengine/helpersse2.h` / `helperavx.h`** — use GCC vector extensions. The `#error` guards require `-msse2` / `-mavx`.
- **Do not “fix” warnings in vendored code** — `dcraw.c`, `cJSON.c`, `canon_cr3_decoder.cc`, `klt/`, and `jpeg_ijg/` intentionally suppress warnings.
- **No `floor()` in `LUT.h` hot paths** (negative-index truncation correctness); no `std::list` in `lcp.h`; do not call the functions marked “do not use in a loop” in `color.h` from loops.
- **Do not use CameraCalibration matrices for DNGs** (`dcraw.cc`, #4129); do not reuse a `ProcessingJob` after destroy or while processing; `RefreshMapEvent` must not combine with other events.
- **No in-source builds** — stale generated files cause CMake errors.
- **Do not modify `rtengine/` or `rtgui/` for pipeline features.** The agentic layer wraps ART; the only allowed core change is the existing `main-cli.cc` handling of explicitly provided files.
- **Do not download the full FiveK dataset.** ART-cli confirmation uses 1–2 samples; the later recommendation subset and split follow WBS B-01/B-02.
- **Do not present Lightroom↔ART `.arp` mapping as exact.** Record mapping limits.

## UNIQUE STYLES

- ART-cli argument parsing uses `processLineParams` with per-OS console handling in `main-cli.cc`; `printhelp.h` is the flag source of truth.
- Theme retirement uses filename suffix `-DEPRECATED` matched in `rtgui/options.h`.
- macOS/Windows GUI builds use `BUILD_BUNDLE`; Linux uses `bundle_ART.py` and AppImage.
- CI build workflows trigger on version tags and `workflow_dispatch`; they do not run PR builds or CI tests. `mirror.yml` triggers on every push/delete. All platform builds produce x86_64 and arm64 artifacts.

## COMMANDS

```bash
# GUI build (from repo root; creates build/)
cmake -DCMAKE_BUILD_TYPE=Release -B build && cmake --build build -j$(sysctl -n hw.logicalcpu 2>/dev/null || nproc) --target art

# CLI-only build
cmake --build build -j$(sysctl -n hw.logicalcpu 2>/dev/null || nproc) --target art-cli
# → build/rtgui/ART-cli   (target is lowercase art-cli, binary is ART-cli)

# Run ART-cli on a RAW with a profile
build/rtgui/ART-cli -a -Y -p <profile.arp> -c <raw-file>

# Pipeline integration test
./scripts/build_art_cli.sh && python3 scripts/download_sample_raw.py && ./scripts/test_art_cli.sh

# RawPedia RAG corpus fetch
python3 scripts/fetch_rawpedia.py -o data/rawpedia -s 50
```

## PIPELINE WORKING RULES

- The fixed 9/1~9/11 sequence is in WBS §5. Use `tasks/todo.md` for detailed acceptance criteria.
- Keep RawPedia source Markdown, ART GitHub source snapshots, search candidates, chunks, and vector-store state separately traceable.
- Chroma is the selected vector store. Do not choose an embedding model or final chunk rule before comparing candidates with the fixed question set.
- Generate structured profile data, validate it against `docs/arp_schema.md`, and use ART-cli rendering as the final format check.

## NOTES

- `-DENABLE_GUI=OFF` used by `build_art_cli.sh` is not a real CMake option; it is harmless intent documentation.
- No unit-test framework is wired into CMake; `rtengine/rtetest.cc` and `rtgui/test_guidedfilter.cpp` are manual harnesses.
- `TODO.txt` at root is an in-flight scratchpad for profile-change/ParamsEdited refactoring and batch-mode CLI work.
- `tools/` has its own `AGENTS.md` for bundling, release, and camera-data tooling.
- `rtdata/` is mostly static data; its related conventions are documented in `rtgui/AGENTS.md`.
- Root JSONs (`camconst.json`, `dcraw.json`, `rt.json`, `wbpresets.json`) use C-style comments; strip comments before parsing.
- `doc/manpage/ART.1` is stale; update `printhelp.h`, not the manpage, when CLI flags change.
