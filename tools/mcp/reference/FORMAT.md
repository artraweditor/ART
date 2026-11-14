# Format every reference file must follow

Audience: an LLM that edits photos by writing *partial ART processing profiles*
(`.arp` sidecar fragments, INI/GLib-keyfile syntax) from a user's plain-language
request ("make the sky moodier", "recover the highlights", "crop to 4:5").
It cannot see the code, so every statement must be concrete and correct.

One `##` section per GUI tool / arp group (use the group name exactly as
written in the file, e.g. `[Exposure]`), in pipeline order where relevant:

```
## Exposure  —  `[Exposure]`
**What it does:** 2-5 sentences in photographic terms; where it sits in the
pipeline; what the image looks like when pushed too far.
**Typical requests:** short list of user phrasings this tool answers.
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Enabled` | bool (`true`/`false`) | ... | | ... |
**How to write it:** exact serialisation of non-trivial values (curves, lists,
colours, per-region data, `;`-separated lists, decimal point is `.`).
**Interactions & pitfalls:** things that make results wrong or surprising
(other tools that overlap, keys that only matter when another is set,
units: stops vs. percent, 0..1 vs 0..100, sign conventions, `Enabled=false`
meaning the other keys are ignored, etc.).
**Example:**
```ini
[Exposure]
Enabled=true
Compensation=0.5
```
**Suggested starting values:** subtle / moderate / strong, when meaningful.
```

Rules
* Document EVERY key that `ProcParams::save` writes for the group (also keys
  that are only written conditionally, and keys read by `ProcParams::load` that
  `save` does not write). Names are case-sensitive and must match the code.
* Take type, default and range from the code: defaults in
  `src/engine/procparams.cc` (constructors / `setDefaults`), save/load in the
  same file, GUI ranges from the `Adjuster(...)` constructors and
  `setLimits`/`MyComboBoxText` entries in `src/gui/*.cc` and the matching
  `src/gui/*.h`, enum value names from the `load` code (the strings that are
  written to the file, not the C++ enumerator names).
* Take purpose, behaviour and advice from the docs in
  `/Users/alb/src/artraweditor.github.io` (`Reference.md` is the best source;
  `Book.md` is large, search it with grep, do not read it whole; also
  `Localediting.md`, `Shapes.md`, `Pipeline.md`, `Quickstart.md`, `Luts.md`,
  `Hdroutput.md`, `AItools.md`) and from the code when the docs are silent or
  disagree. If the docs and code disagree, trust the code and say so.
* Do not invent behaviour. If you are unsure, write `(unverified)`.
* If a range is not enforced by the code, say "soft range" and give the GUI one.
* Keep prose tight: no marketing language, no restating the table in prose.
