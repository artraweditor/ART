---
title: Retinex
contributors:
  - Lebarhon
  - DrSlony
  - XavAL
  - jdc
tags:
  - 'Tool Description'
  - 'Advanced Tab'
toc: true
---

__NOTOC__ Note: This translation of the "Retinex" French page is a
working document.

__TOC__

## Generalities

While the eyes are able to see correctly the colors through a poor
lighting, a colored surrounding or a veil of fog, cameras badly manage
in theses conditions. It is by copying the eyes biological mechanisms to
adapt itself to these conditions that the MSR algorithm (MultiScale
Retinex) has been created. In addition to the digital photography, the
Retinex algorithm (Retinex is the contraction for Retina + Cortex) is
used in astronomy to show up information laying in the astronomical
photographs, in medicine to detect not much visible structures in
radiography and tomodensitometry. Numerous theories and algorithms have
been working out for more than 20 years. The first experimentation has
been proposed by Rahman in 1996. The approach follows the human visual
perception and the Edwin Land's retinal function. In a way, this
approach is pretty similar to CIECAM. This function and more
particularly its general form is similar to a DOG (Difference Of
Gaussian). The idea consists of characterizing the luminous information
of a point from its intensity and the intensity of its neighbours. This
said, this approach has no scientific basis and is only lying on
experience and various empirical constants. Over the years many
improvements have been added, but from my own point of view, no one is
fully satisfactory. I relied on two documents :

