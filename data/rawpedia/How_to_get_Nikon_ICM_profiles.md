---
title: How to get Nikon ICM profiles
contributors:
  - DrSlony
  - XavAL
tags:
  - 'General Photography'
---

Capture NX-D, Capture NX 2 and ViewNX-i are proprietary Windows programs
provided for Nikon cameras and are able to accurately develop raw images
to match the in-camera rendering. Behind the scenes, these programs
generate ICC profiles which incorporate recipes for tonal changes
according to their adjustments. Such profiles can be used as input ICC
profiles in RawTherapee to lead to almost identical rendering, following
the same recipe. The Nikon input ICC profiles are proprietary and cannot
be distributed. Fortunately, this software extracts and generates input
profiles on-the-fly in a temporary folder at run-time. It is up to you
to abide by Nikon's licensing agreement. This process works on
registered and trial versions of these programs.

1.  First you have to install any of these programs.
2.  Open the NEF file in it, and look inside the hidden folder:

    `%APPDATA%\Local\Temp\Nkn`<random strings>`.tmp`

    The temporary ICC profiles will be created in this folder, with
    filenames similar to this one:

    `Nkx_D90_962_1_03_0_00_10_00_00_00_05_00_0200_0_0_476.icm`

    Under Windows XP the path differs:

    `%TEMP%\Nkn`<random string>`.tmp`
3.  Under "Picture Control" click on the "Launch Utility" button. While
    the utility is running the profiles will temporarily appear in the
    folders listed above. They appear there because Nikon applies
    generic profiles to your pictures. These are named "picture control"
    settings. For example, Capture NX 2 has "neutral", "standard" and
    "monochrome" profiles. For use with RawTherapee you must simply know
    that Nikon generates different ICM profiles with every different
    setting. To ease the choices, the "neutral", "standard", "vivid" and
    "landscape" settings seem to be similar, apart from the increased
    saturation and contrast. You can tune the tone curves in
    RawTherapee, so, in general, a single "neutral" profile should be
    good starting point.

**Note**: Other settings may also generate new ICM files! You must be
particularly aware that pictures shot under artificial lighting will
have different profiles, and using such profiles with daylight shots
will result in color casts. D-lighting dramatically changes contrast. If
in doubt, just open the particular image you want the color profile for
in one of these Nikon programs and use that profile in RawTherapee. In
most cases, a generic color profile will be sufficient.
