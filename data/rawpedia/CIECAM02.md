---
title: CIECAM02
date: 2026-02-09
contributors:
  - Jdc - Jacques Desmis
  - XavAL
tags:
  - 'Tool Description'
  - 'Advanced Tab'
toc: true
---

## Color Appearance & Lighting (CIECAM02/16) et Color Appearance (Cam16 & JzCzHz) - Tutorial

[Game changer](/tutorials/)

### Introduction

The following information is common to:

- Color Appearance & Lighting (CIECAM02/16) - Advanced tab
- Color Appearance (Cam16 & JzCzHz) - Local tab

Both of these modules use all or part of a CAM or color appearance
model. When a topic concerns just one of these two modules, it will be
explicitly mentioned. Ciecam02 has many advantages but it has
shortcomings when processing high dynamic range images (HDR) and in
particular in the highlights where the gamut is reduced. The performance
is considerably improved with Ciecam16.

The Color Appearance module (Cam16 & JzCzHz) combines a real CAM (Cam16)
and JzCzHz which is not a real CAM, but has some of its characteristics.

[HDR to SDR: A First Approach (Log Encoding - CAM16 - JzCzHz - Sigmoid)]({{% relref "local_adjustments/#hdr-to-sdr-a-first-approach-log-encoding---cam16---jzczhz---sigmoid" %}})

### Definition of a color appearance model (CAM)

A Color Appearance Model (CAM) is a mathematical model that seeks to
describe the perceptual aspects of human color vision i.e. the viewing
conditions under which the appearance of a color is different to the
corresponding physical measure of the source stimulus (RGB, XYZ are not
CAMs, Lab is a CAM with limited possibilities).

#### CIECAM02/16

This module is based on the CIECAM02/16 color appearance model, which
was designed to better simulate how human vision perceives colors under
different lighting conditions, for example by taking into account
various backgrounds. It takes into account the environment of each color
and modifies its appearance to come as close as possible to human
perception. It also adapts the output to the intended viewing conditions
(monitor, TV, projector, printer, etc.) so that the chromatic appearance
and contrasts are preserved throughout the scene and display
environments.

Some examples of color phenomena that can be addressed with a CAM:

- A color will be perceived differently on a light or dark background.
  The darker the background, the more it will be necessary to reinforce
  the color.
- An object appears brighter and more contrasted in direct light than in
  shadow.
- As the luminance increases, dark colors appear darker and bright
  colors appear brighter.
- Colored objects appear lighter than achromatic objects with the same
  luminance. The most saturated colors appear the brightest.
- Chromatic adaptation, which is the ability of the human visual system
  to adjust to changes in lighting (illuminating) conditions. In other
  words, we adapt to the color of the light source to better preserve
  the color of the objects themselves. For example, under an
  incandescent light, a white book appears yellow. However, we have the
  ability to automatically model yellowish light, so we see the paper as
  white. The world around us would indeed be very complicated if objects
  changed color each time the light source changed, even slightly. Since
  the dawn of time, we have had to be able to tell whether a fruit is
  ripe in the morning, at noon, or in the evening. Chromatic adaptation
  makes this possible.

### Variables - data and vocabulary used by CIECAM

[Variables - datas]({{% relref "ciecam02/#data" %}})

[Definitions]({{% relref "ciecam02/#some-definitions" %}})


### CIECAM02/16 consists of 3 processes:

1.  Source or Scene conditions: the shooting conditions and
    corresponding data are normalized to average or "standard"
    conditions (e.g. D50 white balance) prior to making any subsequent
    CIECAM corrections.
2.  Adjustments to the Source data.
3.  Viewing Conditions: the viewing conditions of the final image
    (monitor, TV, projector, printer...), as well as the corresponding
    environment. This process will take the data from process 2 and
    adapt it to the output medium to take into account the viewing
    conditions and the viewing environment.

### Differences in the way CIECAM is used in the 2 modules (Advanced tab, Local Adjustments tab)

Differences in the way CIECAM is used in the Advanced tab and the Local
Adjustments tab

- “Color Appearance & Lighting (CIECAM02/16)” - Advanced tab: this
  module contains all of the Ciecam02/16 concepts and tools. It was
  developed in 2012 and is specific to Rawtherapee. The user can select
  two levels of complexity:
  - Standard: gives the user the tools and variables needed to become
    familiar with the concepts and for everyday use.
- Advanced: contains the complete set of tools and associated variables
  for advanced use.

<!-- -->

- “Color Appearance(Cam16 & JzCzHz)” - Local tab: This module contains a
  simplified set of Cam16 tools and a first approach to JzCzHz. The user
  can select three levels of complexity:
  - Basic: provides the basic tools and variables used by Cam16.
  - Standard: provides additional Cam16 tools to take into account allFtutorial
    the perceptual aspects of human color vision. It also includes a
    mask.
  - Advanced: adds additional Cam16 tools and variables and a JzCzHz
    module.

[HDR to SDR: A First Approach (Log Encoding - CAM16 - JzCzHz - Sigmoid)]({{% relref "local_adjustments/#hdr-to-sdr-a-first-approach-log-encoding---cam16---jzczhz---sigmoid" %}})

### Some examples of CIECAM02/16

The following five examples will allow us to discover the power of this
tool along with some of its limitations:

#### Example 1  (Advanced Tab)

We are going to use process 1 (Source or Scene conditions) to process a
high dynamic range image with heavily underexposed areas. We will use
CIECAM to modify the Source lighting.

##### Preparation

The image we have selected is a difficult one with deep shadows and
strong sunlit backlighting. Open the image using the default RawTherapee
settings and set the Lockable Color Picker as shown so that you can see
the result of the subsequent adjustments.

Raw file (Pixls.us - Creative Common Attribution-share Alike 4.0):
[1](https://drive.google.com/file/d/1ctjOWX2lVmgcAzJtBwt69FGpxZOq-LyP/view?usp=sharing)

<figure>
<img src="ciecam_light_prepa1.jpg" title="ciecam_light_prepa1.jpg"
width="600" />
<figcaption>ciecam_light_prepa1.jpg</figcaption>
</figure>

##### Lighten the image

Go to the Advanced tab and select Color Appearance & Lighting
(CIECAM02/16). Under Scene Conditions set the Surround-Scene Lighting
combobox to Dark. That's it!
<img src="ciecam_ligh1.jpg" title="ciecam_ligh1.jpg" width="600"
alt="ciecam_ligh1.jpg" />

You can refine the adjustment using the Image Adjustments settings.

- Try adjusting Lightness (J) and Contrast (J).
- Try adjusting Chroma (C).

Switch to Advanced mode (Settings – Preset).

- Choose the algorithm: Lightness + Saturation (JS).
- Try adjusting Saturation (s) and compare the result with Chroma (C),
  looking carefully at the effect on the shadows, midtones and
  highlights.

.

#### Example 2 - Chromatic adaptation

a)Advanced tab. Starting with an almost perfect white balance, we will
use CIECAM in symmetrical mode (Preset cat02/16 \> Automatic Symmetric)
to carry out a chromatic adaptation.

##### CIECAM Advanced tab - preparation

