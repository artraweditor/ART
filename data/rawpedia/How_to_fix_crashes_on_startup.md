---
title: How to fix crashes on startup
contributors:
  - Ingo
  - DrSlony
  - XavAL
tags:
  - 'Troubleshooting'
---

RawTherapee might crash immediately after starting for several reasons,
the common ones being that it tries to open a file which looks like an
image but is not an image, it tries to open a corrupt or unsupported
image or it tries to load a problematic processing profile (PP3, they
store all the tweaks you make in RawTherapee, one PP3 per photo). Even a
normal photo could be problematic if it is corrupt or if it triggers a
bug in RawTherapee or some aspect of it is unsupported, such as unusual
character encoding of the metadata, multiple layers in an image, has
four channels, etc.

RawTherapee supports images with either one channel (grayscale) or three
channels (RGB or CMY). If you try opening a folder which contains images
with four color channels (e.g. CMYK) RawTherapee shows an error popup.
Move these problematic files out from the startup folder, elsewhere, as
described below. Of course it might be a valid RGB photo or processing
profile which crashes RawTherapee, but first check for images with an
unsupported number of channels. The steps below will guide you.

When dealing with bugs always use the latest version of RawTherapee you
can find, preferably a development version, because it is likely that
the bug was already fixed. You can find the latest stable and
development versions on our [website](http://rawtherapee.com/downloads/)
and in the
[forum](https://discuss.pixls.us/t/download-rawtherapee-builds-windows-macos-linux-appimage-other/2924),
or in your package manager if you use Linux.

To find the cause of the problem we will begin with the simplest steps,
and escalate if the simple steps do not help:

1.  First, try having RawTherapee use an empty startup folder:
    1.  Create a new empty folder somewhere on your disk,

        Windows: `C:\\test`

        Linux: `/home/you/test`
    2.  Find the "options" file as described in the [File paths](file_paths) page.
    3.  Open the "options" file in a text editor,

        find the `StartupDirectory` line and set it to
        `StartupDirectory=last`

        find the `StartupPath` line and set it to point to the empty
        folder you just created (it must be an **existing**, **empty**
        folder, and you have to type the whole, absolute path, no
        shortcuts, with double back-slashes if you use Windows) e.g.

        Windows: `StartupPath=C:\\test`

        Linux: `StartupPath=/home/you/test`
    4.  Now try starting RawTherapee again. If it works, then you know
        that one of the photos, PP3 files or other files in the original
        `StartupPath` is faulty, so skip step 2 and jump straight to
        step 3. However, if RawTherapee still crashes right after
        starting, proceed to the next step.
2.  Delete the `batch` folder:
    1.  Find the "batch" folder as described in the [File paths](file_paths) page, zip all of the files it
        contains if there are any, and then delete the folder.
    2.  Try starting RawTherapee again. If it works, then you know that
        one of the processing profiles of the photos you sent to the
        Queue is faulty. Include the zip archive in your [bug report](how_to_write_useful_bug_reports). If it still
        crashes, proceed to the next step.
3.  Delete the `cache` folder:
    1.  Find the "cache" folder as described in the [File paths](file_paths) page.
    2.  Delete the cache folder or rename it if you don't want to lose
        the contents (renaming it from "cache" to "cache2" is enough,
        RawTherapee will only look for "cache" and not find it). Note
        that by default RawTherapee stores processing profiles alongside
        the images they apply to so it is safe to delete the cache, but
        if you set RawTherapee to only store processing profiles in the
        cache and nowhere else then by deleting the cache you would lose
        your tweaks - in that case you may prefer renaming it instead of
        deleting it. Regardless of the PP3 setting, you will not lose
        any photos.
    3.  Try starting RawTherapee again. If it works then you know that
        one of the processing profiles files in the cache is faulty.
        Finding which one requires considerable effort. If you really
        want to, and you haven't deleted the cache folder, then follow
        the "Nail it down" section. If RawTherapee still crashes on
        startup at the same point as before then the problem is not a
        faulty photo or PP3 file, the problem lies elsewhere, outside
        the scope of this guide.
4.  Nail it down
    1.  A stack backtrace would most likely tell us everything we need
        to know, including the name of the faulty file and/or where to
        find the problem in the code. See the
        [guide to stack-backtraces](how_to_write_useful_bug_reports#when_rawtherapee_crashes_-_an_introduction_to_stack_backtraces).
        The instructions may seem complicated but they are simple to
        follow and it would be of tremendous help if you did follow
        them. Sending us a stack backtrace in most cases is enough.
        However in some rare cases we may need you to find and send us
        the specific file which is causing the problem - if that's the
        case, read on.
    2.  You've established that a processing profile, a photo or some
        other file is to blame for the crash. The previous steps should
        reveal where this file is. You could just zip that whole folder
        and send us the zip archive, that would be easy for you. Sending
        the faulty file(s) to us is important so that we can analyze
        them and develop techniques for dealing with such files in the
        future. But if you send us a zip archive with a thousand files
        and the problem is caused by a single file, it would be very
        difficult for us to find the specific file - in that case it
        could be easier for you to find it. To make the procedure clear,
        let us assume three things:

    - That the folder which contains the file which crashes RawTherapee
      is `C:\photos\paris` and that your options file had
      `StartupPath=C:\\photos\\paris`
    - That you changed `StartupPath=C:\\photos\\paris` in the "options"
      file to the existing and empty `StartupPath=C:\\test` folder.
    - That there are 100 photos in the faulty folder, `001.raw` -
      `100.raw`


    Let's find the faulty file:

    1.  This is grunt work. This guide assumes raw files as an example,
        but in your case it might be the processing profiles, downloaded
        images, printer ICC profiles, or other files. What you will now
        do is to keep halving the pool of possibilities until we find
        the problem. This is the fastest way.
    2.  If RawTherapee is running, close it.
    3.  Move half of the files (`001.raw` - `050.raw`) from this
        problematic folder (`C:\photos\paris`) back into a folder that
        RawTherapee reads (`C:\test`)
    4.  Start RawTherapee.
    5.  If it crashed, go to the next step. If it did not crash, then go
        back to step 4.3, but move the other half (`051.raw` -
        `100.raw`).
    6.  Move half of the files you just moved (`001.raw` - `025.raw` or
        `051.raw` - `075.raw`) out from `C:\test` and back into the
        folder which RawTherapee doesn't read (`C:\photos\paris`).
    7.  Jump back to step 4.4. Repeat until you find the faulty file.
    8.  Zip this faulty file. Even if it's a plain-text PP3 file, you
        must zip it because often websites change uploaded files in ways
        you don't see, and we don't want a website touching this faulty
        file in any way as in doing so it might remove or obscure the
        problem. Zip files are safe containers. Upload the zip file
        using [FileBin](http://filebin.net/) and send us the link in
        your [bug report](how_to_write_useful_bug_reports) or
        in the [forum](https://discuss.pixls.us/c/software/rawtherapee)
        (preferably a bug report), along with the stack backtrace and
        other required information described in the guide
        "[How to write useful bug reports](how_to_write_useful_bug_reports)".
