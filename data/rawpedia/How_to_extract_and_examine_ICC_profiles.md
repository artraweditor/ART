---
title: How to extract and examine ICC profiles
contributors:
  - DrSlony
  - XavAL
tags:
  - 'General Photography'
toc: true
---

## DisplayCAL

Project: DisplayCAL
Website: <https://displaycal.net/>

User interface, for Linux, macOS and Windows.

DisplayCAL uses ArgyllCMS, it can show ICC profile details including
curves and colorspace gamut. It also lets you export these as images or
3D HTML/VRML/X3D files which you can share and view in a web browser.

## ArgyllCMS

Project: ArgyllCMS
Website: <https://www.argyllcms.com/>
Documentation (iccdump): <https://www.argyllcms.com/doc/iccdump.html>
Documentation (extracticc): <https://www.argyllcms.com/doc/extracticc.html>

Command-line, for Linux, macOS and Windows.

ArgyllCMS lets you extract ICC profiles from image files and dump the
contents of ICC profiles as human-readable text.

- Extract: `extracticc photo.jpg profile.icc`
- Examine directly in an image: `argyll-iccdump -s photo.jpg`
- Examine an ICC file: `argyll-iccdump profile.icc`

## ICC Examin

Project: Oyranos
Website: <http://www.oyranos.org/icc-examin/>

User interface, for Linux and macOS.

ICC Examin can show you ICC profile details including visualizing it in
3D space.

## colord

Project: colord
Website: <https://www.freedesktop.org/software/colord/>

System service, user interfaces are available. Linux.

colord can show you ICC profile details.

## ExifTool

Project: ExifTool
Website: <http://www.sno.phy.queensu.ca/~phil/exiftool/>

Command-line, for Linux, macOS and Windows.

ExifTool lets you examine ICC profiles, regardless of whether they are
embedded in an image or as stand-alone ".icc" files. It also lets you
extract ICC profiles from images and embed them into images.

- Extract: `exiftool -icc_profile -b -w icc photo.jpg`
- Embed: `exiftool "-icc_profile<=profile.icc" photo.jpg`
- Examine directly in an image: `exiftool -icc_profile:* photo.jpg`
- Examine an ".icc" file: `exiftool profile.icc`

## GIMP

Project: GIMP
Website: <https://www.gimp.org/>

User interface, for Linux, macOS and Windows.

- Extract: Open the photo using GIMP 2.9, then click on "Image \> Color
  Management \> Save Color Profile to File".

## ImageMagick

Project: ImageMagick
Website: <https://www.imagemagick.org/script/index.php>

Command-line, for Linux, macOS and Windows.

- Extract: `convert photo.jpg profile.icc`

## iccToXml

Project: IccXML
Website: <https://sourceforge.net/projects/iccxml/>

Command-line, Linux and Windows. Dead project.

iccToXml is part of the IccXML project. You can use it to convert an ICC
profile to human-readable XML.

Convert: `iccToXml profile.icc profile.xml`

## ICC Profile Inspector

Project: ICC Profile Inspector
Website: <http://www.color.org/profileinspector.xalter>

User interface, Windows only though runs in wine. Dead project.
