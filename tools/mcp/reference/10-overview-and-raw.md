# Overview

## What an .arp is

An `.arp` ("ART processing profile") is a **GLib key-file** (INI dialect), UTF-8 text. ART reads it with `Glib::KeyFile` (`KeyFile` wrapper in `src/engine/procparams.cc`).

* `[Group Name]` headers; `Key=Value` lines. **Group and key names are case-sensitive** and must match exactly (`[Black & White]`, `aCurve` vs `ACurve`, `pixelShiftHoleFill` vs `PixelShiftHoleFill` are different).
* Decimal point is `.` (the loader forces the C locale). No thousands separators.
* Booleans: write `true` / `false` (GLib also accepts `1`/`0`; the loader falls back to integer 0/non-0).
* Integers must be written as integers. `Temperature=5500.5` on an integer key raises a parse warning/error and the key is not applied. Doubles accept `1`, `1.0`, `-0.25`.
* Lists are `;`-separated with a trailing `;` (`Curve=1;0;0;0.5;0.6;1;1;`). Curves are lists of doubles; the first number is the curve type (0 Linear, 1 Spline, 2 Parametric, 3 NURBS, 4 CatmullRom), followed by x;y pairs. A curve list must have an odd length of at least 5 (type + 2 points) or be exactly `0;` (identity); anything else is silently replaced by identity (`sanitizeCurve`, `src/engine/curves.cc`). String lists are also `;`-separated.
* Enumerations are **strings** (`HLRecovery=Balanced`, `Method=rcd`). An unrecognised string is silently ignored (key keeps its current value) except where noted (RAW demosaic `Method` falls back to a default).
* Comments: lines starting with `#` are ignored by GLib. (`;` is NOT a comment character; it separates list items.) Do not put trailing comments on a `Key=Value` line: they become part of the value.
* Unknown groups/keys are ignored. Missing keys are ignored (see "partial profiles").
* Strings containing spaces need no quotes (`Ratio=16:9`, `Method=3-pass (best)`).
* File paths (LUTs, dark frames, lens profiles) are written as `file://` URIs by ART; on load a plain absolute path also works (URI decoding falls back to the raw string). Use absolute paths.

## `[Version]`

| Key | Type | Meaning |
|---|---|---|
| `AppVersion` | string | ART release that wrote the file (informational; default = current). |
| `Version` | int | Profile-format version (`PPVERSION`, currently **1047**). Controls legacy conversion on load. |

**Recommendation for hand-written/partial profiles: omit `[Version]` entirely.** When the group is missing, the loader assumes the current version (1047). Writing an older `Version` triggers legacy behaviour: below 1047 `[Grain] ISO` is multiplied by 1.5 and `[Smoothing]` `NoiseCoarseness_*` by 1.5; below 1043 `[Grain] Strength` is doubled; below 1046 the `[DynamicRangeCompression]` group is read from the legacy name `[FattalToneMapping]`; below 1021 `[OutputSharpening]` is read as `[PostResizeSharpening]`; below 1016 `[Smoothing]` is read as `[GuidedSmoothing]`; below 1000 `[TextureBoost]` is read as `[EPD]`; below 329 Channel Mixer / RGB Curves are implicitly enabled; below 350 the old `[Exposure]` group carried tone-curve keys. If you must write it, write `Version=1047`.

## `[General]` (image-level metadata, not processing)

Only read/written when the profile is an image's own sidecar (`load_general`/`save_general`). When a profile is *applied* (pasted, loaded as a profile, `ART-cli -p`) `[General]` is not applied. Do not put it in edit fragments.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `Rank` | int | absent (= -1, unrated) | Star rating, written only when >= 0. GUI uses 0..5 (unverified). |
| `ColorLabel` | int | 0 | Colour label index; 0 = none, 1..5 GUI labels (unverified mapping). |
| `InTrash` | bool | false | Image flagged as trashed in the file browser. |

## `[Snapshots]` (sidecar only)

`ProcParamsWithSnapshots` stores named snapshots: `[Snapshots]` has `Snapshot_1=<name>`, `Snapshot_2=<name>`, ... (consecutive numbering; reading stops at the first gap). Each snapshot's full parameter set lives in groups whose names are prefixed with `Snapshot_N ` (e.g. `[Snapshot_1 Exposure]`, `[Snapshot_1 ToneCurve]`). The unprefixed groups are the current ("master") edit. Edit fragments should never contain snapshots; editing the master groups leaves snapshots untouched.

## Partial profiles

A *partial profile* is simply an `.arp` that contains only some groups and only some keys.