Raw file (Rawtherapee - Creative Common Attribution-share Alike
4.0):[2](https://drive.google.com/file/d/1CiQ2t4KyD3tdCiNNhskqUG2cH9LT2ly7/view?usp=sharing)

The first step is to set an almost mathematically perfect white balance
in the Color tab using White Balance \> Auto \> "Temperature
correlation".

<figure>
<img src="catatwb1.jpg" title="catatwb1.jpg" width="600" />
<figcaption>catatwb1.jpg</figcaption>
</figure>

Examine the result:

- Temperature : 7450K
- Tint : 1.050

If you choose the option "Camera" you will get Temperature: 7862K, Tint:
1.134

In both cases the image has a yellowish tinge. The white balance
calculation is mathematically correct but the lighting conditions at
that latitude and at that time of the year (London at 8.00 am in
September) are such that our eyes and brain would have adjusted to
preserve the appearance of the colors (chromatic adaptation).

To remedy this, we will use CIECAM02.

##### Automatically set the chromatic adaptation

- Open the Color Appearance & Lighting (CIECAM02/16) tab.
- Select Automatic Symmetric mode in the "Cat02/16 mode" combobox.
  That's it!

<figure>
<img src="catatauto1.jpg" title="catatauto1.jpg" width="600" />
<figcaption>catatauto1.jpg</figcaption>
</figure>

Look at the image, the sky is blue and the skin tones of the passers-by
look natural.

- You can modify the result if you wish by going to Viewing Conditions
  and changing "CAT02/16 adaptation" by bringing it back to 60 for
  example.
- Note the temperature – it is the same as the white balance
  temperature.

#### Example 3 - Viewing conditions

In this example, we will mainly use process 3, Viewing Conditions (with
some process 2 adjustments) to show the impact of the output device and
its environment (using a holiday shot that we want to view on the family
TV in the afternoon).

Raw file (Creative Common Attribution-share Alike 4.0)
[3](https://drive.google.com/file/d/1GdqejdnbW1kJFNY6y9sdQDlF2rCEGMCu/view?usp=sharing)

We will use the usual settings for a TV screen (see the documentation of
the OLED TV)

- Illuminant = D65
- Mean Luminance (Yb%) = 18 (we assume that the average luminance of the
  TV screen is correctly adjusted).

Viewing environment:

- Absolute luminance: at the end of the afternoon we can estimate this
  value to be 10 cd/m2 (ideally we should measure it).
- Surround = Dim: the TV is set against a dark wall.

<figure>
<img src="voyagejpg1.jpg" title="voyagejpg1.jpg" width="600" />
<figcaption>voyagejpg1.jpg</figcaption>
</figure>

Important note: The same image viewed on your PC will not look good
because the viewing conditions are not the same (illuminant, surround,
absolute luminance).

##### Show your friends: the weather was beautiful, the colors are magnificent

Of course, we are so keen to impress our guests that we have cheated a
little by adding some extra contrast and chroma.

<img src="voyage_plus1.jpg" title="voyage_plus1.jpg" width="600"
alt="voyage_plus1.jpg" />

## Local Adjustments - Color Appearance (Cam16 & JzCzHz Experimental)

### Color Appearance - Cam16

As of November 2021, there is a new tool in the development version of
Local Adjustments. The module includes:

- The majority of the algorithms in the main Ciecam module (the
  chromatic adaptation processes have been simplified and the Symmetric
  mode is no longer automatic).
- A "Sigmoid J & Q" function which allows the user to process the
  luminance (lightness or brightness) and the contrast using a single
  algorithm.
- An "experimental" HDR PQ function that applies "gamma HDR" functions
  (PQ and Inverse PQ -note1) to the XYZ\<=\>Cam16 conversion matrices.
- A combobox "CAM Model", which allows the user to select either CAM16
  or JzCzHz.
- A combobox "Change tool position", which allows the user to either:
  - Use the module in stand-alone mode (default).
  - Use the algorithms in conjunction with any of the following tools:
    Tone Mapping, Wavelets, Dynamic Range and Log Encoding.

Note1: PQ =  Perceptual Quantizer

## Jzazbz - a new experimental CAM? (Cam16 & JzCzHz)

See the possible use in Local Adjustments:
[An experimental JzCzHz module]({{% relref "local_adjustments#an-experimental-jzczhz-module" %}})

### General

Jzazbz (and its polar form JzCzHz) is a color space designed for uniform
perception in high dynamic range (HDR) and wide color gamut (WCG)
applications. It is similar to CIE L\*a\*b\*, but provides theoretical
improvements:

- The perceived color difference is predicted by the Euclidean distance
  (deltaE).
- Uniform perception: MacAdam ellipses are more circular and are similar
  in size. This means that the chromatic tolerance is lower and the
  colorimetry is better preserved.
- Hue linearity: a change in saturation or brightness results in less
  hue shift.

But, Jzazbz is not a real CAM (Note 1). It is not capable of
simultaneously processing contrast for example, nor does it allow for
chromatic adaptation. Note that in RawTherapee these problems are
partially solved for SDR images by using Munsell correction (Uniform
Perceptual L\*a\*b\*).

The various comparative studies carried out (in laboratory tests,
theses, etc.) show that: CAM16 obtains the best marks for SDR images and
Jzazbz is just behind. Jzazbz obtains the best marks for HDR images and
CAM16 is clearly behind.

Note 1. The authors of "Jzazbz" are working on ZCAM, which is intended
to be the Jz equivalent of CAM16. Several attempts were made to
implement ZCAM in Rawtherapee in September and October 2021. However the
results were disappointing and the code has been deactivated. Its
presence has no impact on performance, memory usage and processing times
and can be reactivated at a later stage if necessary.

However, the work on the ZCAM and CAM16 code provided an opportunity to
enhance "Jzazbz” with additional CAM characteristics in addition to
"La"(Absolute luminance). i.e.

- The mean luminance of the scene "Yb% - Mean luminance";
- Three settings for the scene surround conditions- Average, Dim, Dark;
- The possibility of using "Relative luminance" instead of "Absolute
  luminance" with the following choices:
  - Lightness and Contrast Lightness instead of Brightness and Contrast
    Brightness.
  - Saturation in addition to Chroma.

### Issues

Jzazbz uses absolute luminance (La), not relative luminance. Relative
luminance refers to the maximum luminance that an input device (such as
a scanner or camera) can record, or the maximum luminance that an output
device (such as a monitor) can display. Thus, a relative luminance of
100% is the maximum that a camera can record or a monitor can display.
Jzazbz is designed to use absolute luminances, which means that 100% in
the Jz channel corresponds to a luminance of 10000 cd/m2 (candelas per
square meter) for example. As we will see, this poses a number of
problems.

These are difficult to address in RawTherapee and a priori in other
development attempts, due to the lack of documentation about the Jzazbz
concept. Certain questions have been raised with the author of the
concept but they remain unanswered at the time of writing. At this
stage, it is not known whether these problems are:

- Design flaws, which have not yet been addressed or resolved.
- Issues that have been resolved but not documented.
- Lack of understanding (especially on my part).

### What are the problems?

- When we examine the data values obtained using the basic settings of
  the original conversion algorithm, we find that they are well below
  the theoretical limits (e.g. 0.08 or less for Jz instead of 1). This
  also applies to "az" and "bz", for which the theoretical limits are +-
  0.5). The immediate consequence is that a simple action on the Jz
  channel results in a marked saturation deficit. I think this behavior
  is due to the PQ function (at the end of procees) and its inverse
  (beginning).
- In the original algorithm, reducing Jz (brightness) produces
  significantly strong artifacts in the shadows.
- There is no gamut control.
- The major problem however (in RawTherapee at least) comes from the ICC
  screen profiles, which can be limited by the PCS (Profile Connection
  Space) in LCMS.
  - Depending on the profils the PCS, can be either 8 bit L\*a\*b\*,
    which limits the transmission to luminance values between 0.1 and
    120 cd/m2, or XYZ which does not limit the luminance values for
    RT4_xxx or RT2_xxx type profiles, whereas an HDR system should be
    able to cope with luminance values between 0.0001 cd/m2 and 10000
    cd/m2.
  - The associated TRC (Tone Response Curve) is an sRGB type, which has
    a linear and a parabolic part. This TRC cannot be replaced by the
    more efficient Perceptual Quantizer (PQ) originally proposed by
    Dolby for HDR. Unlike the L\*a\*b\* sRGB gamma value, which is set
    to 3, the PQ has a gamma which can vary according to the maximum
    luminance peak supported by HDR screens. In theory this can be up to
    10000 cd/m2 for whites, but in practice the value is much lower;
    OLED screen white values are around 700 to 1000 cd/m2 and blacks are
    around 0.0001 cd/m2.
- Solving these problems would mean being able to use Jzazbz as a PCS
  (or failing that XYZ), and to use PQ as a TRC. This implies that LCMS
  is capable of taking this into account and that it is possible to
  increase the accuracy of the screen profile calculations!

[About Lab]({{% relref "toolchain_pipeline/#the-importance-of-ciecam-and-lab" %}})

- I don't have an HDR monitor (HDR monitors have luminance peaks of up
  to 6000 cd/m2 or more), nor do I have suitable ICC profiles or a
  modified version of LCMS, so my proposals are only assumptions based
  on my experience with color management and the problems encountered
  when implementing Ciecam02 and Ciecam16. All of this was carried out
  on an SDR system, which still accounts for the vast majority of users.

