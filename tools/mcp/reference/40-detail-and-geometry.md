# Detail, lens and geometry groups

Groups covered: `[Sharpening]`, `[Impulse Denoising]`, `[Defringing]`, `[Denoise]`,
`[CACorrection]`, `[Vignetting Correction]`, `[Crop]`, `[Coarse Transformation]`,
`[Common Properties for Transformations]`, `[Rotation]`, `[Distortion]`,
`[LensProfile]`, `[Perspective]`, `[Resize]`, `[OutputSharpening]`, `[MetaData]`.

General notes for all groups
* Booleans are `true`/`false`. Decimal point is `.`. Names are case-sensitive.
* A partial profile only changes the groups/keys it contains; omitted keys keep their
  current value. Within a group, keys that are present override, others stay.
* Almost every group has `Enabled`. With `Enabled=false` the other keys are stored
  but have no effect. Exceptions: `[LensProfile]` (no `Enabled`; it is on when
  `LcMode` is not `none`), `[Coarse Transformation]` and
  `[Common Properties for Transformations]` (no `Enabled`).
* Written ranges are GUI ranges. The loader does not clamp most values (soft ranges),
  so stay inside them; out-of-range values can give garbage output.

## Order of operations

Source: `src/engine/simpleprocess.cc`, `improcfun.cc` (`process()` stages), `iptransform.cc`;
`Pipeline.md` on the docs site is a simplified list (disagreements are marked below).

1. Raw stage: black points, raw CA, demosaic, etc. (other reference files).
   `[Coarse Transformation]` (90 degree rotation and flips) is applied here, as the
   image is read: every later coordinate is in the rotated/flipped frame.
2. White balance, spot removal, input profile conversion to the linear working space.
3. `[Denoise]` (linear RGB, on the whole frame, before any geometry change).
4. Stage 0: dehaze, dynamic range compression (other files).
5. Geometry/lens "transform" in one resampling step: `[LensProfile]`, `[Rotation]`,
   `[Perspective]`, `[Distortion]`, `[CACorrection]`, `[Vignetting Correction]`
   (all after demosaic, in the same pass; `AutoFill` from
   `[Common Properties for Transformations]` scales the result). The output frame has
   the same pixel size as the input frame.
   (Disagreement: `Pipeline.md` lists "Vignetting correction" as a RAW-stage module;
   the code applies `[Vignetting Correction]` inside this transform step.)
6. `[Crop]` is cut out of the transformed frame.
   (Disagreement: `Pipeline.md` lists Crop near the end; the code crops here, so all
   later tools, e.g. sharpening and graduated/vignette filters, work on the cropped image only.)
7. Stage 1: channel mixer, exposure, HSL, tone equalizer (other files).
8. Stage 2: `[Sharpening]` -> `[Impulse Denoising]` -> `[Defringing]` (in this order),
   then colour correction, smoothing (other files). Sharpening therefore sees the
   cropped, geometry-corrected, exposure-adjusted image but not tone curves/saturation.
9. Stage 3: texture boost, grain, saturation, film simulation, tone curve, RGB curves,
   Lab, local contrast, B&W (other files).
10. `[Resize]` (export only), then `[OutputSharpening]`, output colour profile.
11. `[MetaData]` is applied when the file is written.