* **Only keys that are present are applied.** The loader does `if (has_key) assign` for every key. Absent keys, and absent groups, keep whatever value the image currently has. There is no "reset" semantics: to return a value to its default you must write the default explicitly (or `Enabled=false`).
* **`Enabled`.** Almost every tool group has `Enabled` (exceptions, which have no switch: `Coarse Transformation`, `Common Properties for Transformations`, `LensProfile` (`LcMode=none` disables), `Color Management`, `MetaData`/`Exif`/`IPTC`, `RAW`, `RAW Bayer`, `RAW X-Trans`; RAW sub-features have their own `...Enabled` keys). When a tool is disabled all its other keys are ignored in processing (they are still stored). Defaults: `Enabled=true` only for `[Exposure]` and `[White Balance]`; **every other tool defaults to `Enabled=false`**. So a fragment like `[Saturation]` `Saturation=30` with no `Enabled=true` has no visible effect unless the tool is already on. **Always write `Enabled=true` for the tool you want to take effect**, and write `Enabled=false` to switch a tool the image already uses off. Writing `Enabled=` alone does not change the other values.
* Keys inside a group are independent (a partial group is fine: `[Exposure]` `Compensation=0.5` leaves `Black` and `HLRecovery` alone). Exceptions where one key's load touches another are listed per group below (notably `[RAW]`: hot/dead pixel keys).
* Curves, lists and per-region data are replaced as a whole when the key is present.
* In the ART-cli, `-p file.arp` applies each given profile in order in **overwrite** mode (`FilePartialProfile(..., append=false)`); later files win key by key.

## Append vs overwrite mode (local-editing tools)

The profile panel has a toggle (option `[Profiles] AppendMode`, default off = overwrite). It only affects the four **region-based local tools**: `[Local Contrast]`, `[TextureBoost]`, `[Smoothing]`, `[ColorCorrection]` (`ParamsEdited::set_append`, `ProcParams::load` `APPEND_` macro).

* Overwrite (default, and what `ART-cli -p` does): if the fragment contains region keys for the tool (`Contrast`, `Contrast_1`, ... plus mask keys), the image's existing regions and masks are **replaced** by the fragment's.
* Append: the image's existing regions/masks are kept and the fragment's regions are added after them.
* All other tools (global ones, and `[Spot Removal]`, `[Crop]`, curves...) are always overwritten per present key. `[Spot Removal]` entries are replaced, not merged (unverified for append mode; `set_append` does not list it).

Region data is serialised as suffixed keys: region 0 has no suffix, region N uses `_N` (`Contrast`, `Contrast_1`, `Contrast_2`), each with its mask keys. See `30-local-editing.md`.

## Default image state (what a fresh image looks like)

A new raw is initialised from the profile named in options `[Profiles] RawDefault` (**`Dynamic`** by default); non-raw from `ImgDefault` (**`Neutral`**). Any existing sidecar `.arp` replaces that start; the MCP edit is then layered on top of whatever state the image is in.

* **Neutral** = built-in `ProcParams()` defaults, no file. Key defaults (all documented per group in the other files): Exposure enabled with `Compensation=0`; White Balance enabled, `Setting=Camera`; every other tool off; `[Color Management]` `InputProfile=(cameraICC)`, `WorkingProfile=Rec2020`, `OutputProfile=RTv2_sRGB`, `OutputProfileIntent=Relative`, `ApplyBaselineExposureOffset=true`, `ApplyHueSatMap=true`; `[LensProfile]` `LcMode=none`; `[RAW Bayer]` `Method=rcd`; `[RAW X-Trans]` `Method=3-pass (best)`; ToneCurve off (so a Neutral raw looks flat/linear-ish with only the camera matrix and baseline exposure applied).
* **Dynamic** = rule list `dynamicprofile.cfg` (user dir `~/.config/ART/dynamicprofile.cfg`, shipped copy `data/dynamicprofile.cfg`). Each rule matches on ISO, aperture, focal length, shutter, exposure compensation, camera, lens, image type; **all matching rules are applied in order, each on top of the previous** (`ProfileStore::loadDynamicProfile`). Shipped rules: all images get `Auto-Matched Curve` (ToneCurve enabled with `HistogramMatching=true`, `HLRecovery=Balanced`, lens `exif` mode, raw CA correction); ISO <= 640 additionally `Sharpening` (`[Sharpening]` rld deconvolution, auto radius); ISO 640..51200 `Noise Reduction Low`; ISO 3200..51200 `Noise Reduction High`. Profile files live in `data/profiles/` (`Auto-Matched Curve.arp`, `Noise Reduction High.arp`, `Noise Reduction Low.arp`, `Sharpening.arp`, `Simple Film Curve.arp`, `Standard Film Curve.arp`) plus the user's profile directory (`~/.config/ART/profiles`, referenced as `${U}/...`; bundled as `${G}/...`). **On this machine** `~/.config/ART/profiles` is a symlink to a personal directory and rule 1 applies `${U}/default` (full profile: tone curve off, WorkingProfile Rec2020, OutputProfile RTv2_sRGB, raw CA on); the actual starting point is therefore user-specific. Another install has other defaults: if the starting state matters, ask the application for the current profile rather than assuming. (There is no `rtdata/` directory in this checkout; resources are under `data/`.)
* Consequence for edit fragments: the base may already have ToneCurve (with histogram-matched curve), Denoise or Sharpening enabled. Writing `Enabled=true` plus your keys overrides only what you name; to remove an existing effect write `Enabled=false` for that tool.

