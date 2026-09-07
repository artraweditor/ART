---
title: Crop
contributors:
  - DrSlony
  - XavAL
  - digitalcarp
tags:
  - 'Tool Description'
  - 'Transform Tab'
toc: true
---

Cropping in RawTherapee does not discard the cropped-off area, it just
hides it by drawing a crop frame and zooming in to fit the frame to the
available screen space. You can still see the cropped-off area in the
preview if you zoom out and set the [background color of the preview](the_image_editor_tab#background_color_of_the_preview)
to "theme-based". The cropped-off area will of course not appear in the
saved image.

## Editor

When you set the [background color of the preview](the_image_editor_tab#background_color_of_the_preview)
to anything other than "theme-based" then the cropped-off area will be
completely hidden from the preview, but if you set it to "theme-based"
then by default the cropped-off area will be covered by a dark but
semi-transparent mask. You can define the color and transparency of this
mask in "[Preferences](preferences) \> General \> [Crop mask color/transparency](preferences#default_theme)".

Activate crop-placing mode by clicking the "Select Crop" button in the
tool panel, the ![<File:crop.png>](/images/crop.png "File:crop.png") button in
the [Editor's](the_image_editor_tab) top toolbar, or the
appropriate [keyboard shortcut](keyboard_shortcuts), then
create the crop by clicking and dragging over the preview with your
mouse. Use the **Shift** key to move (pan) the crop over the image.
Resize a crop by placing the mouse on one of the sides or corners. To
clear the crop, activate crop-placing mode again (via the keyboard
shortcut or either of the buttons mentioned above), and click anywhere
in the preview without dragging. To see only the cropped area, use the
"Fit cropped area to screen" [keyboard shortcut](keyboard_shortcuts).

Use *Guide Type* to select popular guides to help you in composition
while cropping, and a horizontal (landscape) or vertical (portrait)
orientation. By default, RawTherapee automatically detects and uses the same crop orientation as the
orientation of your image - the "As Image" option.

The PPI value (pixels per inch) does not change any physical property of
the image, it only serves to help you see what physical size the current
crop would print at using that PPI value.

## Aspect Ratios

### Custom crop ratio

You can make a custom crop ratio as of RawTherapee 5.1.

1.  Disable "Lock ratio",
2.  Make a crop, use "Width" and "Height" values to define the ratio,
    e.g. to make a crop using a 5:4 ratio set width=5 height=4, or
    width=1280 height=1024.
3.  To resize the crop while maintaining the custom ratio, hold the
    Shift key while dragging the edge or corner of the crop frame with
    the mouse.

### Fixed crop ratio

Use "Lock ratio" to set the crop to a fixed ratio.

- 3:2
  - Classic negatives have this ratio, as do [APS-C](https://en.wikipedia.org/wiki/APS-C)
    [DSLR](https://en.wikipedia.org/wiki/Digital_single-lens_reflex_camera)
    cameras.
- 4:3
  - The [Four Thirds System](https://en.wikipedia.org/wiki/Four_Thirds_system).
- 16:9
  - The [1080p](https://en.wikipedia.org/wiki/1080p) and [720p](https://en.wikipedia.org/wiki/720p)
    [high-definition](https://en.wikipedia.org/wiki/High-definition_video)
    video format, and due to this the most common computer monitor aspect ratio
    since 2010.
- 16:10
  - The most popular computer monitor aspect ratio between 2005-2009. Still
    popular in tablets.
- 24:65 XPan
  - Hasselblad's medium-format cameras.
- [1.414 DIN EN ISO 216](https://en.wikipedia.org/wiki/ISO_216)
  - Standard paper size ratio such as A4, B5, etc.
- 8.5:11
  - The [US Letter](https://en.wikipedia.org/wiki/Letter_(paper_size)) size.
- 11:17 - Tabloid
  - A common [tabloid newspaper format](https://en.wikipedia.org/wiki/Tabloid_(newspaper_format)).
- 45:35 - ePassport
  - Guides to help you crop a portrait for a [biometric passport](https://en.wikipedia.org/wiki/Biometric_passport).
    Official measurements do not specify exact ratios, just min/max
    measurements within which the eyes and chin-crown distance must lie. The
    guides represent the averages of those distances. The first horizontal
    guide is for the crown, the second is roughly for the nostrils, the third
    is for the chin. "On the photo, the face must be between 29mm and 34mm from
    the bottom of the chin to the crown (the top of the head, not the top of
    the hair)." \[[1](http://www.homeoffice.gov.uk/agencies-public-bodies/ips/passports/information-photographers/)\].

## Crop Guides

Crop guides provide overlays for commonly used composition patterns like the
[Rule of Thirds](https://en.wikipedia.org/wiki/Rule_of_thirds). The overlay
is only active when the Crop tool is enabled. While the crop tool is enabled,
the overlay can be shown/hidden by enabling/disabling the crop guide tool.

Enabling the crop guide tool will draw a marquee frame around the crop by
default. This behavior can be changed in "[**Preferences**](preferences) \>
**Image Processing** \> **Guides shown when not editing the crop**".

Composition guides can be enabled individually. Multiple crop guides can be
enabled at the same time by using **Ctrl + Click**. Use the color picker button
to set the color of the guide type.

Use the **Rotate**, **Mirror**, and **Reset** buttons to control the
orientation of the golden triangles golden ratio composition guides.

### Inscribed Aspect Ratio Guides

If an image needs to be cropped for multiple aspect ratios, inscribed aspect
ratio guides can be used. Select the desired aspect ratio from the dropdown.
A guide for the selected aspect ratio is drawn to fit the current crop. Use the
**Rotate** button to control the orientation of the aspect ratio.

### Bleed

The print bleed option adds padding to the crop guides inside the user's
selected crop. In **Scale** mode, the padding is a percentage value of the crop
dimensions. In **Width**, **Height**, **Long Edge**, and **Short Edge** mode, a
uniform padding value is used based on the length of the chosen crop edge.