### Objectives and solutions

I have set myself the following goals:

- Use Jzazbz for SDR without producing artifacts and ensuring good
  colorimetry. Potentially Jzazbz should be able to replace L\*a\*b\*
  (theoretically better?) and provide a set of tools usable in Jzazbz
  mode.
- Pave the way for HDR.

What solutions have I chosen?

- Take into account La (absolute luminance) of the source.
- Calculate the "maximum" value of Jz for the image \[0..1\], often
  around 0.2. This value changes according to the PQ settings.
- Use an estimated value of Jz = 0.25 for 100cd/m2 (derived from a graph
  of measured vs perceived luminance and my own interpretation of the
  results).
- Re-evaluate (remap) the value of Jz, which will be often between 0.5
  and 1 (and Jz="maximum" if "La" is higher or equal to 10000 cd/m2) as
  a function of "La" (absolute luminance) using the 3 values ("La",
  "maximum", "Jz reference 100"). The calculations are done for sliders
  and curves with Jz max = 1, to ensure they behave similarly to what
  users would expect with RGB and L\*a\*b\*. Of course all these values
  are inverted before performing the inverse function (matrix
  XYZ\<=\>JsCzHz, and PQ)
- Change the PQ value, which is uniformly 10000 cd/m2 in the
  calculations, to match the maximum peak luminance value of the monitor
  i.e. 100 to 120 cd/m2 for SDR monitors or any value between 120 and
  10000 cd/m2 for HDR monitors. This prevents low-light artifacts when
  reducing Brightness.

**The graphical user interface (GUI) incorporated in the Local
adjustments Color Appearance module (Cam16 & JzCzHz)**

- The solution has been implemented with 3 slides, located in the "Jz
  remapping" window. If you change "La" (after disabling the checkbox
  "auto" in 'Scene conditions'), you will see the value of PU-adaptation
  change with effects on the image and histogram.
- "PU - adaptation" (Perceptual Uniform): automatically takes into
  account Absolute Luminance.
- "Jz reference 100 cd/m2" : by default set to 0.25.
- "PQ -Peak luminance": to be set according to the characteristics of
  the output monitor. For SDR it should be 100 or 120 cd/m2. For HDR set
  it to the required value.

You can see in the console (if verbose = true in 'options'), the
calculated values of "Remapping" : "Max_real" compared to the original
Jz "maxi" value, as well as the value of the coefficient "To_one"
applied to the sliders and curves so that they behave normally.

### Understanding the CAM - SDR - HDR settings - General

The development of this experimental tool was fraught with difficulties,
due to:

- The lack of in-depth documentation about the JzCzHz concept. The only
  published information is essentially limited to the description of the
  XYZ-Jzazbz conversion matrices.
- The fact that current photographic practice uses SDR concepts and
  tools inherited from technologies used in the 1990s and 2000s for
  limited-performance CRT monitors.
- The fact that photographic images, unlike video, are mostly viewed on
  the web (sRGB) or on printed paper, which is even more limited than a
  CRT in terms of gamut, dynamic range and peak luminance. In view of
  this, we could question the usefulness of HDR except for specialist
  applications.
- HDR tools available for photography use are rare, if not non-existent.
  For example:
  - The monitor output in Rawtherapee is programmed in L\*a\*b\* 8 bits
    (which limits the Dynamic Range to about 7 or 8 Ev, and the peak
    luminance to 100 or 120 cd/m2).
  - Monitor profiles are managed by LCMS which does not have (to my
    knowledge) HDR access.
- The price of an HDR monitor is currently prohibitive for most people:
  - 3000 to 5000€ for a monitor with limited HDR performance (peak
    luminance around 300 cd/m2).
  - 30000 to 60000€ for a monitor able to handle a dynamic range in the
    order of 21 Ev, a peak luminance higher than 3000 cd/m2, and a gamut
    close to Rec2020.
  - Recent OLED TVs have interesting HDR characteristics (larger gamut
    than sRGB - close to DCI-P3, infinite contrast, peak luminance from
    500 to 800cd/m2 and resolution = 3840x2160), however I have not
    found any drivers, nor any ICC profiles adapted to these devices.
    Their use seems possible in partial HDR and WCG (Wide Color Gamut -
    close to DCI-P3), especially when using recent high-dynamic-range
    (14 or 15 Ev) cameras connected directly to the TV with an HDMI
    cable using the 2160p standard. In this case, JPG (or Raw) is used
    with an XYZ ICC profile in a PCS (Profile Connection Space) and is
    not limited to 8 bits. It would be possible to use such a TV as a
    monitor if the graphics card of the PC could cope with the
    requirements of HDR (3840x2160, the gamut and PQ) and the
    appropriate ICC profiles.
- Concepts such as CAM, Absolute and Relative luminance, Dynamic Range,
  PQ and iPQ (instead of TRC) are often abstruse, poorly documented, and
  their use in programming left to the initiative of the programmer (his
  skills, his understanding of the concepts, etc.).

As a consequence, using HDR and programming the necessary applications
has limited application for widespread daily use and is therefore
largely an academic exercise. This did not prevent me from developing
this module, which clarifies a number of concepts and tries to share my
experiences.

#### Understanding the CAM - SDR - HDR settings - Introduction

Jzazbz uses absolute luminance (La), not relative luminance (such as
L\*a\*b\*). Relative luminance refers to the maximum luminance that an
input device (such as a scanner or camera) can record, or the maximum
luminance that an output device (such as a monitor) can display. Thus, a
relative luminance of 100% is the maximum that a camera can record or a
monitor can display. Jzazbz is designed to use absolute luminances. For
example, 100% in the Jz channel corresponds to a luminance of 10000
cd/m2 (candelas per square meter), a parameter used by PQ (origin
Dolby). This, as we shall see, poses many problems. The same applies to
the difference between Q and J in Ciecam02/16, except that Q is not
referenced to a luminance value of 10000cd/m2. It is essential to
understand this difference between absolute and relative luminance.
<https://en.wikipedia.org/wiki/Perceptual_quantizer>

The two conversion matrices XYZ to Jzazbz and its inverse Jzazbz to XYZ
are complex because:

- The color blue is treated differently (contrary to Cam16).
- They introduce the notion of inverse PQ (iPQ) and PQ, which are
  necessary with HDR devices, instead of the usual gamma and i-gamma
  functions found in SDR equipment (except for Cam16 conversion
  matrices), and in Rawtherapee in the form of sRGB gamma (TRC). These
  PQ functions are incorporated into the direct (iPQ) and inverse (PQ)
  matrices and take into account the peak luminance, which is directly
  related to the absolute luminance, La. This will result in a
  significant reduction of the Jz values when using the Jz data (Jz is
  between 0 and 0.02 depending on the value of PQ with an average value
  in the order of 0.003. The maximum range of Jz is \[0..1\]). Of course
  this conversion is anything but linear and makes the use of Jz values
  difficult, if not impossible for curves, sliders and various functions
  such as wavelets, shadows & highlights etc. If I had left everything
  as it is, i.e. not changing anything in the two matrices, the overall
  rendering would have lacked saturation and the 'usual' tools would be
  have been unusable. Not only that, but the activation of certain
  sliders would have led to strong artifacts.

Further information can be found in the following links:

- The schematic diagram of the workflow from light capture to image
  display, figure 1.3 page 29.
- The formulae 1.2 and 1.32 in pages 38 and 39 for the specific handling
  of the blue color component.
