---
title: MacOS
contributors:
  - DrSlony
  - Ion12
  - Ingo
  - Benitoite
  - Lebarhon
  - XavAL
tags:
  - 'Compiling'
toc: true
---

This page details instructions for compiling RawTherapee on **macOS**
systems. There are also separate pages with instructions for compiling
on [Linux](linux) and [Windows](windows). This
guide details the **what** and **how** parts of compilation. For the
**why** and explanations of these commands, for a list of dependencies,
CMake options and other information, please refer to the detailed
[Linux](linux) article.

When in doubt, [join us on IRC](irc) and ask a human!

For instructions on cloning the source, choosing branches, configuring
CMake and doing the actual compilation, see these sections in the
[Linux](linux) guide. The information below is in addition to
that.

## Dependencies

See the list of dependencies in the [Compiling in Linux](linux#dependencies) article.

### Homebrew

The following command installs dependencies for RawTherapee:
`brew install gtk+3 gtkmm3 gtk-mac-integration adwaita-icon-theme libsigc++ little-cms2 libiptcdata fftw lensfun wget llvm shared-mime-info exiv2 jpeg-xl libomp automake libtool imagemagick create-dmg`

- **Configuring the homebrew build environment for Apple Silicon "M1"**


To your `cmake` command add the following flags:


`-DLOCAL_PREFIX:STRING="/opt/homebrew"`

`-DCMAKE_OSX_ARCHITECTURES=arm64`

### MacPorts

Tested on OS X 10.9-12.

- **Prerequisites**
  - Xcode Developer Tools & Command Line Tools
  - MacPorts
    - Detailed instructions on setting up MacPorts and the developer
      tools are available on the [MacPorts website](https://www.macports.org).
- **Configure MacPorts:**


Add the following line to /opt/local/etc/macports/variants.conf


`+quartz -x11 -gnome +openmp`

- **Dependencies**


To install the dependencies, run from the terminal:


`sudo port install git cmake clang-11 libomp gtk3 gtkmm3 gtk-osx-application-gtk3 adwaita-icon-theme libsigcxx2 lcms2 libiptcdata fftw-3-single lensfun`

If compiling on Xcode 9.2 you will also need to do:


`sudo port install ld64 +ld64_xcode`

- **Configuring compile system for MacPorts**


To your `cmake` command add the following flag:


`-DLOCAL_PREFIX:STRING="/opt/local"`

## Compiling

See the [Compiling in Linux](linux#compiling:_the_manual_way)
article for instructions on how to **clone** the source code, choose a
**branch** and how to configure **CMake**. Ignore the ***Now you are
ready to compile*** code on that page and follow the code on this page.

RawTherapee is compiled by the **clang** compiler. It may come with
XCode or be installed as a part of **llvm**. Be advised that Apple uses
a versioning scheme for **Apple clang** which is inconsistent with
**llvm clang**. To figure out which compiler to use, check your system
compiler first:

<div style="margin-left: 2em;">

    -> which clang
    /usr/bin/clang

    -> /usr/bin/clang --version
    Apple clang version 11.0.0 (clang-1100.0.33.17)
    Target: x86_64-apple-darwin18.7.0
    Thread model: posix
    InstalledDir: /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin

</div>

If you see **Apple clang** mentioned in the top line of the **clang**
version output, note the version number it specifies and refer to
[this Wikipeda table](https://en.wikipedia.org/wiki/Xcode#Xcode_7.0_-_12.x_(since_Free_On-Device_Development))
for the mapping between **Apple clang** and **llvm clang** versions.
This knowledge may be useful when tracing any compilation errors.

If you want to upload a generic \`x86_64\` build or otherwise share it
with others, you must use


`-DPROC_TARGET_NUMBER="1"`

and set the processor label manually by setting


`-DPROC_LABEL="generic processor"`

If you want to compile a CPU-optimized for yourself only, or build for
the Apple \`M1\`, then use


`-DPROC_TARGET_NUMBER="2"`

and then the processor label would be irrelevant, you could skip it.

If you wish to
[codesign](https://developer.apple.com/support/code-signing/) your
build, add your details to the CMake command:


`-DCODESIGNID="Developer ID Application: Firstname Lastname (XXXXXXXXXX)"`

The app and the generated dmg (Apple Disk Image) will be codesigned.

To
[notarize](https://developer.apple.com/documentation/security/notarizing_your_app_before_distribution/customizing_the_notarization_workflow?language=objc)
your codesigned build, include your app-specific notarial credential in
the CMake command:


`-DNOTARY="--username user@mail.com --password abcd-efgh-ijkl-mnop"`

The app and dmg will be notarized (scanned for malware) and stapled
(have the notarization ticket attached).

### Compile RawTherapee

Now you are ready to compile:

    cd ~/repo-rt
    rm -rf build
    mkdir build && cd build
    lensfun-update-data
    cmake -DCMAKE_BUILD_TYPE="release" \
          -DPROC_TARGET_NUMBER="1" \
          -DPROC_LABEL="generic processor" \
          -DCACHE_NAME_SUFFIX="5-dev" \
          -DCMAKE_C_COMPILER="clang" \
          -DCMAKE_CXX_COMPILER="clang++" \
          -DWITH_LTO="OFF" \
          -DLENSFUNDBDIR="share/lensfun" \
          -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 \
          ..
    make -j$(sysctl -n hw.ncpu) install
    sudo make macosx_bundle

To compile RawTherapee with a specific llvm **clang** version (e.g.
installed via homebrew), use the following **cmake** command. When doing
so, make sure these settings point to the appropriate paths:

- Using llvm 10 from homebrew:

<div style="margin-left: 2em;">

      -DCMAKE_C_COMPILER="/usr/local/Cellar/llvm/10.0.1_1/bin/clang"
      -DCMAKE_CXX_COMPILER="/usr/local/Cellar/llvm/10.0.1_1/bin/clang++"
      -DCMAKE_AR="/usr/local/Cellar/llvm/10.0.1_1/bin/llvm-ar"
      -DCMAKE_RANLIB="/usr/local/Cellar/llvm/10.0.1_1/bin/llvm-ranlib"

</div>

- The **cmake** command:

<div style="margin-left: 2em;">

    export PKG_CONFIG_PATH=/usr/local/opt/libffi/lib/pkgconfig:/usr/local/opt/expat/lib/pkgconfig && \
    cmake  .. -DCMAKE_BUILD_TYPE="release" \
              -DPROC_TARGET_NUMBER="2" \
              -DCACHE_NAME_SUFFIX="5.8-dev" \
              -DCMAKE_C_COMPILER="/usr/local/Cellar/llvm/10.0.1_1/bin/clang" \
              -DCMAKE_CXX_COMPILER="/usr/local/Cellar/llvm/10.0.1_1/bin/clang++" \
              -DWITH_LTO="ON" \
              -DLENSFUNDBDIR="/Applications/RawTherapee.app/Contents/Resources/share/lensfun" \
              -DCMAKE_BUILD_TYPE=Release \
              -DOpenMP_C_FLAGS=-fopenmp=libomp \
              -DOpenMP_CXX_FLAGS=-fopenmp=libomp \
              -DOpenMP_C_LIB_NAMES="libomp" \
              -DOpenMP_CXX_LIB_NAMES="libomp" \
              -DOpenMP_libomp_LIBRARY="/usr/local/lib/libomp.dylib" \
              -DOpenMP_CXX_FLAGS="-Wno-pass-failed -Wno-deprecated-register -Xpreprocessor -fopenmp /usr/local/lib/libomp.dylib -I/usr/local/include" \
              -DOpenMP_CXX_LIB_NAMES="libomp" \
              -DOpenMP_C_FLAGS="-Wno-pass-failed -Wno-deprecated-register -Xpreprocessor -fopenmp /usr/local/lib/libomp.dylib -I/usr/local/include" \
              -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \
              -DCMAKE_EXE_LINKER_FLAGS="-L/usr/local/opt/libffi/lib -L/usr/local/lib" \
              -DCMAKE_AR="/usr/local/Cellar/llvm/10.0.1_1/bin/llvm-ar" \
              -DCMAKE_RANLIB="/usr/local/Cellar/llvm/10.0.1_1/bin/llvm-ranlib" \
              -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15

</div>

### From-Scratch Method

- This
  [1](https://raw.githubusercontent.com/Benitoite/RTdeps/master/macbuildRT.sh)
  obsolete but useful script of commands may be helpful as a basis for compiling dependencies.
- A
  JDK[2](https://www.oracle.com/technetwork/java/javase/downloads/jdk13-downloads-5672538.html)
  must be installed.
- Xcode 11.1+ [3](https://developer.apple.com/xcode) must be installed

### Run and Share RawTherapee

You will find a disk image in the build directory; this is the
distribution release and can be run on any machine which meets the
architecture requirements you specified in variants.conf earlier.

The generated zip file is named according to this template:


RawTherapee_OSX_**<minimum supported macOS version>**_64_**<RawTherapee version>**.dmg.zip

RawTherapee_OSX_10.9_64_5.8-94-g4dbbc4053.dmg.zip

Upload the zip archive to <http://filebin.net/> and
[open a new issue on our GitHub page](https://github.com/Beep6581/RawTherapee/issues/new)
with the link so that we can upload it to the website.

## macOS installation

To install the RawTherapee application, open the .dmg and drag the
RawTherapee app onto the `/Applications` folder.

To use the optional rawtherapee-cli command line interface, move
rawtherapee-cli into a folder in your \$PATH and install the RawTherapee
app as above.

If the workspace is too small to read, you must change the HiDPI
settings in RawTherapee: Preferences \> General, then enable pseudo
HiDPI mode. [4](https://rawpedia.rawtherapee.com/Preferences#Appearance)
