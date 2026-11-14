# Tone and colour tools

Groups covered: `[White Balance]`, `[Film Negative]`, `[Color Management]`,
`[Dehaze]`, `[DynamicRangeCompression]`, `[Channel Mixer]`, `[Exposure]`,
`[HSL Equalizer]`, `[ToneEqualizer]`, `[Grain]`, `[LogEncoding]`,
`[Saturation]`, `[Film Simulation]`, `[ToneCurve]`, `[RGB Curves]`,
`[Luminance Curve]`, `[SoftLight]`, `[Black & White]`.

General rules for every group below:
* Syntax is GLib keyfile: `Key=value`, booleans are `true`/`false`, the decimal
  point is `.`, lists are `;`-separated (a trailing `;` is normal).
* A fragment that omits the `[Version]` group is read as the current profile
  version (1047), which is what this document describes.
* A key that is absent keeps its default (or whatever the base profile has).
  Within a group that is present, `Enabled` is only read if present: write
  `Enabled=true` explicitly when you want a tool on. Defaults are `false` for
  every tool here except `[Exposure]` and `[White Balance]` (default `true`).
* If `Enabled=false` the tool is skipped entirely and its other keys have no
  visible effect.
* "GUI range" below is the slider range; the file loader does not clamp, so
  out-of-range values are accepted but untested ("soft range").

## Curves and how to serialise them

Every curve is a single key whose value is a `;`-separated list of numbers.
The first number is the curve type; the rest depend on the type. There are two
families.

**Diagonal curves** (x = input, y = output, both 0..1): used by
`[ToneCurve]` `Curve`, `Curve2`, `Saturation2`; `[RGB Curves]` `rCurve`,
`gCurve`, `bCurve`; `[Luminance Curve]` `LCurve`, `aCurve`, `bCurve`.

| First number | Type | GUI name | Rest of the list |
|---|---|---|---|
| `0` | Linear | Linear | alone (`0;`) = identity. With points it is read as piecewise-linear through the points (engine behaviour; the GUI normally only writes `0;`). |
| `1` | Spline | "Standard" (cubic spline through the points) | `x1;y1;x2;y2;...` |
| `4` | CatmullRom | "Flexible" (passes through the points, local edits) | `x1;y1;x2;y2;...` |
| `3` | NURBS | "Control cage" (points are control handles; the curve does not pass through them) | `x1;y1;x2;y2;...` |
| `2` | Parametric | "Parametric" | exactly 7 numbers after the type, see below |

Point lists (types 1, 3, 4, 0):
* Pairs `x;y`, 0..1, x strictly ascending. Include the end points explicitly,
  normally `0;0;` first and `1;1;` last (move them to lift blacks or lower
  whites). If the first x is not 0 or the last x is not 1 the curve is not
  treated as identity even if all points are on the diagonal.
* At least 2 points are needed. The loader rejects lists with an even length or
  fewer than 5 numbers (type byte + 2 points) and silently replaces them with
  identity (`0;`). A type with a single pair, a lone pair of numbers, or a
  missing y therefore does nothing.
* A curve whose points all satisfy x == y (to 1e-5) is treated as identity.
* Spline, NURBS and CatmullRom with only 2 points degrade to a straight line.
* Example (gentle S curve, Flexible): `4;0;0;0.25;0.20;0.75;0.80;1;1;`
* Example (lift blacks, Standard): `1;0;0.06;0.5;0.5;1;1;`

Parametric (type 2): `2;s1;s2;s3;highlights;lights;darks;shadows;` (8 numbers
total).
* `s1;s2;s3` are the three split points between shadows/darks/lights/
  highlights, 0..1, strictly ascending (GUI defaults `0.25;0.5;0.75`; if not
  ascending the loader resets them to those defaults; values are clamped to
  0.001..0.99).
* The last four are the slider values, -100..100, in this order: `highlights`,
  `lights`, `darks`, `shadows` (positive = brighter). All four 0 = identity.
* A ninth number is accepted by the engine; do not write it (meaning
  unverified).
* Example: `2;0.25;0.5;0.75;-20;0;10;15;` (darker highlights, lifted shadows).

**Flat (periodic or 0..1) curves** (x = a position, y = an amount where 0.5 is
"no change"): used by `[HSL Equalizer]` `HCurve`, `SCurve`, `LCurve`;
`[ToneCurve]` `Saturation`; `[Dehaze]` `Strength`.
* `0;` (type Linear/empty) = identity.
* `1;` followed by one 4-tuple per node: `x;y;leftTangent;rightTangent;`.
  `x` 0..1 ascending, `y` 0..1 with `0.5` neutral, tangents 0..1 control how
  much the curve flattens around the node (GUI default `0.35`; `0` makes a
  straight segment/corner towards that neighbour). A list needs at least one
  node (5 numbers); the length must be 1 + 4*N.
* The list is treated as identity when every node has y == 0.5.
* For `[HSL Equalizer]` the curve is periodic: x wraps from 1 back to 0.
* Example (all nodes neutral, the GUI default for HSL):
  `1;0;0.5;0.35;0.35;0.1667;0.5;0.35;0.35;0.3333;0.5;0.35;0.35;0.5;0.5;0.35;0.35;0.6667;0.5;0.35;0.35;0.8333;0.5;0.35;0.35;`

**Non-flat curve value spaces**
* `[Luminance Curve]` curves act on CIE L*a*b* values (x/y = L, a or b
  rescaled to 0..1, 0.5 = a/b of zero).
* `[RGB Curves]` and `[ToneCurve]` curves are applied on the working-space RGB
  after the earlier tools (display-like, 0..1 = black to white). The exact axis
  encoding is not documented in the code reviewed (unverified); treat 0.5 as a
  mid-tone, not as 18% grey.

### Order of operations (for the groups in this file)
Per `ImProcFunctions::process` (`src/engine/improcfun.cc`), after the raw
stage (film negative, demosaic, highlight reconstruction from `[Exposure]`,
white balance) and the input colour profile (`[Color Management]`): Dehaze,
Dynamic Range Compression, Channel Mixer, Exposure compensation/black, HSL
Equalizer, Tone Equalizer, (sharpening, colour correction, smoothing, gradients,
texture boost), Grain, Log Encoding, Saturation/Vibrance, DCP look (if "later"),
Film Simulation (before the tone curve unless `AfterToneCurve=true`), Tone Curve,
RGB Curves, Luminance (L\*a\*b\*) Curve, Soft Light, local contrast, Black &
White, then output profile conversion. So Black & White comes after almost
everything and desaturates whatever colour work was done before it, and
Saturation acts before the tone curve. (`doc` disagreement: `Pipeline.md` lists
Film Grain after Black & White; the code runs it before Log Encoding.)

