---
title: Tone Mapping
contributors:
  - DrSlony
tags:
  - 'Tool Description'
  - 'Exposure Tab'
toc: true
---

The effects of this tool are visible at any zoom level. However, due to
the nature of the algorithm, only the 1:1 (or more) preview zoom will
match the saved image perfectly. If you are zoomed out at less than 1:1,
you should be aware that the preview can match the saved image very well
or not so well, depending on the "*Edge Stopping*" and "*Scale*"
sliders. Read the [Getting the preview to match the saved image](/tone_mapping/#getting-the-preview-to-match-the-saved-image)
section below. Use a detail window (click on the
![window-add.png>](window-add.png "window-add.png") icon
under the [main preview panel](the_image_editor_tab#the_preview_panel)) to inspect a
part of the image, or zoom the main preview to 100% (also called 1:1)
![magnifier-1to1.png>](magnifier-1to1.png "magnifier-1to1.png").

------------------------------------------------------------------------

![](rt407-ba-tonemapping-hdr-cropped.jpg "rt407-ba-tonemapping-hdr-cropped.jpg")
The Tone Mapping tool can be used to lift the dark areas of your photo
in a way that prevents halos from appearing, and it can be used to bring
out or suppress detail, to make the photo more crisp or more 'dreamy'.
Tone mapping adjusts the global contrast of an image differently from
the local contrast. Specifically, it's very useful for decreasing large
scale contrasts while preserving (or boosting) small scale contrasts.
The method used is taken from *Edge-Preserving Decompositions for
Multi-Scale Tone and Detail Manipulation* with some modifications.

Note: tone mapping requires a lot of memory (RAM) and is CPU-intensive.

## Getting the preview to match the saved image

![](rt_tm_preview.jpg)
Edge Stopping=1.40 and Scale=0.10 while
editing, but Scale is set to 1.00 just before saving. 
![](rt_tm_saved.jpg)
Using this trick, the preview matches the saved image very
well.

The effects of this tool highly depend on the size of the input image in
the engine's toolchain. To keep the preview fast, RawTherapee feeds each
tool with the image you see in the preview at the same resolution as the
preview, not the original, huge one (however, when you save an image, it
uses the original, huge one, which is why saving takes longer).
Processing a 900x600 image is much faster than processing a 7360x4912
one, for instance. The side-effects of this are that the zoomed-out
preview may not match the saved image, depending on the "*Edge
Stopping*" and "*Scale*" sliders.

The default values for this tool are Edge Stopping=0.5 and Scale=0.10.
These values generally lead to good results and a preview which quite
closely matches the saved image. If you wish to adjust these values, you
will need to process the full image every time you change them to make
sure that their effects on the saved output are what you had in mind. An
easy way to simplify this task is by setting an image viewer as the
"[External editor](preferences#external_editor)", then you
just hit the keyboard shortcut Ctrl+e to have RawTherapee fully process
the image and automatically view it in your image viewer.

Remember that if you zoom the preview to 100%, it will perfectly match
the saved image regardless of what slider values you use.

## Interface Description

### Strength

This controls the strength of the overall effect. As of version 4.2.156,
as you increase the strength, shadows are raised and highlights are
slightly lowered in a way which aims to preserve the average luminance
of the image thereby preventing the washed-out look.

### Gamma

Gamma moves the action of tone-mapping to shadows or highlights.

### Edge Stopping

This parameter affects sensitivity to edges. The greater it is the more
likely an illumination change is to be considered an "edge". If set to
zero tone mapping will have an effect similar to [unsharp masking](https://en.wikipedia.org/wiki/Unsharp_masking).

### Scale

This control gives meaning to the difference between "local" and
"global" contrast; the greater it is the larger a detail needs to be in
order to be boosted. See the note above on the influence this setting
has on the preview vs the saved image.

### Reweighting Iterates

In some cases tone mapping may result in a cartoonish appearance, and in
some rare cases soft but wide halos may appear. Increasing the number of
reweighting iterates will help fight some of these problems. When more
than zero reweighting iterates are used, the best results will be had if
the edge stopping parameter is set to one (technical detail: this
results in a 1-norm approximation of the smoothness using iteratively
reweighted least squares). Artifacts in high contrast edges may begin to
appear when this value is set to about 1.8 or higher.
