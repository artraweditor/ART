---
title: Edges and Microcontrast
contributors:
  - DrSlony
  - XavAL
  - Jdc
tags:
  - 'Tool Description'
  - 'Detail Tab'
toc: true
---

__TOC__

## General

Unlike *[Unsharp Mask](sharpening#unsharp_mask)*, *Edges* is
a true sharpening algorithm. It does not introduce halos, it can be used
on noisy images and it works in the Lab color space. It emphasizes only
the edges, and can be combined with
[Microcontrast](edges_and_microcontrast#microcontrast) to
also enhance the texture.

Both algorithms were originally implemented by [Manuel Llorens](https://github.com/ManuelLlorens).

## Edges

This tool sharpens any edges that have sufficient contrast for them to
be considered an edge. In other words, it sharpens edges that are
already sharp, ignoring edges that do not have enough contrast. The
algorithm is not affected by image noise and does not generate halos.

This type of sharpening can make the edges look a bit unnatural, as if
they had been "cut out". Also, if the settings are too high, the
resulting edges may exhibit
[Aliasing](https://en.wikipedia.org/wiki/Aliasing). This is why you
should be careful when applying it to images with curved edges. However,
when straight lines predominate (especially if they are not diagonal),
it is a useful method of sharpening, especially if you reduce the size
of the image at the end of processing.

To get the best results, the following settings are recommended:

1.  Iterations: number of iterations carried out by the algorithm. A
    high number produces an overly sharp effect around the edges. With a
    value set to 2 this problem can already be observed in some cases.
    In general, 1 or 2 iterations give the best results.
2.  Quantity: number of adjacent pixels to be analyzed when deciding
    what constitutes an edge. Higher values produce sharper edges and a
    greater "saw-tooth" effect.
3.  Luminance only: the tool works in the L\*a\*b\* color space and with
    this option, only the L\* component is enhanced.


![](edges.jpg "edges.jpg")
<figcaption>edges.jpg</figcaption>




## Microcontrast

"Microcontrast" can be defined as contrast on a pixel
level[1](https://web.archive.org/web/20110625093654/http://www.rawness.es/sharpening/?lang=en#comment-306),
as opposed to "local contrast" which pertains to contrast between larger
(lower frequency) areas.

The Microcontrast tool increases the contrast of a pixel relative to its
neighbors, effectively leading to an apparent increase in texture. The
intention is to allow recovering texture lost due to noise reduction. It
does not introduce
halos.[2](https://web.archive.org/web/20100324142513/http://www.rawness.es/contraste-local-y-microcontraste/?lang=en)


![](seagull-microcontrast.jpg "seagull-microcontrast.jpg")
<figcaption>seagull-microcontrast.jpg</figcaption>
</figure>

The tool's controls are progressive and allow you to choose a balance
between increasing the contrast at the pixel level and the appearance of
artifacts:

- Contrast threshold: sets the minimum contrast at which the tool will
  act on the pixels.
- Quantity: the intensity of the effect. The higher the value, the
  greater the difference between the pixels.
- Uniformity: to the left, the algorithm tends to respect the initial
  contrast gradients. To the right, the contrasts are more intense and
  the initial contrast gradients are ignored, which makes the image
  harsher.
- Matrix: defines the area that will be used to calculate the contrast
  variation. There are two possibilities, a 3x3 pixel matrix around the
  pixel being analyzed, or a 5x5 pixel matrix. By default, it will be
  5x5, giving a more intense effect, the 3x3 matrix will be more
  appropriate for noisy images.
