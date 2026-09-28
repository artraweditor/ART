---
title: Flat-Field
contributors:
  - DrSlony
  - XavAL
tags:
  - 'Tool Description'
  - 'Raw Tab'
toc: true
---

[thumb](/images/flatfield_landscape.jpg) [Flat-field correction](https://en.wikipedia.org/wiki/Flat-field_correction) is used
to compensate for the non-uniformity characteristics of the camera and
lens combination. A well known example of such non-uniformity is
vignetting - a peripheral darkening of the image, more pronounced in the
corner areas. Another example, more familiar to users of digital medium
format cameras, is the lens cast effect - both color and luminance
non-uniformity of the image field. Both of these examples of non-uniform
image capture can be further complicated by a possible misalignment of
the lens mount or by the usage of a tilt-shift lens. Another set of
examples of capture non-uniformity is due to light leakage in the
camera, thermal non-uniformity of the sensor or defects/irregularities
in the sensor readout electronics. An alternate name for this type of
function used by some other raw converters is "LCC correction", ie "lens
color cast correction" or just "lens cast correction", but flat-field
correction is the better name as it can as decribed correct for many
sorts of non-uniformity not just lens cast.

Manual correction of these effects in post-production is quite
difficult, especially when needed to be reproduced on a series of images
captured under various conditions, and would rarely be perfect.

The "Flat-Field" correction tool in RawTherapee allows both automated
and user-guided modes. Flat-field correction is performed only on linear
raw data in the beginning of the imaging pipeline and does not introduce
gamma-induced shifts. Thus in RawTherapee flat-field correction can be
applied to raw files only.

Due to performance considerations, thumbnail images do not reflect
flat-field corrections. At present only the [main preview](the_image_editor_tab#the_preview_panel) in the
[Editor](the_image_editor_tab#the_preview_panel) tab and the
output image can be flat-field corrected.

Accuracy of the flat-field correction is largely based on use of the
appropriate flat-field image which should fulfill the requirements of
the sort of correction you desire - either lens/camera non-uniformity,
or the same including the removal of dust spots. The creation of
flat-field images for both purposes is described in the next section.

[thumb](image:flatfield_remove_itself.jpg) To illustrate
flat-field correction, a flat-field image has been applied unto itself.
There is a noticeable light falloff asymmetry shown in the "before"
image at the top, as well as a green color-cast. Flat-field correction
removes both color and luminance non-uniformities and results in a
perfectly uniform image. The histogram of the bottom "after" image
indicates that the corrected image does not have any variation in the
tones - exactly what one would expect from a uniform (flat) field. The
same level of correction is applied to the "real" image when it is
flat-field corrected.

All parameters in the "Flat-Field" panel are saved to the processing
profile. These settings can be copied and pasted to other images just as
any other setting. This includes the flat-field "Auto-selection" option.
Pasting it to different images will result in independent auto-selection
of the appropriate flat-field for each of them.

## Creating and Using Flat-Field Images for the Correction of Camera/Lens Aberrations

It is sufficient to shoot a single flat field image using the same
camera body and lens setup as the images you want to apply it to - you
do not need several flat-field images with identical settings, unlike
dark-frame shots where in order to get higher quality you do need to
take several shots with identical settings and merge them. You do not
need to merge flat-field shots since merging serves to increase the
signal:noise ratio, and this is unnecessary as the flat-field image gets
blurred and should have a low ISO. Shoot it at ISO-100 if possible. The
white balance setting is irrelevant. The focus setting of the lens while
taking the flat-field shot for camera/lens aberration correction does
not need to match that of the image you want to apply it to (this is not
true if you take flat-field shots for dust removal, as explained in that
section). Keep the lens de-focused while taking the flat-field shot.
Your flat-field image should be a de-focused photograph of a uniformly
lit, uniformly colored and toned subject.

The most optimal method of shooting such a flat-field image is to use a
uniform piece of milky, semi-opaque [Poly(methyl methacrylate)](https://en.wikipedia.org/wiki/Poly(methyl_methacrylate))
(PMMA, also sold under the names Acrylite, Lucite, Perspex and
Plexiglas). It should be plain, colorless, smooth, with no symbols or
writing etched or
[embossed](https://en.wikipedia.org/wiki/Paper_embossing) into it. You
can find sheets at hardware and art stores, or you may have luck finding
an empty container made of such material - [Nordsjö Vävlim](http://www.nordsjoidedesign.se/se/PRODUKTER/Ovrigt/Tapettillbehor-1261.html)
containers work well. Cut out a shape the size of your lens barrel. If
you use a lens with an extremely wide angle of view, such as Samyang 8mm
fisheye lens, then the size will need to be several times larger than
the lens barrel - at least 20x20cm. The thickness of the PMMA sheet
should be at least 1mm, preferably 3mm or more, to sufficiently blur and
distribute the light shining through it. If it is glossy, use fine
sandpaper to make the surface matte. This is now your flat-field filter.

Hold your flat-field filter in front of the lens (at full contact with
the lens/filter barrel), click the auto-exposure button, and photograph
it against a uniform illumination (e.g. point the camera towards a clear
sky). The idea is to provide a uniform entrance illumination into the
lens. As a result, all non-uniformities of the lens/camera combination
can be recognized in the captured raw image as deviations from the ideal
spatially uniform (flat-field) response. Take one shot for every major
aperture value (f/1.4, f/2, f/2.8, f/4, f/5.6, f/8, f/11, etc.).
Remember to press the auto-exposure button each time to keep the
histogram centered without clipping.

If you are using a zoom lens instead of a prime, you may need to check
whether you need to take a series of shots at major zoom intervals. For
example if you have an 18-55mm lens, you may need to take one series of
shots at 18mm, one at 36mm and one at 55mm. Test this for yourself with
your own gear, as maybe the 36mm shot will be almost identical to the
55mm one and so you could skip the 55mm series of shots.

If you use an electronic lens then the filename is irrelevant as
RawTherapee can automatically find the correct flat-field image, however
if you use an old manual lens which does not record the aperture or
focal length, then name your files as follows:

` ff_`*<lens>*`_`*<YYYYMMDD>*`_`*<focallength>*`_`*<aperture>*`.`*<raw>*

for example:

` ff_20141009_pentax18-55mm_36mm_f11.dng`

This will make finding them manually easy.

Apply the flat-field images to your photos, matching the aperture values
of the flat-field images with those of your photos, and keep the "Blur
Radius" slider at 32 or above.

If you haven't yet found a PMMA sheet and you need a flat-field image
ASAP, you can take a completely de-focused photo of a plain blank wall
or a large sheet of matte paper. The difficulty here will be getting the
light uniform - remember that the [inverse square law](https://en.wikipedia.org/wiki/Inverse-square_law) is playing
against you. The light source should be far away and diffused, as you
will not get a good flat-field image using a room light or a diffused
strobe. Do not replace the PMMA sheet for a piece of paper, as the paper
is far too irregular, and all the variations in thickness and density
will show up as lighter/darker regions which will get subtracted from
the photo you apply it to. If you want to use paper then get a large
blank sheet, place it in an evenly illuminated place such as on a wall
opposite a large window or outdoors on the ground under the sky. Zoom
into it, de-focused, and take the shots the same way as described above.

## Creating and Using Flat-Field Images for Dust Spot Removal

Carefully follow the instructions from above, with the changes and
additions described here.

If you intend to use the Flat Field tool for dust spot removal, it is
advisable that you take a flat field shot not too long before or after
you take the actual photos you want to apply it to, because as time goes
by new dust spots on your sensor and lens can appear and old ones can
shift or disappear, and if that happens perfect dust spot removal
becomes impossible.

The main difference between using a flat-field image for the correction
of camera/lens aberrations and for dust removal is that the image will
not get blurred inside RawTherapee, and this requires a few changes.
Using the flat-field filter described above will make taking these shots
significantly easier, faster and more accurate. In addition to shooting
one flat-field image at every aperture value, you will also need to take
into account the focus setting of the lens. Trying to apply a flat-field
image for dust removal taken with the focus setting at 0.5m to a photo
taken with the focus at infinity will most likely fail because changing
focus zooms the image slightly, even on prime lenses. Luckily, the zoom
displacement is very small, so if for every aperture you take one shot
at the closes focus (e.g. 0.5m), one at about 2m and one at infinity,
this will surely suffice.

The same requirements for zoom lenses apply as described above, as does
the file-naming procedure. Additionally, you will want to include the
focus setting in your filename:

` ff_`*<lens>*`_`*<YYYYMMDD>*`_`*<focallength>*`_`*<aperture>*`_`*<focusdistance>*`.`*<raw>*

for example:

` ff_20141009_pentax18-55mm_36mm_f11_2m.dng`

Now you see why having a small PMMA flat-field filter is so useful -
instead of shooting whole series of shots at home, you can just take the
needed shot without changing any camera settings right after you take
your actual photo, standing on the mountain top at sunrise or crouching
shooting fungus in the woods.

When taking certain kinds of shots, for example macro photos, I tend to
stick to f/11, as for my lens this is the optimal trade-off between
depth of field and sharpness. If I make the aperture any smaller,
[diffraction](https://en.wikipedia.org/wiki/F-number#Effects_on_image_sharpness)
will have a negative effect on the perceived sharpness. Sticking to f/11
for my macros also means that I only need to take one series of
flat-field shots. The series would consist of three shots: one at f/11
with the focus at minimum, one with the focus mid-way, and one at
infinity. Having a handy flat-field filter makes that very easy.

## When to Take Flat-Field Shots

As described above, make a library containing all the required series of
shots for your lens(es). You will want to update this library when you
get a new lens or a new camera body. Additionally, if you use flat-field
shots for dust spot removal, you will want to take new shots after you
clean your camera sensor, and when the dust and spot layout changes,
i.e. new dust spots appear or old ones disappear.

## Algorithm Specifics & Concise Summary

[thumb](image:rt_ff_dust1.jpg) The user or auto-selected raw
flat-field image need not have the same white balance as the image to
which it is applied. The flat-field is blurred according to any of
several user-selectable blur types and choice of blur radius. The
blurred flat-field serves as a template for vignetting, lens cast, etc.
and be used to correct the corresponding issues in the image file.
"Area" blur uses a [box blur](https://en.wikipedia.org/wiki/Box_blur) of
the flat-field file and is the normal use of the correction. Use a large
blur radius to smooth the flat-field image in order to mitigate any
imperfections such as flat-field image noise, dust spots, etc. Use of a
small radius will leave these effects in the flat-field image, and will
leave their imprint on the corrected image file. This can be used to
advantage; for instance if the flat-field has the same dust spots as the
image file, use of a blur radius of 0-1 pixels will subtract the
darkening caused by the dust, therefore eliminating dust spots from the
image. Use the "vertical", "horizontal" or "vertical+horizontal" blur
options if the camera generates repeatable line pattern noise, such as
repeatable vertical lines.

## Organising Flat-Fields

[thumb](image:flatfield_flatfields.jpg).\]\] Non-uniformity
of the captured field depends on the following parameters:

- Camera (camera & sensor combination in case when a [digital   back](https://en.wikipedia.org/wiki/Digital_back) is used),
- Lens,
- Focal distance,
- Aperture,
- Lens tilt/shift

It is recommended to assemble a library of flat-fields for camera/lens
combinations, taken at various aperture settings (ones which you use in
your real photography). It is advisable to name flat-fields
descriptively, so that files can be recognized easier by the user,
preferably incorporating all parameters listed above. During the
flat-field correction process these parameters are read from the Exif
data only and the filename is irrelevant from this perspective. The
flat-fields should be stored in a dedicated directory. RawTherapee
allows to point to it via "Preferences \> Image Processing \> Flat Field
\> Flat-fields directory". Setting that directory initiates an analysis
of its content, and a count of qualifying files and templates is
reported in the UI once the scan is complete (this could take a short
while depending on the number of flat-field images you have).

## Flat-Field File Browser Context Menu Options

![thumb](/images/flatfield_moveto.jpg) tab.\]\] You can apply
and manage flat-field images from within the [File Browser](the_file_browser_tab) tab, by right-clicking on a
thumbnail and selecting the "Flat-field" option. You will be presented
with three sub-options:

- "Select flat-field" brings up the file selection dialog to select a
  flat-field file to be applied to the selected images.
- "Auto flat-field" allows to run the "Auto-select" option on the
  currently selected images.
- "Move to flat-fields directory" moves the selected image into the
  directory specified in [Preferences](preferences).



## Auto Selection

[left](/images/flatfield_autoselection.jpg) Flat-field
auto-selection capability can be engaged simply by checking the
"Auto-selection" checkbox. RawTherapee will search through the files in
the "Flat-fields directory" specified in preferences and select the
exact or, if not available, the closest match to the image being
corrected based on camera make, model, lens, focal length, aperture and
date of the flat-field file. If a match is found, the filename of the
selected flat-field image will be displayed along with its aperture
setting. If a match is not found, flat-field correction will not be
applied and a message will be displayed to the user. If more than one
exactly matching flat-fields are found, their data will be averaged and
then used for flat-field correction.

Auto-selection does not account for the tilt-shift settings used on the
lens, therefore such flat-fields should not be stored in the main
flat-fields directory, but rather in a descriptively named
sub-directory. Such unusual flat-field files should be applied
manually.

## Auto matching logic

Key for flat-fields (ffInfo::key):

- camera manufacturer
- camera model
- lens
- focal length
- aperture

The search for the best match is twofold:

- if perfect matches by key are found, then the list is scanned for
  lesser distance in time,
- otherwise if no match is found, the whole list is searched for lesser
  distance in lens and aperture.

## Blur Type

Area
The default and generally most useful setting to apply blur action
equally in all directions. Works well for correcting vignetting and lens
cast.

Vertical
Blurs the flat-field vertically to compensate for vertical
non-uniformities. This is useful if the vertical sensor readout has
variation between columns.

Horizontal
Blurs the flat-field horizontally to compensate for horizontal
non-uniformities. This is useful if the horizontal sensor readout has
variation between rows.

Vertical + Horizontal
Blurs the flat-field sequentially horizontally and then vertically to
compensate for both vertical and horizontal non-uniformities.

Note that the concept of vertical and horizontal is related to how the
sensor is oriented in the raw file, which is always the same regardless
if the camera was held horizontally or vertically when shooting. It
varies between camera models if the sensor data is stored in landscape
or portrait format so if you want to use Vertical or Horizontal modes
you need to test which direction that is right for your model.

## Blur Radius

The "Blur Radius" slider controls the degree of blurring of the
flat-field data. The default value of 32 is usually sufficient to get
rid of localized variations of raw data due to noise. Setting the blur
radius to 0 skips the blurring process and allows to correct for dust
and other debris on the sensor (as long as their position in the
flat-field image is the same as in the photo the flat-field is applied
to) at the expense of carrying noise from the flat-field file into the
corrected image. If such correction is desired, it is advisable to
create flat-field files with minimum amount of noise at the lowest ISO
setting and optimal light exposure.

## Clip Control

Applying a flat-field image can cause nearly-overexposed areas in the
image to become overexposed due to the correction. Activating the Clip
Control option will keep the flat-field image from clipping the actual
image. Areas in the actual image which were already clipped before the
application of the flat-field image may acquire a color cast, therefore
as a rule of thumb, if your photo contains overexposed areas it is
better to not use Clip Control.

To understand exactly how Clip Control works we need to get a bit
technical. The Flat-Field tool works by adjusting the exposure of areas
of the actual image whose corresponding areas in the flat-field image
are different to the measured exposure of the center of the flat-field
image. The factor by which the exposure of an area of the actual image
is increased is proportional to how much darker the corresponding area
in the flat-field image is relative to the measured exposure of the
center of the flat-field image. When Clip Control is disabled or at 0,
pixels of the actual image can have their exposure increased beyond the
raw white level. Clip Control works by calculating the factor against
the raw white level, so that no corrected pixel will exceed the value of
the white level.

For instance, if the required exposure increase of a pixel in the actual
image would be 1.25 that of the raw white level (1), then the limiting
factor would be 1 / 1.25 = 0.8. This factor would then be used in the
final calculation to prevent any pixel's value from exceeding the white
level. The formula for the slider is
`clipControlGui = (1 - limitFactor) * 100`

Therefore with a limiting factor of 0.8, the value of the slider would
be 20.
