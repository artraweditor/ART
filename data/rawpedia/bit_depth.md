---
title: Bit Depth
contributors:
  - DrSlony
tags:
  - 'General Information'
toc: true
---

## Introduction

You will hear terms such as "8-bit", "16-bit", "24-bit", "32-bit",
"64-bit" and "96-bit" with reference to digital images. This article
will clarify what those things mean.

Digital images consist of millions of pixels, and each pixel describes
one or more color channels. Grayscale images need only one channel (a
value of 0 could represent pure black, 255 could represent pure white,
and the values in-between would then represent shades between black and
white), while RGB color images need three channels - one describes red,
one green and one blue. Each channel describes only an intensity, so
there is nothing inherently green about a number which describes a pixel
from the green channel; colors derive from the interaction between all
three channels in the [RGB color model](https://en.wikipedia.org/wiki/RGB_color_model).

A single pixel could represent more than three channels, for example it
could contain information about an alpha channel (which describes
transparency) or an infra-red channel (which some scanners support).

The higher the bit depth, the more precisely a color can be described,
at a cost of requiring longer computation, more RAM and more storage
space.

## Bits Per What?

Bit depth is expressed as a value which describes either the number of
**bits per pixel** (BPP), or **bits per channel** (BPC). The very
popular [JPEG format](https://en.wikipedia.org/wiki/JPEG) typically
saves images with a precision of 8 bits per channel, using three
channels, for a total of 24 bits per pixel. The [TIFF format](https://en.wikipedia.org/wiki/TIFF) supports various bit depths,
for example 32 bits per channel for a total of 96 bits per pixel.

When describing bit depth, state what you're describing to leave no room
for ambiguity. For example, if someone says they have a "32-bit" image,
does that mean the image has 32 bits per channel, or does it have 4
channels at 8 bits per channel?

## Precision

What difference does bit depth make? The more bits are available to
describe a color, the more precisely you can describe that color.

- A precision of 1 bit per channel means that there is only 1 bit to
  describe the value. A bit can only be 0 or 1, so you can only
  represent two values, which typically would mean black or white.
- A precision of 2 bits per channel means there are two bits available
  to describe a color. Since each bit can be 0 or 1, and there are two
  of them, they can represent 4 possible values:
      [00] = 0
      [01] = 1
      [10] = 2
      [11] = 3

  If we use 0 to represent black and 3 to represent white, there are two
  additional shades of gray which can be described.
- A precision of 8 bits per channel means there are 8 bits which can
  represent 256 values:
      [0000 0000] = 0
      [0000 0001] = 1
      (...)
      [1111 1110] = 254
      [1111 1111] = 255

  If we use 0 to represent black and 255 to represent white, 254 shades
  of gray can also be described. This is what JPEG files use - 8 bits
  per channel, with 3 channels. It is sufficient to be used for most
  ready-to-view photographs in the [sRGB color space](https://en.wikipedia.org/wiki/sRGB) without visible
  [posterization](https://en.wikipedia.org/wiki/Posterization), so you
  can use it when saving photographs ready to be viewed over the
  internet. It is not suitable as an intermediate format nor as a final
  format if there is a chance you might need to tweak the photograph
  later on, as you run the risk of introducing posterization artifacts,
  depending on the strength of adjustments. 8-bit precision is not
  enough to represent a high dynamic range scene in a linear way without
  posterization, i.e. you theoretically could use 8 bits of precision to
  describe a high dynamic range scene linearly, but the numbers would be
  so far apart that heavy posterization would occur. For instance, if a
  photograph captures a sunny day in the park, and if we assume that
  black should be 0 and white should be 1 000 000, we could map 0 to 0
  and 255 to 1 000 000, but then there would only be 254 values left for
  describing all the remaining 999 999 shades of the original scene.
- A precision of 16 bits per channel (16-bit integer) means there are 16
  bits which can represent 65536 values:
      [0000 0000 0000 0000] = 0
      [0000 0000 0000 0001] = 1
      (...)
      [1111 1111 1111 1110] = 65534
      [1111 1111 1111 1111] = 65535

  Digital cameras typically capture light in 12-bit or 14-bit precision
  (and due to noise and imprecise electronics the lowest bits are of
  dubious quality). 16 bits per channel are enough for most photography
  needs, including for use in intermediate files (if you want to pass an
  image from one program to another without data loss).
- The values of a 16-bit [floating point](https://en.wikipedia.org/wiki/Floating-point_arithmetic) image,
  also known as [half-precision floating point](https://en.wikipedia.org/wiki/Half-precision_floating-point_format),
  are spread in a way more suitable to sampling light than in 16-bit
  integer. This is so for various reasons: human vision is more
  sensitive to small changes in dark tones than to small changes in
  bright ones; our eyes respond to light in a logarithmic way (light
  must be 10 times more intense in order for us to see it as twice as
  bright); and specular highlights which can be the the brightest
  elements in a scene (the sun reflecting off a door knob) need not be
  described as accurately as all the other tones. In 16-bit
  floating-point notation, values are distributed more closely in the
  (lower) darker tones than in the (higher) lighter ones, thus allowing
  for a more accurate description of the tones more significant to us.
- A 32-bit [floating point](https://en.wikipedia.org/wiki/Floating-point_arithmetic) image
  can represent 4.3 billion values per channel, and requires roughly
  twice the disk space as a 16-bit image. Few programs support 32-bit
  images.

One of the reasons bit depth affects mostly shadows is due to the way
colors are stored. Each color is defined by a mixture of red, green and
blue. Using an 8-bit image and the color orange as an example, many
values are possible when describing bright orange, but the number of
samples available to describe dark orange drops to very few, i.e. only
the lowest 3-4 bits from each channel can be used to describe dark
orange, which means only 16 possibilities exist. The higher the
bit-depth, the more colors can be described, and posterization avoided.

## Gamma Encoding

[Gamma encoding](https://en.wikipedia.org/wiki/Gamma_correction) can be
used when saving image files, meaning that values are modified in such a
way that more can be allocated in the shadow range than in the highlight
range, which better matches the human eye's sensitivity. This means that
an 8 bit JPEG can display as much as log2((1/2^8)^2.2) = 17.6 stops of
dynamic range, which indeed exceeds the 14 stops of the current best
cameras, which explains why you sometimes can see a camera's shadow
noise even in an 8-bit JPEG. However, due to the non-linear
distribution, we lose precision compared to the raw file recorded in a
linear way by the digital camera. Practically this is not a problem when
the output file is the definitive one and will not be processed anymore,
however a photo can be vastly improved when saved as raw data and
processed using a state of the art raw processing program, such as yours
truly - RawTherapee.

## After RawTherapee

Once you have adjusted a photo in RawTherapee and are ready to
[save](/saving_images), you are faced with a choice of
[output format, per-channel bit depth](/color_management#output-profile),
[color space and gamma encoding](/color_management_addon#output-space-output-profile).
If you plan to post-process your photos after RawTherapee in a
16-bit-capable image editing program, it is better to save them in a
lossless 16-bit format. RawTherapee can save images in 16-bit integer
precision (denoted as "TIFF (16-bit)" in the Save dialog) as well as
16-bit floating-point precision (denoted as "TIFF (16-bit float)").
Uncompressed TIFF at 16-bit integer precision is suggested as an
intermediate format as it is the fastest to save and is widely
compatible with other software. 32-bit files are roughly twice the size
and not well supported by other programs.
