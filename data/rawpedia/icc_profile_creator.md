---
title: ICC Profile Creator
contributors:
  - DrSlony
  - Jdc
tags:
  - 'General Information'
toc: true
---

## Introduction

![](rt55_icc_profile_creator.png)

The ICC Profile Creator allows you to generate your own ICC profiles.
You can use standard presets as well as custom values.

RawTherapee can generate output, screen or custom working profiles using
the XYZ matrix generated for [Custom profiles](color_management#adding-custom-working-profiles)

You cannot adjust all the parameters of an ICC profile with this tool,
e.g. the A2B or B2A tags, but you can adjust those that are important
for a photographer i.e. the primary colors, the tone reproduction curve,
and the white point (illuminant).

You can generate profiles in accordance with ICC versions 2 and 4, and
in both cases you can use custom primaries and modify the illuminant as
well as the tone reproduction curve (TRC).

The code and the basic principles are similar to those used for
[Abstract profiles](color_management#abstract-profiles)

## Usage

Access this module via the
![](gamut-plus.png "Gamut-plus.png") ICC
Profile Creator button located either in the bottom-left or top-right of
the RawTherapee window.

To make the profiles you generate available for use in RawTherapee, save
them to the "directory containing color profiles" as specified in
Preferences > [Color Management](preferences#color-management-tab).

### Primary Colors

The three primary colors - Red, Green, Blue - are additive and form the
basis of all other colors. In the ICC profile generator, you specify the
location of the primary colors on the 1931 CIE xy chromaticity diagram
using the xy coordinates.

[CIE xy diagram](color_management#the-cie-xy-diagram)

[Operation of the algorithm](color_management#how-the-primaries-and-white-point-algorithm-works)

### Tone Reproduction Curve

The TRC is essentially a curve that transforms the initial or input
values of the image into values that will be recorded in the output
image. We can modify 2 parameters:

- Gamma,
- Slope.

You can choose predefined values such as 'standard g 2.2', 'sRGB g=2.4
s=12.92' or build a custom TRC. In the case of a custom TRC, the curve
will consist of a linear part and a parabolic part with a seamless
connection between the two.

You can see other uses of the TRC in Abstract profiles \>
[TRC_-_Tone_Response_Curve](color_management#trc---tone-response-curve).

### Illuminant

The illuminant is used to derive the white-point reference for color
profiles. The white point, which is identified by its temperature and a
symbol (D50, StdA 2856K, Fluorescent F11, etc.) is represented using
either XYZ or xyY data calculated as follows: X,Z (Y= 1) = Sum of the
matrix product from 350nm to 830nm \[Observer 2° x or z\]\[Illuminant
I\]/ Sum of the matrix product from 350nm to 830nm \[Observer 2°
y\]\[Illuminant I\].

Although the colors are usually encoded using one of the standard D50,
D60 or D65 illuminants, you can specify a different illuminant in the
drop-down list. In this case, the profile generator will add the
necessary chromaticity adaptation to convert the colors to the selected
illuminant.

For further information on illuminants and Abstract Profiles see:
[Illuminant - Abstract profile](color_management#illuminant---white-point).