- Formulas 1.5 page 30 for the gamma function (BT709 in this case) and
  1.21 page 38 for the iPQ function.

<https://tel.archives-ouvertes.fr/tel-02378332/document>

Additional information about Dynamic Range and Absolute Luminance:

- Dynamic Range (DR) translates the difference in Ev between the maximum
  and minimum luminance values (White Ev - Black Ev). Typical values in
  modern digital cameras are in the order of 13 to 15 Ev or more if we
  use DNG files combining several raw files.
- Absolute Luminance, La corresponds to the luminance of the scene at
  the time of shooting, expressed in candelas per m2. This value is
  estimated using the speed, aperture and ISO in the EXIF data (and may
  not be accurate especially when using DNG files). Values in the range
  of 2000 to 7000 cd/m2 (or more) are common in summer sunlight, or in
  winter snow scenes.
- These values are a long way from the values that can be reproduced by
  an SDR monitor (7 to 8 Ev, and 120 cd/m2)
- Compromises are therefore necessary if we want to fit high DR values
  into the range of an SDR monitor.

### Implementation

- Jzazbz (in the form JzCzHz) is incorporated into the new Local
  adjustments “Color appearance (Cam16 & JzCzHz)“ module.
- Because of its complexity, it is only available in Advanced mode.
- It uses “double” precision instead of “float” to increase the
  precision of the calculations. This has an impact on the processing
  time.

#### Several tools are available to judge the relevance of Jzazbz as a replacement for L\*a\*b\*.

- Traditional tools: Brightness (or Lightness), Contrast Brightness (or
  Contrast Lightness), Chroma, Saturation, Hue rotation as well as the
  following 6 curves Jz(Jz), Cz(Cz), Cz(Jz), Hz(hz), Cz(hz), Jz(hz).
  - Note that for the Hz(hz), Cz(hz), Jz(hz) curves, the inherent design
    of the XYZ\<=\>Jzazbz conversion matrices results in a weaker (or in
    some cases erroneous or zero) response in the blues, especially when
    the saturation is low.
- Shadows/highlights (similar to the tool in L\*a\*b\*).
- Wavelets :
  - Local contrast.
  - Clarity & Sharp mask.
- Recovery based on luminance mask.
- Mask and modifications.
- Other L\*a\*b\* tools could also be added in the future if required.

#### Two tone-mapping tools: Log Encoding Jz and Sigmoid Jz

##### Log encoding Jz

Jz Log Encoding allows the user to modify the balance of Jz values to
reduce contrast, enhance shadows and reduce highlights without overly
distorting the image rendering. The effect is similar to its RGB
counterpart in the Local Adjustments Log encoding module. However,
instead of using RGB luminance we use Jz (Brightness) to completely
separate the luminance channel from the other 2 channels i.e. chroma Cz
and hue Hz. The logarithmic weighting is applied to the Jz channel. The
result is slightly different from the RGB model with better preservation
of the colorimetry.

- Four main settings allow you to adjust the results:
  - “Mean Luminance (Yb%)” in Scene Conditions: reducing the value of
    the slider will increase the overall image luminance. This action is
    carried out before the log conversions.
  - Black Ev and White Ev, allow you to readjust the calculated values
    of the dynamic range and differentiate the modifications made to the
    shadows and highlights.
  - “Viewing Mean luminance (Yb%)” acts in conjunction with Black Ev and
    White Ev to calculate the final log conversion. Increasing this
    value will lighten the image.

##### Sigmoid Jz

This tool dynamically compresses Jz brightness to adjust both contrast
and luminance. It is similar in concept Jz Log Encoding and other tone
mapping tools.

- For an overview of the sigmoid function see
  <https://en.wikipedia.org/wiki/Sigmoid_function>.
- The version used in Rawtherapee is a bit more complex and uses 2
  additional parameters. For information, the mathematical formula is:
  "Sigmoid RT": Result = 1. / (1. + exp(lambda - (lambda/threshold) \*
  Jz)).
  - Lambda acts on the slope of the curve, resulting in more or less
    contrast;
  - Threshold moves the curve towards the shadows or the highlights,
    allowing the action to be distributed more finely according to the
    image. You could even say that Threshold adjusts the “Gray point".
  - A simulation of the sigmoid function using 2 parameters “l”, which
    corresponds to Contrast and "t", which corresponds to Threshold, can
    be found in the following link:
    <https://www.desmos.com/calculator/g382ci99gu?lang=fr> . Note that
    the calculation performed in the code is a little different. This
    simulation is for didactic purposes only.
- The function looks like a tone curve, but without the toe, shoulder or
  linear part. It is a continuous exponential variation with some
  interesting features, particularly in the context of HDR images:
  - For values close to 0 and 1, the curve progresses very slowly and is
    almost linear, allowing it to be used in deep shadows with low Jz
    values around 0.001 cd/m2 (translated into absolute luminance) or in
    high luminance areas with values of several hundred cd/m2.
  - For intermediate values, the exponential part dynamically compresses
    the data similar to a tone-mapping tool.
  - There is an option that allows you to take into account the values
    of Black Ev and White Ev, which are also calculated for Log Encoding
    Jz. In this case, Jz is replaced by its equivalent expressed in Ev
    in the Sigmoid RT formula above i.e. Jz_equivalent_Ev = (log2(Jz) -
    Black Ev) / Dynamic Range. In the latter case, the tool behaves
    similarly to Log Encoding Jz. The important difference is in the
    resolution of the equation:
    - In the case of Log Encoding the values of Black Ev, White Ev and
      "Viewing mean luminance" are used to calculate the log conversion.
    - In the case of Sigmoid, the values of Black Ev, White Ev and
      "Threshold (Gray point)" are modified by the exponential sigmoid
      curve (via the Contrast slider described below, which acts on the
      slope of the sigmoid at the inflection point). This allows the
      user to find the best compromise between the 4 settings – Black
      Ev, White Ev, Threshold (Gray point) and Contrast, which include
      the automatic calculation of the the BlackEv and WhiteEv values.
      The additional contrast slider, which is not present in Log
      Encoding, allows the user to control the luminance distribution.
  - A Blend slider allows the user to adjust the final result (a bit
    like "Viewing mean luminance" used in Log Encoding).

##### Adjusting the saturation

Both tools preserve the hue, which is the basis of Jz. Nevertheless the
action of these 2 tools can have a significant influence on the contrast
thereby increasing or decreasing the resulting Jz luminance. In this
case, it may be necessary to adjust the saturation using the following
tools, which will preserve the hue:

- Chroma and Saturation sliders.
- Cz(Cz) and Cz(Jz) curves.

### Documentation

<https://tel.archives-ouvertes.fr/tel-02378332/document>

<https://www.color.org/groups/hdr/HDRWG-Summer2020.pdf>

<https://www.osapublishing.org/oe/fulltext.cfm?uri=oe-29-4-6036&id=447640>

## About CIECAM02

The following sections are legacy translations from the French original,
which has been subsequently updated, and need to be reviewed.

### Introduction - history

For many years man has been trying to model color and the way it is
perceived.

Theories have been advanced since the Middle Ages, but the main advances
didn’t occur until the 19th and then the 20th centuries.

The information presented below provides some basic information to help
understand the various concepts used in the software. Further
information can be found in the links below or on the Web. Please bear
in mind that I am not a specialist in the physiology of the human visual
system, nor a researcher in the complex field of colorimetry.

Commonly in photography, we use (more than) 50 years old models : RGB
and its derivative (HSV, HSL, CMYK,...), XYZ, and Lab and its derivative
(Luv, Lch). I won't comeback on the RGB model, known by everyone, it is
dependent of the peripheral and doesn't take into account any CAM (color
appearance model). The CIE's definition of XYZ (1931) was the first step
of the « Commission Internationale de l'Éclairage » (CIE - International
Commission on Illumination) towards a human description of the colors
faithful to the human vision. To summarize, a color can be characterized
by its 3 X, Y and Z values, obtained by a combination of « tristrimulus
values », « CIE standard observer » and the base color's « spectral
power distribution ». This model has been taken up in RawTherapee,
particularly in terms of white balance... This model doesn't take into
account any CAM, but it's an extraordinary leap forward, because we now
can model a color in cognitive terms.

