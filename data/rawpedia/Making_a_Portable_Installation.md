---
title: Making a Portable Installation
contributors:
  - DrSlony
  - Hombre
tags:
  - 'General Information'
toc: true
---

RawTherapee and the cache folder can be stored "self-contained" on a USB
flash drive or any other mass-storage device.

## For Windows

Get the latest build of RawTherapee. Since we want it portable, we don't
want the installer, just the bare, zipped program. If the latest version
on our website is in simple zipped form without an installer, you can
skip this step. However, if it is an installer, you need to first
extract the RawTherapee files.

- If it is an Inno Setup installer (.exe extension, all recent Windows
  installers are Inno Setup ones at the time of writing, summer 2014),
  get [innounp](http://innounp.sourceforge.net/) or
  [innoextract](http://constexpr.org/innoextract/) to unpack it.
- If it is an MSI installer (no recent Windows builds use this at the
  time of writing), fire up a command prompt and type:

      msiexec /a RawTherapee.msi TARGETDIR="C:\TargetDir" /qb

  Replace the name of the MSI installer and the target directory as
  appropriate. Spaces in the TargetDir path are allowed, as the path is
  enclosed in quotes.

Let's assume that you've unzipped your archive into `E:\RawTherapee`,
where `E:\` is the drive letter of your USB flash drive. Open the
`E:\RawTherapee\options` file, and set the `MultiUser` option to
`false`. Now when you run RawTherapee, it will store the cache and your
settings in subfolders relative to the executable, named `mycache` and
`mysettings`, respectively, so in `E:\RawTherapee\mycache` and
`E:\RawTherapee\mysettings`.

See also the [File paths](file_paths) page on how to set a
different location for these two folders.

When updating RawTherapee, it is recommended to unzip the new version to
a new folder and simply move `mycache` and `mysettings` into it.

## For Linux

Getting RawTherapee to run off a portable medium such as a USB flash
drive on various Linux systems is not straightforward due to the nature
of Linux systems. While the Windows version of RawTherapee comes bundled
with all required libraries to run on any Windows version, Linux
distributions differ significantly from each other and as a result a
version of RawTherapee built for one distribution is unlikely to run
under a different distribution. One way around this is by using an
AppImage.

A RawTherapee AppImage is a single file which contains a RawTherapee
executable along with all the required files needed for it to run on any
Linux distribution. Download it, make it executable, and run it. We are
currently in the testing phase regarding AppImages. They are not yet
available from our Downloads page, but you can find them on
[our "development builds" page in the forum](https://discuss.pixls.us/t/download-rawtherapee-development-builds/2924?u=morgan_hardwood).

Regardless whether you use the AppImage or a "proper" RawTherapee build
from the distribution's package manager, you will want to be able to
hang on to your RawTherapee configuration and processing profiles.

In order to backup your configuration you will want to copy
RawTherapee's *config* folder onto your USB stick. Specifically, you
want the "*options*" file, your custom "*camconst.json*" if you made
one, and any custom PP3, ICC, DCP and LCP profiles. The
[File Paths](file_paths) article describes where to find these.
