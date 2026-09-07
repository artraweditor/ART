---
title: White Balance
contributors:
  - DrSlony
  - XavAL
  - Jdc
tags:
  - 'Tool Description'
  - 'Color Tab'
toc: true
---

## Introduction

Digital images generally consist of a mixture of the three primary
colors: red, green and blue. For various reasons which you can read
about in-depth elsewhere, the red, green and blue values which serve as
the starting point in any raw photo development program need to be
corrected in various ways before they resemble the photographed scene.
One of these corrections is performed by adjusting the white balance -
ensuring that neutral-colored (white) objects in the photographed scene
still appear neutral on the photograph. Adjusting the white balance
affects all colors, though it is easiest to discern whether the white
balance is correct if an object you know to be of a neutral (white,
gray) color looks non-neutral.

White balancing works by multiplying each of the primary colors by a
different amount, until a satisfactory result is reached. In order to
make this operation more human-friendly, instead of operating on the
three multipliers directly, the user is presented with an abstraction in
the form of a temperature slider which adjusts colors along a
blue-yellow axis, and a tint slider which adjusts them along the
magenta-green axis.

A neutral color is one whose red, green and blue values are equal. For
example, R=G=B=65% and R=G=B=90% are both neutral, the former being
darker than the latter. You can tell whether the white balance of a spot
which should be neutral is correct by checking whether that spot's RGB
values match, or whether the a\* and b\* values in the L\*a\*b\* color
space match, or whether the RGB indicator bars under the main histogram
are directly over each other. You can do this even if you have a very
miscalibrated monitor. Your perception of color changes depending on the
color of the surroundings and of the illumination in your room, so don't
always trust your eyes - verify using the method described above.

Having an incorrect white balance results in the image having a color
tint, typically warmer (orange) or colder (blue). Some people use this
for creative effect, however there are various tools and operations
which rely on the assumption that the white balance of the image is
correct (for example highlight recovery in the
[Exposure](exposure) tool, skin targeting in the [Contrast by Detail Levels](contrast_by_detail_levels) tool, sky targeting
in the [Wavelets](wavelets) tool, the
[CIECAM02](ciecam02) tool), so you should not misuse the
white balance tool to create a color cast for artistic effect but rather
use it to ensure that neutral areas remain neutral, and then use [Color Toning](color_toning) or any of the other tools to render a
creative color tint.

The white balance tool can be turned on/off. When off, the multipliers
are set to R=1 G=1 B=1 when working with raw files. This can be useful
for diagnostic purposes or when working with UniWB images.

## Interface Description

### Method

- [Wb-camera.png](/images/wb-camera.png) Camera

  Takes the white balance used by the camera. If you shoot only in raw
  (so no raw+JPG), put the white balance settings of your camera on
  auto. This should generally give good results.