# Pipeline order

Order the engine executes steps (export path `src/engine/simpleprocess.cc`; preview path in `improccoordinator.cc`/`dcrop.cc` is the same sequence; `Pipeline.md` of the docs site agrees except where noted). "Group" is the arp group controlling the step.

1. Flat field, dark frame — `[RAW]` (`DarkFrame*`, `FlatField*`), on raw data.
2. Raw black points / pre-exposure — `[RAW Bayer]`/`[RAW X-Trans]` `PreBlack*`, `[RAW]` `PreExposure`.
3. Lens-profile vignetting correction — `[LensProfile]` `UseVignette` (skipped when a flat field is used) — on raw data.
4. Hot/dead pixel filter, line-noise / green equilibration / PDAF filters — `[RAW]`, `[RAW Bayer]`.
5. Raw chromatic aberration correction — `[RAW]` `CA*`.
6. Demosaic — `[RAW Bayer]`/`[RAW X-Trans]` `Method`, `Border`, `CcSteps`. (Coarse rotation/flip, `[Coarse Transformation]`, is applied with the image data at load.)
7. Highlight recovery and raw highlight/exposure multipliers — `[Exposure]` `HLRecovery*` (applied while the image is read out, in camera space).
8. White balance — `[White Balance]`.
9. Spot removal — `[Spot Removal]` (camera space; before the input profile).
10. Input colour profile / camera -> working space conversion — `[Color Management]` (`InputProfile`, `WorkingProfile`, DCP options). With `[Film Negative]` `ColorSpace=1` (working) the conversion happens before film negative inversion, otherwise after.
11. Film negative inversion — `[Film Negative]`.
12. Denoise — `[Denoise]` (linear RGB, whole image, before crop).
13. Stage 0 (whole-image tools): Dehaze `[Dehaze]`, then Dynamic range compression `[DynamicRangeCompression]`. (If either is on, a crop does not restrict their input.)
14. Geometry: lens-profile distortion / CA, `[CACorrection]`, `[Distortion]`, `[Rotation]`, `[Perspective]`, `[Vignetting Correction]` (manual), with `[Common Properties for Transformations]` `AutoFill`.
15. **Crop** — `[Crop]` (applied here, i.e. before the tone/colour stages below; later tools like gradient/vignette/local masks are positioned relative to the crop).
16. Stage 1: Channel mixer `[Channel Mixer]`, Exposure compensation/black `[Exposure]` (`Compensation`, `Black`), HSL equalizer `[HSL Equalizer]`, Tone equalizer `[ToneEqualizer]`; ProPhoto blue fix when working profile is ProPhoto.
17. Stage 2: DCP "look" if `DCPLookEarly=true` (`[Color Management]`); Sharpening `[Sharpening]`, Impulse denoise `[Impulse Denoising]`, Defringe `[Defringing]` (preview/output only); Color correction `[ColorCorrection]`; Smoothing `[Smoothing]`.
18. Stage 3: Gradient and PC vignette `[Gradient]`, `[PCVignette]`; TextureBoost `[TextureBoost]`; Grain `[Grain]`; Log encoding `[LogEncoding]`; Saturation/vibrance `[Saturation]`; DCP look (late mode); Film simulation `[Film Simulation]` (before tone curve unless `AfterToneCurve=true`); Tone curve `[ToneCurve]`; RGB curves `[RGB Curves]`; Luminance/LAB curves `[Luminance Curve]`; Soft light `[SoftLight]`; Local contrast `[Local Contrast]`; Black & white `[Black & White]` (last tonal step).
   (Doc/code disagreement: `Pipeline.md` lists Grain after Black & white and places Saturation before Film simulation only loosely; the code order above is authoritative. In the code Exposure is after Dehaze/DRC/geometry and Channel Mixer.)
19. Resize — `[Resize]` (Lanczos; if resizing is enabled the export may resize early, before denoise, for speed — "fast pipeline").
20. Output sharpening — `[OutputSharpening]` (after resize, linear RGB).
21. Output colour conversion and ICC embedding — `[Color Management]` `OutputProfile`, `OutputProfileIntent`, `OutputBPC`.
22. Metadata writing — `[MetaData]`, `[Exif]`, `[IPTC]`.

Practical rules: Exposure comes before ToneCurve, Saturation and Film Simulation (it is scene-linear); ToneCurve/RGB curves/LAB curves/Soft light/B&W work on already exposed, display-referred-ish data; Denoise/Sharpen see no crop/resize; Crop is before tone/colour; Resize and OutputSharpening are last.

# Tool index

