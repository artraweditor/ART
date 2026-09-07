---
title: Film Negative
contributors:
  - DrSlony
  - XavAL
tags:
  - 'Tool Description'
  - 'Color Tab'
toc: true  
---

## Introduction

Negatives are images with reversed lightness and hues, such as those
produced by film cameras. RawTherapee 5.7 introduced the Film Negative
tool to make developing photos of negatives simple.

The tool supports non-raw photos and raw photos of a Bayer or X-Trans sensor. Other raw
types are not supported.

In the negative image, each channel value is proportional to a power of
the reciprocal of the corresponding channel in the original exposure
(see [Photographic Film - Basics](https://en.wikipedia.org/wiki/Photographic_film#Film_basics) for
more info). Each channel value is raised to a different exponent,
depending on the film type, age and possibly other factors, such as
shooting conditions. These exponents can be specified in order to better
adapt the correction process to the characteristics of each film. To
simplify manual tweaking, these three R,G,B exponents are specified as
one "reference" exponent (which gets applied to the Green channel), and
two ratios of the Red and Blue exponents to the reference. Default
values should produce reasonably good results out-of-the-box with some
common Kodak film types like the ColorPlus 200 or Gold 200.

## Usage

1.  Open a photo of a negative.
2.  In the Color tab, activate the Film Negative tool.
3.  Optionally, you can try to automatically set more accurate red and
    blue ratio values. To do so, click the "Pick neutral spots" button,
    then click on a neutral light and dark spot in the photographed
    scene.
    :\* These spots should have had no color tint in the original scene,
    they should have a neutral hue. Keep in mind that these spots might
    not appear neutral in your photographed film negative until you use
    the [White Balance](white_balance) tool.

    :\* The spots should differ in brightness, and should not be
    clipped.


    Picking the spots needs to be done only once per film roll, then the
    processing profile can be copied and pasted onto the other
    photographs from the same roll. This allows you to use any image in
    the whole roll to pick the neutral spots.
4.  White-balance the photo. Picking the white balance off a spot which
    should be neutral in hue, if the image has one, is easiest.

    Picking the neutral spots changes the raw data's values in the
    [pipeline](toolchain_pipeline) before the white balance
    tool takes effect; therefore it is recommended to white-balance the
    photo after having picked the spots. If you white-balanced it before
    picking the spots that is fine, but you might want to
    re-white-balance it again afterwards.

That's it as far as correcting the negative goes. Resume adjusting the
photo just as if it was a normal "positive" photo.

## Interface

### Reference exponent (contrast)

Exponent applied to the Green channel, and proportional to the other
exponents applied to the Red and Blue channels. Changing this value
alters the general image contrast without altering its colors. The
default value is good for an average-contrast negative image. In case of
a very faint, or incorrectly exposed negative, this value may have to be
increased. In case of a very high contrast negative, the converted,
positive image could reach clipping, so this value will need to be
decreased.

### Red ratio

Ratio of the Red channel exponent to the reference exponent. This
coefficient indicates how "bent" the Red channel transfer curve is, with
respect to the Green transfer curve. Changing this value alters the
color characteristics of the correction, while keeping the general image
contrast.

### Blue ratio

Ratio of the Blue channel exponent to the reference exponent. This
coefficient indicates how "bent" the Blue channel transfer curve is,
with respect to the Green transfer curve. Changing this value alters the
color characteristics of the correction, while keeping the general image
contrast.
