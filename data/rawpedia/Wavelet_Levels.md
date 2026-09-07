---
title: Wavelet Levels
contributors:
  - DrSlony
  - Thanatomanic
  - XavAL
  - Jdc
tags:
  - 'Tool Description'
  - 'Advanced Tab'
toc: true
---


![](daffodil_split.jpg "daffodil_split.jpg")


__TOC__

## How is this tool organized?

The Wavelet Levels tool is extensive and its underlying algorithms are
complex. It has most of the functions necessary for processing
photographs from start to finish with the exception of certain tasks
such as interpolation or color management. However, it is most useful
when it is used to complete or refine processing operations carried out
in other parts of RawTherapee. It allows you to work on different levels
of detail to produce subtle contrast and color effects, remove noise or
defects in the image without sacrificing overall detail, or work on the
color and luminance of the image without introducing artifacts.

It can be used for any sort of image but its unique capabilities make it
particularly suitable for portraits, macro photography,
astro-photography etc., where selective control over fine detail is
important. It can also be used to great effect in landscape photography
to remove noise in skies, compress the dynamic range while at the same
time preserving the details, reduce the noise, remove color casts in
shadows, and create interesting luminosity effects. The capabilities are
almost limitless, but you will only be able to use them properly if you
have a good understanding of the underlying principles and operation of
the various tools, **so please read on!**

The tool is organized around a general **Wavelet Settings** module
followed by a series of modules which can activated or deactivated to
perform specific tasks.

## What are Wavelets?

