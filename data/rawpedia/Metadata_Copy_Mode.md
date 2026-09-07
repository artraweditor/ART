---
title: Metadata Copy Mode
contributors:
  - DrSlony
  - XavAL
tags:
  - 'Tool Description'
  - 'Metadata Tab'
---

RawTherapee let's you decide which of three things should happen to
metadata (Exif and IPTC) when saving an image:

- Copy Unchanged - The saved image's metadata will resemble the input
  image's metadata as closely as possible. None of the metadata changes
  you make will have any effect.
- Apply Modifications - The changes you make will be included in the
  saved image.
- Strip All Metadata - All metadata will be removed from the saved
  image. You might want to use this for privacy reasons, e.g. to remove
  GPS info.

Note: prior to RawTherapee 5.4, this option was found in Preferences as
a "Copy Exif/IPTC/XMP unchanged to output file" checkbox and was global,
i.e. it was not specific to any image or processing profile.
