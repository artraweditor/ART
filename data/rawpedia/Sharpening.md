---
title: Sharpening
contributors:
  - DrSlony
  - Ingo
tags:
  - 'Tool Description'
  - 'Detail Tab'
toc: true
---

This article describes the tool called "Sharpening", however RawTherapee
contains other tools which can be used to perform various types of
sharpening - see [Edges and Microcontrast](edges_and_microcontrast) and the
[Wavelets](wavelets) tools.

The Sharpening tool is applied to the full image, **before** the
[Resize](resize) tool. If you would like to apply sharpening
**after** resizing, use the [Post-Resize Sharpening](resize#post-resize_sharpening) tool which you
will find inside the Resize tool.

## Contrast Mask

The "contrast threshold" and "blur radius" sliders allow you to control
a mask that decides which areas are affected by sharpening and which are
left untouched. Activate the "preview the sharpening contrast mask"
button
![contrastmask-off.png>](contrastmask-off.png "contrastmask-off.png")
(in the toolbar above the main preview) to see this mask.

## Methods

### Unsharp Mask

[Unsharp masking](https://en.wikipedia.org/wiki/Unsharp_mask) (USM) is a
technique used to increase the apparent
[acutance](https://en.wikipedia.org/wiki/Acutance) (edge contrast) of an
image, making it appear clearer, even though it technically does not
really sharpen the image. It makes use of several phenomena of the human
visual system in order to accomplish this effect, such as the
[Cornsweet illusion](https://en.wikipedia.org/wiki/Cornsweet_illusion) and
[Mach bands](https://en.wikipedia.org/wiki/Mach_bands). Though unsharp masking
in other software is easily prone to causing
[halos](https://en.wikipedia.org/wiki/Haloing), RawTherapee has a unique
threshold slider which allows you to achieve a superb sharpening effect
with a minimal risk of halos.

#### Radius

The Radius determines the size of the details being amplified and,
consequently, relates to the width of the sharpening halo. In general
the quality of sharpening is best if the sharpening radius is smaller.
For low ISO images that are in focus and without motion blur a value of
0.5-0.7 is satisfactory.

#### Threshold

[usm_threshold.png](usm_threshold.png)

The *Threshold* tool helps to suppress noise amplification and haloing
and to confine sharpening to a desired tonal range. The Threshold tool
allows one to create a curve via which the sharpening is applied. The
vertical axis corresponds to opacity: 0% at the bottom (transparent,
sharpening not visible), 100% at the top (opaque, sharpening visible).
The horizontal axis corresponds to luminosity: select the tonal range
that will get sharpened - the darkest tones are on the left, progressing
to white tones on the right. As mentioned in the tooltip, to move each
of the points in the threshold tool individually, hold the Shift key
before clicking on a point with your mouse. Holding the Ctrl key while
moving a point with the mouse allows for very fine movements.

When moving the right pair of sliders to the left side, sharpening is
reduced in the highlights. When moving the left pair of sliders to the
right side, sharpening is reduced in the shadows and minimizes
amplification of dark noise.

The default threshold values will protect from over-sharpening and
haloing in most cases and limit the sharpening effect to mid-tones. In
the example screenshot, the blackest tones have no USM applied, then USM
is applied to a broad range of tones from dark to light, and the
strength of USM gradually drops off from maximal at the mid-tones to
none at the whitest tones, so as to prevent noise amplification and
haloing.

#### Amount

The *Amount* parameter controls the strength of the sharpening.

#### Sharpen Only Edges

If you activate "*Sharpen only edges*" then uniform areas will not be
sharpened. This is useful when sharpening noisy photos.

Two new sliders appear as well:

- Radius, used for noise detection. If the noise is low, a lower radius
  can be used, and vice-versa. A higher radius slows down the image
  processing.
- Edge Tolerance, determines how much a pixel has to differ from its
  neighbor to be considered as an edge and not as noise. It is very
  similar to the USM *Threshold* parameter and has a high impact on the
  visual quality. For low ISO (low noise) images use 1000 or less, for
  high ISO images use 2500-3000 or even more.

#### Halo Control

"Halo Control" is used to avoid halo effects around light objects when
sharpening too aggressively. When activated, a new slider appears:

- Amount. At 100 it works at maximum, reducing the visual impact of the
  USM filter.

### RL Deconvolution

[RL
deconvolution](https://en.wikipedia.org/wiki/Richardson%E2%80%93Lucy_deconvolution)
is named after the makers of this algorithm, Richardson and Lucy. It
uses the [point spread function](https://en.wikipedia.org/wiki/Point_spread_function) (PSF) to
deconvolve (reverse) the effects of Gaussian-like blur. In reality, the
blur produced by the lens and by motion may differ from Gaussian blur
significantly, therefore some artifacts, such as halos, may appear when
the radius diverges too far from the type of blur in the actual image,
and when then effect is too strong.

#### Radius

The radius defines the [standard deviation](https://en.wikipedia.org/wiki/Standard_deviation) (sigma) of
the [Gaussian blur](https://en.wikipedia.org/wiki/Gaussian_blur) in the
image. Find the right value through trial and error.

#### Amount

Controls the blend factor between the unsharpened image and the
sharpened one.

#### Damping

Damping reduces the effect of the deconvolution at each iteration. It
has the effect of preventing sharpening of the finest details. Use it if
the sharpened image has too much "bite" at the finest level.

#### Iterations

RL Deconvolution is an iterative algorithm; it requires being repeated
in order to achieve the intended results. Each repetition of the process
is called an "iteration", and the result of one iteration is used as the
starting point of the next iteration. While each iteration removes blur,
it also increases processing time and the likelihood that halo artifacts
will appear, so you need to find the perfect balance through trial and
error - the default value should be fine for most cases.
