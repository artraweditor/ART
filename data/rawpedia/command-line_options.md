---
title: Command-Line Options
contributors:
  - DrSlony
  - Hombre
tags:
  - 'General Information'
toc: true
---

## Explanation


`<Chevrons>` indicate parameters you can change.

`[Square brackets]` mean the parameter is not mandatory.

The pipe symbol `|` indicates a choice of one or the other.

The dash symbol `-` denotes a range of possible values from one to the
other.

Since RawTherapee 5.1, two executables are provided.


### RawTherapee GUI

Use this application to start the version with graphical user interface.

Usage:


`rawtherapee <selected dir>`


Start [File Browser](/file_browser) inside folder.

`rawtherapee <file>`


Start [Image Editor](/editor) with file.



`-w`


Do not open the Windows console. This option is available in Windows
only. If you pass parameters to the RawTherapee executable it spawns a
console window so that you can see the verbose output of your
processing. Normally Windows closes this console directly after
RawTherapee is terminated. To let you see the output we added a prompt
which waits for you to hit a key before closing the console. By
specifying `-w` no console will be opened and therefore no key press is
needed. Useful if you want to invoke `rawtherapee.exe` in batch, e.g.
from a PowerShell script. Please note that `-w` will have no effect for
"Debug" builds where a console window will be opened unless you're
starting RawTherapee from a console window already.

<!-- -->


`-v`


Print the RawTherapee version number and exit.

<!-- -->


`-R`


"Remote" mode, available since RawTherapee 5.2. When opening an image
using "Open with" or by passing its filename as an argument, without
using the `-R` option RawTherapee will open in "no-File-Browser" mode -
that is a mode which lacks the File Browser and Queue tabs as well as
the Preferences button. Using the new `-R` mode, RawTherapee will open
in a full-fledged instance. Using `-R` also allows you to open an image
in an already-running instance of RawTherapee, if that instance was also
started using `-R`. The no-File-Browser mode exists for historical
reasons when RAM requirements were higher and stability was worse. Now
that RawTherapee's memory usage is optimized and it can quickly and
reliably open folders with thousands of images, users may prefer using
the `-R` mode by default.



`-h -?`


Display these commands.

### RawTherapee CLI

Use this application to start the command line only version. You'll find
all command line options to develop your photos without any graphical
user interface.

Usage:


`rawtherapee-cli <options> -c <dir>|<files>`


Convert files in batch with default parameters if no <options>
specified.



`-w`


Do not open the Windows console. This option is available in Windows
only. If you pass parameters to the RawTherapee executable it spawns a
console window so that you can see the verbose output of your
processing. Normally Windows closes this console directly after
RawTherapee is terminated. To let you see the output we added a prompt
which waits for you to hit a key before closing the console. By
specifying `-w` no console will be opened and therefore no key press is
needed. Useful if you want to invoke `rawtherapee-cli.exe` in batch,
e.g. from a PowerShell script.

Other options used with `-c`:


`rawtherapee-cli [-o <output>| -O <output> ] [-q] [-a] [-s|-S] [-p <files>] [-d] [-j[1-100] [-js<1-3>]|[-b<8|16>] <[-t[z] | [-n]]] [-Y] [-f] -c <input>`


`-c <files>`


Specify one or more input files or folders.

When specifying folders, RawTherapee will look for image files which
comply with the selected parsed extensions (see the `-a` option).

The `-c` option must always be the last one.



`-o <file>|<dir>`


Select output file or folder.

Saves output file alongside input file if -o is not specified.



`-O <file>|<dir>`


Select output file or folder and copy PP3 file into it.

Saves output file alongside input file if -O is not specified.



`-q <file>|<dir>`


Quick-start mode. Does not load cached files to speedup start time.



`-a <file>|<dir>`


Process all supported image file types when specifying a folder, even
those not currently selected in Preferences \> File Browser \> Parsed
Extensions.


`-s`

Use the existing sidecar file to build the processing parameters, e.g.
for photo.raw there should be a photo.raw.pp3 file in the same folder.
If the sidecar file does not exist, neutral values will be used.



`-S`

Like `-s` but skip if the sidecar file does not exist.



`-p <file.pp3>`

