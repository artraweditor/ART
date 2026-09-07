---
title: Wayland
contributors:
  - DrSlony
  - XavAL
tags:
  - 'Troubleshooting'
---

Users of the Wayland, a computer protocol which specifies the
communication between a display server (called a Wayland compositor) and
its clients, may find that there are some glitches in RawTherapee's user
interface, for example all comboboxes (such as the Processing Profiles
selector, the Film Simulation drop-down, etc.) work correctly only the
first time they are used, and then do not respond until RawTherapee is
restarted. This is not a bug in RawTherapee.

Reports indicate that the cause is an incompatibility between GTK+ and
the Wayland compositor.

Users of the following distributions are most likely affected:

- Fedora \>=25
- Ubuntu 17.10 (Ubuntu 18.04 reverted to X11 due to Wayland issues)

Affected users are advised to:

- Try switching the compositor to Weston.
- Revert to X11 if problems persist.

More information:


<https://fedoraproject.org/wiki/How_to_debug_Wayland_problems>

Note that some information in the above document may not be up to date.
