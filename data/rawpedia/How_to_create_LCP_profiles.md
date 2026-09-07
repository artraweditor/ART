---
title: How to create LCP profiles
contributors:
  - DrSlony
  - XavAL
tags:
  - 'General Photography'
toc: true
---

You can easily create LCP (Lens Correction Profile) profiles using Adobe
Lens Profile Creator.

This section explains how to install Adobe Lens Profile Creator and how
to get you started using it.

__FORCETOC__

## Installation in Linux

We will be using `$HOME/wine-dng` as the Wine prefix.

1.  Install [Wine](http://www.winehq.org/), preferably using your
    package manager.
2.  Set up the Wine prefix:

        WINEPREFIX="$HOME/wine-dng" wineboot --init
3.  [Download Adobe Lens Profile Creator for Windows](http://supportdownloads.adobe.com/detail.jsp?ftpID=5490).
4.  At the time of writing, the latest version is 1.0.4 and it is a
    plain zip file. Extract it somewhere, and move the
    `Adobe Lens Profile Creator 1.0.4` folder to
    `~/wine-dng/drive_c/Program Files (x86)/`
5.  Run Adobe Lens Profile Creator:

        WINEPREFIX="$HOME/wine-dng" wine "$HOME/wine-dng/drive_c/Program Files (x86)/Adobe Lens Profile Creator 1.0.4/Adobe Lens Profile Creator.exe"
6.  Add an alias so that you can run Adobe Lens Profile Creator from a
    console with ease:

        echo "alias lcp='WINEPREFIX=\"\$HOME/wine-dng\" wine \"\$HOME/wine-dng/drive_c/Program Files (x86)/Adobe Lens Profile Creator 1.0.4/Adobe Lens Profile Creator.exe\"'" >> ~/.bashrc && exec bash
7.  To run Adobe Lens Profile Creator, just type `lcp` in a console.

## Installation in Windows

1.  [Download Adobe Lens Profile Creator for Windows](http://supportdownloads.adobe.com/detail.jsp?ftpID=5490).
2.  Install Adobe Lens Profile Creator.

## Usage

Basically you need to print a checker-pattern chart and photograph it at
various apertures, and at various zoom levels if using a zoom lens. You
then open these photos in Adobe Lens Profile Creator, enter some
metadata, scan the chart, and out comes the LCP.

The Adobe Lens Profile Creator User Guide PDF explains the whole
process. You can find it on Adobe's [Digital Negative Resources](https://helpx.adobe.com/photoshop/digital-negative.html#resources)
page, look in the "Adobe Lens Profile Creator" section for "read the
user guide (PDF, 1.64 MB)".
