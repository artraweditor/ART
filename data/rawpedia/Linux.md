---
title: Linux
contributors:
  - DrSlony
  - Ingo
  - Agriggio
  - Thanatomanic
  - XavAL
  - Benitoite
  - Jdc
tags:
  - 'Compiling'
toc: true
---

This page details instructions for compiling RawTherapee on
**GNU/Linux** systems. There are also instructions for compiling on
[Windows](windows) and [macOS](macos).

When in doubt, [join us on IRC](irc) or in the
[Forum](forum) and ask a human!

## Dependencies

To compile RawTherapee your system will need a set of tools and code
libraries from other programs. These are called dependencies, and here
is a list of them for the latest version of RawTherapee:

| Package     | Version             | URL                                                                         |
|-------------|---------------------|-----------------------------------------------------------------------------|
| CMake       | cmake\>=3.5         | <https://cmake.org/>                                                        |
| EXIV2       | exiv2\>=0.19        | <http://www.exiv2.org/>                                                     |
| EXPAT       | expat\>=2.1.0       | <https://libexpat.github.io/>                                               |
| FFTW3       | fftw\>=3.2.2        | <http://fftw.org/>                                                          |
| GCC         | gcc\>=4.9           | <https://gcc.gnu.org/>                                                      |
| GLIB2       | glib-2.0\>=2.24     | <https://www.gtk.org/>                                                      |
| GLIBMM      | glibmm-2.4\>=2.24   | <https://www.gtkmm.org/>                                                    |
| GTK+        | gtk+-3.16 \< 3.24.0 | <https://www.gtk.org/>                                                      |
| GTKMM       | gtkmm-3.16          | <https://www.gtkmm.org/>                                                    |
| JPEG        | libjpeg\>=6b        | <https://libjpeg-turbo.org/> <https://jpegclub.org/> <https://www.ijg.org/> |
| LCMS2       | lcms\>=2.6          | <https://www.littlecms.com/>                                                |
| LENSFUN     | lensfun\>=0.2       | <https://github.com/lensfun/lensfun>                                        |
| LIBCANBERRA | libcanberra\>=0.29  | <http://0pointer.de/lennart/projects/libcanberra/> (Linux only)             |
| LIBIPTCDATA | libiptcdata\>=1.0.2 | <http://libiptcdata.sourceforge.net>                                        |
| PNG         | libpng\>=1.2.44     | <http://www.libpng.org/>                                                    |
| librsvg     | librsvg\>=2.40      | <https://github.com/GNOME/librsvg>                                          |
| SIGC        | sigc++-2.0          | <https://github.com/libsigcplusplus/libsigcplusplus>                        |
| TIFF        | libtiff\>=4.0.4     | <http://libtiff.org/>                                                       |
| ZLIB        | zlib\>=1.2.3        | <http://www.zlib.net/>                                                      |

Build-time dependencies for RawTherapee:

In order to install all these dependencies, you will need to open a
console and paste the code from the appropriate section into the
console.

The code snippets below list dependencies for the latest RawTherapee
code which requires GTK3. We dropped support for GTK2 with release
"5.0-r1-gtk2" in February 2017. If you use a modern distribution, just
copy and paste the code snippets as they are. If you're on an old
distribution without the required GTK3 support, then refer to the
archived [GTK2](linux_gtk2) article, then checkout and
compile the obsolete `5.0-r1-gtk2` tag.

### Arch/Manjaro

Current versions of Arch and Manjaro work well out of the box. Refer to
the [GTK2](linux_gtk2) article if compiling on a version
older than 17.1.12.

    sudo pacman -S cmake ninja exiv2 expat fftw glib2 glibmm gtk3 gtkmm3 lcms2 lensfun libcanberra libiptcdata libjpeg-turbo libpng librsvg libsigc++ libtiff zlib

For Manjaro, you may need to additionally install the `pkg-config`
package.