- "Automatic Image Haze Removal Based on Luminance Component" (Fan Guo,
  Zixing Cai, Bin Xie, Jin
  Tang"[1](http://www.mcs.csueastbay.edu/~grewe/pubs/DistSensorNetworkBook2011/Atmosphere/HazeREmoval_SingleImage_Luminance.pdf)
- "Retinex Algorithm on Changing Scales for Haze Removal with Depth Map"
  (Weixing Wang, Lian
  Xu)"[2](https://scholar.google.com/scholar?cluster=13539545875418694074&hl=en&as_sdt=0,5)
- and from some programming tricks inspired from "2003 Fabien Pelisson"

The use of Retinex might be beneficial to images processing:

- that are hazy, misty or having a veil of fog
- with important luminance gaps
- where the user looks for special effects (tone mapping …)

### Retinex at the beginning of the processing

It is the algorithm described in this page, with its limitations,
advantages and drawbacks.


## Imposed limitations by RawTherapee

The basic algorithm impose a reference to the whole image, and not to a
crop or a reduction like in the RawTherapee processing. This limitation
imposed by the gaussian function causes several consequences:

- The processing has been shifted from its dedicated place, that should
  have been close to "Lab Adjustments" - near the beginning of the
  RawTherapee processing – that is necessarily (except if somebody has
  an idea to do differently) a raw process. As a consequence, non raw
  files (TIFF, JPG, …) can't be processed with this algorithm? This
  problem should be solved soon (November 2015 ??).
- The second consequence of this position is that the characteristics of
  the raw data situated just after demosaicing are very different that
  the ones situated downstream: no gamma, no gamut limitation, no white
  balance, no RGB conversion… so we must expect artefacts and poor
  luminance and color rendering.
- The third consequence is the system response time which imposes to
  re-actualize the whole process after each change in the settings.
- The fourth, sometimes, will need a white point modification, in order
  to avoid colors distortions (for example: a magenta sky).
- But advantage, this process being situated before "Denoise", the noise
  reduction will be here fully effective.

Nevertheless, despite these handicaps, as we will see it further,
results are more than satisfactory as well about processing time that in
contrast and colors rendering, particularly by joining Retinex and
Wavelets actions.

## Algorithm principle

For more information, see the "pdf" documents given in the
"Generalities" section.

### To elaborate a "Transmission Map" file obtained by

- making the difference, for luminance only, between each pixel
  logarithm of the input image and the neighbouring pixels logarithm of
  the matching gaussian image. The standard deviation (gaussian
  function) used here is very high - usually in RawTherapee sigma values
  from 0.5 to 5 are common – here the values range from 10 to 280
  (Single Scale Retinex), it is up to the user to choose.
- modifying the input image luminance distribution by

* applying a gamma before the "Transmission Map" file creation

* applying an inverse gamma to restore the input image characteristics

This modification of the distribution allows:

* to change the image tonality

* to modify the "Transmission Map" file to take into account for
example the under or over exposed areas.

- applying several time (Scale) – 3 times, not modifiable by the user –
  this algorithm with an addition using empirical coefficients (Multi
  Scale Retinex). (Low scale values increase the apparent contrast, but
  give a perspective look to the image, high scale values make the image
  more natural but they have a tendency to increase the noise).
- a variation of MSR by the user according to the desired effect
  (Uniform, Low, High, Highlight)
  - Uniform: Strive to process low and high intensities in an
    equilibrated way.
  - Low: Improve low intensities areas
  - High: Improve the rendering of the more exposed areas
  - Highlight: Improve the rendering of the highlight areas that may
    become magenta, but can bring strong artefacts.
- choosing the colorspace
  - logarithmic L\*a\*b\*
  - logarithmic HSL
  - linear HSL – this version doesn't comply with Retinex algorithm but
    in some cases it allows a more suitable image processing.

So, we get a logarithmic distribution "Transmission Map" – except in
linear mode! - that we will be able to "subtract" to the input image,
either to get an image theoretically free from mist and from veil fog or
to get special effects. This distribution owns approximatively a
gaussian distribution with a minima (minT) about, according to the
images – higher for images with veil – from -10 to -40, an average close
to from -1 to +2, a standard deviation often about 2 to 6 and a maxima
(maxT) about 10 to 40. The negative values mean low intensity and
positive values mean high intensity. Caution, these values are
logarithmic coefficients that stand for very high values (log 1000 =
6.9) (exp 10 \# 22000)...(exp 20) \# 500.000.000. In theory we should
use high "Scale" and gaussian values in the areas closest to the lens
(where the veil effect is low) and low values in the distance (where the
veil effect is important).

### To process the mask resulting from the gaussian process

The "Transmission Map" file correspond to a recursive process of:

- The source image – Input image which went through various
  modifications by the histogram equalizer and the gamma.
- The "mask" files directly resulting from the gaussian process (scale,
  radius, method low.. high…)

### "Mask" display type

"Mask" files processing (the idea came to me by studying the Rusell
Cottrell's plugin) will allow to decrease flares and artefacts. I added
a combo-box to choose the display type. This is both an educational
system and also an aid to find the "right" settings.

- Process:
  - Standard: it is the default setting
  - Mask: display the mask obtained by the Retinex algorithm resulted
    from the gaussian process. We will see here the impacts:
    - of the various settings upstream: method (low, uniform, high and
      highlight), radius, gamma, histogram equalizer. In the other hand,
      the other settings downstream have no effect on this display:
      contrast, gain, brightness, threshold and the curve "Transmission
      Map".
    - of the various settings of the "Mask equalizer".
    - you can export these settings (TIFF/JPG) to use this mask in
      external software (Gimp, Photoshop).
  - Unsharp mask: you can use this option to subtract the mask to the
    input image. In this case, an action on "strength" will allow to
    equilibrate between the input image and the mask. So we get the
    possibility to have images with very high radius values.
  - Transmission: display an "image" of the "Transmission Map" file.
    - this "image" doesn't match the reality that matches to a
      logarithmic scale which values are mostly situated between -30 and
      +30.
    - (fixed) in order to make this values "visible", I applied two
      arbitrary coefficients – I avoided the automatic computing in
      order to allow comparisons – which shift and increase the basic
      values.
    - (auto) I used here the values displayed in the Preview (TM Min Max
      Mean Sigma) to elaborate a file with values ranged from 0
      to 32700. Of course these data become visible with the maximum
      range but don't match the reality. This configuration can be,
      depending on the images, preferable to "fixed".
    - the displayed image takes into account each Retinex processing,
      except: a) "median filter"; b) "Gain"; c) "Brightness"
      (offset); d) and of course "Strength".
    - you can act on the different settings, particularly on the "Mask
      equalizer" ones, and also the "Transmission Map" curve, and then
      see directly the actions on the haze, the artefacts annd
      contrasts.