Every group `ProcParams::save` can write (48, plus `[Snapshots]` written only by the sidecar writer). Files: **10** = `10-overview-and-raw.md`, **20** = `20-tone-and-colour.md`, **30** = `30-local-editing.md`, **40** = `40-detail-and-geometry.md`.

| Group (exact) | What it does | Doc |
|---|---|---|
| `Version` | Format version, see above | 10 |
| `General` | Rank, colour label, trash flag (sidecar only) | 10 |
| `Snapshots` | Named snapshots (sidecar only) | 10 |
| `Exposure` | Exposure compensation (stops), black level, highlight recovery | 20 |
| `Saturation` | Global saturation and vibrance | 20 |
| `ToneCurve` | Contrast, base curve, two-stage tone curves, histogram matching | 20 |
| `Luminance Curve` | LAB L/a/b curves, brightness, contrast, chromaticity | 20 |
| `Channel Mixer` | RGB matrix / hue-sat tweak mixing | 20 |
| `Black & White` | Monochrome conversion with channel mixing and filters | 20 |
| `HSL Equalizer` | Hue/saturation/lightness curves by hue | 20 |
| `ToneEqualizer` | 5-band exposure by luminance zone (shadows..highlights) | 20 |
| `LogEncoding` | Log tone mapping of scene range | 20 |
| `DynamicRangeCompression` | Fattal-style HDR compression | 20 |
| `SoftLight` | Soft-light blend contrast/glow | 20 |
| `Film Simulation` | Apply a HaldCLUT/CLF/CTL film look | 20 |
| `RGB Curves` | Per-channel RGB curves | 20 |
| `White Balance` | Temperature/tint/multipliers | 20 |
| `Dehaze` | Haze removal | 20 |
| `Grain` | Film grain | 20 |
| `Film Negative` | Invert scanned colour negatives | 20 |
| `Color Management` | Input/working/output colour profiles, DCP options | 20 |
| `ColorCorrection` | Region-based colour grading (slope/offset/power, HSL wheels) | 30 |
| `Smoothing` | Region-based smoothing/blur/noise | 30 |
| `Local Contrast` | Region-based local contrast | 30 |
| `TextureBoost` | Region-based texture/clarity | 30 |
| `Spot Removal` | Clone/heal spots | 30 |
| `Gradient` | Graduated filter | 30 |
| `PCVignette` | Post-crop artistic vignette | 30 |
| `Sharpening` | Capture sharpening (USM/RL deconvolution/PSF) | 40 |
| `Impulse Denoising` | Salt-and-pepper removal | 40 |
| `Defringing` | Purple/colour fringe removal | 40 |
| `Denoise` | Luminance/chrominance noise reduction | 40 |
| `CACorrection` | Post-demosaic lateral CA correction | 40 |
| `Vignetting Correction` | Manual lens vignetting correction | 40 |
| `Crop` | Crop rectangle and aspect | 40 |
| `Coarse Transformation` | 90-degree rotation and flips | 40 |
| `Common Properties for Transformations` | `AutoFill` for rotate/perspective | 40 |
| `Rotation` | Fine rotation (-45..45 degrees) | 40 |
| `Distortion` | Manual/auto distortion | 40 |
| `LensProfile` | Lensfun/LCP/EXIF-based lens correction | 40 |
| `Perspective` | Perspective correction | 40 |
| `Resize` | Output resize | 40 |
| `OutputSharpening` | Post-resize sharpening | 40 |
| `MetaData` | Metadata copy/edit mode, EXIF keys, notes | 40 |
| `Exif` | EXIF value overrides (key=value pairs) | 40 |
| `IPTC` | IPTC overrides | 40 |
| `RAW` | Dark frame, flat field, raw CA, hot/dead pixels, pre-exposure | 10 |
| `RAW Bayer` | Demosaic and preprocessing for Bayer sensors | 10 |
| `RAW X-Trans` | Demosaic for Fujifilm X-Trans sensors | 10 |

Naming notes: a scan of `ProcParams::save` shows a literal `"RAW\n            // Bayer"`; this is not a real group, it is a commented-out line (DCB iterations) that was wrapped in the source, so the real name is `RAW Bayer`. Group names with spaces or symbols are real and must be copied exactly: `Black & White`, `HSL Equalizer`, `Luminance Curve`, `Local Contrast`, `White Balance`, `Film Simulation`, `Film Negative`, `Color Management`, `RGB Curves`, `Spot Removal`, `Channel Mixer`, `Vignetting Correction`, `Coarse Transformation`, `Impulse Denoising`, `RAW X-Trans`, `RAW Bayer`, `Common Properties for Transformations`. Group names with no space: `ToneCurve`, `ToneEqualizer`, `TextureBoost`, `ColorCorrection`, `LogEncoding`, `SoftLight`, `PCVignette`, `CACorrection`, `LensProfile`, `OutputSharpening`, `MetaData`, `DynamicRangeCompression`. Legacy read-only names: `HLRecovery`, `FattalToneMapping`, `EPD`, `GuidedSmoothing`, `PostResizeSharpening`.