The Lab model has been designed in 1976 by the CIE by derivating it from
the XYZ model, it characterize a color with an intensity parameter
corresponding to the luminance and two chrominance parameters that
describe the color. It has been specifically studied so that the
computed distance between colors correspond to the differences perceived
by the human eye. The Lab model is well established in RawTherapee, it
is used as a basis for most of the tools : sharpening, denoising, tone
mapping, Lab adjustments, etc.. It integrate some characteristics of a
CAM, but the benefits are sketchy. The CIECAM02 model, derived from
CIECAM97 and using G.Hunt's work, is the first commonly usable model in
photography, because it is invertible... and relatively « simple », it
can take into account other than purely cognitive aspects and is based
on the work of many researchers on the basis of sample of persons who
evaluate different parameters, like :

#### simultaneous contrast

Variation of the colored appearance of an object depending on the
colorimetric characteristics of its close environment. For example, the
same color will be perceived differently on a white or dark background.
The darker the background will be, the more we'll have to boost
colors...

(Creative Common Attribution-share Alike 4.0)
<img src="simult-contr1-1.jpg" title="simult-contr1-1.jpg" width="600"
alt="simult-contr1-1.jpg" />

<img src="simult-contr3-1.jpg" title="simult-contr3-1.jpg" width="600"
alt="simult-contr3-1.jpg" />

#### the Hunt's effect

Increased seen coloration (colorfulness) with luminance. An object
appears more vivid and contrasty in full light than in shade. (Creative
Common Attribution-share Alike 4.0)
<img src="huntseffect1-1.jpg" title="huntseffect1-1.jpg" width="600"
alt="huntseffect1-1.jpg" />
<img src="huntseffect-2-1.jpg" title="huntseffect-2-1.jpg" width="600"
alt="huntseffect-2.jpg" />

#### Stevens's effect

Augmentation of the perceived contrast with the luminance. When the
luminance increases, the dark colors looks like even darker and the
luminous colors appears even more luminous. (Creative Common
Attribution-share Alike 4.0)

<img src="simult-contr2-1.jpg" title="simult-contr2-1.jpg" width="600"
alt="simult-contr2-1.jpg" />

#### Helmholtz-Kohlrausch's effect

Dependence of the brightness in relation to the luminance and
chromaticity. Colored objects appear lighter than the achromatic objects
with the same luminance. The most saturated colors appear brighter.
(Creative Common Attribution-share Alike 4.0)

<img src="helmholtz-effect1.jpg" title="helmholtz-effect1.jpg" width="600"
alt="helmholtz-effect1.jpg" />

#### Chromatic adaptation

Adjustment by the human vision system of some color stimuli. The
chromatic adaptation let us interpret a color depending on its
spatiotemporal environment. It's an essential effect to be taken care of
by a CAM.

The chromatic adaptation is the human visual system's ability to adjust
to changing illuminant conditions. In other words, we adapt to the color
of the light source to better preserve the color of objects. For
example, under incandescent light, a white paper appears yellow.
However, we have the ability to automatically model the yellowish light
so we see as white paper. The world around us would be very complicated
if the objects changed color whenever the light source changes even
slightly. Since the dawn of time, we must be able to know whether a
fruit is ripe, would it be the morning, afternoon or evening. The
chromatic adaptation makes it possible. But it can also be the source of
many optical illusions. I think the majority of RT users know, at least
by name, the previous model of chromatic adaptation, called “Bradford”.
etc.

Note : there will be no question here of "Munsell Correction" because
CIECAM02 is, by principle, built around Munsell's tables... so this
correction is taken into account, even if the model has shortcomings!

#### Origine in Rawtherapee

My first thoughts about CIECAM02 dates back to 2007, and the development
of a spreadsheet, for best results in the development of ICC “input
profiles”. Early 2012, I addressed a request from users: "can we have
reference colors - color palette - (skin, sky, ..) which would allow a
better white balance through a comparison/iteration process". I also
worked on the concept of CRI (Color rendering index) which reflects the
difference of illuminants compared to a base illuminant... The lower the
CRI is, worse the rendering will be with an identical color temperature
see : [Color Management)]({{% relref "color_management/#introduction" %}})

Based on CIECAM02, the patch contains the necessary basic elements to
work these two points, but it lacks an essential element, not easy to
develop : a pipette. I have long considered CIECAM02, not as a gimmick,
but as something difficult to implement... and with a quite small bonus
compared to Lab. The request of Michael Ezra who surprised me at first,
led me to re-open the file; the plug-in for Photoshop was a discovery
for me by the example, of CIECAM02. I am now convinced that even if the
model is not perfect (for some pictures, its use is almost impossible!),
it is today the most undeniable (effective) in terms of color
management. The module I am proposing is an "initiation". From the data
of CIECAM02, it is possible to develop a series of features similar to
those already developed in RT (Lab adjustments with various curves,
tone-mapping, etc.). Probably with significant advancements in terms of
quality.

The lack of effective documentation adds to the complexity... Some
points of view are personal (can be tainted by errors?). If a specialist
reads these lines, I'll be happy to change my text and my algorithms!

#### Evolution

##### Ciecam

Since 2016, Ciecam16 has been proposed by the researchers. I made the
update in 2020, of the module located in the "advanced" tab (with a
choice between Ciecam02 and Ciecam16, to preserve compatibility). From
now on it is called "Color Appearance & Lighting (CIECAM02/16)".

- Ciecam16 has several advantages compared to Ciecam02: simpler code, no
  artifacts, better gamut (much wider);
- Cam16 (another name for Ciecam16) is now present in the tab "Local" -
  "Color appearance (Cam16 & JzCzHz)

##### Jzazbz - JzCzHz

JzCzHz is the simple transform of Jzazbz - like Lch for Lab.

This "experimental" module, directly derived from the work in progress
of researchers, is not a CAM as such. But it behaves very well (once the
numerous malfunctions are solved) for both HDR and SDR images. You can
find it in "Color appearance (Cam16 & JzCzHz)" with the "advanced"
complexity choice

### Some definitions

1.  Brightness \[brilliance\] (CIECAM02/16 & JzCzHz) :

    The amount of perceived light from a stimulus = indicator that a
    stimulus appears as more or less bright, light.
2.  Lightness \[luminance\] (Lab, CIECAM02/16) :

    The clarity of a stimulus relative to the brightness of a stimulus
    that appears white under similar viewing conditions.

    Note that in RT, the“brightness” term applies to “Lightness” ! You
    will need to make a patch to rename “brightness” to “lightness” in
    the “exposure”, “Lab adjustments”, etc... modules.
3.  Hue and hue angle (partly in Lab, CIECAM02/16) :

    The degree to which a stimulus can be described as similar to a
    color described as red, green, blue and yellow.
4.  Colorfulness (CIECAM02/16) :

    The perceived amount of color relative to gray = indicator that a
    stimulus appears to be more or less colored.
5.  Chroma (Lab, CIECAM02/16 & JzCzHz) :

    The “coloration” of a stimulus relative to the brightness of a
    stimulus that appears white under identical conditions.
6.  Saturation (CIECAM02/16 & JzCzHz) :

    Coloration of a stimulus relative to its own brightness.
7.  PQ (Cam16 & JzCzHz) :Perceptual quantizer.

    A sort of variable gamma that is applied to the conversion matrices.
    It takes into account the characteristics of the image (Source) and
    the type of output (HDR or SDR monitor) to adjust the observed
    results. Can be set between 100cd/m2 and 10000cd/m2

To summarize :

1.  Chroma = (Colorfulness) / (Brightness of White)
2.  Saturation= (Colorfulness) / (Brightness)
3.  Lightness= (Brightness) / (Brightness of White)
4.  Saturation= (Chroma) / (Lightness)


    = \[(Colorfulness) / (Brightness of White)\] x \[(Brightness of
    White) / (Brightness)\]

    = (Colorfulness) / (Brightness)