### Action methods on the masks

I set 3 methods:

- a diagonal curve which acts directly on the recursive contrast of the
  "mask" files – take care, this curve is very sensitive and can lead to
  important artefacts. In the other hand, it hasn't, or a few, impact on
  the processing time. You can use this curve alone or combined with the
  other methods.
- a gaussian process is applied to the masks. You can act on the
  shadows, the highlights and the radius. The increase of the processing
  time is moderate.
- a "wavelet" process (Sharp mask) is applied to the masks. You can act
  on the shadows and the highlights. The increase of the processing time
  is important or very important according to the selected method
  (partial or total).

### "Transmission Map" file processing

![](tmap.jpg "tmap.jpg").
 The main basis processing consist of to apply, using the "Transmission Map"
average and standard-deviation, a transformation of the type:

- newT = (oldT – mini) / maxi-mini.
- with mini = average – k \* standard-deviation
- with maxi = average + k \* standard-deviation

The choice of k - contrast (variance): modifiable by the user – is
decisive in the image rendering: low values will increase the seeming
contrast, hight values will make the image more natural with less
artefacts and hazes.

I added three components to this basis processing:

- a threshold - modifiable by the user – that will allow to reduce the
  maximum (maxT) and minimum (minT) values by integrating the clipped
  values to the new distribution.

Example: if the chosen threshold is 10, all the values lower than -10
will became -10, all the values over +10 will become +10. This action is
going to increase the contrasts, but can lead to artefacts.

- a curve (flatcurve) - modifiable by the user – to directly act on the
  distribution, by replacing theory by practical… It is by acting on the
  distribution that the user will be able for example to reduce the
  negative values and increase the positive ones (or the opposite) to
  improve the rendering in real time.
- a median 3x3 - modifiable by the user – that acts on the distribution
  irregularities, in order to reduce the artefacts.

### Elaborating the "Image Haze-free" file

![](hazefree.jpg "hazefree.jpg") This file is obtained by "difference" between the
input image and the "Transmission Map" file.

We are going to use here 2 extra parameters, Gain and Brightness
(Offset): These two parameters allow to include the "Transmission Map"
file to an exploitable image. Indeed, without precautions, the restored
image could be totally out of the luminance usual limits that are
between 0 and 32768.

- "Gain" will act on the total amplitude by increasing or decreasing it
  and for example reduce the numbe of totally black pixels.
- "Brightness" (Offset) will allow to reset the luminance after "Gain"
  acted.

In order to give some help to the user, I added a display for the
"haze-free" image minimum values to the GUI. We can act on "Gain" and
"Brightness" (Offset), but also on each other parameters, particularly
"contrast" (variance) and the "Transmission Map" curve. The ideal is to
set Min values the closest possible to 0 and the Max values the closest
possible to 32768. For example, an "original" image with min=-44230 and
max=76000 could become min=32, max=32500. Of course no other values are
forbidden, but in all cases these values will be clipped and brought
back to 0 – 32768, the "Transmission Map" file wealth is lost !!

Keys for acronyms and labels used in the GUI to ease the use:

- Restored haze-free Min=-5681 Max=34568 : display image maximum and
  minimum values "without veil" before clipping.
- TM Min=-3.8 Max=4.1 Mean=0.1 Sigma=3.3 : Tansmission Map -- Min and
  Max match to the following computing Min=Mean - Sigma \* Variance /
  100 ; Max=Mean + Sigma \* Variance / 100 \*TM Tm=-6.5 TM=11.8 :
  Transmission Map -- Tm minimum value of the distribution; TM maximum
  value of the distribution.