# Recipes

Each snippet is a complete partial profile (omit `[Version]`). Ranges are the GUI ranges; see the group files for details. Values are starting points; adjust to the image.

**1. Brighter / darker.** Exposure is in stops (range -12..12). +0.5 is clearly brighter; -0.5 clearly darker.
```ini
[Exposure]
Enabled=true
Compensation=0.5
```

**2. More (or less) contrast.** `Contrast` -100..100 in ToneCurve (a base curve of `SCurve` adds a filmic S shape); add local contrast for midtone punch.
```ini
[ToneCurve]
Enabled=true
Contrast=25

[Local Contrast]
Enabled=true
Contrast=20
```
Note: if the image's base profile already uses a histogram-matched curve, adding `Contrast` stacks on top. For a flatter look use negative `Contrast`.

**3. Recover highlights and lift shadows.** Lower exposure slightly with highlight reconstruction, then use the tone equalizer (bands -100..100; Band0 = darkest zone to Band4 = brightest, mapping unverified, see 20-tone-and-colour.md).
```ini
[Exposure]
Enabled=true
Compensation=-0.3
HLRecovery=Balanced

[ToneEqualizer]
Enabled=true
Band0=40
Band1=25
Band3=-20
Band4=-35
```
Lift shadows only: keep just `Band0`/`Band1` positive. `HLRecovery` values: `Off`, `Blend`, `Color`, `Balanced`.

**4. Warmer / cooler.** Custom temperature in Kelvin (integer, 1500..60000), `Green` 0.02..10 is the tint (1.0 neutral). Raising `Temperature` makes the image warmer. The value is absolute, so know the as-shot temperature first (compare to the camera setting); +/-500 K from it is a visible change.
```ini
[White Balance]
Enabled=true
Setting=CustomTemp
Temperature=6500
Green=1.0
```

**5. More vivid / more muted.** Both -100..100; vibrance protects already saturated colours.
```ini
[Saturation]
Enabled=true
Saturation=10
Vibrance=30
```
Muted: `Saturation=-30`, `Vibrance=-10`.

**6. Black and white.**
```ini
[Black & White]
Enabled=true
Setting=Luminance
```
`Setting` presets include `NormalContrast`, `HighContrast`, `Luminance`, `Landscape`, `Portrait`, `InfraRed`, `RGB-Rel` (default); `Filter` takes `None`, `Red`, `Orange`, `Yellow`, ... Combine with recipe 2 for contrast.

**7. Sharpen.** Deconvolution (`rld`, default) with automatic radius is the gentle choice; `Method=usm` is classic unsharp mask.
```ini
[Sharpening]
Enabled=true
Method=rld
DeconvAmount=100
DeconvAutoRadius=true
Contrast=20
```
USM variant: `Method=usm`, `Amount=200` (1..1000), `Radius=0.5` (0.3..3).

**8. Reduce noise.** Chrominance automatic, luminance 0..100.
```ini
[Denoise]
Enabled=true
Luminance=20
ChrominanceMethod=1
ChrominanceAutoFactor=1
```
`ChrominanceMethod` 0 = manual (uses `Chrominance`), 1 = automatic. Stronger: `Luminance=40`, `Aggressive=true`.

**9. Straighten and crop to 16:9.** `Degree` -45..45 (sign convention unverified; try small values and check). Crop coordinates are pixels in the image after geometry corrections (unverified exact reference frame), so compute them from the image size. Example for a 6000x4000 frame: 16:9 -> 6000x3375, centred vertically (Y = (4000-3375)/2 = 312).
```ini
[Rotation]
Enabled=true
Degree=-1.2

[Crop]
Enabled=true
X=0
Y=312
W=6000
H=3375
FixedRatio=true
Ratio=16:9
Orientation=Landscape
```
Rotation with `AutoFill` (`[Common Properties for Transformations]` `AutoFill=true`) crops away the empty corners.

**10. Vignette and film look.** `[PCVignette]` darkens corners by `Strength` stops (-6..6; negative brightens); film look = grain + gentle soft light + optional LUT.
```ini
[PCVignette]
Enabled=true
Strength=1.0
Feather=60
Roundness=50

[Grain]
Enabled=true
ISO=800
Strength=40

[SoftLight]
Enabled=true
Strength=20
```
To apply a film LUT add `[Film Simulation]` with `Enabled=true`, `ClutFilename=<absolute path to the .png/.clf/.ctl>`, `Strength=100` (0..100).

# Raw groups: `[RAW]`, `[RAW Bayer]`, `[RAW X-Trans]`

