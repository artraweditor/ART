---
title: Dark-Frame
contributors:
  - DrSlony
  - XavAL
tags:
  - 'Tool Description'
  - 'Raw Tab'
toc: true
---

[Dark-frame subtraction](https://en.wikipedia.org/wiki/Dark-frame_subtraction) is a
method of dealing with thermal, dark-current and fixed-pattern noise. It
is not effective against high ISO noise because of its different random
nature. In long exposure shots (more than 1 sec) the non-homogeneous
thermal noise becomes evident, mainly due to unevenness of the sensor
and surrounding electronics. A method to mitigate this effect is to
subtract one (or more) shots taken in the same conditions, but with the
lens cap on. Only raw images for the same camera model can be used as
dark-frames, preferably taken around the same time as the photo they are
being subtracted from. As taking a dark-frame shot is simply a matter of
putting the lens cap on and pressing the shutter without changing any
settings, this is not a problem.

In the "Dark-Frame" panel, you can specify a single shot to subtract
from the image, or check "Auto-Selection" and let RT choose the best
match from the directory specified in
"[Preferences](preferences) \> Image Processing \>
Dark-Frame". Under the widget, RT shows how many shots are found and how
many groups of shots are found and averaged into a template. From now
on, put your dark-frame shots there if not already done. You could also
move a shot from the "File Browser" tab into the dark-frames directory
by right-clicking on it and selecting "Dark-Frame \> Move to dark-frames
directory". RT chooses the best match looking for the same camera model
with minimal difference in ISO, exposure duration and date. If more than
one shot with exactly the same properties is found, then an average of
them is used: this produces by far less noise, so it's better to have
4-6 frames taken in the same conditions of the actual photo.

When selecting a dark-frame (or with "Auto-Selection"), RawTherapee
extracts from it all the positions of hot pixels and then always
corrects them in the final image. This correction is better than
applying only the "Hot/dead pixel filter", but works only for hot
(=white) pixels not for dead (black) ones.

## Auto-Matching Logic

Key for dark frames (dfInfo::key):

- camera manufacturer
- camera model
- ISO
- shutter speed

The search for the best match is two-fold:

- if perfect matches by key are found, then the list is scanned for
  lesser distance in time,
- otherwise if no match is found, the whole list is searched for lesser
  difference in ISO and shutter.

## Bad Pixels

RawTherapee can correct a list of [bad pixels](https://en.wikipedia.org/wiki/Defective_pixel) (pixels that are
always black or white or stuck at one color) for your particular camera
model. To do this, you need to write a text file with the absolute raw
coordinates of these pixels: each line specifies a pixel with
`x`<space>`y`<return> positions.

**Important**: RawTherapee cuts some pixels from the top and left edges
of the raw image's border (because they can't be interpolated
correctly). If you look at the pixel coordinates in RawTherapee, beware
of the offset introduced by this cutting. To each coordinate you must
add 4 for Bayer Sensors and 7 for X-Trans Sensors! Alternatively you can
specify the offset (4 for Bayer, 7 for X-Trans) in the first line of the
.badpixels file.

The file has to be located in your dark-frames directory. Set it by
going to "*Preferences
[image:preferences.png](image:preferences.png) \> Image
Processing \> Dark-Frame*". The file has to be named exactly as your
camera's make and model: "*make model.badpixels*". Get the make and
model as RawTherapee expects them by opening a raw image you want
corrected in the [Image Editor](the_image_editor_tab) tab and
looking at the displayed name and model in the "Quick info
![Image:info.png](info.png "Image:info.png")" overlay, shortcut "**i**",
e.g.: "`Pentax K200D.badpixels`"

Remember that these .badpixels files must be saved to the dark-frames
directory!

If you've done the steps correctly and it still doesn't work, you can
verify that your badpixels file is being read by closing RawTherapee,
editing the "[options](file_paths)" file in a text editor and
changing "*Verbose=false*" to "*Verbose=true*", then starting
RawTherapee from a console, opening the photo you want fixed, and
looking at the text output in the console. If you see a message like
"*Pentax K10D.badpixels not found*" then you know what to rename your
badpixels file to. Once you get it working, remember to set "*Verbose*"
back to "*false*".

Pixels in the bad-pixels list will always be corrected in processed
photos as long as the camera make and model matches the badpixels
filename.

## Bad pixel detection software

Programs exist to aid in the detection of bad pixels:

Dead Pixel Test
<http://www.starzen.com/imaging/deadpixeltest.htm> (dead)

Mirror:
<https://web.archive.org/web/20140725130003/http://www.starzen.com/imaging/deadpixeltest.htm>

Mirror: <http://rawtherapee.com/mirror/deadpixeltest.zip>


This file is [free from viruses](https://www.virustotal.com/en/file/11e7a0db897fd3ad9f3e24c97c73b178cfe9f9d246e3dadfe57113318e2def06/analysis/1421736881/).

Pixel Fixer
<http://www.pixelfixer.org/>

Remember to fix the 4px or 7px cutting offset if your version of
RawTherapee needs that.
