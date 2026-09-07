---
title: Forum
contributors:
  - DrSlony
  - XavAL
tags:
  - 'Interact'
toc: true
---

Visit our forum at <https://discuss.pixls.us/c/software/rawtherapee>

## Creating Screenshots

You can create screenshots either using the **Print Screen** button on
your keyboard and then pasting the image into any image editor and
saving it as PNG, or better just install a proper image editor, like the
free and open-source [GIMP](http://www.gimp.org/) (links for Windows and
Mac at the bottom of the page). To take a screenshot in Gimp, click on
"File \> Create \> Screenshot".

Wikipedia has a good [comparison of raster graphics editors](http://en.wikipedia.org/wiki/Comparison_of_raster_graphics_editors).

Suggestions screenshot programs:

- [Shutter (Linux)](http://shutter-project.org/)
- [ShareX (Windows)](https://getsharex.com/)

Regardless of what program you use, it is best if you save your
screenshots to the PNG format (use maximum compression), and never
resize them, since we sometimes have problems when people make their
screenshots so small one cannot make anything out. If you want to use
JPG, they will look nicer if you disable chroma subsampling (disabling
it means setting it to "off", "none", "4:4:4" or "1/1" - these are all
equivalent).

## Uploading Files

If you need to upload **FILES** such as ZIP, 7z, TIFF, raw formats such
as CR2, DNG, PEF, problematic files such as JPG (see note below), PNG,
etc. try these sites:

- [FileBin](http://filebin.net/)
- [SendSpace](https://www.sendspace.com/)
- [Zippyshare](http://www.zippyshare.com/)
- [Dropbox](https://www.dropbox.com/)

Note: If you have JPG/PNG images which cause problems in RawTherapee,
then we must get them **exactly** as you have them. Do not use "image
sharing" websites, because those websites re-compress your images, so
the image we get is not identical to the one you uploaded!

Wikipedia has an [explanation and comparison of file hosting websites](http://en.wikipedia.org/wiki/File_hosting).

## Uploading Screenshots

Do **not** upload JPG/PNG images which cause problems in RawTherapee to
these websites! They will automatically process your uploads and
although they might look the same, what you upload is not identical to
what we will download. They are good enough for screenshots, nothing
more. They typically accept screenshots up to 1 or 2MB.

- [imgur](http://imgur.com/)
- [TinyPic](http://www.tinypic.com/)

## Pasting Code

If you need to paste code in the forum (e.g. error messages, console
output), it's easier to read if you use a monospace (fixed-width) font.
You can do so using four methods:

- If it's a short line of code, put it between single backticks, like
  this: `` `some code` ``
- If its several lines of code, put three backticks on a line, then
  start a new line and put the code here, and end with three more
  backticks on a new line, like this:

<!-- -->

    ```
    several
    lines
    of
    code
    ```

- Another method for either a single or several lines of code is to
  insert a blank line to separate this from previous text and then to
  indent each line of code with four space characters, like this:

<!-- -->

    Bla bla some normal text in a paragraph. The next line must be blank.

        some code
        notice the four spaces to the left
    You can continue normal text here.

- A final method for several lines of code is to put them between
  `[code] [/code]` tags, like this:

<!-- -->

    [code]
      _________________________
     / You see me because I    \
     \ use monospace font, moo /
      -------------------------
            \   ,__,
             \  (oo)____
                (__)    )\
                   ||--|| *
    [/code]

Alternatively, you can paste code on one of these sites:

- [Paste2](http://paste2.org/)
- [dpaste](https://dpaste.de/)
- [Pastie](http://pastie.org/)
- [PasteBin.com](http://pastebin.com/)
- [PasteBin.ca](http://pastebin.ca/)

Wikipedia has a good [comparison of
pastebins](http://en.wikipedia.org/wiki/Comparison_of_pastebins).

## Creating Screencasts

A screencast is a recording of your desktop activity. This can be used
to create tutorials, and to get others to help you better if you record
whatever you're having a problem with. Sometimes when reporting a bug,
you might be asked to record a screencast so that others can help you
better.

Here is a list of free open source software for Windows for creating
screencasts:

- [ShareX](https://getsharex.com/)
- [Open Broadcaster Software](https://obsproject.com/)
- [SimpleScreenRecorder](http://www.maartenbaert.be/simplescreenrecorder/)

Linux users have several free screencast software options at their
disposal, in no particular order:

- [Open Broadcaster Software](https://obsproject.com/)
- [SimpleScreenRecorder](http://www.maartenbaert.be/simplescreenrecorder/)
- [recordMyDesktop](http://recordmydesktop.sourceforge.net/about.php)
  using one of the frontends - either QT-recordMyDesktop or
  GTK-recordMyDesktop
- [VLC](https://www.videolan.org/vlc/)
- [Eidete](https://launchpad.net/eidete)
- [Kdenlive](http://www.kdenlive.org/)
- [FFmpeg](http://www.ffmpeg.org/) using command-line, see [FFmpeg's
  official wiki guide](https://trac.ffmpeg.org/wiki/Capture/Desktop), or
  use this if your RawTherapee window is 1920x1080 pixels in size (it
  records the whole screen and then downscales to 720p):

`vid="screencast"; \`
`crf="23"; \`
`preset="slower"; \`
`pushd /tmp/ && \`
`ffmpeg -y -f x11grab -show_region 1 -s 1920x1080 -i :0.0+0,0 -an -c:v libx264 -preset ultrafast -qp 0 -threads 0 /dev/shm/${vid}.mp4 && \`
`` ffmpeg -y -ss 00:00:02 -i /dev/shm/${vid}.mp4 -an -c:v libx264 -preset ${preset} -crf ${crf} -s 1920x1080 -s hd720 -sws_flags lanczos -threads 0 ~/${vid}_${crf}crf_${preset}_`date +%F_%H%M%S`.mp4 && \ ``
`popd && ls -l /dev/shm/${vid}*.mp4 && ls -l ~/${vid}*.mp4 && rm --interactive /dev/shm/${vid}*.mp4`

Wikipedia has a good [comparison of screencasting software](http://en.wikipedia.org/wiki/Comparison_of_screencasting_software)
and [list of screencasting software](http://en.wikipedia.org/wiki/List_of_screencasting_software).

Once you create a screencast, use one of the file hosting sites listed
above to upload it and then paste the link to it to the appropriate
thread in the forum.