- [Wb-auto.png](/images/wb-auto.png) Automatic
  - RGB grey

    Automatically corrects the white balance, by assuming that the
    average color of the scene is neutral gray. Works well for a wide
    range of scenes, and can be a good starting point for manual
    adjustments.
  - Temperature correlation

    Provides a generally better color balance than auto “RGB grey". The
    algorithm is based on the best correlation (Student's test) between
    the colors of the image and an array of 200 spectral reference
    colors.

    - This algorithm may give erroneous results:
      - If the illuminant does not have a CRI (Color Rendering Index)
        close to 100, e.g. "Underwater", "Fluorescent", "Led" lighting
        conditions may give bad results.
      - Some DNG-type files obtained after conversion with a DNG or
        other converter.
      - If the shooting conditions are extreme (very low luminance
        values, etc.).
    - The GUI displays the correlation value:
      - A value of 1000 means either that calculation is not performed
        again and that the previous results are used, or that the
        algorithm has failed to compute a result in which case T=5002 is
        displayed.
      - Values less than 0.01 are good.
    - You can use "Awb temperature bias" to adjust the results. Each
      movement of this command brings a new calculation of temperature,
      tint and correlation.
    - A description of the Itcwb algorithim can be found here

      [algorithm](/white_balance/#the-temperature-correlation-algorithm)

- [Wb-custom.png](/images/wb-custom.png) Custom


Set your own color temperature and green tint by moving the two sliders
and/or using the Spot WB tool.

- Light source presets
  - [Wb-sun.png](/images/wb-sun.png) Daylight (Sunny)
  - [Wb-cloudy.png](/images/wb-cloudy.png) Cloudy
  - [Wb-shade.png](/images/wb-shade.png) Shade
  - [Wb-water.png](/images/wb-water.png) Underwater
  - [Wb-tungsten.png](/images/wb-tungsten.png) Tungsten
  - [Wb-fluorescent.png](/images/wb-fluorescent.png)
    Fluorescent
  - [Wb-lamp.png](/images/wb-lamp.png) Lamp
  - [Wb-led.png](/images/wb-led.png) LED
  - [Wb-flash.png](/images/wb-flash.png) Flash

### Pick

<figure>
<img src="/images/white_balance_1_before.png"
title="white_balance_1_before.png" />
<figcaption>white_balance_1_before.png</figcaption>
</figure>

<figure>
<img src="/images/white_balance_1_after.png"
title="white_balance_1_after.png" />
<figcaption>white_balance_1_after.png</figcaption>
</figure>

When you click on the Pick button
![<File:Color-picker.png>](/images/Color-picker.png "File:Color-picker.png")
(shortcut: **w**), the cursor changes into a pipette when it's over the
preview. Click on a neutral area to set the correct white balance for
the whole image based on the clicked area.

Pick a spot which should have a neutral tone - gray or white. This spot
should not be clipped in any of the three channels, as clipping means
that information from the clipped channel is missing. As far as white
balancing is concerned, "white" does not mean R=100% G=100% B=100% as
that would be clipped, but instead means a shade of gray - even a very
light one, but still one without any clipping. The picked spot should
also not be black, as black means that insufficient data was captured
for that area, and so a correct white balance calculation cannot be
performed.

You can use the picker multiple times on different places in the photo
until you find an ideal spot. Use the *Size* drop-down box to change the
size of the pipette.

This tool can be used as well inside a detail window. Right-click to
cancel the tool and to get the regular cursor back.

### Temperature and Tint

The temperature slider adjusts colors along the blue-yellow axis. Moving
it to the left makes the image cooler (bluish); moving it to the right
makes it warmer (yellowish).

The tint slider adjusts colors along the magenta-green axis. Moving it
to the left makes the image more magenta; moving it to the right - more
green.

### Blue/Red Equalizer

The red/blue equalizer allows to deviate from the normal behavior of
"white balance", via increase or decrease of the ratio between red and
blue. This can be useful when shooting conditions are far from the
standard illuminant, e.g. underwater, or are far from conditions where
calibrations were performed, for which the color matrices in the input
profile are unsuitable.

### AWB Temperature Bias

The auto white balance temperature bias slider allows you to specify how
much the automatically-calculated temperature should deviate. Use this
if you would like the automatically-calculated white balance to be
cooler or warmer.

## White Balance Connection to Exposure

The white balance is described in temperature and tint, but when working
with raw images it will be translated into weights of the red, green and
blue channels. The weights will be adjusted so that the channel with the
smallest weight reaches clipping in the working space (usually ProPhoto
RGB) when the raw channel is clipped. In other words, with exposure set
to 0.0 and no highlight recovery enabled the full visible range is fully
defined by the raw backing. As white-balancing changes the weights you
may see a slight exposure change if you make drastic changes to white
balance.

## The Temperature Correlation Algorithm

This section is a technical description of the temperature correlation
algorithm and its implementation. It is not necessary to know this if
you just want to use the "Automatic \> Temperature Correlation" white
balance method, but will be of interest to those studying the matter.

This algorithm is referred to in abbreviation as "ITCWB".

Unlike the majority of white balance algorithms based on gray tones,
this one is based on color. Put simply, the algorithm compares a large
number of sample colors in the image with a set of reference colors and
their associated spectral data.

### Origin

This algorithm was developed by Jacques Desmis. It was based off an
unpublished research summary, which divides the process up into 3
phases:

1.  xyY comparison
2.  Spectral data analysis
3.  Color histogram analysis

These phases form the basis of the algorithm described below, which was
developed from scratch and is not based on any existing algorithms or
code.

### Performance

The performance of the algorithm depends on:

- The choice of colors in the image obtained by sampling and selecting
  the dominant colors (skin, sky, plants etc.).
- The determination of certain parameters, which will be used as the
  basis for the calculations i.e. camera white-balance temperature,
  which acts on the red and blue components and tint, which acts on the
  magenta and green components, etc.
- The choice of the RGB channel multipliers and their calculation based
  on the temperature of the illuminant.
- The calculation of the XY values of the reference colors (spectral
  data), using an "exact" formula and samples of spectral data at 5nm.
  Matrix \[Color seen\] = Matrix \[illuminant\] \* Matrix \[color\] /
  Matrix \[Observer 2°\].
- Multiple iteration of the calculations taking into account, in equal
  proportions, the balance between green-magenta and red-blue.
- Rigorous calculations if the illuminant has a CRI (Color Rendering
  Index) close to 100 i.e. illuminant close to Daylight in the limit
  4000K - 15000K or Blackbody from 2000K to 4000K.
- Statistical correlation using a Student's test.

### Reference Spectral Colors

The origin and nature of the 429 reference spectral colors:

- Data found on the web for flowers, foliage.
- A ColorChecker24 or other color patches.
- The 468 calibration chart that I developed for calibration a few years
  ago.
- The Colorlab utility (Logo Gmbh).
- These colors are distributed almost equally over the entire color
  palette (Red, Orange, Yellow, Green, Cyan, Blue, Magenta…).
- These colors are also sorted into neutral or close to gray, slightly
  saturated, pastel and saturated.
- The luminance has little significance because the comparison is made
  on the chroma component.

### General Principles

- Using the RGB values just after demosaicing, 3 tables are generated
  (Red, Green, & Blue) for 1 pixel out of every 3 in the image
  (horizontally and vertically). It is possible to change this value if
  necessary for more precision. The values are then adjusted so that
  they are in the range 0 to 65535.
- Then we switch to a procedure called "autowb", which is common to both
  automatic white balance algorithms. It calculates the RGB channel
  multipliers, and passes on the values to either to "Itcwb" or
  "rgbgray".
- The parameters that "wbauto" passes on to "Itcwb" include the
  important reference temperature (the value present in the Exif camera
  data) and the tint (also present in the Exif data), whose values are
  limited to the range between 0.77 and 1.30. There is no Daylight or
  Blackbody illuminant beyond these arbitrary limits and any
  calculations would therefore be fanciful or false.

### Simplified Temperature Correlation Algorithm

1.  Phase one
    1.  Calculate the RGB multipliers for each temperature between 2000K
        and 15000K (blackbody and daylight illuminant's) and for the
        tint.
    2.  Calculate the XY values from the 429 spectral-data values for
        each temperature.
    3.  Select a temperature data range relative to the reference.
    4.  Calculate the xy values in the form of a histogram and select
        from among the 236 possible values, the most commonly used
        colors (skin, sky, etc.) for each temperature.
    5.  Sort the data in ascending numerical order.
    6.  For the most frequently occurring data values, calculate the
        chromatic values of the image.
    7.  Use the deltaE chroma values to select the reference colors from
        the 429 available possibilities.
    8.  Calculate the reference RGB values as a function of the
        reference temperature.
2.  Phase two
    1.  Calculate the XY values for each selected reference color as a
        function of temperature and tint.
    2.  Calculate the RGB values of the image from the XY values using
        the RGB multipliers.
    3.  First calculation of the Student correlation.
    4.  For each tint and temperature range, calculate the channel
        multipliers and the XY values from the corresponding spectral
        data.
    5.  Calculate the correlation coefficient as a function of the color
        green.
    6.  Sort these values.
    7.  Optimize the values to determine the correct temperature and
        tint values.
    8.  Send these parameters to "wbauto”.
    9.  Display the results and update Improccoordinator.cc.

#### Latest Improvements 05 2023

- By default, because it is necessary to have a starting reference for
  the algorithm, the parameters chosen are those of "Camera"
  (temperature and tint). It turns out that in a few cases, these values
  are obviously wrong. In case "camera" tint is higher than 1.5 or
  "Camera" temperature is lower than 3300K or "Camera" temperature is
  higher than 7700K, the new starting reference is the one calculated
  with "Automatic RGB grey"(or a mix "Camera" and "Automatic RGB grey").
- When "Camera" temperature is lower than 4000K or "Camera" temperature
  is higher than 6000K, a 2 pass process of the algorithm is set up,
  looking if another value far enough away gives better results. When
  "Camera" temperature is higher than 4000K or "Camera" temperature is
  lower than 6000K, another 2-pass process is implemented with smaller
  deviations from "Camera".
- If you activate "Low sampling & No use Camera settings", upstream of
  the algorithm and in the algorithm, the system will use a mix of the
  "Camera" and "Auto WB grey" settings
- the calculation of green (hue) has been revised;
- an attempt to optimize the patch is made from the chromatic analysis
  of the image (deviation from the white point). The lower the value,
  the more credible the patch is in theory.
- before developing the histogram of the image data, a slight denoising
  (median 3x3) is applied.
- a gamut control is also applied when calculating the histogram data,
  eliminating outliers.

#### Data displayed in the GUI - limitations of interpretation

- Multipliers r, g, b. These are given for information only and cannot
  be modified;
- Correlation factor: gives a probability of correlation between the
  image data and the spectral data. This coefficient is indicative,
  because it assumes that the patch is "good", but this patch is
  relatively indeterminate: what data to take? On what amplitude?
  Certainly it will translate, for a choice of temperature / start-up
  shade, the best compromise, but is the choice of start-up optimum?
- Passes ( 1 or 2) and Alt_temp: displays the number of passes of the
  algorithm carried out as well as the possible alternative temperature.
  In the "2 passes" case, the "Remove 2 passes algorithm" check box is
  active, otherwise it is inactive.
- Read colors: number of colors read in the image (depends on
  "Sampling"), the maximum is 237 which covers the entire CIExy diagram.
- Chroma patch: indicator that translates the attempt to optimize the
  patch. A weighting according to an exponential law, affects the values
  to try to take into account "at best" the flat areas (sky, skin, etc.)
  and the more isolated data.
- Size: number of colors chosen at once in the image (from the highest
  in number of corresponding pixels, to the lowest).
- patch ΔE: shows the deltaE of the patch between the image data and the
  spectral data.
- datas x9: displays the number of records found for each color. Maximum
  and minimum. The absolute minimum is (arbitrarily) set at 400. To have
  a real evaluation, multiply these values by 9, because only 1 pixel
  out of 3 (horizontal and vertical) is analyzed to minimize the
  processing time.

##### Interpretation limits

The Itcwb algorithm is complex, but devoid of intelligence. There is no
interpretation of the image: is it a portrait? a landscape ? what is the
nature and distribution of the data? So it is necessary to be very
careful on the "steering" of the system by these indicators. Certainly
they seem robust in a majority of cases but there are many exceptions:

- just because "chroma patch" is lower doesn't mean the patch is
  necessarily better. This indicator shows for a "temperature / tint"
  couple a possible optimization, but is it the best? It will allow you
  to choose the size of the patch.
- it is not because "deltaE" is the lowest, that this choice is the most
  optimal. Admittedly, it translates for a given temperature/tint
  amplitude around a reference value, the optimization of the
  deviations, but that does not demonstrate that it is the best choice.
- it is not because "Correlation factor" is the lowest that the result
  is optimal. This correlation translates the optimization of a "hue"
  and a temperature around a reference value, but this does not
  demonstrate that it is the best choice.

The system first determines (before approaching the algorithm itself)
which base references to use (Camera or auto WB grey). Then he
determines if he will use 1 or 2 passes. In the first step, for each
pass (or only one) the optimization of the patch is done with "patch
chroma". Then the algorithm tests a range of temperature and tint and
calculates deltaE of the patch and correlation. A compromise is found by
taking into account the minimum of the multiplication of the detaE and
the correlation. On the other hand, when we are far from D65 (value
taken into account by Adobe) to determine the color matrix, the
probability that the data read and interpreted are incorrect increases
and the results displayed are probably flawed. There is currently no
possibility in Rawtherapee to read 2 color matrices. Even if this
possibility existed, the work of calculating and entering the stdA color
matrices would be considerable.

There will be images that will (obviously) not be processed well. For
example, images with one or two strong colors (red, yellow, purple..)
and virtually no white (gray) will be poorly optimized. For example the
flowers on a foliage background. Similarly illuminants far from Daylight
(LED) or illuminant mixtures will lead to incorrect results.

### Using AWB Temperature bias

AWB temperature bias is a simple way to get a quick result.

Indeed, acting on this slider completely reexecutes the algorithm by
shifting the initialization temperature. All parameters are
re-calculated temperature, tint, correlation.

- it can be to readjust the colorimetry - a bit like CIECAM chromatic
  adaptation, but it's not exactly the same thing.
- or to visually obtain an image more in line with your expectations,
  without color shifts.
- it has a role similar for "temperature" to "Green refinement".

### User-Modifiable Settings

By default the settings should be suitable in most cases. However, it is
possible to make custom modifications to the operation of the algorithm.

In the Color / White Balance tab you can make a series of settings
appear that allow you to adapt the algorithm. Eventually (I hope) to be
able to remove the majority of these settings (perhaps all). The purpose
of this provision of these settings (optional) is essentially the
development of the algorithm.

To make these additional settings appear, go to Preferences / Color
Management / White balance - Automatic temperature correlation, and
check the corresponding box. If "Temperature correlation" is not
selected in "White Balance", the choices appear in gray.

#### Relevant Parameters

I think I can give a relevant opinion, being the designer of the
algorithm. Nevertheless, I leave the 'door open' to other hypotheses
because the colorimetry in general and the adaptation to Rawtherapee can
reserve hypotheses.

Parameters having an influence (a priori):

- The 3x3 color matrix
  - The 3x3 matrix ensures the conversion of raw Raw data, into useful
    data. In Rawtherapee, these indispensable matrices which are used in
    several algorithms of the Raw part have their origin in Adobe,
    either they come from Dcraw, or they come from Adobe DNG converter
    (Tag matrix). How were these matrices obtained by Adobe ? Internal
    research, link with the manufacturer,... ? One can think that these
    matrices are built in a common process, and thus that all things
    being equal, the same scene, under the same illuminant, taken under
    the same conditions (speed, diaphragm, lens,...), with a common Raw
    process, must bring usable color data (and not luminance or Dynamic
    Range) Raw approximately identical (with the possible reserves due
    to the parameters developed below). That it is a matrix of Bayer, or
    other, a case of mark X or Y. Obviously there are differences, in
    particular for the non-Bayer matrices which have a specific
    demosaicing algorithm, as well as for the optics... but the
    differences must be minimal.
  - This matrix is for illuminant D65. In the exifs there is a second
    matrix for the illuminant stdA (tungsten). A test carried out on a
    raw at 2600K, leads me to think that this choice would be (a little)
    better. However Rawtherapee only knows how to manage a color matrix.
    On the other hand how would one make the choice? And how to update
    the hundreds of cameras?
  - This matrix is "the factor" that identifies make, model. It allows
    (unless this matrix is poorly calculated, a rare case…) to say that
    the brand and the model are taken into account and that they have no
    influence on the algorithm. This does not mean that the brand, the
    model are not important, obviously they are in the gamut sense. But
    it is taken into account by the algorithm if the spectral data are
    sufficient.
- The number of spectral colors: it seems obvious that the more the
  perimeter of the image data is increased (reading the xyY data in the
  CIExy diagram), the newer the devices will be with a very wide gamut,
  the more spectral data will be needed matches – otherwise the
  algorithm malfunctions. The number of spectral color data has
  increased from 201 until the beginning of 2023, to 429 currently.
  Maybe more data is needed?
- The demosaic algorithm also has a influence, especially the algorithms
  designed to handle noisy images (LMSSE, IGV), and of course the
  demosaic algorithm for non-Bayer, for example Xtrans-demosaic (Frank
  Markesteijn's algorithm, and Ingo Weirich). Each demosaicing algorithm
  has its particularities (this is not the place to discuss it here),
  but brings its own colorimetry. Between several methods (Amaze, DCB,
  VNG4,...) there are differences which are quantified in delatE, and
  induce different colors. This will affect Itcwb. Similarly, Raw
  processing performed before Itcwb also has a certain impact, for
  example Capture Sharpening and chromatic aberration correction. But
  this is not a malfunction, on the contrary it shows that the algorithm
  works and takes into account the color changes induced by these
  processes.
- Each sensor - associated with a housing - has :
  - a specific DR (Dynamic range) - often around 12Ev for older cameras,
    close to 15 or 16Ev for recent cameras. This DR must have a low
    impact on the algorithm, the luminance component is almost ignored
    (otherwise to determine the gamut).
  - White-levels and Black-levels : if these 2 components have a strong
    importance for the whole of the Raw processing, except if they are
    very badly adjusted, they should not have an influence on the
    algorithm.
  - the gamut of the sensor : I do not know of official documents
    showing the limits of transcription of the colors. It is reasonable
    to think that before conversion matrix, these limits are largely
    sufficient and beyond Prophoto, and thus have no or little influence
    on the algorithm (not on the result of course).
- The nature and intensity of colors: the distribution in the xyY
  diagram. Are we with an image where the colors are close to the white
  point (pastel colors or neutral), or images with colors at the limits
  of those of human perception?
- The distribution of these colors in relation to each of the primary
  Red / Green / Blue.
- As the algorithm divides the 'xy' space into 236 areas, covering the
  entire visible space, the algorithm must take into account this
  distribution: for example a majority of tones very close to the white
  point (neutral) or a dominant important (sky, or skin).
- And of course the illuminants. An image with parts in the sun and
  shade is actually with 2 illuminants Daylights (near 5000K in the sun
  and 7000K in the shade). This is almost insoluble with a single
  setting of the white balance (temperature, hue). Of course it is even
  more complex if Fluorescent or LED illuminants are present. But these
  remarks are not specific to Itcwb, but to all white balance
  algorithms. Local adjustments (Warm/cool) allows you to correct double
  illuminants (sun, shadow...) quite well.
- Still on the subject of illuminants, they have a theoretical
  definition that links spectral data to a temperature. These formulas
  by principle can not be perfect and respond to all environments:
  latitude, altitude, time, meteorological conditions (fog, gradients,
  ...) that must have an impact on the shade (green) that becomes
  different from 1.
- Observer 2° (1931) or Observer 10° (1964): the second provides a
  better perception of human vision.
- As a reminder, a perceived color with its XYZ data is the combination
  of 3 matrices:
  - spectral data of the illuminant (function of temperature and nature
    of the illuminant).
  - spectral data of the base color (measured with the spectro).
  - spectral data of the observer (2° or 10°).

By principle this algorithm is not designed to correct the malfunctions
of the processing (which is always very complex). Of course it can
(possibly) correct a problem, but this is not its purpose.

#### Settings

- CIExy diagram and gamut: You can see on the 2 diagrams below, that it
  is not because a color is in the CIExy diagram that it is in the
  gamut.
  - Example for a luminance of 10 and a luminance of 50 \[0..100\].

<figure>
<img src="/images/gamu-comp10.jpg" title="gamu-comp10.jpg" />
<figcaption>gamu-comp10.jpg</figcaption>
</figure>

<figure>
<img src="/images/gamu-comp50.jpg" title="gamu-comp50.jpg" />
<figcaption>gamu-comp50.jpg</figcaption>
</figure>

<figure>
<img src="/images/rec2020_pointer.jpg" title="rec2020_pointer.jpg" />
<figcaption>rec2020_pointer.jpg</figcaption>
</figure>

<figure>
<img src="/images/srgb2jdcmax.jpg" title="srgb2jdcmax.jpg" />
<figcaption>srgb2jdcmax.jpg</figcaption>
</figure>

Impact settings: After several weeks of user testing and optimization of
the algorithm, 3 parameters remain directly accessible to the user:

- Green refinement : Allows you to change the "tint" (green) which will
  serve as a reference when starting the algorithm. It has substantially
  the same role for greens as "AWB temperature bias" for temperature.
  The whole algorithm is recalculated. Depending on the case, in order
  to guide the algorithm, the action on "Remove 2 pass algorithm" may be
  necessary.
- Remove 2 passes algorithm: this checkbox, manages a complex background
  process, which leads to that in cases where there is only one pass
  chosen by the algorithm, the action on this box to check has no
  effect. In the case where 2 passes are detected, this checkbox allows
  switching from one setting to another. Be careful, when one of the 2
  settings comes out towards temperatures close to stdA (Tunsgtene
  2855K), the indicators (correlation deltaE..) must be taken with care,
  the results are probably marred by errors linked to the color matrix.

<!-- -->

- Choice of image data sampling: 2 choices are offered:
  - Low sampling – limits data to sRGB values. In some conditions (green
    camera \> 0.8), forces the algorithm in some cases not to use camera
    settings.
  - Medium sampling (default)- near Pointers's gamut. Beta RGB primaries
    are used, thus obtaining a color range close to human vision in
    reflected color (Subtractive color mixing). I could also have chosen
    Rec2020 which is a bit "bigger" than "Beta RGB".
  - Camera XYZ matrix - uses the matrix directly derived from Color
    matrix (Adobe)
  - Close to full CIE diagram – the limit is that of JDCmax, i.e. close
    to entire CIE diagram.
  - These 3 samplings have no relation to the 'working profile' and have
    no effect on the rest of the processing, except on the balance of
    the white balance.
  - I recommend, insofar as there is enough spectral data to choose this
    last choice 'Close to full CIE diagram', even if it generates
    imaginary colors (most in greens), because it is much in the
    continuity of the process before the application of the "working
    profile". The exceptions can be of 2 orders: a) an image contains
    data which is not referenced in spectral values (must be very rare)
    and in this case the deltaE and correlations will be biased; b) you
    voluntarily wish not to take into account parts of the image with
    high gamut, which could disturb the result or no use of camera
    settings.
  - Note that when there is no "Input profile" (or when the user chooses
    "Camera standard") the reference data used for sampling are
    processed in such a way as to be similar to those used later.

Settings accessible from pp3:

- Itcwb_findgreen - Find green student: number of iterations to find the
  best compromise between the correlation (student) and the value of
  green which for Daylight / Blackbody illuminants is close to 1.
  Default: 3. Range of settings taken into account \[2..5\]. It seems
  that the value 3 is a good compromise, which would make it possible to
  remove this setting.
- No purple color used: by default this setting is not taken into
  account. If the image needs highlight reconstruction it may need to be
  enabled.
- itcwb_minsize: set by default to 20. Sets the minimum value for the
  patch size.
- Itcwb_rgreen - Geen range: sets the amplitude of examination of the
  value of green in the iterations, from a low amplitude of 0.82 to 1.25
  up to a maximum of amplitude 0.4 to 4. Default: 1. Range of settings
  taken into account \[0..3\]
- Itcwb_delta - Delta temperature in green loop: Fixed for each "green"
  iteration tried, the temperature difference to be taken into account.
  Default: 4. Range of settings taken into account \[1..8\]

Settings accessible from the 'options' file

- Itcwb_deltaspec: if Verbose is "true", displays the results (in the
  console) of the image and spectral data whose deviation in deltaE is
  greater than this value. Default 0.075.
- Itcwb_maxsize: sets the maximum patch size. By default the value is
  70.

#### Chromatic Adaptation

The results of the Itcwb algorithm, reflect the relevance of
calculations on objective mathematical foundations. but this result - as
indeed all settings of the white balance - do not take into account in
full human perception: surround, simultaneous contrast, ... and
especially the adaptation of our eye / brain to temperature differences
from D50 (which is the reference in colorimetry). To overcome this gap,
it is possible to set up a chromatic adaptation "integrated" with the
white balance (this is what I had proposed in 2018...) or let the user
set up this adaptation with the module "Color Appearance & Lighting".

To achieve this and limit the role of Ciecam, put Ciecam in "Automatic
symmetric" mode, the system will apply 2 chromatic adaptations, the
first from "Scene conditions" to the reference illuminant (usually D50,
but you can change it), the second from the reference illuminant to
"Viewing conditions". By default the 2 adaptation percentages are set to
90%, you can increase or decrease these values. You can also modify the
temperature in "Viewing conditions" to obtain a warmer or colder
rendering. You can, if you wish, change the Ciecam settings like
"Absolute luminance", "Surround", etc. See the tutorial on Ciecam.

### Examples

I have (arbitrarily) chosen these 6 examples, to show what Itcwb can
(and cannot) do, associated or not with Color Appearance & Lighting.

- [Salt mountain in Turkey](https://drive.google.com/file/d/1azCxu1midw6dcuN7SbvbAiJH4pxX5BTA/view?usp=sharing)
  (`_ASC4145.NEF` CC BY-SA 4.0 Jacques Desmis)

  This image that seems harmless is complex in terms of photography, for
  several reasons:

  - the white of the salt mountain is difficult to process, and is
    affected by a complex structure that makes it difficult to process
    in general.
  - The point that interests us here is the white balance and color
    distribution. The majority of the image sky, trees, mountain is in
    sRGB, but the flowers at the bottom of the image (red, orange,
    yellow) are well beyond: how to treat, incidences of settings
- [Lunching Room](https://drive.google.com/file/d/1MMNzw3tPQuMeD5baqDlBXRvl4lDy2mLX/view?usp=sharing)
  (`LunchingRoom.CR2` CC BY-SA 4.0 Rawtherapee)

  This image shows that the algorithm can handle complex situations.

  - By default with White Balance set to "Camera" the image is green.
  - Try successively, White Balance auto : 'Rgb grey' and "Temperature
    correlation".
- [London Bridge](https://drive.google.com/file/d/1CiQ2t4KyD3tdCiNNhskqUG2cH9LT2ly7/view?usp=sharing)
  (`london_bridge_moving_1.pef` CC BY-SA 4.0 Maciej Dworak)

  This image shows both the need for chromatic adaptation and the
  relevance of the Itcwb algorithm.

  - The default setting 'Camera' gives a green/yellowish cast.
  - Itcwb allows to find a good mathematical compromise, but the
    temperature is high, giving the image a warm coloring, which can be
    seen on the faces, the deck stays.
  - Try Color Appearance in "Automatic symmetric" mode
- [Calibration test pattern](https://drive.google.com/file/d/1YFeoPL-RhStftDCCkbDNlmj8New_SgX0/view?usp=share_link)
  (`DSCF5334.RAF` CC BY-SA 4.0 RawTherapee)
  - I chose this test pattern for two reasons: the first is that it is a
    RAF file, so it is not a Bayer file; the second is that it contains
    almost pure whites and blacks (the grays are dominant in the image,
    which could interfere with the algorithm).
  - Nevertheless it does not seem to examine the results (neutral) that
    the camera has been calibrated (or I do not have the profile), the
    information is therefore orders of magnitude, but quite close to
    reality (whites, blacks, ColorChecker)
  - Itcwb doesn't know it's a calibration test pattern, try because of
    the high gamut 'Force use of the entire CIE diagram' on or off,
    'Sort in chroma order' off or on and 'no purple used' off - the
    results are very close to those with Camera (which are better?), but
    there is no drift.
- [Caribbean Backlight](https://drive.google.com/file/d/1iqpj-vT3dqUmaXKOQE7QB_rynf4cbRPC/view?usp=share_link)
  (`DSC02973.ARW` CC BY-SA 4.0 Jacques Desmis)
  - I chose this image, taken with my old Sony during a trip to the
    Caribbean, where I had chosen the automatic white balance.
  - Try with 'Camera' then 'Itcwb'.
- [Using Inpaint-opposed](https://drive.google.com/file/d/1ZxucMREXAAFlBijiBiBZoH9kG4-O2GYb/view?usp=share_link)
  (`Nikon - D800 - 14bit uncompressed (3_2).NEF` CC0 1.0 Pascal Obry)
  - I chose this image to show the impact of the white balance on
    "Inpaint opposed" (highlight reconstruction), especially the green
    (tint) influence
  - Try "Inpaint opposed" with White Balance set to Camera, notice the
    large artifacts in the sky
  - Choose "Itcwb" and try various settings.

### 5.9 Compatibility

The Temperature Correlation Algorithm (Itcwb) versions after 5.9 have
limited compatibility with version 5.9:

- Only the "Observer 10° instead of Observer 2°" setting is available.

When the system encounters a pp3 file of version 5.9 using Itcwb, then
the "Temperature correlation" method is displayed. The only relevant
information is:

- white balance multipliers;
- temperature and tint;
- the "Observer 10° instead of Observer 2°" check box.

You can switch to the current Temperature Correlation Algorithm (Itcwb)
method by changing the "White balance" method, for example by going
through "Custom", then "Temperature correlation"