---

## White Balance  —  `[White Balance]`
**What it does:** Neutralises the colour cast of the light source by applying
per-channel gains (red/green/blue multipliers). It runs on the raw data right
after demosaicing and highlight reconstruction, before the input colour profile,
so it changes every colour. Pushed too far you get strong casts, noisy
channels and clipped colours. It is meant for correctness, not for artistic
grading (use Channel Mixer, HSL Equalizer or RGB Curves for that).
**Typical requests:** "warmer", "cooler", "fix the colour cast", "make whites
white", "less green/magenta", "use the camera white balance", "tungsten look".
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `true` | `true`/`false` | `false` = no white balance gains at all (unity multipliers), image looks very green/dark for a raw. |
| `Setting` | string | `Camera` (when the key is written by ART). **If the group is present in a fragment but `Setting` is missing, the loader uses `CustomTemp`.** | `Camera`, `Auto`, `CustomTemp`, `CustomMult`, `CustomMultLegacy` | Method. `Camera` = as shot (raw files). `Auto` = estimate from the image. `CustomTemp` = use `Temperature`/`Green`/`Equal`. `CustomMult` = use `Multipliers`. `CustomMultLegacy` = same with pre-v1027 interpretation; do not write. |
| `Temperature` | int (Kelvin) | `6504` | GUI 1500..60000 | Colour temperature of the illuminant. **Higher value = the image is made warmer (more yellow/orange)**, lower = cooler (bluer); this is the temperature you tell ART the light *was*. Only used by `CustomTemp` (for `Camera`/`Auto` ART overwrites it with the computed value). |
| `Green` | double | `1.0` | GUI 0.02..10 | Tint. `>1` adds green, `<1` adds magenta. Used by `CustomTemp`. |
| `Equal` | double | `1.0` (key not written when 1) | GUI 0.8..1.5 | Blue/red equaliser: scales the red/blue ratio away from the strict blackbody relationship. Used with `Camera`, `Auto` and `CustomTemp`; ignored by `CustomMult`. |
| `Multipliers` | 3 doubles `R;G;B;` | `1;1;1;` | GUI 0.1..20 per channel | Camera-channel gains used only by `CustomMult`. |
**How to write it:** `Multipliers=2.1;1;1.5;` (green normally 1). Prefer
`Setting=CustomTemp` with `Temperature`/`Green`; use `Multipliers` only when
copying values from another profile.
**Interactions & pitfalls:**
* Raw input: `Camera` uses the camera's recorded multipliers; `Auto` analyses
  the image; Temperature/Green are converted to camera multipliers through the
  camera matrix, so the same Kelvin value looks different on different cameras.
  `Multipliers` (1,1,1) means no gain (unverified exact reference).
* Non-raw input (JPEG/TIFF/PNG): there are no camera multipliers; `Camera` means
  no change (unity). `CustomTemp`/`Auto`/`CustomMult` still work: ART computes
  luminance-normalised channel gains from the temperature/multipliers and applies
  them directly to the already-rendered RGB. Results are rougher than on raw
  (no matrix, data already tone-mapped). Use small adjustments only.
* To warm a raw that is at `Setting=Camera`, you must change `Setting` to
  `CustomTemp` and give a `Temperature` (the as-shot value is not known to the
  writer of the fragment). Relative "+500 K" requests need the current value;
  if unknown, use Auto or a typical value (daylight 5500..6500, shade 7500,
  tungsten 2850..3200).
* `Temperature` and `Green` saved by ART with `Camera`/`Auto` are informational;
  they are recomputed on load.
**Example:**
```ini
[White Balance]
Enabled=true
Setting=CustomTemp
Temperature=5200
Green=1.02
```
**Suggested starting values:** daylight 5500, cloudy 6500, shade 7500, flash
5500, tungsten 3000, fluorescent 4200; tint ±0.05 subtle, ±0.15 strong.

---

## Film Negative  —  `[Film Negative]`
**What it does:** Inverts a photographed colour negative into a positive. Each
channel is raised to a negative power (`value^-exponent`) and scaled so a
reference input maps to a reference output. It runs in the raw stage, on the
demosaiced white-balanced data, before the input colour profile (default,
`ColorSpace=1` works after the input profile in the working space). Wrong
exponents give a flat or crushed image with a colour cast.
**Typical requests:** "this is a negative, invert it", "convert film negative",
"fix the orange mask", "more contrast on the negative".
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `false` | | Turns the inversion on. |
| `RedRatio` | double | `1.36` | GUI 0.3..5 | Ratio of the red exponent to `GreenExponent`. |
| `GreenExponent` | double | `1.5` | GUI 0.3..4 | Reference exponent (applied to green). Raises overall contrast; raise for faint negatives, lower for contrasty ones. |
| `BlueRatio` | double | `0.86` | GUI 0.3..5 | Ratio of the blue exponent to `GreenExponent`. |
| `ColorSpace` | int | `1` | `0` = input colour space (invert before the input profile), `1` = working colour space (after it) | Space used for the inversion. |
| `RefInput` | 3 floats `R;G;B;` | `0;0;0;` | | Input RGB of the reference (film base or sampled grey). If the green value is `<= 0`, ART computes it from the image (median of the central 80%). |
| `RefOutput` | 3 floats `R;G;B;` | `0;0;0;` | | Output RGB corresponding to `RefInput`. If the green value is `<= 0`, defaults to 65535/24 for each channel (median mapped to roughly 4% of full scale), i.e. an automatic output level and balance. |
| `BackCompat` | int | `0` (not written when 0) | 0 current, 1 v1, 2 v2 | Marks profiles from older ART versions so their reference values are upgraded on load. Never write it. |
| `RedBase`, `GreenBase`, `BlueBase` | double | n/a | | Only read from old (profile version 1011..1038) files and only written when `BackCompat=2`. Do not write. |
**How to write it:** leave `RefInput`/`RefOutput` out (or `0;0;0;`) to get the
automatic estimate; when the GUI has been used it writes the sampled values:
`RefInput=2450.5;1800.2;1100.7;` and `RefOutput=2730;2730;2730;` (the output
level and Cool/Warm, Magenta/Green sliders are expressed through `RefOutput`:
scale all three for brightness, change the R:B ratio for warmth).
**Interactions & pitfalls:**
* Meant for raw files of photographed negatives (linear camera data). On a
  non-raw input the same formula runs on already gamma-encoded values and the
  result is usually wrong (unverified); the tool is not restricted to raw in the
  code.
