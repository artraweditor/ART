---
title: Clipping Indication
contributors:
  - DrSlony
---

The clipped shadow
![<File:Warning-shadows.png>](Warning-shadows.png "File:Warning-shadows.png")
and
![<File:Warning-highlights.png>](Warning-highlights.png "File:Warning-highlights.png")
highlight indicators in the Editor allow you to easily see which areas
of the image are too dark or too bright. Highlighted areas are shaded
according to the much they transgress the thresholds.

The thresholds for these indicators are defined in Preferences \>
General.

The clipped **shadow** indicator will highlight areas where all three
channels fall at or below the specified shadow threshold.

The clipped **highlight** indicator will highlight areas where at least
one channel lies at or above the specified highlight threshold. If you
want to see only where all channels are clipped, then enable the
luminosity preview mode in addition to the clipped highlight indicator.

Clipping is calculated using data which depends on the state of the
gamut button
![<File:Gamut-hist.png>](Gamut-hist.png "File:Gamut-hist.png") which you
can toggle above the main preview in the Editor tab. When the gamut
button is enabled the working profile is used, otherwise the
gamma-corrected output profile is used.
