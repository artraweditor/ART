---
title: Gamma - Differential
contributors:
  - Lebarhon
  - DrSlony
---

Translation in progress. Any help welcome

## Generalities

The tone curves are one of the key points of all the image processing
softwares. RawTherapee does not escape to this rule... and we can even
say the user is fulfil by the number and the types of the available
curves (may be they are too many?) in RGB mode and L\*a\*b\*, CIECAM,
... The question is often to know - and for the "Input" (matrix, ICC,
DCP...) as well - what are we looking for and what are we acting on?
Usually we act on the luminance L (L\* of L\*a\*b\*), or Y (Y of XYZ),
or L equivalent rebuilt from the RGB values, ... Of course, users and
programmers are aware of a problem! it is enough to read the forums,
everyone try to add its contributions suggesting some options - mixing
or not with the color component - as for instance the introduction of a
logarithmic scale or a tainted appearance model.

### Proposition

- The color management - LCMS for RawTherapee - put at programmer and
  user disposal some performing tools to handle the data.
- These tools are used:

1.  A the process start by attributing an input profile of matrix type,
    ICC or DCP,
2.  During the conversion of the data stored on the sensor into data
    usable by the main process, in a color space (sRGB, Adobe,
    ...Prophoto), immediately before the image processing,
3.  At last, to convert these data for various use (screen, Web,
    printing, ...) into an output space.

It is obvious that the points 1, 2 and 3 must be improved in order to
make RawTherapee more efficient, but the goal of this proposition is new
for reason of the introduction of color management in the middle of the
processing in order to allow:

1.  tone modifications that take color management (primaries, gamma,
    TRC) into account and not only the luminance variations
2.  a differentiated action inside a part of the processing in order to
    boost or attenuate the actions in relation with luminance, without
    globally alter the processing.

## Methods

Several methods are at your disposal:

### Standard (Relative ICC)

- Apply a ICC profile over the current parameters ("gamma sRGB" witch is
  the RawTherapee default parameter)
- This profile use the current color space primaries and apply a gamma
  and a TRC on the RGB data placed after the RGB conversion and the
  white balance for each canal Red, Green and Blue. That leads to the
  modification of the RGB values that is taken into account in the
  processes that use these data, and of the L\*a\*b\* values that are
  deduced of it.
- You can act - in a relative way - on the gamma from 0.5 to 3.0, and on
  the slope from 0.0 to 20.0
- All the no-raw processes are concerned. So, are excluded: Demosaicing,
  Raw Black and White points, Preprocessing, Chromatic aberration, Dark
  Frame, Flat field, Retinex, Noise Reduction, White Balance, Crop,
  Resize, Lens/Geometry, Distorsion, Vignetting,...

### Standard (Absolute ICC)

- Apply a ICC profile on data from which the original gamma ("gamma
  sRGB") has been subtracted
- We could thing that this method is like a "softproofing"; it's almost
  true... but there are some differences because the transformation is
  done at the start of the process not at the end.
- This profile use the current color space primaries (except if "use
  output profile instead of working" is enabled), and apply a gamma and
  a TRC on the RGB data placed after the RGB conversion and the white
  balance for each canal Red, Green and Blue. That leads to the
  modification of the RGB values that is taken into account in the
  processes that use these data, and of the L\*a\*b\* values that are
  deduced of it.
- You can act - in an absolute way - on the gamma from 0.5 to 3.0, and
  on the slope from 0.0 to 20.0 and thus handle specific renderings.
  - Linear : gamma=1.0
  - BT-709 : gamma=2.22 slope=4.5
  - sRGB: gamma=2.4 slope=12.92
  - Adobe : gamma=2.2 slope=0.0
  - Prophoto : gamma=1.8 slope=0.0
  - and so on.
- All the no-raw processes are concerned (dito Relative ICC).
- If you enable "use output profile instead of working", the output
  profile will be applied - The gamma and slope sliders are disabled -
  (keep in mind that only the option "Output profile" with Output gamma
  = default and Free gamma disabled is taken into account).

===Standard (Absolute ICC g=1 + end)===

- At the very beginning of the RGB process, we subtract the original
  gamma ("gamma sRGB") from the data.
- then we apply a ICC profile, at the end of the RawTherapee process,
  immediately before the conversion towards TIFF/JPG.
- if you choose a gamma=2.40 and slope=12.92, this leads to have the
  whole process RGB + L\*a\*b\* treated in linear mode without gamma).
  Note: We can achieve the same result with "RGB + L\*a\*b\*
  differential (ICC)" and gamma=0.417 slope=12.92.
- all the no-raw processes are concerned (dito Relative ICC)
- same options and remarks as "Absolute ICC".

### RGB differential (Only gamma)

- Apply a gamma, over the current parameters for one part of the RGB
  process, then at the end of the process, apply an inverse gamma
- creates a differentiated action inside a part of the process (RGB
  here) in order to boost or attenuate the actions in relation with
  luminance, without globally alter the processing.
- I set up this mode partially for pedagogical reasons to show out the
  difference between applying a gamma and its inverse, with the
  following method (RGB + L\*a\*b\* differential ICC). The method "RGB
  differential Only gamma" can introduce artefacts when acting a lot on
  the sliders.
- If all the RGB process settings are at null (or neutral) or the curves
  disabled, this dual conversion has no effect. On the other hand, if a
  setting is enabled, it will be boosted or attenuated according to the
  chosen settings for gamma and slope
- You can act - in an differential way - on the gamma from 0.5 to 3.0,
  (reduced values about 0.7 et 2.5 are advised in order to limit the
  artefacts) and on the slope.
- following processes are concerned: Exposure, Channel Mixer, HSV
  equalizer, RGB Curves, Color Toning

### RGB + L\*a\*b\* differential (ICC)

- Apply a ICC profile, over current parameters and apply an inverse ICC
  profile at the end of the RawTherapee main precess, immediately before
  the conversion towards the output profile (output TIFF, JPG,...)
- creates a differentiated action inside a part of the process (RGB +
  L\*a\*b\* here) in order to boost or attenuate the actions in relation
  with luminance, without globally alter the processing.
- This profile uses the primaries of the current color space and apply a
  gamma and a TRC on the RGB data placed after the RGB conversion and
  the white balance, for each canal Red, Green and Blue and an inverse
  system at the end of the main process. That leads to an alteration of
  the RGB and L\*a\*b\* values that is taken into account in the
  processes that use these data.
- This process has the drawback, compared to "RGB differential", to
  increase a lot the processing time, but in the extremes cases it will
  produce less artefacts.
- If all the RGB and L\*a\*b processes settings are at null (or neutral)
  or the curves disabled, this dual conversion has no effect. On the
  other hand, if a setting is enabled, it will be boosted or attenuated
  according to the chosen settings for gamma and slope
- So, we will be able to heavily differentiate the image processing
  according to the chosen gamma, to boost a processing for the shadows
  or the highlights, and so on.
- all the no-raw processes are concerned (dito Relative ICC)
