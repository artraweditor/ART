# Agentic Pipeline (feature/agentic-pipeline)

**Generated:** 2026-08-20
**Commit:** da5e80e30

## OVERVIEW
External AI layer wrapping ART (positioned as an agent around the tool, NOT a core extension). LangGraph multi-agent pipeline: user request → router → 4 services (P1 Master Q&A / RAG, P2 FiveK correction recommendation, P3 feedback summarization, P4 translation) → `.arp` profile generation → `ART-cli` rendering → corrected image. Vision doc: `docs/ART_agentic_project_plan.md` (P1–P4 priorities per `docs/ART_agentic_project_pitch_summary.md`). **Execution authority: `docs/ART_agentic_wbs.md` (v2.5, 2026-09-02; 12주, 9/1–11/21, 169h 계획)** — scope is P1 (Master Q&A) + P2 (FiveK correction recommendation) only; P3 (feedback summarization) + P4 (translation) dropped.

## STRUCTURE
```
agentic-pipeline/
├── scripts/
│   ├── build_art_cli.sh      # cmake -DCMAKE_BUILD_TYPE=Release .. && make art-cli → build/rtgui/ART-cli
│   ├── download_sample_raw.py # fetch sample NEF (rawsamples.ch) → data/fivek_sample/sample1.nef
│   ├── fetch_rawpedia.py     # RawPedia MediaWiki API → markdown corpus (RAG) [-o outdir, -s limit]
│   └── test_art_cli.sh       # integration test: ART-cli -a -Y -p profile -c raw → assert JPEG
├── data/
│   ├── fivek_sample/         # sample1.nef (real RAW, untracked), sample1.dng (dummy), sample1.jpg (output)
│   ├── rawpedia_sample/      # seed RAG corpus (en/de/fr/it pages)
│   └── test_profile.arp      # synthetic minimal profile for tests
└── docs/
    ├── ART_agentic_wbs.md            # execution WBS (Korean) — SCOPE/SCHEDULE AUTHORITY (9/1–11/21, 169h plan)
    ├── ART_agentic_project_plan.md   # strategy doc (Korean) — superseded by WBS for scope/schedule
    ├── ART_agentic_project_pitch_summary.md
    └── arp_schema.md                # .arp INI groups/keys — authoritative for generation
```

## WHERE TO LOOK
| Task | Location |
|------|----------|
| Project vision / roadmap / evaluation metrics | `docs/ART_agentic_project_plan.md` |
| **Execution WBS — tasks, milestones, cut lines, risks (current authority)** | `docs/ART_agentic_wbs.md` |
| `.arp` keys for generation | `docs/arp_schema.md` + `rtgui/ppversion.h` (PPVERSION coupling) |
| ART-cli invocation | `scripts/test_art_cli.sh` (flags: `-a -Y -p <profile> -c <raw>`) |
| RAG corpus building | T4: RawPedia 원문 Markdown과 수집 목록; T10-1: ART GitHub 원문 수집; T10-2a: GitHub 검색 후보 정제; T8-1: 수집 문서 질문셋; T8-2: 두 출처 청킹·임베딩 모델 시험; T9: 벡터DB 적재·검색 공통 모듈; T10-2c: GitHub 변경분 반영 |
| Render & evaluate | ART-cli output vs FiveK Expert C (SSIM/LPIPS/ΔE); RAGAS for QA |

## CONVENTIONS
- Python 3.14 venv at repo ROOT (`venv/`) — deps: requests, beautifulsoup4, markdownify. **No requirements.txt**; install into that venv.
- Scripts: argparse + docstrings + `main()` guard; bash `set -e`; `-j$(sysctl -n hw.logicalcpu 2>/dev/null || nproc)` for cross-OS.
- CMake target is `art-cli` (lowercase); produced binary is `ART-cli` at `build/rtgui/ART-cli` (not `build/ART-cli`).
- `-DENABLE_GUI=OFF` in build_art_cli.sh is a no-op — harmless, kept for intent.

## ANTI-PATTERNS (THIS PROJECT)
- **Do NOT download the full FiveK dataset** (multi-GB). ART-cli 초기 확인에는 1–2개 샘플만 사용하고, ① 구현용 서브셋 규모·분할은 실행 권한인 WBS의 B-01·B-02를 따른다.
- Generated `.arp` must validate against `docs/arp_schema.md`; any format change bumps PPVERSION.
- Don't modify ART core (`rtengine/`, `rtgui/`) for agentic needs — the layer is external by design; the only allowed core change is what the branch already did to `main-cli.cc` (process explicitly-provided files without extension checks).
- FiveK Lightroom↔arp mapping is approximate — document the limitation, don't present as exact.

## COMMANDS
```bash
# From this worktree root
./scripts/build_art_cli.sh                                  # build ART-cli
python3 scripts/download_sample_raw.py                      # fetch sample NEF (once)
./scripts/test_art_cli.sh                                   # integration test
python3 scripts/fetch_rawpedia.py -o data/rawpedia -s 50    # RAG corpus
# Manual render:
./build/rtgui/ART-cli -a -Y -p data/test_profile.arp -c data/fivek_sample/sample1.nef
```
