---
title: Dynamic Range Compression
contributors:
  - Thanatomanic
  - DrSlony
tags:
  - 'Tool Description'
  - 'Exposure Tab'
toc: true
---

## Introduction

Dynamic range is the ratio of the largest to the smallest value of a
measured signal. In photography it commonly refers to the ratio of the
brightest element of a scene to the darkest. An outdoor scene on a very
foggy day commonly has very little difference between the brightest and
darkest elements, which is known as a *low dynamic range* scene. In
contrast, an indoor scene with a visible sunny sky through a window is
known as a *high dynamic range* scene.

The dynamic range of a scene can easily exceed the dynamic range of the
"sensor" that captures the scene. The [human visual system](https://en.wikipedia.org/wiki/Dynamic_range#Human_perception)
has an adaptive and wide dynamic range (you can see faint stars at night
but also bright skies during day). This is very different from the
fixed, lower dynamic range of your camera sensor and the (usually even
lower) dynamic range of your monitor. As such, photography and image
processing needs to deal with mapping high dynamic ranges to lower ones.

In general there are two ways to handle dynamic range changes: either
discard a portion of the data outside the destination range (e.g.
clipping highlights), or compress the data so that it fits the
destination range. The Dynamic Range Compression tool uses the latter
approach based on the Gradient Domain High Dynamic Range Compression algorithm developed
by R. Fattal and coworkers. This algorithm is often simply referred to
as "Fattal", e.g. in Luminance HDR.

The algorithm uses two parameters to control the compression (α and β)
which can be tuned by the "detail" and "amount" sliders of the tool,
respectively. The tool operates in RGB space and is applied right after
[Noise Reduction](noise_reduction) and [Haze Removal](haze_removal), but before other tone curve
adjustments such as the [Exposure](exposure) controls.

N.B. There are alternative ways of compressing the dynamic range using
other tools. The simplest would be a negative contrast value in the
[Exposure](exposure) tool to reduce (or rather to
redistribute) the dynamic range, however the effect would most likely
appear flat and unappealing. A curve gives more control over the
process, but may need a lot of fine-tuning.

## Usage

Use this tool when the dynamic range of the photographed scene is too
high to be reproduced on your monitor in an aesthetically pleasing way,
that is when you find that the difference between the dark tones and the
bright tones (the contrast) is so strong that there is a lack of detail
in those areas.

Heads-up panorama stitchers! The effects of this tool depend on the
dynamic range (and histogram) of the image being edited. If you are
processing a series of images intended for stitching, where each image
contains a section of a scene adjacent to the one before it, even if you
were to apply identical parameters to these images using this tool, the
end results would not be consistent - there would be sudden changes in
brightness between adjacent images. Do not use this Dynamic Range
Compression tool on the source images. If you need to compress the
dynamic range across a series of images in a consistent way, use a curve
instead. You can, however, use this tool on the stitched panorama.


![](drc-example.jpg "drc-example.jpg")
<figcaption>DRC-Example.jpg</figcaption>


## Interface

### Amount

Sets the strength of the compression. Higher values lead to a narrower
dynamic range (you can easily see the effect by observing the
histogram).

### Detail

Sets how much local contrast is preserved. Positive values reduce the
compression in favor of more contrast, negative values reduce the
contrast.

### Anchor

Biases the compression towards the shadows or highlights, effectively
functioning as an exposure compensation.