## Sharpening  —  `[Sharpening]`
**What it does:** "Capture sharpening" of the detail tab, applied to luminance only,
after exposure/tone-equaliser work and before tone curves (see order above). It is limited
by a contrast mask (`Contrast`) so that flat/noisy areas are left alone. Three methods:
`usm` (unsharp mask), `rld` (Richardson-Lucy deconvolution, default, best for
recovering lens/AA blur) and `psf` (RL deconvolution with a user-supplied PSF kernel
image). Pushed too far: halos around edges, crunchy noise, ringing near clipped highlights.
**Typical requests:** "sharpen", "make it crisper", "more detail", "soften less",
"sharpen for web" (that one is `[OutputSharpening]`).
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `false` | | Master switch. |
| `Method` | string | `rld` | `usm`, `rld`, `psf` | Selects which of the keys below are used. Anything else is treated as `usm` by the engine. |
| `Contrast` | double | `20` | 0..200 | "Contrast threshold": detail with lower local contrast than this is not sharpened (blend mask = (Contrast/100)^1.2). Used by ALL methods. Raise for noisy/high-ISO images. |
| `Radius` | double | `0.5` | 0.3..3 | `usm` only. Blur radius of the mask, pixels. 0.5-0.7 for in-focus low ISO. |
| `Amount` | int | `200` | 1..1000 | `usm` only. Strength. Note: the engine skips sharpening entirely if `Amount` < 1, whatever the method. |
| `Threshold` | int list (4) | `20;80;2000;1200` | each 0..2000 | `usm` only. Luminance-opacity curve: bottom-left, top-left, bottom-right, top-right handle positions. Left pair = how fast sharpening fades in from the shadows, right pair = fades out toward highlights. Exact semantics unverified; leave at default unless asked. Must contain 4 values or it is ignored. |
| `OnlyEdges` | bool | `false` | | `usm` only. Sharpen only detected edges (useful on noisy files). |
| `EdgedetectionRadius` | double | `1.9` | 0.5..2.5 | `usm` + `OnlyEdges` only. Radius of the edge/noise detector (bigger = slower). |
| `EdgeTolerance` | int | `1800` | 10..10000 | `usm` + `OnlyEdges` only. Difference needed to count as an edge. ~1000 or less at low ISO, 2500-3000+ at high ISO. |
| `HalocontrolEnabled` | bool | `false` | | `usm` only. Enable halo suppression. |
| `HalocontrolAmount` | int | `85` | 1..100 | `usm` + halo control only. 100 = maximum. (GUI slider starts at 75, the profile default is 85.) |
| `DeconvRadius` | double | `0.75` | 0.4..2.5 | `rld` only. Gaussian sigma of the blur to undo. Ignored while `DeconvAutoRadius=true` (the radius is then taken from the raw data when available). Too small = weak, too large = halos. |
| `DeconvAmount` | int | `100` | 0..100 | `rld` only. Blend between unsharpened and sharpened. |
| `DeconvAutoRadius` | bool | `true` | | `rld` only. Estimate the radius automatically. Set `false` to make `DeconvRadius` effective. |
| `DeconvCornerBoost` | double | `0.0` | 0..1 | `rld` only. Extra radius toward the corners (softer corners). 0 = off. |
| `DeconvCornerLatitude` | int | `25` | 0..100 | `rld` only. Size of the corner region; no effect while `DeconvCornerBoost` is 0. |
| `PSFKernel` | string (file path) | empty | path to a kernel image | `psf` only. Without a kernel `psf` is useless; use `rld` instead. |
| `PSFIterations` | double | `10` | 1..100 | `psf` only. RL iterations. |
**How to write it:** `Threshold=20;80;2000;1200;` (semicolon list, trailing `;` optional).
Writing `Method` with other method's keys is harmless: unused keys are ignored.
**Interactions & pitfalls:**
* For `rld` the iteration count is automatic. Good white-level in the raw is important.
* `Method=rld` ignores `Amount`/`Radius`/`Threshold`; `Method=usm` ignores all `Deconv*`.
* Sharpen after denoise: strong sharpening re-amplifies residual noise; raise `Contrast`.
* Do not confuse with `[OutputSharpening]` (after resize, for export size).
**Example:**
```ini
[Sharpening]
Enabled=true
Method=rld
Contrast=20
DeconvAutoRadius=true
DeconvAmount=100
```
**Suggested starting values** (heuristic, not from code):
* Base ISO (<=400), `rld`: defaults (Contrast 20, DeconvAmount 100, auto radius).
* ISO 800-3200: `rld` with `Contrast=30`, `DeconvAmount=70`.
* ISO >=6400: `usm`, `Radius=0.6`, `Amount=100`, `OnlyEdges=true`, `EdgeTolerance=3000`, `Contrast=40`; or skip.
* "Subtle" `DeconvAmount=50`; "strong" `usm` `Amount=300`, `Radius=0.8` with `HalocontrolEnabled=true`.

## Impulse Denoising  —  `[Impulse Denoising]`
**What it does:** removes salt-and-pepper (isolated bright/dark) pixels after demosaic,
in stage 2 right after sharpening. Not for hot/dead sensor pixels (that is a raw-stage tool).
**Typical requests:** "remove speckles/white dots", "salt and pepper noise".
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `false` | | Master switch. |
| `Threshold` | int | `50` | 0..100 (GUI) | Level a pixel must exceed to be treated as an impulse. (Engine divides it by the preview scale; direction of "stronger" is unverified: docs say it is the threshold that must be exceeded.) |
**Interactions & pitfalls:** acts on isolated pixels only; does nothing for normal luminance/chroma noise (use `[Denoise]`).
**Example:**
```ini
[Impulse Denoising]
Enabled=true
Threshold=50
```
**Suggested starting values:** default 50; leave alone otherwise.

## Defringing  —  `[Defringing]`
**What it does:** suppresses coloured fringes (purple/green/blue) along high-contrast
edges by averaging chroma over a neighbourhood, in Lab, after sharpening (stage 2). Only hues
selected by `HueCurve` are treated; the default curve targets purple/magenta fringes.
**Typical requests:** "remove purple fringing", "chromatic aberration at edges", "green/purple halos".
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `false` | | Master switch. |
| `Radius` | double | `2.0` | 0.5..5 | Averaging radius in pixels; larger removes wider fringes but desaturates real edges. |
| `Threshold` | int | `13` | 0..100 | Chroma-edge level above which defringing is applied; lower = more aggressive. (Stored as integer.) |
| `HueCurve` | flat curve (list of doubles) | see below | points in 0..1 | X = hue, Y = strength of removal at that hue. |
**How to write it:** `HueCurve` is a `;`-separated list. First number is the curve type
(`1` = control-point curve, `0` = linear/identity), followed by 4 numbers per point:
`x;y;leftHandle;rightHandle`. Default curve (peaks at x about 0.83, i.e. purple/magenta;
zero elsewhere):
`HueCurve=1;0.166666667;0;0.35;0.35;0.347;0;0.35;0.35;0.513667426;0;0.35;0.35;0.668944571;0;0.35;0.35;0.8287775246;0.97835991;0.35;0.35;0.9908883827;0;0.35;0.35;`
To target all hues use `HueCurve=0;` (unverified that a bare linear type means "all hues at full strength"; safer: raise the Y of the other points of the default curve).
The hue-to-colour mapping of the x axis is unverified beyond "0.83 is purple".
**Interactions & pitfalls:** can wash out genuine small coloured details at edges; also see `[CACorrection]`/`[LensProfile]` with `UseCA=true` which fix the cause.
**Example:**
```ini
[Defringing]
Enabled=true
Radius=2.0
Threshold=13
```
**Suggested starting values:** subtle `Radius=1.5, Threshold=20`; moderate default (2.0/13); strong `Radius=3.5, Threshold=8`.