It is obvious that that a display in the GUI:

- of the "Transmission Map" distribution and also
- the "Haze-free" image data

should make easier both the system understanding and ergonomics… but
this is still to be fixed ???

### Retinex versus Tone-mapping

There is several ways (according to specialized sources in this field)
to adapt the Retinex algotithm to a "Tone-mapping" rendering. The study
of the existing code, my understanding of the problem… led me to the
following solution:

- Proceed to several "Retinex" specific code iterations. I name specific
  code the part excluding
  - the RGB==\>L\*a\*b\* or RGB==\>HSL conversions (and vice versa).
  - the histogram equalizer.
  - the differential gamma (low, middle, high, free).
  - in the same way, aren't concerned the choices done with the "Low",
    "Uniform", "High", "Highlight" method
- These iterations obviously increase the time processing, up to 2.5
  times for 5 iterations.

You can choose between several ways to carry on the iterations by acting
on 3 factors I called "Gradient":

- "Gaussian gradient" that act on: a) the gaussien blur sigma (Radius or
  Neighboring pixels); b) the "scale" factor (set by default to 3).
- "Transmission gradient" that act on: a) the contrast (variance); b)
  the threshold that limits the "Transmission Map" file.
- "Strength gradient" that act on "Strength", that means the combination
  of the "Haze-free" image with the original one.

If you set these 3 gradients to 0, all the iterations will be identical
to the basis processes.

Impact of each gradient:

- "Gaussian gradient" : according to the "it" iteration, the Radius
  value ("Neighboring pixels") is divided by "grad" and "scale" value
  become:
- Gg 1 :
  - it=1 grad=1; it=2 grad=1.25;it=3 grad=1.5;it=4 grad=1.75;it=5 grad=2
  - it=1 scal=4; it=2 scal=3; it=3 scal=3; it=4 scal=3; it=5 scal=2
- Gg 2 :
  - it=1 grad=1; it=2 grad=1.50;it=3 grad=2.0;it=4 grad=2.50;it=5 grad=3
  - it=1 scal=5; it=2 scal=4; it=3 scal=3; it=4 scal=2; it=5 sca=2
- Gg 3 :
  - it=1 grad=1; it=2 grad=1.66;it=3 grad=2.3;it=4 grad=3.00;it=5
    grad=3.6
  - it=1 scal=5; it=2 scal=4; it=3 scal=3; it=4 scal=2; it=5 scal=2
- Gg 4 :
  - it=1 grad=1; it=2 grad=1.80;it=3 grad=2.6;it=4 grad=3.40;it=5
    grad=4.2
  - it=1 scal=5; it=2 scal=4; it=3 scal=3; it=4 scal=2; it=5 scal=2
- Gg 5 :
  - it=1 grad=1; it=2 grad=3.50;it=3 grad=6.0;it=4 grad=8.5;it=5
    grad=11.0
  - it=1 scal=5; it=2 scal=4; it=3 scal=3; it=4 scal=2; it=5 scal=2
- Gg 6 :
  - it=1 grad=1; it=2 grad=6.00;it=3 grad=11.0;it=4 grad=16.0;it=5
    grad=21.0
  - it=1 scal=5; it=2 scal=4; it=3 scal=3; it=4 scal=2; it=5 scal=2
- Gg-1 :
  - it=1 grad=1; it=2 grad=0.875;it=3 grad=0.75;it=4 grad=0.50;it=5
    grad=0.37
  - it=1 scal=3; it=2 scal=3; it=3 scal=3; it=4 scal=3; it=5 scal=3

For Gg 5 and Gg 6, the selected method is taken into account, if it is
"highlight", 'grad' values are boosted according to "Highlight
Threshold"

