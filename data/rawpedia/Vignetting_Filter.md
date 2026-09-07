---
title: Vignetting Filter
contributors:
  - DrSlony
tags:
  - 'Tool Description'
  - 'Exposure Tab'
toc: true
---

The vignette filter is intended for adding artistic vignetting to your
image. This vignetting filter is placed relative to the crop, if
cropping is used.

For correcting vignetting caused by the lens light fall-off (as opposed
to this filter which is not for correction but for artistic effect), use
the [Vignetting Correction](/lens--geometry/#vignetting-correction) filter in
the Transform tab, in the Lens/Geometry tool. Even better, use the
[Flat Field](flat_field) tool.

## Strength

The amount of darkening the filter will apply, in stops. The full
strength is reached in the corners of the image. If you apply a negative
amount the corners will be brightened instead of darkened.

## Feather

The feather slider controls the width of the feathering. If at 0 only
the corners will be feathered and the rest of the image will not be
affected by the filter. At 50 the feather reaches halfway to center and
the rest is unaffected, and at 100 the feather reaches all the way into
the center.

![](vignette-filter_4.00_00_50.jpg)
Feather = 0.

![](vignette-filter_4.00_99_50.jpg)
Feather = 100

## Roundness

The roundness slider controls the geometry of the filter. At 0 the shape
is rectangular (with rounded corners), at 50 it is a fitted ellipse, and
at 100 it’s circular. Note that if your image is square the fitted
ellipse will of course be a circle, so the shape will then not change in
the range 50 to 100.

![](vignette-filter_4.00_50_00.jpg)
Roundness = 0

![](vignette-filter_4.00_50_99.jpg)
Roundness = 100
