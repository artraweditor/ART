---
title: How to write useful bug reports
contributors:
  - DrSlony
  - Patdavid
  - XavAL
tags:
  - 'Troubleshooting'
toc: true
---

Bugs must be reported in our [RawTherapee GitHub issue tracker](https://github.com/Beep6581/RawTherapee), not in the forum and
not in IRC.

We use the term "bug report" in the broadest sense, any issue you have
must include the information detailed below.

## Requirements of a good bug report

You must always provide the following information:

- **Version details** of the RawTherapee build you're using. You can
  find this information by clicking on
  ![image:preferences.png](/images/preferences.png) Preferences
  \> About \> Version - in most cases that tab will be full of
  information, all of which you should copy and paste into your bug
  report. If that tab is empty, then you can get the RawTherapee version
  from the RawTherapee window's titlebar.
- Your **operating system name and version**.
  - Windows users can find this information by pressing the + keyboard
    shortcut.
  - Linux users can find it by typing `uname -a` inside a terminal.
- Explain the exact **steps to reproduce** the problem. Apply the
  "Neutral" processing profile to your photo and then explain what needs
  to be done to trigger the problem from there.
- **Upload the raw + PP3**. Make your raw and PP3 files available to us
  by uploading them to a file sharing service such as
  [www.filebin.net](http://filebin.net/) if your bug involves a
  particular raw file, a particular setting, or lack of support for your
  raw file.
- **Show a screenshot of the problem**. Do not crop the screenshot, show
  the full RawTherapee window.
- Open **one issue per bug**, or one issue per feature request. Do not
  report different bugs in the same issue, do not request different
  features in the same issue.

Additionally, it is often beneficial to do the following:

- Show a **video recording** of how you trigger the problem. A video
  recording is the best way of explaining a problem if your screenshot
  and "steps to reproduce" are insufficient. You can make one easily
  usinng free software:
  - Windows: [ShareX](https://getsharex.com/)
  - Linux:
    [SimpleScreenRecorder](http://www.maartenbaert.be/simplescreenrecorder/)
- **Check for duplicates**. Search the forum and our issue tracker
  before filing a new report as chances are that someone has already
  reported the problem before you, and duplication wastes time.
- Make sure you **use the latest version of RawTherapee** as it's likely
  that a bug in an old version has been fixed in the latest one.
- If your issue involves a crash, provide a **stack backtrace** if you
  can. How to get one is explained below. If you're not capable of
  getting one, then make sure you have at least provided the information
  above, otherwise your bug report is useless.

For more information on how to report bugs, read this:
<http://www.chiark.greenend.org.uk/~sgtatham/bugs.html>

## When RawTherapee crashes - An introduction to stack backtraces

A stack backtrace is a log of the steps a program took up to a point in
time. For our needs, that point will be the crashing of RawTherapee, and
we will use it for debugging RawTherapee, to fix the crash. We release
builds of RawTherapee which are stable for us. If they crash for you,
then you need to supply us with useful information so that we can either
reproduce the crash ourselves, or fix the bug without being able to
reproduce the crash. The stack backtrace is extremely useful.

Programs need to be compiled to run, and generally speaking they can be
compiled in one of two ways:

- The "release" way, which optimized the program for speed but doesn't
  offer any useful information if it crashes,
- The "debug" way, which provides plenty of useful information when it
  crashes but runs considerably more slowly.

While it is possible to get some information out of a *release* build,
the information is far more precise if you use a *debug* one. You will
find both "release" and "debug" builds of RawTherapee on the
[official downloads page](http://rawtherapee.com/downloads) as well as in
[the forum](https://discuss.pixls.us/c/software/rawtherapee). For day-to-day
use, use a release type build because it runs much faster. If you
encounter a crash, use a debug type build to provide us with useful
information so that we can fix the crash.

Availability of debug builds:

- Windows RawTherapee installers should contain a "release"
  `rawtherapee.exe` and a "debug" `rawtherapee-debug.exe`.
- Linux packages are generally "release" unless the package has the word
  "debug" in the title, though this depends on your distribution,
  package manager and source of packages. The best way to get a debug
  build is to compile one yourself; see the [Linux](linux)
  compilation guide - it's easy!
- macOS packages currently contain "release" executables only. We will
  hopefully include "debug" ones in the future.

### How to get a debug version?

Check [our download page](http://rawtherapee.com/downloads) or your
package manager for a *debug* build of the latest version of
RawTherapee. If there is none, then either ask us for one, or make one
yourself. If you use Linux, making one is very easy, just follow the
[Compiling in Linux](linux) guide.

Software can (and often does) behave differently running under a
debugger than it normally would, due to the inevitable changes the
presence of a debugger will make to a program's internal timing. This
means that you might encounter bugs (crashes) during your day-to-day use
which will not occur when you run RawTherapee from GDB (GDB is the
debugger)! This is an unfortunate fact, but luckily it's a rare one, so
go ahead, get a debug build, and try to reproduce the crash, this time
getting a juicy stack backtrace.

**ALWAYS** download and try the latest version of RawTherapee before
reporting a bug! Reporting based on an old version is a waste of time.

## Step by step

1.  Get GDB. How you do this will depend on your operating
    system/distribution.
    - **Linux** - Use your package manager.

      In Ubuntu you would open a terminal and write:

          sudo apt-get install gdb

      In Manjaro you would write:

          sudo pacman -Syu gdb
    - **Windows** - Download
      [`gdb.exe`](http://www.equation.com/ftpdir/gdb/64/gdb.exe) into
      your RawTherapee folder, so that it sits alongside
      `rawtherapee.exe`
    - **macOS** - if you know how to install GDB under macOS, please
      help write this section.
2.  Run RawTherapee from within GDB.
    - In **Linux**, assuming you used [compiled RawTherapee
      yourself](Linux#Compiling:_The_Manual_Way.md), open up a
      terminal and, assuming your debug build was compiled to
      `~/rt_debug/` then type:
          gdb ~/rt_debug/rawtherapee
    - In **Windows**, open up the command prompt, navigate to your
      RawTherapee folder and assuming that the filename of the
      RawTherapee debug executable is `rawtherapee-DEBUG.exe` then type:
          gdb rawtherapee-DEBUG.exe
3.  GDB starts up and is ready to run RawTherapee. To run it, type:

        r
4.  RawTherapee runs, and you will notice a flood of information in GDB.
    Most of it will look like this:

    `[New Thread 0x7fffa7b2e700 (LWP 11532)]`

    `[New Thread 0x7fffa9b32700 (LWP 11533)]`

    `[Thread 0x7fffaab34700 (LWP 11528) exited]`

    `[Thread 0x7fff97dfd700 (LWP 11507) exited]`

    Do what you did to trigger the crash, and when RawTherapee does
    crash, its window will just freeze, it won't close. You can tell
    that it crashed by the fact that everything in that window will have
    stopped responding. Alt+Tab back to the GDB terminal window.
5.  We want a detailed stack backtrace, and to make sending it to us
    easier in the next three commands you will tell GDB to save it to a
    text file called `log.txt` (or anything, the name doesn't matter).
    Type:

        set pagination off

        set logging file log.txt

        set logging on
6.  Now to have GDB show info on all threads and print the actual stack
    backtrace, type:

        info threads

        thread apply all bt full

    You should see a screen full of text, numbers, code and magical
    spells. These were automatically saved to the `log.txt` file.
7.  You may now quit GDB. To do so, type:

        q

    and confirm it with a

        y
8.  Now it is time to send us the bug report. Open a new issue in our
    [GitHub bug tracker](https://github.com/Beep6581/RawTherapee/issues/new),
    explain the steps you performed which lead to the crash, **attach
    the `log.txt` file**, and provide all the information we asked for
    above.

Nicely formatted text is easier to read. If you are pasting code into
GitHub, use backticks or indent the code - read
[GitHub's Basic Formatting Syntax documentation](https://guides.github.com/features/mastering-markdown/)
to learn how to do that. If you want to paste code into the
[forum](https://discuss.pixls.us/c/software/rawtherapee), you can also
use backticks or indent the code - read
[the Forum code formatting guide](forum).

Remember to include the contents of
"*[image:preferences.png](image:preferences.png) Preferences
\> About \> Version*", a sample raw and PP3 file, along with full
version information of your operating system, your CPU type and speed,
and how much RAM you have, what you had for lunch, etc.
