---
title: Chromatic Aberration
contributors:
  - DrSlony
  - Ingo
  - XavAL
tags:
  - 'Tool Description'
  - 'Raw Tab'
---

![](images/chromatic_aberration_auto1.jpg "chromatic_aberration_auto1.jpg")

![](chromatic_aberration_auto2.jpg "chromatic_aberration_auto2.jpg")

This "Chromatic Aberration" tool
works on the image **before** demosaicing, that's why it's located in
the *Raw* tab. The [Chromatic Aberration Correction](/lens--geometry/#chromatic-aberration-correction)
tool in the *Transform* tab works on the image **after** demosaicing.

Chromatic aberration correction on the raw level is currently only
supported for raw files from cameras with a [Bayer filter](https://en.wikipedia.org/wiki/Bayer_filter). If you need to
remove chromatic aberration from raw photos from X-Trans sensor cameras
(Fuji), then use the [Chromatic Aberration Correction](/lens--geometry/#chromatic-aberration-correction)

tool in the *Transform* tab.

Chromatic aberration can be corrected by using the "Red" and "Blue"
sliders. Normally you won't see any chromatic aberration in the
fit-to-screen preview, therefore it is highly recommended to open a
detail window
![<file:window-add.png>](window-add.png "file:window-add.png") or to
zoom the main preview in to 100%
![<file:magnifier-1to1.png>](magnifier-1to1.png "file:magnifier-1to1.png")
or more when you attempt this kind of correction.

This tools corrects bluish-green and magenta fringes due to lens lateral
chromatic aberration that show mainly in the borders of the image. This
correction is performed before demosaicing and can sometimes improve the
quality of demosaicing.

## Auto-Correction

If Auto-correction is checked, the "Red"/"Blue" sliders are disabled
and an automated detection and correction of chromatic aberration is
performed. Where manual correction applies a constant value across the
image, auto-correction divides the image into many blocks and tailors
the values required to eliminate chromatic aberration to each block. For
this reason auto-correction usually performs better than manual
correction, and the auto-correction values cannot be displayed in the
sliders.

## Iterations

This setting is available if Auto-Correction is checked. Auto-Correction
is conservative, means it often does not correct all Chromatic
Abberation. To correct the remaining Chromatic Aberration, from RawTherapee 5.5
on you can use up to 5 iterations of automatic Chromatic Aberration
correction. Each iteration will reduce the remaing Chromatic Aberration
from the last iteration at the cost of additional processing time.

## Red/Blue

If the "Red"/"Blue" sliders are non-zero the given values are used to
correct chromatic aberration. They cannot be used at the same time as
Auto-correction.
