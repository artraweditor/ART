---
title: Impulse Noise Reduction
contributors:
  - DrSlony
  - Andrea.romagnoli
  - XavAL
tags:
  - 'Tool Description'
  - 'Detail Tab'
toc: true
---

Suppresses salt-and-pepper noise - sudden white and black pixels, which
remind one of salt and pepper sprinkled over a photo. This is done after
demosaicing.

Whereas salt-and-pepper noise is typically just white or black, hot
pixels can be of a pure, saturated color, while dead pixels are black.
Hot and dead pixels occur for a very different reason than
salt-and-pepper noise and should be handled using the
[Hot/Dead Pixel Filter](preprocessing#hot.2fdead_pixel_filter), which works
before demosaicing.

The slider adjusts the threshold which must be exceeded for the
suppression to be applied.