CIECAM02/16 develops and uses several types of correlated variables that
allow the use of these concepts :

J : lightness or clarity, close to L (Lab)

C : Chroma, close to C (Lab)

h : hue angle, close to H (Lab)

H : hue. A color can be described by the composition of 2 base colors
between 4 (red, yellow, green, blue), e.g. 30B70G or 40R60Y.

Q : brightness

M : colorfulness

ac, bc : close to a and b (Lab)

Why the saturation in addition to other close variables? Here is a quote
from a text by Robert Hunt (2001) :



*“Of the three basic color perceptions, hue, brightness and
colorfulness, hue has no relative version, but brightness has lightness,
and colorfulness has chroma and saturation. Correlates of chroma are
widely used in color difference formulae, but saturation currently plays
little part in color science and technology. This is perhaps because in
many industries, flat samples are viewed in uniform lighting for the
evaluation of color differences, and in this case chroma is the
appropriate contributor for samples of small angular subtense. For
samples of large angular subtense, however, a correlate of saturation
may be more appropriate to use. In the real world, it is common for
solid objects to be seen in directional lighting; in these
circumstances, saturation is a more useful percept than chroma because
saturation remains constant in shadows. In imaging, artists and
computer-graphics operators make extensive use of series of colors of
constant saturation. In optical imaging, saturation can be an important
percept in large dark areas. Recent experimental work has provided a
much improved correlate of saturation.“*

## The 3 processes

Three processes allow the use of CIECAM, their names depends on each
designer. I've made a synthesis (reminder: this document is not a
course, or a thesis on CIECAM... but an aid to its understanding and
use).

### Process 1

Names like “origin”, “forward”, “input”, “source” are generally used...
I finally chose “source”, which corresponds to shooting conditions and
how to bring back the conditions and data to a “normal” area. By
“normal”, i mean medium or standard conditions and data, i.e. without
taking into account CIECAM corrections, e.g. “surround = average”, D50
white balance !

### Process 2

It corresponds to the treatment of correlated variables (J C h H Q M s a
b) for various purposes : action on lightness (J), brightness (Q),
chroma (C), saturation (s), color level (colorfullness M), the hue angle
h, as well as ac et bc. It is quite possible to build an images editing
software around those variables...

In the case of this patch for RT, I arbitrarily selected 4 groups of
algorithms :

1.  JC by adding an contrast function ;
2.  Js, as above
3.  QM
4.  All: all parameters including h.

These modules are simple, more for pedagogical purpose than trying to
solve the problems of colorimetry, even if the results are in my opinion
excellent.

I completed this process by :

1.  double “curves”, acting on contrasts J (lightness) or Q
    (brightness), whose principle is similar to the double curves of
    “exposure” ;
2.  a choice for color curves between chroma, saturation and color level
    (colorfulness).

    We could add other algorithms based on the Fourier transform, or
    replacing equivalent functions of RT...

### Process 3

Names like “inverse”, “reverse”, “output” or “viewing conditions” are
generally used. I've chose “viewing conditions”, which reflects the
media on which the final image will be viewed (monitor, TV, projector,
...), as well as its environment. This process will take the data from
the process 2 and “bring them” to the support so that the viewing
conditions and environment are taken into account.

Note: we find here the explanation of the rendering difference between a
printed photo and a picture viewed on a monitor - even if the printer is
a high-end and well calibrated one: the viewing (observation)
conditions! A printed photo will often viewed in an album, often on a
black background, in low light... and often tungsten lighting. The
original will be seen on a monitor with a light background, and a D50
illuminant... There is no question of changing the “print” output, but
to adapt the “monitor, TV...” output.

That is to say, but here stops the comparison, that we realize something
like soft-proofing, but it's not the case because it's the purpose of
CIECAM. We takes into account the settings specified in “Preferences”
(white point of the output device \[screen TV, projector...\] and its
average luminance \[% gray\]. We also takes into account the luminance
of the room in which the observation is made, as well as the relative
luminance of the visualization device's environment (more or less
black).

Simplified synthesis of what RT allows with the current patch :

1.  general case of the user who uses RT to see his development... that
    should represent 95% of the cases. In this case, “viewing
    conditions” corresponds to the RT work environment, e.g.:
    - monitor's white point set to 6000K
    - calibrated monitor, so Yb=18
    - but according to :
      - the selected “theme” in “Preferences / General” (almost black or
        gray), you have to change “surround”
      - the monitor's location (on a neutral or dark background), you
        have to change “surround”
      - lighting of the room, that will change with time, you'll have to
        change “adaptation luminosity viewing La” : e.g. at night
        without lighting up, “La” will be close to 0 or 1, and on the
        contrary by day in a bright room, “La” will be close to 1000
2.  less common case, but possible, because I've already done it, I use
    RT and the family TV to show pictures as well as RT's possibilities.
    The “viewing conditions” will be different and to be adapted to each
    case ; you'll have to review each of the points above with possibly
    different settings: TV's white point, TV's Yb (empirical?), a
    different “surround” because we generally look at the TV with a dull
    background, and with a reduced room's illumination.
3.  you want to prepare a series of photographs for an exhibition: in
    this case, in a professional manner, we will "see" viewing
    conditions on site and ask for data like the projector's white
    point, calibration (?), the room's brightness on the day of the
    exposition, etc... In RT, the user will set "viewing conditions" to
    suit the exposition's conditions, and produce X corresponding jpeg
    (or TIFF)
4.  etc.
5.  that's why I put most of the settings for the process \# 3 (Output
    Device) in "Preference" is not an error, but appears similar setting
    as the monitor profile that depends on the monitor...

## Data

Which data are taken into account and which simplifications I
(arbitrarily?) made? How to adjust them? :

### Yb
 (also in JzCzHz): Yb is the relative luminance of the
  background ! With that, we're much advanced ! Specifically it is
  expressed in % of gray. A 18% gray corresponds to a background
  luminance expressed in CIE L of 50%.

* for process #3, if your monitor is calibrated, you can easily have
a value of Yb close to 18 or 20. If your TV or projector, that seems
difficult to calibrate, seems dark or light, you can adjust this value
empirically. It depends on the visualization support and can be
considered as constant for a set of photos and in a condition of
observation. If you want to change this value, go to “Preferences /
Color Management / Yb luminance output device (%)”

* for process #1, it's much more complex because:

* an image has rarely a constant exposure and small luminance
variations

* I placed the CIECAM module at the end of the Lab process, just
before the RGB conversion and the sends to the output device, so we can
assume that the user has used various tools of RT to make the image have
an "average" histogram



So I arbitrarily made Yb inaccessible by calculating it from the average
luminance of the image. Of course, if in the future RT integrates
pipettes to separate the image areas (dark, normal, bright...) then it
would be possible to enter several Yb values. For example on an image we
could see three areas :

- standard, which corresponds to the average luminance of the image with
  a Yb set to 20%;
- underexposed (approximate contours delineated by the pipette...),
  where the luminance would be calculated and would e.g. give as a
  result Yb=5%;
- overexposed, where Yb would be as high as 70%..

### La
(also in JzCzHz): La is the adaptation field's absolute
  luminance ! Again, we're much advanced now !

* In process #1, it corresponds to the luminance when shooting. E.g.
if you make a photo in the shade, “La” will be close to 2000cd/m2; if
you make interior shots, “La” will vary depending on the lighting from
20 to 300cd/m2... In reproduction, these values may be even lower

