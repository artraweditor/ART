---
title: Adding Support for New Raw Formats
contributors:
  - DrSlony
tags:
  - 'General Information'
toc: true
---

## Introduction

Supporting a raw format requires the following:

- Being able to decode the raw file, so that the program has access to
  the image data and metadata stored within each raw file. This is
  handled either by a program embedded inside RawTherapee called
  "dcraw", or by custom code.
- Being able to interpret the image data. This can be further broken
  down into:
  - Measuring the "white levels" (and sometimes the "black levels") of
    the decoded image data, as every camera's idea of what is the
    whitest white (and sometimes the blackest black) is different. White
    levels are measured using photos called "white frames".
  - Determining where the actual image lies on the raw canvas - the "raw
    crop".
  - Creating an "input profile" to accurately reproduce the colors.

You will have to shoot the photos, but you do not heave to understand or
carry out the measurement - we can do that for you.

### Storing and Reading This Information

RawTherapee looks for information regarding how to interpret the image
data (the black and white levels, the color matrix, and some other
details, but not the input profile) in three places:

- In the dcraw code which is embedded inside RawTherapee,
- In the raw file itself,
- In a text file on your system which is installed with RawTherapee
  called `camconst.json`

Information is gathered from all three places, and values from
`camconst.json` are prioritized above those from other sources. There is
an exception for the [input color matrix](color_management#input-profile), in that if the raw
file is in the DNG format and the `Software` Exif tag (`0x0131`) does
not begin with the string `Adobe DNG Converter` and the file does
contain a `ColorMatrix2` tag, then the value from this tag is
prioritized.

If you made any changes to `camconst.json` while RawTherapee was
running, restart it for the changes to take effect.

## White Levels

What are black and white levels? A sensor is made of millions of tiny
photo-sensitive elements called photosites or sensels. Each one measures
the intensity of the light which falls upon it, and records that
intensity as a number - the more light, the higher the number. The bulk
of the raw file consists of these recorded measurements. Each photosite
has a level below which it cannot sense any light - there might be some
light falling upon it, but this light is too weak to register a signal.
This is called the black level, and it is not always 0. There is also a
level beyond which the photosite will not register a change in light
intensity even if the light does keep getting brighter - this is the
white level. A photosite which cannot record any brighter light is said
to be fully saturated, and in post-processing this state is called
clipping.

The white levels are measured based on completely overexposed photos
called "white frames".

Some camera models use the same white and black levels regardless of
other settings, while for other models these levels depend on other
factors, such as ISO sensitivity. We need photos from across the whole
range of ISO values to determine this.

Some cameras have built-in noise reduction, often called LENR - Long
Exposure Noise Reduction. It could affect the white and black levels. If
you enable it, typically it only kicks-in when the exposure time is over
1s. The steps explained further on will explain when to turn it on and
off.

The white levels for some camera models change depending on the
aperture, but generally this only happens for wide-open apertures. To
avoid this being a problem, set the aperture to f/5.6 or higher unless
instructed otherwise.

For more documentation detailing the required photos and instructions
how to measure them, read the comments inside the `camconst.json` file:
<https://github.com/Rawtherapee/RawTherapee/blob/dev/rtengine/camconst.json>

### How to Shoot White Frames

Shoot raw photos in manual (M) mode. If your camera has several raw
modes, use the full one, uncropped, lossless compression if possible.

Each photo must be completely overexposed all across the frame. As such,
it does not matter what you shoot since everything will be white anyway,
but its easiest to achieve this while pointing the camera at the sky or
at a white light bulb. It does not matter whether the sky is sunny or
overcast, but don't point it at the sun as you might damage the sensor.

It does not matter what lens you use, but it will be easier to make the
whole image overexposed if you do not use a wide angle lens.

### Needed Photo Sets

We have broken down the image requirements into three sets, where each
subsequent set would improve the quality of support, but would also
require more effort from you and from us. In most cases, you only need
to photograph the first two sets. All sets involve photographing
completely overexposed white frames.

The sets:

1.  If your camera has LENR, turn it OFF. Set the aperture to f/5.6 or
    higher. Take a series of photos, one photo for every ISO value your
    camera supports, making sure not to exceed an exposure time of 1
    second. As an example, you could end up with about 8 photos for ISO
    100, 200, 400, 800, 1600, 3200, 6400 and 12800. Most cameras include
    intermediate ISO values, e.g. ISO160 or ISO320, so if you wanted to
    improve white level accuracy you would need to photograph these
    intermediate values as well.
2.  If your camera has LENR, turn it ON. Take a second series of photos,
    as described above but this time making sure that the exposure time
    in all cases is at least 2 seconds, not less. That's another 8 or
    more photos. In most cases these two sets should be enough.
3.  Some cameras scale raw values for larger apertures, particularly
    Canon and Nikon models. The only way to know whether your camera
    does this for sure is to take a photo and measure it. Take one photo
    as described above but using your lens's widest aperture, e.g.
    f/1.7, at ISO100 with LENR turned OFF, and send it to us along with
    the rest of the shots. If we detect that there is raw scaling (or if
    you detect it yourself if you do your own measurements) then we will
    ask you shoot a series of photos using an exposure time of less than
    1 second, from the widest aperture your lens supports down every 1/3
    of a stop until such an aperture where raw scaling is no longer
    performed. This could mean many photos. Handling raw scaling caused
    by large apertures is not very important so don't feel daunted by
    it, you don't need to do it even if your camera does do raw scaling,
    but if you have the time and bandwidth then it would be better to
    check for it.

At the very least, you should end up with a series of about 8 photos
from point 1. It is recommended that you take photos for both points 1
and 2, leading to 16 or more photos, plus the one raw scaling test photo
from point 3. If it is found that your camera performs raw scaling, you
could additionally take the needed series described in point 3, but
since this could potentially mean many photos (over 50) it is not
expected.

Compress all these photos, upload them to
[filebin.net](http://filebin.net/) and send us the full link either
through our [GitHub](https://github.com/Beep6581/RawTherapee/issues/new)
page or in the [Forum](http://rawtherapee.com/forum).

Completely clipped photos can have amazing compression, don't forget to
compress them (7-Zip, ZIP, bzip2, whatever) before uploading! As an
example, 10 completely clipped Sony 7M2 raw files with LENR disabled
weigh 234MB but if you ZIP them you get a 1MB file.

### Renaming

In order to simplify working with these white frame images, the
filenames should segregate the photos by LENR, aperture and ISO.
ExifTool can rename them automatically:

`exiftool '-FileName<${make}_${model}_${LongExposureNoiseReduction}_${aperture}_${iso}%-c.%le' dir`

## Raw Crop

The raw crop can be determined from any photograph, no extra photos are
needed.

## Input Profile

An input profile is required in order to reproduce colors accurately.
One is needed per camera model. Read the "[How to Create DCP Color Profiles](how_to_create_dcp_color_profiles)" article to learn
about the types of input profiles and how to shoot photos of a color
target so that we may create an input profile for your camera model.
