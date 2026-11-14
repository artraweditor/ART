# Local editing: regions, masks and the local tools

Groups covered: `[ColorCorrection]`, `[Smoothing]`, `[Local Contrast]`,
`[TextureBoost]` (all four are **region lists**: each region = one adjustment
layer with its own parameters and its own mask), plus the stand-alone local
tools `[Spot Removal]`, `[Gradient]`, `[PCVignette]` (no regions, no masks).

Everything below was cross-checked against `ProcParams::load/save`
(`src/engine/procparams.cc`), the mask engine (`src/engine/masks.cc`) and by
rendering fragments with `ART-cli` on `build/test.arw` (items marked
"rendered" were verified that way). `PPVERSION` is **1047**. Either omit the
`[Version]` group or write `Version=1047`; a lower number switches on legacy
value translations (e.g. `<1028` re-maps ColorCorrection `A_N/B_N` and HSL
values, `<1045` rescales `ExternalMaskFeather`, `<1047` rescales
`NoiseCoarseness_N`).

---------------------------------------------------------------------------

## 0. Mental model

* A local tool applies each of its regions **in list order**; region *i* works
  on the result of region *i-1*; the result is blended with the input by the
  region's mask (`out = mask*processed + (1-mask)*input`).
* A region with no mask keys has the default `Mask` = whole image, full
  strength (rendered).
* Mask = product of independent sub-masks (parametric, colour-similarity,
  area, external, linked, brush) then post-processing, inversion, opacity
  (exact order in section 2.8).
* There is **no** `Areas=...;...|...` string syntax in this ART version. All
  region/mask data live in *indexed keys* of one group (section 1). The only
  `|` is in `LinkedMask` (`toolname|maskname`). The only packed value is a
  base64 list for polygon knots and brush strokes (section 2.5/2.6).

---------------------------------------------------------------------------

## 1. How regions are serialised (indexed keys)

Every key of a region carries a **suffix** `<S>`; per-shape keys of an area
mask additionally carry an **infix** `<I>`.

| Group | Region 1 suffix | Region 2 suffix | Region k |
|---|---|---|---|
| `[ColorCorrection]` | `_1` | `_2` | `_k` (k>=1) |
| `[Smoothing]` | `_1` | `_2` | `_k` (k>=1) |
| `[Local Contrast]` | *(empty)* | `_1` | `_(k-1)` |
| `[TextureBoost]` | *(empty)* | `_1` | `_(k-1)` |

(**Index base differs**: ColorCorrection/Smoothing are 1-based with the suffix
always present; Local Contrast/TextureBoost use no suffix for the first region
and `_1`, `_2`, ... for the next ones.)

Examples: `Slope_1`, `MaskEnabled_2` (ColorCorrection); `Contrast`,
`MaskEnabled`, `Contrast_1`, `MaskEnabled_1` (Local Contrast).

Area-mask shape infix `<I>`: shape 0 -> empty; shape j>=1 -> `_j_`.
Full key = `AreaMask` + `<I>` + `Field` + `<S>`. Example, ColorCorrection
region 2, shapes 0 and 1: `AreaMaskType_2`, `AreaMaskX_2`, `AreaMask_1_Type_2`,
`AreaMask_1_X_2`. Local Contrast region 0, shape 1: `AreaMask_1_Type`.

**Loading rules (from the code; they cause silent data loss if violated):**

1. The loader reads region 1 (Smoothing/CC) or region 0 (LC/TB), then the next
   index, and **stops at the first index for which not one single key exists**.
   A fragment containing only `Slope_2` (no `_1` key) loads *nothing*
   (rendered: LC with only `Contrast_1` had no effect). Number regions
   contiguously and always put at least one key in the first region.
2. If at least one region key was found, the tool's `regions` and `masks`
   lists are **replaced** (or appended, section 4). Keys missing from a loaded
   region take the constructor defaults, not the previous values.
3. Group-level keys (`Enabled`, `ShowMask`, `SelectedRegion`) are scalar:
   absent key = previous value kept. **Always write `Enabled=true`.**
4. For each area-mask shape the loader needs **all** keys of that shape
   (`Type`, geometry keys, `Mode`, `ShapeFeather`, `ShapeBlur`); if one is
   missing the shape *and all later shapes* are dropped, and for rectangle
   shapes a missing key can dereference a null pointer (crash) - always write
   the complete set.
5. Unknown keys are ignored. Keys are case-sensitive. Decimal point is `.`.
6. Only the groups present in the fragment are touched.

`ShowMask=-1` / `SelectedRegion=0` can be omitted. `ShowMask=k` shows region
k's mask in the preview only (index in the merged list). `[ColorCorrection]`
`ShowMask` is written by save as `ShowMask` but the loader reads
`LabRegionsShowMask` for `Version>=348` - it never round-trips (harmless).

---------------------------------------------------------------------------

## 2. Region mask keys (shared by all four region tools)

Notation: `<S>` = region suffix (section 1), `<I>` = shape infix.
Defaults are those of a fresh `Mask`/region. `0..1` etc. are the *stored*
ranges; GUI ranges are given where they differ.

### 2.1 Region-level keys

| Key | Type | Default | Range / values | Effect |
|---|---|---|---|---|
| `MaskEnabled<S>` | bool | `true` | | `false` disables the whole region (its params are not applied; the mask is not computed unless linked) |
| `MaskInverted<S>` | bool | `false` | | invert the final combined mask (`1-m`), applied after everything but opacity |
| `MaskName<S>` | string | empty | any | label; also the name other regions use in `LinkedMask` |
| `MaskCurve<S>` | diagonal curve | `0;` | curve string, sect. 3 | tone curve on the final mask (x=in, y=out) |
| `MaskPosterization<S>` | int | `0` | 0..6 | quantise mask to fewer levels (6 = two levels) |
| `MaskSmoothing<S>` | int | `0` | 0..100 | softens posterization edges; no effect when posterization is 0 |
| `MaskOpacity<S>` | int | `100` | 0..100 | multiply mask by opacity/100 (effect strength) |

### 2.2 Parametric mask (hue / chroma / lightness / contrast)

**Must have `ParametricMaskEnabled<S>=true`**, otherwise every key of this
block is ignored (default is `false`; a very common mistake).

| Key | Type | Default | Range | Effect |
|---|---|---|---|---|
| `ParametricMaskEnabled<S>` | bool | `false` | | master switch of this block |
| `HueMask<S>` | flat curve, periodic | `1;0.166666667;1;0.35;0.35;0.8287775246;1;0.35;0.35;` (= everything) | x 0..1, y 0..1 | y = mask value for pixels of hue x (see 2.2.1) |
| `ChromaticityMask<S>` | flat curve | `1;0;1;0.35;0.35;1;1;0.35;0.35;` | x 0..1, y 0..1 | y = mask for pixels of chromaticity x (0 = grey) |
| `LightnessMask<S>` | flat curve | `1;0;1;0.35;0.35;1;1;0.35;0.35;` | x 0..1, y 0..1 | y = mask for lightness x (x = CIE L*/100 of the pixel in the working profile) |
| `LightnessMaskDetail<S>` | int | `0` | 0..100 | detail protection: blend between the pixel lightness and a smoothed, posterised lightness before the L curve (only used if the L curve is non-default) |
| `ContrastThresholdMask<S>` | int | `0` | -150..150 | select pixels by local contrast; `0` off; negative values invert (select flat areas) |
| `ParametricMaskBlur<S>` | float | `0` | -10..500 | softness of the parametric mask (guided-filter based). `-10` = no smoothing at all; the guided-filter size factor is `-1/blur` for blur<0 and `1+blur` for blur>=0: `-1` and `0` = default size, `-2` = half size (sharper edges), `-0.5` = double, `+1` = double Also used as the blur of the contrast-threshold mask |

