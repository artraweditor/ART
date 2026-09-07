---
title: Exif Tab
contributors:
  - DrSlony
  - XavAL
tags:
  - 'Tool Description'
  - 'Metadata Tab'
toc: true
---

<div class="pagetitle">

EXIF Tab

</div>

The "Metadata" tab lets you control which
[Exif](https://en.wikipedia.org/wiki/Exif) metadata will be contained in
the saved (developed) image file. The Exif metadata is usually created
by the camera itself and implemented into the raw image file. Basic Exif
information is directly visible. Extended Exif information and so-called
[makernote data](https://en.wikipedia.org/wiki/Exchangeable_image_file_format#MakerNote_data)
is organized into a tree. Click on the arrow at the very left of the
desired sub-tree and you'll see its contents.

You can "Remove", "Keep", or "Add/Edit" Exif metadata. Manipulating
metadata does not change the source file in any way! If you want to
restore a value you have changed or removed by accident, simply press
"Reset". "Reset All" works similarly but is used for trees and works
recursively, which means that all values changed/removed in this
sub-tree are restored.

You can "Add/Edit" the following Exif information:

- Artist
- Copyright
- ImageDescription
- Exif.UserComment

Only the English names of the Exif fields are displayed for easy
reference. They are not translated when you choose a different GUI
language.

With the Exif tab focused, you can press  + to search for an entry,
though only entries with a check-mark can be searched.

## Technical Details

Reading `Exif.UserComment`:

- ASCII values are read without problems.
- For Unicode values, RT detects if the string has an odd length.
  - If so, it's assumed to be a UTF-8 string.
  - If not, it tries to detect if a UCS-2 byte order mark (BOM) is set.
  - If no BOM is provided, and the string is a valid UTF-8 string, it
    tries to auto-detect the byte order based on the "zeros" count.
  - If it is not a UTF-8 string, it is assumed to be a UCS-2 string with
    the platform's byte order.
- For undefined values, RT tries to convert the string from the local
  charset to UTF-8.

Saving `Exif.UserComment`:

- If the string is pure ASCII, it is saved as ASCII.
- Otherwise the string is converted to UTF-16 and saved without a byte
  order mark, using the metadata's byte order.