General notes for all three groups:
* They apply **only to raw files**; for JPEG/TIFF/PNG input they are ignored. `[RAW Bayer]` only applies to Bayer sensors, `[RAW X-Trans]` only to Fujifilm X-Trans sensors (a fragment may contain both; the wrong one is ignored).
* Most of these tools are for reproducing a given look or fixing camera-specific defects, and are **rarely needed for ordinary edits**. Do not touch them for "brighter", "more contrast", etc. The Reference docs say of Raw Black Points and Raw Gain/White Point that "it is unlikely you will ever need" them other than for diagnostics.
* Raw tools run first in the pipeline (before white balance and everything else) and change the entire downstream look, so changing them in a fragment re-renders the whole image.
* Doc/code note: the docs call the demosaic list "AMaZE, RCD, LMMSE, IGV, ..."; the file values are lower-case as below. The code also contains legacy methods (`dcb`, `ahd`, `eahd`, `hphd`, `amazevng4`, `rcdvng4`, `dcbbilinear`, `dcbvng4`) that are **not selectable**: on load `amazevng4` and `dcbvng4` map to `amazebilinear`, `rcdvng4` maps to `rcdbilinear`, any other unknown string becomes `amaze`.

## RAW  —  `[RAW]`
**What it does:** Sensor-level corrections applied to raw data before demosaicing: dark-frame subtraction, flat-field correction, raw chromatic aberration correction, hot/dead pixel replacement, and a pre-demosaic exposure multiplier. No group-wide `Enabled`; each feature has its own switch.
**Typical requests:** "remove hot pixels", "fix colour fringing in the raw (lateral CA)", "subtract a dark frame", "apply a flat field".
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `DarkFrameEnabled` | bool | false | | Enable dark-frame subtraction. |
| `DarkFrame` | path/URI | empty | existing raw file | Dark frame file. Ignored when `DarkFrameAuto=true`. |
| `DarkFrameAuto` | bool | false | | Pick a matching dark frame from the dark-frame directory in Preferences. |
| `FlatFieldEnabled` | bool | false | | Enable flat-field correction (raw only). |
| `FlatFieldFile` | path/URI | empty | existing raw file | Flat-field image. Ignored when `FlatFieldAutoSelect=true`. |
| `FlatFieldAutoSelect` | bool | false | | Auto-select a flat field from the flat-field directory. |
| `FlatFieldBlurRadius` | int | 32 | 0..200 (GUI) | Blur of the flat-field data; 0 = no blur (corrects dust on the sensor). |
| `FlatFieldBlurType` | string | `Area Flatfield` | `Area Flatfield`, `Vertical Flatfield`, `Horizontal Flatfield`, `V+H Flatfield` | Direction of the blur. |
| `FlatFieldAutoClipControl` | bool | false | | Automatic clip control. |
| `FlatFieldClipControl` | int | 0 | 0..100 (GUI) | Keeps the flat field from clipping near-overexposed areas; clipped areas may gain a colour cast. |
| `FlatFieldUseEmbedded` | bool | false | | Use the flat field embedded in the raw (e.g. DNG gain maps) (unverified). |
| `CAEnabled` | bool | false | | Enable raw CA correction. |
| `CA` | bool | false | | Automatic CA correction. When true `CARed`/`CABlue` are ignored. |
| `CAAvoidColourshift` | bool | true | | Avoid colour shift in the automatic correction. |
| `CAAutoIterations` | int | 2 | 1..5 (GUI) | Iterations of the automatic correction. |
| `CARed` | double | 0.0 | -4.0..4.0 (GUI) | Manual red/green CA shift (used when `CA=false`). |
| `CABlue` | double | 0.0 | -8.0..8.0 (GUI) | Manual blue/yellow CA shift (used when `CA=false`). |
| `HotDeadPixelEnabled` | bool | false | | Enable the hot/dead pixel filter. |
| `HotPixelFilter` | bool | false | | Replace hot pixels. |
| `DeadPixelFilter` | bool | false | | Replace dead pixels. |
| `HotDeadPixelThresh` | int | 100 | 20..200 (GUI) | Detection threshold (higher = fewer pixels detected). |
| `PreExposureEnabled` | bool | false | | Enable the raw gain / white-point multiplier. |
| `PreExposure` | double | 1.0 | 0.1..32 (GUI, log scale) | Linear multiplier applied to raw data before demosaic (not stops). Diagnostic use. |
Read-only legacy keys: `HotDeadPixels` (bool; old combined switch that sets both hot and dead filter), and, for profiles older than version 320, Bayer keys in this group (`CcSteps`, `LMMSEIterations`, `LineDenoise`, `GreenEqThreshold`, `PreBlackzero/one/two/three`, `PreTwoGreen`). Do not use them.
**How to write it:** `CAEnabled=true` together with `CA=true` is the usual "automatic raw CA correction" (it is part of the shipped default profiles). Paths: absolute path (ART itself writes `file://` URIs). Booleans lowercase.
**Interactions & pitfalls:**
* **Load quirk:** whenever a profile contains a `[RAW]` group, the loader executes `DeadPixelFilter := HotPixelFilter` *before* reading `HotPixelFilter`/`DeadPixelFilter`. So a `[RAW]` fragment that does not list `DeadPixelFilter` will copy the current `HotPixelFilter` value into the dead-pixel setting. Always write `HotPixelFilter` and `DeadPixelFilter` together, or neither plus no other `[RAW]` keys if you care.
* Raw CA correction (`[RAW]` `CA*`) works before demosaic; `[CACorrection]` (Transform) works after demosaic. Use one, not both.
* A feature switched off by its `...Enabled=false` ignores all its other keys.
* Raw CA correction is already on in the shipped Dynamic profiles; leave it alone unless asked.
* Dark frame / flat field need real files; never guess paths.
**Example:**
```ini
[RAW]
CAEnabled=true
CA=true
HotDeadPixelEnabled=true
HotPixelFilter=true
DeadPixelFilter=true
HotDeadPixelThresh=100
```
**Suggested starting values:** hot/dead threshold 100 (lower to 40..60 for more aggressive removal); CA automatic with 2 iterations.

