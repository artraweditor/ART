---
title: Film Simulation
contributors:
  - DrSlony
  - Andrea.romagnoli
  - XavAL
tags:
  - 'Tool Description'
  - 'Color Tab'
toc: true
---

<img src="/images/Rt_haldclut_london.jpg" title="Rt_haldclut_london.jpg"
width="900" alt="Rt_haldclut_london.jpg" /> The *Film Simulation* tool
allows you to match the colors of a photo to a reference image with a
single click. This tool requires the use of reference images in the
HaldCLUT pattern, in either PNG or TIFF format. Each HaldCLUT image
corresponds to one "look". Though the look can be based on anything,
most of the reference images we ship are based on classic film stock,
ergo the tool's name.

RawTherapee needs to be able to access these HaldCLUT images in order
that you can use the tool. You can download our collection, called the
[RawTherapee Film Simulation Collection - HaldCLUT](http://rawtherapee.com/shared/)
or you can design your own - more on that later on. The first time you
run this tool you will see a message informing you that you need to
point RawTherapee to a folder which contains the reference images. Once
you have them ready in a folder, go to "Preferences \> Image Processing
\> Directories" and set "HaldCLUT directory" to the appropriate folder.

## Startup time

RawTherapee will scan the designated HaldCLUT folder every time you
start the program, that's why it is important that you create a folder
which you will use only for storing these HaldCLUT images and nothing
else, so that RawTherapee does not waste time scanning unrelated files.
As a safety measure, you will be warned if scanning the folder on
startup takes more than 10 seconds. Should that happen, just click the
button in the popup to stop the scan, then go to "Preferences \> Image
Processing \> Directories" to see which folder is being used, and either
point RawTherapee to a folder which contains only HaldCLUT images and
nothing else, or to an empty folder if you don't want to use the Film
Simulation tool.

To give you an idea of how the startup time is affected, the difference
between having 0 images in the HaldCLUT folder and having 500 images in
it (that's more than in our collection) results in a 100ms startup time
difference - that's nothing. If, however, you were to accidentally tell
RawTherapee that the HaldCLUT folder is `C:\Program Files (x86)`, then
the startup time could even take several minutes, as that folder
contains hundreds of thousands of files. As you can see, there is no
reason to worry when using HaldCLUTs as long as you use a dedicated
folder as suggested, keeping only HaldCLUT images in it.

## How It Works

![](/images/Hald_CLUT_Identity_12.png "Hald_CLUT_Identity_12.png") The Film
Simulation tool uses specially prepared images in what is called a
HaldCLUT pattern. "CLUT" means "Color Look-Up Table", while what "Hald"
means is anyone's guess. A HaldCLUT image contains a set of gradients of
various hues arranged in a matrix. The neutral, unaltered state is
called an "identity HaldCLUT image". Applying an identity HaldCLUT to a
photo will result in no change at all. To create a "look", the identity
image is opened in an image editing program - any image editing
program - and the colors are modified in some global way, for example
using levels, curves, adjusting hue and saturation, etc. Only global
adjustments such as the ones listed should be applied - local
adjustments are incompatible with how this works; for example you cannot
make a HaldCLUT image give a tone-mapped look, it cannot denoise, it
cannot sharpen, but it can make skin tones look more tan, and foliage
more vibrant.

Though the HaldCLUT image looks like a set of gradients, what it really
is, is a graphic representation of a matrix of numbers. These numbers
are the "out" or "result" values. The "in" or "source" values are known
to all programs which support HaldCLUT images. These programs known that
a pixel at position x=143 y=0, for example, should be pure red,
RGB=(100%, 0%, 0%). If you edit the identity image and change that pixel
to be less red, say more orange, then save this modified identity image
as a new HaldCLUT file, then whenever you use that file RawTherapee will
make the same change to all pure red pixels. This means that if you want
to make a certain shade of red more orange you don't need to edit boring
numbers to accomplish it - you just open the HaldCLUT identity image in
an image editor such as RawTherapee or GIMP, adjust that shade of red
(or, more realistically, perform some global adjustment which affects
all colors or all shades of red, such as though manipulating RGB
curves), save the HaldCLUT image, and then use it in RawTherapee on any
photo to repeat that same adjustment.

It should not make a difference what color profile is assigned to a
HaldCLUT image. What matters are pixel values, because, remember, a
HaldCLUT image is just a matrix of "out" numbers. "Assigning" a color
profile does not alter stored pixel values, but "applying" or
"converting" does alter stored pixel values, so don't do that. Having
said that, the assigned color profile could have an influence, because
an image editing program might change the working space depending on the
assigned color profile.

Color transformation precision depends on the number of levels (columns)
in the HaldCLUT image, and on the bit depth used to store the HaldCLUT
image. If working on a 32-bit image (or an any-bit image in 32-bit
space) using a 12-level HaldCLUT stored in an 8-bit file, for example,
the HaldCLUT stores color values with less precision than your image (or
your working space) can store. This should not be a problem though, as
unknown colors are interpolated from known values, so posterization
should not occur. The bit depth of the image format you use to store the
HaldCLUT depends on your needs, and is not strictly related to the
number of levels the HaldCLUT contains. Photographers will be fine using
8-level HaldCLUTs stored in 8-bit images even when processing higher
bit-depth photographs.

For more information, refer to Eskil Steenberg's page on HaldCLUT:
<http://www.quelsolaar.com/technology/clut.html>

To generate an identity 12-level 16-bit HaldCLUT image using
ImageMagick, run this command in a console:

`convert hald:12 -depth 16 -colorspace sRGB hald12_16bit.tif`

## Caveat

If you need to generate your own identity HaldCLUT, do not use the
program for generating HaldCLUT images from www.quelsolaar.com as it has
a bug which causes issues with highlights. Use ImageMagick or
GraphicsMagick instead. You can of course use the identity file we
provide here - it is bug-free.

## Make Your Own

This section explains how to put the HaldCLUT identity file to use so
that it reproduces a specific rendering of color and lightness.

1.  Open a photo in RawTherapee or some other image editing program and
    tweak it to your liking. Remember that the Film Simulation tool can
    only reproduce global tonal changes, so make no local changes - no
    local contrast, no tone-mapping, etc.; make no changes which move
    pixels - no distortion correction; use no sharpening or noise
    reduction; make only global tonal adjustments - color and saturation
    changes, curves, levels, L\*a\*b\* adjustments. Save the sidecar
    file or write down the changes you made so that you can reproduce
    the changes in the next step.
2.  Open the identity HaldCLUT image in the same program and apply the
    same sidecar file or re-do the same tweaks as you did in the step
    above.
3.  Save this image as an 8-bit sRGB TIFF or PNG in the HaldCLUT folder
    you pointed RawTherapee to. It's now ready for use. Restart
    RawTherapee so that your new HaldCLUT appears in the list.

Even if your HaldCLUT image contains colors in only 8-bit precision,
missing values will be interpolated so that posterization will not occur
in your photo. As such, since there is no visual loss of quality, we
recommend using the level 12 identity file, or even an 8-level one, and
storing your self-made HaldCLUT images in an 8-bit per channel PNG or
TIFF file.

The color profile assigned to the saved HaldCLUT image does not matter.
What matters are the pixel values of that image. You may be familiar
with the terms "assign color profile" vs "convert to color profile" from
GIMP or Photoshop - which color profile is assigned does not matter
because it does not alter pixel values, but converting matters because
then the pixel values change. When editing the HaldCLUT image in
RawTherapee, the choice of [output color profile](color_management#output_profile) matters because it
alters pixel values of the saved image. As the identity image provided
by us or generated according to our recipe uses the sRGB primary
chromaticities, so you should use RTv2_sRGB or RTv4_sRGB when saving it
in order to preserve the colors.

If you apply this HaldCLUT to a photo in RawTherapee and the photo
unexpectedly and unintentionally becomes considerably darker or lighter
than it should have, then it's likely that the program which you ran it
through did something with the gamma. To remedy, you have to undo what
that program did. Try generating your own 12-level HaldCLUT but instead
of using the "sRGB" colorspace use just "RGB".

### Advanced - Identity DNG

This is experimental, it may or may not work.

Some programs might not let you open a TIFF image. If the program
supports DNG files, and demosaiced ones at that (what Adobe DNG
Converter refers to as "Linear (Demosaiced)"), then you can use this
trick. Using ImageMagick, ExifTool and the commands below, making use of
the fact that DNG is just a form of TIFF, you can generate an identity
HaldCLUT in DNG format:

    convert hald:12 -depth 16 -colorspace RGB -gravity NorthWest -splice 4x4 -gravity SouthEast -splice 4x4 foo.tif
    exiftool -DNGVersion=1.4.0.0 -PhotometricInterpretation='Linear Raw' foo.tif
    mv -v foo.tif Hald_CLUT_Identity_12.dng

Raw editing programs will discard a certain number of pixel rows and
columns from the image edges for technical reasons to do with
demosaicing. How many rows and columns get discarded depends entirely on
the program. You need to figure this out. A 12-level identity HaldCLUT
will have precisely 1728x1728 pixels. When you process that CLUT in a
program whose color effects you're trying to emulate, the saved image
must have precisely 1728x1728 pixels. Since you're fooling the program
into thinking it's working on a raw file, and since it will probably
discard some pixels around the edges, you need to figure out exactly how
many rows and columns of padding are needed and add them around the
image. RawTherapee cuts off 4 pixels all around when reading demosaiced
DNG files, so the command above first adds a 4 pixel row and column to
the bottom and right edges, then another 4 pixel row and column to the
top and left edges. When you open this image in the target program, zoom
into each side's edge and figure out whether you need to add more (or
remove some), then modify the command accordingly.

Once you have the borders figured out, merely open this DNG in the
target program and follow the steps above in the "Make your own"
section.

## Browsing in RawTherapee

There are many HaldCLUTs in our collection, and you will probably want
to try them all out. You don't need to click on each one with the mouse!
There is an easy way of checking them all out, one by one, which
requires no mouse work. Assuming you had already set RawTherapee up to
point to the HaldCLUT folder and you have a photo open and the Film
Simulation tool is ready for use, just select any HaldCLUT image using
the combobox, and use the "up" and "down" keys on your keyboard to apply
the previous and next one. Using this trick, you can even jump across
sub-folders - for example from Black-and-White to Color if using our
collection.

If you would like to compare the effects of several specific HaldCLUTs,
and they are not consecutive in the combobox, an easy way to switch
between them is by [taking a snapshot](the_image_editor_tab#snapshots) after applying
each, and then just clicking on the snapshots.

## RawTherapee Film Simulation Collection

This archive contains a collection of film simulation HaldCLUTs which
you can apply to your photos to instantly match their colors to the film
stocks the HaldCLUTs are based on. Unless otherwise noted in the
filename, they are all in the sRGB color space, 8-bit per channel, in
the PNG image format. Most of them are designed to mimic the results of
various film stocks, pushed and pulled in various ways or faded over
time.

The suffixes +, ++, +++, -, --, --- refer to the strength the film was
[pushed or pulled](https://en.wikipedia.org/wiki/Push_processing) during
development (non-linear), and "generic" refers to the film type usually
sold for rebranding.

[Download](http://rawtherapee.com/shared/HaldCLUT.zip) (402MB!)

<!-- -->

Changelog
2015-09-20
Added the "CreativePack-1" color collection.

Converted all TIFFs to PNG (except for the identity image).

2015-03-25
The identity CLUT had a bug causing cyan colors in the highlights, it
has been replaced with a fixed one.

Numbered the files so they are sorted in the correct order when pushed
or pulled (--, -, normal, +, ++).

2014-08-25
The first public release.

Re-organized into *Color* and *Black-and-White*, sub-folders sorted by
brand.

2014-08-15
Expanded README.txt and added disclaimer.

2014-07-05
The first internal release.

All images re-compressed with maximum lossless compression.

Learn more about HaldCLUTs here:


<http://www.quelsolaar.com/technology/clut.html>

<http://blog.patdavid.net/2013/08/film-emulation-presets-in-gmic-gimp.html>

<http://blog.patdavid.net/2013/09/film-emulation-presets-in-gmic-gimp.html>

Credits:


Pat David - <https://discuss.pixls.us/u/patdavid>

Pavlov Dmitry

Michael Ezra - <https://discuss.pixls.us/u/michaelezra>

Disclaimer:


The trademarked names which may appear in the filenames of the HaldCLUT
images are there for informational purposes only. They serve only to
inform the user which film stock the given HaldCLUT image is designed to
approximate. As there is no way to convey this information other than by
using the trademarked name, we believe this constitutes fair use.
Neither the publisher nor the authors are affiliated with or endorsed by
the companies that own the trademarks.