Proceed to [Compilation](#compilation).

### CentOS

CentOS 8 and newer, lack the `libiptcdata` library. Manual compilation
is required - any suggestions on how to proceed are welcome.

CentOS 7 has very outdated packages and requires extra steps to install
a recent GCC, git, lensfun and libtiff. The steps below were verified to
work in CentOS 7.4.1708, but proceed at your own risk.

GCC \>=4.9.3:

    sudo yum update
    sudo yum install cmake git
    sudo yum install centos-release-scl
    sudo yum install devtoolset-7-gcc*
    scl enable devtoolset-7 bash
    source /opt/rh/devtoolset-7/enable

git \>=2.7:

    sudo yum install http://opensource.wandisco.com/centos/7/git/x86_64/wandisco-git-release-7-2.noarch.rpm

lensfun:

    wget http://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
    sudo rpm -ivh epel-release-latest-7.noarch.rpm

libtiff \>=4.0.4:

    sudo yum install ninja-build
    mkdir ~/programs && cd ~/programs
    wget http://download.osgeo.org/libtiff/tiff-4.0.9.tar.gz
    tar zxvf tiff-4.0.9.tar.gz
    mkdir tiff-4.0.9/libtiff-build && cd tiff-4.0.9/libtiff-build
    cmake -DCMAKE_INSTALL_DOCDIR=/usr/share/doc/libtiff-4.0.9 -DCMAKE_INSTALL_PREFIX=/usr -G Ninja ..
    ninja-build
    sudo ninja-build install

Install the other dependencies:

    sudo yum install curl expat-devel fftw-devel gtk3-devel gtkmm30-devel lcms2-devel lensfun-devel libcanberra-devel libiptcdata-devel libjpeg-turbo-devel libpng-devel librsvg2-devel zlib-devel

Symlink libatomic:

    sudo ln -s /usr/lib64/libatomic.so.1 /usr/lib64/libatomic.so

As you proceed to the next step - compilation - you will need to edit
the `build-rawtherapee` script and add these three lines to the CMake
section near the end of the file, for example after the
"-DWITH_BENCHMARK" line before the "\$HOME" line:

        -DTIFF_INCLUDE_DIR="$HOME/programs/tiff-4.0.9/libtiff" \
        -DTIFF_LIBRARY="$HOME/programs/tiff-4.0.9/libtiff-build/libtiff/libtiff.so" \
        -DCMAKE_CXX_FLAGS="-Wno-deprecated -Wno-parentheses" \

Proceed to [Compilation](#compilation).

### Debian/Ubuntu/Mint/elementary OS

Current versions of these distributions work well out of the box (Debian
\>=9, Ubuntu \>=18.04 LTS, Linux Mint \>=19, elementary OS \>=5).
Instructions below assume a fully updated system. Additional packages
may need to be installed on your particular system.

    sudo apt install git build-essential cmake curl pkg-config libgtk-3-dev libgtkmm-3.0-dev liblensfun-dev librsvg2-dev liblcms2-dev libfftw3-dev libiptcdata0-dev libtiff5-dev libcanberra-gtk3-dev libexiv2-dev libraw-dev libjxl-dev

#### Ubuntu 16.04 LTS (Xenial)

RawTherapee 5.8 (release) is the latest version supported for
Xenial-based distributions. See
[here](https://github.com/Beep6581/RawTherapee/issues/5943) for details.
When compiling, follow the manual steps and use git to checkout the
`5.8` branch.

==== Ubuntu \<=14.04 LTS (Trusty), Debian \<=8 (Jessie) ==== Refer to
the [GTK2](linux_gtk2) article if compiling on earlier
versions of Ubuntu. The GTK3 version of RawTherapee is unsupported in
these distributions.

Proceed to [Compilation](#compilation).

### Fedora

Current versions of Fedora work well out of the box. Instructions below
assume a fully updated system. Additional packages may need to be
installed on your particular system.

    sudo dnf install git make cmake curl gcc gcc-c++ gtk3-devel exiv2-devel gtkmm30-devel lensfun-devel librsvg2-devel lcms2-devel fftw-devel expat-devel libiptcdata-devel libtiff-devel libjpeg-turbo-devel libcanberra-devel libatomic

==== Fedora \<=23 ==== RawTherapee 5.6 (release) is the latest version
supported for this distribution because of a higher CMake version
requirement (see
[here](https://github.com/Beep6581/RawTherapee/issues/5302)). When
compiling, follow the manual steps and use git to checkout the `5.6`
branch.

#### Fedora 22

In addition to the above notices and requirements, an updated version of
`libtiff` needs to be compiled manually.

    sudo yum install ninja-build
    cd ~
    wget http://download.osgeo.org/libtiff/tiff-4.0.9.tar.gz
    tar zxvf tiff-4.0.9.tar.gz
    cd tiff-4.0.9.tar.gz
    mkdir build && cd build
    cmake -DCMAKE_INSTALL_DOCDIR=/usr/share/doc/libtiff-4.0.9 -DCMAKE_INSTALL_PREFIX=/usr -G Ninja ..
    ninja-build
    sudo ninja-build install

==== Fedora \<=21 ==== Refer to the [GTK2](linux_gtk2)
article if compiling on earlier versions of Fedora. The GTK3 version of
RawTherapee is unsupported in these distributions.

Proceed to [Compilation](#compilation).

### Gentoo/Sabayon

Sabayon users should use the same dependencies as for Gentoo, but
instead of `sudo emerge -uva` use
`sudo equo install sys-devel/gcc dev-vcs/git`.

    sudo emerge -uva dev-cpp/gtkmm:3.0 dev-libs/expat dev-util/cmake media-gfx/exiv2 media-libs/lcms media-libs/lensfun media-libs/libcanberra media-libs/libiptcdata media-libs/libjpeg-turbo media-libs/libpng gnome-base/librsvg media-libs/tiff net-misc/curl sci-libs/fftw sys-libs/zlib x11-libs/gtk+:3

Proceed to [Compilation](#compilation).

### openSUSE

openSUSE Leap 15 and Tumbleweed should work well out of the box. Serious
compilation issues can be expected with other versions. Refer to the
[GTK2](linux_gtk2) article if compiling on a version older
than 42.1.

    sudo zypper install git cmake gcc gcc-c++ gtk3-devel gtkmm3-devel liblcms2-devel fftw3-devel libiptcdata-devel librsvg-devel libtiff-devel libjpeg8-devel libcanberra-gtk3-devel

For openSUSE 15.1 and newer, the `lensfun` library needs to be installed
as follows:

    sudo zypper install lensfun-data liblensfun1

For openSUSE Tumbleweed, the package is slightly different:

    sudo zypper install lensfun-devel

For other versions of openSUSE `lensfun` has to be installed manually:

    wget https://sourceforge.net/projects/lensfun/files/0.3.2/lensfun-0.3.2.tar.gz
    tar xvf lensfun-0.3.2.tar.gz
    cd lensfun-0.3.2.tar.gz
    mkdir build
    cd build
    cmake ../
    make
    sudo make install

Proceed to [Compilation](#compilation).

## Compilation

There are two general ways you can compile RawTherapee: either use the
[automatic](#the_automatic_way) Bash script which compiles
RawTherapee for you (recommended), or do so
[manually](#the_manual_way).

### The Automatic Way

This is the recommended way of compiling RawTherapee as it is fast,
simple and fool-proof. It relies on a Bash script which downloads the
latest RawTherapee source code and compiles it in a way which is
optimized for your CPU. The compiled builds are ready for use. The
script does not check for build-time dependencies, so be sure to read
the [Dependencies](#dependencies) section before using the
script. The compiled builds are standalone, meaning that you can keep
several versions of RawTherapee at the same time simply by renaming the
build folders so that creating a new build does not overwrite the
previous build, which happens by default.

Run the script as a normal user, not as root!

Open a terminal, get the script, make it executable, and run it:

    cd ~
    wget https://raw.githubusercontent.com/Beep6581/RawTherapee/dev/tools/build-rawtherapee -O build-rawtherapee
    chmod +x build-rawtherapee
    ./build-rawtherapee

If everything goes well, the script will terminate with the message, "To
run rawtherapee type: ...".

To update RawTherapee if you previously compiled it using this script,
just re-run the script. That's it.

The `build-rawtherapee` script is included in RawTherapee's source code.
Since running the script updates the source code, after you compiled
your first build you can delete the script you downloaded manually above
using wget, and instead use
`~/programs/code-rawtherapee/tools/build-rawtherapee` which will always
update itself.

The script compiles the current branch, which by default is `dev` where
most of the development happens. To compile a different branch, check it
out using standard git commands before running the script. Read more
about RawTherapee's branches below in the [Choose a Branch](linux#choose_a_branch) section.

For more information, see `./build-rawtherapee --help`

You have finished, RawTherapee is ready for use. You can skip the
"Manual Way" section.

### The Manual Way

The recommended way of compiling RawTherapee is by using the automatic
script - see [Compilation: The Automatic Way](#the_automatic_way). If you want to learn how to compile
manually, read on.

In order to keep your "home" folder clean when manually compiling
multiple programs (i.e. when not using your distribution's package
manager) and for this manual compilation tutorial to maintain
compatibility with the automatic compilation script, you will create the
folder `~/programs/` which will contain all RawTherapee-related source
code in the `~/programs/code-rawtherapee` folder, and the compiled build
in the `~/programs/rawtherapee` folder. You can use the same scheme when
compiling other programs.

#### Clone the source

First, you need to clone RawTherapee's source code repository. Bring up
your console and run this:

    mkdir -p ~/programs
    git clone https://github.com/Rawtherapee/RawTherapee ~/programs/code-rawtherapee
    cd ~/programs/code-rawtherapee

#### Choose a branch

- Features are developed on their own feature branches.
- Development happens in the `dev` branch. Feature branches are merged
  into the `dev` branch when they're ready. The `dev` branch is
  unstable.
- Releases are tagged in the `releases` branch.

Checkout the latest tag if you want the most stable code. To see all
available tags, type:

    git tag

Checkout the `dev` branch or some other feature branch if you want to
test the latest bleeding-edge code. To see all available branches, type:

    git branch -a

Checking out is done via the "git checkout" command. To checkout a tag
or a branch, type:

    git checkout <tag or branch>

RawTherapee uses GTK+ for the user interface and requires GTK+ version
3.16 or newer. If your system does not support version 3.16 or newer
then you must checkout the `5.0-r1-gtk2` tag. Our GTK2 support has
officially ended on 2 February 2017 - refer to the archived
[GTK2](linux_gtk2) article, and update your system.

Compiling old versions of RawTherapee will fail on a modern system, as
you will be missing the old dependencies.




#### Compile RawTherapee

Now you will make an out-of-source compilation of RawTherapee, it will
be built into the `~/programs/code-rawtherapee/build/release` folder,
and then you will move this folder to `~/programs/rawtherapee`

##### CMake

There are a few compilation settings you need to be aware of, you will
pass these to CMake using the `-D` option as described below:

CMAKE_BUILD_TYPE
One of: `release` (default), `relwithdebinfo` or `debug`.

This controls whether the build will favor faster execution time or more
verbose debugging output.

The "debug" and "relwithdebinfo" builds will let you [get a useful stack-backtrace](how_to_write_useful_bug_reports) if
RawTherapee crashes while running through GDB which you can then submit
to us so we can find the problem and fix it. The "debug" build is the
slowest but generates the most detailed information. The
"relwithdebinfo" build is almost as fast as a "release" build and
generates often sufficient information, though not as detailed as a
"debug" build. The "release" build will not provide any useful
information when it crashes, but does contain many speed optimizations
resulting in a program that works several times faster than the "debug"
build would. For normal use, make a "release" or "relwithdebinfo" build.
If you find a reproducible bug, then make a "debug" build and send us a
stack-backtrace (or fix it yourself and send us the patch!). We prefer
stack backtraces from debug builds than from relwithdebinfo ones.


To make a "release" type build, set: `-DCMAKE_BUILD_TYPE="release"`

<!-- -->

USE_OLD_CXX_ABI
`ON` or `OFF` (default).

When compiling a program, one must use the same conventions as those
used by the libraries which that program relies upon, otherwise
compilation (linking) will fail. Generally one does not need to concern
oneself with this, but we are now at a time when GCC4 is being phased
out by GCC5, each by default using a convention incompatible with the
other, and so this issue is relevant. If the libraries on your system
have been compiled using GCC5, they probably use a standard called
C++11. This means that your RawTherapee build must use the same
standard, which is the case by default. However, if despite using GCC5
your libraries were built using the older C++03 standard, then
RawTherapee must be set to use the same, and this is when you would set
"USE_OLD_CXX_ABI" to "ON".


To enable USE_OLD_CXX_ABI, set: `-DUSE_OLD_CXX_ABI="ON"`

<!-- -->

CACHE_NAME_SUFFIX
The CACHE_NAME_SUFFIX options sets the suffix of the cache and config
folder names the compiled RawTherapee build will use. See the
[File Paths](file_paths) article for an explanation of what those
are.

For stable releases (if you checkout the "releases" branch) use
`-DCACHE_NAME_SUFFIX=""`

For development builds (if you checkout the "dev" branch or any branch
other than "releases") use `-DCACHE_NAME_SUFFIX="5-dev"`

<!-- -->

PROC_TARGET_NUMBER
From `0` (default) to `9`.

The PROC_TARGET_NUMBER option sets which CPU type to optimize for.

If building for yourself, use "2". It means "native", so the
optimizations will be automatically detected for your CPU and
RawTherapee will perform as fast as possible on your CPU. It might not
run at all on older or other CPU architectures.

If building for distribution (for other people), use "1". It means
"generic", so only optimizations supported by most CPUs will be used,
meaning the build can be downloaded and used by anyone, though it won't
benefit from the best optimizations possible for the user's CPU.

For more info, see the file "ProcessorTargets.cmake" in the cloned
repository.


To make a build using "native" optimizations, set:
`-DPROC_TARGET_NUMBER="2"`

<!-- -->

BUILD_BUNDLE
`ON` or `OFF`.

Forced to "ON" for Windows and macOS. Optional in Linux where it is
"OFF" by default.

If set to ON, the program will be built into the `DATADIR` folder,
otherwise it will be installed relative to `CMAKE_INSTALL_PREFIX` which
would typically be system-wide.

<!-- -->

BUNDLE_BASE_INSTALL_DIR
Use an absolute path.

The program will be built into this folder.

For example, set it to:
`-DBUNDLE_BASE_INSTALL_DIR="$HOME/programs/rawtherapee"`

If it is not set, the default is to use
`${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}`

<!-- -->

LENSFUNDBDIR
Unset by default.

The `LENSFUNDBDIR` option permits to locate the lensfun database in the
specified directory. It can be unset, absolute or relative.

When unset, Lensfun uses its own logic to find the database. This is the
recommended option if you have Lensfun installed system-wide and want to
use it.

You can set it to a relative or absolute path if you want to use a
custom lensfun database.

If building a bundle, it is relative to the bundle's root folder,
otherwise it is relative to `DATADIR`, i.e.
`${CMAKE_INSTALL_PREFIX}/share/rawtherapee`

<!-- -->

OPTION_OMP
`ON` (default) or `OFF`.

Build with OpenMP support when enabled, which enables multithreading and
makes RawTherapee much faster.

<!-- -->

WITH_LTO
`ON` or `OFF` (default).

Build with link-time optimizations when enabled, which may make
RawTherapee run a little faster.

<!-- -->

WITH_PROF
`ON` or `OFF` (default).

For debugging purposes. Generate extra code to write profile information
suitable for the analysis program gprof.

<!-- -->

WITH_SAN
`OFF` (default) or one of various other options.

For debugging purposes. Allows enabling various sanitizers to help
detect program issues.

See GCC manual's [Program Instrumentation Options](https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html)
chapter for more information.

<!-- -->

WITH_SYSTEM_KLT
`ON` or `OFF` (default).

Build using system KLT library when ON, otherwise use KLT files bundled
with RawTherapee.

The Kanade–Lucas–Tomasi (KLT) feature tracker is used by the
[Auto Distortion Correction](lens/geometry#distortion_correction)
tool.

<!-- -->

WITH_BENCHMARK
`ON` or `OFF` (default).

Build with timing functions enabled to benchmark performance.

<!-- -->

ENABLE_TCMALLOC
`ON` or `OFF` (default).

In some cases the operating system has trouble handling memory
allocation and deallocation required by RawTherapee (see
[here](https://github.com/Beep6581/RawTherapee/issues/5459) for more
information). Linking against
[TCMalloc](https://gperftools.github.io/gperftools/tcmalloc.html) may
alleviate these problems. This library may not be available on all
platforms or distributions.

- JPEG XL read support depends on libjxl. By default, RawTherapee builds
  with JPEG XL support if and only if libjxl is present. Use
  -DWITH_JXL="ON" or DWITH_JXL="OFF" to explicitly enable or disable,
  respectively, JPEG XL support.
- RawTherapee builds with a custom version of LibRaw by default. To use
  the system LibRaw, use -DWITH_SYSTEM_LIBRAW="ON". Requires LibRaw \>=
  0.21.
  


##### Make

Find out how many threads your CPU supports. This only influences the
compilation speed, it has no influence over how fast the compiled
RawTherapee build runs. To find out how many threads your CPU supports,
run this in a terminal:

    nproc --all

It will return a number. Use this number for the `--jobs` parameter
below.

Compile:

    cd ~/programs/code-rawtherapee
    mkdir build
    cd build

    cmake \
        -DCMAKE_BUILD_TYPE="release"  \
        -DCACHE_NAME_SUFFIX="5-dev" \
        -DPROC_TARGET_NUMBER="2" \
        -DBUILD_BUNDLE="ON" \
        -DBUNDLE_BASE_INSTALL_DIR="$HOME/programs/rawtherapee" \
        -DOPTION_OMP="ON" \
        -DWITH_LTO="OFF" \
        -DWITH_PROF="OFF" \
        -DWITH_SYSTEM_LIBRAW="ON"\
        -DWITH_SAN="OFF" \
        -DWITH_SYSTEM_KLT="OFF" \
        ..

    make --jobs=4
    or:
    make --jobs=16 //if your processor support 16 threads
    make install

#### Run RawTherapee

To run RawTherapee:

    ~/programs/rawtherapee/rawtherapee

Or to run the CLI version:

    ~/programs/rawtherapee/rawtherapee-cli

The source code repository is in `~/programs/code-rawtherapee` and the
compiled program is in `~/programs/rawtherapee`

You can safely delete `~/programs/code-rawtherapee` if you so wish. The
compiled program will still work, but then you will have to redo all the
above steps if you want to update. Rather, leave the repository intact
so that you can do the next step in a week or a month's time when you
want to update.

#### Update RawTherapee

Every time you want to update RawTherapee to the latest code available,
just do the following:

    cd ~/programs/code-rawtherapee
    git pull

Then repeat the [Make](#make) step above.

When updating, you can re-use the `build` folder from last time to avoid
having to recompile things which have not changed, to make the whole
process faster. CMake should automatically detect changes. However,
there are situations when compilation may fail when re-using an old
`build` folder - typically that might happen when hopping between very
different branches. If that happens, just delete the `build` folder,
then proceed with the steps in the "Make" section.