A curve that equals its default is skipped (no cost). The three curves
multiply with each other and with the colour-similarity mask.

#### 2.2.1 Axes of the parametric curves

* **Lightness**: x = L*/100 (0 black .. 1 white) of the pixel *in the working
  space at that point of the pipeline*. 18 % grey is x=0.5. Typical: deep
  shadows 0..0.2, midtones 0.3..0.7, bright sky/highlights 0.7..1.
  (Rendered: on the `test.arw` export most pixels sit at x~0.1..0.3; if a
  lightness ramp seems to select nothing, the pixels are darker than you
  assumed - the sidecar's exposure/tone settings shift this.)
* **Hue**: x = `lin2log( (H + 1/6) mod 1 , base 3 )` where H is ART's
  approximate HSV hue (0 red, 1/6 yellow, 1/3 green, 1/2 cyan, 2/3 blue,
  5/6 magenta). Precomputed:

  | colour | H | x |
  |---|---|---|
  | red | 0.00 | 0.26 |
  | skin / orange | 0.02..0.10 | 0.29..0.39 |
  | yellow | 0.17 | 0.47 |
  | green (foliage) | 0.25..0.40 | 0.55..0.68 |
  | cyan | 0.50 | 0.77 |
  | sky blue | 0.58..0.67 | 0.83..0.90 |
  | blue / violet | 0.67..0.78 | 0.90..0.97 |
  | magenta | 0.85 | 0.03 |

  The hue curve is **periodic** (the last segment wraps to the first point),
  so a selection may straddle x=1 -> 0 (magenta/red-magenta).
  (Rendered: a curve around x=0.27..0.42 selected pixels with median HSV hue
  0.083 on the test image.) Grey pixels have no meaningful hue: pair a hue
  curve with a chromaticity ramp to exclude low-chroma pixels.
* **Chromaticity**: x is a logarithmic (base 50) scale of Lab chroma, 0 = grey,
  larger = more saturated. The exact scale factor was not verified
  (unverified); prefer the colour-similarity mask when you need a chroma
  target.

### 2.3 Colour-similarity mask (DeltaE)

Selects pixels whose CIE Lab colour is close (CIEDE2000) to a reference colour.
Easiest way to select "skin", "the blue sky", "the red car": give Lab
lightness/chroma/hue of the reference. All keys `DeltaEMask<Field><S>`:

| Key | Type | Default | Range | Effect |
|---|---|---|---|---|
| `DeltaEMaskEnabled<S>` | bool | `false` | | master switch |
| `DeltaEMaskL<S>` | float | `0` | 0..1 | reference L*/100 |
| `DeltaEMaskC<S>` | float | `0` | 0..1 | reference chroma / 100 (0.25 = Lab chroma 25) |
| `DeltaEMaskH<S>` | float | `0` | 0..360 | reference Lab hue angle in degrees (0 = +a red-magenta, 90 = +b yellow, 180 = green-cyan, 270 = blue) |
| `DeltaEMaskRange<S>` | float | `1` | 0.1..100 | tolerance in DeltaE units |
| `DeltaEMaskDecay<S>` | float | `1` | 1..100 (GUI) | edge sharpness (small effect). **Negative value inverts the DeltaE mask** (GUI shows its absolute value + "inverted" box) |
| `DeltaEMaskStrength<S>` | int | `100` | 0..100 | scale the mask (opacity of this sub-mask) |
| `DeltaEMaskWeightL<S>` | int | `50` | 0..100 | importance of lightness difference (0 = ignore L) |
| `DeltaEMaskWeightC<S>` | int | `75` | 0..100 | importance of chroma difference |
| `DeltaEMaskWeightH<S>` | int | `100` | 0..100 | importance of hue difference |

Engine behaviour: for distance d (weighted DeltaE2000): `d <= 0.4*Range` ->
mask 1; `d >= 5*Range` -> 0; between: `1 - 1/(1+100*exp(-k*(d-Range)))`
with `k = 1 + |Decay|/100` (so the half-way point is ~`Range+4.6/k`). With the
default Range=1 the mask is very tight; for "a colour family" use
Range 8..15. If all three weights are 0 the sub-mask is 1 (inactive).
Typical Lab references (approximate, unverified): skin L 0.55..0.75,
C 0.15..0.35, H 35..65; blue sky L 0.5..0.75, C 0.2..0.4, H 240..275;
foliage L 0.3..0.6, C 0.2..0.5, H 110..140.
To make the selection independent of brightness set `DeltaEMaskWeightL` low.
(Rendered: H=55, L .6, C .25, Range 8 selected 88 % of the (orange) test
image; H=250 selected 1 %.)

### 2.4 Area mask: common keys

| Key | Type | Default | Range | Effect |
|---|---|---|---|---|
| `AreaMaskEnabled<S>` | bool | `false` | | **must be `true`** for shapes to matter |
| `AreaMaskFeather<S>` | float | `0` | 0..100 | edge-aware (guided filter) feathering of the *combined* shape mask, as % of the smallest shape half-extent. **Leave 0 when the mask consists only of gradient shapes** (the radius is derived from rectangle/polygon sizes; for gradient-only masks it is undefined (code reading)) |
| `AreaMaskBlur<S>` | float | `0` | 0..500 | Gaussian blur of the combined mask (pixels at full resolution, scaled with zoom) |
| `AreaMaskContrast<S>` | diagonal curve | `0;` | sect. 3 | curve on the combined area mask |

Shapes are listed in order (shape 0, 1, 2, ...), combined top to bottom:
start with an empty mask; `add` unions the shape, `subtract` cuts it,
`intersect` multiplies the running mask by the shape. Gaps in numbering stop
the loader.

Per-shape keys (all with `AreaMask<I>` ... `<S>`):

| Field | Type | Default | Range | Applies to | Effect |
|---|---|---|---|---|---|
| `Type` | string | - | `rectangle`, `gradient`, `polygon` (any other string loads as polygon) | all | shape kind |
| `Mode` | string | `add` | `add`, `subtract`, `intersect` (anything else = add) | all | boolean combination with previous shapes |
| `ShapeFeather` | float | `0` (gradient created in the GUI: `25`; the loader requires the key to be present, so always write it) | 0..100 | all | edge feather of this shape. For rectangle/polygon: guided-filter radius as % of the shape's smaller half-extent (rectangle; polygon: of its size). For gradient: width of the transition as % of the image **diagonal** (the full ramp is `ShapeFeather`% of the diagonal) |
| `ShapeBlur` | float | `0` | 0..500 | rectangle, polygon (ignored for gradient) | Gaussian blur of this shape's mask |
| `X`, `Y` | float | `0` | -100..100 | rectangle, gradient | shape centre |
| `Width`, `Height` | float | `100` | 1..200 (GUI) | rectangle | size |
| `Angle` | float | `0` | rectangle 0..180; gradient -180..180 (degrees) | rectangle, gradient | rotation |
| `Roundness` | float | `0` | 0..100 | rectangle | 0 = rectangle, 100 = ellipse (inscribed in width x height) |
| `StrengthStart`, `StrengthEnd` | float | `100`, `0` | 0..100 | gradient | mask strength on the start side / end side of the ramp |
| `Knots` | base64 string | - | see 2.5 | polygon | knots (x,y,roundness) triples |

#### 2.4.1 Coordinate system and units of area shapes

* **Reference frame**: the *full, uncropped image after all geometry
  corrections* (coarse rotate/flip, rotation, perspective, lens distortion),
  in its final orientation (+x right, **+y down**). The crop is applied
  afterwards: shape coordinates are **not** relative to the crop (rendered:
  with a crop of the right half, a gradient centred at X=0 had its centre on
  the crop's left edge). A subject at the right edge of the full frame is at
  X=+100 even if the crop removes the left half. Changing rotation or lens
  correction after drawing masks moves the masks (Reference.md "Mask
  precautions").
* **Normalised percent, centre-origin**: for `X`, `Y` (rectangle, gradient)
  and polygon knots: `0` = image centre, `-100` = left/top edge, `+100` =
  right/bottom edge. (Pixel position = `half_size + v/100 * half_size`.)
  Polygon knots may go to +-200 (outside the frame). Coordinates are
  independent of image resolution.
* **Rectangle** `Width`/`Height`: percent of the **full** image width/height
  (100 = whole width, not half). Shape is axis-aligned in the image, then
  rotated by `Angle` about its centre. Positive `Angle` rotates
  **counter-clockwise** on screen (rendered: a bar at +30 degrees rose to the
  right).
* **Ellipse / radial / spotlight** = rectangle with `Roundness=100`; circle
  needs `Width/100*imgW == Height/100*imgH` (e.g. in a 3:2 landscape image use
  Width=60, Height=90 for a circle).
* **Gradient**: line through the centre `(X,Y)`. At `Angle=0` the *start* side
  (`StrengthStart`) is the **top**, the *end* side the bottom; `Angle=90`:
  start = left; `180`: start = bottom; `-90` (or `270`): start = right
  (rendered, all four). Transition is centred on the line with full width
  `ShapeFeather`% of the image diagonal; outside the ramp the mask is
  `StrengthStart` or `StrengthEnd` percent (for `subtract` the values are
  taken as 100 - value). Default (100 -> 0) = "affect the top, leave the
  bottom" i.e. a sky gradient. `ShapeBlur` is ignored for gradients and `AreaMaskFeather` should stay 0.

### 2.5 Polygon knots encoding (`AreaMask<I>Knots<S>`)

Value = base64 (standard alphabet, `=` padding, length multiple of 4) of a
little-endian **IEEE half-float (float16)** array `[x0,y0,r0, x1,y1,r1, ...]`
(3 numbers per knot, >= 3 knots needed for a non-empty shape; fewer = shape
ignored). `x`,`y` in the coordinate system above, `r` = corner roundness
0 (sharp) .. 100 (fully round). Half-float precision is ~3 significant digits
(about 0.05 at |v|=64..100); values outside +-200 are not meaningful.

```python
import numpy as np, base64
pack   = lambda v: base64.b64encode(np.array(v, dtype='<f2').tobytes()).decode()
unpack = lambda s: np.frombuffer(base64.b64decode(s), dtype='<f2').astype(float).tolist()
pack([-100,-100,0, 100,-100,0, 0,0,0])   # 'QNZA1gAAQFZA1gAAAAAAAAAA'  (top triangle; rendered)
```

For a rounded outline alternate sharp knots (r=0) on the border and fully
round knots (r=100) as control points (Shapes.md).

### 2.6 Brush mask (`DrawnMask*`)

| Key | Type | Default | Range | Effect |
|---|---|---|---|---|
| `DrawnMaskEnabled<S>` | bool | `false` | | master switch |
| `DrawnMaskMode<S>` | int | `0` | 0 = intersect, 1 = add, 2 = add-bounded | 0: brush mask multiplies the other masks (restricts); 1: strokes are added on top of the other masks (with no other mask the whole image is already selected, so painting adds nothing - use 0 for a stand-alone brush); 2: like 1 but only inside the area mask (applied before the area mask) |
| `DrawnMaskFeather<S>` | float | `0` | 0..100 | edge-aware feather of the brushed mask |
| `DrawnMaskOpacity<S>` | float | `1` | 0..1 | global opacity of the brush mask (GUI shows 0..100) |
| `DrawnMaskSmoothness<S>` | float | `0` | 0..1 | Gaussian smoothing of the strokes, scaled by stroke radius (GUI 0..100) |
| `DrawnMaskContrast<S>` | diagonal curve | `0;` | sect. 3 | curve on the brush mask |
| `DrawnMaskStrokes<S>` | base64 string | empty | | strokes, same packing as 2.5 |

`DrawnMaskStrokes` half-float list = repeated groups
`[n, radius, paint, opacity, x1,y1, ..., xn,yn]` where `n` = number of dabs in
the group (<= 2048), `radius` in 0..1 (dab radius in pixels =
`min(imgW,imgH) * radius * 0.25`, so radius 1 = disc radius of 25 % of the
short side; the header comment "1 = 10 %" is wrong), `paint` = 1 (paint) or 0
(erase), `opacity` 0..1 (hardness of the dab), and the dab centres `x,y` in
**0..1 fractions of the full image width/height from the top-left corner**
(note: 0..1, not -100..100). Consecutive groups are applied in order.
Rendered example: `pack([3,0.5,1,1, 0.5,0.8, 0.55,0.8, 0.6,0.8])` with mode 0
darkened a ~12 %-of-width blob around (55 %, 80 %) only. Legacy read-only keys:
`DrawnMaskTransparency` (Version<1038, opacity = 1 - value), `DrawnMaskAddMode`
(Version<1019).

### 2.7 Linked and external masks

| Key | Type | Default | Effect |
|---|---|---|---|
| `LinkedMaskEnabled<S>` | bool | `false` | multiply this region's mask by another region's final mask |
| `LinkedMaskInverted<S>` | bool | `false` | use `1 - other` |
| `LinkedMask<S>` | string `toolname\|maskname` | empty | which mask: `toolname` in {`colorcorrection`, `smoothing`, `localcontrast`, `textureboost`}, `maskname` = the other region's `MaskName`. A value without `\|` is a load **error** (exception) |
| `ExternalMaskEnabled<S>` | bool | `false` | multiply by a greyscale image file used as mask |
| `ExternalMaskInverted<S>` | bool | `false` | invert the file mask |
| `ExternalMaskFilename<S>` | path/URI | empty | image file (green channel used; stretched over the full image - warns if aspect differs) |
| `ExternalMaskFeather<S>` | float | `0` | 0..100 edge-aware feather |

The linked mask must already have been computed when the linking region is
evaluated: same tool + lower region index always works; a mask from another
tool works only if that tool runs earlier in the pipeline (order unverified);
otherwise a warning is shown and the link is ignored. A mask is stored only
if it has a non-empty `MaskName` and some linking region refers to it.
External mask filename uses the URI conventions of `filenameToUri`
(`file://U/...` user config dir, `file://S/...` ART base dir,
`file://B/...` sidecar directory, `file://H/...` home); a plain absolute path
also loads. Neither feature was tested (unverified).

### 2.8 Evaluation order of one region's mask (code)

1. Parametric block: colour-similarity x hue x chroma x lightness (each only
   if enabled and non-default), then `ParametricMaskBlur` smoothing.
2. Contrast-threshold mask (multiplies).
3. Brush with mode 2 (add-bounded).
4. Area mask (shapes combined, feather, blur, contrast curve) - multiplies.
5. External mask - multiplies.
6. Linked mask - multiplies.
7. Brush with mode 0/1.
8. `MaskCurve`, `MaskPosterization` + `MaskSmoothing`.
9. `MaskInverted`.
10. `MaskOpacity`.

(Localediting.md describes only steps 1-6 loosely; it omits external/linked,
post-processing and opacity.) If no sub-mask is active the mask is all ones.
A region with `MaskEnabled=false` is skipped entirely.

---------------------------------------------------------------------------

## 3. Curve strings

Curves are `;`-separated numbers ending with `;`; first number = type.

* **Diagonal curve** (`MaskCurve`, `AreaMaskContrast`, `DrawnMaskContrast`):
  `0;` = linear (identity). `1;x1;y1;x2;y2;...;` = spline, `4;...` Catmull-Rom,
  `3;...` NURBS control points; x ascending, x,y in 0..1 (x = input mask value).
  Example "harder mask": `1;0;0;0.3;0.1;0.7;0.9;1;1;`. A value whose type is
  not in `[0, 4]` is replaced by linear when loaded (for AreaMaskContrast).
* **Flat curve** (hue/chroma/lightness masks, Local Contrast `Curve`):
  `1;` followed by points of **four numbers each**: `x;y;leftWidth;rightWidth`.
  y is the curve value at x. `leftWidth`/`rightWidth` (0..1, default 0.35) are
  fractions of the distance to the neighbouring point over which the curve
  eases in/out; 0 makes a straight line to that neighbour. Points are
  expected in ascending x. For masks y = 1 means "selected".
  *Select a band* (skin hue): `1;0.27;0;0.35;0.35;0.31;1;0.35;0.35;0.37;1;0.35;0.35;0.42;0;0.35;0.35;`
  (rises 0.27->0.31, plateau to 0.37, falls to 0.42; used in the rendered
  test). *Lightness ramp* (highlights only):
  `1;0;0;0.35;0.35;0.5;0;0.35;0.35;0.8;1;0.35;0.35;1;1;0.35;0.35;`.
  A curve with all y equal to the default (0.5 identity for flat curves
  internally) is treated as unchanged.

---------------------------------------------------------------------------

## 4. Append vs overwrite when a partial profile is applied

* **Overwrite (default "partial profile" application, and loading a sidecar):**
  only groups present in the fragment are touched. Inside a region group,
  scalar keys present overwrite; if the fragment contains any region key, the
  **entire region and mask list of that tool is replaced** by the fragment's
  regions (missing keys in those regions = defaults).
* **Append mode** (GUI profile panel "append" toggle ->
  `FilePartialProfile(..., append=true)` -> `ParamsEdited::set_append(true)`):
  for exactly these four tools - `[ColorCorrection]`, `[Smoothing]`,
  `[Local Contrast]`, `[TextureBoost]` - the loader does
  `new_list = existing_regions (first) + fragment_regions`, same for
  masks, **iff the existing lists differ from the pristine default** (one
  default region with default mask). If the tool is still pristine, the
  fragment simply replaces it. Existing regions keep their position (first);
  the fragment's regions come after them, so they act later and see the
  earlier regions' output. Everything else (all other groups, and the scalar
  keys `Enabled`, `ShowMask`, `SelectedRegion` of these tools) is still
  overwritten when present: so write `Enabled=true`, and do not write
  `ShowMask`/`SelectedRegion` (their indices would refer to the merged list).
* Write the fragment region numbering as if it were standalone (`_1`, `_2`,
  ...; LC/TB from no suffix); the loader renumbers on append.
* `[Spot Removal]` entries are **always pushed onto the existing list**
  (`spot.entries` is not cleared by the loader), in both modes (code reading).
* `[Gradient]` and `[PCVignette]` are plain scalar groups: overwritten key by
  key.
* Append behaviour was verified by reading the code only (not run) because
  `ART-cli -p` uses the plain overwrite path.

---------------------------------------------------------------------------

## ColorCorrection  --  `[ColorCorrection]`

**What it does:** the general-purpose local colour/tone tool (called
"Color/Tone Correction"): per region it can change luminance (gain / lift /
gamma / pivot / highlight compression), colour (a tint "wheel", input and
output saturation, hue shift), per-channel RGB curves, three-way HSL colour
wheels, or run a LUT/CTL script. Each region is limited by its mask. Placed
near the end of the pipeline, after tone curves and geometry. Pushed too far:
banding in shadows (large Offset/Slope), posterised or neon colours (large
wheel values, saturation +100), crushed or clipped regions where the mask edge
is abrupt.
**Typical requests:** "darken the sky", "warm the shadows", "cool tint overall",
"make the red car pop", "desaturate the background", "brighten the subject",
"split-tone", "lift the blacks", "apply this LUT only to the foreground".

**Group keys:**

| Key | Type | Default | Range | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `false` | | tool on/off (regions ignored when false) |
| `ShowMask` | int | `-1` | -1..regions-1 | preview-only mask display (not read back, see sect. 1) |
| `SelectedRegion` | int | `0` | | GUI selection only |
| `DisableSpatialGuides` | bool | `false` | | written only when true; stops spatial filtering of CTL script guides |

**Region keys** (suffix `_N`, N = 1..):

| Key | Type | Default | Range (GUI; soft) | Effect |
|---|---|---|---|---|
| `Mode_N` | string | **`Jzazbz`** if absent | `YUV` ("Standard"), `Jzazbz` ("Perceptual"), `RGB` ("Separate RGB channels"), `HSL` ("HSL factors"), `LUT` | which controls are used. Files from Version<1005 use `RGBChannels_N`, `Channel_N` (legacy) |
| `Slope_N` | float | `1` | 0.01..32 | gain (highlights/gain): multiplies linear luminance; 2 = +1 stop, 0.5 = -1 stop. Used by modes YUV, Jzazbz (and read, but overridden, in HSL) |
| `Offset_N` | float | `0` | -0.15..0.15 | lift (shadows): added before gamma (applied as Offset/2) |
| `Power_N` | float | `1` | 0.1..4 | midtone gamma: >1 brightens midtones, <1 darkens (exponent is 1/Power) |
| `Pivot_N` | float | `1` | 0.001..2 | fixed point of the Power curve (0.18 ~ mid grey; 1 = white) |
| `Compression_N` | float | `0` | 0..1 | soft highlight compression after the curve |
| `SlopeR_N`, `SlopeG_N`, `SlopeB_N` | float | `1` | 0.01..10 | per-channel gain, **used only in mode `RGB`** |
| `OffsetR_N`, `OffsetG_N`, `OffsetB_N` | float | `0` | -0.15..0.15 | per-channel lift (RGB mode) |
| `PowerR_N`, `PowerG_N`, `PowerB_N` | float | `1` | 0.1..4 | per-channel gamma (RGB mode) |
| `PivotR_N`, `PivotG_N`, `PivotB_N` | float | `1` | 0.001..2 | per-channel pivot (RGB mode) |
| `CompressionR_N`, `CompressionG_N`, `CompressionB_N` | float | `0` | 0..1 | per-channel compression (RGB mode) |
| `RGBLuminance_N` | bool | `false` | | RGB mode: the per-channel curves alter only luminance, keeping colour |
| `A_N`, `B_N` | float | `0` | -1..1 (times `ABScale`) | colour wheel position (tint added proportionally to luminance) - modes YUV and Jzazbz only; see below |
| `ABScale_N` | float | `1` | | GUI zoom of the wheel only; the engine ignores it (A/B are already stored scaled) |
| `InSaturation_N` | float | `0` | -100..100 | chroma multiplier `1+x/100`, applied before the colour wheel (after the tone curve in YUV/Jzazbz modes, before the per-channel curves in RGB/HSL modes) |
| `OutSaturation_N` | float | `0` | -100..100 | saturation change after the wheel |
| `HueShift_N` | float | `0` | -180..180 | hue rotation in degrees (not used in `RGB` mode) |
| `SlopeH_N`, `SlopeS_N`, `SlopeL_N` | float | `0` | H -180..180 deg; S 0..100; L -50..50 | **HSL mode** "highlights" wheel: hue, saturation, lightness factor (%) |
| `OffsetH_N`, `OffsetS_N`, `OffsetL_N` | float | `0` | H -180..180; S 0..100; L -10..10 | HSL mode "shadows" wheel |
| `PowerH_N`, `PowerS_N`, `PowerL_N` | float | `0` | H -180..180; S 0..100; L -50..50 | HSL mode "midtones" wheel |
| `HSLGamma_N` | float | `2.4` (`1.0` for files older than Version 1042 that lack the key) | 1..3 | HSL mode working gamma |
| `LUTFilename_N` | path/URI | empty | | LUT / CTL script for mode `LUT` (and built-in CTL "modes") |
| `LUTParams_N` | string list | empty | | CTL script parameters: `;`-separated `name=v1,v2,...` entries (e.g. `RED_H=0.12;YEL_S=-0.28;`) |

plus all mask keys of section 2 with suffix `_N`.
(Check: the saturation range of the HSL wheel is a GUI-widget property;
0..100 is an assumption (unverified).)

**How to write it:** one `[ColorCorrection]` group; keys of region *k* end in
`_k`. Which keys matter depends on `Mode_k`: modes `YUV`/`Jzazbz`/`HSL`
read the *plain* `Slope_k/Offset_k/Power_k/Pivot_k/Compression_k` (HSL mode
recomputes its gain/lift/gamma from the `*H/*S/*L` wheel keys instead and
ignores the plain ones); mode `RGB` reads only the `*R/*G/*B` keys. Keys for
other modes can be left out. Processing order inside a region: hue shift ->
tone (slope/offset/power/pivot/compression on linear luminance) -> input
saturation -> colour wheel (`A`,`B`) -> output saturation. In `Standard` and
`Perceptual` modes the luminance maths is identical; `Perceptual` performs the
saturation/wheel/hue operations in the Jzazbz space.
**Colour wheel A/B** (YUV and Jzazbz): `(A,B)` is a 2-D point; its direction is
the tint hue and its length the strength. Rendered with `A=+-0.3`/`B=+-0.3`:
`A>0,B=0` -> red; `A=0,B>0` -> yellow-green; `A<0,B=0` -> cyan;
`A=0,B<0` -> blue/violet. So hue angle (HSL degrees) ~ `atan2(B, A)`:
orange (30 deg) ~ `A=0.87m, B=0.5m`; yellow ~ `A=0.5m,B=0.87m`
(approximate - ART's hue circle is not exactly HSL). Strength `m` is passed
through `(4^m-1)/3`: subtle `m` 0.03..0.08, moderate 0.1..0.2, strong 0.3+
(A=0.3 shifted mean R by +15 %, G/B by -8 % on a warm test image). Values
are clipped to +-1 in the GUI; larger magnitudes are allowed by the file
loader (soft range).
**Interactions & pitfalls:**
* Absent `Mode_k` means **`Jzazbz`** (default region), not `YUV`.
* `Slope`/`Offset`/`Power` act on *linear* light: `Slope=0.6` is -0.74 stop
  (rendered: the output luma of a full-strength region dropped to ~0.7 of the
  original).
* The default region does nothing; an enabled tool with all-default regions is
  a no-op.
* For local use, `AreaMaskEnabled`/`ParametricMaskEnabled`/`DeltaEMaskEnabled`
  must be `true` (section 2).
* Overlapping area/parametric masks multiply; overlapping *regions* compound.
* `Enabled=false` -> everything else in the group ignored.
**Example (global warm tint + gentle lift, one region, no mask):**
```ini
[ColorCorrection]
Enabled=true
Mode_1=YUV
Power_1=1.1
A_1=0.07
B_1=0.04
OutSaturation_1=8
```
(no `Mask*` keys: default mask = whole image; `A/B` ~ orange tint, `Power` 1.1
lifts midtones; `Offset_1=0.02` would also lift blacks.)
**Suggested starting values:** gain `Slope` 0.8 / 1.25 (subtle), 0.6 / 1.6
(moderate), 0.4 / 2 (strong); wheel strength `m` 0.05 / 0.15 / 0.3;
`OutSaturation` +-8 / +-25 / +-50; hue shift 5 / 15 / 40 degrees.

---------------------------------------------------------------------------

## Smoothing  --  `[Smoothing]`

**What it does:** per-region spatial filter applied to luminance, chrominance
or RGB: guided (edge-preserving) blur, Gaussian blur, glow, non-local-means
denoise, motion blur, lens blur, added noise (grain), halation, wavelet
denoise. Typical local uses: skin smoothing, background blur, local noise
reduction, glow on highlights, halation around bright lights. Pushed too far:
plastic skin / lost texture (guided radius large, detail low), halos at mask
edges (feather the mask), banding.
**Typical requests:** "smooth the skin", "soften the background", "reduce
noise in the shadows only", "add a dreamy glow", "add film grain to the sky",
"motion blur on the water", "halation on the street lights".

**Group keys:** `Enabled` (bool, default `false`), `ShowMask` (int, -1),
`SelectedRegion` (int, 0) - as in ColorCorrection. For Version<1016 the group
was called `[GuidedSmoothing]` (still read).

**Region keys** (suffix `_N`, N = 1..; defaults are the region constructor):

| Key | Type | Default | Range (GUI) | Used by mode | Effect |
|---|---|---|---|---|---|
| `Mode_N` | int | `0` | 0..8 | all | 0 Guided, 1 Gaussian, 2 Glow, 3 Non-local means, 4 Motion blur, 5 Lens blur, 6 Add noise, 7 Halation, 8 Wavelets |
| `Channel_N` | int | **`2`** | 0..2 | 0,1,3,6,8 | 0 luminance, 1 chrominance, 2 RGB. Glow forces RGB; motion/lens/halation ignore it |
| `Radius_N` | int | `0` | 0..1000 | 0 Guided, 4 Motion, 5 Lens | blur radius in pixels at full resolution (guided: 0 = nothing) |
| `Sigma_N` | float | `0` | 0..500 | 1 Gaussian, 2 Glow | Gaussian sigma in pixels at full resolution (0 = nothing) |
| `Epsilon_N` | int | `0` | -10..10 | 0 Guided | "Detail": higher keeps more edges/detail (less smoothing), lower smooths more |
| `Iterations_N` | int | `1` | 1..10 | 0,1,2,3 | repeat count |
| `Falloff_N` | float | `1` | 0.5..2 | 2 Glow | how fast the glow decays per iteration |
| `NLStrength_N` | int | `0` | 0..100 | 3 NL-means | denoise strength (0 = nothing) |
| `NLDetail_N` | int | `50` | 1..100 | 3 NL-means | detail preservation |
| `NumBlades_N` | int | `9` (GUI default 5) | 3..11 | 5 Lens | aperture blades |
| `Angle_N` | float | `0` | -90..90 | 4 Motion, 5 Lens | direction (motion) / blade rotation (lens), degrees |
| `Curvature_N` | float | `0` | -2..2 | 4 Motion | curvature of the motion path |
| `Offset_N` | float | `0` | -1..1 | 4 Motion | displacement of the effect relative to the pixel |
| `NoiseStrength_N` | int | `10` | 0..100 | 6 Noise | amount of added noise (not drawn in thumbnails/navigator) |
| `NoiseCoarseness_N` | int | `30` | 0..100 | 6 Noise | grain size (files <1047 are rescaled x1.5) |
| `HalationSize_N` | float | `1` | 0.001..5 | 7 Halation | glow radius scale / strength ("Strength" in GUI) |
| `HalationColor_N` | float | `0` | -0.5..0.5 | 7 Halation | tint of the glow ("Tint") |
| `WavStrength_N` | float | `0` | 0..100 | 8 Wavelets | denoise strength |
| `WavLevels_N` | int | `5` | 2..8 | 8 Wavelets | wavelet levels |
| `WavGamma_N` | float | `2.2` | 1..4 | 8 Wavelets | gamma of the working space |

plus all mask keys of section 2 with suffix `_N`.

**How to write it:** write only the keys used by the chosen mode (plus
`Mode_N`, `Channel_N` when relevant). The default region (radius 0, sigma 0,
strength 0) does nothing, so a region *must* set a size/strength key. Radii
are pixel counts at full resolution: a 24 MP image (6000 px wide) uses larger
values than a 6 MP one for the same look.
**Interactions & pitfalls:** The tool operates after the geometry/tone stages;
`Mode_N` is an **integer** here (unlike ColorCorrection's string). Motion and
lens blur only replace pixels inside the mask (no bleeding from outside).
Regions whose mask is small are processed on the bounding box only (speed).
**Example (guided luminance skin smoothing; see recipe 3 below):**
```ini
[Smoothing]
Enabled=true
Mode_1=0
Channel_1=0
Radius_1=30
Epsilon_1=-3
Iterations_1=2
```
**Suggested starting values:** guided luminance: radius 8/20/40 px, epsilon
-1/-3/-5, iterations 1/2/3 (subtle/moderate/strong; moderate was rendered).
NL-means chroma denoise: `NLStrength` 20/40/70, `NLDetail` 50, `Channel_N=1`.
Gaussian blur background: `Sigma` 3/8/20. Glow: `Mode_1=2`, `Sigma` 20,
`Iterations` 3, `Falloff` 1.2 (unverified look). Grain: `Mode_1=6`,
`NoiseStrength` 10/25/50, `NoiseCoarseness` 30.

---------------------------------------------------------------------------

## Local Contrast  --  `[Local Contrast]`

**What it does:** per-region wavelet-based local contrast on L*: the curve
scales contrast per detail scale (left of the curve = fine detail, right =
coarse), the "residual contrast" slider changes the coarsest residual. Applied
to luminance. Pushed too far: halos around edges, crunchy / HDR look.
**Typical requests:** "add punch / clarity", "more local contrast on the
mountains", "soften midtone contrast", "bring out detail in the clouds".

**Group keys:** `Enabled` (bool, `false`), `ShowMask` (int, -1),
`SelectedRegion` (int, 0).

**Region keys** (suffix: region 0 none, then `_1`, `_2`, ... - section 1):

| Key | Type | Default | Range | Effect |
|---|---|---|---|---|
| `Contrast<S>` | float | `0` | -100..100 | residual (coarse) contrast; positive boosts, negative flattens |
| `Curve<S>` | flat curve | `1;0;0.5;0;0;1;0.5;0;0;` | y 0..1, 0.5 = neutral | contrast per detail scale: y above 0.5 boosts that scale, below reduces; x 0 = finest detail |

plus all mask keys of section 2 with suffix `<S>`.

**How to write it:** a curve with points `(x;y;0;0)` is piecewise linear;
`1;0;0.75;0.35;0;1;0.5;0;0;` boosts the finest scales (the mf-like profile uses
y=0.73 at x=0). Strength of the effect is the distance of y from 0.5.
**Interactions & pitfalls:** with `Contrast=0` and the default curve the region
does nothing. Region 0 must have at least one key; `Contrast_1` alone is not
loaded (rendered). Overlaps with TextureBoost and Sharpening - avoid stacking.
**Example (global clarity on one region, second region softening the centre):**
```ini
[Local Contrast]
Enabled=true
Contrast=30
Curve=1;0;0.65;0.35;0;1;0.5;0;0;
Contrast_1=-50
Curve_1=1;0;0.5;0;0;1;0.5;0;0;
AreaMaskEnabled_1=true
AreaMaskType_1=rectangle
AreaMaskX_1=0
AreaMaskY_1=0
AreaMaskWidth_1=30
AreaMaskHeight_1=30
AreaMaskAngle_1=0
AreaMaskRoundness_1=0
AreaMaskMode_1=add
AreaMaskShapeFeather_1=20
AreaMaskShapeBlur_1=0
```
(structure rendered: second region acted only inside the box.)
**Suggested starting values:** `Contrast` +15 / +35 / +60; curve y at x=0:
0.58 / 0.68 / 0.8.

---------------------------------------------------------------------------

## TextureBoost  --  `[TextureBoost]`

**What it does:** per-region edge-aware detail/texture amplification of
luminance in linear RGB (guided-filter based, two detail levels). Positive
strength adds texture/clarity, negative softens. Pushed too far: grungy,
over-sharpened look, noise amplification. (Legacy group name `[EPD]` for
Version<1000.)
**Typical requests:** "more texture", "bring out the grain of the wood",
"soften skin texture a bit", "add clarity".

**Group keys:** `Enabled` (bool, `false`), `ShowMask` (int, -1),
`SelectedRegion` (int, 0).

**Region keys** (suffix like Local Contrast: none for region 0):

| Key | Type | Default | Range (GUI) | Effect |
|---|---|---|---|---|
| `Strength<S>` | float | `0` | -2..2 | >0 boosts texture, <0 softens; **0 = region does nothing** |
| `DetailThreshold<S>` | float | `0.2` | 0.01..100 | detail scale: filter radius = 3.5 x value pixels at full resolution (small = fine texture, large = coarse structure). Files <1009 call it `EdgeStopping` |
| `Iterations<S>` | int | `1` | 1..5 | repeat count |

plus all mask keys of section 2 with suffix `<S>`.

**Interactions & pitfalls:** overlaps with Local Contrast/Sharpening. Region 0
must have at least one key.
**Example:**
```ini
[TextureBoost]
Enabled=true
Strength=0.5
DetailThreshold=0.5
Iterations=1
```
**Suggested starting values:** Strength 0.2 / 0.6 / 1.2 (negative -0.3 for
skin softening); DetailThreshold 0.2 (fine) .. 2 (medium).

---------------------------------------------------------------------------

## Spot Removal  --  `[Spot Removal]`

**What it does:** clone/heal patches: copies image content from a source
circle onto a target circle (feathered), optionally blending the original
detail (heal). Runs early in the pipeline (before most other tools). No
mask, no regions.
**Typical requests:** "remove the dust spots", "clone out the sign at ...",
"remove that person" (small objects only).

**Keys:**

| Key | Type | Default | Range | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `false` | | tool on/off |
| `Spot1`, `Spot2`, ... | 8 numbers, `;`-separated | - | see below | one spot per key, numbered from 1 without gaps (loading stops at the first missing number) |

**How to write it:** `SpotK=srcX;srcY;dstX;dstY;radius;feather;opacity;detail;`
(a trailing `;` is what ART writes; both forms load). A list of exactly 8
numbers is required, other lengths are ignored.
* `srcX,srcY` / `dstX,dstY`: **pixel** coordinates (integers) of the centres of
  the source and target circles in the full-resolution image, origin top-left,
  +y down, in the final orientation of the image, uncropped (the GUI sliders go
  0..image width/height). Rendered: `Spot1=500;500;2000;3000;60;0;1;0` changed
  exactly the box x 1941..2059, y 2941..3059 of the output. Coordinates are
  not normalised: use the actual pixel size of the image (not verified for
  rotated/flipped raws or after a crop beyond this check).
* `radius`: int, 2..200 pixels (clamped on load).
* `feather`: 0..1 (fraction of the radius added as soft edge; feather circle
  radius = radius x (1 + feather)); ART default 1.
* `opacity`: 0..1 (default 1).
* `detail`: int 0..5 detail protection: 0 = plain clone, higher = heal-like
  blending with the target's own detail (default 2).
Spots whose source or target lie outside the image are skipped.
**Interactions & pitfalls:** spots are **appended** to the spot list already
in the image's sidecar when a fragment is applied (see sect. 4), so
re-applying the same fragment duplicates spots. Source and target boxes
should not overlap.
**Example:**
```ini
[Spot Removal]
Enabled=true
Spot1=1200;800;1260;840;25;1;1;2
Spot2=3000;2100;3050;2100;18;1;1;2
```
**Suggested starting values:** dust: radius 10..25, feather 1, opacity 1,
detail 2..3.

---------------------------------------------------------------------------

## Gradient  --  `[Gradient]` (Graduated Filter)

**What it does:** a global exposure ramp, like a graduated neutral-density
filter: darkens (or brightens) one side of the frame smoothly. No mask; for
local colour/tone gradients use a region with a gradient shape instead. Runs
in the geometry stage on the full uncropped frame (the centre is relative to
the full frame, not the crop; rendered with a crop for the area-mask gradient,
code reading for this tool).
**Typical requests:** "darken the sky gradually", "graduated ND filter",
"burn the top edge".

**Keys:**

| Key | Type | Default | Range | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `false` | | on/off |
| `Strength` | float | `0.6` | -5..5 (stops) | exposure change on the "top" side in stops: **positive darkens**, negative brightens (gain `2^-Strength`) |
| `Degree` | float | `0` | -180..180 | rotation of the gradient (degrees). `0`: the affected side is the **top**; `90`: left; `180`: bottom; `-90`: right (rendered) |
| `Feather` | int | `25` | 0..100 | width of the transition as % of the image diagonal (0 = hard edge) |
| `CenterX`, `CenterY` | int | `0` | -100..100 | position of the transition centre: 0 = frame centre, +-100 = frame edge (centre = 0.5 + v/200 of the width/height) |

**Interactions & pitfalls:** ignored if `Strength` is ~0. Strength is in stops
(unlike the area-mask gradient, which uses a 0..100 % ramp).
**Example (darken sky by 1 stop, horizon slightly above centre):**
```ini
[Gradient]
Enabled=true
Degree=0
Feather=30
Strength=1.0
CenterX=0
CenterY=-10
```
(rendered with Strength=1, CenterY=0: top x0.62, bottom x1.0 in encoded luma.)
**Suggested starting values:** Strength 0.5 / 1 / 2 stops; Feather 25..50.

---------------------------------------------------------------------------

## PCVignette  --  `[PCVignette]` (Vignette Filter, post-crop)

**What it does:** artistic vignette that darkens (or brightens) the corners
with a rectangle/ellipse/circle-shaped falloff. It is **placed relative to
the crop rectangle** (post-crop), so the vignette follows the final framing.
No mask. Pushed too far: obvious dark corners, banding.
**Typical requests:** "add a vignette", "darken the edges", "make a radial
vignette", "brighten the corners".

**Keys:**

| Key | Type | Default | Range | Effect |
|---|---|---|---|---|
| `Enabled` | bool | `false` | | on/off |
| `Strength` | float | `0.6` | -6..6 (stops) | darkening in the corners in stops: **positive darkens**, negative brightens (gain `2^-Strength`); at 6 the corners go black |
| `Feather` | int | `50` | 0..100 | how far the falloff reaches towards the centre: 0 = only the corners, 50 = halfway, 100 = to the centre |
| `Roundness` | int | `50` | 0..100 | 0 = rectangular with rounded corners, 50 = ellipse fitted to the crop, 100 = circle |
| `CenterX`, `CenterY` | int | `0` | -100..100 | move the centre: +-100 = shifted by half the crop width/height |

**Interactions & pitfalls:** relative to the crop rectangle (rendered: centre
unchanged, corners x0.59, edge-middle x0.79 at Strength=1, Feather=50,
Roundness=50). For a *spotlight on a subject*, use an area-mask region
instead (recipe 4). Do not confuse with `[Vignetting Correction]` (lens fall-off
correction).
**Example:**
```ini
[PCVignette]
Enabled=true
Strength=1.0
Feather=60
Roundness=60
CenterX=0
CenterY=0
```
**Suggested starting values:** Strength 0.4 / 0.8 / 1.5 stops; Feather 50..80;
Roundness 50..80.

---------------------------------------------------------------------------

## Recipes (all structurally cross-checked against the loader; most rendered)

### Recipe 1 - global tone/colour change, one region, no mask

Use `[ColorCorrection]` with a single region and no `Mask*` keys (see the
ColorCorrection example). Same idea for the other tools:
`[Smoothing]` `Mode_1/Channel_1/Radius_1` only, `[Local Contrast]` `Contrast=..`
only, `[TextureBoost]` `Strength=..` only. Minimal, complete and valid:
```ini
[ColorCorrection]
Enabled=true
Mode_1=YUV
Slope_1=1.2
OutSaturation_1=10
```

### Recipe 2 - darken the sky (gradient area mask x lightness mask)

Gradient shape: start side = top (angle 0), strength 100 -> 0, centre slightly
above the middle, long feather; plus a lightness ramp so only the bright sky is
touched. The masks multiply.
```ini
[ColorCorrection]
Enabled=true
Mode_1=YUV
Slope_1=0.6
MaskEnabled_1=true
MaskName_1=Sky
ParametricMaskEnabled_1=true
LightnessMask_1=1;0;0;0.35;0.35;0.5;0;0.35;0.35;0.8;1;0.35;0.35;1;1;0.35;0.35;
AreaMaskEnabled_1=true
AreaMaskFeather_1=0
AreaMaskBlur_1=0
AreaMaskType_1=gradient
AreaMaskX_1=0
AreaMaskY_1=-10
AreaMaskStrengthStart_1=100
AreaMaskStrengthEnd_1=0
AreaMaskAngle_1=0
AreaMaskMode_1=add
AreaMaskShapeFeather_1=40
AreaMaskShapeBlur_1=0
```
Rendered (with a lightness ramp matched to that image): top band darkened to
~0.9 of the original where the ramp selected, bottom untouched; without the
lightness mask the top ran to ~0.7 and dark foreground pixels were also
darkened (0.82 vs 0.99). Tune the lightness ramp x-positions to the image
(x = L*/100). `Slope_1=0.6` ~ -0.74 stop; for "moodier" also add
`OutSaturation_1=10` or `A_1/B_1` towards blue (`A_1=-0.05`, `B_1=-0.05`
~ blue-cyan tint).

### Recipe 3 - skin / hue-selective smoothing

Two interchangeable selectors (both rendered): a hue curve plus optional
chroma ramp, or a colour-similarity mask. Smoothing on **luminance only**
(`Channel_1=0`) keeps colour. Guided filter (`Mode_1=0`).
```ini
[Smoothing]
Enabled=true
Mode_1=0
Channel_1=0
Radius_1=30
Epsilon_1=-3
Iterations_1=2
MaskEnabled_1=true
MaskName_1=Skin
ParametricMaskEnabled_1=true
HueMask_1=1;0.27;0;0.35;0.35;0.31;1;0.35;0.35;0.37;1;0.35;0.35;0.42;0;0.35;0.35;
ChromaticityMask_1=1;0;0;0.35;0.35;0.25;1;0.35;0.35;1;1;0.35;0.35;
ParametricMaskBlur_1=0
```
(`ChromaticityMask` ramps from 0 at x=0 to 1 at x=0.25 to drop greys; the exact
chroma scale is unverified - if skin vanishes lower the ramp.) Alternative
selector (replace the three Parametric keys by):
```ini
DeltaEMaskEnabled_1=true
DeltaEMaskL_1=0.65
DeltaEMaskC_1=0.25
DeltaEMaskH_1=55
DeltaEMaskRange_1=10
DeltaEMaskDecay_1=1
DeltaEMaskStrength_1=100
DeltaEMaskWeightL_1=20
DeltaEMaskWeightC_1=75
DeltaEMaskWeightH_1=100
```
To restrict to a person rather than all skin-coloured pixels intersect with an
area mask (`AreaMaskEnabled_1=true` plus an ellipse around the face) or a
brush mask (`DrawnMaskMode_1=0`). Add `MaskOpacity_1=70` to keep some pores.

### Recipe 4 - radial vignette / spotlight

* Whole-frame vignette: use `[PCVignette]` (above).
* Spotlight / "darken everything except the subject": ColorCorrection region
  with an ellipse (`Roundness=100`), centred on the subject, inverted:
```ini
[ColorCorrection]
Enabled=true
Mode_1=YUV
Slope_1=0.4
MaskEnabled_1=true
MaskInverted_1=true
AreaMaskEnabled_1=true
AreaMaskType_1=rectangle
AreaMaskX_1=0
AreaMaskY_1=0
AreaMaskWidth_1=70
AreaMaskHeight_1=70
AreaMaskAngle_1=0
AreaMaskRoundness_1=100
AreaMaskMode_1=add
AreaMaskShapeFeather_1=60
AreaMaskShapeBlur_1=0
```
  (rendered: centre x0.99, mid-edges x0.8..0.9, corners x0.5..0.58.) For
  "brighten the subject" drop `MaskInverted_1`, use `Slope_1=1.5`, move
  `AreaMaskX_1/Y_1` onto the subject (e.g. `-30`, `-10`) and shrink
  `AreaMaskWidth_1/Height_1` (35/50). Width/Height are percentages of the full
  image width/height, so a circle in a non-square frame needs different values.

### Recipe 5 - append a layer to existing local edits

Write the fragment normally (regions numbered from `_1`), include
`Enabled=true`. Applied in append mode (section 4) it is added after the
existing regions of the same tool; applied in overwrite mode it replaces them.

---------------------------------------------------------------------------

## Doc / code disagreements and unverified items

Disagreements (code trusted):
* The task premise of `Areas` strings with `;`/`|` separators does not exist
  here; masks are indexed keys. Only `LinkedMask` uses `|`.
* `Localediting.md` mask pipeline omits external/linked masks, mask curve,
  posterization, opacity and the brush "add-bounded" ordering (code: sect. 2.8).
* `procparams.h` comments: DrawnMask radius "1 = 10 %" (code: radius px =
  short side x radius x 0.25); `AreaMask::Gradient::x/y` "percentage of width/
  height" (code: percent of the *half* extent, 0 = centre); gradient angle
  "[0;360[" (GUI and engine accept -180..180 and negative angles).
* `NumBlades` constructor default 9 vs GUI default 5 (range 3..11).
* `[ColorCorrection] ShowMask` is saved as `ShowMask` but loaded as
  `LabRegionsShowMask` (never round-trips).
* `Reference.md` says only the vignette is crop-relative: confirmed
  (`[PCVignette]`); area masks and `[Gradient]` use the full uncropped frame.

Unverified:
* Chromaticity-curve x scale; exact hue-axis match to real HSV (derived from
  code, one render agreed to within ~0.1 in hue).
* Flat-curve interpolation details beyond the rendered examples.
* LinkedMask across tools (pipeline order), ExternalMask, HSL-mode wheel ranges,
  LUT mode / CTL parameters, Smoothing glow/halation/wavelet looks.
* Append semantics (code reading only), and that `ART-cli -p` uses overwrite.
* Spot coordinates for rotated/flipped raws and with a crop.
* Rotation sense of the gradient `[Gradient] Degree` beyond the four rendered
  angles; rectangle `Angle` sense confirmed at +30 only.
