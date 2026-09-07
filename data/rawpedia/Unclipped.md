---
title: Unclipped
contributors:
  - DrSlony
  - Lebarhon
tags:
  - 'Bundled Processing Profiles'
toc: true
---

## Introduction

The "Unclipped" [processing profile](sidecar_files_-_processing_profiles) (introduced in
RawTherapee 5.6) allows one to save an image in a way which preserves
data across the whole tonal range, including clipped shadows and
highlights, thus allowing for strong exposure adjustments and dynamic
range compression of the saved file while retaining detail in the
shadows and highlights. This may be desired not only for scientific
purposes but also when the saved image is destined for further
manipulation and the final exposure is yet unknown, such as can be the
case when stitching panoramas which require exposure adjustments so that
images seamlessly blend into one another.

<div align="center">

![](batcat.jpg).

This is a raw file of a scene with a very high
dynamic range. Notice that the histogram shows clipped shadows and
highlights. 

![](batcat.jpg). 

The raw file does however contain data
across the whole dynamic range, including those shadows and highlights
which appear clipped. We know this to be the case because adjusting
exposure compensation in RawTherapee reveals those details. However,
there are situations when one needs to manipulate the image as little as
possible at this point, which precludes manipulating the exposure
compensation or performing dynamic range compression.

![](batcat.jpg)

When saved to an ordinary 16-bit TIFF file, what
appeared clipped before is now really clipped - the data is lost, one
cannot recover the shadows nor the highlights from this saved TIFF file.
Here an exposure compensation of -2EV was performed in GIMP, and the
results are unusable. 

![](batcat.jpg).

 The solution is to apply this
"Unclipped" profile, as described below in the "Usage" section, and save
to a floating-point 16-bit or 32-bit TIFF file. The resulting TIFF file
contains data across the whole dynamic range, and even strong exposure
adjustments reveal perfectly intact details.

</div>

Not all tools can be used when saving an unclipped image. The following
tools or settings are known to be incompatible with saving unclipped
images in RawTherapee 5.6, and so are disabled by this processing
profile:

- [Exposure tone curves](exposure#tone_curves),
- [Vibrance](vibrance),
- Before/After curves in the
  [Black-and-White](black-and-white) tool,
- [Film Simulation](film_simulation),
- [RGB Curves](rgb_curves) in luminosity mode,
- [DCP look table](color_management#use_dcp.27s_look_table),
- [CIECAM02](ciecam02)

## Usage

1.  Apply the "Unclipped" profile. This profile disables those tools and
    settings which are incompatible with saving unclipped images. Ensure
    that your output ICC profile is either v4, or a linear tone response
    curve v2.
    - If you want to apply this profile and reset all tools to safe
      default values, then set the [processing profile fill mode](sidecar_files_-_processing_profiles#partial_processing_profiles_and_fill_modes)
      to "Filled"
      ![<File:Profile-filled.png>](/images/Profile-filled.png "File:Profile-filled.png")
      before applying this profile.
    - If you want to apply this profile while preserving existing
      adjustments, then set the processing profile fill mode to
      "Preserve"
      ![<File:Profile-partial.png>](/images/Profile-partial.png "File:Profile-partial.png")
      before applying this profile.
2.  Save the image as either a 16-bit floating-point TIFF or a 32-bit
    floating-point TIFF.