* Auto reference estimation assumes the frame contains mostly the picture; a
  dark film holder border biases it (the central 80% is used). For consistency
  across a roll, take `RefInput` from one frame and write it to all.
* Disable or neutralise `[White Balance]` oddities: the inversion sees the
  white-balanced data, and changing WB afterwards changes the result.
* After inversion, brightness/contrast are adjusted with the normal tools.
**Example:**
```ini
[Film Negative]
Enabled=true
GreenExponent=1.5
RedRatio=1.36
BlueRatio=0.86
```
**Suggested starting values:** the defaults for an average negative;
`GreenExponent` 1.8..2.2 for flat/underexposed, 1.2..1.4 for contrasty.

---

## Color Management  —  `[Color Management]`
**What it does:** Selects the camera input profile (how raw camera RGB becomes
colour), the working colour space where all editing happens, and the output
profile/intent used on export. Changing the working or output profile changes
gamut and, for wide spaces, how saturated colours clip. It is not a creative
grading tool.
**Typical requests:** "use a wider colour space", "export as AdobeRGB/Display
P3", "use the camera's own profile", "apply the DCP look", "ProPhoto working
space".
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `InputProfile` | string | `(cameraICC)` | `(none)`, `(embedded)`, `(camera)`, `(cameraICC)`, or `file:/absolute/path/to/profile` (ICC, DCP, or OCIO file) | `(none)` = unit transform (do not use for normal editing). `(embedded)` = profile embedded in the file. `(camera)` = ART's simple standard matrix for the camera. `(cameraICC)` = auto-matched camera profile (DCP/ICC shipped with ART) when available, else standard. `file:` = a custom profile; relative paths are stored as URIs by ART but a plain absolute path after `file:` is read. |
| `ToneCurve` | bool | `false` | | Apply the DCP profile's tone curve (only when the DCP has one). |
| `ApplyLookTable` | bool | `false` | | Apply the DCP LookTable (needs a DCP with one). |
| `ApplyBaselineExposureOffset` | bool | `true` | | Apply the DCP's baseline exposure (invisible on the Exposure slider). |
| `ApplyHueSatMap` | bool | `true` | | Apply the DCP HueSatMap base table. |
| `DCPIlluminant` | int | `0` | `0` interpolated, `1` illuminant 1, `2` illuminant 2 (GUI shows them as `Interpolated`, the two temperatures) | Which DCP illuminant to use (dual-illuminant DCPs only). |
| `DCPLookEarly` | bool | `false` | | `true` = apply DCP baseline exposure/tone curve/look table early in the pipeline (before sharpening, colour correction); `false` = later, near the tone curve. |
| `WorkingProfile` | string | `Rec2020` | `sRGB`, `Adobe RGB`, `ProPhoto`, `Rec2020`, `ACESp0`, `ACESp1`, plus any custom working profile installed | Linear working space for all processing. `ProPhoto` additionally runs a blue-fix step. |
| `OutputProfile` | string | `RTv2_sRGB` | Names of the files in the output-profile directory without `.icc`: `RTv2_sRGB`, `RTv2_Medium`, `RTv4_sRGB`, `RTv4_Medium`, `RTv4_Large`, `RTv4_Rec2020`, `RTv4_DisplayP3`, `RTv4_ACES-AP0`, `RTv4_Linear_Rec709`, `RTv4_Linear_Rec2020`, `RTv4_Rec2100_PQ`; or `No ICM: sRGB output` | Colour space of exported files (embedded in the output). |
| `OutputProfileIntent` | string | `Relative` | `Perceptual`, `Relative`, `Saturation`, `Absolute` | Rendering intent for working to output conversion. Photographic use: `Relative` or `Perceptual`; do not use `Absolute`. |
| `OutputBPC` | bool | `true` | | Black point compensation on output. Keep `true`. |
| `InputProfileCAT` | bool | `false` | | Perform chromatic adaptation for the input profile (matters with some ICC/DCP profiles). |
**How to write it:** write only the keys you change, e.g. `OutputProfile=RTv4_DisplayP3`.
`InputProfile=file:/Users/me/profiles/cam.dcp`.
**Interactions & pitfalls:**
* Raw input: all `InputProfile` options apply; `DCP*`, `ApplyLookTable`,
  `ApplyHueSatMap`, `ToneCurve`, `ApplyBaselineExposureOffset` only have an effect
  with a DCP input profile (including auto-matched ones).
* Non-raw input: `(embedded)`, `(camera)`, `(cameraICC)` and empty all mean "use
  the embedded profile, or sRGB if none/not RGB"; DCP keys are irrelevant;
  `(none)` skips conversion.
* Tone Curve in this group (`ToneCurve=true`) is the DCP curve, unrelated to
  `[ToneCurve]`.
* Wide working spaces (Rec2020, ProPhoto) are fine; a narrow output space
  (sRGB) clips saturated colours on export.
**Example:**
```ini
[Color Management]
WorkingProfile=Rec2020
OutputProfile=RTv4_DisplayP3
OutputProfileIntent=Relative
OutputBPC=true
```
**Suggested starting values:** web: `RTv4_sRGB`; wide-gamut displays: `RTv4_DisplayP3`;
print/archive: `RTv4_Large` or `RTv4_Rec2020`.

---

