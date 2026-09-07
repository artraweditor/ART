---
title: Vibrance
contributors:
  - DrSlony
  - XavAL
  - Jdc
tags:
  - 'Tool Description'
  - 'Color Tab'
toc: true
---

*Vibrance* is an intelligent saturation adjustment tuned to correlate
with the color sensitivity of human vision. The vibrance effect is
applied with higher accuracy on a correctly white balanced image when
the RGB *Saturation* and Lab *Chromaticity* sliders are set to 0. You
can separately control the vibrance of *pastel* tones (tones of low
saturation) and *saturated* tones (as the name implies, tones of high
saturation).

## Interface Description

### Pastel/Saturated Tones

These *Pastel Tones* and *Saturated Tones* sliders let you individually
control saturation of saturated and pastel tones.

### Pastel/Saturated Tones Threshold

The threshold adjuster lets you differentiate between pastel and
saturated tones - the threshold between what is considered a pastel tone
and what is considered to be saturated.

### Protect Skin Tones

When enabled, colors closely resembling natural skin tones are not
affected by the vibrance adjustments.

### Avoid Color Shift

When enabled, exercises extra control to avoid hue shifting.

### Link Pastel and Saturated Tones

When enabled, the vibrance level is adjusted with a single slider
equally controlling saturation of both pastel and saturated colors.

### Skin Tones - Hue According to Hue

[thumb](/images/vibrance_hh.jpg) This H=f(H) curve lets you
change the hue of skin tones. It behaves just like the tone curve you
know from any image editing program, but instead of working on a whole
color channel it just works on a range of common skin tones. The x axis
represents the input hue, and the y axis the output hue. The adjustment
range mainly covers the red and orange hues and is expressed in radians.
It starts at -0.05 radians (357.13 degrees) and ends at 1.6 radians
(91.7 degrees).

Decide on the specific skin tone hue you want to change (the one your
subject has in the photo), find it on the x axis, and then change the
curve to map it to a nicer tone on the y axis. In practical terms, you
can use this tool to map a British-pink suntan skin color to a nice
Maldives-brown one.
