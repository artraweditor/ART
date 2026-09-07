---
title: Contrast by Detail Levels
contributors:
  - DrSlony
  - XavAL
tags:
  - 'Tool Description'
  - 'Detail Tab'
toc: true
---

*Contrast by Detail Levels* uses wavelet decomposition to decompose the
image into six levels, each adjusted by a slider. Slider 0 (Finest) has
a pixel radius of 1, sliders 1 to 5 have a pixel radius of approximately
2, 4, 8, 16 and 32 pixels. Giving a slider a value less than 1.0
decreases local contrast at that level, while giving it a higher value
increases it. Thus you can use it to increase perceived sharpness of an
image, to increase local contrast, or to mitigate certain levels of
detail.

You should remember that resizing an image has a direct impact on
perceived sharpness, as does viewing distance. In practical terms this
means that you should use this tool while zoomed more or less to a level
representative of your intended final image size and viewing distance,
so if you intend to print the high resolution image on a 90x60cm canvas
and admire it from 30cm away then it makes sense to zoom in to 100% and
tweak the "0 (Finest)" slider. However, in real life such large prints
usually hang on the wall and are appreciated from the couch a few meters
away - from there the finest detail level setting will have no effect
whatsoever - your eyes cannot make out the detail from that distance.
The same goes for images you intend to resize (downscale) for use on the
internet or email to friends or clients - not only do you lower the
resolution by downscaling, but they will also be viewed on low
resolution devices, probably not even fullscreen, e.g. on a laptop,
tablet or phone. In this case too playing with the "0 (Finest)" detail
level will make no difference to the end result. Most of the time only
sliders "3" and "4" will have a practically useful effect.

![](rt_cbdl_100.jpg "rt_cbdl_100.jpg")
 
For example, in order to remove skin blemishes
while retaining skin texture on this 10 megapixel photo, where your
intention is to view it at its full size and from up close (e.g. art
gallery), zoom to 100% and start by setting the sliders as follows, then
fine-tune to taste:

`0 (Finest)  : 1.4`
`1           : 1.4`
`2           : 0.4`
`3           : 0.4`
`4 (Coarsest): 1.2`
`Skin Tones Targetting/Protection: -75`

![](rt_cbdl_25_1.jpg "rt_cbdl_25_1.jpg")

If you want to downscale the photo to use it
on the web, you should zoom out to about 25% which is more or less the
size at which the image will be viewed. Using the settings above we can
still see that the skin is smoothed, but if you reset the first three
sliders to "1.0", you will see doing so made no difference! The reason
is that at this smaller resolution, changes to those levels are lost in
the downscaling process. The result would be identical if you saved the
full-sized image and then downscaled it to 25% afterwards in some other
program, so don't think this is RawTherapee's shortcoming - this is
simply how sharpness (resolution and
[acutance](https://en.wikipedia.org/wiki/Acutance)) work. Knowing this,
if you only intend to share the downscaled version of your photo, you
can save yourself time by simply ignoring the first three sliders.

![](rt_cbdl_25_2.jpg "rt_cbdl_25_2.jpg")

Furthermore, you may find when you zoom out
that the sliders had effects which were not immediately apparent at 100%
zoom. For example you may find that using the "4 (Coarsest)" slider can
pleasantly mitigate large, harsh shadows, so set it to 0.5 and fine-tune
to taste.

## Process Locate Before/After *Black-and-White*

This combo-box lets you decide when in the pipeline the CbDL tool will
run. This tool had been added to RawTherapee long ago, and more recently
the Black-and-White tool was added, placed before CbDL in the pipeline.
An unforeseen result was that if you enabled the B&W tool then you could
not use CbDL's Skin Targetting/Protection because a black-and-white
image has no skin color information. This combo-box was added to remedy
the situation. Running the CbDL tool before the B&W tool lets you target
skin tones before conversion to black-and-white. We recommend you stay
with the default option, "Before Black-and-White".

## Contrast +/- and Neutral

Use the "*Contrast-*" button to move all five sliders by preset amounts
to the left (noise reduction). Use the "*Contrast+*" button to move all
five sliders by preset amounts to the right (sharpening). Use the
"*Neutral*" button to reset all sliders to 0.

Feel free to move individual sliders as well, and inspect the results in
the detail window; you may want to zoom in to 200% or more to see better
what this filter does.

For high ISO shots (1600+), try for example clicking on the
"*Contrast-*" button twice and using [Sharpening](sharpening)
\> [Unsharp Mask](sharpening#unsharp_mask) with an amount of
80.

## Threshold

The "*Threshold*" parameter is used to prevent the sharpening of noise:
if a pixel's luminance differs only a bit from its neighbors (the
difference is less than the threshold), then it is not sharpened. You
can set the threshold also to 0 but then everything will be sharpened
(even the noise).