## Dehaze  —  `[Dehaze]`
**What it does:** Removes (or adds) atmospheric haze by estimating a depth map
and a veil, using a dark-channel method, before most other tone tools (linear RGB,
stage 0). Strongly raised amounts give grey-blue skies, dark halos and
over-saturation; negative amounts add haze. By default the tool, when enabled,
already removes some haze (default curve at 0.75).
**Typical requests:** "remove haze", "clearer distant mountains", "more
atmosphere/fog", "add mist".
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `false` | | |
| `Strength` | flat curve | `1;0;0.75;0;0;1;0.75;0;0;` | flat curve, y 0..1 where `0.5` = no effect, `>0.5` removes haze, `<0.5` adds haze | Amount as a function of pixel luminance (x = luminance 0..1, gamma-encoded). The effective amount is `(y-0.5)*1.3`. A straight line at y gives a uniform effect. |
| `Depth` | int | `25` (not written when 25) | GUI 0..100 | Haze depth / how far the removal reaches; larger values clear more distant, hazier areas. |
| `Luminance` | bool | `false` | `false` = RGB mode, `true` = luminance mode | Luminance mode removes haze from luminance only and adds less saturation; RGB mode also boosts colour. |
| `Blackpoint` | int | `0` | GUI 0..100 | Subtracts a black point before dehazing (deepens blacks). |
| `ShowDepthMap` | bool | `false` (not written when false) | | Debug view of the depth map. Do not write. |
**How to write it:** the curve is a flat curve (see "Curves"): a uniform
amount `a` (0..1, 0.5 neutral) is
`Strength=1;0;a;0;0;1;a;0;0;`. For `a=0.75` this is the default, for `a=0.9`
strong removal, for `a=0.35` adds haze.
**Interactions & pitfalls:**
* Needs the whole image; with a crop active ART processes the full image first.
* Strong values plus the tone curve and Saturation quickly give an unnatural,
  over-processed look; lower `Saturation`/vibrance if colours look harsh.
* Dehaze and Dynamic Range Compression both run at the very start and interact.
**Example:**
```ini
[Dehaze]
Enabled=true
Strength=1;0;0.8;0;0;1;0.8;0;0;
Depth=40
Luminance=false
```
**Suggested starting values:** subtle `a=0.6`, moderate `0.75`, strong `0.9`.

---

## Dynamic Range Compression  —  `[DynamicRangeCompression]`
**What it does:** Compresses the contrast between very bright and very dark
regions (Fattal-style tone mapping) while keeping local detail, at the start
of the linear-RGB chain. Useful for high-contrast scenes (backlit, windows);
too much produces a flat, HDR-ish look with halos.
**Typical requests:** "reduce the contrast between sky and ground", "recover
shadows and highlights", "HDR look", "tame a high-contrast scene".
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `false` | | |
| `Amount` | int | `20` | GUI 1..100 | Compression strength. |
| `Threshold` | int | `30` | GUI -100..300 | Shown as "Detail" in the GUI: how much local contrast is preserved (positive = more detail/less compression, negative = flatter). |
| `SaturationControl` | bool | `false` | | Limits the saturation increase in compressed highlights. |
**Interactions & pitfalls:**
* In files written by ART versions before profile 1046 the group was called
  `[FattalToneMapping]`; write `[DynamicRangeCompression]`.
* Overlaps with Tone Equalizer, Log Encoding and Dehaze; do not stack strong
  settings of several.
**Example:**
```ini
[DynamicRangeCompression]
Enabled=true
Amount=30
Threshold=50
SaturationControl=true
```
**Suggested starting values:** subtle `Amount=15`, moderate `30`, strong `60`;
`Threshold` 30..80.

---

## Channel Mixer  —  `[Channel Mixer]`
**What it does:** Builds each output channel from percentages of the input R, G
and B (a 3x3 matrix), or alternatively shifts the hue and saturation of the
three primaries. Runs in linear RGB after Dehaze/DRC and before Exposure.
Extreme values give colour posterisation and clipping; keep rows summing to
about 100% to preserve brightness.
**Typical requests:** "swap red and blue", "make greens more yellow", "teal and
orange look", "shift the red primary", "desaturate blues only" (use HSL for that).
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `false` | | |
| `Mode` | int | `0` | `0` = RGB matrix, `1` = primaries correction | Which controls are used. |
| `Red` | 3 ints `R;G;B;` | `1000;0;0;` | GUI -500..500 percent (value = percent x 10, so -5000..5000) | Output red = R*a + G*b + B*c with `a;b;c` in tenths of a percent (`1000` = 100%). Used when `Mode=0`. |
| `Green` | 3 ints | `0;1000;0;` | same | Output green mix. |
| `Blue` | 3 ints | `0;0;1000;` | same | Output blue mix. |
| `HueTweak` | 3 ints `R;G;B;` | `0;0;0;` | GUI -100..100 | Hue shift of the red, green, blue primaries (`Mode=1`). Per GUI icons: red positive moves towards orange, negative towards purple/pink; green positive towards cyan, negative towards yellow; blue positive towards purple, negative towards cyan. |
| `SatTweak` | 3 ints `R;G;B;` | `0;0;0;` | GUI -100..100 | Saturation change of each primary (`Mode=1`). |
**How to write it:** the three rows are written as three separate keys, always
all of `Red`, `Green` and `Blue` together (the loader ignores the matrix unless
all three keys are present with 3 values each). Example swapping red and blue:
`Red=0;0;1000;`, `Green=0;1000;0;`, `Blue=1000;0;0;`. Remember values are x10:
50% green into red is `500`.
**Interactions & pitfalls:**
* In `Mode=1` the `Red/Green/Blue` rows are ignored (recomputed from the
  tweaks), and in `Mode=0` the tweaks are ignored.
* Output channels are clipped at 0 (negative results become 0).
* The matrix acts on the working space, before Exposure: it can change overall
  brightness if the row sums are not 1000.
**Example:**
```ini
[Channel Mixer]
Enabled=true
Mode=0
Red=900;100;0;
Green=0;1000;0;
Blue=0;0;1000;
```
**Suggested starting values:** off-diagonal terms 50..150 (5%..15%) subtle,
300 moderate; primaries mode: hue/sat 10..20 subtle, 40..60 strong.

---

