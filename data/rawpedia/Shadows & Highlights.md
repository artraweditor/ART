---
title: Shadows Highlights
contributors:
  - DrSlony
tags:
  - 'Tool Description'
  - 'Exposure Tab'
toc: true
---

## Introduction

Use this tool to brighten the shadows or darken the highlights of an
image.

This tool has received a new engine in RawTherapee 5.5, it now uses an
edge-aware [fast guided filter](https://arxiv.org/abs/1505.00996) to
prevent halos and operates by default in RGB space to preserve
saturation.

## Usage

Use it in moderation to preserve a natural look. If the photographed
scene has a high dynamic range (very deep shadows and very bright
highlights) then use the [Dynamic Range Compression](dynamic_range_compression) tool to compress the
dynamic range to a more manageable level, and then optionally use this
Shadows/Highlights tool on top of that.

## Interface

### Color Space

Adjusting shadows and highlights in the RGB space preserves image
saturation which usually looks more natural than working in L\*a\*b\*
space which tends to desaturate affected areas. However, in some cases
working in RGB space may oversaturate the shadows, in which case you
should switch to L\*a\*b\* space.

### Shadows

Allows you to brighten the darkest parts of the image

### Highlights

Allows you to darken the brightest parts of the image.

### Tonal Width

Shadows/Highlights Tonal Width allows you to control how bright an area
must be for it to be affected by the highlights slider, and how dark an
area must be for it to be affected by the shadows slider. Though the
underlying math is a bit more complicated, think of a histogram - the
highlights tonal width specifies the range of tones from the white end
of the histogram which the highlights slider will affect, and the
shadows tonal width specifies the range of tones from the black end of
the histogram which the shadows slider will affect. The higher the tonal
width value, the more tones are affected.

### Radius

The value of the Radius slider influences the effective area of the
Shadows and Highlights sliders.
