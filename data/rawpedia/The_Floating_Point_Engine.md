---
title: The Floating Point Engine
contributors:
  - Lebarhon
  - DrSlony
tags:
  - 'General Information'
toc: true
---

RawTherapee performs all calculations in 32-bit [floating point](https://en.wikipedia.org/wiki/Floating_point) precision (in
contrast to 16-bit integer as used in many other converters such as
[dcraw](https://en.wikipedia.org/wiki/Dcraw) and also in RawTherapee up
to version 3.0).

Classical converters work with 16-bit integer numbers. A pixel channel
has values ranging from 0-65535 in 16-bit precision (to increase
precision, converters usually multiply the 12- or 14-bit camera values
to fill the 16-bit range). The numbers have no fractions, so for example
there is no value between 102 and 103. In contrast, floating point
numbers store a value at a far wider range with a precision of 6-7
significant digits. This helps especially in the highlights, where
higher ranges can be recovered. It allows intermediate results in the
processing chain to over- or undershoot temporarily without losing
information. The fraction values possible also help to smooth color
transitions to prevent color banding.

The downside is the amount of RAM that floating point numbers require,
which is exactly twice that of 16-bit integer. Together with the
ever-increasing megapixel count of digital cameras, a 32-bit operating
system can quite easily run out of memory and cause RawTherapee to
crash. Therefore a 64-bit operating system is highly recommended for
stability.

We officially ended support for 32-bit versions of RawTherapee with
release 5.0-r1 in February 2017. Do not file bug reports regarding
issues on 32-bit systems.

If you nevertheless need to use RawTherapee on a 32-bit system, the
following will help make the most of it:

- Use 4-Gigabyte Tuning in Windows. See "[4-Gigabyte Tuning: BCDEdit and Boot.ini](http://msdn.microsoft.com/en-us/library/bb613473%28VS.85%29.aspx)"
  for an explanation of what it is, and find out how to do it by reading
  the guide "[How to set the /3GB Startup Switch in Windows XP and Vista](http://avatechsupport.blogspot.se/2008/03/how-to-set-3gb-startup-switch-in.html)".
- Close other programs while working in RawTherapee.
- Use a [single Editor tab](preferences#layout).
- Turn off "auto-start" in the [Queue](queue). Add photos to
  the Queue as usual. When ready to start processing them, restart
  RawTherapee to free up RAM (no image open in the Editor), and start
  the queue.
- Ensure that RawTherapee [does not load](preferences#directories) dark-frame or flat-field
  images if you do not use them.
- Avoid having more than a few hundred photos per folder, as each photo
  requires a little RAM (thumbnail, embedded ICC profile, etc.).

## Memory Requirements

To open an image in the Editor, RawTherapee 5.6 needs very roughly this
much RAM, in bytes:

- Non-raw
  - 8-bit:
    `(width * height * 3) + (width * height * 4) + (previewWidth * previewHeight * 28)`
  - 16-bit:
    `(width * height * 3 * 2) + (width * height * 4) + (previewWidth * previewHeight * 28)`
  - 32-bit:
    `(width * height * 3 * 4) + (width * height * 4) + (previewWidth * previewHeight * 28)`
- Raw
  - `(width * height * 4) + (width * height * 4) + (width * height * 12) + (previewWidth * previewHeight * 28)`

Some overhead memory is additionally required, for example for
generating thumbnails of other images which reside in the opened image's
folder.

The memory requirement for processing and saving an image depends on
what tools you use and can vary significantly from the above - the above
pertains only to opening an image.
