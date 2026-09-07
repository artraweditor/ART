---
title: Batch Adjustments - Sync
contributors:
  - DrSlony
tags:
  - 'General Information'
toc: true
---

RawTherapee lets you batch-adjust, or sync, the processing settings in
many photos at the same time in generally two ways. It lets you copy and
paste a [processing profile](/sidecar_files_-_processing_profiles) (a collection
of tool settings), in parts or in full, to any number of images. It also
lets you select any number of images and adjust any tool in all of them
at once (sync), and it lets you do this in two ways. Let's take a closer
look.

Both ways involve making a selection of photos you want the processing
profile or adjustments applied to. Selections are made using standard
key combinations: Shift+click to select a range, Ctrl+click to select
individual images, or Ctrl+A to select everything. Both ways are
performed from the [File Browser](/the_file_browser_tab) tab.
The "copy & paste" method can also be done via the
[Filmstrip](/editor/#the-filmstrip).

## Copy & Paste
Copying and pasting a processing profile to a
selection of images is a very common task. Assume you took a series of
photos - for example studio shots, wedding portraits or focus-bracketed
macro photos. All images in each series are going to be very similar;
they will probably use the same lens, the same ISO, the same white
balance, and end up being used for the same purpose. This means that
they will all probably require the same processing settings - the same
noise reduction, the same sharpening and lens distortion correction, and
so forth.

To process the lot, what you would usually do is open any one image from
the whole series in the [Editor](editor) tab
and tweak it to your liking. Once you have finished tweaking it, you
will apply this image's processing profile to all other images in the
same series. To do that, go to the [File Browser](file_browser) tab, right-click on this photo
and select "*Processing Profile Operations \> Copy*", then select the
images you want to apply this profile to, right-click on any one of them
(it doesn't matter which) and select "*Processing Profile Operations \>
Paste*". In one quick operation you have replicated the same tool
settings in the whole series of images.

Additionally, RawTherapee lets you apply only a part of the copied
processing profile, for example only the "Resizing" tool. To do this,
use the "*Processing Profile Operations \> Paste Partial*" option
instead of the "Paste" option.

## Sync
RawTherapee lets you instantly apply tool adjustments
to a selection of images. Similar functionality in other software is
called "sync". This method is useful for when you don't need to see an
accurate preview of your changes, for example when you only want to
enable the "Resizing" tool in a selection of photos, because when
working in the File Browser tab your only preview are the small and
inaccurate thumbnails. This method can only be performed from the File
Browser tab because you need access to that tab's batch tools (the panel
on the right).

When you're in the File Browser tab, select the images you want to
batch-adjust (sync), then use the tool panel on the right to make
adjustments. Your tweaks can either replace the existing ones ("Set"
mode), or be added to them ("Add" mode). For example if you select two
photos, one of which has previously been tweaked with +1EV Exposure
Compensation and one which has not, and you set Exposure Compensation to
+0.6EV, then the previously-tweaked photo would end up having +1.6EV
Exposure Compensation in "Add" mode and just +0.6EV in "Set" mode. The
photo which was not previously tweaked would have +0.6EV in both modes.
You can decide which tools should work in which mode from the
[Batch Processing tab in Preferences](/preferences/#batch-edit-tab).