- "Transmission gradient" : according to the "it" iteration, the
  contrast (variance) and threshold are multiplicated by "var":
  - Tg 1: it=1 var=1; it=2 var=0.875; it=3 var=0.75; it=4 var=0.625;
    it=5 var=0.5
  - Tg 2: it=1 var=1; it=2 var=0.800; it=3 var=0.60; it=4 var=0.400;
    it=5 var=0.2
  - Tg-1: it=1 var=1; it=2 var=1.125; it=3 var=1.25; it=4 var=1.375;
    it=5 var=1.5
  - Tg-2: it=1 var=1; it=2 var=1.400; it=3 var=1.80; it=4 var=2.200;
    it=5 var=2.6
- "Strength gradient" : according to the "it" iteration, "strength" is
  multiplicated by "s":
  - Sg 1 : it=1 s=1.30; it=2 s=1; it=3 s=0.70; it=4 s=0.5; it=5 s=0.5
  - Sg 2 : it=1 s=1.60; it=2 s=1; it=3 s=0.40; it=4 s=0.3; it=5 s=0.3
  - Sg-1 : it=1 s=0.80; it=2 s=1; it=3 s=1.20; it=4 s=1.2; it=5 s=1.2
  - Sg-2 : it=1 s=0.60; it=2 s=1; it=3 s=1.40; it=4 s=1.5; it=5 s=1.5

Note : "Hue equalizer" and "Transmission median filter" are applied only
once, whatever the number of iterations is.

## The various adjustments

### Adjustments always accessible

There are 5 adjustments at your disposal:

- Retinex method : with "Low" "Uniform" "High" "Highlight"
  - Low: improve low intensity areas
  - Uniform: Strive to process low and high intensities in an
    equilibrated way.
  - High: Improve the rendering of the more exposed areas
  - Highlight: Improve the rendering of the highlight areas that may
    become magenta, but can bring strong artefacts.

These 4 methods act directly on the "Transmission Map" file.

- Space
  - L\*a\*b\*
  - HSL Log
  - HSL Lin

The choice between these 3 spaces depends on the image… The 2 firsts
"meet" the Retinex algorithms.

- Strength: Combine "Haze-free" image with the original image.
  Strength=0 brings no Retinex processing to the image. Strength=100
  brings a total processing, "Haze-free" image only is displayed. Values
  under 50 are recommended.
- Chroma (patch): act on the color component, using the Retinex
  algorithm, in percentage of the "strength" value relative to the
  luminance. The slider runs a Retinex process dedicated to the color
  component by "simplifying" the process, luminance specific
  components - gamma, luminance, gaussian mask, ...- aren't executed.
  Caution, processing times are doubled and the extra memory calls
  important.
- Radius (Neigboring pixels): take into account the neighbouring pixels
  using the "Difference gaussian" algorithm, higher are the values, more
  affected is the foreground. Lower are the values, more affected are
  the distant areas, act directly on the "Transmission Map" file.
- Highlight threshold: only accessible with the " Highlight" method; set
  the "Highlight" method threshold, a value of 1 will give about the
  same effects that the "High" method, high values should allow to
  reduce the magenta colors in the overexposed areas. Be careful, it
  will probably be needed to:
  - change some adjustments, in particular "Radius" (Neigboring pixels)
  - increase the "Raw white point" value in case of magenta hues in the
    overexposed areas
  - use "Hue equalizer" in case of magenta hues in the overexposed areas
  - act on "Gamma Retinex"
- Contrast (Variance): This coefficient equilibrates "Transmission Map"
  average and standard-deviation. It is a key factor for the image
  rendering. It acts on the low and high contrast thresholds of the
  "Transmission Map" file, the lower are the values the more contrasted
  is the image, the higher are the values the more natural is the image
  and the artefacts reduced. This Contrast (Variance) doesn't act on the
  "Transmission Map" file but allow the Haze-free image computing.

### Adjustments accessible with "Setting"

- Gamma Retinex: with 4 choices "Low", "Middle", "High" and "Free", it
  allows to choose the gamma and the slope for a gamma applied before
  the Retinex algorithm and for an inverse gamma applied immediately
  after rebuilding the image. High gamma values associated with low
  slope values can lead to artefacts. The using depends on images,
  La\*b\* or HSL space, used settings… and the wished effects.
