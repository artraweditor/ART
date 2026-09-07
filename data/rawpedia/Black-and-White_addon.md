---
title: Black-and-White addon
contributors:
  - Lebarhon
  - XavAL
tags:
  - 'Tool Description'
  - 'Color Tab'
toc: true
---

## The different methods

### General remarks

The Black-and-White tool is organized in three methods, each producing a
different black and white result.

1.  Desaturation;
2.  Luminance Equalizer
3.  Channel Mixer

Please note that Rawtherapee can produce black-and-white images without
the use of this tool:

1.  by setting the [Saturation](exposure#saturation) slider
    in the [Exposure](exposure) tool of the Exposure tab to
    -100;
2.  by setting the
    [Chromaticity](lab_adjustments#chromaticity) slider in
    the [Lab Adjustments](lab_adjustments) tab to -100;
3.  by enabling [Film Simulation](film_simulation) in black
    and white (films Ilford, Kodak, Fuji...)

Nevertheless only the methods in the current tool gives you the maximum
of possibilities for a black-and-white conversion.

For a perfect gray tone, except in the case of [Color Toning](color_toning), as "Ajustements Lab" are treated at
the end of the pipeline, the values of “a” and “b” in the [Lab Adjustments](lab_adjustments) of the "Exposure" tab are set
to zero.

Note also the interaction with the [Color Toning](color_toning) tool.

### Desaturation

This method works in such a way that for each pixel (R=G=B) is given an
equivalent luminance value of L=0.299\*r + 0.587\*g + 0.114\*b. This
ensures a totally neutral gray image.

Note: the other 2 methods of desaturation in Rawtherapee (quoted above)
give other results due to different algorithms. In "Exposure" it is
channel “S” from HSV that is set to 0. in "Lab Adjustments" it is
chromacity C=sqrt(a\*a+b\*b) that is set to 0.

### Luminance Equalizer

This method use a [flat curve](general_comments_about_some_toolbox_widgets#the_flat_curve),
that allows to modify the luminance based on hue.

The algorithm uses a conversion rgb==\>LCH \[modifying L based on
H\]==\>rgb with gamut control.

Unlike some other commercial software that act only on a limited number
of colors with sliders, you can interact on the whole color palette with
Rawtherapee.

Finally, the R, G, B values are set at the same level to ensure a
perfect gray tone.

There is a gamut control, but it doesn't prevent you to obtain very good
special effects by pushing the curve to extreme values.

Here, the pipette is very useful. For example, choose with the pipette a
tone area you want to darken. This adds a control point on the curve.
Move this control point downwards to darken (or upwards to lighten) this
tone.

### Channel Mixer

At first sight this method seems to be very complex !

There is however a simple explanation: this method uses a channel mixer
in order to carefully manage the balance between the different color
components of the image, to reconcile the distribution of the lights,
mid-tones and shadows. It amounts to take a percentage of each channel
(R,G,B) and put them together !

The sensible reader with a mathematical mind will notice that the sum of
the 3 channels should be 100% to avoid clipped highlights. The same
reader will look only for positive values (because they are logical) and
no negative values.

But don't let this stop your creativity, open your mind to :

` a) values over 100%; `

`b) negative values.`

`With these two possibilities that you have to experiment with, you can create special effects and color filters such as infrared, but also some common settings such as landscapes, portrait, contrast, etc.`

#### Components of the channel mixer

##### Presets

It allows to choose between:

- predefined settings (Normal contrast, High contrast, Luminance,
  Portrait, Landscape, High and Low Sensitivity, Panchromatic, Hyper
  Panchromatic, Orthochromatic, Infrared);
- settings at user disposal based on four criteria:

1.  **Absolute RGB**: it offers to the user to mix the three channels R,
    G and B without any control for the limits. You can enter positive
    or negative values with a sum that is lower, equal to or higher than
    100%.
2.  **Relative RGB**: it offers to the user to mix the three channels R,
    G and B, but with control for the limits. You can enter positive or
    negative values, but the sum will always been forced to 100%. E.g.
    if you set R=10%, G=10%, B=30%, this is translated to R=20%, G=20%,
    B=60%. This mode is the default one for all the predefined settings
    as "Landscape": R=66% G=24% B=10%. Relative RGB is the most
    intuitive and simple setting, especially if one doesn't set negative
    values.
3.  **Absolute ROYGCBPM**: (for Red Orange Yellow Green Cyan Blue Purple
    Magenta). This “special” mixer offers 2 interesting options:
    - Tweak the complementary colors: in this case, if one acts on a
      "OYCPM" slider, a correction is automatically made on the basic
      colors (R,G,B);
    - algorithm OYCPM: if set to “Linear”, it has a strict proportional
      response to the wanted strength and acts directly and
      proportionally on the 2 basic colors, e.g. Orange acts on Red and
      Green; If set to “Special effects”, it introduces a funny effect
      in the conversion of Orange to red and Green (non-linear response,
      and possibly also on Blue depending on the values of the sliders).


    This is the least intuitive setting but with maximal creativity
    possibilities
4.  **Relative ROYGCBPM**:as above, but with a limit control to 100% for
    the 3 basic channels R,G,B

##### Color Filter

The color filter simulates shootings with a colored filter placed in
front of the lens. These filters reduce the transmission of a specific
color and have therefore an impact on the luminance. E.g. a red filter
darkens a blue sky and lightens the reds. This filter works as a
multiplicator for the settings made with “presets”.

##### Auto

This button activates an algorithm that calculates on the entire image
and strictly balance the 3 basic channels R, G, B to give them the same
relative weights.

#### Warnings

- You will notice the incidence of the final tuning of the channel mixer
  after you have made all tunings in Presets (including "Adjust
  complementary color" and ""Algorithm OYCPM"). The line under Presets
  displays 4 numbers, e.g.: R=37,2% G=-82,3% B=126,6% Total=155%. In
  this case, the global image lightness will exceeds the original one of
  55%, and each pixel will have its own values multiplicated – before
  mixing – by the 3 previous data.
- For the positive values in relative mode, the result is predictable...
  It's the usual mode for this channel mixer. You can find on the web
  values for black-and-white film simulating, e.g. “Ilford Delta 100 :
  21,42,37”, etc.
- On the contrary, in the absolute mode, negative values, the use of the
  sliders “OYCPM” and the algorithm “Special effects” can lead to
  unexpected results: black screen, artefacts, ...

## Gamma Correction

One can change the rendering of the tones for each channel (R, G, B)
using the gamma sliders. This command roughly simulates the rendering of
the paper under the enlarger (hard, normal, soft). Pushing the sliders
to the left (negative values) darkens the image and gives more contrast,
pushing to the right (positive values) softens the image. Notice: there
isn't a bug in the Blue channel: when using Prophoto as working space,
the blue channel is less active. Use the sRGB working space to see the
effect.

## 'Before' curve and 'After' curve

These curves are used the same way and have the same final result as
those described in the [Tone Curves](exposure#tone_curves)
section of the [Exposure](exposure) tool. They allow
customizing the Black-and-White tool, making it independent of the
tunings made elsewhere. Notice: the “After curve” has only one mode, as
the image is then in black-and-white!

## Color Toning

- You can use [Color Toning](color_toning) with the
  Black-and-White tool for special effects. You can also use [Color Toning](color_toning) with black-and-white film
  simulations, but provided the black-and-white tool is enabled.
- The architecture (the various tools order in the processing pipeline),
  the algorithms “Color toning” and “Black-and-White” have been adapted
  to give you the maximum of the joined effects.
- You can act simultaneously on all the possibilities in “Color toning”,
  nevertheless, [Color Balance Shadows/Midtones/Highlights](/color_toning/#color-balance-shadowsmidtoneshighlights)
  gives the most possibilities.
- Try switching between [Color Balance Shadows/Midtones/Highlights](/color_toning/#color-balance-shadowsmidtoneshighlights)
  and e.g.  [L\*a\*b\*  blending](/color_toning/#lab-blending-particularities)
  try the gamma sliders and the curves in the "Black-and-White" tool.
- Of course you have to walk through a number of trials and errors
  iterations if you are looking for special effects.
