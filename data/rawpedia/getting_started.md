---
title: Getting Started
translationKey: getting-started
contributors:
  - DrSlony
  - XavAL
  - Thanatomanic
tags:
  - 'General Information'
weight: 1
---

## Scope

RawTherapee is a powerful cross-platform raw image processing program,
released under the GNU General Public License Version 3. Started in 2005
by Gábor Horváth, it was released as open-source software in 2010 and
has been under development by an international team ever since.
RawTherapee has an extensive set of tools specifically aimed at
processing photographs. It works very well in conjunction with raster
graphics editors, such as Photoshop or GIMP, and a digital asset
manager, such as digiKam.


## Get RawTherapee

Head over to the [Download](/download) page to get stable
builds for production use or unstable development builds for testing.

## Start RawTherapee

When you start RawTherapee you will land in the [File Browser](/file_browser) tab, and it might be empty. You need
to point RawTherapee to where your raw photos are stored. Use the folder
tree browser on the left of the *File Browser* tab to navigate to your
raw photo repository and double-click on the folder to open it. Then
double-click on a raw photo to start editing it.


![](images/rt_setm_fb.png) 

RawTherapee in Single Editor Tab Mode - Vertical Tabs, showing: 
1. Main sections: File Browser (currently opened), Queue, Editor and Preferences. 
2. Panels used for navigating to files and folders. 
3. Thumbnails of the currently opened folder. 
4. Filters to limit the thumbnails shown to only those which match some metadata or state. 
5. Thumbnail zooming and info. 
6. Quick image operations. 
7. Sub-tabs of the File Browser: Filter (currently opened), Inspect (to see a full-sized embedded JPEG preview), Batch Edit (to apply some setting to all selected images) and Fast Export (low quality and bypasses some tools but fast saving - don't use this for typical saving!). 
8. Right-click context menu (you will typically use this to apply some processing profile to all selected files).


## Edit your first image

First, a little background. A raw photo contains a dump of sensor data,
which makes up the bulk of the raw file. This sensor data does not look
like a pretty image, in fact it does not look like anything - it is
"raw" data, ergo the name. It must be "cooked" to look like the image
you saw through the viewfinder. Your camera cooks the raw data into a
pretty image, which it stores as a JPEG file inside the raw file (yes,
even when you're shooting in only "RAW" mode as opposed to "RAW+JPEG"
mode). Due to this fundamental fact of the data being "raw", there is no
one correct way for a raw photo to look - the way your camera makes it
look is not "the right way", nor is it the only way. However, many
photographers would like to use the "camera look" as a starting point
for further adjustments, and RawTherapee makes this possible.

When displaying a raw photo in the [File Browser](/file_browser) which has never been edited in
RawTherapee before, the photo's thumbnail is based on the JPEG image
embedded inside that raw file -- the exact same image you see when
viewing that photo on your camera or in most other software. Once you
open that photo in the [Editor](/editor), RawTherapee creates
a new thumbnail based on the actual raw data. Since creating an image
from raw data requires "cooking" it, and since you have not manually
edited that image yet, RawTherapee uses parameters from the [default processing profile for raw photos](/preferences#default-processing-profile) to process
it. From that moment on, the photo's thumbnail is no longer based on the
embedded JPEG but on the actual raw data. When you make adjustments to
the image in the Editor, the thumbnail is updated to reflect your
changes.

Editing is done in the [Editor](/editor). This is where you
work with RawTherapee to create stunning works of art - or perhaps just
apply first aid to your snapshots. When you open a raw photo in the
Editor for the first time, the [default processing profile for raw photos](/preferences#default-processing-profile) is applied,
which as of RawTherapee 5.4 is set to "Auto-Matched Curve - ISO Low"
(unless you changed it in Preferences), and it automatically adjusts
your raw photo to look like the out-of-camera JPEG. It does so by
analyzing the JPEG image which was created by your camera and is stored
within the raw file, and adjusting the tone curve so as to match it. In
most cases this match is very close to the "camera look". In rare cases
it may fail. See the [Auto-Matched Curve](/auto-matched_curve)
article for more information.

{{< figure src="/images/rt_setm_editor.png" >}}

Take a moment to look around this Editor tab. Notice that there are tabs
within this tab - on the right of screen towards the top. These tabs and
the controls under them are the Toolbox. You probably have the first tab
open and, if you hover your mouse over it, you'll find that it's called
the Exposure tab. Below the choice of tabs are the tools the chosen tab
contains – Exposure, Shadows/Highlights, Tone Mapping etc. If you click
on one of them it will expand so that you can see its contents. Click
again and it will collapse. Right-click on one and that one will expand
while all others will collapse - a time-saving shortcut. To the left of
each tool's label is a power button ![](/images/power-on-small.png)
on /
![](/images/power-off-small.png)
off) which lets you turn it on or off, or in some cases instead of a
power button there is a triangular expander
![](/images/expander-closed-small.png).
Read the [Tools section of the General Comments About Some Toolbox Widgets](/general_comments_about_some_toolbox_widgets#tools)
article for a detailed explanation. Browse through the tabs and panels
until you feel totally overwhelmed by all that's available.

Before you start working on an image, here is some important advice –
**Don't Panic!** You are in no danger of destroying any of your prized
images if you make a mistake. RawTherapee has some features which help
you protect your images:

- RawTherapee does non-destructive editing of your raw files. This means
  that RawTherapee will never, ever change the raw file itself. All
  changes are stored in sidecar files. You can find out more about them
  in the [Sidecar Files - Processing Profiles](/sidecar_files_-_processing_profiles) article.
- When using the Editor, you'll see the
  [History](/editor#history) panel on the left. This panel
  shows a history stack of every change you have made to your image. To
  go back to any step (including when the image was first loaded), just
  click on the relevant line in the History panel.
- Under the History panel you'll see a
  [Snapshots](/editor#snapshots) panel. You can skip it for
  now, but you'll find it handy when you gain experience with
  RawTherapee. This panel stores the state of all the tools as a
  "snapshot". This allows you to easily, for example, tweak your photo
  to a nice and colorful look and take a snapshot, then tweak it again
  to a lovely black-and-white look and take a snapshot, and then compare
  the two just by clicking on either snapshot. (Note: RawTherapee does
  not save snapshots to the PP3 file yet, it will do so in the future.
  If you have three snapshots which you want to retain, you will need to
  click through them and save a PP3 file each time under a unique name).
- As you might expect, Control-z will undo the previous change.

### Basics

1.  Open the raw photo. RawTherapee automatically makes it look like
    your camera's output. If you're happy with the result, you're done.
    Else read on.
2.  Click on the
    ![](/images/color-circles.png)
    Color tab and expanding the [White Balance](/white_balance) tool by right-clicking on it (or
    use the [keyboard shortcut](/keyboard_shortcuts)).
    RawTherapee will start with the white balance used by your camera.
    Most white balance adjustments involve moving the Temperature and
    Tint sliders, or using the
    ![](/images/color-picker.png)
    Spot White-Balance Picker on a colorless (neutral gray) patch.
    Adjust to taste.
3.  Next, fix the exposure by going to the
    ![](/images/exposure.png)
    Exposure
    tab, expanding the [Exposure](/exposure) tool and
    adjusting it to taste. For now, just use the Exposure Compensation
    and Saturation sliders.
4.  If your image is noisy, switch to the
    ![](/images/detail.png)
    Detail tab, zoom
    to 100% either using the
    ![](/images/magnifier-1to1.png)
    button or using the keyboard shortcut, because the effects of the
    tools in this tab are only visible in the zoomed-to-100% preview
    (and of course in the saved image), and enable the [Noise Reduction](/noise_reduction) tool by clicking on the power
    button
    ![](/images/power-on-small.png)
    leaving the settings at their default values for now. RawTherapee
    has automatically removed color (chrominance) noise. Luminance noise
    is removed [manually](/noise_reduction#usage), though
    leave it for now as luminance noise generally lends a pleasing,
    grainy, film-like look. As a general rule, when using noise
    reduction don't use sharpening. Zoom back out to see the whole image
    either using the
    ![](/images/magnifier-fit.png)
    button or using the keyboard shortcut key.
5.  Now you decided you want to fix the
    [geometry](/lens/geometry) and composition of your photo.
    - First make the horizon level, or correct the things which should
      be vertical such as street lamps or building edges. To easily do
      this, press the "s" key on your keyboard (the same as clicking the
      ![](/images/rotate-straighten.png)
      button), and click-and-drag a line along the horizon or along the
      edge of a building over the preview. Your image will rotate
      accordingly and you will automatically be taken into the
      ![](/images/transform.png)
      Transform tab.
    - To crop the photo, press the shortcut key on your keyboard (or use
      the ![](/images/crop.png)
      button) and
      click-and-drag a crop over the preview; you will notice that the
      [Crop](/crop) tool becomes automatically enabled. There
      is no need to "apply" a crop - it takes effect the moment you draw
      it. You can zoom to fit the crop area by using the keyboard
      shortcut, or + if you want to fit the whole image. You may want to
      set the Crop "Guide type" to "none" if it's a problem.
    - Finally, you want to downscale the photo, because who wants to
      upload a 10MB JPEG to your social network. Enable the
      [Resize](/resize) tool and the [Post-Resize Sharpening](/resize#post-resize_sharpening) sub-tool,
      and leave them at the default settings. The resizing effect is
      only applied to the saved image, not to the preview, so you won't
      see any change in the preview as you enable these tools.
6.  You're all set, let's [save](/saving) it straight away.
    Click the ![](/images/save.png)
    Save Current
    Image button (located below the lower left corner of the preview
    area), or use the + keyboard shortcut. Save it as a JPG file using
    default settings (quality at "92", subsampling at "balanced"). These
    are good all-round settings. Choose a folder where you want it saved
    to, and after a few seconds your file will be ready in the folder
    you selected. If you close RawTherapee, the settings you used will
    be stored in a [PP3 sidecar file](/sidecar_files_-_processing_profiles) next to the
    raw file, so that you can re-open the raw photo in the future and
    retain the tool settings you used.

Now that you went through basic photo adjustment and are familiar with
the steps, let's recap the steps but with more advanced details.

### Advanced

Always read each tool's article here on RawPedia before using it, to get
a firm understanding of what it does. The articles explain how the tools
work in RawTherapee, while the general concepts unspecific to
RawTherapee are left to the user to find on Wikipedia or elsewhere.

Be sure to see the [Keyboard Shortcuts](/keyboard_shortcuts).

The order of the tools inside RawTherapee's engine pipeline is
hard-coded, so from that point of view it does not matter when you
enable or disable a tool. However some tools can make a large impact on
other tools, e.g. changing exposure may require you to re-adjust color
toning, and some tools may require plenty of CPU power to calculate the
preview making updates of the preview from then on slow, so it is for
this reason we suggest you stick to this general order of operations:

1.  Start off by making sure that RawTherapee's environment is set up
    correctly, meaning:
    - Make sure that RawTherapee is using your monitor's color profile
      if you use a color-managed workflow. Check Preferences \> Color
      Management. You may also need to load the appropriate calibration
      curves into your graphics card if you built your monitor color
      profile on top of them, though how you do that is outside the
      scope of RawTherapee.
    - Make sure that the Color Management tool is configured correctly.
      Usually the defaults are best. Read the [Color Management](/color_management) and [Color Management addon](/color_management_addon) articles. If instead of
      using the color matrix or DCP or ICC profiles shipped with
      RawTherapee you decide to use an external one, for example a
      self-made DCP or one from Adobe, load it as the first thing you
      do, otherwise you may need to re-adjust some of the color tools.
      Always use an output profile - in most cases the default one,
      RT_sRGB. If you think you're being smart by selecting "No ICM:
      sRGB Output", you're mistaken.
2.  If you want to use a [Flat-Field](/flat_field) and/or
    [Dark-Frame](/dark_frame) image, do so now, to avoid
    re-adjustment.
3.  Now set the correct [White Balance](/white_balance). You
    may fix the exposure first if the image is too dark (or too bright)
    to see white balance changes.
4.  Next, adjust the [Exposure](/exposure), using the Exposure
    Compensation and Black sliders to get the image into the right
    ballpark. Once in the right ballpark, continue with using both tone
    curves. Be sure to read the [Tone Curve section](/exposure#tone_curves) in the Exposure article to
    learn why there are two of them and how best to use them - they are
    a very powerful tool!
5.  In the Basics section above we suggested that you use the
    [Saturation](/exposure#saturation) slider (in the Exposure
    tool). Now that you've learned the basics and are exploring more
    advanced techniques, we suggest you not use the Saturation slider
    anymore, and instead use the more powerful [CC curve](/lab_adjustments#cc-curve) in the [Lab Adjustments](/lab_adjustments) tool, as it gives you finer
    control.
6.  The order of the rest gets fuzzy. Some tools will unavoidably
    influence others. Carry on with the [Lab Adjustments](/lab_adjustments) tool and then the rest of
    the tools in the Exposure tab.
7.  Then use the tools in the
    ![](/images/color-circles.png)
    Color tab.
8.  Then zoom to 100% and use the tools in the
    ![](/images/detail.png)
    Detail tab.
    Generally, don't sharpen if you're using noise reduction.
9.  Finally, zoom out again and use the tools in the
    ![](/images/transform.png)
    Transform tab. The reason you left these for last is that they may
    make the preview image appear a bit blurry, because in order for the
    preview to be responsive, RawTherapee uses that very preview image
    you see at the very resolution you see - small - to show what the
    tools do, and when you rotate or otherwise change the geometry of a
    small image, there is a clear softening. This is not a problem when
    saving as by that point RawTherapee does its processing on the
    full-sized image, which is slow but of high quality.
10. You can edit metadata in the
    ![](/images/metadata.png)
    [Meta](/metadata_copy_mode) tab at any time before saving.
11. Save, either directly ![](/images/save.png)
    when you want to save a single photo, or via the
    ![](/images/gears.png)'
    [Batch Queue](/the_batch_queue) when you want to process many
    photos. See the [Saving Images](/saving_images) article.
