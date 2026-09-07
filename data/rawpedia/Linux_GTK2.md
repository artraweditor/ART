---
title: Linux GTK2
contributors:
  - DrSlony
---

This archived page details instructions for compiling the obsolete GTK2
version of RawTherapee on old **GNU/Linux** systems. The last version of
RawTherapee to support GTK2 was tagged in git as `5.0-r1-gtk2` in
February 2017.

## Dependencies

To compile RawTherapee your system will need a set of tools and code
from other programs. These are called dependencies, and here is a list
of them for the obsolete GTK2 version of RawTherapee:

| Package      | Version             | URL                                                                                |
|--------------|---------------------|------------------------------------------------------------------------------------|
| EXIV2        | exiv2\>=0.19        | <http://www.exiv2.org/>                                                            |
| EXPAT        | expat\>=2.1.0       | <http://expat.sourceforge.net/>                                                    |
| FFTW3        | fftw\>=3.2.2        | <http://fftw.org/>                                                                 |
| GCC          | gcc\>=4.9           | <http://gcc.gnu.org/>                                                              |
| GLIB2        | glib-2.0\>=2.24     | <http://www.gtk.org/>                                                              |
| GLIBMM       | glibmm-2.4\>=2.24   | <http://www.gtkmm.org>                                                             |
| GTK+         | gtk+-2.0\>=2.24.18  | <http://www.gtk.org/>                                                              |
| GTK2-Engines | gtk-engines-2.20.2  | <http://www.gtk.org/>                                                              |
| GTKMM        | gtkmm-2.4\>=2.22    | <http://www.gtkmm.org>                                                             |
| JPEG         | libjpeg\>=6b        | <http://libjpeg-turbo.virtualgl.org/> <http://jpegclub.org/> <http://www.ijg.org/> |
| LCMS2        | lcms\>=2.6          | <http://www.littlecms.com/>                                                        |
| LENSFUN      | lensfun\>=0.2       | <http://lensfun.sourceforge.net/>                                                  |
| LIBCANBERRA  | libcanberra\>=0.29  | <http://0pointer.de/lennart/projects/libcanberra/> (Linux only)                    |
| LIBIPTCDATA  | libiptcdata\>=1.0.2 | <http://libiptcdata.sourceforge.net>                                               |
| PNG          | libpng\>=1.2.44     | <http://www.libpng.org/>                                                           |
| SIGC         | sigc++-2.0          | <http://libsigc.sourceforge.net/>                                                  |
| TIFF         | libtiff\>=3.9.4     | <http://www.remotesensing.org/libtiff/>                                            |
| ZLIB         | zlib\>=1.2.3        | <http://www.zlib.net/>                                                             |

Build-time dependencies for RawTherapee:

To compile the obsolete RawTherapee version 3, you will need these:

| Package | Version     | Gentoo          | Debian/Ubuntu | URL                         |
|---------|-------------|-----------------|---------------|-----------------------------|
| LCMS1   | lcms\<=1.99 | media-libs/lcms | liblcms1-dev  | <http://www.littlecms.com/> |

Build-time dependencies for the obsolete RawTherapee 3:

Refer to the current [Linux](linux) article for compilation
instructions, and replace the GTK3 dependencies with the GTK2 ones where
needed. Use the obsolete `5.0-r1-gtk2` branch, do not use the `dev`
branch.

### Debian/Ubuntu/Mint/elementary OS

#### Ubuntu 15.04, 14.10, 14.04 LTS

    sudo apt-get update
    sudo apt-get install build-essential cmake curl git libbz2-dev libcanberra-gtk-dev libexiv2-dev libexpat-dev libfftw3-dev libglibmm-2.4-dev libgtk2.0-dev libgtkmm-2.4-dev libiptcdata0-dev libjpeg8-dev liblcms2-dev libpng12-dev libsigc++-2.0-dev libtiff5-dev zlib1g-dev

RawTherapee requires GCC version 4.9 or higher. Ubuntu 14.04 LTS ships
with GCC version 4.8.2 which is too old - to get 4.9, follow these
steps:
<http://askubuntu.com/questions/466651/how-do-i-use-the-latest-gcc-on-ubuntu-14-04>

#### Ubuntu 13.10, 13.04, 12.10, 12.04 LTS, 11.10

These versions of Ubuntu are badly outdated. The code below used to work
but it may stop working at any moment. Upgrade your operating system.

As these versions of Ubuntu only support GCC-4.8.1 and older, the latest
commit you will be able to compile is [commit b343b9a7](https://github.com/Beep6581/RawTherapee/commit/b343b9a7) from
2015-12-29 - newer versions will not compile.

    sudo apt-get update
    sudo apt-get install build-essential cmake curl git libbz2-dev libcanberra-gtk-dev libexiv2-dev libexpat-dev libfftw3-dev libglibmm-2.4-dev libgtk2.0-dev libgtkmm-2.4-dev libiptcdata0-dev libjpeg8-dev liblcms2-dev libpng12-dev libsigc++-2.0-dev libtiff5-dev zlib1g-dev

#### Ubuntu 10.10 and 11.04

These versions of Ubuntu are badly outdated. The code below used to work
but it may stop working at any moment. Upgrade your operating system.

    sudo apt-add-repository ppa:dasprid/rawtherapee
    sudo apt-get update
    sudo apt-get install cmake curl git libbz2-dev libcanberra-gtk-dev libexiv2-dev libexpat-dev libglibmm-2.4-dev libgtk2.0-dev libgtkmm-2.4-dev libiptcdata0-dev libjpeg62 liblcms2-dev libnm-glib2 libpng12-dev libsigc++-2.0-dev libtiff4-dev zlib1g-dev