Specify processing profile to be used for all conversions. You can
specify as many sets of "-p \<file.pp3\>" options as you like, each will
be built on top of the previous one, as explained below.


`-d`

Use the default raw or non-raw PP3 file as set in
[Preferences](/) > [Image Processing](/image_processing_tab) > [Default Processing Profile](/image_processing_tab#default-processing-profile)


`-j[1-100]`

Specify output to be JPEG (default, if -t and -n are not set).  
Optionally, specify compression 1-100 (default value: 92).



`-js<1-3>`

Specify the JPEG [chroma subsampling](http://en.wikipedia.org/wiki/Chroma_subsampling) parameter,
where:


* 1 = Best compression: 2x2, 1x1, 1x1 (4:2:0)  
    Chroma halved vertically and horizontally.

* 2 = Balanced: 2x1, 1x1, 1x1 (4:2:2)  
    Chroma halved horizontally.

* 3 = Best quality: 1x1, 1x1, 1x1 (4:4:4)  
    No chroma subsampling.



`-b<8|16>`


Specify bit depth per channel (16 by default).  
Only applies to TIFF and PNG output, JPEG is always 8.



`-t[z]`

Specify output to be TIFF (16-bit if `-b8` is not set).  
Uncompressed by default, or ZIP compression with 'z'.



`-n`

Specify output to be compressed PNG (16-bit if `-b8` is not set).  
Compression is hard-coded to level 6.



`-Y`

Overwrite output if present.


`-f`

Use the custom fast-export processing pipeline.

Your PP3 files can be incomplete, RawTherapee will set the values as
follows:

1.  A new processing profile is created using neutral values,
2.  If the `-d` option is set, the values are overridden by those found
    in the default raw or non-raw processing profile,
3.  If one or more `-p` options are set, the values are overridden by
    those found in these processing profiles,
4.  If the `-s` or `-S` options are set, the values are finally
    overridden by those found in the sidecar files.

The processing profiles are processed in the order specified on the
command line.

## Redirect Output

To redirect RawTherapee's output to a text file, you have to start it
from a console and append the redirection code as follows:

Windows (cmd.exe)  
`rawtherapee.exe > rtlog.txt 2>&1`

Linux  
`rawtherapee &> rtlog.txt`

## Examples

### Example 1

In Linux, process a single raw which resides in /tmp and is called
"photo.raw", use its sidecar file "photo.raw.pp3" during conversion,
save it in the same folder as "foo.tif", and overwrite the file
"foo.tif" if it exists:

`rawtherapee-cli -o /tmp/foo.tif -s -t -Y -c /tmp/photo.raw`

### Example 2

In the next example, we'll assume that you want to quickly process all
your raw photos from the /tmp/jane01 folder to a web sub-folder by using
the default profile as a basis, using the sidecar profile if it exist,
but with removing some Exif tags (e.g. the camera's serial number) and
adding some IPTC tags (e.g. your usual copyright parameters), plus
resize and sharpen the image for the web (spread over multiple lines for
clarity):

`rawtherapee-cli -o /tmp/Jane01/web -p ~/profiles/iptc.pp3 -s -p ~/profiles/exif.pp3 -p ~/profiles/web.pp3 -t -Y -d -c /tmp/Jane01/`

The processing profile will be built as follows:

1.  A new profile is created using internal default values (hard-coded
    into RawTherapee),
2.  then overridden by those from the default raw profile (`-d`),
3.  then overridden by those found in iptc.pp3,
4.  then overridden by those found in the sidecar file (`-s`) if it
    exists, so you can force some IPTC tags even if already set by
    iptc.pp3,
5.  then overridden by those found in exif.pp3, so you can force the
    profile to erase some tags,
6.  then overridden by those found in web.pp3, to resize and sharpen the
    image, and make sure that the output colorspace is sRGB.

As you can see, the position of the `-s` switch tells when to load the
sidecar profile relative to the other `-p` parameters. That is not the
case for the `-d` switch.

### Example 3

In the third example, we will see how long it takes to process every raw
file in a folder, assuming that each raw photo has a corresponding
processing profile, and discard each output file:

`time { for f in /home/user/photos/2011-11-11/*.raw; do rawtherapee-cli -o /dev/null -S -t -Y -c "$f"; done }`
