---
title: How to create DCP color profiles
contributors:
  - DrSlony
  - XavAL
tags:
  - 'General Photography'
toc: true
---

## What Are DCP Profiles and Why Do I Need Them?

Technically, each photosite in a digital photography camera's [image sensor](http://en.wikipedia.org/wiki/Image_sensor) outputs a certain
current based on the number of
[photons](http://en.wikipedia.org/wiki/Photon) of light that hit that
photosite. The current is converted into a number. These numbers, along
with some [metadata](http://en.wikipedia.org/wiki/Metadata), are stored
in what is known as a "[raw file](http://en.wikipedia.org/wiki/Raw_image_format)". At this point
there is no concept of color and the raw data looks nothing like an
image. As in traditional photography, the image must be "developed" into
a usable form. One of the steps of this development involves translating
the numbers into accurate colors, and for that you need to profile the
camera, to map the numbers to specific known colors.

Practically, you must use an input color profile in order to get
accurate colors, and currently the best way to go about this is using a
"DNG camera profile" (DCP for short - do not confuse with the entirely
unrelated [Digital Cinema Package](http://en.wikipedia.org/wiki/Digital_Cinema_Package)). The
input color profile is what makes a camera's colors look they way they
do when you open a photo, before you make any tweaks.

RawTherapee ships with the following profile types:

- ICC - These are matrix profiles, tailored only to daylight, not so
  good in incandescent light. This is the old kind of profile.
- DCP - These are the new types of profiles.
  - DCP single-illuminant - These are tailored only to daylight.
  - DCP dual-illuminant - These are tailored both to daylight and to
    incandescent light, so they perform great in most lighting
    situations.

You can create your own DCP profiles tailored to specific lighting
situations for ultimate color accuracy. Let's say you are shooting a
wedding, or panoramas for a virtual tour. You need accurate and
consistent colors. The [color spectrum](http://en.wikipedia.org/wiki/Color_spectrum) of the light
outside the building is completely different to that inside, and even
indoors the light will differ between the rooms as general consumer
light bulbs produce light of different temperatures and spectrums, and
there are usually several different types of light bulbs illuminating a
single room. If you come prepared with a color calibration chart, such
as the X-Rite ColorChecker Passport, all you need to do is to take a
photo of this chart in the same location(s) as your normal photos, one
shot per lighting situation (which usually translates to one shot per
room and outdoors), then generate DCP profiles from these shots and use
them in RawTherapee to achieve correct and consistent colors and [white balance](http://en.wikipedia.org/wiki/White_balance) between all your
shots.

## Color Targets

The best target for most shooting situations is one which is non-glossy,
has good reference measurements, and is portable and rugged. For that
reason we recommend the X-Rite ColorChecker Passport Photo.

Don't be mislead into thinking that the more patches a target has, the
better - that is not the case, and having more patches makes tweaking
and producing a high quality DCP more difficult.

Supported color targets:

- Matte
  - X-Rite ColorChecker Passport
  - X-Rite ColorChecker 24, also known as the X-Rite ColorChecker
    Classic
  - Datacolor SpyderCHECKR
  - X-Rite ColorChecker SG
  - QP-Card 203
  - QP-Card 202
- Glossy (difficult to use and therefore not recommended):
  - IT 8.7 Reflective DIN A4 (Wolf Faust Coloraid C1)
  - CMP Digital Target 8
  - HutchColor HCT (Fuji RVP)

For more information, refer to the "Choosing test target" chapter of the
DCamProf documentation:
<https://torger.se/anders/dcamprof.html#test_target>

## More Info

Find out more:

- DCamProf manual <https://torger.se/anders/dcamprof.html>
- X-Rite ColorChecker Passport product page:
  <http://xritephoto.com/ph_product_overview.aspx?id=1257>
- X-Rite ColorChecker Passport PDF manual:
  <https://www.xrite.com/-/media/xrite/files/manuals_and_userguides/c/o/colorcheckerpassport_user_manual_en.pdf>
- QPcard article on ICC and DCP color profiles:
  <http://www.qpcard.com/dcp_icc_profile>

## Shooting the Color Target

![](pentax_k10d_daylight_london_summer.jpg "pentax_k10d_daylight_london_summer.jpg")
![](colortarget_glare.png "colortarget_glare.png")
![](gluehlampe_01_KMJ.png "gluehlampe_01_KMJ.png")

This guide assumes you use the X-Rite ColorChecker Passport, though you
can use any supported color target. If using the X-Rite ColorChecker
Passport, while it's enough if you shoot just the 24-patch part we do
prefer shots of the whole target.

The light shining on the color target (the "illuminant") must meet
certain requirements ("[standard illuminants](https://en.wikipedia.org/wiki/Standard_illuminant)"), and
you will be shooting in two types of light:

- Light from a non-tinted incandescent tungsten light bulb, known as
  Standard Illuminant A, or **StdA** for short.
- Daylight, known as Standard Illuminant D, and you should strive for a
  light between D50 (known as "horizon light") and D65 (known as "noon
  daylight").

There are two scenarios when you will want to shoot a color target:

1.  To provide RawTherapee with an accurate general-purpose
    dual-illuminant (StdA and D50/D65) DCP profile for use by everyone
    with your camera model,
2.  To handle specific lighting situations for yourself only.

In the former case, you will want to take extra care to make sure your
profile sticks to the requirements because many people will use it. In
the latter case, you will probably only use that profile on that
specific occasion, and it will be of no use to other people.

Shooting the color target for inclusion in RawTherapee:

- We need photos taken in both incandescent light and daylight.
- Ideally, wait for a clear, sunny day when the sun is high above the
  horizon, near the zenith at midday. You can also photograph the target
  on a lightly overcast day around noon, but not on a stormy day, in
  winter, or even in summer in the early morning or evening. You don't
  want stray colors in the spectrum which appear when the sun is near
  the horizon or behind heavy clouds or smog.
- For the daylight shot, find a spot far away from anything that could
  reflect light. Your balcony is not a good spot - the walls will
  reflect light and negatively influence the light spectrum, even if you
  can't see that with your bare eyes. The park is not a good spot.
  Standing under a transparent roof or sun umbrella is not a good spot.
  A large, empty-ish parking, far away from walls and trees and parked
  cars, is a great place. Anything other than clear, direct sunlight is
  bad. Forget about using a flash. Nothing matches real sunlight.
- For the incandescent shot, you need a real tungsten light bulb, the
  kind that were used everywhere until the world moved to compact
  fluorescent lamps in the last decade. Don't use fluorescent lights -
  even if they have a warm color, the light spectrum is absolutely
  different to tungsten. Don't use halogen lights. Don't use tinted
  bulbs.
- Position your color target and yourself relative to the source of the
  light (the sun or the light bulb) in such a way that the light does
  not bounce directly from the target into your lens as that would cause
  glare, but also so that it does not come in at an extreme angle. When
  using a light bulb, to avoid uneven lighting, do not place your target
  less than 1 meter away from it - 2 meters is a good distance.
- Ensure that the face of the target is evenly lit and that the light
  reflecting from the ground and nearby objects does not fall on it.
- While the sun is far away and you could get away with tilting the
  color target not-perpendicular to the sun, a light bulb will be close
  to you and due to the inverse-square law a tilted target could suffer
  from uneven lighting. Point it towards the light source to guarantee
  even lighting but in such a way as to avoid glare.
- Ensure that the background is non-reflective so as to avoid background
  glare. A tar road or a black cloth work well, white paper does not.
- Make sure that the photos are sharp. The DCP generation software needs
  to be able to identify dust and scratches to isolate them, so do not
  shoot out-of-focus or with motion blur. Use a tripod for the
  incandescent shot.
- Position the target so that it fills the center-third of your frame -
  not more, not less. The center of the frame has the best optics and
  lowest vignetting.
- Remove all filters from your lens.
- Set your aperture to between f/5.6 and f/8. This minimizes vignetting.
- Shoot at your camera's lowest base ISO, typically ISO100. For the StdA
  shot don't go above ISO800 - use a tripod. Using ISO100 and f/8,
  typical exposure times for daylight are about 1/500s and for StdA
  about 1-5s.
- Shoot raw (full raw, not "s-Raw" or any other variety).
- Turn off any camera setting which could influence the raw file, such
  as long exposure noise reduction.
- Exposure bracketing is acceptable and even recommended - it will help
  you/us find the best-exposed shot without clipping. e.g. shoot
  -1EV/0EV/+1EV. Feel free to send all three bracketed images (or all
  six - 3 for daylight, 3 for StdA).
- Rename and/or describe your photos - we don't know what
  "_DSC1234.DNG" means. Rename them to "MAKE MODEL light.extension" for
  example "NIKON D810 daylight +2EV.nef". If you're uploading more than
  one photo, describe how they differ. Let us know where in the world
  you shot them, e.g. "Bikini Atoll, Marshall Islands". Tell us when
  your target was manufactured, or at least the year when you bought it.
  A starting point to automatically rename the files using
  [ExifTool](https://exiftool.org/):

  `exiftool '-FileName<${make}_${model}_iso${iso}_f${aperture}_%c.%le' dir`
- Read each of these points again and make sure your photo conforms to
  them before uploading.
- Open a new issue on the [RawTherapee GitHub page](https://github.com/Beep6581/RawTherapee/issues/new), upload your
  raw photos using [Filebin](https://filebin.net/) and paste a link to
  them in your new GitHub issue. We are interested only in the raw
  photos, not in a ready-made DCP - we will make one ourselves. If you
  bracketed, you can upload a series of each and we will choose the best
  ones.

Shooting the color target in specific lighting situations for yourself:

- As above, you will want the color target to fill a third of your
  frame, f/8, ISO100, sharp, though the points about light do not apply
  here. If you're shooting at an event at which you will take many
  photos under the same lighting conditions - business portraits, real
  estate, a series of images for a 360° panorama in the forest, or a
  wedding - take a shot of the color target just before (or after) you
  take your actual photos.

## Creating the DCP

There are several ways of creating a DCP. The simplest is using the
software that came bundled with the test target. The higher quality and
open-source alternative is using DCamProf.

### Creating DCP profiles using X-Rite software

Simply install the X-Rite ColorChecker Passport software bundled with
your chart (it works in Linux too using [wine](http://www.winehq.org/),
just follow the same steps as outlined for
[installing Adobe DNG Converter in Linux](how_to_convert_raw_formats_to_dng#installing_adobe_dng_converter_in_linux))
and open your shot in it. It expects the shot to be in the DNG format,
so you should first [convert it to DNG](how_to_convert_raw_formats_to_dng). Using it is
straightforward, it's intuitive.

### Creating DCP profiles using DCamProf

DCamProf, written by Anders Torger, is a free and open-source command
line tool for generating camera profiles and performing tasks related to
camera profiles and profiling. Notably, it can generate profiles in both
the ICC and DNG formats, and it is capable of generating high quality,
smooth LUT profiles from CC24 shots or other targets.

The program has extensive, clear documentation. Use that as your primary
source of information. This RawPedia article serves merely as a
reference, to exemplify the process of using it to create a
dual-illuminant DCP profile.

- DCamProf GitHub project and documentation:

  <https://github.com/Beep6581/dcamprof>
- DCamProf homepage:

  <https://torger.se/anders/dcamprof.html>

You will need [ArgyllCMS](http://www.argyllcms.com/) to be installed, as
you need to use some of its tools. In the example code below, the
ArgyllCMS tools are prefixed with `argyll-`. This is needed by some
distributions, such as Gentoo and Sabayon where the "scanin" executable
is called `argyll-scanin`, but not by others, such as Ubuntu where it is
called just `scanin`. Adjust accordingly.


![](daylight_crop.jpg "daylight_crop.jpg")
<figcaption>Daylight_crop.jpg</figcaption>

To generate a dual-illuminant DCP profile from two ColorChecker Passport
shots:

1.  Take two photographs of the color target as described above in the
    [Shooting the color chart](how_to_create_dcp_color_profiles#shooting_the_color_chart)
    section.
2.  For each of the two photos, do the following:
    1.  Open the photo in the latest version of RawTherapee,
    2.  Apply the Neutral profile,
    3.  Looking at the 24-patch target (if you target includes a second
        26-patch side then ignore the 26-patch side), rotate it so that
        the brown patch is in the top-left corner and the black one in
        the bottom-right corner,
    4.  Activate the crop tool, disable "lock ratio", and crop the
        24-patch target at the corner markers,
    5.  In the Color Management tool click the "Save Reference Image"
        button, and in the window that appears make sure "Apply white
        balance" is not checked. Save the one shot as `daylight.tif` and
        the other as `tungsten.tif` to the same folder which contains
        the compiled `dcamprof` executable.
3.  Go into the folder with the compiled `dcamprof` executable and run:

<!-- -->

    argyll-scanin -v -dipn tungsten.tif /usr/share/argyllcms/ref/ColorChecker.cht data-examples/cc24_ref.cie tungsten-diag.tif
    argyll-scanin -v -dipn daylight.tif /usr/share/argyllcms/ref/ColorChecker.cht data-examples/cc24_ref.cie daylight-diag.tif
    ./dcamprof make-profile -i StdA tungsten.ti3 tungsten.json
    ./dcamprof make-profile -i D50 -C daylight.ti3 daylight.json
    ./dcamprof make-dcp -n "Pentax K10D" -d "Pentax K10D" -c "RawTherapee CC0" -t acr -o neutral tungsten.json daylight.json "PENTAX K10D.dcp"

It is very important that during the `make-dcp` step you specify
`tungsten.json` first and `daylight.json` second!

In this example, the output file name is `PENTAX K10D.dcp`. You must use
the name exactly as RawTherapee identifies it. Simply open the StdA or
D50/D65 photo in RawTherapee and toggle the "Quick info" panel (shortcut
key "i") to see which name RawTherapee identifies your camera as.

Your dual-illuminant DCP profile is ready for use.

To have RawTherapee automatically use your new DCP, place the DCP file
in the `dcpprofiles` folder. This folder will typically be inside the
RawTherapee installation folder in Windows, or in
`/usr/share/rawtherapee/dcpprofiles/` in Linux and macOS. Alternatively,
create a new default PP3 for raw files which uses your new DCP - see
[Creating processing profiles for general use](creating_processing_profiles_for_general_use).

Per default DCamProf will smooth the LUT to prioritize smoothness over
accuracy. This makes the profile more robust for general-purpose use. If
you wish you can control all smoothing parameters manually but it's
generally not needed. See DCamProf's documentation for further
information.