- Histogram Equalizer: This curve is a lesser evil… because it aims
  alone to restore gamma, RGB conversion, icc input profile, white
  balance … which gap is due to the Retinex position in the processing,
  that is placed in Raw in order to work on the whole image. Without
  this curve, "Transmission Map" data are generally bad and bring
  important artefacts. To be noticed that the interface has 2 different
  curves for L\*a\*b\* and HSL. This curve is without action by default,
  but in L\*a\*b\* mode it is quite mandatory to reduce the first
  quarter action (to be adjusted by the user) and to reduce the high
  values in order to reduce artefacts and haze. It is experience that
  will guide the user.
- Hue equalizer: This curve will allow:
  - to adjust Retinex action according to the hue, for example to weaken
    a sky, to boost leaves …
  - to reduce the action in the overexposed areas of magenta hue
  - it should be noted that when the "highlight" method is selected,
    this curve also acts on the final image chroma.

These 3 adjustments (Gamma Retinex, Histogram Equalizer,Hue equalizer),
don't act directly on the "Transmission Map" file, but modify its
original value.

- Gain and Brightness (Offset): used together, they allow to correctly
  locate the "haze-free" image. You can find help in the information
  given in the GUI, and place Min close to 0 and Max close to 32768.
  Gain and offset act on the "haze-free" image and have no action on the
  "Transmission Map" file.
- Threshold: this coefficient act on the "Transmission Map" maximum and
  minimum values.
- Transmission Map: this curve is interactive with the "Transmission
  Map" file, the X axis of the curve represents the "Transmission Map"
  distribution basis (MinT, mean, MaxT). To drag down the left part will
  reduce the minimum values (the ones relative to the foreground) and in
  the same way, to drag down the right part will reduce the maximum
  values (the ones relative to the background). This curve is a key
  point for controling the image seeming quality.
- Transmission median filter: reduce the artefacts due to the too strong
  local variations of "Transmission Map".

I added a "combobox" that allow to select the display type. It is both a
pedagogic system and also an help to find the "right" settings.

- Process
  - standard: the default setting
  - mask: display the mask obtained by the Retinex algorithm, resulting
    from the gaussian processing. We will se here the results of:
    - the different adjustments placed upstream: method (low, uniform,
      high and highlight), radius, gamma, histogram equalizer. In the
      other hand, the other adjustments placed downstream have no impact
      on the display: contrast, gain, brightness, threshold and the
      "Transmission map" curve.
    - the variouos "mask equalizer" adjustments.
    - you can export these adjustments (TIFF/JPG) to use this mask in
      external software (Gimp, Photoshop)
  - Unsharp mask: you can use this option in order to subtract the mask
    to the original image. In this case, the action on "Strength" will
    alloww to equilibrate the combination between the original image and
    the mask. Doing so give the possibility to have image with very high
    radus values.
  - Transmission: display an "image" of the "Transmission Map" file
    - this "image" doesn't match the reality, that one corresponding to
      a logarithmic scale which values are generally ranging between -30
      to +30.
    - (fixed): in order to make these values "visible", I applied 2
      arbitrary coefficients – I avoided automatic computing to allow
      comparisons – that shift and boost the basis values.
    - (auto): I use here the values displayed in the Preview (TM Min Max
      Mean Sigma) to elaborate a file which values are ranging between 0
      and 32700. Of course these data become visible with the maximum
      range but doesn't match the reality. This configuration may,
      according to the image, be preferable to "(fixed)".
    - the displayed image take into account the totality of the Ratinex
      processings, except a) the "median filter"; b) the "Gain"; c)
      "Brightness" (offset); d) and of course "Strength".
    - you can act on the various adjustments mostly the "Mask Equalizer"
      ones and also the "Transmission Map" curve and thus directly see
      actions on the haze, the artefacts and the contrast.