## Denoise  —  `[Denoise]`
**What it does:** wavelet-based luminance and chrominance noise reduction in the
linear RGB working space at the very beginning of the RGB pipeline (before geometry
and before tone/contrast tools, so it denoises the unamplified data). Optional
"final smoothing" adds a guided chroma filter and non-local-means (NL-means) luminance
smoothing. Pushed too far: waxy, plastic textures, loss of fine detail, colour bleeding.
**Typical requests:** "reduce noise", "clean up the grain in the shadows", "high ISO
denoise", "remove colour noise", "smooth skin noise".
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `false` | | Master switch. With `Luminance=0` and `Chrominance=0` the wavelet stage does nothing (final smoothing may still run). |
| `ColorSpace` | string | `RGB` | `RGB`, `LAB` | Space in which the wavelet denoise runs. RGB is the default; LAB is the older behaviour. |
| `Aggressive` | bool | `false` | | `false` = conservative (keeps low-frequency chroma patterns), `true` = aggressive (removes blotchy low-frequency chroma noise, more levels, slower). |
| `Gamma` | double | `1.7` | 1.0..3.0 | Distributes strength over tones. Lower = targets shadows, higher = reaches brighter tones. |
| `Luminance` | double | `0` | 0..100 | Wavelet luminance denoise strength. 0 = no luminance denoise. |
| `LuminanceDetail` | double | `0` | 0..100 | "Detail recovery": brings back fine luminance detail. (GUI slider starts at 50; the profile default is 0.) |
| `LuminanceDetailThreshold` | int | `0` | 0..100 | "Detail threshold" for detail recovery (restricts recovery to the strongest details when set). |
| `ChrominanceMethod` | int | `1` | `0` = manual, `1` = automatic | Written as an integer. Automatic estimates chroma noise from the image. |
| `ChrominanceAutoFactor` | double | `1` | 0..1 | Automatic mode only: scales the estimated strength. |
| `Chrominance` | double | `15` | 0..100 | Manual mode only: master chroma denoise. |
| `ChrominanceRedGreen` | double | `0` | -100..100 | Manual mode only: extra red-green chroma denoise (added on top of master). |
| `ChrominanceBlueYellow` | double | `0` | -100..100 | Manual mode only: extra blue-yellow chroma denoise. |
| `SmoothingEnabled` | bool | `false` | | Enable "Final smoothing" (the next three keys). |
| `GuidedChromaRadius` | int | `3` | 0..20 (GUI) | Final smoothing: radius of the guided-filter chroma smoothing; 0 = off. (GUI slider starts at 0.) |
| `NLStrength` | int | `0` | 0..100 | Final smoothing: NL-means luminance strength; 0 = off. |
| `NLDetail` | int | `80` | 1..100 (GUI) | Final smoothing: NL-means detail preservation (GUI slider starts at 50). Only used when `NLStrength` > 0. |
**How to write it:** all scalars. `SmoothingMethod` (int) is read from old profiles only
(any value other than 1 turns smoothing off) and is never written; do not emit it.
**Interactions & pitfalls:**
* Which keys belong to which method: chroma has two modes. `ChrominanceMethod=1` uses `ChrominanceAutoFactor` and ignores `Chrominance`/`ChrominanceRedGreen`/`ChrominanceBlueYellow`; `ChrominanceMethod=0` is the reverse. Luminance has one method (wavelet: `Luminance`, `LuminanceDetail`, `LuminanceDetailThreshold`), plus the optional NL-means pass via `SmoothingEnabled=true` + `NLStrength`.
* Because `ChrominanceMethod` defaults to automatic, a partial profile with just `Enabled=true` already denoises chroma automatically (and the default `Chrominance=15` is ignored then).
* Setting `Luminance` does nothing visible while `Enabled=false`.
* Denoise runs before the crop is cut out; it makes export slower. Very strong `Luminance` plus `LuminanceDetail=0` looks plastic.
* The GUI may show only some of these controls depending on mode, but all keys are stored.
**Example:**
```ini
[Denoise]
Enabled=true
Luminance=25
LuminanceDetail=50
ChrominanceMethod=1
ChrominanceAutoFactor=1
```
**Suggested starting values** (heuristic, not from code; assume modern full-frame sensor, scale down for small sensors):
* ISO <=800: usually off; or `Luminance=0`, auto chroma (`ChrominanceMethod=1`).
* ISO 1600-3200: `Luminance=20`, `LuminanceDetail=50`, auto chroma.
* ISO 6400-12800: `Luminance=40`, `LuminanceDetail=50`, auto chroma, `Aggressive=false`; optionally `SmoothingEnabled=true`, `GuidedChromaRadius=3`.
* ISO >=25600: `Luminance=60`, `LuminanceDetail=40`, `Aggressive=true`, `SmoothingEnabled=true`, `GuidedChromaRadius=5`, `NLStrength=20`, `NLDetail=50`.
* "Subtle" = lower half of those, "strong" = next ISO bracket.