## Exposure  —  `[Exposure]`
**What it does:** Global exposure compensation in stops (linear gain 2^value,
no clipping), a black-point shift, and highlight reconstruction for clipped raw
data. Compensation and black run in linear RGB after the Channel Mixer;
highlight reconstruction runs on the raw data during decoding. Too much positive
compensation clips highlights and amplifies noise; too much negative lifts
nothing and looks muddy.
**Typical requests:** "brighter", "darker", "+1 stop", "recover blown highlights",
"deeper blacks", "lift shadows" (see Tone Equalizer/Curve for shadows only).
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `true` | | `false` disables compensation and black point (highlight reconstruction is separate, see pitfalls). |
| `Compensation` | double (stops, Ev) | `0` | GUI -12..12 | Multiply linear values by 2^Compensation. +1 = twice as bright. |
| `Black` | double | `0` | GUI -2..2 | Black point compensation: positive raises the black level (darkens shadows), negative lowers it (lightens). Linear units; the scale is not stated in code (unverified). Small values (0.01..0.1) are already visible. |
| `HLRecovery` | string | `Off` | `Off`, `Blend`, `Color`, `Balanced` | Highlight reconstruction method for clipped raw channels. `Color` is the strongest, `Balanced` less aggressive, `Blend` simplest. |
| `HLRecoveryBlur` | int | `0` | GUI 0..3 | Smoothness of colour propagation (relevant to `Color`/`Balanced`). |
**Interactions & pitfalls:**
* Highlight reconstruction only has an effect on raw files; on JPEG/TIFF it does
  nothing (clipped pixels cannot be rebuilt).
* `Compensation` happens before the tone curve and after the white balance, so
  negative values bring back highlight detail only if the raw is not clipped.
* Old profile keys `HLRecoveryEnabled`/`HLRecoveryMethod` are only read for
  profile versions below 1000; use `HLRecovery`.
**Example:**
```ini
[Exposure]
Enabled=true
Compensation=0.7
HLRecovery=Color
```
**Suggested starting values:** subtle ±0.3, moderate ±1, strong ±2 Ev; `Black`
0.02 subtle.

---

## HSL Equalizer (Color Equalizer)  —  `[HSL Equalizer]`
**What it does:** Selectively changes hue, saturation or luminance of colours
depending on their hue, with curves over the hue circle. Runs in linear RGB
(YUV hue/chroma) after Exposure. Pushing narrow nodes hard produces banding
and edge artefacts; use `Smoothing` to soften.
**Typical requests:** "make the sky bluer", "more saturated greens", "shift
oranges toward yellow", "darker blue sky", "desaturate the reds".
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `false` | | |
| `HCurve` | flat curve (periodic) | `0;` (identity) | y 0..1, `0.5` = no change | Hue shift: x = source hue, y above 0.5 shifts hue up (toward higher hue index), below shifts down. Maximum shift is about +-pi at the extremes (non-linear). |
| `SCurve` | flat curve (periodic) | `0;` | y 0..1, `0.5` neutral | Saturation: x = hue, y above 0.5 increases chroma, below decreases (0 = fully desaturated). |
| `LCurve` | flat curve (periodic) | `0;` | y 0..1, `0.5` neutral | Luminance: x = hue, y above 0.5 brightens those hues, below darkens. |
| `Smoothing` | int | `0` | GUI 0..10 | Blurs the edges of the affected areas (guided filter). |
**How to write it:** see "Curves" (flat curves). x is the hue angle of the
working-space YUV hue divided by 2 pi, so it wraps around. Approximate hue
positions (Rec2020 working space; sRGB is within about +-0.02): red 0.05,
yellow 0.24, green 0.37, cyan 0.55, blue 0.74, magenta 0.88. Because the curve
interpolates between nodes, place a node at the target hue and neutral
(y=0.5) nodes on both sides. Example boosting blue and cyan saturation:
`SCurve=1;0;0.5;0.35;0.35;0.2;0.5;0.35;0.35;0.4;0.5;0.35;0.35;0.55;0.65;0.35;0.35;0.74;0.7;0.35;0.35;0.9;0.5;0.35;0.35;`
**Interactions & pitfalls:**
* Overlaps with `[Saturation]` (global), `[Channel Mixer]` primaries mode and
  the per-region Color/Tone Correction (not in this file).
* The hue positions above were derived from the engine's hue definition, not
  from the GUI strip; the GUI colour strip is shifted by about 1/19 relative to
  it (empirical, unverified).
**Example:**
```ini
[HSL Equalizer]
Enabled=true
SCurve=1;0;0.5;0.35;0.35;0.2;0.5;0.35;0.35;0.4;0.5;0.35;0.35;0.55;0.65;0.35;0.35;0.74;0.7;0.35;0.35;0.9;0.5;0.35;0.35;
Smoothing=2
```
**Suggested starting values:** node y 0.55..0.6 subtle, 0.65..0.75 moderate,
>0.85 strong; for darkening a sky use `LCurve` y 0.3..0.4 at the blue node.

---

## Tone Equalizer  —  `[ToneEqualizer]`
**What it does:** Lifts or lowers five luminance zones (blacks, shadows,
midtones, highlights, whites) while preserving local contrast (an edge-aware
parametric tone mapper). Runs in linear RGB after the HSL Equalizer. Large
values with low regularisation give halos; the zones are defined relative to
`Pivot`.
**Typical requests:** "lift the shadows", "recover highlights", "darken the
sky/highlights only", "open up shadows without making it flat".
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `false` | | |
| `Band0` | int | `0` | GUI -100..100 | Blacks. Positive = brighter. |
| `Band1` | int | `0` | -100..100 | Shadows. |
| `Band2` | int | `0` | -100..100 | Midtones. |
| `Band3` | int | `0` | -100..100 | Highlights. |
| `Band4` | int | `0` | -100..100 | Whites. |
| `Regularization` | int | `4` | GUI 0..4 | Smoothing of the zone transitions and of the luminance mask. Higher = smoother, fewer halos but less local selectivity. |
| `Pivot` | double (Ev) | `0` | GUI -12..12 | Shifts the exposure of the midtone centre: positive moves the zone positions so more of the image counts as shadows. |
**Interactions & pitfalls:**
* The "show tonal map" overlay is a GUI-only flag and is not saved.
* `Detail` (profile versions < 1002) is only a legacy name for `Regularization`.
* Compared with `[Exposure]` it does not move the whole histogram; compared with
  `[ToneCurve]` it preserves local contrast.
**Example:**
```ini
[ToneEqualizer]
Enabled=true
Band0=0
Band1=35
Band2=10
Band3=-25
Band4=-10
Regularization=4
```
**Suggested starting values:** bands +-20 subtle, +-40 moderate, +-70 strong.

---

