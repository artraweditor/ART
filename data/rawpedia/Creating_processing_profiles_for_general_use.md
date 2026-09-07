---
title: Creating processing profiles for general use
contributors:
  - DrSlony
tags:
  - 'Processing Profiles'
toc: true
---

In RawTherapee, we call the sidecar files 'processing profiles'. We
supply a bunch of processing profiles with RT, so that you can start off
with an existing look and modify it to your liking, saving you some
time. One example of such a profile is "Pop 1" - it will make your photo
vibrant and lively, lifting the shadows and bringing out detail.

You can see the whole list of processing profiles in the *[Image Editor](the_image_editor_tab)* tab, if you expand the
*Processing Profiles* list. You can also see them if you right-click on
a thumbnail in the *File Browser* tab and move your mouse over to
"*Processing Profile Operations \> Apply Profile*".

Read the short "[Processing Profile Selector](the_image_editor_tab#processing_profile_selector)"
article to make sure you understand how to make full use of the
selector, partial profiles and the fill mode toggle button.

## Creating Processing Profiles

![](rt_filebrowser_customprofile.jpg "tt_filebrowser_customprofile.jpg")

![](rt_imageeditor_customprofile_cropped.jpg "rt_imageeditor_customprofile_cropped.jpg")

in the [Image Editor](the_image_editor_tab).\]\] You can
create your own processing profiles and have them shown in the
[Processing Profile Selector](the_image_editor_tab#processing_profile_selector)
drop-down list.

- Open a photo you want to create a good starting point profile for.
- You could start off with the 'Neutral' profile, or make changes to any
  of the other profiles that come bundled with RawTherapee. Just apply
  the desired profile to your photo.
- Make the changes you like, remembering that the more specific your
  tweaks, the fewer photos they will work well with because every photo
  is different so what works well for one may not work well for another
  if they differ significantly. For example if your camera has a very
  low-noise sensor and your lens is not very sharp you could probably
  enable [sharpening](sharpening), or conversely if your
  camera has a noisy sensor you may want to apply a certain level of
  [noise reduction](noise_reduction) by default. Maybe you'd
  like to set your name in the [IPTC](iptc_tab) "Author"
  field and have RawTherapee [copy your metadata changes to the saved   files](metadata_copy_mode). You will generally want to
  leave the [white balance](white_balance) set to "Camera"
  since your photos will be taken under various lighting.
- When you are done tweaking, click the *Save Current Profile* icon
  ![<File:save.png>](save.png "File:save.png") in the *Processing
  Profiles* panel. Enter any name; you don't need to specify the
  extension - RawTherapee will add it for you. To have it appear in the
  drop-down list you need to save it to the "profiles" sub-folder in the
  "config" folder - refer to the [File Paths](file_paths)
  page to find out where this folder is on your system.
- Restart RawTherapee, and now your new processing profile will appear
  in the drop-down list under "My profiles".



## Partial Processing Profiles

![thumb](pp3_partial_window.png)

Sometimes, you will want to save only a subset of the parameters
available, e.g. to avoid storing geometric parameters like rotate, crop
and resize. In this case, hold the *Control* key while clicking on the
*Save* button. When you select the output file name and click *Save*, a
window will let you choose which parameters to select. You can then
share these profiles with your friends or in our forum.

Remember that in order for a profile to be universally applicable to all
photos of the same scene and situation (baby portrait photos in this
example), you need to think of all the variations in all of the baby
portrait photos you might want to apply it to. Remember that exposure
will vary between shots, even if you shot the baby in a studio, as the
little one is likely to be crawling around, and even more so if you
upload your profile on the internet for other baby photographers with
different cameras and different lighting gear to use, so instead of
setting a specific exposure, such as +0.60, you should rather turn on
*Auto Levels*. This applies to all other settings - remember to set just
the bare minimum number of options to achieve the effect you want. Leave
the rest untouched, as it is very likely that if you had set those other
options, they will not apply well to other photos. If your processing
profile is meant to make baby face photos look soft and cuddly by a
clever mixture of highlight recovery, auto exposure, Lab and RGB tone
curves, then don't enable noise reduction (as photos might be shot at
different ISO values), don't set custom white balance (as light may have
changed between shots), don’t rotate the photo, and so forth. All these
superfluous parameters are likely to change between photos and not
influence your soft baby look in any way, so turning them on will just
litter your profile. Double-check these things before sharing your
profiles.

## Default Raw/Non-raw Processing Profile

To use your own processing profile as the default for raw or non-raw
photos, set it under "*Preferences \> Image Processing \> Default
Processing Profile*".

If you want to use a dynamic profile, then set "Default processing
profile for (non/)raw" to "(Dynamic)".