* ”Scene luminosity” and the “Auto” checkbox** (process #1):

* If Checkbox enabled, La is calculated with Exif data (shutter
speed, ISO speed, F number, Camera Exposure comprensation) and also Raw
White Point and Exposure compensation slider
 
* In process #3, it corresponds to the luminance of the place in which
is made the observation. When you calibrate your monitor, you are asked
for this value... or you are offered the choice of using a probe. Orders
of magnitude from 15 to 100 should resolve most cases. But e.g. for a
theatrical projection in the dark, it can lead to lower values (1-10)

* These 2 values of “La” are adjustable in RT, in the “CIE Color
Appearance Model 2002” tool

### Surround
(also in JzCzHz: Scene Conditions)


Again, I have made simplifications...

- for process \#1, this data reflects some shooting conditions, such as
  photos in a museum with a dark background, or portrait shooting on a
  black background. Usually RT's user will have corrected the deviations
  from its perception thanks to its numerous tools. However, I've added
  a checkbox “Surround (scene) dark”, which can be activated if
  necessary. Its use will lighten the image (recall: this process bring
  the data “back to normal”)
- this data reflects the surroundings of the image when viewing. The
  darker the surrounding will be, the more you'll have to increase the
  contrast of the image. The “surround” variable is not acting as a
  D-lighting or tone curve, it also changes the colors in the red-green
  and blue-yellow axis. If the environment's luminance is greater than
  20%, choose “average”, otherwise adapt to your conditions, e.g. RT's
  settings (Preferences / General / Select theme) will affect the final
  rendering. This setting is accessible by “Surround (viewing)”. The
  darker the surrounding will be, the higher will be the image's
  simultaneous contrast.

### White-points model

* “WB RT + output” : here we trust RT's white balance for the process
#1; CIECAM uses D50 as a reference: RT's white balance bring back the
conditions to a D50 equivalent, while for process #3, it will be
necessary - as needed - to set the white point of the output device. Go
to "Preferences / Color Management / Settings white output device
(monitor, TV, projector)" and select an illuminant in the list (is it
sufficient? I have no idea about the characteristics of projectors,
light, temperature...)

* WB RT+CAT02 + output” : for process \#3, we are in the same situation
than above; for process \#1, a mix is made between RT's white balance
and CAT02 that is using its settings, which makes a solution where the
two effects (RT and CAT02) are combined. You can modulate the action of
CAT02 by acting on the “CAT02 adaptation” slider. You'll probably have
to change RT's white balance settings to benefit from the “mix”
advantages, otherwise the effects does add themselves.

* CAT02 is a chromatic adaptation, it converts the XYZ values of an
image whose white points is Xw0, Yw0, Zw0, in new XYZ values whose white
point becomes XW1, Yw1, ZW1 ; the algorithm used is similar to the one
from Von Kries, therefore different from RT's correction that takes into
account the channels multipliers !

### ”CAT02/16 adaptation” and the “Auto” checkbox

* see above for the usage of “WB CAT02/16 + output”

* however, even when “white point model” is set to “equal”, this slider
can be useful. Usually, the “auto” checkbox must be checked and CIECAM
calculates itself an internal “D” coefficient that is used for other
purpose than the chromatic adaptation. The result is a value greater
than 0.65 (65%); you can uncheck the box that will alter process \#1,
the effects can be unexpected...

## Normal or symmetrical mode

## Algorithm

You can choose between JC, JS, QM (of course there are other possible
combinations!), or “All” which lists all the possible settings (I
arbitrarily excluded “h” as well as “ac” and “bc” from the 3 algorithms
JC, JS, QM). The most common use (if one can use that term for CIECAM)
is JC, then act on the sliders to get the desired rendering... which I
recall depends on the display device, its environment, its settings and
the brightness of the room.

- **“JC” algorithm**
  - J simulates the lightness – close to L (Lab) – and C simulates the
    chroma, near the c (Lch) chromaticity. But, important difference, J
    and C take into account the “effects” (simultaneous contrast, Hunt,
    Stevens, etc...) which is not the case of Lab and even less RGB.
  - J varies in the \[ 0-100 \] range and corresponds to a relative
    value of the brightness (likewise L, or Value...) and theoretically
    C in the \[ 0-180 \] range (it can be higher)
  - The two cursors using J and C may vary from -100 to +100 with
    actions similar to the “Brightness” (to be renamed “Lightness”) and
    “Chromaticity” of “Lab adjustments” sliders.
  - with the “JC” algorithm, a skin tones control is possible, the
    action is similar to the similar cursor from “Lab adjustments”
  - The "contrast" cursor modulates the action of "J" with an "S" curve,
    which takes into account the histogram 's average brightness "J".

<!-- -->

- **“Js” algorithm** It is similar to JC, but :
  - the chroma is replaced by the saturation (CIECAM). But for which
    purpose? I'm quoting again an excerpt of the text from G.Hunt For
    samples of large angular subtense, however, a correlate of
    saturation may be more appropriate to use. In the real world, it is
    common for solid objects to be seen in directional lighting; in
    these circumstances saturation is a more useful percept than chroma
    because **saturation remains constant in shadows**. In imaging, artists
    and computer-graphics operators make extensive use of series of
    colors of constant saturation. In optical imaging, saturation can be
    an important percept in large dark areas. Recent experimental work
    has provided a much improved correlate of saturation.
  - Skin tones control is less "fine" than with "JC", globally wider in
    the reds

<!-- -->

- **“QM” algorithm**
  - here we use 2 variables Q (“brightness”) and M (“Colorfulness”) that
    are not relative data, but absolute. We takes into account the
    white's brightness. It is easy to realize that the same white
    "J=100" will appear brighter in the sun than in a dark room.
  - the white's brightness takes into account the following parameters
    (scene) : “adaptation luminosity La”, “CAT02 adaptation” and “Yb”
    (currently not adjustable).
  - in common use, the control is more difficult than with "JC", however
    it provides opportunities for high contrast images and opens the
    door for HDR processing.
  - Skin tones control is less "fine" than with "JC", globally wider in
    the reds.
  - The “contrast” obviously acts different, since it takes into account
    Q differently than J.

<!-- -->

- **“All” algorithm**
  - you can control all the CIECAM variables: J, Q, C chroma, saturation
    s, M color level, J contrast, Q contrast, hue angle h, skin tones
    protection (acts on C only)

## Tone curves and color

### Curves

- you have – likewise in the “exposure” module – a set of 2 tone curves,
  which acts on the “J” lightness and “Q” brightness. You can use one
  curve only or both, by eventually mixing “lightness” and “brightness”.
  Beware, “brightness” curves can easily lead to out of bounds results!
  "Brightness" is an absolute scale, while “Lightness” is a relative
  scale, the same “J” white will appear whiter directly illuminated by
  the sun than in the shade, which is taken into account by "brightness"
  (Q). Thus shadows and highlights will be rendered differently by the
  “lightness” and “brightness” curves.
- you also have a set of “chroma” curve with 3 choices: chroma (the most
  common), saturation and colorfulness. These 3 curves are used to
  adjust the chosen parameter according to itself, e.g. modulate the
  saturation to avoid that the already saturated colors goes out of
  gamut. For these 3 curves, the “red and skin tones protection” cursor
  is operational, it is more suitable for skin tones in the “chroma”
  mode. I recommend using the “parametric” mode that allow to
  differentiates according to the colors's saturation level. Note: All
  “chroma curves” combinations (chroma, saturation, colorfulness) and
  sliders (chroma, saturation, colorfulness) are not possible without
  overly complicating the code, that's why in few cases some sliders are
  grayed out.

### Histograms in Tone Curves

The tone curve histograms in the *CIE Color Appearance Model 2002* tool
can show values before or after CIECAM02 is applied. To view the values
after CIECAM02 adjustments, enable the "*Show CIECAM02 output histograms
in curves*" option. If disabled, the histograms show values before
CIECAM02.

### Histograms in the Color Curve

The histogram in the Color curve shows the distribution of chroma
(saturation/colorfulness) according to the intensity of the chroma
(saturation/colorfulness) or chromaticity in Lab mode. The more the
histogram is shifted to the right, the more saturated colors are close
to the limits of the gamut. The more the histogram is shifted to the
left, the more the colors are dull.

The abscissa represents the value of the chroma
(saturation/colorfulness) or chromaticity (in Lab mode). The abscissa
scale is "open".

As usual, the ordinate represents the number of pixels involved.

