---
title: File Paths
contributors:
  - DrSlony
  - Benitoite
tags:
  - 'General Information'
toc: true
---

RawTherapee makes use of a "cache" folder to store temporary files which
are safe to delete, and a "config" folder which stores your RawTherapee
settings, custom processing profiles and other user-editable files.
These folders reside in a special place, described below, and have a
name that begins with the word "RawTherapee" optionally followed by a
suffix. This suffix is set by the person who made the build of
RawTherapee you're using. Some examples of what it can look like:

- RawTherapee
- RawTherapee**4.2**
- RawTherapee**5**
- RawTherapee**5-dev**
- RawTherapee**_test**
- And other possibilities always beginning with "RawTherapee"

The first part, "RawTherapee", is hard-coded. The second part, the
suffix, is up to the person who made the build. It might be specific,
like "5.0-gtk2-123-g87654321", it could be general, like "5", it could
be anything else, like "_test", or it could be not set. We recommend
that RawTherapee stable releases not use a suffix at all, while all
development versions use "5-dev" - hopefully the person who made the
build you're using took this into account.

## Config

The RawTherapee config folder contains:

- the "options" file, which contains all of your settings from
  [Preferences](preferences),
- the "batch" folder, which stores temporary [processing profiles](sidecar_files_-_processing_profiles) of the
  photos you sent to the [Queue](queue),
- the user-editable
  [camconst.json](adding_support_for_new_raw_formats) file,
  where you can define details of how a specific raw format is to be
  treated (this overrides the values from the system camconst.json
  file),
- the [dynamic profile](dynamic_processing_profiles) rules,
- and the "profiles" folder where you can save your custom [processing profiles](sidecar_files_-_processing_profiles) to if you
  want them to appear in RawTherapee's drop-down list.

You could include this folder in your backups so that you can regain all
of your settings and custom processing profiles if you install
RawTherapee on a new system.

Default locations for the RawTherapee config folder (look for the
"RawTherapee\*" prefix as described above):

|            |                                                                                         |
|------------|-----------------------------------------------------------------------------------------|
| Windows XP | `%USERPROFILE%\Local Settings\Application Data\`                                        |
| Windows 7+ | `%LOCALAPPDATA%`                                                                        |
| Linux      | `~/.config/`                                                                            |
| macOS      | `~/Library/Containers/RawTherapee/Data/Library/Application Support/RawTherapee/config/` |
{.table .table-striped .table-sm}


Under the Finder's 'Go' menu click 'Go to Folder' (shortcut
Command+Shift+g), you can then type/paste any path you want to navigate
to, even if it's hidden.

## Cache

The RawTherapee cache folder contains sets of cached items, where each
set consists of:

- a thumbnail,
- metadata,
- a sidecar file,
- and optionally an embedded profile.

By default, RawTherapee keeps up to 20 000 cached sets. Keep an eye on
the "cache" folder as over time it may grow considerably in size! This
is mostly due to the cached thumbnails which are stored in the "images"
sub-folder. Deleting the "images" sub-folder is safe, you will not lose
any image settings, RawTherapee will just have to regenerate the
thumbnails.

Default locations for the RawTherapee cache folder (look for the
"RawTherapee\*" prefix as described above):

|            |                                                                                         |
|------------|-----------------------------------------------------------------------------------------|
| Windows XP | `%USERPROFILE%\Local Settings\Application Data\`                                        |
| Windows 7+ | `%LOCALAPPDATA%`                                                                        |
| Linux      | `~/.cache/`                                                                             |
| macOS      | `~/Library/Containers/RawTherapee/Data/Library/Application Support/RawTherapee/cache/` |
{.table .table-striped .table-sm}


Under the Finder's 'Go' menu click 'Go to Folder' (shortcut
Command+Shift+g), you can then type/paste any path you want to navigate
to, even if it's hidden.

A cached set used to include a 32kB histogram file, but as of
RawTherapee 5.5 the need for storing the histogram was eliminated.

## Custom config and cache folders

You can have RawTherapee use a custom config folder by setting the
`RT_SETTINGS` environment variable to an **absolute** path which you
have read and write access to, and likewise you can use a custom cache
folder by setting the `RT_CACHE` environment variable. How you do that
depends on your operating system, so just search on the internet for
"how to set environment variables in *<your operating system>*".

Some examples:

Windows
Variable name: `RT_SETTINGS`, value: `%LOCALAPPDATA%\rawtherapee\5.7`

Variable name: `RT_CACHE`, value: `Z:\rawtherapee\cache`

Linux and macOS
`RT_SETTINGS=/home/bob/.config/rawtherapee/5.7`

`RT_CACHE=/home/bob/junk/rtcache`

## Processing Profiles

If you create your own [processing profiles](sidecar_files_-_processing_profiles), to have them
appear in RawTherapee's "Processing Profiles" list you should save them
to the "profiles" folder which you will find inside the "config" folder
as described above.

## Temporary Folder

The "[Edit Current Image in External Editor](edit_current_image_in_external_editor)" tool stores
intermediate image files in the folder specified in [Preferences \> External Editor \> Output Directory](preferences#external-editor). By default this is
the operating system's default temp folder. RawTherapee will use a
subdirectory with the name format `rawtherapee-`<username>, e.g.
`rawtherapee-Lawrence37`, and permissions are set to user-only
read/write access. If this subdirectory already exists but has the wrong
permissions, a new directory will be created with the correct
permissions and with the name format `rawtherapee-`<username>`-xxxxxx`
where `xxxxxx` is a random sequence of 6 characters, e.g.
`rawtherapee-Lawrence37-abc123`.

Windows
The operating system's default temp folder is the one stored in the
`$TEMP` environment variable, which is usually `%LOCALAPPDATA%/Temp`

If you do not have the `$TEMP` environment variable set, `C:\` is used.

Linux and macOS
The operating system's default temp folder is the one stored in the
`$TMPDIR` environment variable, which is usually `/tmp`

If you do not have the `$TMPDIR` environment variable set, `/tmp` is
used.