### Practical synthesis of the various adjustments

| Adjustments                            | Main feature                                                                                                                  | Complementary feature - comments                                                     |
|----------------------------------------|-------------------------------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------|
| Strength                               | Combine the action of the Retinex image with the original one.                                                                | No impact on the Retinex processing                                                  |
| Method : Low,Uniforme, High, highlight | Differentiate the action according to the luminance intensity (low = low intensity, highlight = may reduce colored artefacts) | Higher are the values, more impacted is the foreground (high favours the foreground) |
| Radius                                 | High values for the foreground, low values for the background                                                                 | High values may create artefacts                                                     |
| Contrast                               | Low values increase the global contrast, favour the foreground, create artefacts                                              | High values reduce artefacts                                                         |
| Threshold                              | Low values increase the global contrast, increase transition artefacts                                                        | Low values reduce background artefacts                                               |
| Transmission-map curve                 | Left part acts on foreground, right part acts on background                                                                   | Moving the median point acts on contrast and artefacts                               |
| Gain and Brightness                    | Locate Retinex according to the source image (see Settings - restored haze-free)                                              | May give illusion to act on contrast and lightness                                   |
|                                        |                                                                                                                               |                                                                                      |

Synthesis of the effects of the main adjustments on the contrast

| Adjustments                   | Main feature                                                        | Complementary feature - comments                                     |
|-------------------------------|---------------------------------------------------------------------|----------------------------------------------------------------------|
| Gaussian-mask and Sharp-mask  | Reduce the artefacts resulting from too high local contrasts        | High values reduce Retinex action                                    |
| Method L\*a\*b\* - HSL -...   | Change the luminance initial repartition, allow to reduce artefacts | L\*a\*b\* seems more adapted for images with high expositions        |
| Gamma and Histogram equalizer | Change the luminance initial repartition to reduce artefacts        | Gamma is compensated by its inverse at the end of Retinex processing |
|                               |                                                                     |                                                                      |

Other adjustments synthesis

## Use in combination with wavelets and Lab Adjustments

In case of atmospheric veil, the contrast restoration with Retinex
algorithm:

- is sometimes not good enough or leads to artefacts if "Strength" too
  high,
- leads to a chroma and sharpness insufficiency

Several solutions can be tried, particularly:

