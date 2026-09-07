---
title: Dynamic processing profiles
contributors:
  - Agriggio
  - DrSlony
tags:
  - 'Processing Profiles'
---


![](dynamic-profile-rules-screenshot.png "dynamic-profile-rules-screenshot.png")
<figcaption>dynamic-profile-rules_screenshot.png</figcaption>


Sometimes a single, "static" default processing profile is not enough to
cover all use cases. For example, the amount of noise reduction to apply
varies according to the camera and ISO setting used. Another example is
the kind and amount of lens corrections needed, which is obviously
dependent on the lens used.

In order to handle such cases, RawTherapee provides a feature that
allows to create a default processing profile "dynamically", based on
the metadata of the image being processed (such as camera and lens name,
shutter speed, ISO value, and so on).

This is done by defining a set of "dynamic profile rules". Each rule has
a [(partial) processing profile](creating_processing_profiles_for_general_use)
attached to it, plus some conditions on the image metadata that define
whether the rule is applicable. When a picture is edited for the first
time, the list of rules is scanned, and all the profiles that match are
combined (in the order given, so later rules can override earlier ones)
to build the initial processing profile.

In order to activate the functionality, the [default processing profile](preferences#default_processing_profile) must be set
to "(Dynamic)". Rules are defined in the [Dynamic Profile Rules](preferences#dynamic_profile_rules_tab) section of the
Preferences window.

In order to invoke the dynamic processing profile chain in batch, after
having configured the dynamic profile rules and set the default profile
for raw/non-raw photos to "(Dynamic)", select multiple images in the
File Browser, right-click any selected image and select "Processing
Profile Operations \> Reset to Default" in the popup context menu.

Dynamic profile rules work on the following image metadata:

Camera
the camera name (including brand) as shown in the image info overlay of
the [Image Editor](the_image_editor_tab). If active, by
default this entry will cause the rule to apply only to pictures taken
with the exact camera specified here (the name is case-insensitive).
However, if the entry starts with the `re:` prefix, then the rest of the
string will be interpreted as a [regular expression](https://en.wikipedia.org/wiki/Regular_expression) to use for
the matching. For example, a rule with the Camera value set to
`re:SONY ILCE-[56].00` will be applied to all Sony Alpha a5xxx and a6xxx
cameras.

<!-- -->

Lens
The full lens name. As above, a regular expression can be used by
starting with the `re:` prefix.

<!-- -->

ISO
The range of ISO values.

<!-- -->

Aperture
The range of apertures of the lens, measured in
[f-stops](https://en.wikipedia.org/wiki/F-number).

<!-- -->

Focal length
The range of focal lengths used, in mm.

<!-- -->

Shutter
The range of shutter speeds, in seconds. For example, enter 0.03 for a
speed of 1/30".

<!-- -->

Exposure compensation
The range of exposure compensation values, in stops.