## CA Correction  —  `[CACorrection]`
**What it does:** corrects lateral chromatic aberration (red and blue channels slightly scaled relative to green) after demosaic, as part of the geometry transform. (The raw-tab CA correction is a different tool, acting before demosaic.) Use zoom 100% to judge.
**Typical requests:** "fix colour fringing in the corners", "red/blue edges toward the border", "CA".
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `false` | | Master switch (also needs a non-zero `Red` or `Blue` to do anything). |
| `Red` | double | `0.0` | -0.005..0.005 (GUI) | Radial scale offset of the red channel versus green (dimensionless fraction of radius). |
| `Blue` | double | `0.0` | -0.005..0.005 (GUI) | Same for blue. |
**Interactions & pitfalls:** values are tiny (steps of 0.0001); direction (sign) depends on the lens, no rule of thumb: the MCP cannot see the result without a preview. Prefer `[LensProfile]` with `UseCA=true` or `[Defringing]`.
**Example:**
```ini
[CACorrection]
Enabled=true
Red=0.0008
Blue=-0.0006
```
**Suggested starting values:** only if the user gives numbers; typical real corrections are within +-0.002.

## Vignetting Correction  —  `[Vignetting Correction]`
**What it does:** radial brightening (or darkening) of the image toward the corners to cancel lens vignetting; applied in the geometry transform step, after demosaic. For artistic vignettes use the Vignette/Graduated filter tools instead.
**Typical requests:** "remove lens vignetting", "brighten the corners" (technical); "darken the edges" is better served by the artistic vignette (other file).
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `false` | | Master switch (also needs `Amount` non-zero). |
| `Amount` | int | `0` | -100..100 | Positive brightens the edges (corrects vignetting), negative darkens them. |
| `Radius` | int | `50` | 0..100 | How far in from the edge the effect reaches. Lower = bigger affected area, higher = only the very corners. |
| `Strength` | int | `1` | 1..100 | Amplifies `Amount`/`Radius` (gain is 1 + Strength*|Amount|*3/400). |
| `CenterX` | int | `0` | -100..100 | Horizontal centre offset in percent of image width. |
| `CenterY` | int | `0` | -100..100 | Vertical centre offset in percent of image height. |
**How to write it:** integers only (not `0.5`).
**Interactions & pitfalls:** `Strength` default 1 is already a mild effect; with `Strength=1` the internal factor v is only 1 + |Amount|*3/400 (1.75 at Amount=100), so raise `Strength` for a visible effect. If `[LensProfile]` has `UseVignette=true` do not also correct manually. The centre is relative to the geometry frame (before cropping; unverified).
**Example:**
```ini
[Vignetting Correction]
Enabled=true
Amount=60
Radius=50
Strength=20
```
**Suggested starting values:** subtle `Amount=30, Strength=10`; moderate `Amount=60, Strength=30`; strong `Amount=100, Strength=60`.

## Crop  —  `[Crop]`
**What it does:** keeps a rectangle of the image; the rest is discarded in the export.
It is the first thing after geometry correction (see order). Resize "Cropped area" refers to it.
**Typical requests:** "crop to 4:5", "crop to square", "crop in tighter", "remove the left third", "16:9 crop", "centre crop".
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `false` | | If `false` the crop rectangle is ignored: the whole image is exported (X/Y/W/H are kept in the file and can be re-enabled). Must be `true` for any crop to apply. |
| `X` | int | `-1` | 0..width-1 | Left edge, pixels. `-1` = not set. |
| `Y` | int | `-1` | 0..height-1 | Top edge, pixels. |
| `W` | int | `-1` | 1..width-X | Crop width, pixels. |
| `H` | int | `-1` | 1..height-Y | Crop height, pixels. |
| `FixedRatio` | bool | `true` | | "Lock ratio". With `X/Y/W/H` all given, the engine just uses the rectangle as is (ratio keys do not recompute it). They matter (a) when geometry is missing (see below) and (b) as the constraint the user sees when editing in the GUI. |
| `Ratio` | string | `As Image` | `As Image`, `Current`, `3:2`, `4:3`, `16:9`, `16:10`, `1:1`, `2:1`, `3:1`, `4:1`, `5:1`, `6:1`, `7:1`, `4:5`, `5:7`, `6:7`, `6:17`, `24:65 - XPAN`, `1.414 - ISO 216 (A4 paper)`, `3.5:5`, `8.5:11 - US Letter`, `9.5:12`, `10:12`, `11:14`, `11:17 - Tabloid`, `13:19`, `17:22`, `45:35 - ePassport`, `64:27` | Written as the exact label string. `As Image` and `Current` are not numeric (they mean "keep image ratio"/"keep current rectangle's ratio"). Old names `DIN`, `1.414 - DIN EN ISO 216`, `8.5:11`, `11:17` are mapped to the labels above on load. |
| `Orientation` | string | `As Image` | `As Image`, `Landscape`, `Portrait` | Which way round the ratio is applied; see the quirk below. |
| `Guide` | string | `Frame` | `None`, `Frame`, `Rule of thirds`, `Rule of diagonals`, `Harmonic means`, `Grid`, `Golden Triangle 1`, `Golden Triangle 2`, `ePassport` | Overlay in the crop editor only; no effect on the image. |