- to increase the chroma using L\*a\*b\* adjustments (Chroma slider or
  C/C curve or \* = f(a\*) and b\* = f(b\*) curves;
- to adjust the global contrast using La\*b\* adjustments (L curve)
- to increase the chroma using the residual image of wavelet,
- it is of course possible to use other RawTherapee controls,
  particularly in the "Exposure" tab.

But I think that to give priority to the wavelet tool is a good choice,
mostly for images with an atmospheric veil. We will be, in addition to
the adjustments given above, to:

- increase the residual image contrast,
- use the "contrast" compression method by sliding down "compression
  strength" and adjusting "gamma",
- lightly increase "Edge sharpness" with "edge detection"
- use "Final touchup", in particular: "Final local contrast", "Final
  contrast curve" and if necessary ("Tone-Mapping" effect) , "Contrast
  balance method".

## A possible example – Dehaze – some recommendations

In order to better explain the processing using Retinex, I will take the
example of a difficult image, very hazy and tainted with an atmospheric
veil [Madeira](http://rawtherapee.com/shared/test_images/madeira.dng)

Be careful, this processing is for pedagogic goal only and has no claim
to obtain a perfect image! (by the way, what does it mean, it is
subjective). The user has total freedom to proceed to changes,
modifications, to get the rendering he wishes.

### First Retinex adjustments

- At the start \[ following not understandable\]
- Enable "Retinex" and that's all

To get a synthesis of the different adjustments see \# Practical
synthesis of the various adjustments

With this kind of image, where the veil is present both in the
foreground and in the background and having an uniform histogram, I
choose the options:

- Method: Uniform
- L\*a\*b\*: because it is the most efficient of the 3 (Lab, HSL - Log
  and HSL Lin)

Then I choose, for "Radius", "Contrast" and "Strength", the adjustments
by default, that means respectively 80, 200 and 20.

#### Transmission map adjustment – Iterations with Radius, contrast, scale

![](tmap.jpg "tmap.jpg")
 The next step consist to study in the tool "settings" the indicator that
give partial information about Retinex optimization, the
"Transmission-map" file, I quoted "Restore haze-free". You can also, iif
you want, view the "Transmission-map" file, (set Process to the
"Transmission fixed" option).

We can see the values: Min=-5799 Max=14069., and also other values: TM
min =-9.8, Max=7.7 Mean=8.8 Sigma=4.4 Tm=-13.1 TM=20.7

Let's have a look to the both values Min=-5799 and Max=14069. That means
that the image corresponding to these values and which will be
subtracted to the original image, doesn't correspond at all to the usual
luminance values that, as a reminder, are between 0 and 32768 in
RawTherapee using L\*a\*b\*. The first thing to do is to use the
"Transmission-map" curve to get more relevant values. Be careful, the
goal isn't to get Min = 0 and Max = 32768, but more reasonable values.
For example, it is possible to lower a little bit the left part of the
curve and to lift a little bit the central part. We end up for example
to the following adjustments:
:[Media:madeira-first.pp3](/images/madeira-first.pp3)

##### The Haze-free image

![](hazefree.jpg "hazefree.jpg")
Just now I don't modify the values that could have
an influence on "Transmission-map" like "radius", "contrast".

Then, in an iterative way between "Gain transmission" + "offset" and
"radius", "contrast" and "scale", I will try:

- to act on the "Transmission-map" curve and "offset" in order to get
  Min and Max values close to 0 and 32768, but -400 and +39000 may be a
  good choice too.
- and then to act on "radius", "contrast", "scale" to obtain a looking
  fine image, contrasted enough, but without too much effect of depth…
  it is a matter of taste.
- work by iterations

I arbitrary took the following values given in the pp3
[Media:madeira-second.pp3](/images/madeira-second.pp3)

#### To apply a Gaussian mask and a gamma

These two tools allow to modify the distribution of the action between
high and low lights.

- choose "Mask Method" = Gaussian mask and increase hihlight = 23 to
  lighten the sky, you can use the "mask" option to view the changes
- put "Gamma" in "settings" on "Low"

Again, all is matter of taste.

We end up, for example to the following adjustments:
[Media:madeira-third.pp3](/images/madeira-third.pp3)

### Act on the chroma

Two cases:

- In the version "Retinex at the beginning of the processing" there is
  no Retinex action on the chroma. You can use the "Lab adjustments"
  tool and act either on the "Chromacity" slider or on the two curves
  a\*=f(a\*) and b\*=f(b\*), or on the curve C=f© more intuitive.
- In the version "Retinex in Wavelets", act on the Chroma slider in
  Wavelet.

We end up to the following adjustments:
[Media:madeira-four.pp3](/images/madeira-four.pp3)

### To boost the contrast and to reduce the atmospheric veil even more – Wavelet

- Slightly increase the contrast with "contrast levels"
- In Residual image" increase contrast and chroma (contrast = 25, chroma
  = 22), then choose "Compression method" = "Tone mapping" and reduce
  compression strength = -0.29
- In "Final Touchup", choose "Contrast balance" select "slider" and
  adjust "Contrast balance d/v-h" to 19
- Enable "chroma balance"

You end up to the following adjustments:
[Media:madeira-five.pp3](/images/madeira-five.pp3)

### Final adjustment

You can, if necessary :

- Adjust "Strength" in Retinex
- Act on "Final contrast" and/or "after contrast curve" in wavelet
- And also on all the other usual adjustments.

You end up to the following adjustments:
[Media:madeira-six.pp3](/images/madeira-six.pp3)
