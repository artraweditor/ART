---
title: Noise Reduction
contributors:
  - DrSlony
  - Gaaned92
  - XavAL
tags:
  - 'Tool Description'
  - 'Detail Tab'
toc: true
---

## Introduction


![](noise-cactus-0-ba.png "noise-cactus-0-ba.png")
<figcaption>Noise-cactus-0-ba.png</figcaption>

Photography is based on recording light which falls on a medium during
an exposure. The medium is typically film or a digital sensor. The light
signal *recorded* on the medium is not an ideal representation of the
signal which *fell upon* that medium - these differences constitute
noise. Film and digital photographs alike are susceptible to noise
(called "grain" in film), however there are various types of noise from
various sources specific to each medium.

In order to effectively deal with mitigating noise it is useful to
understand what types of noise exist and where they come from. The topic
of noise is well explained in this paper by one of RawTherapee's
developers and physics professor at the Enrico Fermi Institute, Emil J.
Martinec:

[Noise, Dynamic Range and Bit Depth in Digital SLRs](https://homes.psd.uchicago.edu/~ejmartin/pix/20d/tests/noise/index.html)

Light consists of packets of energy called photons. A
[digital sensor](https://en.wikipedia.org/wiki/Image_sensor) comprises of
millions of light-sensitive elements called *photosites* (also known as
*sensels* - sensor elements). Each photosite is capable of recording a
signal from a certain range of photons - too few and the photosite will
not register anything; too many, and the photosite will "clip" to pure
white (completely overexposed). Think of it as a bucket collecting
water - despite there being moisture in the air, if it doesn't rain then
the bucket is empty, but if it rains too much then it overflows.

Note that the idea of "pixel" does not exist at this point yet -
information from several photosites will later be combined into one
pixel during a process called [demosaicing](demosaicing).
Also note that some sources do write "pixels" when they mean
"photosites".

The physical sensitivity of the sensor is constant, however the
photographer can amplify the recorded signal by modifying a setting you
know as ISO (see the [Film Speed](https://en.wikipedia.org/wiki/Film_speed) article on Wikipedia).
Since the signals recorded by the sensor are not perfect, using a higher
ISO amplifies not only the desired signal, but also the noise. Sensors
are susceptible to noise at every ISO level, however the higher the ISO
the more apparent the noise.

There are different tools for dealing with different types of noise:

- The Noise Reduction tool is best at dealing with photon shot noise
  ([Gaussian](https://en.wikipedia.org/wiki/Gaussian_noise) and
  [Poisson](https://en.wikipedia.org/wiki/Shot_noise) noise) and film
  grain, and some sensor read noise.
- Sensor read noise and thermal noise are best handled by the
  [Dark-Frame](dark-frame) tool.
- Salt and pepper noise (sudden white or black pixels) is best handled
  by the [Impulse Noise Reduction](impulse_noise_reduction)
  tool.
- Hot and dead pixels are best dealt with using the [Hot/Dead Pixel
  Filter](preprocessing#hot.2fdead_pixel_filter.md).
- Pattern noise (periodic, anisotropic) is best handled by the [Line
  Noise Filter](preprocessing#Line_Noise_Filter.md). You can
  also fix pattern noise (de-screen) after RawTherapee in GIMP, using
  the Fourier transform in G'MIC.

Regardless the source, noise will manifest itself as blotches of
deviating color - "chrominance noise", and deviating brightness -
"luminance noise".

1.  Chrominance noise is endemic to digital images, it is generally
    unattractive and something you will always want to remove.
2.  Luminance noise, on the other hand, looks like film grain and can be
    attractive, so it's not uncommon to want to remove chrominance noise
    but keep luminance noise.

<div align="center">

![](noise-wall.png) noisy test photo taken at ISO 6400.

![](noise-wall-demosaic-amaze.png)
AMaZE demosaicing leads to small maze-like patterns. 

![](noise-wall-demosaic-lmmse.png
LMMSE demosaicing avoids maze-like patterns while preserving detail.

![](noise-wall-luminance100-chrominance-off.png)
This is what chrominance noise looks like. Luminance detail was obliterated to make
the chrominance noise more clear. Notice the color blotches in what
should be a smooth wall.

![](noise-wall-luminance100-chrominance-on.png)
Enabling chrominance noise reduction eliminates the colored blotches.

![](noise-wall-chrominance.png)
This is what luminance noise looks like. Chrominance noise was removed to make the luminance noise more clear.

![](noise-wall-luminance-tweaked-chrominance-on-median-off.png)
Both chrominance and luminance noise were removed.

![](noise-wall-luminance-tweaked-chrominance-on-zoom-median-off.png)
Tiny pixel-sized artifacts are left-over from noise reduction.

![](noise-wall-luminance-tweaked-chrominance-on-zoom-median-on.png
These artifacts can be removed using the median filter.

</div>

Not everyone's requirement for good noise reduction is the same. Some
like a completely clean, smooth result, while others prefer to have some
grain left over to give the photo a more film-like quality.
RawTherapee's powerful *Noise Reduction* tool caters to all your needs -
it lets you eliminate noise while retaining detail. It uses
[wavelets](http://en.wikipedia.org/wiki/Wavelet), a
[Fourier transform](https://en.wikipedia.org/wiki/Fourier_transform) and a
[median filter](https://en.wikipedia.org/wiki/Median_filter) to work its
magic. Read on to learn how to use it efficiently.

## Usage

This section details the order of operations for removing noise.

1.  Start by ensuring you're using the optimal demosaicing algorithm.
    AMaZE is recommended for general RawTherapee use, however, when
    working with very noisy, high-ISO images, it is recommended to use
    the LMMSE or IGV demosaicing methods instead. AMaZE can lead to tiny
    maze-like artifacts appearing in very noisy images, whereas LMMSE
    and IGV are designed to prevent that from happening.
2.  Check the sharpening tools to make sure that you're not sharpening
    any fine detail, because your noisy photo has no fine detail! If
    you're using [Contrast by Detail Levels](contrast_by_detail_levels) or
    [Wavelets](wavelets), make sure that the first one or two
    fine-detail contrast sliders are at 0 to prevent these tools from
    amplifying noise.
3.  Zoom into the photo to 100% or more and find an area that has both
    sharp, in-focus parts as well as large, plain or out-of-focus ones,
    so that you have a good overview of the effects of the tool.
4.  Enable the [Hot/Dead Pixel Filter](preprocessing#hot.2fdead_pixel_filter) if you
    notice salt-and-pepper noise (black and/or white pixels).
5.  Enable the Noise Reduction tool. Chrominance noise is automatically
    removed and usually does not require any tweaking. At this point the
    remaining noise looks more like film grain. If you are happy with
    keeping it then you are done, else keep reading.
6.  To remove luminance noise, set the *Detail recovery* slider to 0,
    and increase the *Luminance* slider until the noise has been
    smoothed-away.
7.  Increase the *Detail recovery* slider until you regain a
    satisfactory level of detail.
8.  You may notice some small artifacts remain from the noise reduction
    process. Use the Median filter to remove them.
9.  While it is generally not recommended to combine sharpening with
    noise reduction, RawTherapee-5.5 has a "contrast threshold" adjuster
    in the [Sharpening](sharpening) tool, thanks to which you
    can sharpen details while preserving the smoothness of uniform, flat
    areas.

<div align="center">

![](noise-cactus-1-amaze.png)
The noisy image.

![](noise-cactus-2-lmmse.png)
Changing the demosaicing method to LMMSE eliminates the small maze-like patterns and makes the salt-and-pepper noise more clear.

![](noise-cactus-3-pixelfilter.png)
Enabling> the [Hot/Dead Pixel Filter](preprocessing#hot.2fdead_pixel_filter) eliminates the
salt-and-pepper noise. 

![](noise-cactus-4-nr-chroma.png)
Enabling automatic chromaticity noise reduction renders a pleasantly-grainy image. 

![](noise-cactus-5-nr-luminance.png)
Luminance noise was smoothed-away using the *Luminance* slider.

![](noise-cactus-6-nr-detailrecovery.png)
Detail> was restored using the *Detail recovery* slider. 

![](noise-cactus-7-nr-median.png)
The median filter was used to eliminate left-over artifacts.

![](noise-cactus-8-sharpen.png)
Sharpness was restored using an unsharp-mask with a contrast threshold to prevent sharpening areas which should be smooth.

</div>

## Interface

### General

Closely examine large areas of strong saturation with fine detail - such
as the pattern on a colored shirt or the petal of a flower - as you
switch between the RGB and L\*a\*b\* spaces.

The following images demonstrate the effects of various types of noise
reduction, exaggerated for clarity. While the source image does not
contain any very-low-frequency *noise*, it was chosen because it does
display the effects (and side-effects) very well.

<div align="center">

![](noise-handkerchief-1-off.png)
The noisy image. Detail recovery will be intentionally left at 0, and chromaticity strength will be
intentionally set very high, to emphasize effect.

![](noise-handkerchief-2-luminance-lab-conservative.png)
Luminance noise reduction in L\*a\*b\* space, conservative.

![](noise-handkerchief-3-luminance-lab-aggressive.png)
Luminance noise reduction in L\*a\*b\* space, aggressive.

![](noise-handkerchief-4-luminance-rgb-conservative.png)
Luminance noise reduction in RGB space, conservative.

![](noise-handkerchief-5-luminance-rgb-aggressive.png)
Luminance noise reduction in RGB space, aggressive.

![](noise-handkerchief-6-chrominance-lab-conservative.png)
Chrominance noise reduction in L\*a\*b\* space, conservative.

![](noise-handkerchief-7-chrominance-lab-aggressive.png)
Chrominance noise reduction in L\*a\*b\* space, aggressive. Notice how colors bleed
one into another in areas where one hue meets the other - navy blue
bleeds into cyan, green bleeds into dull-red, dull-red bleeds into
crimson-red, etc.

![](noise-handkerchief-8-chrominance-rgb-conservative.png)
Chrominance noise reduction in RGB space, conservative.

![](noise-handkerchief-9-chrominance-rgb-aggressive.png)
Chrominance noise reduction in RGB space, aggressive. Low-frequency detail is lost.

</div>

#### Color Space

Noise reduction can be performed in the L\*a\*b\* and RGB color spaces.

When working in the L\*a\*b\* space, the L\* channel is used for
luminance and the a\* and b\* channels are used for chromaticity.

When working in the RGB space, the Y from the CIE XYZ color space is
used for luminance and (X-Y) and (Y-Z) are used for chromaticity.

#### Mode

There are two general noise reduction modes which control whether only
high frequency or also low frequency noise is removed. *Low frequency*
noise is noise whose blotches cover a large area; conversely, *high
frequency* noise has smaller blotches which cover fewer pixels.

1.  Conservative - removes all except very low-frequency noise, so color
    detail is better preserved at the expense of not removing very large
    blotches. Use in most cases.
2.  Aggressive - removes also very low-frequency noise at the expense of
    being more aggressive with higher frequency noise. Use only on
    extremely noisy photos.

#### Gamma

Gamma varies noise reduction strength across the range of tones. Smaller
gamma values let noise reduction affect all tones emphasizing the action
on shadows, while higher gamma values limit the effect to brighter tones
only.

### Luminance


![](rt_nr_luminancecurve_books.jpg "rt_nr_luminancecurve_books.jpg")
<figcaption>Rt_nr_luminancecurve_books.jpg</figcaption>


"Luminance control" lets you choose whether you want to manipulate the
luminance noise reduction via sliders or a curve.

Adjusting the "Luminance" slider is equivalent to manipulating the
amplitude of the luminance curve - both affect how strong the noise
reduction effect is. The curve has the additional advantage of letting
you control noise reduction strength as a function of the pixels'
luminance - e.g. it allows you to have strong luminance noise reduction
in the shadows and none in the highlights.

The "Detail recovery" slider allows you to recover structure while not
reintroducing noise, unless you set this value too high.

### Chrominance

Method

Chrominance noise reduction can be performed using one of three methods:

- Manual
- Automatic global
- Preview

Preview noise

The "Preview noise" indicator gives the estimated chromatic noise values
after "Chrominance" processing:

- Mean: estimates the average noise value across all channels.
- High: estimate the highest noise value across all channels.

#### Manual

The three sliders and the curve act on the full image. You control the
settings manually.

Master

Controls the strength of chrominance noise reduction. Functions as an
offset independently to the red-green and blue-yellow values. For
example if master=50, red-green=-50 and blue-yellow=-50, the end result
is 0; no effect.

Red-Green

Reduce/boost noise reduction in the red-green channel (a\* in
L\*a\*b\*).

Blue-Yellow

Reduce/boost noise reduction in the blue-yellow channel (b\* in
L\*a\*b\*).

Chrominance curve

The chrominance curve lets you control chrominance noise as a function
of the pixels' chrominance - e.g. it allows you to have strong
chrominance noise reduction in areas of low saturation and weak noise
reduction in areas of high saturation. This curve modulates the
"Master", "Red-Green" and "Blue-Yellow" sliders' action by multiplying
their values by the curve ordinate. For example, if the master slider is
set to 30 and the curve is at mid-height, the equivalent result will be
about 45. It can be useful to boost noise reduction in grey or drab
areas, as we distinguish noise more easily in areas of low saturation
than we do in areas of high saturation. When using the "automatic
global" noise reduction method, the automatically-calculated parameters
are an average for the whole image, and they might be insufficient to
remove noise in these low-saturation areas - the chrominance curve can
help.

#### Automatic Global

The algorithm splits the image into multiple cells. For each cell the
following are calculated:

- An average noise level for the red-green channel and the blue-yellow
  channel.
- A maximum noise level for the same channels.

#### Preview

This method is only operational when zoomed to 100% or more. It analyzes
the areas currently visible in the preview (if you are zoomed to 100% or
more) and calculates:

- An average noise level for the red-green channel and the blue-yellow
  channel.
- A maximum noise level for the same channels.

The three sliders - Master, Red-Green and Blue-Yellow - as well as the
"Preview noise" values, are updated accordingly.

If you would like to keep the currently calculated values then you
should switch back to "manual" method, else the values will be
re-calculated when you pan or when you copy the profile to other images.

### Median

![](rt_nr_median_books.jpg "rt_nr_median_books.jpg")


![](rt_nr_median_zoom_books.jpg "rt_nr_median_zoom_books.jpg")

Use this filter to remove tiny, sharp-looking artifacts left-over from
noise reduction.

Median Type

The [median filter](https://en.wikipedia.org/wiki/Median_filter)
replaces each pixel with the median value of its neighboring pixels. The
contiguous group of pixels being sampled is called the "windows". This
window slides pixel by pixel over the entire image. You can choose the
size of this window using the "Median type" drop-down. The larger the
size, the longer it takes.

Available window sizes:

- 3x3 soft: treats 5 pixels in a 3x3 pixel window.


○●○

●●●

○●○

- 3x3: treats 9 pixels in a 3x3 pixel window.


●●●

●●●

●●●

- 5x5 soft: treats 13 pixels in a 5x5 pixel window.


○○●○○

○●●●○

●●●●●

○●●●○

○○●○○

- 5x5: treats 25 pixels in a 5x5 pixel window.


●●●●●

●●●●●

●●●●●

●●●●●

●●●●●

- 7x7: treats 49 pixels in a 7x7 pixel window.


●●●●●●●

●●●●●●●

●●●●●●●

●●●●●●●

●●●●●●●

●●●●●●●

●●●●●●●

- 9x9: treats 81 pixels in a 9x9 pixel window.


●●●●●●●●●

●●●●●●●●●

●●●●●●●●●

●●●●●●●●●

●●●●●●●●●

●●●●●●●●●

●●●●●●●●●

●●●●●●●●●

●●●●●●●●●

Sometimes it is possible to achieve higher quality running several
iterations with a small window size than one iteration with a large
window size.

Median Method

You have five methods at your disposal:

- Luminance only: works in the L\*a\*b\* space, but only affects the L\*
  channel.
- Chroma only: works in the L\*a\*b\* space, but only affects the a\*
  and b\* channels.
- Weighted L\* (little) + a\*b\* (normal): affects all channels in the
  L\*a\*b\* space, but acts more weakly on the L\* channel.
- L\*a\*b\*: affects all channels equally.
- RGB: works in the RGB space, and the window size choice is limited to
  3x3 soft, 3x3 and 5x5.

When using the "Luminance only" and "L\*a\*b\*" methods, median
filtering will be performed just after the wavelet step in the noise
reduction pipeline. When using the RGB color space, it will be performed
at the very end of the noise reduction pipeline.

You may wonder what other uses median filtering has apart from the
elimination of pixels which strongly differ from their surrounding
neighbors for aesthetic reasons. One of these benefits is a reduction in
file size when saving to compressed formats such as JPEG and PNG. Median
filtering removes variations which you will lose anyway if you downscale
the image. You are also likely not to see these variations if you print
the image. Removing them using median filtering can reduce the file size
by even 40% (tested using JPEG compression strength 92 with "balanced
quality" [chroma subsampling](https://en.wikipedia.org/wiki/Chroma_subsampling)), so give
it a try if output file size is a factor.

Finally, the "chroma only" median filter method can be used as a
complement to automatic chrominance noise reduction calculation - by
reducing sharp outliers it can soften the calculated values, thereby
avoiding fading out color detail too much.
