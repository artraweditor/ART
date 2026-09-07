---
title: Saving Images
contributors:
  - DrSlony
tags:
  - 'General Information'
toc: true
---

Your original raw file will never be altered by RawTherapee.

There are several ways of saving an image from the
[Image Editor](the_image_editor_tab) tab:

- ![<File:Save.png>](/images/Save.png "File:Save.png") Save immediately from the
  Editor tab,
- ![<File:Gears.png>](/images/Gears.png "File:Gears.png") Save via the
  [Queue](queue),
- ![<File:Palette-brush.png>](/images/Palette-brush.png "File:Palette-brush.png")
  [Edit Current Image in External Editor](edit_current_image_in_external_editor) (described
  in its own article).

## Save Immediately

In the Editor tab, if you click on the little hard disk icon
![<File:Save.png>](/images/Save.png "File:Save.png") at the bottom-left of the
preview image, or hit the  + shortcut, you can "*Save immediately*".
This works as a standard "Save As" dialog. You can select the name and
location for the output file (RawTherapee will automatically add the
extension based on the chosen format), choose the output file format and
bit depth, set the compression level, choose whether you want the
processing profile saved alongside the output image, etc. The last
option lets you choose whether you want to "*Save immediately*" or "*Put
to the head/tail of the processing queue*". If you choose to "*Save
immediately*", RawTherapee will be busy saving your photo as soon as you
click "*OK*", so it will be less responsive to any adjustments you might
try doing while it's busy saving, and it will also take longer to open
other images as long as it's busy saving this one. For this reason it is
recommended that you use the queue if you want to tweak other photos
right away.

The Save window will by default open the location you saved to the last
time you used this window. For your convenience, a shortcut to the
folder containing the source image is automatically added to the
bookmarks panel on the left side of the Save window. If you want to save
the image to the source folder, clicking on this bookmark will save you
having to manually click through to navigate to that location.

A shortcut for the *OK* button is  + .

## Put to the Head / Tail of the Processing Queue

If you click on the gears icon
[<file:gears.png>](/images/gears.png) or in the "*Save*" window
choose "*Put to head or tail of the processing queue*", your image will
be kept in a queue of files to be processed, so RawTherapee can make the
most of your CPU and be responsive while you tweak your photos. Once
you're done tweaking and adding them to the queue, you can have
RawTherapee start processing the queue while you go and enjoy some tea.
The benefit of putting it to the queue using the "*Save*" window is that
you can individually change the file format, name and destination of
each image, whereas putting images to the queue without using the
"*Save*" window will use the settings from the
"[Queue](queue)" tab.

## Naming

If your original raw file was called `photo_1000.raw`, the default
processed file name will be `photo_1000.jpg` (or `.tif` or `.png`).
There is an option in the "*Save current image*" window: "*Automatically
add a suffix if the file already exists*". When checked, you can make
different versions of one raw, which will be saved as `photo_1000.jpg`,
`photo_1000-1.jpg`, `photo_1000-2.jpg`, etc. The same applies when you
send different versions of the same image to the
[Queue](queue).
