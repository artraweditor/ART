---
title: Image Processing Tab
contributors:
  - DrSlony
---

## Default Processing Profile

Specify which profile RawTherapee is to use when opening a raw photo and
when opening a non-raw photo. When you have made your own default
profile, you can tell RawTherapee to always use that one. To do that, to
have it show up in the list, you must save it to RawTherapee's
"*config*" folder. You can find out where it is on the
[file paths](file_paths#processing_profiles) page.

The default processing profile for non-raw files like JPEG or TIFF is
best set to "Neutral". The "Neutral" profile just loads the photo as it
is, without applying anything like [Auto Levels](exposure#auto_levels) or
[Sharpening](sharpening).

## Custom Processing Profile Builder

Executable (or script) file called when a new initial processing profile
should be generated for an image. The path of the communication file
(\*.ini style, a.k.a. "Keyfile") is added as a command line parameter.
It contains various parameters required for the executable or script to
allow a rules-based processing profile generation.

This feature is very powerful; for example it allows you to set lens
correction parameters or noise reduction based on image properties. It
is called just once on the first edit of the picture, or called manually
from the context menu when right-clicking on a thumbnail in the
[File Browser](the_file_browser_tab) or
[Filmstrip](the_image_editor_tab#the_filmstrip)

<b>Note:</b> You are responsible for using double quotes where necessary
if you're using paths containing spaces.

## Processing Profile Handling

"Save processing parameters next to the input file": When checked,
RawTherapee writes a PP3 file with all the edits you made to your photo
next to the input (raw) file. This represents your work (e.g. sharpening
settings used) and can be reloaded later.

"Save processing parameters to the cache": Instead of creating a PP3
file next to the raw, this option - when checked - writes the PP3 to the
cache. When you check the last option only, chances are that you lose
your work (the edits) when installing RawTherapee on a new PC for
instance.

It's usually a good idea to only save the processing parameters next to
the input file, since you can e.g. back them up along with the your
raws.

## Dark-Frame

Specify the directory on your hard disk for searching for the dark frame
shots for long exposure noise subtraction. File with coordinates listing
of the bad pixels must be placed in the same directory for auto
correction.

## Flat-Field

Specify the directory on your hard disk for searching for the flat field
reference images.

## Film Simulation

Specify the directory which contains the HaldCLUT film simulation
presets. See the [Film Simulation](film_simulation) article
for more information.

## Metadata

The option "Copy IPTC/XMP unchanged to output file" changes
RawTherapee's metadata behavior. Usually it will remove all IPTC/XMP
data from the input image and write only its own tags in IPTC section.
This may become a problem when you tag your input files using other
programs - raw files usually contain XMP data. This would be lost. By
checking this option RT will not touch IPTC and XMP data at all, just
pass them through. On the downside all your tagging within RT will not
be saved though.
