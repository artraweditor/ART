---
title: Demosaicing
contributors:
  - DrSlony
  - Ingo
  - XavAL
tags:
  - 'Tool Description'
  - 'Raw Tab'
toc: true
---

![](/imags/Chipincamera.jpg "Chipincamera.jpg")
![](/imags/Bayer_pattern_on_sensor.svg "Bayer_pattern_on_sensor.svg")
![](/imags/Bayer_pattern_on_sensor_profile.svg "Bayer_pattern_on_sensor_profile.svg")

## Introduction

Most digital cameras use a sensor which contains millions of homogeneous
light-sensitive elements, called sensels or photosites. In order to
capture color, a [color filter array](https://en.wikipedia.org/wiki/Color_filter_array) (CFA) is placed
over the sensor, so that specific photosites register specific
wavelengths of light. The "[Bayer filter](https://en.wikipedia.org/wiki/Bayer_filter)" is the most
common - it uses a repetitive 2x2 matrix of green, blue, red and green
patches. It is used by most camera manufacturers. There is also a filter
arrangement called
"[X-Trans](https://en.wikipedia.org/wiki/Fujifilm_X-Trans_sensor)", used
by some Fujifilm cameras, with a repetitive 6x6 matrix of patches. As
each photosite is responsible for capturing only a specific band of
light, there are three main problems that need to be dealt with:

1.  There is no concept of color yet, as each photosite registers only a
    single electric charge induced by the photons which pass through the
    filter and strike it,
2.  There are twice as many green photosites as either red or blue,
3.  and thus half (green) or three-fourths (red, blue) of each color
    channel consist of a lack of data (black, unexposed photosites,
    since there is for example only one red-filtered photosite out of
    every four).

Displaying an image from a camera with a Bayer or X-Trans sensor is
therefore not straight-forward - the mosaic of discrete data points need
to be converted into a cogent color image. This process is called
**demosaicing**.

RawTherapee offers several demosaicing algorithms, each with its own
characteristics. The differences between them can be subtle - one might
need to zoom in to 100% or more to discern them. However, as the
demosaiced image constitutes the foundation upon which all other tools
work, the choice of demosaicing algorithm can have a visually
significant effect on the end result, particularly when viewed up close.
The most visible effects of the choice of demosaicing algorithm include
the rendering quality of fine detail and the visibility of artifacts in
the form of maze-like patterns.

Concerning Bayer cameras, AMaZE is generally the best method for
lower-ISO images, while LMMSE or IGV are better for higher-ISO ones.
Concerning X-Trans cameras, 3-pass (Markesteijn) is generally the best
method.

On a side note, the [Foveon X3 sensor](https://en.wikipedia.org/wiki/Foveon_X3_sensor) does not use a
color filter array and so images coming from camera with such a sensor
do not need to be demosaiced. They are, however, unsupported by
RawTherapee.

<div align="center">

Image:Demosaicing city1 amaze.png\|AMaZE Image:Demosaicing city1
igv.png\|IGV Image:Demosaicing city1 lmmse.png\|LMMSE Image:Demosaicing
city1 eahd.png\|EAHD Image:Demosaicing city1 hphd.png\|HPHD
Image:Demosaicing city1 vng4.png\|VNG4 Image:Demosaicing city1
dcb.png\|DCB Image:Demosaicing city1 ahd.png\|AHD Image:Demosaicing
city1 fast.png\|Fast Image:Demosaicing city1 none.png\|None

</div>

## Demosaicing Methods

- Common methods:
  Fast
  Very fast but simple and low quality demosaicing method, not
  recommended.

  Mono
  Only useful for users of monochrome cameras, or cameras with the color
  filter array removed.

  None
  No demosaicing is performed. This can be useful for diagnostics, but
  you would not use it for photography.
- Bayer methods:
  AMaZE
  AMaZE (Aliasing Minimization and Zipper Elimination) is the default
  demosaicing method, as it yields the best results in most cases. In
  RawTherapee versions 2.4 and older VNG4 used to be the preferred
  algorithm for Olympus cameras, as AMaZE didn't exist yet and VNG4
  eliminated certain maze pattern artifacts that might have been created
  by the other methods, but with the introduction of the AMaZE method in
  RawTherapee 3.0, Olympus users might prefer that instead.

  RCD
  RCD ([Ratio Corrected Demosaicing](https://github.com/LuisSR/RCD-Demosaicing)) does an
  excellent job for round edges, for example stars in astrophotography,
  while preserving almost the same level of detail as AMaZE.

  DCB
  [DCB](https://discuss.pixls.us/t/diagonal-interpolation-correction-artifacts-with-amaze-demosaicing/3338/45?u=morgan_hardwood)
  produces similar results to AMaZE. AMaZE can often be slightly
  superior in recovering details, while DCB can be better at avoiding
  false colors especially in images from cameras without anti-aliasing
  filters.

  LMMSE and IGV
  These are recommended when working with very noisy, high ISO images,
  in conjunction with the [Noise Reduction](noise_reduction)
  tool. They will prevent false maze patterns from appearing, and
  prevent the image from looking washed-out due to heavy noise
  reduction. IGV is also quite effective at mitigating moiré patterns.

  AHD, EAHD and HPHD
  AHD (Adaptive Homogeneity-Directed), EAHD (Horvath's AHD) and HPHD
  (Heterogeneity-Projection Hard-Decision) are old methods which are
  generally slow and inferior to the other methods.

  VNG4
  If you use a medium format technical camera with near-symmetrical wide
  angle lenses such as the Schneider Digitar 28mm or 35mm it's likely
  that the image captured by your sensor will contain some crosstalk
  between photosites, especially if the lens is shifted (due to the low
  angle of incoming light from these lenses some light leaks over to the
  next pixel on the sensor), and in this case you can get mazing
  artifacts with AMaZE and DCB because of the green channel separation
  caused by the crosstalk. If you combine a mirrorless camera using an
  adapter with a wide angle lens designed for film, you may also get
  crosstalk. It can then be better to use the VNG4 algorithm (Variable
  Number of Gradients), which handles this situation well, at the cost
  of some fine detail. An alternative is to enable [green  equilibration](preprocessing#green_equilibration) to
  even-out the green channel differences.

  Pixel Shift
  Some Pentax and Sony cameras support shooting in Pixel Shift mode,
  which shoots four frames with the sensor offset one pixel at a time in
  a circular direction, and then stores all four frames in one large raw
  file. RawTherapee can combine all frames into one image while
  automatically masking-out moving objects, thereby reducing the level
  of noise and increasing the image sharpness.
- Fujifilm X-Trans methods:
  3-Pass
  For Fujifilm cameras with X-Trans sensors. It runs three passes over
  the image which leads to sharper results though you can only see this
  on low ISO photos. It is slower than 1-Pass.

  1-Pass
  For Fujifilm cameras with X-Trans sensors. It is faster than the
  3-pass method but slightly inferior in quality, though this difference
  is only visible in low ISO shots. If speed is an issue, you can use
  this method on high ISO shots with no visual difference in quality.

## How to Find the Best Demosaicing Method

[thumb](image:demosaicing_city1_example_bad.jpg)

Zoom in to at least 100% (1:1) and try all the demosaicing algorithms,
see which works best for you. Try them on sharp photos with fine detail
and tiny patterns, such as the wavy and repetitive fabric of a sweater
(watch for maze pattern artifacts), a distant brick wall, a distant
round road sign (watch for aliasing along the round edges), and test
with both low and high ISO shots. Use photos from your own camera;
what's best for Nikon raw images may not be what's best for Olympus
ones.

## Monochrome Cameras

A monochrome camera has a homogeneous filter in front of the sensor -
you get a black-and-white image, and no demosaicing is required. Some of
these cameras have no infrared filter and are thus sensitive to infrared
light, which can be used for creative black and white photography.

RawTherapee supports monochrome cameras, but the user interface is not
specifically adapted for it, so when you load a monochrome file all
color tools will still be available.

There are a few additional factors to consider when working with
monochrome files: some monochrome cameras report that they have only a
single monochrome channel and a neutral color matrix (such as the Leica
M Monochrom), while others report RGB channels in a Bayer configuration
(such as the Phase One IQ260 Achromatic, or IR-modified cameras). If the
camera reports only one channel, RawTherapee recognizes this and won't
perform any demosaicing (the demosaicer selection is still enabled but
does not do anything), and everything works normally. However, if the
camera identifies as an RGB Bayer camera, demosaicing will be performed
and a color matrix will be applied. To disable this, you should select
the "Mono" demosaicing method, and select "No profile" as [input profile](color_management#input_profile) in the Color
Management panel.

## Interface

The demosaicing methods and their associated settings are separated into
two main tools, "Sensor with Bayer Matrix" and "Sensor with X-Trans
Matrix", each of which is visible when editing a raw file which
originates from a camera which uses the given filter matrix. The
settings in one tool have no influence over the settings in the other -
if you open a raw image from a Bayer-type sensor, only the settings from
the "Sensor with Bayer Matrix" tool will be used, the settings from the
"Sensor with X-Trans Matrix" tool will be ignored, and vice versa.

### Method

The following demosaicing algorithms are available for raw files from
Bayer sensors:

- AMaZE
- AMaZE+VNG4
- RCD
- RCD+VNG4
- DCB
- DCB+VNG4
- LMMSE
- IGV
- AHD
- EAHD
- HPHD
- VNG4
- Fast
- Mono
- Pixel Shift
- None

The following demosaicing algorithms are available for raw files from
X-Trans sensors:

- 3-pass+fast
- 3-pass (Markesteijn)
- 1-Pass+fast
- 1-Pass (Markesteijn)
- Fast
- Mono
- None

#### Dual Demosaic

The dual-demosaic methods, such as AMaZE+VNG4, allow you to demosaic
areas of high contrast (i.e. detail) using one method and areas of low
contrast (i.e. no detail, plain areas such as sky) using the other
algorithm. As some algorithms are better at rendering fine detail while
others are better at rendering smooth, plain areas, these dual-demosaic
options allow you to combine the best of both worlds, this providing a
better starting point for the other tools and mitigating the need for
sharpening or noise reduction. The downside is that the image needs to
be demosaiced twice, thus taking longer than using a single demosaicing
method.

The threshold for adjusting the level of detail at which one demosaicing
algorithms should be used over the other is controlled using the
"contrast threshold" slider. The slider includes a checkbox which
computes an optimal level automatically.

### Border

Demosaicing may rely on sampling the pixels which surround a given
pixel. When demosaicing a pixel which lies at the top edge of the raw
image, there are no pixels above it, thus it cannot be demosaiced in the
same way and at the same quality as those pixels which are surrounded by
a sufficiently large number of pixels. Most raw converters thus crop off
a few rows and columns from around the image periphery, as does
RawTherapee by default. However, you can override this cropping by
manipulating the "border" value. Setting it to "0" means no cropping
occurs, and RawTherapee will do what it can to demosaic the border
pixels, though artifacts may appear!

You should generally leave this setting at its default value, and change
it to 0 only when absolutely needed, for example when processing
[1080p](https://en.wikipedia.org/wiki/1080p)raw DNG frames where you
need to preserve the 1920x1080 pixel count.

### Sub-Image

Some raw files contain more than one image. When editing such an image,
the sub-image option appears, and allows you to edit a specific
sub-image, or to combine the sub-images in one way or another.

Some Fujifilm EXR cameras support "SN mode" at capture time, which
stands for "Signal to Noise priority". When editing such an image, the
sub-image combobox allows you to select "SN mode", which blends pixels
from both sub-images using a mean average, leading to less noise.

### False Color Suppression Steps

Sets the number of median filter passes applied to suppress demosaicing
artifacts when applying the demosaicing algorithm. False colors
(speckles) could be introduced during the demosaicing phase where very
fine detail is resolved. False color suppression is similar to color
smoothing. The luminance channel is not affected by this suppression.

False colors are generally more apparent in images from cameras without
anti-aliasing filters. Note that it is foremost the selected demosaicing
algorithm which is the deciding factor in how prominent will be the
false color problem with which you will have to deal. In some situations
it may be better to change the demosaicing algorithm than to enable
false color suppression, as the latter reduces color resolution.
