---
title: Lens Geometry
contributors:
  - DrSlony
  - XavAL
tags:
  - 'Tool Description'
  - 'Transform Tab'
toc: true
---

To keep the preview fast, RawTherapee uses the preview image of the
current zoom level when applying these transformations. Because of this,
the preview image can become soft. Lets assume you are editing a Nikon
D700 image: 4256×2832px (that's 12.1 megapixels), and the preview
image's size is 600x400px. Rotating it 5° will not be the same as
rotating the full 12.1Mp image and then scaling it down to 600x400px.
The former will be softer than the latter, though rotating the former
will take less time than rotating the latter, which is why RawTherapee
does that. But don't worry, when saving the image RawTherapee uses the
full resolution image, so it will be sharp. If you zoom the preview in,
then RawTherapee will use this higher resolution preview image when
calculating the transformation, so to see what the saved file will look
like, just zoom in to 100%
![magnifier-1to1.png](magnifier-1to1.png "magnifier-1to1.png").

## Introduction

This tool comprises of several sub-tools which share common adjustment
parameters. They allow you to fix errors and distortions which derive
from imperfections in your camera gear, such as lens distortion and
vignetting, as well as to improve issues deriving from the placement and
adjustment of your camera, such as an uneven horizon or perspective
distortion.

### Lens Hood

Do not confuse vignetting with a blurry lens hood being visible in the
corners of your image. Some cameras, typically small ones - compacts,
bridge-type and even mirrorless - will capture parts of the lens hood or
lens mechanism in the corners of the frame. Typically the same cameras
have lenses which suffer from strong distortion. The way these cameras
deal with these two problems is by correcting the distortion, as a
result of which the corners of the image get "pushed out" beyond the
frame, thereby hiding the corners darkened by the lens or lens hood.
When you view a JPEG image from these cameras the distortion has already
been corrected in-camera, so owners are often unaware that the problem
existed in the first place and are surprised to find that the raw image
shows these dark corners.

It is not possible to fix the dark corner issue by using vignetting
correction - there is no information about the scene in those corners,
the scene is occluded by the lens mechanism/lens hood. Do as the camera
does: correct the distortion, and the dark corners disappear

## Automated Correction

Some of these sub-tools are capable of performing their function in an
automated way with a single click by relying on data which describes the
imperfections inherent to a given camera and lens. There are two sources
for this data: Lensfun and Adobe LCP.

### Lensfun

Lensfun is an open-source project which contains a database of lens
parameters. You can find more information about it in the [Lensfun
FAQ](https://lensfun.github.io/faq/).

The tools "Profiled Lens Correction" and "Perspective" rely on data from
Lensfun to simplify and automate the work of specifying the right
camera/lens information and finding the optimal correction parameters.

In order for RawTherapee to be able to automatically query the Lensfun
database, RawTherapee needs to know which camera and lens was used to
take a given photo. This information is decoded from the photo's
metadata (currently using a home-made solution, and in the future using
[Exiv2](https://exiv2.org/)). To check whether the camera and lens names
are decoded correctly, open a photo and in the Editor tab hit the
"![info.png](info.png) Quick info" button. The
info panel should show your camera and lens names. If it does not, then
RawTherapee failed to decode this information correctly. For the time
being, there is nothing you can do about this but wait for a new version
of RawTherapee. This will hopefully change in RawTherapee 6.0 which will
use Exiv2.

If your camera and lens cannot be decoded from the photo's metadata, you
may still be able to specify them manually, depending on the tool.

#### How to check whether your Lensfun database is intact

To find out whether RawTherapee can query the Lensfun database at all,
look at the "Camera" and "Lens" drop-downs in the Profiled Lens
Correction tool. If you click on either drop-down, it should contain a
long list of entries. If it does not, then your RawTherapee installation
is buggy - try reinstalling, and if the problem persists then seek help
via the [forum](forum).

#### How to check whether your Lensfun database contains information about your camera/lens

To find out whether your Lensfun database contains data for your camera
and lens, open a raw file and look at the "Camera" and "Lens" drop-downs
in the "Profiled Lens Correction" tool. They should both automatically
show the correct camera and lens.

Common causes of failure to auto-detect a profile include:

- RawTherapee could not decode the camera and lens make and model.
- A profile for your camera/lens combination does not exist in your
  version of the Lensfun database.
- A profile for your camera/lens combination does exist, but it uses
  names and parameters which differ enough to confuse the matching
  algorithm, for example "Pentax" vs "Ricoh Pentax", or "F4.0" vs "f/4".

You can try manually looking through each drop-down list to select the
right camera and lens.

If you do not find your camera or lens in the drop-downs, then your
version of the Lensfun database does not contain this data. Update your
Lensfun database. If it still does not contain your camera or lens, then
play your part in the open-source world and contribute this
information - see the [Lensfun documentation](https://lensfun.github.io/calibration/) to find out how
to measure the parameters and contribute them for everyone's benefit.

#### Updating your Lensfun database in Linux

Run the executable
[`lensfun-update-data`](https://lensfun.github.io/manual/v0.3.2/lensfun-update-data.html)
to download the latest version of the Lensfun database. Restart
RawTherapee.

Alternatively, you could copy and paste the relevant section from the
Lensfun database (which could be taken from one of the files in
`/usr/share/lensfun/`) into a new file
`$HOME/.local/share/lensfun/myLensfun` and modify the relevant parameter
to match the metadata from your photos. You can find the camera and lens
name and parameters contained in your photos by viewing the
"![info.png](info.png) Quick info" panel.

Note that while editing the Lensfun database in `/usr/share/lensfun/`
directly may be possible, this is not recommended because you could lose
your changes during an update.

Should you need to use a Lensfun database in a custom location, you can
point RawTherapee to it by editing the
[`options`](file_paths#config) file and setting the
`DBDirectory` key's value to the absolute path of the custom Lensfun
database file.

#### Updating your Lensfun database in Windows

1.  Close RawTherapee while doing this.
2.  Download the Lensfun database. Either get the appropriate XML file
    from <https://github.com/lensfun/lensfun/tree/master/data/db> or
    download the latest release zip from
    <https://github.com/lensfun/lensfun/releases> and find all XML files
    in the `data\db` folder.
3.  Move these XML files to the Lensfun database folder on your
    computer, which could be in
    `C:\Program Files\darktable\share\lensfun\version_1`.

### LCP

Adobe provides Lens Correction Profiles (LCP) and the tools needed to
create them. These are text files which describe the distortion,
vignetting and chromatic aberrations (CA) of a lens, so that simply
loading this file in LCP-capable software such as RawTherapee will
correct these issues. Select an [Adobe LCP](http://www.adobe.com/products/photoshop/extend.displayTab2.html#resources)
file (read the guide on [how to get LCP profiles](how_to_get_lcp_and_dcp_profiles)) to automatically
correct geometric distortion, vignetting and lateral chromatic
aberrations.

The [Profiled Lens Correction](/lens--geometry/#profiled-lens-correction) tool's
"geometric distortion" feature can be used together with the manual
[Distortion Correction](/lens--geometry/#geometric-distortion)
tool, and the vignetting correction feature can be used together with
the manual [Vignetting Correction](/lens--geometry/#vignetting-correction) tool. This
lets you apply manual adjustments in addition to the LCP profile's
automatic adjustments, either for artistic reasons or if the LCP fails
to sufficiently correct a parameter. Be careful that you don't overdo
the distortion and vignetting correction by forgetting to turn the
manual tools off if you use the LCP equivalents. However, the Profiled
Lens Correction tool's vignetting correction is mutually exclusive with
the [Flat Field](flat_field) tool - that is, when you select
a flat-field image, then the LCP's vignetting correction will have no
effect.

The following restrictions apply:

- Geometric distortion, vignetting and chromatic aberration correction
  are all supported in raw files, but only geometric distortion
  correction is supported in non-raw files.
- Chromatic aberration correction is only supported if the Exif
  information contains the focus distance (e.g. in DNGs from Nikon
  files).
- To keep the preview fast and responsive, the preview image is used to
  show the effects of the LCP. As this preview image is small (exactly
  the size you see), fixing the distortions will make it appear a little
  blurry. This has no effect on your saved image, which will be sharp,
  and so will the zoomed-to-100% preview. Only the zoomed-out preview
  might look soft.

As with any other tool, you can apply an LCP to multiple images either
by including it in the processing profile (see
[Creating processing profiles for general use](creating_processing_profiles_for_general_use)), or by
selecting multiple images where the same lens was used (you can use the
Metadata Filter in the File Browser tab to make this easier) and
applying the LCP from the File Browser tab.



## Interface

### Method

TODO Explain what "logarithmic" and "linear" do.

### Auto-Fill

![](lensgeometry_bluehorse_barrel_distortion_correction.jpg "lensgeometry_bluehorse_barrel_distortion_correction.jpg")

Distortion correction

![](lensgeometry_bluehorse_barrel_distortion_correction_autofill.jpg "lensgeometry_bluehorse_barrel_distortion_correction_autofill.jpg")

Autofill


This option will upscale or downscale the photo to the extent that the
whole image fits within the image boundaries with no black borders
visible.

When correcting images that suffer from [barrel distortion](https://en.wikipedia.org/wiki/Distortion_(optics)),
"Auto-fill" will perform downscaling to fit as much of the re-projected
image as possible into the image boundaries, so that you don't
unnecessarily lose any parts of the image. Conversely if the image
suffers from pincushion distortion, "Auto-Fill" will upscale the
corrected image to fill the frame without black borders around the
periphery.



### Auto-Crop

![](lensgeometry_bluehorse_autocrop_after_distortion_correction.jpg "lensgeometry_bluehorse_autocrop_after_distortion_correction.jpg")

Distortion correction

![](lensgeometry_bluehorse_autocrop_after_rotation.jpg "lensgeometry_bluehorse_autocrop_after_rotation.jpg")

After rotation


"![crop-auto.png](crop-auto.png) Auto-Crop" is
available when "Auto-fill" is disabled. When activated, it will not
cause image interpolation, but instead will crop away the empty space
left by the distortion correction or image rotation.



### Rotate

![900px](rotate.jpg)

Rotate the image between -45° and +45°. Use the
"![rotate-straighten.png>](rotate-straighten.png "rotate-straighten.png")
Select Straight Line" button to set either a vertical or a horizontal
image alignment. Use the mouse to draw this line - click and hold mouse
to start, move to draw a new vertical or horizontal axis and release to
engage image rotation.



### Perspective

The Perspective tool offers the "simple" manual mode, which is there to
provide backward compatibility with RawTherapee versions 5.8 and older,
and a powerful "camera-based" guided/automated mode introduced in
RawTherapee 5.9. Try the "camera-based" mode first, as when it works
it's the easiest and most accurate. Click on the "automatic" buttons for
correcting horizontal, vertical or both perspectives. Revert to manual
adjustments if these automatic adjustments fail.

![](perspective_horizontal.jpg "perspective_horizontal.jpg")

Perspective_horizontal

![](perspective_vertical.jpg "perspective_vertical.jpg")

Perspective_vertical.

#### Simple

- Horizontal: when your picture was taken while you were slightly
  off-center of the object, you can correct this (within certain limits)
  with the horizontal slider.
- Vertical: very useful to correct 'falling lines', e.g. when
  photographing architecture. Higher values for both sliders produce
  heavy distortion, so use with care. Or don't care at all and have fun!

#### Camera-Based

Camera-based correction considers the image's field of view and offset
from the optical center to produce a physically correct perspective
correction.

##### Correction

These parameters define your image's attributes and correct perspective
distortion.

- Focal length: set this to the lens' physical focal length in
  millimetres. Automatically set from the image metadata if present.
- Crop factor: set this to the image's crop factor (the camera crop
  factor and any additional cropping such as digital zoom). It is
  automatically set from the image metadata if present. This tool really
  only needs to know the field of view, so the "Focal length" and "Crop
  factor" sliders are for convenience. You can use any equivalent
  combination of focal length and crop factor. For example, if you only
  know the 35mm equivalent focal length, use that for "Focal length" and
  set "Crop factor" to 1.
- Horizontal/Vertical shift: use these to shift the image until the
  resulting image's center lines up with the optical center. This is
  usually not necessary unless you've used the shift function of a
  tilt-shift lens or are editing an image that has already been
  post-processed with an off-center crop. Units are in percent of the
  image width/height.
- Rotation: corrects rotation around the optical axis. This is different
  from the [Rotate](lens/geometry#rotate) tool because it is
  applied after the shift (they are effectively the same if
  horizontal/vertical shift are 0). Units are degrees.
- Horizontal/Vertical: corrects perspective distortion due to the
  camera's yaw and pitch relative to the subject. Units are degrees.

##### Post-correction adjustment

These are applied **after** correcting all perspective distortion.

- Horizontal/Vertical Shift: use these to re-center the image to your
  liking. Units are percent of the image width/height.
- Rotation: totation that is applied after correction. Units are
  degrees.

##### Recovery

These **add** ​perspective distortion. If no post-correction adjustments
are made and only one of horizontal or vertical corrections are made,
these can have the same effect as reducing the strength of the
horizontal/vertical correction.

An example use-case is making pictures of buildings look more natural.
After applying both horizontal and vertical correction, you get
unnatural results when reducing the strength of the vertical correction.
Instead, use Vertical Recovery.

##### Automatic

Taken from [ART](https://bitbucket.org/agriggio/art/wiki/Home), which in
turn was taken from [darktable](https://www.darktable.org/), automatic
correction finds likely parallel lines in the image and corrects the
perspective automatically. The camera data (focal length, crop factor,
and shifts) must be properly set for this to work correctly. There are 3
automatic options:

- Vertical: automatically sets Rotation and Vertical correction.
- Horizontal: automatically sets Rotation and Horizontal Correction.
  Vertical correction must be properly set because the camera's pitch
  determines how the camera is rotated to represent yaw.
- Both: automatically sets Rotation, Vertical, and Horizontal
  correction.

##### Control lines

Automatic line detection may not always work properly. In this case, you
can manually draw the control lines. There are two types of lines:
horizontal and vertical.

###### Drawing lines

Start by clicking either the perspective correction button on the
editor's top toolbar, or clicking the Control lines's Edit button.

Add a new line by holding the \`ctrl\` key and clicking. The line will
start and end where you clicked. If instead you don't let go of the
mouse button, you can drag one of the endpoints and place it where you
want. The type of line is automatically set according to the orientation
of the line.

Fine-tune the line by click-and-dragging the endpoints or the line
itself. Change the line type by clicking the icon in the middle of the
line.

Delete a line by right-clicking on it. Delete all lines by clicking the
Delete all button.

Exit the line editing mode by doing any of the following: click the
perspective correction button, click the Edit button, click the Apply
button, right-click the image, or use another on-image tool (graduated
filter, local adjustments, straighten, etc.).

###### Apply

In editing mode, click the Control lines's Apply button or the
perspective correction button on the top toolbar to apply correction
using the control lines. There are three correction options which behave
the same as the automatic options. The correction option is determined
by the number of each type of line you have. For vertical correction,
you need at least two vertical lines and at most one horizontal line.
Horizontal correction is similar. For vertical and horizontal
correction, you need at least two of each type of line.

### Profiled Lens Correction

This tool group allows you to use either Lensfun or Adobe LCP to
automatically correct:

- Geometric distortion
- Vignetting
- Chromatic aberration

Some of these automatic corrections can work together with manual
corrections, while others are mutually exclusive. See the notes in the
Lensfun and LCP sections of this page.

### Geometric Distortion

![framed](/images/rt_distortion_correction.png)

Corrects lens distortion. A negative number corrects barrel distortion,
a positive value will correct pincushion distortion. You can place a
grid over the image by activating [Crop](crop) (without
cropping) and using "*Guide Type \> Grid*". This may serve as a guide to
correct lens distortion.

The "*Automatic*" button only works if your camera corrected the
distortion of the JPEG image embedded in the raw file (most cameras
embed a JPEG image in every raw file, and some cameras correct the
distortion of that image too). What this feature does is it looks at the
JPEG image and, if it was corrected, tries to fix distortion in the raw
image by making it match the JPEG image. There are two limitations:

- If the JPEG image was not distortion-corrected by your camera, this
  button will have no effect.
- If the JPEG image is insufficiently corrected or over-corrected, so
  will the results be, but as the computed correction will be shown on
  the *Amount* slider, you can further refine it manually.



### Chromatic Aberration Correction

![](chromatic_aberration_auto1.jpg "chromatic_aberration_auto1.jpg")

chromatic_aberration_auto1

![](chromatic_aberration_auto2.jpg "chromatic_aberration_auto2.jpg")

chromatic_aberration_auto2


This "Chromatic Aberration Correction" tool in the *Transform* tab works
on the image **after** demosaicing. The [Chromatic Aberration](chromatic_aberration) tool in the *Raw* tab works
on the image **before** demosaicing.

Chromatic aberration can be corrected by using the "Red" and "Blue"
sliders. Normally you won't see any chromatic aberration in the
fit-to-screen preview, therefore it is highly recommended to open a
detail window
![window-add.png](window-add.png "window-add.png") or to
zoom the main preview in to 100%
![magnifier-1to1.png>](magnifier-1to1.png "magnifier-1to1.png")
or more when you attempt this kind of correction. As in other software
tools, this algorithm eliminates moderate chromatic aberration quite
well. Do not expect miracles with images having extremely high chromatic
aberration - garbage in, garbage out.



### Vignetting Correction

Vignetting means light fall-off around the periphery of an image as
compared to the center. One of the differences between a cheap lens and
an expensive one is that the former is likely to produce stronger
vignetting than the latter. The "Vignetting Correction" tool is meant to
correct vignetting caused by the lens. This tool is not intended for
artistic vignetting; use the [Vignetting Filter](vignetting_filter) tool for that.

Amount
Setting the "Amount" slider to a positive value brightens the four edges
of the images to correct the classical vignetting. Setting it to a
negative value darkens them.

Radius
Influences how much of the image beginning from the edges will be
brightened or darkened. Lower values: area of darkening is bigger;
higher values: area of darkening is smaller.

Strength
Amplifies the settings of the "Amount" and "Radius" sliders. Set
"Amount" to -100, "Radius" to 50 and move "Strength" from 1 to 100 to
see how this works.
