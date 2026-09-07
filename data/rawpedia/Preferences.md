---
title: Preferences
contributors:
  - DrSlony
  - Hombre
  - Agriggio
  - Benitoite
tags:
  - 'General Information'
toc: true
---

You can access the Preferences window by clicking on the Preferences
button ![preferences](preferences.png "preferences.png") which
is either in the bottom-left corner of the RawTherapee window, or the
top-right one, depending on your
[Editor tab mode layout](the_image_editor_tab#editor_tab_modes).

## About

The About button opens a window which contains a splash screen,
technical details of the specific RawTherapee build you're running,
credits, licence and release notes.

Please include these technical details when filing a
[bug report](how_to_write_useful_bug_reports).

## General Tab

### Layout

- Editor Layout


The layout of RawTherapee's user interface can be adapted to suite your
taste and needs, specifically pertaining to whether you would like to
have more than one raw file open simultaneously, and whether you use one
monitor or more. The following modes are available:

- Single Editor Tab Mode
- Single Editor Tab Mode, Vertical Tabs
- Multiple Editor Tabs Mode
- Multiple Editor Tabs In Own Window Mode

<!-- -->


Remember that if you simultaneously open several images each in its own
[Editor](the_image_editor_tab) tab, each tab and instance
will require a significant amount of RAM. Only use multiple Editor tabs
if you have quite a lot of RAM - exactly how much depends on what
resolution your images are, which tools you use, how many other programs
you run in the background, and so forth, however a rule of thumb could
be to not use multiple Editor tabs unless you have more than 8GB of RAM.

<!-- -->


A restart is required for these changes to take effect.

- Position of Curve Copy & Paste Buttons


Curves include adjacent buttons for copying, pasting, opening and saving
the curve, and some include buttons for placing a node on the curve by
picking a sample from the preview and for setting numeric in/out values.
This option lets you decide where these buttons will be positioned
relative to the curve widget.

<!-- -->


A restart is required for these changes to take effect.

- Histogram in Left Panel


Position the main histogram in the left panel above "History", or in the
right tool panel above the tools.

- Show Filmstrip Toolbar


![](rt_setm_fb.png "rt_setm_fb.png")
<figcaption>Rt_setm_fb.png</figcaption>



The Filmstrip is a narrow panel which you can toggle to appear within
the Editor tab. It contains thumbnails of the images in the currently
opened folder, along with the filter and ratings toolbar. It can be
useful to see the Filmstrip while working in the Editor tab, but you can
hide it if you need the extra vertical screen space. Use this option to
show or hide it. Note that you can also toggle its visibility from the
Editor tab by using the "Toggle the visiblity of the Filmstrip's
toolbar" [keyboard shortcut](keyboard_shortcuts).

- Compact Toolbars in File Browser

![](single-row-file-browser-toolbar-off.png "single-row-file-browser-toolbar-off.png")
![](single-row-file-browser-toolbar-on.png "single-row-file-browser-toolbar-on.png")


Enable this option if you have a high resolution screen to merge all the
toolbars at the top of the File Browser tab into one.

<!-- -->



- Hide Vertical Scrollbar


You can hide the vertical scrollbar from the toolbox to save a little
horizontal screen space. Use the mouse scroll-wheel to scroll when the
scrollbar is hidden.

- Tool Collapsed/Expanded State


If you have a few favorite tools which you would like to always see
expanded, you can expand them now, hide the others, then return here to
Preferences, disable "Automatically save tools' collapsed/expanded state
before exiting", click "Save tools' collapsed/expanded state now", and
click "OK" to close the Preferences window and commit your changes.

<!-- -->


Alternatively, if you would instead like RawTherapee to remember which
tools are collapsed and which are expanded at the end of every editing
session, then enable "Automatically save tools' collapsed/expanded state
before exiting".

### Language

Select a language for the user interface. "Use system language" will try
to auto-detect your language based on environment variables. You can
override the auto-detected language by selecting one manually.

If you would like to help by updating one of the translations or
creating a new one, see this post:


<https://discuss.pixls.us/t/localization-how-to-translate-rawtherapee-and-rawpedia/2594>

A restart is required for these changes to take effect.

### Appearance


![](image_editor.png "the image editor tab showing: (1) the preview background, (2) the crop mask, (3) lockable color pickers and (4) buttons which toggle the color of the preview background between black, white and theme-based.")
<figcaption>The Image Editor tab showing: (1) the preview background,
(2) the crop mask, (3) lockable color pickers and (4) buttons which
toggle the color of the preview background between black, white and
theme-based.</figcaption>

- Theme


Choose a theme for the user interface. Although you will see
theme-related changes as soon as you hit the "OK" button, you need to
restart for the changes to take affect correctly.

The way human vision perceives colors depends on various factors, of
particular importance to this paragraph are the properties of the area
which surround the observed region. The way you perceive the colors of a
photograph viewed on a screen depends in part on the colors of the area
surrounding the photograph. You can read more about this in the
[CIECAM02/16](ciecam02) article. In order to mitigate the errors
the user makes while adjusting a photo, RawTherapee ships themes which
use neutral background colors. While all of the themes are based on
shades of grey, the theme which is most suited to avoid affecting human
perception is "TooWaGrey - Average Surround", available from version 5.2
onward.

- Main font, and color picker font


Choose a custom main font, and a font for the Lockable Color Picker in
the [Image Editor](the_image_editor_tab) tab, marked "3" in
the screenshot.

Some users will find the default font size too small or too large due to
their screen resolution and DPI setting. You can fix that by changing
the font size.

- Crop mask color


Adjust the color and transparency of the area outside of a cropped
region, marked "2" in the screenshot. By clicking on the colored button,
a new window appears where you can select a standard color or click on
"Custom" to specify a new color. The vertical axis adjusts hue, while
the horizontal axis adjusts transparency. Partial transparency is useful
as it allows the cropped-off part of the photo to remain somewhat
visible (2), so that you can move the crop around to find the best
composition (hold the **Shift** key and move the crop with the mouse).


![](image_editor_navigator.png "the image editor tab showing: (1) the Navigator panel, (2) the Navigator guide which marks the area currently visible in the main preview when zoomed-in.")
<figcaption>The Image Editor tab showing: (1) the Navigator panel, (2)
the Navigator guide which marks the area currently visible in the main
preview when zoomed-in.</figcaption>


- Navigator guide color


Adjust the color and transparency of the frame (marked "2" in the second
screenshot) visible in the
[Navigator](the_image_editor_tab#navigator) panel (marked
"1") when the main preview is zoomed-in.


![](rt56_hidpi.png "rt56_hidpi.png")
<figcaption>Rt56_hidpi.png</figcaption>


### Clipping Indication

The clipped ![shadow.png](warning-shadows.png "warning-shadows.png") and ![highlight](warning-highlights.png "warning-highlights.png") indicators in the Editor allow you to easily see which areas of the image are too dark or too bright. Highlighted areas are shaded according to the much they transgress the thresholds.

The thresholds for these indicators are defined in Preferences > General.

The clipped shadow indicator will highlight areas where all three channels fall at or below the specified shadow threshold.

The clipped highlight indicator will highlight areas where at least one channel lies at or above the specified highlight threshold. If you want to see only where all channels are clipped, then enable the luminosity preview mode in addition to the clipped highlight indicator.

Clipping is calculated using data which depends on the state of the gamut button ![](Gamut-hist.png "Gamut-hist.png") which you can toggle above the main preview in the Editor tab. When the gamut button is enabled the working profile is used, otherwise the gamma-corrected output profile is used. 

### Pan Rate Amplification

Imagine you are viewing a high resolution images while zoomed-in to
100%. In order to pan the image around the screen you would have to make
multiple mouse movements (or have a very large mouse pad). RawTherapee
saves you from this by using "pan rate amplification" - when set to 5,
RawTherapee multiplies every pixel you pan by 5. If you'd normally move
the cursor 500 pixels in one comfortable mouse movement, you will have
panned 2500 pixels with this option set to 5.

The effect is most visible when you are zoomed in, and least visible
when zoomed out.

When "Remember zoom % and pan offset" is enabled, when you open the next
image RawTherapee will try to show the same area at the same zoom level
as the current image. This only works in "Single Editor Tab Mode" and
when Demosaicing method used for the preview at \<100%
zoom is set to "As
in PP3".

### External Editor


![](rt510_preferences_external_editor.png "rt510_preferences_external_editor.png")
<figcaption>Rt510_preferences_external_editor.png</figcaption>


RawTherapee can send the processed image directly to an external
program, e.g. an image editor, an image viewer or a script. This is done
using the
![<image-editor.png](image-editor.png "image-editor.png")
"[edit current image in external editor](edit_current_image_in_external_editor)" button in the
Editor tab under the main preview - see the [Saving](saving)
article. It is here in Preferences where you can customize which program
the processed image is to be sent to when you click the button.

To get started, click the
![add-small.png](add-small.png "add-small.png") plus button.
This will add a new entry to the list. Then, click Change Application or
Change Executable to select the external editor. The Change Application
button opens a list of installed applications to choose from, while the
Change Executable button opens an executable file selector. Multiple
external editors can be added this way. To remove an editor, select the
entry in the list and click the
![remove-small.png](remove-small.png "remove-small.png")
minus button.

The external editor names and commands can be edited directly by double
clicking the text. You must confirm the edit by hitting enter, else your
edit will be discarded.

Processed images are stored in the location specified by "Output
directory", and are not deleted when RawTherapee is closed. However, the
default location is the operating system's temp directory which is
typically lost when the operating system shuts down.

Suggestions for external editor commands:

- `geeqie --remote` will open images in a single Geeqie window,
  preventing clutter caused by multiple Geeqie windows.
- macOS users may use the "open" command, `open -a "External Program"`,
  for example `open -a "/My stuff/Programs/Turbo Pixels"`. We have
  reports that it is not necessary to write the full path to the program
  even if it does not reside in the standard `/Applications/` folder.

## Image Processing Tab

### Default Processing Profile

Specify which profile RawTherapee is to use when opening a raw or
non-raw photo.

- The default processing profile **for raw photos** as of RawTherapee
  5.4 is 'Auto-Matched Tone Curve' ISO
  Low".
- The default processing profile **for non-raw photos** (such as JPEG,
  TIFF or PNG) is Neutral. The "Neutral" profile
  just loads the photo as it is, without applying any changes.

To have processing profiles you have made yourself appear in the list,
save them to the "*profiles*" sub-folder within the "*config*" folder.
You can find out where it is on the [file paths](file_paths#processing_profiles) page.

The special entry "Dynamic" activates the support for
[Dynamic Processing Profiles](dynamic_processing_profiles).

When you right-click on a thumbnail and select "Processing profile
operations \> Reset to default" RawTherapee will apply whichever
processing profile is selected as default for that image type. If the
default is set to "Dynamic", then RawTherapee will run through the
dynamic profile rules to generate a profile dynamically.

### Custom Processing Profile Builder

Executable (or script) file called when a new initial processing profile
should be generated for an image. The path of the communication file
(\*.ini style, a.k.a. "Keyfile") is added as a command line parameter.
It contains various parameters required for the executable or script to
allow a rules-based processing profile generation.

This feature is very powerful; for example it allows you to set lens
correction parameters or noise reduction based on image properties. It
is called just once on the first edit of the picture, or called manually
from the context menu when right-clicking on a thumbnail in the
[File Browser](the_file_browser_tab) or
[Filmstrip](the_image_editor_tab#the_filmstrip)

<b>Note:</b> You are responsible for using double quotes where necessary
if you're using paths containing spaces.

### Processing Profile Handling

- Processing profile saving location

  Choose whether you want RawTherapee to store the processing profiles
  next to the input file (the default behavior), to a
  [central cache](file_paths#cache), or both.

  It is a good idea to save the processing profiles next to the input
  files, as that lets you easily backup and handle your photos and their
  associated processing profiles.
- Processing profile loading location

  RawTherapee will look for processing profiles alongside the images,
  and in the central cache. If a profile exists in both places and they
  are not identical, this setting allows you to choose which one should
  have the deciding say.

### Directories

Specify the location of your [Dark-Frame](dark-frame),
[Flat-Field](flat-field), [HaldCLUT Film Simulation](film_simulation), Camera/Lens profiles and Lensfun database folders.

### Crop Editing

This section lets you decide which guides are shown when the crop is
_not_ being manipulated. "Original" means the guide type currently
selected, so for example if you would like to see the "Rule of Thirds"
guide while dragging a crop, and to have the guide automatically
disappear once you are done dragging the crop, then set this option to
"Frame" or "None".

Once a crop is in place, RawTherapee can automatically zoom the cropped
area to fit the screen if you enable this option.

## Favorites Tab

This tab allows you to choose which tools appear in the Favorites tab of
the editor toolbox.

### Available Tools

Checkboxes indicate which tools and sub-tools are in the Favorites tab.
Only tools that can be marked as a favorite will have a checkbox.
Toggling a checkbox adds or removes the corresponding tool to/from the
Favorites tab.

### Favorites Panel

Favorite tools are listed here in the same order as they will appear in
the Favorites tab. Selecting a tool from the list and clicking the up or
down button located to the left of the list moves the tool up or down
the list. It is also possible to rearrange tools by dragging them. Tools
can be removed from favorites by selecting them and clicking the remove
button. Use the Ctrl key to select multiple tools or the Shift key to
perform a range selection.

### Options

- <b>Keep favorite tools in original locations</b>: If set, favorite
  tools will appear in both the favorites tab and their original tabs.
  Enabling this option may result in a slight delay when switching tabs.

## Dynamic Profile Rules Tab

Here you can define your custom rules for creating [Dynamic Processing
Profiles](Dynamic_processing_profiles.md).

## File Browser Tab

### Image Directory at Startup

At the top you can define the image directory to use at startup. It
could be the RawTherapee installation directory, the last-visited
directory, the home directory, or a custom directory.

### File Browser / Thumbnail Options

These options determine which information is visible in the thumbnails
and how it should be displayed.

### Context Menu Options

Adjust the grouping of the right-click context menu in the
[File Browser](the_file_browser_tab) (and
[Filmstrip](the_image_editor_tab#the_filmstrip)).

### Parsed Extensions

Choose which files are recognized as images and displayed in the
[File Browser](the_file_browser_tab). All supported extensions are
set by default, except for PNG which is disabled by default.

If a desired extension is missing you can easily add it by clicking the
"Add" ![add-small.png>](add-small.png "add-small.png")
button.

Some users reported that their Parsed Extensions panel is empty. This
could happen after updating from an unspecified older version of
RawTherapee. If your parsed extensions panel is empty, we recommend you
close RawTherapee, then find and delete the
"[options](file_paths#config)" file. The next time you run
RawTherapee you will be using the latest defaults, and your list of
parsed extensions will contain all supported formats.

### Cache Options

To understand this section, first read the
[Cache](file_paths#cache) article. The typical user should
not need to change these defaults.

The maximum thumbnail height decides how large you can make the
thumbnails. Each thumbnail is stored in RawTherapee's
[cache](file_paths#cache) folder and requires disk space, so
keep this in mind if you increase the default size.

The maximum number of cache entries decides how many of these cached
files are kept before the oldest ones are deleted once the limit is
reached.

You can manually clear elements of the cache using the "Clear" buttons.

You could "clear all cached files except for cached processing profiles"
when updating RawTherapee to keep your disk clean and benefit from new
cache-related improvements.

## Color Management Tab

Use the "Directory containing color profiles" button to point
RawTherapee to the folder which contains color profiles.

Standard locations where color profiles are stored:

Windows
    `C:\Windows\system32\spool\drivers\color`

Linux
    `/usr/share/color/icc/`

macOS
    `/library/ColorSync/Profiles/Displays/`

### Monitor

Set the "Default color profile" to the ICC file you generated when
calibrating and profiling your monitor. You can have RawTherapee try to
auto-detect the profile by using the "Use operating system's main
monitor color profile" option.

- In Linux, the
  [`_ICC_PROFILE`](http://www.burtonini.com/computing/x-icc-profiles-spec-latest.html)
  X11 atom is used to automatically find the monitor's ICC profile.
  Since there is only one such atom, and it is used by the "main"
  monitor, automatic detection is also limited to the main monitor,
  though you can copy multiple ICC profiles into the standard location
  and then you will be able to manually select them under the preview in
  the Editor.
  One very simple way of "installing" a monitor profile in Linux so that
  the atom gets set correctly is using
  [DisplayCAL](https://displaycal.net/), via the menu "File \> Install
  profile".

  To see if the X11 atom is set, run this in a console:

      xprop -len 8 -root _ICC_PROFILE

  If the result is "_ICC_PROFILE: no such atom on any window", then the
  atom is not set. If the result is a bunch of numbers, then it is set.
- In Windows, right-click an ICC (or ICM, they're identical) file and
  select "Install profile" in the context menu, or search for "colour
  management" in the Start menu.
- In macOS, monitor profiles on an application level are not supported.
  All displayed colors will be in the [sRGB space](https://en.wikipedia.org/wiki/SRGB), and then, if necessary,
  converted by the native macOS color pipeline to match the screen
  calibration, if any. This means that you cannot choose a monitor color
  profile in macOS. If you have a wide-gamut screen, RawTherapee's
  displayed colors will still be limited to sRGB. This will however not
  affect output, i.e. you can still produce images with colors outside
  the sRGB space. For more information, see:
  <https://discuss.pixls.us/t/wide-gamut-preview-in-macos/2481>

Rendering intents and black point compensation are explained below.

The monitor profile must be of the "device" class in the RGB colorspace.

### Printer (Soft-Proofing)

You can select here the color profile of your own printer or your print
service in order to simulate the rendering of the printed image.

The printer profile must be of the "output" class in either the RGB or
CMYK colorspaces.

See below for Black Point Compensation.

### Rendering Intents

The "[Rendering
intent](https://en.wikipedia.org/wiki/Rendering_intent#Rendering_intent)"
drop-down lets you choose how the ICC profiles are used for translation
between gamuts or color spaces. When in the "Monitor" section, the
"source" is the color space within which lies the image data at the end
of the pipeline before being put into the monitor profile's color space,
and the "destination" is the selected monitor profile's color space.
When in the "Printer (Soft-Proofing)" section, the "source" is the image
data at the end of the pipeline, and the "destination" is the selected
printer profile's color space.

   Relative Colorimetric



Colors from the source which lie outside the gamut of the destination
color space will be shown using the nearest in-gamut color without
affecting other in-gamut colors. The white point will be corrected. This
is the default option and works with all profiles.

   Perceptual



Colors from the source which lies outside the gamut of the destination
color space will be compressed into the destination's gamut at the
expense of also affecting in-gamut colors. How the compression is
performed is up to the gamut mapping contained within the color
profile - it usually involves desaturation, and sometimes even hue
shifts. The perceptual intent only works with LUT profiles which contain
the required gamut mapping tables - most ICC profiles do not, and in
those cases "relative colorimetric" will be silently used instead (this
is standard behavior across most software).

   Absolute Colorimetric



Similar to relative colorimetric, but the white point will not be
corrected. For this reason, it is used when you want to match paper
whiteness to screen. You might want to use it when proofing, but not
otherwise.

### Black Point Compensation

When enabled, the Black Point level of the input image is moved to the
Black Point level of the output image in a color transformation (e.g.
from working profile to display profile). It means that the luminance
channel alone is compressed or expanded to match the output
capabilities. This feature will keep details in the shadows (avoid flat
dark areas) at the expense of less color correctness.

## Batch Edit Tab

Batch editing is making adjustments to more than one image at the same
time. This is done through the [Batch Edit](the_file_browser_tab#batch_adjustments_-_sync) tab in
the File Browser.

The tool panel in the Batch Edit tab looks similar to the tool panel
from the [Image Editor](the_image_editor_tab) tab, but it
uses checkboxes to communicate which tool settings are consistent across
the selected images and which are not. These checkboxes have three
states:

`[ ]` Disabled

`[✓]` Enabled

`[-]` Values differ across selected images.

Batch editing is done by selecting multiple images in the
[File Browser](the_file_browser_tab) (hold the ⇧ Shift or ^ Ctrl key, then click
the images you want to select), then you can edit those images using the
tools in the Batch Edit panel on the right.

The controls (sliders, spinboxes, etc.) in the Batch Edit panel show the
values of the processing parameters for the selected images. These can
be the values of the default processing profile or the values from your
last edit session of those photos.

If an image is currently being edited in the
[Editor](the_image_editor_tab), the editor's values will be
reflected in real time in the Batch Edit panel, and vice versa, so take
care what you're doing.

What happens to the tool values as you manipulate them depends on the
"Behavior" setting in this Batch Edit tab.

#### The "Add" Mode
This mode may also be understood as "relative". Modifying sliders which
are set to the "Add" mode will result in the value of the modification
being added to the existing value. For example, if you select two images
by holding the **Ctrl** modifier key, one image which has an
[Exposure#Exposure_Compensation Exposure Compensation](exposure#exposure_compensation_exposure_compensation)
of -0.5 EV and the other which has +1.0 EV, moving the "Exposure
Compensation" slider up to +0.3 will result in setting a value of -0.2
EV for the first image and +1.3 EV for the second one.

<!-- -->


Using the "Reset" button will move the slider to its default (zero)
position and will then bring back the initial value of that slider for
each selected image.

<!-- -->

#### The "Set" Mode
This mode may also be understood as "absolute". Modifying sliders which
are set to the "Set" mode will result in the value of the modification
being set, irrelevant of what the existing value was. If we use the same
example as before, moving the slider up to +0.3 EV will result in
setting a value of +0.3 EV for both images (one value for all images).

<!-- -->


Using the 'Reset' button will move the slider to its default position
(different for each slider), and will then reset this parameter for each
image.

## Performance Tab

The "Performance" tab is only for people who know what they're doing. It
lets you poke under the hood and tweak some parameters depend on
available RAM and CPU speed.

### Preview Demosaicing Method

The "Demosaicing method used for the preview at \<100% zoom" option sets
which [demosaicing](demosaicing) method is used for the main
preview in the Editor. By default, the same demosaicing method is used
as specified in the Demosaicing section of the Raw tab, but if you are
on a a very slow computer you can save a few hundred milliseconds by
using the "Fast" demosaicing method. The trade-off is that the "Fast"
method has the worst quality, though in most cases the difference is
slight.

### TIFF Read Settings

"Serialize read of TIFF files", enabled by default, can speed up
thumbnail generation when opening for the first time a folder full of
uncompressed TIFF files.

### HaldCLUT Cache

The "Maximum number of cached CLUTs" setting lets you specify how many
last-used HaldCLUT ([Film Simulation](film_simulation))
images are stored in RAM for faster access when switching back and forth
between them in the Editor.

### Inspect

Most raw files contain an embedded JPEG preview image. To show that
image in the [Inspect](file_browser#inspect) tab it needs to
be extracted, which takes a fraction of a second. The "Maximum number of
cached images" setting lets you specify how many of the last-viewed
embedded images are kept in RAM, so that if you view the previous image
in the Inspect tab, RawTherapee will not need to re-extract it, but just
access it from RAM.

The "Image to show" option lets you decide whether to use the embedded
JPEG image or to render one based on the real raw data using the
"Neutral" processing profile. Using the embedded
image is faster than rendering from the real raw data.

### Threads

Splitting calculations and running them as concurrent
[threads](https://en.wikipedia.org/wiki/Thread_(computing)) allows them
to complete faster, however doing so requires more RAM. By default,
RawTherapee decides automatically how many threads to use. You can
override this value here.

Most modern CPUs run two threads per physical core. Find out what CPU
you have and how many cores it has, multiply that number by two, and you
get the maximum number of threads it would make sense to run
simultaneously. Let's call this number *T<sub>max</sub>*. You would not
benefit from running more threads than this - in fact you would likely
suffer a small speed penalty.

Setting this parameter to "0" will let your CPU figure out what
*T<sub>max</sub>* is, and use that. If you experience crashes due to
insufficient RAM, then you can calculate *T<sub>max</sub>* yourself and
use a number lower than that.

## Sounds Tab

The "Sounds" tab lets you set an audible notification when a lengthy
operation ends. It is currently only supported on Windows and Linux.

The "Queue processing done" sound is played after the last
[Queue](the_batch_queue) image finishes processing. The
"Editor processing done" sound is played after a lengthy
in-[editor](the_image_editor_tab) operation that took longer
than the specified number of seconds is complete.

Sounds can be muted either by disabling the "Enabled" checkbox or by
setting fields with sound file references to blank values.

The "Queue" and "Editor processing done" text boxes can either point to
wave (.wav) files, or can specify one of the following values:

Windows:

- SystemAsterisk
- SystemDefault
- SystemExclamation
- SystemExit
- SystemHand
- SystemQuestion
- SystemStart
- SystemWelcome

Linux

- bell
- camera-shutter
- complete
- dialog-warning
- dialog-information
- message
- service-login
- service-logout
- suspend-error
- trash-empty
- possibly the name of any file in
  `/usr/share/sounds/freedesktop/stereo/` without the path or extension.

RawTherapee relies on libcanberra to produce sounds. In case of issues
in Linux, you can trigger a sound to play from a terminal by invoking
the following command (replace "bell" with any of the above):

    canberra-gtk-play -i bell
