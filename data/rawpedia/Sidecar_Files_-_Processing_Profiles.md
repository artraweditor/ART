---
title: Sidecar Files - Processing Profiles
contributors:
  - DrSlony
  - Thanatomanic
tags:
  - 'Processing Profiles'
toc: true
---

## Introduction

Processing profiles (with a PP3 extension for version 3 or PP2 for the
older version 2) are text files which contain all of the tool settings
which RawTherapee applies to the associated photo. If you are familiar
with other raw processors, you may know their equivalent as "presets".
They are stored alongside their associated photos, which is why they are
also called [sidecar files](https://en.wikipedia.org/wiki/Sidecar_file).

When you open a folder with photos in RawTherapee's File Browser for the
first time, none of the images will have PP3 sidecar files. The
thumbnails shown for images which have no processing profile assigned
(images which have never been opened or edited) are created from the
JPEG image embedded in each raw file. A processing profile is assigned
to the image the moment one of these actions are taken:

- You open the photo for [editing](the_image_editor_tab).
- You apply a processing profile manually, by using the right-click
  context menu in the [File Browser](the_file_browser_tab) or
  [Filmstrip](the_image_editor_tab#the_filmstrip).
- You apply a [dynamic processing profile](dynamic_processing_profiles).

When you open an image for editing, or when a processing profile is
assigned, RawTherapee will convert the real raw data into a viewable
image. In order to do this, there are many settings which need to be set
to *something*, and what these specific values are depends on:

- Your [default processing profile](preferences#default_processing_profile).
- Your [dynamic processing profile rules](dynamic_processing_profiles), if any.
- Or on the processing profile you selected from the right-click context
  menu if you right-clicked on a thumbnail.

## Sources

Processing profiles come from three quite different sources, though they
work in exactly the same way:

- "Bundled profiles".

  RawTherapee comes with a bundle of profiles. Their purpose is to give
  you a good starting point, to demonstrate how the tools can be used
  together. They are the ones you see in the [Processing Profile Selector](the_image_editor_tab#processing_profile_selector)
  drop-down list's "Bundled profiles" section, in the [Image Editor](the_image_editor_tab).
- "My profiles".

  When you make a processing profile which you want to re-use, for
  example one which works well with your camera and your style, you can
  save it so that it also appears in the Processing Profile Selector
  drop-down list, in the "My profiles" section. To have it appear there,
  save it to the "profiles" folder within the "config" folder - see the
  [File Paths](file_paths) article to find it.
- Automatically generated profiles.

  Whenever you edit an image, the tool settings you want applied to that
  image are stored in a processing profile that is particular to that
  image (ranking information, the history panel contents and snapshots
  are not stored in these files yet, see [issue \#473](https://code.google.com/p/rawtherapee/issues/detail?id=473)).

## Saving

As simply viewing the image requires processing, RawTherapee stores the
settings it used to show you the image in a sidecar processing profile.
That processing profile also stores all the tool tweaks you made in the
Editor tab.

The processing profile is written to disk:

- When you apply a processing profile manually or using a dynamic
  profile.
- When you close the current image (the Editor tab) if using
  [Multiple Editor Tabs Mode](the_image_editor_tab#editor_tab_modes)
  (METM).
- When you close the current image by opening a different image if using
  [Single Editor Tab Mode](the_image_editor_tab#editor_tab_modes) (SETM).
- When you close the current image by closing RawTherapee.
- When you manually save the processing profile using the [Processing Profile Selector](the_image_editor_tab#processing_profile_selector)
  panel in the Editor tab.
- When you use the "force saving current settings to the processing
  profile" [keyboard shortcut](keyboard_shortcuts) from the
  Editor tab.

If a photo has an associated processing profile, a green check mark will
appear over its thumbnail.

If you have a photo opened in an Editor tab and you make changes to it
from the File Browser, the changes are reflected immediately in the
Editor tab.

## Storage

Where the processing profile is stored can be configured in [Preferences \> Processing Profile Handling](preferences#processing_profile_handling).

By default, the processing profile for an image is stored alongside the
input image (if you open `kitty.raw`, a new file `kitty.raw.pp3` will be
created next to it), but they can also be stored in a [central cache](file_paths). You can choose whether RawTherapee should
use the cache, write the processing profile alongside the image, or
both, from "*Preferences \> Image Processing*". We suggest you store
these files alongside your input image files so that if you decide to
move the images you can move the processing profiles easily along with
them.

When saving an image you have the option of ticking the "Save processing
parameters with image" checkbox. If it is ticked, and if you are working
on `kitty.raw` and saving to a JPG file, then the processing profile
used to develop that image will be stored in a file called
`kitty.jpg.out.pp3`. The ".out" part if there so that conflicts do not
occur if you are working on a non-raw file.

## Defaults

The default processing profile used when opening **non-raw** images is
called "Neutral". This profile has all tool
settings at their neutral values, so they have no effect. Since non-raw
images usually have already been processed and are ready for viewing,
having RawTherapee not introduce any tweaks by default is the desirable
behavior.

The default profile for **raw** photos is called "[Auto-Matched Curve](auto-matched_curve)" (from RawTherapee 5.4 onward).
This profile makes your raw image look like the out-of-camera JPEG,
which is usually a desirable starting point.

Furthermore, most tools in the Editor tab have a reset button.

- Clicking the reset button resets the tool to its hard-coded neutral
  value, usually zero.
- Ctrl+clicking the reset button resets the tool to whatever value it
  had when you opened the image, i.e. the way it was if you rewind the
  history stack to the top.

## Partial Processing Profiles and Fill Modes


![](processing-profiles-selector.png  "processing-profiles-selector.png")
<figcaption>Processing-profiles-selector.png</figcaption>


Processing profile storage (saving to a file or copying to memory) and
application (loading from a file or pasting from memory) can be partial,
where only a subset of the parameters are involved, or full, involving
all parameters. These operations are performed using the buttons in the
[Processing Profile Selector](editor#processing_profile_selector) located in the
top-right corner of the [Editor](editor) tab. Clicking these
buttons invokes an operation on the full profile, while +clicking
invokes an operation only on a subset of parameters. When a partial
operation is invoked, a windows pops up letting you choose which
parameters to include. This feature, for instance, allows you to copy
only the white balance and noise reduction parameters from one image to
another, while omitting all other parameters.

The processing profile fill mode allows you to decide what happens when
you apply (load or paste) a partial processing profile.

- [image:Profile-filled.png](/images/profile-filled.png) "Fill"
  mode takes missing values from RawTherapee's hard-coded defaults. For
  instance, if you apply a partial profile containing only sharpening
  settings, all of the remaining tools will be set to their default
  parameters, overwriting any edits you have made.
- [image:Profile-partial.png](/images/profile-partial.png)
  "Preserve" mode applies only those parameters that are available in
  the partial profile and leaves missing values unchanged. Using the
  previous example, only the sharpening settings would be applied and
  all other parameters would be left intact.

Most of the profiles that come bundled with RawTherapee are partial
profiles. The [File Paths](file_paths) article explains where
the processing profiles shown in the drop-down list can be found on your
file system.

## Creating Your Own Processing Profiles

Using certain tools in certain ways may make your processing profile
only usable with that specific image. For example if you set a white
balance, cropping and rotation, you won't get good results if you apply
that profile to an image taken under different lighting with the camera
rotated any other way. See the article
[Creating processing profiles for general use](creating_processing_profiles_for_general_use)
for advice on how to make processing profiles which can be used on many
images.

## Compatibility

Processing profiles evolve from one version of RawTherapee to the next.
We strive to ensure backward compatibility (e.g. a profile created in
5.3 and opened in 5.4 should look the same), but this is not always
possible.

Processing profiles can gain new parameters or lose ones which became
obsolete. Tool behavior can also evolve, wherein default values change
or in extreme cases the meaning of a value is interpreted differently;
an example of this is the noise reduction tool, where a luminance noise
reduction value of 10 in RawTherapee-3.0 would lead to a different
result in RawTherapee-4.0.10 as the whole noise reduction engine has
been greatly improved.

Consolidating processing profiles into a cache allows one to store
isolated copies of the processing profiles per specific version of
RawTherapee. In such a case, the cache can be used to re-process photos
in order to get the same output as originally intended (but e.g. with a
new size or output color space) using the same version of RawTherapee in
which the image was originally edited. Whether this is desirable is
debatable. Consider that you want to squeeze as much out of your raw
files as possible. If two years later you want to go back to an old raw
file, perhaps getting the same result as you did two years ago is not
the best idea, because RawTherapee's capabilities would have greatly
improved in that time, you may have acquired a better monitor, and your
taste and skill would also have evolved. Nevertheless, by backing up the
whole cache folder, when installing a new version of RawTherapee you
retain the option of going back to an older version of RawTherapee in
order to get the exact same result.

The [File Paths](file_paths) article describes where you can
find the "*cache*" and "*config*" folders on your system.

When releasing a major new version of RawTherapee, it may happen that we
use a new suffix for the "*cache*" and "*config*" folders. This means
that the new version of RawTherapee will not see your old configuration
or processing profiles. Though this sounds undesirable, there are good
reasons we may (rarely) choose to do that.

- Backward compatibility. There may be changes in behavior between old
  and new versions of a specific tool. For instance, the effects of the
  Auto Levels tool have changed (for the better) between versions 4.0.11
  and 4.0.12, so if your old processing profiles had it enabled, the
  results in 4.0.12 will be a little different and may require tuning
  your old profiles.
- Some users have not checked "[Preferences](preferences)" in
  a long time, and their program is tuned for what worked best long ago,
  not for what works best now. Our defaults are good defaults, we keep
  them up to date to make RawTherapee look and function well
  out-of-the-box, so sometimes having RawTherapee start with fresh
  defaults is a good thing, and it will motivate users to look into
  "[Preferences](preferences)" again.
- Some users have never looked inside
  "[Preferences](preferences)" in the first place, and are
  unaware of some of the features that can be unlocked there. As above,
  fresh defaults will activate these things.
- Some old cache and config files can cause RawTherapee to crash. While
  we patch the specific cases made known to us, it is safe to assume
  there will always be cases unknown to us which will still cause
  instability. Starting with clean cache and config folders mitigates
  this problem.
