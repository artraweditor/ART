---
title: Download
contributors:
  - DrSlony
  - XavAL
  - Thanatomanic
tags:
  - 'General Information'
weight: 3
toc: true
---

## Stable Releases

Stable builds can be downloaded from [**our
website**](https://rawtherapee.com/downloads) or through your **package
manager**. Stable builds are tried and tested and suitable for
production use.

## Development Builds

Development builds can be downloaded from our official [**GitHub
repository**](https://github.com/Beep6581/RawTherapee/releases/tag/nightly-github-actions).

Development build filenames follow this structure:

`RawTherapee_<branch>_[operating-system]_<build-type>.<extension>`

Branch  
Each commit happens on a branch. The main branch is called `dev`. New
features are developed on their own branches, and then merged into `dev`
when ready. When in doubt, get `dev`.

Operating system  
A build is made for a specific operating system: `win64` builds run on
any modern 64-bit version of Windows, `macOS_10.15` builds run on the
specified version of macOS or newer, and Linux builds which should run
on any modern distro omit the OS segment and instead use `AppImage` as
the extension.

Build type  
The build type is either `release` or `debug`. Release-type builds are
optimize to run fast, but do not provide any useful information in case
of a crash. Conversely, debug-type builds are capable of producing
useful information during a crash [for a bug
report](https://rawpedia.rawtherapee.com/How_to_write_useful_bug_reports),
but will run significantly slower. When in doubt, get `release`.

Every time the source code is changed, a development build is
automatically created. These builds are provided for the sole purpose of
allowing you to try out the newest features and the latest fixes, and to
report your findings back to us via
[GitHub](https://github.com/Beep6581/RawTherapee/) or [the
forum](https://discuss.pixls.us/c/software/rawtherapee/).

Development builds are **not intended for production use** and we
provide **no backward or forward compatibility for [sidecar files
(pp3)](/Sidecar_Files_-_Processing_Profiles)** created using a
development build! That means that a sidecar file created using a dev
build will very likely lead to different results when used in a stable
release or in another dev build. If stability and compatibility are key
for you, we always recommend that you use the latest stable release. If
you want to make use of a feature or fix present in the latest dev build
in production, you have to wait until we release the next stable
version.