## Red - Green - Blue - Hue rotation (h) - Saturation (s) - Brightness curve (Q)
<figure>
<img src="red-green-blue.jpg" title="red-green-blue.jpg"
width="300" />
<figcaption>red-green-blue.jpg</figcaption>
</figure>
I've added an extra control to 'Red Green Blue'. For each R, G, B channel, in addition to Hue rotation (h) and Saturation (s) adjustments, you can adjust the Brightness (Q) using a curve.


### 'Red Green Blue' allows you to either
* Correct any color deviations due to either: gamut overshoot following various adjustments in the process, or poor white balance.
* Achieve a better balance of colors and luminances to satisfy your preferences.
* Film Simulation: with or without combined use in the 'Color' panel > Film Simulation > HaldCLUT (for example, using Kodak Kodachrome 25 film). 

The system operates after the first process (Scene) which converts the 'Working profile' data into that of 'CAM16'. At this stage, the J, Q, C, s, M, h data take into account the physiological aspects considered by CIECAM, and a chromatic adaptation is performed.

For each main color of CIECAM ac-bc [chroma based] hue plane - Red, Green, Blue - you can perform a slight hue (h) rotation in degrees (sliders) , adjust the saturation (s) as a percentage (sliders) , and adjust brightness (Q) (curves).

As a general rule, small adjustments are sufficient: 1° to 3° for hue rotations (h), 5 to 10% more or less for saturation (s), very limited variations in brightness (Q). Of course, if you are looking for special effects, such as simulating a film, the settings can be more pronounced.

These adjustments are made just before the other CIECAM adjustments: Lightness (J), Brightness (Q), Chroma (C), Saturation (s), Colorfullness (M), hue (h), contrast (J), contrast (Q) and curves and are added to them. 

Next, the third process converts the data from the 'CAM16' color space to the 'Working profile', taking into account the Viewing conditions. Color adaptation is then performed.

These hue, saturation and brightness adjustments do not use primaries. They are based on the principle of CAM (Color Appearance Models) in 3 processes.

To simplify use, I've only included one slider per channel for hue rotation, and one slider per channel for Saturation (s). I could have also included a tone equalizer for the red, green, and blue ranges... If that proves useful, aside from complicating the interface, it doesn't pose any problem. Note that the 3 Brightness curves allow you to adjust the brightness and contrast for each color range. Specifically, brightness acts on the perceived chroma via the (s) Saturation function.

**Threshold Brightness Curves**

 The ‘Threshold Brightness Curves’ aims to limit artifacts due to variations in color differences.

<figure>
<img src="redgreenblue-thres.jpg" title="redgreenblue-thres.jpg"
width="300" />
<figcaption>red-green-blue Threshold</figcaption>
</figure>

## Gamut control (Lab + CIECAM)

- this checkbox will constrain the data into the workspace. I could
  perform this action in CIECAM mode (process \#3), but it would have
  considerably slowed down the system
- The used algorithm is the same than in "Lab adjustements", it works in
  relative colorimetry. I think that differences with what could be
  produced in CIECAM mode are minimal.
- Some adjustments of the CIECAM code are made when “Gamut control” is
  enabled.
- With [Game Changer](/tutorials/) and CIECAM 16, this control is maintained solely through compatibility.  

## Code, calculation accuracy and processing time

The code for processes \#1 and \#3 is strictly the one of CIECAM02
(M.Fairchild, Billy Biggs, ...) that I've adapted and optimized to RT,
as well as improvements to the gamut correction by Changjun Li, Esther
Perales, M Ronnier Luo and Francisco Martínez-Verdú.

The processes \#1 and \#3 are symmetrical and stacks many floating point
calculations. The use of “double” is mandatory, hence important
processing time of about 1 second per Mpix. After extensive testing, we
have insights that using float instead of double racing acceleration
allowed for processing without sacrificing image quality You can change
this setting in "Preferences / Color Management"

In terms of accuracy, I wanted to make some checks by comparing a series
of data before and after CIECAM ; the differences are very small, e.g.
an XYZ input value of 6432.456 has an output value of 6432.388, which is
correct.

## Limitations of CIECAM02 & CIECAM16

This model (CIECAM02) is not perfect, and the following limitations are
identified. They can lead for certain images to process them correctly :

- we have already seen this for the Yb settings.
- CIECAM02 is not a workspace as sRGB or Prophoto, or even Lab. So it is
  difficult to control the gamut. CIECAM is even known for its problems
  of narrow gamut, that's why unexpected results may occur to the limits
  if you're pushing up too much the sliders (J, C, s …) ; this may lead
  in critical situations (highlights, ...) to black or white spots in
  these areas. Do not hesitate to use RT's tools (highlight recovery,
  highlight reconstruction, impulse noise reduction, ...), or burned or
  black areas (raw white and black points, avoid color shift, ...)
- large workspaces (widegamut, Prophoto, ...) can lead, in some cases,
  to black areas while they won't appear in sRGB (narrowness of CIECAM's
  gamut).
- The noisy images will influence CIECAM, which will think that the
  colored dots are realities; that's why I placed CIECAM after “denoise”
- the CIECAM model favors "cones" and takes sparsely into account the
  "sticks", which means that peripheral vision is sparsely taken into
  account.
- So, with CIECAM, do not expect to find a cure for "difficult" pictures
  (overexposure, sensor's saturation, etc...). But for "normal" images
  (which is the majority), advances that seem more than significant.
- Etc...

Ciecam16 appeared in 2016, updated in Rawtherapee in 2020. It corrects
almost all the defects

## The 12 principles of CAM by R.Hunt

1.  The model should be as comprehensive as possible, so that it can be
    used in a variety of applications; but at this stage, only static
    states of adaptation should be included, because of the great
    complexity of dynamic effects.
2.  The model should cover a wide range of stimulus intensities, from
    very dark object colors to very bright self-luminous color. This
    means that the dynamic response function must have a maximum, and
    cannot be a simple logarithmic or power function.
3.  The model should cover a wide range of adapting intensities, from
    very low scotopic levels, such as occur in starlight, to very high
    photopic levels, such as occur in sunlight. This means that rod
    vision should be included in the model; but because many
    applications will be such that rod vision is negligible, the model
    should be usable in a mode that does not include rod vision.
4.  The model should cover a wide range of viewing conditions, including
    backgrounds of different luminance factors, and dark, dim, and
    average surrounds. It is necessary to cover the different surrounds
    because of their widespread use in projected and self-luminous
    displays.
5.  For ease of use, the spectral sensitivities of the cones should be a
    linear transformation of the CIE x , y , z or x 10 , y 10 , z 10
    functions, and the V’() function should be used for the spectral
    sensitivity of the rods. Because scotopic photometric data is often
    unknown, methods of providing approximate scotopic values should be
    provided.
6.  The model should be able to provide for any degree of adaptation
    between complete and none, for cognitive factors, and for the
    Helson- Judd effect, as options.
7.  The model should give predictions of hue (both as hue-angle, and as
    hue-quadrature), brightness, lightness, saturation, chroma, and
    colorfulness.
8.  The model should be capable of being operated in a reverse mode.
9.  The model should be no more complicated than is necessary to meet
    the above requirements.
10. Any simplified version of the model, intended for particular
    applications, should give the same predictions as the complete model
    for some specified set of conditions.
11. The model should give predictions of color appearance that are not
    appreciably worse than those given by the model that is best in each
    application.
12. A version of the model should be available for application to
    unrelated colors (those seen in dark surrounds in isolation from
    other colors).

## Some links

CIECAM02 Wikipedia [4](http://en.wikipedia.org/wiki/CIECAM02)

Color Appearance Model - Fairchild
[5](http://www.cis.rit.edu/fairchild/PDFs/AppearanceLec.pdf)

[CIECAM Handprint](https://www.handprint.com/HP/WCL/color7.html#CIECAM)

[CIECAM Ming Ronnier Luo and Changjun Li](https://cielab.xyz/pdf/CIECAM02_and_Its_Recent_Developments.pdf)
