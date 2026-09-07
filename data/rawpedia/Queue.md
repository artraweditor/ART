---
title: Queue
contributors:
  - DrSlony
  - Thanatomanic
tags:
  - 'General Information'
toc: true
---

## Introduction

[Saving images](saving_images) from RawTherapee can be done
in several ways, the two most common of which are either saving the
image immediately ![file:save.png](save.png "save.png") from the
Editor tab, or adding it to the batch processing queue
![gears.png](gears.png "gears.png") which resides in the
Queue tab.

Using the "Save immediately" feature will put your CPU immediately to
work, and as a result, opening and tweaking other images in the Editor
will be somewhat slow while the image is being saved. The queue
mechanism allows you to put edited images which are ready to be saved to
a virtual queue which you can start processing at a later time. Adding
them to the queue is instant, so you can continue editing other images
and making the most of your CPU for editing. Once you are done editing
and putting images to the queue, you can flip the large "On" switch and
go off to brew yourself a coffee while RawTherapee grinds away at all
the images in the queue.

The queue is persistent - you can exit RawTherapee and restart it later;
the queued images will still be there. The queue can even survive a
crash.

## Adding Images to the Queue

There are several ways of adding an image to the queue:

1.  When you are done tweaking an image in the Editor, click the "Put
    current image to processing queue" button
    ![<File:Gears.png>](/images/Gears.png "File:Gears.png").
2.  Also in the Editor tab, click the "Save current image" button
    ![<File:Save.png>](/images/Save.png "File:Save.png") and select "Put to the
    head/tail of the processing queue".
3.  Right-click on a thumbnail in the [File Browser](file_browser) or the
    [Filmstrip](editor#the_filmstrip) and select "Put to
    queue".

Regardless which method you use, when you go to the *Queue* tab you will
see your photos lined up, ready for processing (if you had the queue set
to "Auto-start", it may have finished processing before you viewed it).

## Queue Settings

![](save_window.png "save_window.png")

Save-window.png


The Queue has several settings, such as the output file format and
destination. These settings take effect in all cases except when you use
the "Save current image" button
![<File:Save.png>](/images/Save.png "File:Save.png"), select "Put to the
head/tail of the processing queue" and enable the "Force saving options"
checkbox. In this case, the settings seen in the "Save" window will be
used, and the ones from the Queue tab ignored. In all other cases, the
settings from the Queue tab will be used.

The settings speak for themselves. Two things worth pointing out:

1.  "Save processing parameters with image" will save a sidecar file
    alongside the output file, with the same filename as the output
    image but with a ".pp3" extension. This is useful when you want to
    save multiple copies of the same photo, each one tweaked a bit
    differently.
2.  The destination folder can be set by selecting "Save to folder", but
    if you need to dynamically customize the destination folder and
    filename then select "Use template" instead. Hover your mouse over
    the *Use template* input box and a tooltip with an explanation will
    pop up:

`Specify the output location based on the source photo's location, rank, trash status or position in the queue.`

`Using the following pathname as an example:`
<b>`/home/tom/photos/2010-10-31/photo1.raw`</b>
`the meaning of the formatting strings follows:`
<b>`%d4`</b>` = `<i>`home`</i>
<b>`%d3`</b>` = `<i>`tom`</i>
<b>`%d2`</b>` = `<i>`photos`</i>
<b>`%d1`</b>` = `<i>`2010-10-31`</i>
<b>`%f`</b>` = `<i>`photo1`</i>
<b>`%p1`</b>` = `<i>`/home/tom/photos/2010-10-31/`</i>
<b>`%p2`</b>` = `<i>`/home/tom/photos/`</i>
<b>`%p3`</b>` = `<i>`/home/tom/`</i>
<b>`%p4`</b>` = `<i>`/home/`</i>

<b>`%r`</b>` will be replaced by the photo's rank. If the photo is unranked, '`<i>`0`</i>`' is used. If the photo is in the trash, '`<i>`x`</i>`' is used.`

<b>`%s1`</b>`, ..., `<b>`%s9`</b>` will be replaced by the photo's initial position in the queue at the time the queue is started. The number specifies the padding, e.g. `<b>`%s3`</b>` results in '`<i>`001`</i>`'.`

`If you want to save the output image alongside the source image, write:`
<b>`%p1/%f`</b>

`If you want to save the output image in a folder named '`<i>`converted`</i>`' located in the source photo's folder, write:`
<b>`%p1/converted/%f`</b>

`If you want to save the output image in`
`'`<i>`/home/tom/photos/converted/2010-10-31`</i>`', write:`
<b>`%p2/converted/%d1/%f`</b>

## Running the Queue

In the top-left corner of the Queue tab you will find an "On/Off"
switch, and an "Auto-start" checkbox.

1.  If "Auto-start" is enabled, processing will start as soon as an
    image is sent to the queue. Usually you will not want this, as this
    will use your CPU for processing the photos in the queue leaving
    very little CPU time for allowing RawTherapee to be responsive while
    you tweak other photos.
2.  If "Auto-start" is not checked, you will have to activate the queue
    manually by hitting the "On/Off" switch.

You can pause the queue by hitting the "On/Off" switch - RawTherapee
will first finish processing the current photo.

## Clearing the Queue

You can remove a specific image from the queue by clicking the small
"Cancel job"
![<File:Cancel-small.png>](/images/Cancel-small.png "File:Cancel-small.png")
button in the corner of each thumbnail.

You can clear the whole queue right-clicking on a thumbnail and clicking
"Select all" and "Cancel job", or by using the  + keyboard shortcut to
select all thumbnails and then hitting the key on the keyboard.