A [Wavelet](https://en.wikipedia.org/wiki/Wavelet), or more precisely a
[Wavelet Transform](https://en.wikipedia.org/wiki/Wavelet_transform), is
a complex mathematical function which is very useful in image
processing. It allows you to split images into different levels of
detail so that you can work on the level that interests you.

The wavelets term was introduced in the early 1980s by French physicists
Jean Morlet and Alex Grossman: they used the French word *ondelette*,
which means *small wave*. Later, this word was adapted to English
changing *onde* by *wave*, leading to *wavelet*.

The *Wavelet Transform*, which is similar to a *Fourier Transform*,
represents data as combinations of known and predefined waves (the
frequencies), so that the result is as close as possible to the original
data. Broadly speaking, the main difference for two-dimensional images
is that in the Wavelet Transform the data being analyzed is represented
as the frequencies present at the pixel level of the image, whereas in
the standard Fourier Transform the data represents the frequencies
present in the full image. Therefore, using wavelets offers more
precision when analyzing the data.
<span style="font-size: 0.7em; font-style: italic;">\[Obviously this is
a very simplistic explanation: mathematicians would surely have a lot to
say here...\]</span>

![](wavelet_daubechies20.jpg "Wavelet_daubechies20.jpg")RawTherapee uses
wavelets in various tools, and in this one in particular it uses the
[Daubechies](https://en.wikipedia.org/wiki/Daubechies_wavelet) wavelet,
to decompose the elements of the image into the components of the
[L\*a\*b\* color space](https://en.wikipedia.org/wiki/CIELAB_color_space) (*L\**, *a\** and *b\**).

Image decomposition is carried out using an
[algorithm](https://en.wikipedia.org/wiki/Algorithm) to analyze the
«internal» contrast of groups of pixels (2x2=4 pixels on the first
level, 4x4=16 pixels on the second level, ...) in three directions:
vertical, horizontal and diagonal. This analysis converts these contrast
values into sets of wavelets with different amplitudes and intensities
and stores their characteristics in coefficient matrices, which indicate
how the wavelets should be combined to regenerate an image as close as
possible to the original.

Each time you make modifications (contrast, tone, noise, ...) this
regeneration is done automatically, so that you can immediately see the
result of your adjustments.

In fact the moment the image is decomposed, it ceases to exist, leaving
only sets of coefficients (one set for each level) which will be used
subsequently by the tool. These coefficient sets can be used to
characterize the image in the following two ways:

- Several levels of detail: the first level corresponds to details with
  an area of 2x2 pixels; the tenth level corresponds to «details» with
  an area of 1024x1024 pixels. The choice of how many you use depends on
  your needs, however keep in mind that processing time and memory
  requirements will increase with the number of levels.


Because only *variations* (gradients, or differences) in
[hue](https://en.wikipedia.org/wiki/Hue) or
[luminance](https://en.wikipedia.org/wiki/Luminance) are analyzed at
each level, the levels will not contain any information if an image is
absolutely uniform in luminance and color. In this case any differences
extracted from individual levels will come from digital noise and
changes in contrast (or chromaticity) due to edge effects, fog or other
scene-related optical phenomena.

- A residual image: the result of removing the details from all of the
  decomposed levels of the original image. Consequently, any
  modifications (contrast, chromaticity etc.) that are carried out
  within a particular level will have no effect on the residual image
  and vice versa.

Moreover, each level the tool takes into account the set of coefficient
values and calculates their arithmetic mean (for each level the mean
will be different) and the [standard deviation](https://en.wikipedia.org/wiki/Standard_deviation). By adding
the maximum and minimum coefficients to this data, a characteristic
distribution curve is generated for each level (it should be noted that
this curve is not
[Gaussian](https://en.wikipedia.org/wiki/Normal_distribution)). This
information is used in different ways in the various algorithms used by
the wavelet levels tool.

## In practice

After decomposition, the resulting levels can be used for different
purposes: image compression, noise reduction,
[secret watermarking](http://www.intechopen.com/books/discrete-wavelet-transforms-algorithms-and-applications/application-of-discrete-wavelet-transform-in-watermarking),
specific residual image treatment for astronomy, etc.

Depending on your needs, you can work either with an individual level of
detail, with several levels of detail (one after another), with the
residual image, or with all of them combined.

The size of the details included in each level is:


![]wavelet_detail_size.jpg "wavelet_detail_size.jpg")
<figcaption>wavelet_detail_size.jpg</figcaption>




**1 (Finest)** : 2x2 pixels

**2** : 4x4 pixels

**3** : 8x8 pixels

**4** : 16x16 pixels

**5** : 32x32 pixels

**6** : 64x64 pixels

**7** : 128x128 pixels

**8** : 256x256 pixels

**9 (Coarsest)** : 512x512 pixels

**Extra** : 1024x1024 pixels

If you were to select 5 detail levels, the changes in the various levels
would be limited to details with 32 pixels size or smaller. In this case
the residual image would have all the details of the image, except those
included in levels *1* to *5*. And since the details that have been
removed are relatively small, the residual image would be similar to the
original image.

On the other hand, if you chose *level 9* you could change the details
with a size of 512 pixels and 1024 pixels (*level Extra*). In this case
the residual image would be quite different from the original image, as
the levels from *1* to *Extra* would contain all the details, leaving
little more than a blurred background.

Wavelet decomposition separates the
*[lightness](https://en.wikipedia.org/wiki/Lightness)* and the
*[chroma](http://www.huevaluechroma.com/015.php)* channels ([*a\** and *b\**](https://en.wikipedia.org/wiki/CIELAB_color_space#CIELAB)) in the
residual image and in each of the levels. This allows you to apply
different adjustments to the brightness and tones of each level and
carry out completely different processing operations on the residual
image. This means that the levels and the residual image are independent
and the tool will only modify those levels where changes have been made.
The rest will remain untouched and the residual image will continue to
be what is left after the details in each of the levels have been
removed (regardless of whether they have been modified).

Note that if you want to use the Wavelet Levels tool at the same time as
the CIECAM tool, you may get artifacts due to the
fact that the CIECAM color model uses specific values that are close to,
but different from the values of the Lab color space. Because of the way
the tool is coded these artifacts are unavoidable, but their appearance
will depend on which processing operations have been carried out.

#### The preview

The size of the image on the screen has a direct impact on the perceived
sharpness and on one’s ability to see any small changes introduced by
the various modules: **the effects of this tool are only visible at full
size** (or larger).

In practice, this means that, [for processing speed reasons](general_comments_about_some_toolbox_widgets#the_preview_area),
you must have the final size of the image in mind. If it is planned to
reduce the image size (scale it down, not crop it), then it is advised
to first export it with its final size and process it afterwards with
wavelets. Keep this in mind because otherwise what you see in the
preview will not be the same as the final exported result.

There is also another limitation: RawTherapee uses all the levels it can
in the preview and ignores levels that have details larger than the
portion of the image you see on the screen. However, if the changes in
the ignored levels are not shown on screen, they will be applied when
the image is saved to disk.

**Examples**

- ***Example 1***: the image is **4096x2160** pixels, you have enlarged
  it (to 100% or more) and in the preview you see a **1500x1200**
  pixel-area similar in size to the final image. This is the ideal case
  because on the screen you can see all the modifications in all of the
  levels (up to the *level Extra*). In addition, any changes made in any
  of the levels will be included in the final image.
- ***Example 2***: the image is **4096x2160** pixels, but you have
  enlarged it and can only see **300x200** pixels in the preview. On the
  screen you won't be able to see any change in details bigger than
  *level 7* (details of 128 pixels), but when you save it the changes
  you made in levels *8*, *9* and *Extra* will be included (because the
  image is bigger than 1024x1024 pixels).
- ***Example 3***: the image is **720x480** pixels and you have enlarged
  it until you can only see **300x200** pixels in the preview. On the
  screen you will not be able to see any modification in details bigger
  than those of the *level 7* (details of 128 pixels). When saving, the
  changes you made in *level 8* will be included (details of 256
  pixels), but levels *9* and *Extra* **will NOT** be included.

To help keep this important information in mind, the tool indicates how
many levels are being used for the preview (under the last slider of the
*Contrast* module). In the examples 2 and 3 it would indicate:
«**Preview maximum possible levels = 7**».

#### Contrast by Detail Levels vs Wavelet Levels

It's worth mentioning that RawTherapee has a tool called *[Contrast by Detail Levels](contrast_by_detail_levels)* and although it
looks like the Wavelet Levels tool, there are several important
differences between them:

- *Contrast by Detail Levels* has fewer levels (6, instead of up to 10),
- *Contrast by Detail Levels* only allows you to adjust the luminance of
  each level, while Wavelet Levels also allows you to adjust the chroma
  of each level,
- *Contrast by Detail Levels* adjusts equally all luminance (or chroma)
  values present in the level, while Wavelet Levels performs a
  progressive adjustment ([this is explained further in the contrast attenuation section](#the_attenuation_curve)),
- *Contrast by Detail Levels* doesn't have a residual image.

That said, it is possible to use both tools at the same time. It should
be noted however that *Contrast by Detail Levels* is applied earlier in
the [Processing Pipeline](toolchain_pipeline), so depending
on the intensity of the adjustments made there, the details presented in
levels from *1* to *6* may be affected. In other words, since the
contrast will have changed with the *Contrast by Levels of Detail*
settings, the analysis by Wavelet Levels could decompose the image in a
different way, so the results would be different. In any case, if you
have to use both tools, it is recommended that you adjust first the
Contrast by Detail Levels and then adjust the Wavelet Levels.

## General tool configuration

When this tool is turned on, any adjustments will affect all the
subsequent modules.

### Strength

With this slider you can adjust the overall intensity of the tool. It
works on a similar principle to the opacity slider used for blending
layers in the GIMP: any adjustments made in the Wavelet Levels tool can
be blended back into the original image using the Strength slider. This
allows you to make fairly aggressive adjustments and then adjust the
overall intensity to achieve the desired result.

### Wavelet levels

This slider lets you decide how many detail levels the image will be
decomposed into. You can choose any level between *4* and *9* (the 10th
level, called *Extra*, appears automatically when you select *level 9*).
The higher the number, the more processing time and memory will be
required.

### Tiling method

A drop-down list allows you to choose from:

- Full image,
- [Tiles](https://en.wikipedia.org/wiki/Euclidean_tilings_by_convex_regular_polygons).

It is always preferable to use *Full image*, because it avoids problems
in the transition area between tiles.

However, if you do not have enough RAM, or if you are processing very
large images (e.g., 50 Megapixels or more), you may have to use the
tiles:

| Required memory, in bytes, with 9 detail levels    |
|----------------------------------------------------|
|                                                    |
| Megapixels (Mpx)                                   |
| To open the image (all tools turned off)           |
| Contrast, Chromaticity or Hue Protection turned on |
| \+ Avoid color shift                               |
| Total                                              |

### Edge performance

An image that has been decomposed into its component parts using the
Daubechies method may have up to 10 coefficient scales ranging from D2
(which corresponds to the Haar decomposition) to D20. In RawTherapee the
coefficients *D2 (low), D4 (standard), D6 (standard plus), D10 (medium)*
and *D14 (high)* are used. The more coefficients there are, the more
detail the wavelet will distinguish albeit with a slight increase in
processing time (often negligible).

Although there is no direct relationship between the resulting quality
and the number of coefficients (depending on the original image),
choosing the right number of coefficients will allow you to refine the
quality of the lower levels, or that of the residual image:

- in some cases the best results for edge detection are obtained with D2
- in other cases with D6 or D14

This parameter has a fairly strong impact on *[Edge detection](#edge_detection)* and also on global decomposition
(the relationship between the residual image and each level).

### Preview

This group of controls will help you understand how to work with the
wavelets tool and assist when fine-tuning the parameters of the various
modules (e.g. noise reduction).

You have a total of four drop-down lists, allowing you to tailor what
you see in the preview.

The group is divided into two main drop-down lists (and several others
that will be activated when you make certain selections in the main
lists):

- the first lets you choose the preview background
- the second lets you choose which levels will be displayed in the
  preview

#### Background

In the ***Background*** list you can choose between 3 possible
backgrounds: *Black*, *Gray* or *Residual Image*, which will be used
when viewing any of the levels.

The histogram will take into account these options and will allow you,
for example, to see the effects of the settings on the residual image.
Note however, that if you choose the black or gray background, you will
not see the residual image (the real background) and you may find that
the image has a strange look. You should be especially aware of this if
you make changes to the detail levels, as the actual effect will not be
seen until you put the residual image back into the background. In spite
of this, it is sometimes interesting to see the changes against a
neutral background to better judge what is happening (for example in
noise reduction).

#### The process levels

In the ***Process*** list you can select:

1.  *One level*
2.  *Finer details levels, with selected level*: all levels **from** the
    selected level down to *level 1*,
3.  *Coarser details levels, without selected level*: all levels up to
    the *level Extra* (plus the residual image), **with the exception**
    of the selected level
4.  *All levels, in all directions*

In previous versions of the program, the list was shown with different
labels, but the behaviour of the sliders remains unchanged:

- One level
- Below or equal the level: now *Finer details levels, with selected
  level*
- Above the level: now **Coarser details levels, without selected
  level**
- All levels, in all directions

If you select any of the first three options, two drop-down lists will
be activated just below *Process:*.

- in the list on the left you can decide which level the previous
  options refer to (from *level 1* to *9*, the *level Extra*, or the
  *Residual Image*).
- in the list on the right you can choose the wavelet decomposition
  direction (*Vertical, Horizontal, Diagonal, All directions*).

If you select the option *All levels, in all directions*, you can edit
the levels directly on the residual image (the two lower lists would
remain disabled). This option is useful if you already have experience
with the tool and you prefer to view the entire image while editing it.
It is also the option you should select before exporting. Keep in mind
that what you see in the preview will be what is exported in the final
image and is shown in the histogram: if you have selected *One level*,
you will see only one level on the screen and the histogram will reflect
the RGB values of that particular level. When you export the image only
the chosen level will be included in the final image so *'before
exporting, make sure you select*All levels, in all directions'''.

#### Suggestions for use

- you can select *One level* with a gray background to see how the
  selected Daubechies' coefficient (from D2 to D14) has decomposed the
  details, and then try out different coefficients to see which one
  offers the most accurate detail separation
- you can select *One level* to find the level that has the details you
  want to work on (such as the level that has extracted the blemishes
  from the skin, but not its texture)
- you can select *One level* and see the effect of contrast changes on
  that particular level, or fine tune the noise reduction
- you can select *Coarser details levels, without selected level* and
  *8*, to see the residual image along with the largest details and
  better appreciate the action of the various parameters of the module
  *Residual Image*
- you can select *Finer details levels, with selected level 4* and as a
  background *Residual Image*, to see the modifications in the finest
  details in their context, without the larger details masking what you
  are doing

### Example (the preview)

Below is a sample image with minimal processing that will be used in all
the subsequent examples. Next to it, from left to right you will see the
*level 2* details, the *level 4* details and the *residual image*.

In the two examples showing the details, the decomposition has been done
with ***Edge performance*** set to *D6 - standard plus*, the color
*gray* has been selected as ***Background***. In addition, to isolate
the detail,*One level'' has been selected in***Process**''.

The *Residual Image* is the result of removing all of the details after
choosing *5 Wavelet Levels*.

<span style="font-size: 0.7em; font-style: italic;">  ***To enlarge the
images click on them and when the new page loads, click on the image a
second time***. </span>
SSZZZ

<div>


![](wavelet_pic.jpg "wavelet_pic.jpg")

![](wavelet_config_l2.jpg "wavelet_config_l2.jpg")


![](wavelet_config_l4.jpg "wavelet_config_l4.jpg")

![](wavelet_config_ri.jpg "wavelet_config_ri.jpg")


</div>

## Contrast module

In this module you can modify the lightness contrast (*L\** component of
decomposition) of the details in each level independently. This allows
you to increase the contrast of smaller details to give an impression of
greater sharpness, while reducing the contrast of larger details. A
practical benefit of this approach is that by reducing the overall
contrast (the large details), you do not have to increase the fine
details as much to achieve an impression of sharpness. This makes it
easier to avoid introducing artifacts.

### The attenuation curve

As discussed in an earlier chapter, the wavelets tool calculates the
mean and standard deviation for each decomposition level and will use
these values in all of the modules.

In the case of the Contrast module, the first step is to set the
contrast slider values for each decomposed level depending on the effect
required. However, if you only perform this action, the contrast
variations would be proportional to the original contrast (
[homothetical modifications](https://en.wikipedia.org/wiki/Homothetic_transformation),
as in the *Contrast by Detail Levels* tool) and it would be quite easy
to generate artifacts.

To overcome this problem, the contrast values for the details in each of
the levels are analyzed and sorted before being modified and
progressively attenuated similar to the following curve :


![](wavelet_beta.png "wavelet_beta.png")


Broadly speaking and for each level, the graph shows that:

- the lowest contrast values are to the left and the highest contrast
  values to the right
- the contrast value set for each level (*contrast* in the graph)
  defines the maximum modification that will be applied to the contrast
  values present in the level
- the modification will be maximum around the average contrast value of
  each level (the *mean* value on the graph)
- the more the contrast values vary from the average contrast value, the
  less they will be changed
- high or strong contrast values are more attenuated than low ones

This means that for each level, the biggest contrast changes will be
made to the mid-contrast values while avoiding the extreme values to
avoid excessive effects or artifacts. However, keep in mind two
fundamental points:

1.  the mean contrast value is the arithmetic mean **of the contrast
    values present in the level**: if all the contrast values are high
    (strong contrasts), the mean value will also be high and the extreme
    contrasts in that particular level will be modified less
2.  each level has its own average value, which depends on the contrast
    values present in the details of that particular level

### Contrast Levels

![](wavelet_contrast_buttons.jpg "wavelet_contrast_buttons.jpg") 

The
number of levels shown is defined by the ***Wavelet levels*** and you
can reduce or increase this number in the wavelet configuration
settings.

The ***Contrast -*** and ***Contrast +*** buttons make it easier to
progressively change the values of each level: stronger in the first
levels and more discreet in the last. As you can see in the example, the
progression is homogeneous: starting from the *Extra level*, which has
not been modified, each level has been increased by 31 units with
respect to the previous level (the actual amount will depend on the
number of times you click on the Contrast+ or the Contrast- buttons).

In general these buttons allow you to define a logical progression of
[microcontrast](edges_and_microcontrast#microcontrast)
values: higher for the first levels and lower for the last levels.

Don't forget that if a level has uniform contrast, the slider action for
that level will not have any effect (if there are no details, nothing is
changed).

Note that the residual image is not included in this group of controls
because it is not a level: it is what is left of the original image
after removing all the details distributed across all of the levels.

### Attenuation and selectivity in contrast changes

There are 3 sliders that allow you to adjust the curve for each level,
as explained in [Analysis of the contrasts in each level](#analysis_of_the_contrasts_in_each_level):

1.  ***Attenuation Response***: by selecting positive values the upper
    part of the curve becomes wider around the medium contrast area, and
    is weighted towards the higher contrasts. Conversely, selecting
    negative values narrows the curve, thus reducing the range of
    contrasts that undergo any noticeable modification. Graphically:
    <div>

  
      ![](wavelet_beta+damper.png "wavelet_beta+damper.png")



      ![]wavelet_beta-damper.png "wavelet_beta-damper.png")


    </div>
2.  ***Offset***: shifts the top of the curve, so that the strongest
    contrast modifications are no longer made to the medium contrasts.
    By shifting the curve to the right, the higher contrast values will
    vary more, whereas with negative slider values, the lower contrast
    values will be modified more. Graphically:
    ![](wavelet_beta+offset.png "wavelet_beta+offset.png")
3.  ***Low contrast threshold***: this is the minimum contrast value
    that the details in the decomposition level must have for them to be
    taken into account. Lower contrast values, which have a value lower
    than the minimum value, will not be taken into account when
    calculating the mean of that level, nor will they undergo any
    variation, whatever the slider settings. In this way we can avoid
    highlighting noise or finer and more delicate textures.

### Apply To

This control block allows you to decide whether changes in contrast in
individual levels apply to all the details or only to those details with
pixels that are within a given range of
[luminance](https://en.wikipedia.org/wiki/Luminance). This allows you,
for example, to increase the contrast of fine details with high
luminance and reduce the contrast of larger details with low luminance.

In the drop-down list, you decide where to apply the contrast changes:
over the whole range of luminance values (i.e. to all the details in
each level) or only to details that have a certain luminance value.

#### Luminance ranges

If you have selected the *Whole luminance range*, the modification will
apply to all of the details in each of the levels. However, if you
choose *Selective luminance range*, you can decide which details will be
modified in which levels.

In addition, after selecting the *Selective luminance range* two
threshold curves and two sliders will appear allowing you to customize
the result. i.e.:

- **Finer levels luminance range**:
  - this is a small area with a black and white gradient and four points
    that define the range of luminance values that will be affected by
    the change in contrast
    <div style="overflow: hidden">


    ![](wavelet_contrast_highlight.jpg "wavelet_contrast_highlight.jpg")

    </div>
  - If you move your mouse over it, you will see where the default
    limits are: *Bottom-Left: 50, Top-Left: 75, Top-Right: 98,
    Bottom-Right: 100*. This range covers the highlights
  - these are the luminance values that must be in the image for the
    contrast change to be applied to the details (see following slider
    explanation)
  - the default values are as follows:
    - details with luminance of 50 or less will not be changed
    - details with a luminance of 50 to 75 will be subject to an
      increasing amount of modification
    - between 75 and 98, 100% of the modification will be applied
    - between 98 and 100, progressively less change will be applied
  - to change the values of the points on the curve, we have two
    options:
    - click and move one of the two points on one side (left or right)
      and slide the two points together
    - press the *shift* key, click on a point and slide it to move only
      that point
  - the default values are set for the highlights but you can modify the
    points to cover any part of the range from the shadows to the
    highlights as required.

<!-- -->

- **Finer levels**: only levels from the selected value and below will
  be affected by the *Finer levels luminance range* threshold curve.

<!-- -->

- **Coarser levels luminance range**:
  - another small area with a black and white gradient and four points
    that define the range of luminance values that will be affected by
    the change of contrast
    <div style="overflow: hidden">


    ![](wavelet_contrast_shadow.jpg "wavelet_contrast_shadow.jpg")


    </div>
  - again, by hovering your mouse over it, you will see where the
    default boundaries are around the shadows: *Bottom-Left: 0,
    Top-Left: 2, Top-Right: 25, Bottom-Right: 50*
  - the default levels are:
    - details with luminance between 0 and 2 will be subject to an
      increasing amount of contrast change
    - between 2 and 25, 100% contrast modification will be applied
    - between 25 and 50, progressively less change will be applied
    - from 50 onwards no change will be applied
  - the default values are set for the shadows but you can modify the
    points to cover any part of the range from the shadows to the
    highlights as required.

<!-- -->

- **Coarser levels**: only those levels from the value set with this
  slider up to the selected number of *wavelet levels* will be affected
  by the *Coarser levels luminance range* threshold curve.

No modifications will be made to any level that is not included in
either a *Finer levels* or a *Coarser levels* selection, no matter what
values have been set with the Contrast sliders. For these levels the
final result will be the same as setting a contrast-slider value of *0*.
</br>

#### Case Studies

- you are using 7 levels and only want to change *level 7* within the
  range set by the *Coarser levels luminance range* threshold curve:
  adjust the slider for *Coarser levels* to *7*
- you are using 7 levels and only want to selectively modify the finest
  details: set the *Finer levels* to the highest level you want to
  modify, and set the Contrast sliders for the rest of the levels to *0*
- you are using 7 levels and you want to selectively adjust levels *1*
  and *2* in accordance with the luminance values set in the threshold
  curve for *Finer levels luminance range* and adjust levels *6* and *7*
  in accordance with the luminance values set in the threshold curve for
  the *Coarser levels luminance range*: set the *Finer levels* slider to
  *2* and the *Coarser levels* slider to *6*. This will selectively
  modify levels *1*, *2* and *6*, *7* in accordance with the relevant
  threshold curve settings and the details in levels *3*, *4* and *5*
  will remain unchanged

### Example (changing contrast)

The sample image is shown below and next to it, from left to right, are
several different possibilities when a contrast increase is applied to
all levels (after pressing *15 times* on the button ***Contrast +***).

First the effect on the ***Whole luminance range*** is shown and to the
right the effect if you set the ***Selective luminance range**''.
Finally, an example of how the changes can be nuanced by
the***Strength**'' slider.

The sliders not mentioned have been left at their default values (the
control points on the curves, ...).

<div>


![](wavelet_pic.jpg "wavelet_pic.jpg")

![](wavelet_contrast_15c+_wl.jpg "wavelet_contrast_15C+_wl.jpg")

![](wavelet_contrast_15c+_h3s6.jpg "wavelet_contrast_15c+_h3s6.jpg")

![](wavelet_contrast_15c+_h3s6_str50.jpg "wavelet_contrast_15c+_h3s6_str50.jpg")

</div>

And now both the original image and the final image, side by side to
better appreciate the differences: you can see an increase in the
sharpness of the texture of the petals, without ruining the overall
effect.

<div>

![](wavelet_pic.jpg "wavelet_pic.jpg")


![](wavelet_contrast_15c+_h3s6_str50.jpg "wavelet_contrast_15c+_H3s6_str50.jpg")


</div>

## Chroma module

This module works in a similar way to the contrast module, except that
in this case, the tool analyzes the color contrast (components *a\** and
*b\**).

In the drop-down list ***Chrominance method*** you have the following
options:

- *Whole chroma range*: with this option, any change in any level will
  affect the full range of chroma, regardless of the values that have
  been set in the *Contrast module* levels.
- *Saturated/pastel*: here you can modify two threshold curves that act
  simultaneously and limit the pastel and saturated tones, regardless of
  the values in the *Contrast module* levels.
- *Link contrast levels*: the changes in chroma will be directly related
  to those made at each level of the *Contrast module*.

When you select *Whole chroma range* or *Saturated/pastel* you can use
the *Neutral* button to reset all the level sliders to their default
value (0).

In addition, there is a *Attenuation Response* slider for all 3 options,
which will act in the same way as described in the
[section on the attenuation of the Contrast module](#attenuation_and_selectivity_in_contrast_changes).

### Whole chroma range

If you choose this option, the entire chroma range in the image is
changed, regardless of how saturated each color is already.

The same observation as for contrast applies here: for there to be
changes in color, there must be a pre-existing color variation in the
level. If a level has a uniform color, the slider will have no effect.

The modifications at each level are limited to the range *\[-100,+100\]*
: the value *-100* is the equivalent of completely desaturating the
level, while the value *+100* increases the chroma of each detail. This
method almost always introduces artifacts because the formula that is
applied to the color value for each detail does not take into account
whether there are any deviations from the initial value.

<div>


![](wavelet_contrast_15c+_h3s6.jpg "wavelet_contrast_15c+_h3s6.jpg")

![](wavelet_chrom_wc_full.jpg "wavelet_chrom_wc_full.jpg")

![](wavelet_chrom_wc_l1l2full.jpg "wavelet_chrom_wc_l1l2full.jpg")

![](wavelet_chrom_wc_detail.jpg "wavelet_chrom_wc_detail.jpg")


</div>

The above examples mean that you should only make subtle changes with
this option because depending on the level and the strength of the
change, it is very easy to introduce highly visible artifacts. However,
if the changes are too subtle, they will hardly be noticeable. In all
cases the chroma noise will be affected and will increase significantly.

### Saturated/pastel

With this option, the color changes in each level are focused on the
saturated tones of levels with finer details and on the pastel tones of
the other levels (with coarser details).

After selecting this option, a threhold slider and two threshold curves
will appear, which operate in the same way as the contrast threshold
curves above.

- **Saturation/pastel threshold**
  - with this control you decide from which level to switch from
    saturated to pastel tones
  - the default value is *5*, i.e. in the first 5 levels the saturated
    tones will be changed, and in the higher levels the pastel tones
    will be changed
  - please note that if this value is higher than the number of levels
    of the wavelet decomposition, only the saturated tones will be
    changed
  - on the other hand, if you choose *1* (the level with only the finest
    details), it is as if you only modify the pastel tones
- **Pastel chroma range**:
  - the threshold curve is the same as for the contrast. The points
    define the saturation level for which a change in color will be
    effective


  ![](wavelet_chrom_pastel.jpg "wavelet_chrom_pastel.jpg")
  - it should be noted that the dark area of the gradient corresponds
    to the pastel tones and the lighter area corresponds to the
    saturated tones (following [this explanation of saturation](https://en.wikipedia.org/wiki/Colorfulness))*
  - hovering the mouse over it, you can see the limits: by default the
    values presented are *Bottom-Left: 0, Top-Left: 2, Top-Right: 20,
    Bottom-Right: 30*.
  - changes to the curve are made in a similar way to those made to the
    contrast curves
- **Saturated chroma range**
  - hovering the mouse over it, you will see where the limits are: by
    default the values shown are *Bottom-Left: 30, Top-Left: 45,
    Top-Right: 100, Bottom-Right: 130*


  ![](wavelet_chrom_chrom.jpg "wavelet_chrom_chrom.jpg")


  - although the values of both curves do not overlap, you can see an
    overlap on the graphical interface. And in practice it seems that
    changes around the threshold level affect both the saturated and
    pastel tones. To be able to see clearly whether there is an effect
    or not (depending on whether the tone is pastel or saturated), it is
    necessary to use very saturated or very *desaturated* (pastel)
    values.

Nonetheless, as with the *Whole chroma range* option, the changes are
not noticeable unless you are willing to introduce fairly visible
artifacts.

<div>


  ![](wavelet_contrast_15c+_h3s6.jpg "wavelet_contrast_15c+_h3s6.jpg")


  ![](wavelet_chrom_wc_l1l2full.jpg "wavelet_chrom_wc_l1l2full.jpg")


  ![](wavelet_chrom_sp_l1l2full_l3_60.jpg "wavelet_chrom_sp_l1l2full_l3_60.jpg")


  ![](wavelet_contrast_15c+_h3s6.jpg "wavelet_contrast_15c+_h3s6.jpg")


</div>

As you can see, despite applying 100% changes in some levels, the
differences are subtle and may appear to be negligible if you don't look
closely. The most visible changes are the more intensely colored «veins»
in the petals.

### Link contrast levels

This option is an interesting one because the changes in chroma are
directly related to those made to each of the contrast levels.

The ratio between the changes in contrast and color is adjusted with the
***Chroma-contrast link strength*** slider: thus *0* will have no effect
on chroma, while *100* will provide the maximum effect, and is more
intense than for the *Whole chroma range* option (particularly
noticeable in *chroma noise*).

Keep in mind that if you apply strong changes to the contrast levels,
they will also appear in the chroma and will most likely generate
undesirable artifacts: your best ally will always be the
***Chroma-contrast link strength*** control, to achieve clearly visible
effects without producing artifacts that will ruin the photo.

### Example (changing chroma)

<div>


  ![](wavelet_chrom_link_100.jpg "wavelet_chrom_link_100.jpg")

  ![](wavelet_chrom_link_50.jpg "wavelet_chrom_link_50.jpg")


  ![](wavelet_chrom_link_50_str50.jpg "wavelet_chrom_link_50_str50.jpg")



  ![](wavelet_contrast_15c+_h3s6_str50.jpg "wavelet_contrast_15c+_h3s6_str50.jpg")


</div>

The modifications to the original image have been exaggerated so that
the results are clearly visible. Consequently, the contrast and color
modifications made to the last photo have introduced blue edges on the
petals, halos around the anthers of the stamens and a noisy background.
Despite this, the image is not a complete disaster given how aggressive
the modifications are. At this point it is worth noting the intensity of
the color in the «veins» of the petals.

## Gamut module

This module is linked to the [Contrast](#contrast_module) and
[Chroma](#chroma_module) modules, so that adjustments can be
targeted as a function of the chroma in the details. In other words, for
the details in each of the wavelet levels, you can not only take into
account the contrast of the luminance (contrast module) or the contrast
of the tones (chroma module), you can also choose the color range that
these modifications will be applied to.

### Reduce artifacts in blue sky

Digital images often have speckled noise in the blue colors of the sky.
Wavelet processing can accentuate this noise or generate small artifacts
because it increases local contrast.

This checkbox introduces a [median filter](https://en.wikipedia.org/wiki/Median_filter) to reduce these
artifacts, at the expense of loss of detail and generation of artifacts
in areas where there are changes in tone or which have high contrast.
Although useful for fast and undemanding processing, you will actually
achieve better results with a judicious combination of the
*[Noise Reduction](noise_reduction)* tool in the Detail tab and the
*[Denoise and Refine module](#denoise_and_refine_module)* in
this tool.

<div>


  ![](wavelets_gamut_nosky.jpg "wavelets_gamut_nosky.jpg")


  ![](wavelets_gamut_sky.jpg "wavelets_gamut_sky.jpg")


</div>

### Skin hue

Although the title refers to *skin hues*, the adjustment is not
restricted to these and you can specify the range of tones you want to
modify. The selected range will govern the changes made by the other
controls in the module. However, the default range is for the usual skin
tones.

For the examples that follow, the following (rather restrictive) range
of red tones has been chosen:


![](wavelets_gamut_skin_hue.jpg "wavelets_gamut_skin_hue.jpg")


### Skin targetting/protection

This allows you to modify the contrast and/or color of details that have
colors included in the above range:

- with the slider at *0* all the colors of the image are modified
  equally
- selecting *-100* (sliding left) centers the contrast and color changes
  **in the selected color range**
- on the contrary, if you select *100* (by sliding right) the colors
  that **do not** coincide with the selected range will be modified

In the intermediate positions between *0* and *±100* the changes
increase progressively towards either the chosen range, or towards the
rest of the colors.

<div>

 ![](wavelets_gamut_skin.jpg "wavelets_gamut_skin.jpg")\]


  ![](wavelets_gamut_skin_target0.jpg "wavelets_gamut_skin_target0.jpg")



  ![](wavelets_gamut_skin_target-100.jpg "wavelets_gamut_skin_target-100.jpg")


  ![](wavelets_gamut_skin_target+100.jpg "wavelets_gamut_skin_target+100.jpg")

</div>

### Curve

Once you've set the desired *Skin targetting/protection*, you can use
this graph to fine tune the contrast/chromaticity variation for each
color: moving a control point up will increase the variation for that
color, while moving it down will mitigate the changes for that
particular color (although it won't eliminate the effect entirely).

However, only those colors within the range selected above will be taken
into account regardless of the colors modified with the curve.

<div>


  ![](wavelets_gamut_curve_target100.jpg "wavelets_gamut_curve_target100.jpg")


  ![](wavelets_gamut_curve.jpg "wavelets_gamut_curve.jpg")


</div>

### Avoid color shift

Processing by Wavelet Levels can introduce significant hue changes,
especially near the limits of the color range of the
[working color space](color_management#working_profile). Activating this
option makes a series of corrections to ensure that the resulting hue is
related to the initial color.

## Toning module

This module can be used for color toning specific detail levels as
required.

However, it is not possible to act directly and accurately on the hue in
each individual level because the components *a\** and *b\** have been
decomposed and it is very difficult to create a precise mathematical
relationship between the selected hue and the decomposed components.

Still, you can control to some extent which hues will be modified and
decide which color dominants they will take.

As with the other modules, there is an *Attenuation Response* slider,
which will act in the same way as described in the
[chapter on the attenuation in the Contrast module](#attenuation_and_selectivity_in_contrast_changes).

### Excluded Colors

The ***Excluded Colors*** graph is based on the color distribution of
the chromatic coordinates used for the L\*a\*b\* color space: the
horizontal axis represents the a\* component (going from green to red)
and the vertical axis represents the b\* component (going from blue to
yellow).

However, because it is complicated to represent the actual L\*a\*b\*
space color distribution in two dimensions, the pastel shades as shown
in the interface, while being mathematically accurate, are not visually
intuitive, especially when selecting yellow tones. Perceptually they are
equivalent to a graph such as the one below:


![](cielab_8x8.jpg "cielab_8x8.jpg")


In the center of the graph there is a white dot which, when dragged,
will produce a second black dot. These two dots define the centers of
the color ranges that will be protected to a greater or lesser extent by
any toning adjustments subsequently carried out in this module. Putting
the white dot on a particular color on the graph defines the center of
the first color range. Similarly, the position of the black dot defines
the center of the second color range. If the black dot is not moved from
the initial position at the center, then the second range is ignored.

With the slider ***Range a and b %*** a zone of influence is created
around the center as defined by the position of the dot on the graph and
the slider % determines how large the zone will be.

With the ***Protection*** slider, the effect of any adjustments on the
selected colors is reduced in the zone of influence (center plus range).
The protection slider value corresponds to the % of the protection
effect and will decrease as you move away from the center, until at the
end of the range (at the periphery of the zone of influence) the
reduction is equivalent to half the established value.

For example: *Protection=80* means that the protection is 80% and
therefore the center of each range will only receive 20% of the toning
values set in the equalizer modules (explained below). As we move away
from the center and until we reach the limit set by *Range a and b %*,
the toning will become progressively more intense until it reaches a
maximum of half of the *Protection* value. In this case it would be 40
meaning that the colors on the periphery would undergo 60% of the set
value.

### Toning controls

In this group of controls, two curves are presented:

- the ***Opacity Red-Green*** (the *a\*-curve*) which acts on the
  red-green tones
- the ***Opacity Blue-Yellow*** (the *b\*-curve*) which acts on the
  blue-yellow tones

But don't forget that the final colors of the photo will be a
combination of the tones of these two curves. For example: if you modify
the *a\** curve (***Red-Green Opacity***) to red, all the tones of the
level you are modifying will take on a red/reddish tone, but will not
necessarily become red (if they also had a strong blue component, they
would turn towards magenta/violet).

From a practical point of view: a tone can become more or less saturated
up *to a certain limit* and at the same time undergo a change in hue. To
better visualize these effects, take a look at this
[view from above the *L\*a\*b\* color space*](https://upload.wikimedia.org/wikipedia/commons/0/06/CIELAB_color_space_top_view.png),
with *b\** as the vertical axis and *a\** as the horizontal axis. And
don't miss this [front view of the *L\*a\*b\* color space*](https://upload.wikimedia.org/wikipedia/commons/7/7d/CIELAB_color_space_front_view.png).
The bottom of the top view matches the front of the front view.

In the interface you will find two *curve types:* *Linear*
(![](wavelet_toning_linear.jpg "wavelet_toning_linear.jpg")) and
*Equalizer* (![](wavelet_toning_curve.jpg "wavelet_toning_curve.jpg")).
To choose between one or the other, click on the small triangle on the
right.

The *linear* curve cancels the effect of the axis to which it refers: if
you select it in the ***Red-Green Opacity**'', you will not perform any
action on those tones. Similarly with the***Blue-Yellow Opacity**''.

In each *equalizer*curve there is a horizontal axis (or *x* axis) and a
vertical axis (or *y* axis):

- the *x* axis represents the 10 possible levels, in ascending order
  from left to right and evenly distributed
- the *y* axis represents the intensity of the modification: when the
  curve rises above or falls below the mid-line the color is modified
  towards one end or the other of the axis of the component being
  modified (*a\** or *b\**)
- in the ***Red-Green Opacity*** (the *a\*-curve*), moving the curve
  upward introduces a reddish tint, while moving it downward introduces
  a greenish tint
- in the ***Blue-Yellow Opacity*** (the *b\*-curve*), moving the curve
  upward introduces a yellow tint, while moving it downward introduces a
  bluish tint

By default, the curve is flat and lies on the mid-line. To get an idea
of how you can interact with the curve, see the explanations of the
*[Tone Curves](exposure#tone_curves)*. And remember that if
you don't like the changes you have made to the curve, you can always
start over by clicking the reset arrow
![<File:resetbutton.png>](/images/resetbutton.png "File:resetbutton.png").

As long as there are variations in contrast in the original image color,
these curves will allow you to selectively vary the tone of the desired
details. The resulting changes depend on where you place the points in
the curve and the amplitude of the modification (i.e. the number of
levels it affects). Everything has to be done «by eye», as there is no
reference to the levels on the *x* axis, however you can see the effect
of the modification by looking at the preview.

If you use less than 10 levels, the points affecting the rightmost
levels will simply be ignored: if you are modifying an image with 4
levels, the rightmost 6 (the ones with the largest details) will be
ignored.

### Example (applying toning)

You will recall that we had some ugly blue halos around parts of the
flower, so let's try to eliminate them (or at least hide them) with the
toning controls. We take advantage of the fact that most of the image
has a red dominant so we can modify the blue component, without it being
too noticeable in the overall result. For this example, none of the
colors have been excluded:

<div>

  ![](wavelet_chrom_link_50_str50.jpg "wavelet_chrom_link_50_str50.jpg")


  ![](wavelet_toning_opbyfull.jpg "wavelet_toning_opbyfull.jpg")



  ![](wavelet_toning_opbyfull_curve.jpg "wavelet_toning_opbyfull_curve.jpg")

</div>
<div>

![](wavelet_toning_opby.jpg "wavelet_toning_opby.jpg")


![](wavelet_toning_opby_str50.jpg "wavelet_toning_opby_str50.jpg")


![](wavelet_toning_opby_curve.jpg "wavelet_toning_opby_curve.jpg")

</div>

There are still some traces of blue halos in the final result although
they are not as visible, and the overall appearance of the photo seems
to be the same.

## Denoise and Refine module

This module complements the general **[Noise Reduction](noise_reduction)** tool (in the *Detail tab*) and
**Edge Sharpness** (explained in the next section).

Noise management is a complex issue because decisions have to be made as
to where should it be done in the processing pipeline (e.g. at the
beginning or at the end), what should be done and how.

In RawTherapee the general noise reduction tool is placed at the
beginning of the processing pipeline to prevent subsequent tools from
enhancing the noise to unacceptable levels. In the *Noise Reduction*
tool in the Detail tab you have the following possibilities:

- process the luminance (also based on wavelets) as a block, i.e. with
  no distinction between the wavelet levels
- process the color noise using a different method: this generally
  requires a higher number of wavelet levels (4 to 7) and more complex
  processing
- add *Fourier Transform* processing to refine the luminosity
- add a median filter

Although this may be sufficient, use of the Wavelet Levels tool can
provide some additional benefits (even though it uses the same algorithm
as the general tool):

- it is at the end of the processing pipeline, thus reducing the impact
  of noise added by other general tools (*Exposure*, *Curves*, *Dynamic
  Range*, etc.)
- it acts separately and independently on each of the first 4 levels,
  whereas the *standard* noise reduction has an effect on the entire
  image. This is especially useful for low noise images and for images
  where the general tool has been used sparingly to preserve detail
  (i.e. to reduce rather than to eliminate the noise)
- it reduces the incidence of noise in the other wavelet processing
  modules, e.g. allowing you to process skies without exaggerating the
  noise
- it adjusts both noise processing and the degree of contrast
  amplification/reduction at each level, which is useful for example for
  astronomical images

### The controls

You can adjust the noise reduction by levels as required with the
following set of controls, which not only decide what noise to act on,
but also link its effect to the *Edge Sharpness* module and the chroma
denoise.

#### Link with Edge Sharpness' Strength

This option will modify the behavior of the lower slider of each level
(explained below).

- if you choose **not to activate it**, then the lower slider of each
  level (the **Strength** slider) will have a similar effect to the
  Contrast module when it is used over the *Whole luminance range*
- if you activate it, you can change the distribution of the sharpness
  improvement in the first levels with the lower slider (this is
  explained in more detail in the next module, *Edge Sharpness*)

#### Denoise equalizer White-Black

Human vision is able to distinguish noise more easily in light areas
than in dark areas, even when there is more noise in the dark areas (the
shadows).

With this slider, noise reduction can be increased either in the shadows
(with values to the right) or in the highlights (with values to the
left).

It is easier to adjust if you choose an area with both light and dark
areas, so that you can see the difference between the noise levels in
the shadows and in the highlights.

#### Denoise and Strength

These sliders are used to control noise in the 4 finer-detail levels of
the image:

- the upper slider of each level performs the **Denoise**.
- the lower one, called **Strength**, modifies the contrast of the
  details for that particular level. It should be noted that this
  adjustment isn't as sophisticated as the adjustments made in the
  Contrast module

Although the *Strength* slider may seem redundant, it is quite useful
for recovering lost contrast in the details when higher values of
*Denoise* have been applied. That way you don't have to jump from one
module to another to quickly adjust the image. It also serves to
modulate the distribution of the effect on the first 4 levels of the
*Edge Sharpness*.

#### Denoise chrominance

The chroma denoise is complementary to the *Noise Reduction* tool in the
Detail tab.

Because the wavelet chrominance denoise is at the end of the processing
pipeline, it is useful for removing any chrominance noise that wasn't
removed when using the noise reduction in the Detail tab, or noise that
has been generated by other tools.

In this group of sliders you will find:

- the **Denoise Equalizer Blue-Red**: chrominance noise usually comes in
  the form of red or blue dots and with this slider you can increase the
  reduction of the blue dots (to the left), or the red dots (to the
  right)
- the **Chrominance Fine** slider: reduces the chrominance noise in the
  finest details, i.e. at the lowest levels
- the **Chrominance Coarse** slider: reduces the chrominance noise in
  the coarser details, i.e. at the higher levels. This noise can be seen
  as patches of color that appear to be «dirty» or «do not belong» to
  the image and that cannot be removed with the *Chrominance Fine*
  slider because of their size

### Example (applying denoise)

To get a better understanding of the extent to which the noise levels
can be improved, it is useful to proceed on a level-by-level basis,
taking advantage of the fact that you can view the detail in each
individual level on a neutral background (as explained when dealing with
the *[Preview](#the_preview)*). Turn off *Link with Edge
Sharpness' Strength* and then increase the *Strength* slider of the
level you are working on to the maximum: the noise will become obvious
and you will be able to assess how much denoise is needed. Once you have
adjusted the *Denoise* slider, move the *Strength* slider to the value
that suits you best (negative values can also be used) and move to the
next level.

<div>


  ![](wavelet_denoise_orig.jpg "wavelet_denoise_orig.jpg")


  ![](wavelet_denoise_l2d00s100.jpg "wavelet_denoise_l2d00s100.jpg")



  ![](wavelet_denoise_l2d30s100.jpg "wavelet_denoise_l2d30s100.jpg")



  ![](wavelet_denoise_l2d30s27.jpg "wavelet_denoise_l2d30s27.jpg")


</div>
<div>

 ![](wavelet_denoise_l1d00s00.jpg "wavelet_denoise_l1d00s00.jpg")
>


  ![](wavelet_denoise_l1d00s100.jpg "wavelet_denoise_l1d00s100.jpg")



  ![](wavelet_denoise_l1d12s100.jpg "wavelet_denoise_l1d12s100.jpg")



  ![](wavelet_denoise_l1d12s-17.jpg "wavelet_denoise_l1d12s-17.jpg")


</div>
<div>

  ![](wavelet_denoise_orig.jpg "wavelet_denoise_orig.jpg")


  ![](wavelet_denoise_final.jpg "wavelet_denoise_final.jpg")



  ![](wavelet_denoise_orig100.jpg "wavelet_denoise_orig100.jpg")



  ![](wavelet_denoise_final100.jpg "wavelet_denoise_final100.jpg")

</div>

As a general rule, it is best not to eliminate the noise completely, but
simply reduce it so that it is only just visible and at the same time,
increase the contrast of the details in that particular level. By
amplifying the presence of the details, the noise will be ignored when
the image is viewed and the photo will have a light textured look. The
procedure is as follows:

1.  reduce the luminance noise slightly using the *Noise Reduction* tool
    in the detail tab, taking special care not to lose any detail
2.  select *one level*, *gray background* and *level 1*
3.  zoom in to 300-400% on an area with poor background detail (there
    must be enough detail with good contrast, but not so much that it
    masks the noise)
4.  move the *level 1 strength* slider in *Denoise* to the maximum (or
    almost), making sure that the details can still be distinguished
    from the noise
5.  move the top slider to a point where the noise reduction is
    medium-low (do not remove noise completely) and then return the
    lower strength slider back to its original position.
6.  the noise reduction adjustment will result in a loss of contrast in
    the details. To remedy this, increase the lower slider to a strength
    level that allows you to recover the initial contrast of the details
7.  switch to *level 2* and continue with points 4, 5 and 6, this time
    adjusting the *strength* of *level 2*
8.  continue in the same way with levels *3* and *4*
9.  finish the process by selecting *All levels in all directions*

If the image is not very noisy, you can go straight to step 2. However,
if the image is very noisy, it is important to adjust the luminance
noise reduction at step 1: you need to play with the *Luminance* denoise
slider and the *Gamma* slider (in the Noise Reduction tool), to direct
the noise reduction to the shadows or the highlights. The more care you
take with this step, the better the final result.

To increase the presence of detail you can use the lower sliders and
increase the strength of each level as much as you like, but it is
preferable to use the sliders in the Contrast module, as they offer more
control and give better results with fewer artifacts. Also, don't forget
that in this example only the *Denoise* and *Strength* sliders have been
used, but the result can be further refined if necessary with the other
sliders in this module.

Do not confuse the denoise in this module with the **Threshold low
(noise)** function used for **Edge detection** in the *Edge Sharpness*
module, which takes noise into account (without reducing it) to avoid
highlighting it when analyzing the edges

## Edge Sharpness module

This module applies a form of contour detection on the details in each
of the wavelet levels.

At first glance it looks like an **[Unsharp Mask](sharpening#unsharp_mask)**, because decomposition by
wavelet levels generates a *residual image* that looks a bit like a
mask, but that's where the similarities end.

If you want results that are similar to the *Unsharp Mask* or
*[Deconvolution](https://en.wikipedia.org/wiki/Deconvolution#Optics_and_other_imaging)*,
then you will need to select the *Edge detection* box and choose a high
value of *Gradient sensitivity* (70 or more; by default it will be 90).
It is better not to modify the first contrast levels in *Contrast by
Detail Levels*, because they can impair the way the algorithm works (all
of these options are explained in detail below).

Before explaining how to use the module, keep in mind the following to
avoid generating excessively strong artifacts or effects: both the
configuration of the *[Edge performance (D2, D4 ... D14)](#edge_performance)* and the
[strength of each level in *Denoise and Refine*](#denoise_and_refine_module) (when you
have *Link with Edge Sharpness' Strength* activated) have a noticeable
effect on *Edge detection*. Each time you make an adjustment, you need
to evaluate the result and readjust everything to obtain good sharpness
with the minimum of artifacts.

In the interface you have several control blocks:

- ***settings***: this first block allows you to adjust the way the tool
  detects the edges
- ***local contrast***: in this block you can decide how contrast
  changes are applied to the details based on their initial contrast
  values.
- ***edge detection***: to increase sharpness where it is needed most
  (i.e. at the edges)

### Configuration

***For the time being do not enable*Edge detection*, as the results will
be different if this option is activated.***

There are 4 sliders:

1.  **Strength**: is the amount of contrast enhancement applied to the
    details. The higher the *Strength* value, the greater the change in
    contrast. Its effect is stronger at finer levels and resetting this
    slider cancels out any changes in the rest of the module.
2.  **Attenuation Response**: this operates in the same way as the
    attenuation control described in the [chapter covering the attenuation of the Contrast module](#attenuation_and_selectivity_in_contrast_changes).
    It controls the extent to which the contrast values will be
    modified.
3.  **Radius**: generates a three-dimensional image effect and may give
    the impression of more *volume* in the details, or more pronounced
    *texture* and has an influence even if the slider is set to zero. It
    should also be noted that the effect of the *Radius* is modified by
    the value of the *Detail*.
4.  **Detail** : changes the way the contrast is distributed between the
    levels. The effect will be stronger in the first 3 levels if the
    cursor is moved to the right, whereas if you move it to the left
    (towards negative values), the contrast changes in the first 3
    levels will be practically canceled out.

The following example allows you to see how closely the changes in
contrast in each of the levels are related to the values of the *Radius*
and the *Detail* sliders, and how the effects of the *Radius* slider are
modified according to the value of the *Detail* slider. To illustrate
this we set *First level: Unchanged* (which will be explained later) and
*Link with Edge Sharpness' Strength* disabled (in the *Denoise and
Refine* module).

- **the Radius-Contrast ratio**: changing the value of the *Radius*
  modifies the contrast of the details. In general, the strongest
  contrast changes are observed between radii 40 and 75. Below 40, level
  1 is enhanced and above 75, level 3 and to a lesser extent the higher
  levels (the effect becomes increasingly softer the higher the level
  and at levels *9* and *Extra* the effect is negligible).
- **Radius-Detail relationship**: depending on the value of *Detail*,
  modifying the *Radius* increases, more or less, the contrast of
  details in one level or another.

The following is a summary of the main points. The graphical
representation makes it easier to understand:

<div>


  ![](wavelet_edge_sharpening_d-50.png "wavelet_edge_sharpening_d-50.png")



  ![](wavelet_edge_sharpening_unchanged.png "wavelet_edge_sharpening_unchanged.png")
  <figcaption>wavelet_edge_sharpening_unchanged.png</figcaption>


  ![](wavelet_edge_sharpening_d100.png "wavelet_edge_sharpening_d100.png")
  <figcaption>wavelet_edge_sharpening_d100.png</figcaption>


</div>

Just under the *Detail* control, you have a drop-down list with 3
options for the **First (wavelet) level**:

- *Reinforced*: the effect is increased at *level 1*.
- *Unchanged*: the distribution of the algorithm is unchanged.
- *Reduced*: the modification is reduced for *level 1*.

Being able to see the differences between these three options will
depend on the amount of fine detail in the image, how much contrast
there is in the details and the choice of coefficients used for the
decomposition (*D2*, *D4*, ..., *D14*): in night photos with overexposed
points of light (e.g. streetlamps etc.) the *Reduced* option will soften
any high-contrast noise in the first level. Often however, there are
hardly any relevant details in *level 1* so the option you choose will
probably not be important.

In addition, if you use the option *Reduced*, you may see a somewhat
strange or at least «different» behavior for *level 1*: the contrast
decreases progressively from a maximum at *Radius: 0* to an almost total
blurring of the details at *Radius: 19* and then jumps to another
maximum at *Radius: 20*. It then slowly reduces back to *Radius:100*.
Graphically:


![](wavelet_edge_sharpening_reduced.png "wavelet_edge_sharpening_reduced.png")
<figcaption>wavelet_edge_sharpening_reduced.png</figcaption>


The best thing to do is to choose *Unchanged*, because with the option
*Link to Edge Sharpness' Strength* the changes will be more progressive
and controllable.

### Link to Edge Sharpness' Strength

All of the above is valid as long as you do NOT activate the option
*Link to Edge Sharpness' Strength* (in *Denoise and Refine*). Not
activating it means that contrast changes will be made depending on the
values of *Radius* and *Detail*.

However, if you have activated the option *Link to Edge Sharpness'
Strength*, the strength settings for each *Denoise* level will regulate
the strength of the effect in each of the first four levels of *Edge
Sharpness*. This allows you to adjust the sharpness for certain levels
only and use significantly higher increases in contrast than can be
obtained with the 10 *Contrast* sliders.

For example, you can:

- leave *level 1* contrast unchanged
- increase the maximum strength in *level 2*
- reduce contrast in *level 4* (negative *Strength*)

### Local contrast

For each level of decomposition, the tool calculates the
[mean](https://en.wikipedia.org/wiki/Mean) and [standard deviation](https://en.wikipedia.org/wiki/Standard_deviation) of the
internal contrast of the details (also called *local contrast*) and then
uses the results for subsequent modifications.

Remember that a «detail» is actually a group of pixels in the original
image. The higher the wavelet level, the larger the group (at *level 1*
a group consists of 2x2=4 pixels, at *level 2* 4x4=16 pixels, etc).
Because each pixel has a different initial luminance from the rest of
the pixels in the detail, the tool can derive and analyze the internal
contrast between pixels.

Any modification to these internal or local contrast values is based on
a pattern (or curve) derived from the mean of the local contrast values
in the level and on their standard deviation. It is applied to the local
contrast values in the same way in each of the levels. i.e. centered
around the mean contrast values.

So, for example, you can:

- for low initial contrast values (usually located in the shadows):
  reduce local contrast to soften the detail
- for average values: enhance them by increasing the local contrast
- for high values (usually located in very bright areas): reduce or even
  remove the local contrast, to avoid *clipping* the highlights

You can choose between two graphical controls for setting the
parameters:

1.  a threshold curve with four movable points that represent (from left
    to right) the minimum contrast, the mean, the average + standard
    deviation and the maximum contrast
2.  a curve, which by default is an asymmetrical Gaussian type curve,
    with the following characteristics
    - the center of the
      [abscissa](https://en.wikipedia.org/wiki/Cartesian_coordinate_system)
      corresponds to the mean value of the contrast values
    - the area from the center covering one-third of the width of the
      graph on each side are the details with contrast values that are
      either higher or lower than the average value and lie within the
      range of values set by the standard deviation

***THE CURVE WITH SLIDERS***

<div class="parrpad">


![](wavelet_local_contrast_thresholds.jpg "wavelet_local_contrast_thresholds.jpg")
<figcaption>wavelet_local_contrast_threshold.jpg</figcaption>


</div>

If you choose the threshold curve, moving a point to the right will
increase its contrast, while moving it to the left will decrease it (the
whiter the background color, the higher the contrast, whereas a dark
background indicates low contrast).

As indicated above, the points correspond, from left to right, to the
minimum contrast, the mean, the mean + the standard deviation and the
maximum contrast. For example, moving the point that represents the mean
of the contrast values to the right will increase the contrast values
close to the mean. Similarly for the other points i.e. the tool modifies
each group of contrast values (mean, standard deviation, minimum and
maximum) in accordance with the position of the adjustable points.

In practice:

- Using the default values, the position of the points on the threshold
  curve usually gives a more *natural* look at the expense of
  exaggerated brightness, and in particular, specular brightness.
  Details however are enhanced more naturally and with more definition
  than with the Gaussian curve, without excessively highlighting noise
  or grain
- in daylight photos, which have a predominance of mid-tones (as in
  cityscapes), the default settings will highlight details without
  exaggerating their contrast (however reflections will be more
  pronounced)
- however, in high contrast photos (night photos with artificial
  lighting, astronomical photography) the default settings tend to
  exaggerate the contrast values of bright points of light
- in photos with high contrast (with a lot of contrasting noise): if you
  have selected *first level reduced* the effect on *level 1* decreases
  and the noise is quite smooth. However, in photos with moderate
  contrasts there is practically no difference

***THE GAUSSIAN CURVE***

<div class="parrpad">


![](wavelet_local_contrast_gauss.jpg "wavelet_local_contrast_gauss.jpg")
<figcaption>wavelet_local_contrast_gaus.jpg</figcaption>

</div>

In this case the shape of the curve serves as a visual guide when
modifying the contrast values of the details. Remember that the standard
deviation of the contrast values are one third to the right and one
third to the left of the center of the graph.

Compared to the threshold curve, this graph allows you to not only
modify the range of local contrast values that will be affected but also
the strength of the modification: if you move a point on the curve to
the right or left, you will change the range of contrast values that
will be affected (as with the slider points), whereas moving it up or
down will increase or decrease the strength of the changes in detail.

With the default curve shape (which you can reset with the
![<File:ResetButton.png>](ResetButton.png "File:ResetButton.png")
button) the effect achieved is similar to a
[HIRALOAM](https://www.ledet.com/margulis/Makeready/MA69-Life_on_the_Edge.pdf):
it enhances the contrast values by controlling the shadows, while making
strong contrasts less prominent. It is like highlighting the volume of
each detail, keeping both noise in the shadows and overexposure in the
highlights under control (especially with specular highlights). However,
the grain and noise of the medium contrast values are excessively
enhanced.

The threshold curve and the Gaussian curves will give different results
with their respective default values, so the choice depends on what you
are trying to achieve. However, when using the Gaussian curve the
*Strength* of the tool should be kept low to avoid overly exaggerated
results. With the threshold curve you can use higher strength values and
still achieve *natural* results. However, you can achieve the same
effect by adjusting the Gaussian curve, with the added advantage of
being able to vary the contrast values to either increase or reduce
them, or even flatten them completely by moving the curve below the
horizontal line).

<div>


  ![](wavelet_local_contrast_curves_o.jpg "wavelet_local_contrast_curves_o.jpg")
  <figcaption>wavelet_local_contrast_curves_o.jpg</figcaption>



  ![](wavelet_local_contrast_curves_t.jpg "wavelet_local_contrast_curves_t.jpg")
  <figcaption>wavelet_local_contrast_curves_t.jpg</figcaption>



  ![](wavelet_local_contrast_curves_g.jpg "wavelet_local_contrast_curves_g.jpg")
  <figcaption>wavelet_local_contrast_curves_g.jpg</figcaption>


</div>

### Edge detection

Before you start using this part of the module you need to adjust the
above parameters carefully, especially the local contrast. You should
only make subtle changes, as it is easy to generate exaggerated effects
and artifacts in the image.

By activating the edge detection, the result obtained will be different
from those of the traditional algorithms (unsharp mask,
deconvolution...), as the tool performs a series of operations on the
details of each level to highlight the edges without enhancing the
noise: it intensifies the details of the decomposition, blurs them to
remove the noise and selects those details that are considered to be
part of an edge.

The process is based on the Sobel-Canny algorithm, customized to fit the
components of the decomposition and reduce the necessary variables to 3
sliders:

- **Gradient sensitivity**: the more you move the cursor to the right,
  the more the detection algorithm will focus on sharp edges and the
  less it will take into account the local contrast values of small
  areas (such as noise or small details). Conversely, moving the cursor
  to the left will detect more edges, even the smallest ones, but it
  will also highlight noise.
- **Threshold low (noise)**: this slider configures a [Gaussian filter](https://en.wikipedia.org/wiki/Gaussian_blur) that does not
  directly modify the image, but rather the decomposition coefficients.
  On the left it acts on a 3x3 matrix, while on the right it acts on a
  larger 5x5 matrix. By blurring the image, noise and finer or lower
  contrast details are lost or obscured. This is good for mitigating
  noise, but it also results in less accurate edge detection. The
  farther to the right the cursor is, the better the noise will be
  mitigated, but fewer edges will be detected. A value around 3x3 is
  better for detecting fine edges, but is more prone to interference
  from noise. A value of 5x5 is better for wider or more prominent
  edges, at the cost of losing the finer edges and making detection less
  accurate.

In addition, in a later step of the algorithm, this threshold will
remove those edges that, although detected, are unlikely to be true
edges. The lower this threshold is, the more it will detect low-contrast
edges. However, it will also be more likely to interpret noise as a
probable edge.

- **Threshold high (detection)**: once the edges of the image have been
  detected, this slider allows the tool to analyze the reliability of
  the edge detection (i.e. whether it is a sharp or blurred edge) and
  then either attenuate or enhance the local contrast changes depending
  on how sharp the edge is. Moving the cursor to the right will enhance
  the contrast of the sharp edges and moving it to the left will
  attenuate it.

### Enhanced algorithm

Activating this part of the module allows you to configure certain
aspects of the edge detection algorithm:

- Edge sensitivity: this value allows you to discard details that do not
  have a higher contrast than the value set by the slider. The more the
  slider is moved to the right, the higher the contrast in the details
  must be for them to be considered as a possible edge (lower-contrast
  edges will be ignored).
- Base amplification: this slider intensifies the initial values before
  starting the calculations to improve edge detection. The further to
  the right, the better the distinction between *edge* and *non-edge*,
  but the greater the risk of artifacts.
- Neighboring pixels: here you decide what influence the pixels
  surrounding the detail will have on edge detection. You have 3
  options: *None*, *Low*, *High*.

### Example (modifying noise reduction and edge sharpness)

<div>

![](wavelet_edge_sharpness.jpg "wavelet_edge_sharpness.jpg")
<figcaption>wavelet_edge_sharpness.jpg</figcaption>


</li>
</ul>
</div>

## Blur levels module

This module allows you to selectively blur («defocus») the details of
selected levels. The result is stronger in the higher levels (from
*level 7* upwards) and is especially useful in astrophotography.

The ***Attenuation Response*** slider acts as described in the
[chapter on the attenuation of the Contrast module](#attenuation_and_selectivity_in_contrast_changes).

The ***Blur by levels*** curve modifies the luminance of each level: it
is divided into 10 zones, with *level 1* on the left and *Extra* on the
right. Raising the curve in the area of a level will increase the blur
for that particular level.

The ***Blur Chroma*** slider blends the colors with their surroundings,
creating a subtle smudge effect. It acts on the same levels defined by
the previous curve.

## Sharp-mask and Clarity module

The *Sharp mask* feature is a new way of enhancing sharpness alongside
the other methods available in RawTherapee (i.e. *Unsharp Mask*,
*Deconvolution* and *Wavelet Levels Edge Sharpness*). It can be used in
conjunction with them or on its own.

The *Unsharp mask* is the method that has been traditionally used to
accentuate edges and increase the impression of sharpness in an image:
it is based on the creation of a blurred Gaussian type mask generated by
taking each pixel and recursively blurring the neighboring pixels. This
mask is then subtracted from the original image to emphasise the edges.
The radii used with this method are usually small (in the order of less
than 1 to a few pixels).

The mask in this module is based on part of the wavelet decomposition
and can be used in two ways:

- enhancing the finer levels using the **Sharp mask** option.
- enhancing the coarser levels using the **Clarity** option, which
  increases the impression of local contrast and local saturation in the
  image.

When you activate the module, the general configuration of the tool will
automatically change to:

|             | Sharp mask               | Clarity                    |
|-------------|--------------------------|----------------------------|
| Background: | Residual Image           | Residual Image             |
| Levels:     | Levels with fine details | Levels with coarse details |
| Level:      | 3                        | 7                          |

In either case you can change the reference *level*. In the case of the
*Sharp Mask* the change of level is similar to changing the radius in
the *Unsharp Mask* and you can choose any level between *1* and *4*. For
*Clarity* a level change results in a greater or lesser
*three-dimensional volume* effect on the image. In this case you can
select the levels from *5* to *Extra*.

When you deactivate the module the *merge* values are not lost and you
will only have to re-select the desired *level* when you re-activate the
module.

The *merging* in the *Sharp Mask* consists of enhancing the details
equal to and below the selected level and merging (blending) the result with the
remaining levels: if you have selected *level 3*, the details of levels
*1*, *2* and *3* will be enhanced and then merged with levels 4 and
above (including the residual image). The *merge* slider allows you to
adjust the mix, giving more or less relevance to the enhanced details
with respect to the rest of the image.

Similarly, in the case of *Clarity* the coarser levels are enhanced and
merged with the rest of the photo.

In both cases you have 3 sliders to adjust the changes:

1.  *Merge Luma*: by moving it to the right (positive values) the
    contrast of the details is enhanced, while with negative values the
    image becomes *blurry, like a dream*.
2.  *Merge Chroma*: by moving it to the right (positive values)
    saturated tones are enhanced more than the less saturated (pastel)
    tones. With negative values, the image becomes less vivid and the
    pastel tones remain virtually unchanged.
3.  *Soft Radius*: high merge values can generate halos around the
    contrasting areas. With this control you can smooth them without
    affecting the image too much. However, it does have side effects:
    the dark areas become darker and more and more areas will be
    considered as not having sufficient contrast to be enhanced, so they
    remain unchanged.

<div>

  ![](wavelet_smc_original.jpg "wavelet_smc_original.jpg")

  ![](wavelet_sharpm_ml60mc30.jpg "wavelet_sharpm_ml60mc30.jpg")

  ![](wavelet_clarity_ml60mc30.jpg "wavelet_clarity_ml60mc30.jpg")


</div>

Finally, under the *Soft Radius* you have the option to *Show wavelet
'mask*', which allows you to see which details will be enhanced:

- in the case of the *Sharp Mask*, an image will be displayed with a
  black background and the details will be highlighted in white (if you
  have ever seen an *Unsharp Mask*, you will find it similar).
- however, with the *Clarity* option, the mask is different and is based
  on [Guided Image Filtering](https://kaiminghe.github.io/eccv10/index.html) and is less
  intuitive. In general, the white areas (although blurred) are the ones
  that will be highlighted.

## Residual Image module

You will recall that the residual image corresponds to the original
image minus the details that were extracted in the levels. Any changes
made in the levels will therefore have no effect on the residual image
and the more levels you select in the general settings of the tool, the
greater the difference between the original and the residual image.
Furthermore, when you select more than 6 levels, the residual image will
contain almost no noise, so changing the global contrast or chromaticity
in the residual image will have almost no effect on the noise.

<div>

![](wavelets_residual_original.jpg "wavelets_residual_original.jpg")


![](wavelets_residual_l5.jpg "wavelets_residual_l5.jpg")
  <figcaption>wavelets_residual_l5.jpg</figcaption>


![](wavelets_residual_l7.jpg "wavelets_residual_l7.jpg")
  <figcaption>wavelets_residual_l7.jpg</figcaption>


![](wavelets_residual_l8.jpg "wavelets_residual_l8.jpg")
  <figcaption>wavelets_residual_l8.jpg</figcaption>


</div>

It is important to note that to avoid artifacts and out of
[gamut](https://en.wikipedia.org/wiki/Gamut) tones, you will need to
closely observe the adjustments you make on each of the levels and on
the residual image: if the original image already has tones close to the
boundaries of the color space, significantly increasing the contrast or
chromaticity of the detail will almost inevitably result in artifacts or
tones outside the color range. In this case, increasing or decreasing
the residual image contrast and chromaticity will allow you to keep the
colors within the range defined by the color space.

As you can see, residual image adjustments are a fundamental aspect of
wavelet level processing. They allow you to:

- work with shadows and highlights regardless of the details they
  contain,
- reduce overall contrast and chromaticity, to better perceive
  micro-contrast,
- change the chromaticity to reduce artifacts resulting from excessive
  modifications in the levels (e.g. in the skies)
- etc

### Shadows/highlights of the residual image

Moving the sliders for Shadows and Highlights to the right increases the
luminance of those areas; to the left (negative values), it reduces it.
This can be used to further darken the shadows and increase the
brightness of the highlights:

<div>


  ![](wavelets_residual_original.jpg "wavelets_residual_original.jpg")
  <figcaption>wavelets_residual_original.jpg</figcaption>



  ![](wavelets_residual_sh.jpg "wavelets_residual_sh.jpg")
  <figcaption>wavelets_residual_sh.jpg</figcaption>



  ![](wavelets_residual_sh-out.jpg "wavelets_residual_sh-out.jpg")
  <figcaption>wavelets_residual_sh-out.jpg</figcaption>


</div>

The effect of these changes is influenced by the threshold sliders,
which have a luminance scale ranging from *0* to *100*: for the shadows,
moving the threshold slider to the left will progressively limit the
action of the slider to the darker luminance values. Similarly for the
highlights, moving the threshold slider to the right will progressively
limit the action of the highlights slider to the higher luminance
values. To help decide which threshold values to use you can check the
luminance values by hovering the mouse over the relevant area in the
image and reading off the L\* luminance value in the Navigator panel.
These values correspond directly to the scale used for the threshold
sliders.

If sliders for shadows and highlights have positive values, you can only
*recover* the shadows (lightening them) or highlights (darkening them).
The result is similar to the
[Shadows/Highlights](shadows/highlights) tool in the Exposure
tab. However, note that this tool does not have the ability to
[Reconstruct Highlights](exposure#highlight_reconstruction).

- negative values have a strong impact on the residual image
- highlights are darkened if negative values are used
- highlights are lightened if positive values are used
- shadows are darkened if negative values are used
- shadows become lighter if positive values are used

<div>


  ![](wavelets_residual_shneg_original.jpg "wavelets_residual_shneg_original.jpg")
  <figcaption>wavelets_residual_shneg_original.jpg</figcaption>



  ![](wavelets_residual_shneg_old.jpg "wavelets_residual_shneg_old.jpg")
  <figcaption>wavelets_residual_shneg_old.jpg</figcaption>


  ![](wavelets_residual_shneg_new.jpg "wavelets_residual_shneg_new.jpg")
  <figcaption>wavelets_residual_shneg_new.jpg</figcaption>


</div>

The last option in this group is the *Radius Shadows/Highlights*. Note:
this slider will be hidden when the *Algorithm using negative values*
option is activated.

This slider applies [Guided Image Filtering](https://kaiminghe.github.io/eccv10/index.html) that
attenuates the transitions in the shadows and the highlights caused by
the previous adjustments. It controls the area of influence of
adjustments made in the shadows and highlights to improve the way they
are integrated into the rest of the image.

You can adjust the shadows and highlights to:

- add impact to shiny objects,
- prevent the highlights from becoming saturated,
- lift the shadows,
- etc

### Residual Image Contrast Compression

This is one of the key aspects of wavelet level processing: the
***Contrast*** slider allows you to make changes to the contrast of the
residual image separately from contrast changes to the details in the
levels.

Moderate reductions in the residual image contrast will make the
contrast in the details stand out more clearly and give a greater
impression of depth and relief. This allows you to limit increases in
contrast for the details to avoid generating artifacts and at the same
time, produce an effect that is visually equivalent to a greater
increase.

However for certain images, drastic changes in the contrast of the
residual image will allow you to achieve some interesting effects:

<div>


  ![](wavelets_residual_contrast-.jpg "wavelets_residual_contrast-.jpg")
  <figcaption>wavelets_residual_contrast-.jpg</figcaption>



![](wavelets_residual_contrast+.jpg "wavelets_residual_contrast+.jpg")
  <figcaption>wavelets_residual_contrast+.jpg</figcaption>


</div>

To help you control the effect of this module, there is a set of options
that allow you to adjust the dynamic range of the residual image. These
options are grouped into *Contrast Compression* and *Tone Mapping
Compression*.

#### Compression Method: Contrast

The effects of the contrast slider are immediately visible and vary
almost linearly depending on the position of the cursor: to the right
the contrast will increase and to the left it will decrease. The action
of the slider is limited internally to avoid artifacts.

The **Compression strength** modifies the dynamic range of the residual
image: moving the slider to the right reduces it (shadows are lightened
and highlights are reduced) and to the left, increases it (shadows are
darkened and highlights are slightly intensified).

The **Compression gamma** modifies the distribution of light and shadow,
effectively moving the histogram to the left or right.

With these controls you can, for example, reduce the effects of fog or
compress the dynamic range of high dynamic range images.

#### Compression Method: Tone Mapping

In this case the compression method used is the same as the [Tone
Mapping](Tone_Mapping.md) tool and its sliders act in the same
way. It acts only in the residual image and will make in-depth
modifications to the contrast (in the same way as Tone Mapping). Because
of this, you will most likely have to readjust any changes in the levels
as well to ensure that the overall image remains balanced.

Although you are applying tone mapping to the residual image, this
doesn't prevent you from activating the global *Tone Mapping* tool as
well. In this case, care is needed because using both tools at the same
time can generate artifacts.

### Blur

This group of two sliders may seem trivial, but it can be an interesting
aid to improve the [bokeh](https://en.wikipedia.org/wiki/Bokeh) of the
image, by blurring the luminance of the residual image (***Blur
Luminance**'') and its color component (***Blur Chroma**'').

The result is closely linked to the values and to the number of [Wavelet levels](#wavelet_levels): to obtain a good bokeh you need to
be able to adjust the amount of detail in the image depending on whether
you want the background to be partially recognizable or completely
blurred.

Normally the best results will be obtained with 7 or more wavelet levels
and with modest blur values (around *50*), as it is easy to generate
halos and artifacts with lower levels or extreme values.

### Chroma of the residual image

The Chroma slider works on the same principle as described for the
contrast slider above, except that in this case it will change the
saturation of the residual image tones.

The *Intensity* control is linked to the values set in the sky hue
protection range as explained below.

#### Sky hue

The title refers to *sky hue*, but you are not restricted to sky colors
and can adjust the color tones without limitation. As mentioned
previously, the colours are linked to the *Intensity* control.

The default range covers the usual shades of blue in the sky.

#### Sky targetting/protection

With this control you can decide whether you want to modify, the chroma
of areas containing colors defined in the previous range:

- with the slider at *0* the changes made to the chroma control will be
  applied equally to all tones in the image
- selecting *-100* (sliding left) centers chroma changes in the selected
  tonal range
- on the contrary, selecting *100* (sliding right) will modify the tones
  that *don't* match the selected range.

In the intermediate positions between *0* and *±100* the changes are
made progressively towards the chosen range, or towards the rest of the
tones.

This control is very useful to prevent over-saturation of human skin
tones, which results in an immediately noticeable «carrot look».

### Residual image curve

This is a Flat Curve with colored vertical lines and sliding dots and is
independent of the *sky hue range* and the Chroma Intensity control: you
can use it to modify the tones so that they take on a dominant color,
independently of the rest of the controls of the module.

It works as follows: if you move a dot upwards, the areas of the image
with the color of that particular line will take on the hue of the line
immediately to the right. If you move it down, the dominant will be the
color of the line immediately to the left. For example: by moving the
point of the yellow line up, the yellow areas of the residual image will
take on a greenish hue (the line to the right of the yellow), whereas if
you move the point down, the dominant will be orange (the line to the
left of the yellow).

### Toning and Color Balance

Activating this option presents 3 pairs of tone controls for
*Highlights*, *Midtones* and *Shadows*, respectively, which can be used
to color tone the residual image.

The controls modify the *a\** and *b\** components of the Lab color
space, so the changes will be:

- for component *a\** = from green to magenta
- for component *b\** = from blue to yellow

The results will depend on the intensity of the changes:

- with high values you can create special effects, similar to those
  achieved with the *Chroma Module*, but focused on the residual image.
  You can use it in combination with that module if you wish
- with moderate values you can manually correct the white balance: for
  example, imagine a scene where the main details are in a shaded area
  which has a blue color cast and the background is in full daylight and
  has a different color temperature. In this case you can adjust the
  white balance for the details (and remove the blue cast) and then
  readjust the background (the residual image) and customise the white
  balance for each area (highlights, mid-tones and shadows)

Note that the residual image contrast compression controls, which modify
the luminance values of each area, will have a direct influence on the
results achieved with this part of the module.

## Final Touchup module

In this module you can apply small touch-up adjustments to the image.
However because it is located at the end of the Wavelet levels tool, any
significant modifications carried out here may mean that you have to
readjust the rest of the modules.

### Directional Contrast

In general, the initial balance between the 3 directional components of
image decomposition is respected throughout the tool: vertical,
horizontal and diagonal. However, with this module you can increase the
weight of one over the other, to achieve a different result.

#### Contrast Balance Method

This *balance* control changes the balance between diagonal
decomposition on the one hand and vertical and horizontal decomposition
on the other. Its principle is similar to that of the [edge-preserving decomposition](http://www.cs.huji.ac.il/~danix/epd) which is based on
the [Cholesky factorization](https://en.wikipedia.org/wiki/Cholesky_decomposition) and
linearly modifies the luminance values of the image.

This module allows you to modify the effects of the following modules:
Contrast, Chroma and Residual Image Tone Mapping.

You have two choices:

- Slider: with this option, the ***Contrast balance d/v-h*** control
  appears allowing you to modify the contrast values in the image.
  Moving it to the right intensifies the overall image contrast, whereas
  moving it to the left reduces the contrast. Bear in mind though that
  you are acting on the balance between the different directions of
  decomposition, so with extreme values you'll introduce important
  artifacts.
- Curve: In this case the curve ***Contrast balance d/v-h balance
  curve*** appears, which acts on the balance of the luminance values of
  the image.

In both cases, there is an additional control: **Delta balance levels**.
When it is set to zero (default), all levels of the decomposition are
processed in the same way. If it is placed on the left, the lower levels
are emphasized (the fine details) and the upper levels are reduced
(those that give volume to the image). On the other hand, if it is
placed to the right, the lower levels are reduced and the upper levels
are increased.

You also have at your disposal an ***Attenuation Response*** slider,
which will act as described in the [chapter on attenuation of the Contrast module](#attenuation_and_selectivity_in_contrast_changes).

Finally the option ***Chrominance balance*** allows you to modify the
d/v-h balance of the chromatic components of the decomposition using the
same controls as above (slider or curve).

### Final Local Contrast

This curve is located at the end of the processing pipeline, just before
recomposition and acts non-linearly on the contrast of the decomposition
levels.

Please note that this curve, which acts on the initial local contrast
and not on the luminance, does not duplicate the previous one (*Contrast
balance*).

On the graph, the center of the abscissa corresponds to the mean value
of the local contrast and a third of the way on each side there is a
point corresponding to the mean plus one standard deviation of the local
contrast values. By changing the shape of the curve you will reduce or
increase the effects of the modulus of *Contrast*, the *Edge sharpness*,
the *Balance method* and even the very principle of decomposition -
recomposition.

By default the curve is *flat* (i.e. it has no effect), although you can
modify it to your liking to e.g. further reduce the value of low local
contrast and thus soften the visibility of the noise, or reduce the
values of high local contrasts and avoid artifacts.

In addition, there is also a ***Attenuation Response*** slider, which
will act in the same way explained in the
[chapter on the attenuation of the Contrast module](#attenuation_and_selectivity_in_contrast_changes).

### *After* Contrast Curve

This curve is not related to the previous curves. It is at the end of
the wavelet levels processing pipeline, after the recomposition of the
levels plus the residual image, and allows you to modify the global
contrast of the image.

It acts on luminance and its use is similar to the other tonal curves
found in RawTherapee, although in this case you will not see a
background histogram.

Finally, you also have a ***Soft Radius*** slider that allows you to
apply a blur to selected areas so that they blend in better with the
image.

## Final comparison



<div>


  ![](wavelets_original_big.png "wavelets_original_big.png")
  <figcaption>wavelets_original_big.png</figcaption>


  ![](wavelets_final_big.jpg "wavelets_final_big.jpg")
  <figcaption>wavelets_final_big.jpg</figcaption>


</div>