## RAW Bayer  —  `[RAW Bayer]`
**What it does:** Demosaicing (turning the Bayer mosaic into RGB) and Bayer-specific preprocessing: pre-demosaic black levels, line-noise/green-equilibration/PDAF filters, and pixel-shift (multi-shot) handling. The demosaic method affects fine detail and "maze" artefacts at 100% zoom; AMaZE/RCD are the quality defaults, LMMSE/IGV suit very noisy high-ISO images.
**Typical requests:** "use a different demosaic for this noisy shot", "remove banding/line noise", "fix green-split maze pattern", "pixel-shift combine".
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Method` | string | `rcd` | `amaze`, `rcd`, `lmmse`, `igv`, `amazebilinear`, `rcdbilinear`, `vng4`, `fast`, `mono`, `pixelshift`, `none` | Demosaic algorithm (`amaze` is AMaZE; `none` shows the raw sensor pattern; `mono` for monochrome cameras; `pixelshift` for Pentax/Sony multi-shot raw). `*bilinear` are dual-demosaic variants (detail from AMaZE/RCD, flat areas bilinear). |
| `Border` | int | 4 | 0..16 | Pixels cropped around the edge to hide demosaic artefacts. 0 keeps full size (only needed e.g. for 1080p DNG video frames). |
| `ImageNum` | int | 1 | 1..4 (GUI offers 1,2,...) | Which sub-image of a multi-image raw (written 1-based; internally minus one). |
| `CcSteps` | int | 0 | 0..5 | False-colour suppression median passes. |
| `PreBlackEnabled` | bool | false | | Enable the raw black-point offsets below. |
| `PreBlack0` | double | 0 | -2048..2048 | Black-level offset, CFA channel 0 (R). |
| `PreBlack1` | double | 0 | -2048..2048 | Channel 1 (G1). |
| `PreBlack2` | double | 0 | -2048..2048 | Channel 2 (B). |
| `PreBlack3` | double | 0 | -2048..2048 | Channel 3 (G2). Channel-to-colour mapping follows CFA layout (unverified). |
| `PreTwoGreen` | bool | true | | Treat the two greens as one black-level channel. |
| `PreprocessingEnabled` | bool | false | | Enable the preprocessing group (`LineDenoise`, `LineDenoiseDirection`, `GreenEqThreshold`). |
| `LineDenoise` | int | 0 | 0..1000 | Line-noise filter strength (banding). |
| `LineDenoiseDirection` | int | 3 | 1 = horizontal, 2 = vertical, 3 = both, 5 = PDAF lines | Direction of the banding. |
| `GreenEqThreshold` | int | 0 | 0..100 | Green equilibration (fixes green-split mazing). |
| `PDAFLinesFilter` | bool | false | | Remove phase-detection AF line stripes (backlit flare). |
| `DynamicRowNoiseFilter` | bool | false | | Correct dynamic row noise; needs optical-black columns in the raw. |
| `LMMSEIterations` | int | 2 | 0..6 | LMMSE refinement passes (only for `lmmse`). |
| `DualDemosaicAutoContrast` | bool | true | | Automatic contrast threshold for `amazebilinear`/`rcdbilinear`. |
| `DualDemosaicContrast` | double | 20 | 0..100 | Contrast threshold when auto is off. |
| `PixelShiftMotionCorrectionMethod` | int | 1 | 0 = off, 1 = auto, 2 = custom | Motion handling for pixel shift. |
| `PixelShiftEperIso` | double | 0.0 | -5.0..5.0 | Noise-estimate ISO adjustment (custom motion mode). |
| `PixelShiftSigma` | double | 1.0 | 0.5..25 | Motion-mask blur sigma. |
| `PixelShiftShowMotion` | bool | false | | Overlay motion mask (diagnostic). |
| `PixelShiftShowMotionMaskOnly` | bool | false | | Show only the mask (diagnostic). |
| `pixelShiftHoleFill` | bool | true | | Fill holes in the motion mask. |
| `pixelShiftMedian` | bool | false | | Median filter on motion areas. |
| `pixelShiftGreen` | bool | true | | Use the green channel for motion detection. |
| `pixelShiftBlur` | bool | true | | Blur the motion mask. |
| `pixelShiftSmoothFactor` | double | 0.7 | 0..1 | Mask transition smoothing. |
| `pixelShiftEqualBright` | bool | false | | Equalise brightness between frames. |
| `pixelShiftEqualBrightChannel` | bool | false | | Equalise per channel. |
| `pixelShiftNonGreenCross` | bool | true | | Use non-green cross-checking in detection. |
| `pixelShiftDemosaicMethod` | string | `amaze` | `amaze`, `amazevng4`, `lmmse` | Demosaic used for motion areas of a pixel-shift image. |
Read-only legacy key: `pixelShiftLmmse` (bool, profile version < 336). Dormant (not saved or loaded): DCB iterations/enhance.
**How to write it:** exact lower-case method strings as above (`Method=amaze`, not `AMaZE`). Pixel-shift key spelling is mixed: `PixelShiftEperIso`, `PixelShiftSigma`, `PixelShiftShowMotion`, `PixelShiftShowMotionMaskOnly`, `PixelShiftMotionCorrectionMethod` start upper-case; `pixelShiftHoleFill`, `pixelShiftMedian`, `pixelShiftGreen`, `pixelShiftBlur`, `pixelShiftSmoothFactor`, `pixelShiftEqualBright`, `pixelShiftEqualBrightChannel`, `pixelShiftNonGreenCross`, `pixelShiftDemosaicMethod` start lower-case. Copy exactly. `LineDenoiseDirection` and `PixelShiftMotionCorrectionMethod` are integers, not names.
**Interactions & pitfalls:**
* Changes are only visible at 100% preview zoom or above.
* `PreBlack*`, line noise etc. only take effect when their group switch (`PreBlackEnabled`, `PreprocessingEnabled`) is true. `PDAFLinesFilter` and `DynamicRowNoiseFilter` are saved with the preprocessing group but have no separate switch of their own in the file; whether they require `PreprocessingEnabled=true` is unverified.
* For `Method=pixelshift` on a non-pixel-shift raw the engine falls back (unverified).
* Normal photographic requests never need this group; leave default (`rcd` here, AMaZE per docs) unless the user asks for a demosaic change.
**Example:**
```ini
[RAW Bayer]
Method=lmmse
LMMSEIterations=2
CcSteps=1
PreprocessingEnabled=true
GreenEqThreshold=20
```
**Suggested starting values:** noisy high-ISO: `Method=lmmse` or `igv`; green maze: `PreprocessingEnabled=true`, `GreenEqThreshold=10..30`; banding: `LineDenoise=100..300` with a direction.

## RAW X-Trans  —  `[RAW X-Trans]`
**What it does:** Demosaicing and black levels for Fujifilm X-Trans (6x6 CFA) sensors. Ignored for Bayer cameras.
**Typical requests:** "use faster demosaic", "better X-Trans detail".
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Method` | string | `3-pass (best)` | `4-pass`, `3-pass (best)`, `2-pass`, `1-pass (medium)`, `fast`, `mono`, `none` | Demosaic algorithm. The strings contain spaces and parentheses exactly as shown. Unknown string -> `3-pass (best)`. |
| `DualDemosaicAutoContrast` | bool | true | | Automatic contrast threshold for the dual demosaic. |
| `DualDemosaicContrast` | double | 20 | 0..100 | Contrast threshold when auto is off. Whether it affects any current X-Trans method is unverified. |
| `Border` | int | 7 | 0..16 | Cropped border pixels. |
| `CcSteps` | int | 0 | 0..5 | False-colour suppression passes. |
| `PreBlackEnabled` | bool | false | | Enable the black offsets. |
| `PreBlackRed` | double | 0 | -2048..2048 | Red black-level offset. |
| `PreBlackGreen` | double | 0 | -2048..2048 | Green black-level offset. |
| `PreBlackBlue` | double | 0 | -2048..2048 | Blue black-level offset. |
**How to write it:** quote nothing: `Method=1-pass (medium)`.
**Interactions & pitfalls:** differences between `3-pass (best)` and `1-pass (medium)` show only on low-ISO images at 100%; use `1-pass (medium)` for speed on high ISO. Raw preprocessing (line noise, green equilibration), pixel shift and the hot-pixel keys of `[RAW]` are Bayer-only (hot/dead filter and CA live in `[RAW]` and work for both).
**Example:**
```ini
[RAW X-Trans]
Method=1-pass (medium)
CcSteps=0
```
**Suggested starting values:** default `3-pass (best)`; `1-pass (medium)` for quick previews or ISO >= 3200.