**Units and coordinate frame (important):**
* `X, Y, W, H` are integer pixels of the FULL-RESOLUTION image after `[Coarse Transformation]` (so for a 90/270 degree rotation, width and height are swapped) and after the geometry transform (`[Rotation]`, `[Perspective]`, `[Distortion]`, `[LensProfile]`, with `AutoFill`), whose output frame has the same size as its input. The origin is the top-left of that frame. They are NOT relative to the rotated bounding box and NOT percentages, and they are independent of the preview zoom or any `[Resize]`.
* The frame size is the raw image size after the raw border/crop of the decoder, not necessarily the camera's nominal megapixel count: the MCP server supplies the exact width x height.
* The engine clamps: `X`/`Y` < 0 become 0; `W`/`H` <= 0 or overrunning the frame are trimmed to the frame edge; `X > width` or `Y > height` disables the crop entirely.
* Missing geometry: if `X`, `Y`, `W`, `H` are not all present (a partial profile with only `Enabled`, `FixedRatio`, `Ratio`, `Orientation`), both the editor and the export compute the largest centred crop with the requested ratio. Honoured only when `FixedRatio=true` and `Ratio` is numeric (`As Image`/`Current` give the whole frame). Prefer explicit numbers when the size is known.

**Ratio/Orientation quirk:** the ratio is first reduced to its long side. Then:
`Landscape` gives w/h = long/short, `Portrait` gives w/h = short/long, `As Image` follows the image (landscape if width >= height, portrait otherwise). BUT for ratios whose label is written short-side first (value < 1: `4:5`, `5:7`, `6:7`, `6:17`, `24:65 - XPAN`, `8.5:11 ...`, `9.5:12`, `10:12`, `11:14`, `11:17 ...`, `13:19`, `17:22`, `3.5:5`; `1.414 - ISO 216` is long-first so not affected) the GUI stores the orientation swapped: the GUI's "portrait" is stored as `Landscape` and vice versa. In the engine: `Ratio=4:5` + `Orientation=Landscape` yields w/h = 0.8 (portrait 4:5); `Orientation=Portrait` yields 1.25 (landscape 5:4). Ratios written long-first (`3:2`, `16:9`, `4:3`, ...) behave as named (`Landscape` = wide). Avoid the ambiguity by always writing explicit `X/Y/W/H` computed as below, and set `Ratio`/`Orientation` consistently for the GUI.