## Film Grain  —  `[Grain]`
**What it does:** Adds film-like grain (noise built from smoothing-noise layers)
late in the linear pipeline. Larger ISO = coarser grain. Too much looks like
digital noise.
**Typical requests:** "add film grain", "gritty look", "35mm film feel",
"finer grain".
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `false` | | |
| `ISO` | int | `400` | GUI 100..6400 (step 50) | Coarseness (grain size). The profile loader scales old (pre-1047) values by 1.5, new files are used as is. |
| `Strength` | int | `50` | GUI 0..100 | Amount of grain. (The GUI slider's reset value is 25; the profile default is 50.) |
| `Color` | bool | `false` | | `false` = luminance (B&W) grain only; `true` = also colour grain. |
| `Scale` | int | n/a | | Legacy, read-only: if present the loader multiplies `Strength` by `Scale`/100. Do not write. |
**Interactions & pitfalls:**
* Noise is added before Log Encoding, Saturation, Tone Curve, so tone curves
  and Black & White act on top of the grain.
* Combine with `[Black & White]` for monochrome film looks; for colour film
  use `Color=true`.
**Example:**
```ini
[Grain]
Enabled=true
ISO=800
Strength=35
Color=false
```
**Suggested starting values:** subtle `ISO=400, Strength=20`; moderate `ISO=800,
Strength=40`; strong `ISO=3200, Strength=70`.

---

## Log Tone Mapping  —  `[LogEncoding]`
**What it does:** Log-encodes the scene (black/white relative exposure in
stops around mid grey), then maps to display with an optional local-contrast
preserving regularisation, to compress dynamic range (pull highlights, push
shadows). Runs in linear RGB after Grain, before Saturation and the tone
curve. Strong compression gives a flat, low-contrast image that needs a
contrast curve afterwards.
**Typical requests:** "compress the dynamic range", "recover the sky and
shadows", "flat log look", "auto tone map".
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `false` | | |
| `Auto` | bool | `false` | | `true` = ART computes `BlackEv`/`WhiteEv` from the image (the stored values are then ignored). |
| `AutoGain` | bool | `false` | | `true` = ART computes `Gain` automatically. |
| `Gain` | double (Ev) | `0.0` | GUI -10..10 | Exposure applied before the mapping (ignored when `AutoGain=true`). |
| `TargetGray` | double | `18.0` | GUI 5..80 | Output brightness (percent) that input mid grey (0.18) is mapped to; raises/lowers overall brightness. |
| `BlackEv` | double (Ev below grey) | `-13.5` | GUI -16..0 | Input black point; higher (closer to 0) = more clipping of shadows. |
| `WhiteEv` | double (Ev above grey) | `2.5` | GUI 1..32 | Input white point mapped to 1.0; larger = more highlight compression. |
| `Regularization` | int | `60` | GUI 0..100 (GUI reset value is 65) | Local-contrast preservation; high = better detail, risk of halos. |
| `SaturationControl` | bool | `true` | | Limits saturation boost in highlights. |
| `HighlightCompression` | int | `0` | GUI 0..100 | Compresses the highlight end before the mapping. |
**Interactions & pitfalls:**
* `Auto=true` ignores `BlackEv`/`WhiteEv`; set `Auto=false` to control them.
* The `[ToneCurve]` contrast pivot and base curve use `TargetGray`/100 as their
  grey point when log encoding is enabled.
* Legacy keys `GrayPoint`, `SourceGray`, `AutoGray` appear only in files with
  profile version < 1024; do not write.
* Do not combine strong settings with Dynamic Range Compression/Tone Equalizer.
**Example:**
```ini
[LogEncoding]
Enabled=true
Auto=true
AutoGain=true
TargetGray=18
Regularization=60
```
**Suggested starting values:** `Auto=true, AutoGain=true` first; manual
`WhiteEv` 3..5 with `BlackEv` -9..-12 for moderate compression.

---

## Saturation / Vibrance  —  `[Saturation]`
**What it does:** Global colour intensity: Saturation scales all colours,
Vibrance mostly boosts weakly saturated colours (protects skin and already
vivid colours). Runs in linear RGB after Log Encoding and before the tone curve.
Too much produces neon, clipped colours.
**Typical requests:** "more colourful", "punchier colours", "desaturate",
"muted look", "subtle vibrance".
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `false` | | |
| `Saturation` | int | `0` | GUI -100..100 | `-100` = greyscale, positive = more saturated. |
| `Vibrance` | int | `0` | GUI -100..100 | Positive boosts muted colours more than vivid ones. |
**Interactions & pitfalls:**
* `[ToneCurve]` also has `Saturation`/`Saturation2` keys; they are different
  (curve-based) and in a different group.
* The tone curve afterwards may change perceived saturation depending on the
  curve mode (Standard S curves add saturation; Perceptual/Luminance keep it).
**Example:**
```ini
[Saturation]
Enabled=true
Saturation=10
Vibrance=25
```
**Suggested starting values:** subtle Vibrance 15, moderate 30 with Saturation
10, strong Vibrance 50 / Saturation 25.

---

## Film Simulation  —  `[Film Simulation]`
**What it does:** Applies a 3D LUT (Hald CLUT, CLF, 3dl/cube via OCIO or CTL
script) to emulate a film stock or look, with an opacity slider. By default
(`AfterToneCurve=false`) it runs before the tone curve.
**Typical requests:** "Kodak Portra look", "apply this film emulation/LUT",
"cinematic LUT at half strength".
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `false` | | |
| `ClutFilename` | string | empty | absolute path, or path relative to the user's LUT directory (Preferences) with subfolders allowed | The LUT/CTL file. ART saves it as a URI-like path; a plain path is read. A missing/unknown file makes the tool do nothing. |
| `Strength` | int | `100` | GUI 0..100 | Opacity of the LUT. |
| `AfterToneCurve` | bool | `false` (key written only when `true`) | | `true` = apply after `[ToneCurve]` (for LUTs expecting display-referred input). |
| `ClutParams` | list of `name=v1,v2,...` strings | empty | `;`-separated entries | Parameter values for parametric CTL scripts, e.g. `ClutParams=exposure=0.5;tint=0.1,0.2;`. Entry names depend on the script. |
**How to write it:** the writer cannot know which LUTs exist; use only a LUT
file name the user supplied or that is listed by the user's setup. Values in
`ClutParams` use `.` decimals and `,` between the components of one parameter
(components of one parameter are a comma list; entries are `;` separated).
**Interactions & pitfalls:**
* Hald CLUTs expect gamma-encoded (typically 2.2) bounded input, CLF/CTL work on
  linear unbounded data; ART converts appropriately.
* Stacks with `[ToneCurve]` and `[RGB Curves]`: avoid doubling contrast.
**Example:**
```ini
[Film Simulation]
Enabled=true
ClutFilename=Kodak/Portra400.clf
Strength=70
```
**Suggested starting values:** Strength 50..70 to blend a strong look.

---

## Tone Curve  —  `[ToneCurve]`
**What it does:** The main contrast/tone tool: two cascaded tone curves with
selectable colour handling, a contrast slider, optional white point, base curve
(highlight roll-off or smooth S), and curve-based saturation. Runs in linear RGB
after Saturation and Film Simulation (unless after). Too steep S curves clip and
over-saturate (depending on `CurveMode`).
**Typical requests:** "more contrast", "S curve", "matte/faded blacks", "film
look", "brighten midtones", "lower contrast", "punchier".
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `false` | | |
| `Contrast` | int | `0` | GUI -100..100 | Contrast slider (pivot at mid grey, 0.18 or `[LogEncoding]` TargetGray/100). |
| `CurveMode` | string | `Neutral` | `Neutral`, `Standard`, `WeightedStd`, `FilmLike`, `SatAndValueBlending`, `Luminance`, `Perceptual` (`OpenDisplayTransform` is an accepted alias of `Neutral`) | How curve 1 is applied to colour. `Neutral`: same curve on all channels plus gamut compression and highlight desaturation. `Standard`: per-channel (S curves raise saturation). `WeightedStd`: less colour shift. `FilmLike`: Adobe-style, constant hue. `SatAndValueBlending`: like a Lab luminance curve. `Luminance`: colour kept, luminance changed. `Perceptual`: constant colour appearance (slow). |
| `CurveMode2` | string | same as `CurveMode` | same values | Mode for curve 2. Only written when curve 2 is non-linear and the modes differ. When `CurveMode` is read, `CurveMode2` is set to the same value first, so write `CurveMode2` after it only when you want them different. |
| `Curve` | diagonal curve | `0;` | see "Curves" | Tone curve 1. |
| `Curve2` | diagonal curve | `0;` | see "Curves" | Tone curve 2, applied in series with curve 1 (typical use: curve 1 lowers, curve 2 raises). |
| `HistogramMatching` | bool | `false` | | "Auto-matched tone curve": curves/sliders are computed to match the embedded JPEG look (raw only; the curves are overwritten). |
| `CurveFromHistogramMatching` | bool | `false` | | Marks that `Curve` came from histogram matching; if `true` on load, `HistogramMatching` is forced `false`. Do not write. |
| `Saturation` | flat curve | `0;` | see "Curves" | Saturation vs luminance: x = luminance, y `0.5` = unchanged, `1` = double chroma, `0` = none. |
| `Saturation2` | diagonal curve | `0;` | see "Curves" | Saturation vs chroma: maps input chroma to output chroma (x/y 0..1 of the chroma range). |
| `PerceptualStrength` | int | `100` (written only when != 100) | GUI 0..100 | Strength of curves in `Perceptual` mode. |
| `ContrastLegacyMode` | bool | `false` (written only when `true`) | | Old contrast algorithm; deprecated, do not write. |
| `WhitePoint` | double | `1.0` | GUI 1..100 | Scene-referred white point (values above 1 let highlights above 1.0 be compressed); ignored (treated as 1) if legacy contrast is on or curve modes `SatAndValueBlending`/`Perceptual` use a non-linear curve. |
| `BaseCurve` | string | `Linear` | `Linear`, `Rolloff`, `SCurve` | Fixed base mapping applied first: `Rolloff` = smooth highlight roll-off, `SCurve` = smooth S shape. Not applied together with `Neutral` mode and a single curve (Neutral does its own). |
**How to write it:** curves are diagonal curves, e.g. a moderate S:
`Curve=4;0;0;0.25;0.2;0.75;0.8;1;1;` (type 4 Flexible). Matte look (raised
blacks): `Curve=4;0;0.05;0.5;0.5;1;0.97;`. Always leave the end points in the
list. A flat saturation curve boosting midtone colour: `Saturation=1;0;0.5;0.35;0.35;0.5;0.65;0.35;0.35;1;0.5;0.35;0.35;`.
**Interactions & pitfalls:**
* `Contrast` and `Curve` both add contrast; use one mainly.
* Curve mode determines whether S curves change saturation; for a pure tonal
  curve prefer `Neutral`/`Luminance`/`Perceptual`.
* `[Saturation]`, `[LogEncoding]` and `[Film Simulation]` run before this tool
  and change what the curve sees.
* Histogram matching needs a raw with an embedded preview; on other files it has
  no effect (unverified).
**Example:**
```ini
[ToneCurve]
Enabled=true
CurveMode=Neutral
Contrast=15
Curve=4;0;0;0.25;0.2;0.75;0.8;1;1;
```
**Suggested starting values:** Contrast +10 subtle, +25 moderate, +50 strong;
S curve handle offset 0.03, 0.06, 0.1 from the diagonal.

---

## RGB Curves  —  `[RGB Curves]`
**What it does:** Applies an independent diagonal tone curve to each of the red,
green and blue channels (in the working space, after the Tone Curve). Used for
colour grading (split toning of shadows/highlights, cross-process looks) and
cast correction in specific tonal ranges. Strong differences between the
channels give strong colour casts.
**Typical requests:** "warm highlights, cool shadows", "teal shadows", "remove
the green cast in the shadows", "cross-processed look".
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `false` | | |
| `rCurve` | diagonal curve | `0;` | see "Curves" | Red channel: y above x makes the channel stronger at that tone (adds red), below removes red (adds cyan). |
| `gCurve` | diagonal curve | `0;` | | Green channel (below = adds magenta). |
| `bCurve` | diagonal curve | `0;` | | Blue channel (below = adds yellow). |
**How to write it:** case-sensitive lowercase key names. Example, warm
highlights and cool shadows:
`rCurve=4;0;0;0.25;0.22;0.75;0.8;1;1;`, `bCurve=4;0;0;0.25;0.28;0.75;0.72;1;1;`.
Keep the curves' end points `0;0;` and `1;1;`.
**Interactions & pitfalls:**
* Moving all three channels equally is a luminance change (use Tone Curve).
* Differences > 0.05 between channels already tint noticeably.
**Example:**
```ini
[RGB Curves]
Enabled=true
rCurve=4;0;0;0.25;0.22;0.75;0.8;1;1;
bCurve=4;0;0;0.25;0.28;0.75;0.72;1;1;
```
**Suggested starting values:** channel offsets of 0.02 subtle, 0.04 moderate,
0.08 strong at the quarter points.

---

## L\*a\*b\* Adjustments  —  `[Luminance Curve]`
**What it does:** Adjusts lightness, contrast and chroma in CIE L\*a\*b\*, with
optional curves on L, a and b. Runs after RGB Curves. Separating luminance from
colour avoids hue shifts. `Chromaticity=-100` removes all colour, but Black &
White is the recommended way to go monochrome.
**Typical requests:** "lift lightness without changing colour", "more chroma",
"boost greens/magentas with a\* curve", "warm/cool with b\*".
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `false` | | |
| `Brightness` | int | `0` | GUI -100..100 | Lightness curve on L (black and white points stay). |
| `Contrast` | int | `0` | GUI -100..100 | Contrast on L. |
| `Chromaticity` | int | `0` | GUI -100..100 | Chroma scaling of a and b; -100 = no colour. |
| `LCurve` | diagonal curve | `0;` | | L\* = f(L\*). |
| `aCurve` | diagonal curve | `0;` | | a\* (green-magenta) curve; curve above the diagonal = more magenta. GUI labels: low = green saturated, high = red saturated. |
| `bCurve` | diagonal curve | `0;` | | b\* (blue-yellow) curve; above = more yellow. |
**How to write it:** key names are `LCurve`, `aCurve`, `bCurve` (lowercase a/b).
**Interactions & pitfalls:**
* `[HSL Equalizer]` and `[Saturation]` do similar jobs earlier in the pipeline.
* a/b curves shift colour globally (all hues); for targeted colour changes use
  HSL Equalizer.
**Example:**
```ini
[Luminance Curve]
Enabled=true
Brightness=5
Contrast=10
Chromaticity=15
```
**Suggested starting values:** Chromaticity +10 subtle, +25 moderate; Contrast +10.

---

## Soft Light  —  `[SoftLight]`
**What it does:** Blends the image with a copy of itself in "soft light" mode
(like the GIMP layer mode), adding contrast and saturation with a pleasant
roll-off. Runs in gamma-2.2 bounded RGB near the end (after Luminance Curve).
**Typical requests:** "a bit of glow/punch", "soft contrast boost", "dreamy
contrast".
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `false` | | |
| `Strength` | int | `30` | GUI 0..100 | Opacity of the soft light layer. |
**Example:**
```ini
[SoftLight]
Enabled=true
Strength=40
```
**Suggested starting values:** subtle 20, moderate 40, strong 70.

---

## Black & White  —  `[Black & White]`
**What it does:** Converts to monochrome by mixing the R, G, B channels
(like panchromatic film plus optional lens colour filter), per-channel gamma
and an optional colour tint. It runs last of the colour tools, after Soft Light
and local contrast, in gamma-2.2 RGB, and replaces all colour (so colour
adjustments before it only change tonality through the mixer). The group name
contains `&` and spaces: write exactly `[Black & White]`.
**Typical requests:** "black and white", "dramatic B&W with dark skies", "red
filter", "infrared look", "sepia/blue tone".
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `false` | | |
| `Setting` | string | `RGB-Rel` | `NormalContrast`, `HighContrast`, `Luminance`, `Landscape`, `Portrait`, `LowSensitivity`, `HighSensitivity`, `Panchromatic`, `HyperPanchromatic`, `Orthochromatic`, `RGB-Abs`, `RGB-Rel`, `InfraRed` | Mixer preset. All presets except `RGB-Abs`/`RGB-Rel` ignore `MixerRed/Green/Blue` and use their own weights (Normal 43/33/30, Panchromatic 33/33/33, Landscape 66/24/10, Portrait 54/44/12, Luminance 30/59/11, Orthochromatic 0/42/58, InfraRed -40/200/-17, etc., normalised to a sum of 100). `RGB-Rel` uses the mixer values normalised to their sum; `RGB-Abs` uses them as given (total brightness changes). |
| `Filter` | string | `None` | `None`, `Red`, `Orange`, `Yellow`, `YellowGreen`, `Green`, `Cyan`, `Blue`, `Purple` | Simulated colour filter: lightens its own colour, darkens complementary colours (`Red`/`Orange` darken blue skies). |
| `MixerRed` | int | `33` | GUI -100..200 | Red weight (percent). Used by `RGB-Abs`/`RGB-Rel`. |
| `MixerGreen` | int | `33` | -100..200 | Green weight. |
| `MixerBlue` | int | `33` | -100..200 | Blue weight. |
| `GammaRed` | int | `0` | GUI -100..100 | Per-channel gamma (mid-grey point) of the red channel. |
| `GammaGreen` | int | `0` | -100..100 | |
| `GammaBlue` | int | `0` | -100..100 | |
| `ColorCast` | 2 ints `saturation;hue;` | `0;0;` | saturation 0..100, hue 0..360 (degrees) | Tint: saturation (0 = no tint, the effect is nonlinear, `(s/100)^3`) and hue angle (0..360) of the tint colour. Hue positions follow the GUI spectrum slider (unverified colour for each angle: try values and view). |
**How to write it:** a plain red-filter dark-sky conversion:
`Setting=Panchromatic`, `Filter=Red`. A custom mix: `Setting=RGB-Rel`,
`MixerRed=60`, `MixerGreen=40`, `MixerBlue=0`.
**Interactions & pitfalls:**
* The sliders show nothing when a preset other than `RGB-*` is chosen: edit the
  mixer only with `RGB-Rel`/`RGB-Abs`, or the values are ignored.
* Chromaticity -100 (Luminance Curve) is an alternative but less controlled.
* Grain is applied before B&W, so it stays in the result.
**Example:**
```ini
[Black & White]
Enabled=true
Setting=RGB-Rel
Filter=Red
MixerRed=40
MixerGreen=40
MixerBlue=20
```
**Suggested starting values:** subtle contrast `Panchromatic`; dramatic skies
`Filter=Red` or `Orange`; portraits `Portrait`.
