---
title: RGB and Lab
contributors:
  - DrSlony
tags:
  - 'General Information'
---


*[RGB](https://en.wikipedia.org/wiki/RGB_color_space)* and *[CIE L\*a\*b\*](https://en.wikipedia.org/wiki/Lab_color_space)* (or just
"*Lab*") are two different [color spaces](https://en.wikipedia.org/wiki/Color_space), or ways of
describing colors.

Many people wonder what the differences are between adjusting lightness,
contrast and saturation in the RGB color space, or lightness, contrast
and chromaticity in the Lab color space. RGB operates on three channels:
red, green and blue. Lab is a conversion of the same information to a
lightness component L\*, and two color components - a\* and b\*.
Lightness is kept separate from color, so that you can adjust one
without affecting the other. "Lightness" is designed to approximate
human vision, which is very sensitive to green but less to blue. If you
brighten in Lab space, the result will often look more correct to the
eye, color-wise. In general we can say that when using positive values
for the saturation slider in Lab space, the colors come out more
'fresh', while using the same amount of saturation in RGB makes colors
look 'warmer'.

<div align="center">

![](colorspace_flowers_900_1_neutral.jpg)
Neutral

![](colorspace_flowers_900_2_rgb_lightness.jpg)
RGB Lightness +30

![](colorspace_flowers_900_3_lab_lightness.jpg)
Lab Lightness +30

![](colorspace_flowers_900_4_rgb_contrast.jpg)
RGB Contrast +45

![](colorspace_flowers_900_5_lab_contrast.jpg)
 Lab Contrast +45

![](colorspace_flowers_900_6_rgb_saturation.jpg)
RGB Saturation +25

![](colorspace_flowers_900_7_lab_chromaticity.jpg)
Lab Chromaticity +25

![](colorspace_flowers_900_8_vibrance.jpg)
Vibrance +25

</div>

The difference between the *Lightness* slider in the
*[Exposure](exposure)* section (in RGB space) and the
*Lightness* slider in the *[Lab](lab_adjustments)* section is
subtle. A RGB *Lightness* setting of +30 produces an image that is
overall a bit brighter than when using a Lab *Lightness* setting of +30.
The colors in Lab *Lightness* are somewhat more saturated. The contrary
is true for the Contrast sliders; when using a RGB *Contrast* of +45 the
colors will be clearly warmer than when using a Lab *Contrast* of +45.
The contrast itself is about the same with the two settings. Do not
hesitate to use both sliders to adjust saturation and/or contrast. As
for the *Saturation*/*Chromaticity* sliders, setting the RGB
*Saturation* slider to -100 renders a black and white image which
appears to have a red filter applied, while the Lab *Chromaticity*
slider renders a more neutral black and white image. Positive RGB
*Saturation* values will lead to hue shifts (the larger the value, the
more visible the shift), while positive Lab *Chromaticity* values will
boost colors while keeping their hues correct, rendering a crisp and
clean result. Lab chromaticity (via the "*Chromaticity*" slider or
"*CC*" curve) is the recommended method for boosting colors.