**Worked formula: centred crop with ratio R = rw:rh (w/h wanted) for a frame of W0 x H0** (W0,H0 after coarse rotation; MCP server knows them):
```
R = rw / rh                         # for "4:5 portrait" R = 0.8 ; "5:4 landscape" R = 1.25
if W0 / H0 >= R:  H = H0 ; W = round(H0 * R)     # image is wider than the target: trim the sides
else:             W = W0 ; H = round(W0 / R)     # image is taller: trim top/bottom
X = (W0 - W) // 2 ; Y = (H0 - H) // 2
```
Example, 6000x4000 landscape, "crop to 4:5" portrait: R=0.8, 6000/4000=1.5 >= 0.8, so H=4000, W=3200, X=1400, Y=0. Same frame, "crop to 5:4": R=1.25, H=4000, W=5000, X=500, Y=0. 4000x6000 portrait frame, 4:5: R=0.8, 0.667 < 0.8, so W=4000, H=5000, X=0, Y=500. Square 1:1 on 6000x4000: W=H=4000, X=1000, Y=0. If the user does not say orientation for a ratio, pick the one matching the image's orientation ("4:5" for a landscape photo is usually meant as portrait 4:5 only if they say so; ask or state the assumption).
Off-centre crop: choose X/Y yourself, keeping X+W <= W0, Y+H <= H0 (e.g. "left third" is X=0, W=W0//3, Y=0, H=H0).

**How to write it:** ratio strings contain spaces and punctuation, write them verbatim without quotes: `Ratio=24:65 - XPAN`.
**Interactions & pitfalls:**
* Coordinates move when the user later rotates by 90 degrees or applies a rotation/perspective with `AutoFill`: crop after those decisions.
* `[Resize]` with `AppliesTo=Cropped area` scales relative to the crop; with `Full image` the whole image is scaled to the given size and the crop is then scaled by the same factor.
* With `[Rotation]` and `AutoFill=false` the rotated corners are empty; crop inside the valid area or enable `AutoFill`.
**Example (4:5 portrait crop, 6000x4000 frame):**
```ini
[Crop]
Enabled=true
X=1400
Y=0
W=3200
H=4000
FixedRatio=true
Ratio=4:5
Orientation=Landscape
Guide=Rule of thirds
```
(`Orientation=Landscape` is correct here because of the short-side-first storage quirk above.)

## Coarse Transformation  —  `[Coarse Transformation]`
**What it does:** lossless 90 degree rotations and mirror flips applied as the image is read from the raw, before everything else. Use for "rotate the photo to portrait", "flip horizontally".
**Typical requests:** "rotate 90 degrees clockwise", "turn it upside down", "mirror it".
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Rotate` | int | `0` | `0`, `90`, `180`, `270` | Clockwise rotation in degrees (`90` = rotate right/clockwise, `270` = rotate left/counter-clockwise). Any other value is treated as 0. |
| `HorizontalFlip` | bool | `false` | | Mirror left-right. |
| `VerticalFlip` | bool | `false` | | Mirror top-bottom. |
**Interactions & pitfalls:** no `Enabled` key: the values apply directly. For `90`/`270` width and height swap, so any `[Crop]` coordinates change meaning. Order when combined: the flips are applied to the already rotated image. This is a different tool from `[Rotation]` (free angle). Whether the camera's EXIF orientation is already included before this rotation is unverified; assume `Rotate=0` shows the image as the app displays it by default.
**Example:**
```ini
[Coarse Transformation]
Rotate=90
HorizontalFlip=false
VerticalFlip=false
```

## Common Properties for Transformations  —  `[Common Properties for Transformations]`
**What it does:** the "Geometry" tool: shared option for `[Rotation]`, `[Perspective]`, `[Distortion]` and `[LensProfile]`.
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `AutoFill` | bool | `true` | | `true`: the image is scaled up/down just enough so that no empty (black) border appears after rotation/perspective/distortion (some edge content is lost). `false`: no scaling; empty corners may appear (then the GUI's "Auto-Crop" button sets a `[Crop]` to the valid area; that button is not a profile key). |
**Interactions & pitfalls:** the default `true` is what you want for rotations of a few degrees. Cropped content differs between `true` and `false`, so decide before computing a `[Crop]`.
**Example:**
```ini
[Common Properties for Transformations]
AutoFill=true
```

## Rotation  —  `[Rotation]`
**What it does:** fine rotation by an arbitrary angle (horizon levelling), resampled together with the other geometry tools.
**Typical requests:** "straighten the horizon", "tilt it 2 degrees", "level the image".
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `false` | | Must be `true` or `Degree` is ignored. |
| `Degree` | double | `0.0` | -45..45 | Rotation angle in degrees. |
**Sign convention (derived from the code, `iptransform.cc`):** positive `Degree` rotates the image content COUNTER-CLOCKWISE (rotate left); negative rotates it clockwise. To fix a horizon that slopes down to the right (clockwise tilt) by 2 degrees, use `Degree=2`. (The GUI icon on the positive side shows "rotate left"; a code comment calls the slider counter-intuitive.)
**Interactions & pitfalls:** rotates about the image centre; `[Common Properties for Transformations]` `AutoFill` decides zoom vs empty corners. `[Perspective]` has its own `Angle` (-20..20) which also rotates. Do not use both for the same correction.
**Example:**
```ini
[Rotation]
Enabled=true
Degree=1.5
```

## Distortion  —  `[Distortion]`
**What it does:** manual radial lens distortion correction.
**Typical requests:** "fix barrel distortion", "straighten the curved lines", "pincushion".
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `false` | | Master switch. |
| `Amount` | double | `0.0` | -0.5..0.5 (GUI) | Negative corrects barrel distortion, positive corrects pincushion (per docs). Typical needed values are small (|Amount| < 0.1). |
| `Auto` | bool | `false` | | Compute `Amount` automatically by matching the embedded JPEG preview (export/auto at load; works only if the camera corrected the embedded JPEG; otherwise no effect). |
**Interactions & pitfalls:** if `[LensProfile]` already corrects geometry (`UseDistortion=true`), do not also set this. `AutoFill` determines zoom.
**Example:**
```ini
[Distortion]
Enabled=true
Amount=-0.03
Auto=false
```

## LensProfile  —  `[LensProfile]`
**What it does:** profile-based lens correction (geometric distortion, vignetting, lateral CA) from the lensfun database, an Adobe LCP file, or data embedded in the file's metadata.
**Typical requests:** "apply lens correction", "correct the lens distortion/vignetting automatically", "use my LCP".
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `LcMode` | string | `none` | `none`, `lfauto`, `lfmanual`, `lcp`, `exif` | `none` = tool off. `lfauto` = automatic match of camera+lens from the metadata in the bundled lensfun database. `lfmanual` = use `LFCameraMake`/`LFCameraModel`/`LFLens`. `lcp` = use `LCPFile`. `exif` = correction data embedded in the file (only works when the file carries it; otherwise the GUI falls back to `lfauto`). Unknown strings read as `none`. |
| `LCPFile` | string (path) | empty | absolute path or `file://` URI | `lcp` mode only. Written as a URI; relative paths resolve against the sidecar directory in profiles of the current version. |
| `UseDistortion` | bool | `true` | | Correct geometric distortion. |
| `UseVignette` | bool | `true` | | Correct vignetting. |
| `UseCA` | bool | `false` | | Correct lateral chromatic aberration. |
| `LFCameraMake` | string | empty | lensfun maker name | `lfmanual` only (e.g. `Canon`). |
| `LFCameraModel` | string | empty | lensfun model name | `lfmanual` only. |
| `LFLens` | string | empty | lensfun lens name | `lfmanual` only. |
**How to write it:** strings verbatim. Easiest and safest request-time value: `LcMode=lfauto`.
**Interactions & pitfalls:**
* There is no `Enabled`; the group is active iff `LcMode` is not `none`. To switch it off write `LcMode=none`.
* The GUI can disable options the profile does not provide; the file may still say `true` (no effect).
* Distortion/vignetting corrections here overlap `[Distortion]` and `[Vignetting Correction]`; use one source. With `lfauto` and an unmatched lens nothing is corrected (unverified: silent).
* Needs a larger working region (the editor widens by ~15%); `AutoFill` governs zoom.
**Example:**
```ini
[LensProfile]
LcMode=lfauto
UseDistortion=true
UseVignette=true
UseCA=true
```
**Suggested starting values:** `lfauto` with all three corrections on, unless the user wants to keep optical character (then `UseVignette=false`).

## Perspective  —  `[Perspective]`
**What it does:** corrects converging verticals/horizontals (keystone), shear and aspect, with a projective transform based on the focal length (darktable "ashift" algorithm). Includes auto-correction from detected or user-drawn lines (the GUI computes the numbers; in a profile you write the result).
**Typical requests:** "fix converging verticals", "straighten the building", "keystone", "make the lines parallel".
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `false` | | Master switch. Only acts if one of `Horizontal`, `Vertical`, `Angle`, `Shear` is non-zero. |
| `Horizontal` | double | `0.0` | -200..200 | Horizontal perspective correction (yaw). Sign convention unverified. |
| `Vertical` | double | `0.0` | -200..200 | Vertical perspective correction (pitch); the usual keystone fix. Sign convention unverified. Unit: the docs say percent for the lens-shift part, but the values are in a tool-internal scale; judge by preview. |
| `Angle` | double | `0.0` | -20..20 | Rotation applied with the perspective transform (degrees). |
| `Shear` | double | `0.0` | -50..50 | Corrects diagonal distortion. |
| `FocalLength` | double | `0` | 1..1200 (mm) | Real focal length. `0` means "not set" (the GUI shows 28 as a slider default). Strongly recommended to write the real one: the MCP server knows it from EXIF. |
| `CropFactor` | double | `1` | 0.1..10 | Sensor crop factor. Only the product `FocalLength*CropFactor` matters; with a 35 mm-equivalent focal length use `CropFactor=1`. |
| `Aspect` | double | `1` | 0.5..2 | Non-skewing aspect-ratio adjustment (<1 narrower, >1 wider; unverified direction). |
| `ControlLines` | int list | empty | groups of 5 ints | Lines for the auto mode: `type;x1;y1;x2;y2` repeated; `type` `0` = vertical line, `1` = horizontal line; coordinates in pixels. Informational for the GUI; the engine applies the numeric keys above, so an LLM normally should not write this. |
**Interactions & pitfalls:** an LLM cannot compute good Horizontal/Vertical values without seeing the image; use small values (|Vertical| <= 15) only on an explicit request and tell the user it needs a visual check. The tool is applied together with `[Rotation]` and zoom/fill rules; crop afterwards. Needs the whole frame in the export (no crop optimisation). Do not combine `Angle` with `[Rotation]`.
**Example:**
```ini
[Perspective]
Enabled=true
Horizontal=0
Vertical=8
Angle=0
Shear=0
FocalLength=24
CropFactor=1.5
Aspect=1
```
**Suggested starting values:** subtle `Vertical=+-4`; moderate `+-10`; strong `+-20`. Always set `FocalLength`/`CropFactor`.

## Resize  —  `[Resize]`
**What it does:** scales the output (export) image. Applied after tone processing and before output sharpening. It affects the saved/queued output, not the editor preview image (the editor preview is not resized; the factor is only used to scale `[OutputSharpening]` in the preview; unverified details).
**Typical requests:** "export at 2048 px on the long side", "half size", "1080 wide", "fit in 1920x1080", "print 20x30 cm at 300 dpi".
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `false` | | Master switch. |
| `DataSpecified` | int | `3` | `0` scale, `1` width, `2` height, `3` bounding box | Which mode is used. Written as an integer. |
| `Scale` | double | `1.0` | 0.01..16 | Mode 0: uniform factor (1 = no change). Minimum output size is about 32x32 px. |
| `Width` | double | `900` | 32.. (soft) | Modes 1 and 3. In `Unit` units. Mode 1: scale so the width equals this (image aspect kept, height follows). Mode 3: box width. |
| `Height` | double | `900` | 32.. (soft) | Modes 2 and 3, same idea. |
| `Unit` | string | `px` | `px`, `cm`, `in` | Unit of `Width`/`Height`. For `cm`/`in` pixels = PPI * size (`cm`/2.54). |
| `PPI` | int | `300` | soft (GUI: 1..) | Pixels per inch for unit conversion and for `CopyPPIToExif`. |
| `CopyPPIToExif` | bool | `false` | | Write `PPI` to EXIF X/YResolution (inches) of the output when metadata mode is `1` (edit). |
| `AppliesTo` | string | `Cropped area` | `Cropped area`, `Full image` | Reference size: `Cropped area` = size of the crop (when crop is enabled) is what Width/Height/Scale refer to. `Full image` = refer to the full uncropped frame, then the crop is scaled by the same factor (so the output is smaller than the numbers given). |
| `AllowUpscaling` | bool | `false` | | If `false`, modes 1, 2, 3 never enlarge (factor > 1 is ignored; mode 3 clamps to 1). Mode 0 (`Scale`) may always upscale (the export code uses `AllowUpscaling || DataSpecified==0`). |
**Modes:** mode 1 (width): `scale = Width / refW`; mode 2 (height): `scale = Height / refH`; mode 3 (bounding box): fit inside Width x Height keeping aspect, regardless of orientation (landscape or portrait photo both fit). Modes 1/2 are literal (not orientation-aware): "2048 on the long side" means choose mode 1 for a landscape photo, mode 2 for a portrait one, or simply use mode 3 with Width=Height=2048.
**Interactions & pitfalls:**
* Pixel size is `round(ref * scale)` where ref is the crop (if `AppliesTo=Cropped area` and crop enabled) else the frame. If the crop is disabled, both options use the whole frame.
* Mode 3 and modes 1/2 never enlarge unless `AllowUpscaling=true`; the engine skips resizing when scale is within 1e-5 of 1.
* Resize is separate from `[OutputSharpening]`; enable the latter for downsized exports.
**Example (2048 px long-edge bounding box):**
```ini
[Resize]
Enabled=true
DataSpecified=3
Width=2048
Height=2048
Unit=px
AppliesTo=Cropped area
AllowUpscaling=false
```
**Suggested starting values:** web 2048 or 1600 box; social 1080 box; screen 3840 box.

## Output Sharpening  —  `[OutputSharpening]`
**What it does:** unsharp-mask/RL sharpening applied at the very end, after `[Resize]`, to compensate for downsizing. Same engine as `[Sharpening]`, but with fewer saved keys and different defaults. Older profiles called this group `PostResizeSharpening` (still read).
**Typical requests:** "sharpen for web", "sharpen the exported image", "output sharpening".
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `false` | | Master switch. Independent of `[Resize]`. |
| `Method` | string | `usm` | `usm`, `rld` (GUI offers only these two) | Method. |
| `Contrast` | double | `25` | 0..200 | Contrast threshold mask, as in `[Sharpening]` (GUI slider default 15; profile default is 25). |
| `Radius` | double | `0.5` | 0.3..3 | `usm`. |
| `Amount` | int | `200` | 1..1000 | `usm`. |
| `Threshold` | int list (4) | `20;80;2000;1200` | each 0..2000 | `usm`. As in `[Sharpening]`. |
| `OnlyEdges` | bool | `false` | | `usm`. |
| `EdgedetectionRadius` | double | `1.9` | 0.5..2.5 | `usm` + `OnlyEdges`. |
| `EdgeTolerance` | int | `1800` | 10..10000 | `usm` + `OnlyEdges`. |
| `HalocontrolEnabled` | bool | `false` | | `usm`. |
| `HalocontrolAmount` | int | `85` | 1..100 | `usm` + halo control. |
| `DeconvRadius` | double | `0.75` | 0.4..2.5 (GUI default 0.45) | `rld`. |
| `DeconvAmount` | int | `100` | 0..100 | `rld`. |
**How to write it:** as `[Sharpening]`. The `DeconvAutoRadius`, `DeconvCornerBoost`, `DeconvCornerLatitude`, `PSFKernel` and `PSFIterations` keys are not saved or loaded for this group: do not write them (corner boost is effectively off; auto radius, being a default `true` in the params struct, uses the raw-derived radius only for the capture sharpening, so set `DeconvRadius` explicitly if using `rld`: unverified).
**Interactions & pitfalls:** do not also oversharpen in `[Sharpening]`. Needed mostly after strong downsizing.
**Example:**
```ini
[OutputSharpening]
Enabled=true
Method=usm
Contrast=25
Radius=0.5
Amount=150
```
**Suggested starting values:** web export `usm`, `Radius=0.5`, `Amount=100..150`; print `Radius=0.8`, `Amount=200`; "subtle" `Amount=60`.

## MetaData  —  `[MetaData]`
**What it does:** controls what Exif/IPTC metadata goes into the exported file (not pixel processing).
**Typical requests:** "strip all metadata / GPS for privacy", "keep original metadata", "add a copyright".
**Keys:**
| Key | Type | Default | Range / allowed values | Effect |
|---|---|---|---|---|
| `Mode` | int | `1` | `0` copy unchanged (tunnel), `1` edit/apply modifications, `2` strip all | `0`: output metadata copies the input; edits ignored. `1`: apply the `[Exif]`/`[IPTC]` edits and keep only `ExifKeys`. `2`: remove all metadata. |
| `ExifKeys` | string list | the 16 keys below | Exiv2 key names, or the single item `*` | Whitelist of Exif keys that are kept in mode 1. `*` alone keeps everything. Any key not listed is dropped. |
| `Notes` | string | empty | free text | Free-form note stored in the profile (not written to the image; unverified). |
**How to write it:** `ExifKeys` is a `;`-separated list: `ExifKeys=Exif.Image.Copyright;Exif.Image.Artist;...;`. Default list: `Exif.Image.Copyright`, `Exif.Image.Artist`, `Exif.Image.ImageDescription`, `Exif.Photo.UserComment`, `Exif.Image.Make`, `Exif.Image.Model`, `Exif.Photo.LensModel`, `Exif.Photo.FNumber`, `Exif.Photo.ExposureTime`, `Exif.Photo.FocalLength`, `Exif.Photo.ISOSpeedRatings`, `Exif.Photo.ExposureBiasValue`, `Exif.Photo.Flash`, `Exif.Photo.DateTimeOriginal`, `Exif.Image.XResolution`, `Exif.Image.YResolution`. Edited tag values (copyright text etc.) live in separate `[Exif]` and `[IPTC]` groups, not covered here.
**Interactions & pitfalls:** with `Mode=1` (default) many tags (GPS, serial numbers, ...) are already dropped because they are not in `ExifKeys`; `Mode=2` is the explicit "strip everything" request. `Resize` `CopyPPIToExif` only has an effect in mode 1. Old profiles (version < 1012) default to mode 0 with `ExifKeys=*`.
**Example:**
```ini
[MetaData]
Mode=2
```
**Suggested starting values:** privacy requests: `Mode=2`; keep-everything requests: `Mode=0`.
