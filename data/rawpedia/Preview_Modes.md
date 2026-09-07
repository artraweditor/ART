---
title: Preview Modes
contributors:
  - DrSlony
---

## Introduction

In addition to the normal preview, RawTherpaee supports a number of
other preview modes to help you tweak your photos. Preview modes are
controlled via buttons in the Editor toolbar or via shortcuts. Only one
preview mode can be engaged at a time.

The following preview modes are currently supported:

- Red channel,
- Green channel,
- Blue channel,
- Luminosity, which is calculated as 0.299\*R + 0.587\*G + 0.114\*B,
- Focus mask, to see which areas are in focus

Image:Preview_1_regular.jpg\|Regular Image:Preview_2_red.jpg\|Red
Image:Preview_3_green.jpg\|Green Image:Preview_4_blue.jpg\|Blue
Image:Preview_5_luminosity.jpg\|Luminosity
Image:Preview_6_focus.jpg\|Focus Mask

<table>
<caption>align="bottom" | * The preview is returned to normal by
deselecting any other mode.</caption>
<thead>
<tr class="header">
<th><p>Preview Mode</p></th>
<th><p>Shortcut</p></th>
<th><p>Button</p></th>
</tr>
</thead>
<tbody>
<tr class="odd">
<td style="padding:10px;"><p>Regular*</p></td>
<td></td>
<td><figure>
<img src="/images/preview_mode_1_regular.png"
title="Image:preview mode 1 regular.png" />
<figcaption>Image:preview mode 1 regular.png</figcaption>
</figure></td>
</tr>
<tr class="even">
<td style="padding:10px;"><p>Red channel</p></td>
<td style="text-align: center;"><p>r</p></td>
<td><figure>
<img src="/images/preview_mode_2_red.png"
title="Image:preview mode 2 red.png" />
<figcaption>Image:preview mode 2 red.png</figcaption>
</figure></td>
</tr>
<tr class="odd">
<td style="padding:10px;"><p>Green channel</p></td>
<td style="text-align: center;"><p>g</p></td>
<td><figure>
<img src="/images/preview_mode_3_green.png"
title="Image:preview mode 3 green.png" />
<figcaption>Image:preview mode 3 green.png</figcaption>
</figure></td>
</tr>
<tr class="even">
<td style="padding:10px;"><p>Blue channel</p></td>
<td style="text-align: center;"><p>b</p></td>
<td><figure>
<img src="/images/preview_mode_4_blue.png"
title="Image:preview mode 4 blue.png" />
<figcaption>Image:preview mode 4 blue.png</figcaption>
</figure></td>
</tr>
<tr class="odd">
<td style="padding:10px;"><p>Luminance channel</p></td>
<td style="text-align: center;"><p>v</p></td>
<td><figure>
<img src="/images/preview_mode_5_luminance.png"
title="Image:preview mode 5 luminance.png" />
<figcaption>Image:preview mode 5 luminance.png</figcaption>
</figure></td>
</tr>
<tr class="even">
<td style="padding:10px;"><p>Focus Mask</p></td>
<td style="text-align: center;"><p>Shift+f</p></td>
<td><figure>
<img src="/images/preview_mode_6_focus.png"
title="Image:preview mode 6 focus.png" />
<figcaption>Image:preview mode 6 focus.png</figcaption>
</figure></td>
</tr>
</tbody>
</table>

align="bottom" \| \* The preview is returned to normal by deselecting
any other mode.

## Red, Green, Blue and Luminosity Preview Modes

When clipping indicators are engaged in the RGBL preview modes, shadow
clipped areas are indicated in a blue color and highlight clipping is
indicated in red. As during normal preview, the lightness of the
clipping highlight is indicative of the degree of clipping.

Preview of individual channels may be helpful when editing RGB curves,
planning black/white conversion using the channel mixer, evaluating
image noise, etc. Luminosity preview is helpful to instantly view the
image in black and white without altering development parameters, to see
which channel might be clipping or for aesthetic reasons.

## Focus Mask

![Focus mask indicating the focusing plane](/images/Preview_6_focus_2.jpg "Focus mask indicating the focusing plane")
The focus mask is designed to highlight areas of the image which are in
focus. Naturally, focused areas are sharper, so the sharp areas are
being highlighted. The focus mask is more accurate on images with a
shallow depth of field, low noise and at higher zoom levels.To improve
detection accuracy for noisy images evaluate at smaller zoom, around the
10-30% range. Note that the preview is rendered more slowly when the
focus mask is enabled.

The current implementation analyzes the preview image which is rescaled
from the original captured size. This process of rescaling reduces the
noise and is helpful to identify truly sharper details rather than noise
itself which may also contain micro texture. At the same time, rescaling
of the original image to the preview size compresses larger scale
details into a smaller size, and it may introduce aliasing artifacts,
both of which could lead to false positives. You can increase your
confidence by viewing the mask at various zoom levels. It is not always
fault proof, but can be helpful in many cases.

**Warning**: Be sure to double-check your images if you decide to delete
them based on the focus mask.
