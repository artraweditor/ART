---
title: Exposure
contributors:
  - DrSlony
  - Fherb
  - Lebarhon
  - Jdc
tags:
  - 'Tool Description'
  - 'Exposure Tab'
toc: true
---

## Auto Levels

The *Auto Levels* tool analyzes the histogram and then adjusts the
controls in the Exposure section to achieve a well-exposed image.

It uses only the "Exposure compensation", "Highlight compression",
"Highlight reconstruction", "Black", "Lightness" and "Contrast" sliders.

Think of the adjustments Auto Levels comes up with as a good starting
point. It has been tuned to work best with "typical shots", so the
result should often be aesthetically pleasing, but as the program
doesn't know your taste or expectations this will not always be the
case. For example you might be going for a high-key look (i.e. not
typical), in which case you should adjust the values yourself.

You can reset all of the sliders in the Exposure section by clicking on
the Reset button. The Tone Curves will not be touched.

## Clip %

Auto Levels uses the "Clip %" value to adjust the exposure. This number
defines the percentage of pixels allowed to clip to the white and black
points of the raw histogram. The minimum value is 0.00, the maximum is
0.99. Higher values increase the contrast, lower decrease it.

As of RawTherapee 5.5, while the Auto Levels tool in the Editor still
uses the Clip % value, the thumbnails use a fixed value of 0.2% - this
was done to reduce the number of files needed to be stored in the
[cache](file_paths#cache).

## Highlight Reconstruction

And also for 'Color Propagation' - Lowlight Reconstruction

Use *Highlight Reconstruction* (HR) to try to restore overexposed
highlights in raw files. It attempts to restore clipped (blown-out)
regions in the raw image relying on the fact that the three channels in
a raw file do not clip at the same time and so a missing (clipped)
region in one channel could be guessed from the present data of one of
the other color channels. Using the *Color Propagation* method, it can
also guess clipped data using nearby data from unclipped channels, if
present. Remember, this tool is used for the *reconstruction* of clipped
highlights, while if you just want to *compress* highlights which were
not clipped in the first place but became clipped due to the use of, for
example, *Exposure Compensation*, then use the [Highlight Compression](/exposure/#highlight-compression) slider. (A tool to avoid when using [Game Changer](/game_changer) )
The *Auto Levels* button will automatically enable *Highlight
Reconstruction* if necessary.

Four different methods of highlight reconstruction are available:

1.  Luminance Recovery

    Recovered details will be neutral gray.

    "*Highlight Compression*" values under 100 are recommended in most
    cases.
2.  Color Propagation

    This is the most powerful recovery method. In addition to restoring
    luminosity, *Color Propagation* tries to restore color information
    by 'bleeding' the surrounding known color into the missing clipped
    area. This method works best on small overexposed areas, and can
    work wonders on overexposed skin. Its weakness is that it may
    sometimes 'bleed' the incorrect colors, depending on the image
    elements surrounding the blown highlights, or the colors can bleed
    into undesirable patterns. It is also computationally intensive and
    is therefore slower than the other methods.

    Note that 'Color Propagation' also allows data to be restored in very low light, when the values ​​in the blacks are around zero.

    In both cases, the 'Blur' slider allows for a smoother transition between restored areas and those that are unchanged.

    Works well even with very high "*Highlight Compression*" values
    (under 500). A tool to avoid when using "Game Changer"[Game Changer](/game_changer)
3.  Inpaint opposed

    This method restores clipped pixels by looking at selected pixels
    that are next to clipped regions and uses the average of all of
    these pixels to estimate the proper colour of the highlights. This
    works well for most images, but images that have clipped areas of
    significantly different colours may not work as well.

    A team made up of members of Gmic and Darktable have developed
    "Inpaint opposed". Alberto Grigio ported it on ART, and now you can
    find it on Rawtherapee.

    This algorithm is sensitive to white balance settings. Automatic
    White Balance - Temperature Correlation is a possible answer to get
    good settings, especially for Green (Tint).
4.  CIELab

    Reduces the luminance channel and tries to restore colors
    afterwards.

    "*Highlight Compression*" values under 100 are recommended in most
    cases.
5.  Blend

    Attempts to guess clipped color channels by filling in their values
    from the closest match from unclipped highlight regions nearby.

    "*Highlight Compression*" values under 100 are recommended in most
    cases.

## Exposure Compensation

The values of the Exposure Compensation slider (EC) are ISO values. This
means a value of +1 equals one stop of overexposure (+1 EV, exposure
value; also known as +1 LV, light value)). If you make two photos, one
without correction (EV = 0) and one underexposed by one stop (EV = -1),
you can make both photos match by setting exposure compensation for the
overexposed photo to -1, or for the underexposed photo to +1.

Take a look at the histogram while moving this slider. Moving it to the
right shifts the whole histogram to the right. This means this slider
changes the black point (on the very left of the histogram) and the
white point (on the very right).

If you try decreasing the exposure of a photo which contains clipping,
you will notice that the clipped areas turn flat-gray. Enabling
"*Highlight Reconstruction*" will prevent this flat-gray look to some
degree by recovering highlight information from the remaining
non-clipped channels.

If you've used RawTherapee for a while you may notice that you almost
always need a positive Exposure Compensation - as if all your photos are
underexposed. Don’t worry - this is normal. Most cameras deliberately
underexpose to preserve highlights (though some other raw processors
disguise it).

For the user interested in the internals follows a technical description
of how EV=0 relates to the raw data: while EV=0 equals 1.0 in gain (that
is, no change), there is a white balance dependent base gain applied
before exposure. This base gain is calculated such that the color
channel with the smallest range just can reach the maximum value.
Although all raw channels have the same range in the raw file, white
balance will re-balance them (lower temperatures mean more gain on red,
higher more gain on blue) so they do not clip at the same level. The
base gain raises the smallest channel just enough to make sure that at
EV=0 no highlights will be based on clipped raw data. As white balance
changes raw channel gains the base gain will thus change as a side
effect when white balance is changed. For large white balance changes
you may therefore see a slight change of brightness in the image. Note
that the base gain relates to the maximum values channels can represent,
that is if there is no highlight clipping in the raw file no highlight
will reach the maximum level at EV=0.

## Highlight Compression

The Highlight Compression slider (HC) can be used to recover the
highlights in a photo by compression, useful for 'dimming' (or burning)
slightly overexposed areas. While the *Highlight Reconstruction* tool
lets you try to *reconstruct* missing data in the raw file relying on
the fact that the three channels in a raw file do not clip at the same
time and so a missing (clipped) region in one channel could be guessed
from the present data of one of the other color channels, this
*Highlight Compression* tool only works on data which is already there -
which has not been irreversibly lost at the time of shooting. If the
original image has no clipping, but due to the action of, for example,
exposure compensation you caused clipping, then you can use *HC* to
compress these clipped regions back into view. As such, it works not
only on raw files but on normal images too.

To see if your photo contains overexposed areas, click on the clipped
highlight indication icon
![<File:Warning-highlights.png>](Warning-highlights.png "File:Warning-highlights.png")
on the top right of the image window. Overexposed areas will show up in
black.


![](hra_0.jpg "hra_0.jpg")
<figcaption>Hra_0.jpg</figcaption>



![](hra_0_chi.jpg "Hra_0_chi.jpg")
<figcaption>Hra_0_chi.jpg</figcaption>
</figure>

By dragging the *Highlight Compression* slider to the right, the
intensity of the highlights will decrease. For highlight compression to
work best you must also enable *Highlight Reconstruction*. Each of the
HR methods has its strong and weak points, as explained above. Color
Propagation is the most likely method to produce good-looking results
when the HC slider is significantly over 100. For the other methods you
will usually want to keep the HC slider around or below 100 - watch the
histogram and the preview!


![](hra_125_ba.jpg "hra_125_ba.jpg")
<figcaption>Hra_125_ba.jpg</figcaption>
</figure>

To find out the optimal HC value, you can make use of the histogram. In
the screenshots above, what you see are overexposed clouds above the
Teide volcano in Tenerife. When hovering the mouse cursor over the
overexposed area, the pixel values indicator (in the *Navigator* panel,
under the little preview) shows that lightness (L) is at 100, and the
histogram shows that all channels are clipped (see the red, green and
blue squares in the top-right corner of the histogram, they mean that
there are so many pixels of maximum value that they are off-scale).
Increase the HC slider until the red, green and blue channels in the
histogram no longer squash up against the right end of the histogram -
you want them to touch it, but not to cram up against it. You can enable
the *Clipped highlight indication* icon
![<File:Warning-highlights.png>](Warning-highlights.png "File:Warning-highlights.png")
before moving the HC slider up. Once the indicator's black areas
disappear from the white parts you want compressed, which is also when
the lightness of those pixels drops from L=100 to L=99, you stop. Don't
increase the HC slider any more, because now the hopelessly-lost white
areas would start turning gray. You don't want them to turn gray. That
would make the photo look dull. In this example the indicator's black
areas disappeared when I set HC to 125.

![](hra_toomuch_histogram.png "hra_toomuch_histogram.png")
<figcaption>Hra_toomuch_histogram.png</figcaption>


As a rule, the histogram of a correctly developed image should touch
both ends - the black and the white end. Not doing so means the image
was incorrectly developed. This is true for the vast majority of photos,
the only exceptions being photos of scenes which lack dynamic range,
such as misty scenes. If you increase the HC slider too much, then
whites turn gray as your histogram no longer touches the maximum end.
Examples of photos that have been over-compressed can be easily found on
the internet. They look horrible, don't do that! Recover what you can,
but what's clipped beyond repair should stay white.

RawTherapee offers more ways to deal with blown highlights. The
side-effect of all these methods is that they also take away some of the
brilliance of the photos, as they get more 'flat' or 'dull' as a result.
Highlight compression is very useful when used with moderation, but
remember that you cannot recover what is not there to begin with, so
once you notice that the completely clipped white areas become gray you
should reduce the compression amount until these areas become white once
more. To create the best possible output, feed RawTherapee the best
possible input - so expose well in the first place!

## Highlight Compression Threshold

The Highlight Compression Threshold slider sets the point where the HC
slider starts implementing compression. A value of 0 means the threshold
is zero: data compression occurs over the whole range of tonalities. 100
sets the threshold at one stop below the white point, so all the
compressed highlights are squeezed into the top stop. In practical
terms, more highlights are compressed when this slider is set to 0.

## Black

Use this to set the black point. See the left side of the histogram move
when you touch the slider. Values greater than 0 will make the image
darker, negative values will lighten up the shadow parts of the photo.

## Shadow Compression

The *Shadow Compression* slider 'dampens' the effect of the *Black*
slider. The maximum value of 100 gives a less dark image. This slider
only has effect when the *Black* slider is set to a value other than 0.
Practical use of this *Shadow Compression* slider is to fine-tune the
shadow intensity of the image.

## Lightness

This slider applies a hard-coded tone curve to lift or lower the
tonalities of the photo, resulting in a more or less light image. The
same tone curve is applied separately to each R, G and B channel. The
black point and the white point keep their positions.

## Contrast

This slider increases or reduces the contrast of the photo. It applies a
contrast curve centered at the average luminance level. Tonalities above
the average are lifted (lowered), while tonalities below the average are
lowered (lifted). The same contrast curve is applied separately to each
R, G and B channel.

## Saturation

This slider makes the photo more or less saturated. In more technical
terms, it adjusts the saturation of the image by applying a multiplier
to the saturation level of pixels in the HSV color space.

## Tone Curves

Here you can construct your own tone curves. They work on all three R, G
and B channels at the same time (so you can't work on the R channel
only).

There are two tone curves available, which can be designed with various
curve types, and applied in several different modes all explained below.
Clicking on the curve button hides the curve from the interface - it
does not disable the curve.

The histogram displayed as the curve's background shows you the levels
of the data as it flows into the curve at that point in the processing
pipeline. You will notice that it differs from the main histogram which
shows you the levels of the final image, at the very end of the
pipeline.

While you are free to use only one tone curve to make your adjustments,
you can gain much finer tonal control if you use two curves at once. The
typical use of both curves is to lower values using the first curve, and
to raise values using the second one. It is similar to creating an S
curve in one of them, but you should be able to make finer adjustments
by using both without entering too fast in the "danger zone" where your
colors becomes unrealistic.

You can save a curve to disk. Click on the *Save current curve* button
![Image:Gtk-save-large-dark.png](Gtk-save-large-dark.png "Image:Gtk-save-large-dark.png")
next to the graph and give it a name. Use the *Load a curve from file*
button ![Image:Gtk-open.png](Gtk-open.png "Image:Gtk-open.png") to apply
this curve later to another file. Use the *Reset curve to linear* button
![Image:Gtk-undo-ltr.png](Gtk-undo-ltr.png "Image:Gtk-undo-ltr.png") to
delete all the points you created and to reset the curve to
neutral/linear. You can also *Copy*
![Image:Gtk-copy.png](Gtk-copy.png "Image:Gtk-copy.png") and *Paste*
![Image:Gtk-paste.png](Gtk-paste.png "Image:Gtk-paste.png") curves
to/from RawTherapee's own clipboard, which comes in very useful if you
want to quickly apply an identical curve to a different tool. Saved
curves have the `.rtc` filename extension.

You can use as many control points in a curve as you like.

You can define different curves for all curve types if you like, but
only the one that is selected in the dropdown menu will be applied to
the photo.

The curve and histogram is always displayed with sRGB gamma, regardless
of working or output profile. This means that the shadow range is
expanded and highlights compressed to better match human perception.

### Auto-Matched Tone Curve

### Curve Type

#### Linear

![](rt55_curve_linear.png "rt55_curve_linear.png")
<figcaption>Rt55_curve_linear.png</figcaption>

This represents the unaltered (or linear) image, so without any tone
curve applied. It disables the curve.

#### Standard


![](rt55_curve_standard.png "rt55_curve_standard.png"
<figcaption>Rt55_curve_standard.png</figcaption>


This is a classic cubic spline curve, seen in many other programs as
well. The left part of the graph represents the darker tones, the right
part represents the brighter tones of the photo. Click on the curve to
mark a point and drag it with the mouse to change tonalities. Dragging
the point down makes the image darker, while pushing it up makes it
brighter. The dotted diagonal line marks the linear or unaltered state
of the photo. Press and hold the **Control** key to slow down the
movement. Hold the **Shift** key to snap the point to key elements:
maximum value, minimum value, middle value (i.e. snapped to the diagonal
or horizontal dotted line), same value of the preceding point, same
value of the next point, and for the *Control Cage* type, the line going
through the previous and next points. Delete a point on the curve by
dragging it out of the editor area.

The top-right point represents the brightest areas in the photo. Drag
that point vertically down to make the highlights less bright; move it
horizontally to the left to make bright areas brighter, perhaps at the
cost of some overexposure.

The bottom-left point represents the darkest areas in the photo. Move
that point horizontally to the right to make the photo darker, perhaps
at the cost of some underexposure. Move it vertically up to make the
darks lighter.

Flip the diagonal line (from bottom-left and top-right to top-left and
bottom-right) to produce a negative image.
A typical usage of the standard curve is to construct a so-called
*S-curve*. Mark three points at the 'coordinates' (1,1), (2,2) and (3,3)
respectively. Drag the point at (1,1) somewhat lower and the point at
(3,3) a bit higher. Your image will get more 'punch' this way. If your
S-curve is symmetrical, i.e. if you move the point you first placed at
1,1 by the same amount as the one you placed at 3,3 but in the opposite
direction, then the effect will be identical to manipulating the
*Contrast* slider.

#### Flexible


![](rt55_curve_flexible.png "rt55_curve_flexible.png")
<figcaption>Rt55_curve_flexible.png</figcaption>


A characteristic of the "Standard" cubic spline curve is that editing
one node could have a huge impact on what happens to the curve in
relation to the other nodes. The "Flexible" centripetal Catmull–Rom
spline curve allows you to make adjustments to any part of the curve
with little impact on the other parts.

#### Parametric


![](rt55_curve_parametric.png "rt55_curve_parametric.png")
<figcaption>Rt55_curve_parametric.png</figcaption>


This curve presents four sliders and three control points. The sliders
are used to control highlights, lights, darks and shadows respectively
(shadows mean deep darks here). Move the mouse over the four sliders and
a dark area under the curve tells you which slider alters what part of
the curve. Move the Highlights slider to the left to make highlights
less bright, move it to the right to make them brighter. The *Lights*
slider moves the lights but not the highlights in the same way as above,
as does the *Darks* slider: moving it to the right lightens the dark
tones, moving it to the left darkens them. The *Shadows* slider works as
the *Darks* slider, but only on the darkest parts of the photo. You can
again construct the above mentioned stylized S-curve, although the
parametric curve gives you less 'extreme' control over the form of the
curve. This mode, however, has its own benefits, as curves can be shaped
in a well controlled manner. Note that using these sliders can have a
profound influence on the overall contrast of the image.

Use, if needed, the three control points under the curve. They determine
what point of the curve will be affected when moving the sliders. Moving
the middle control point to the right makes the image darker (the form
of the curve changes again, as does the dark area around the curve),
moving it to the left makes the image brighter. Moving the left control
point to the right darkens the dark areas somewhat, moving it to the
left lightens them, again somewhat. Moving the right control point to
the right makes the highlights brighter, moving it to the left darkens
the highlights.

Use the *Reset to default* buttons
![Image:Gtk-undo-ltr.png](Gtk-undo-ltr.png "Image:Gtk-undo-ltr.png")
next to the sliders to reset individual sliders, use the same button at
the top of the tone curve section to reset all four sliders and the
control points to linear (zero).

#### Control Cage


![](rt55_curve_control_cage.png "rt55_curve_control_cage.png")
<figcaption>Rt55_curve_control_cage.png</figcaption>
</figure>

At first sight this curve type looks very much like the *Standard*
curve, but there are some differences though. With the *Standard* curve,
the curve touches all the control points. This is not the case with the
*Control Cage* curve. To see this, click somewhere on the line and move
the black point to the left or to the right. Now the curve passes nearby
the black point, but doesn't touch it. Another difference is that the
*Control Cage* allows for a straight section of the curve, while you
can't do this with the *Standard* curve. The *Control Cage* curve needs
at least three points for that (so five in total). Holding down the
**Shift** key while dragging a point will help you to easily create a
straight line by snapping the point to the line made by the previous and
next point (displayed in red by the *Snap to* tool). Now make a new
point between the two most left ones and move it. As you can see, only
the part on the left side moves, not the rest of the curve.

### Curve Mode

Next to each curve type, you'll find a *Curve Mode* combobox selector.
This will let you choose the algorithm that will be used for the
corresponding curve. The curve mode will have a strong effect on the
appearance of colors, especially if you use a contrast-enhancing curve
(S-curve). This can be used for creative effect, but can for some
purposes or styles cause undesired color changes depending which mode
you choose. Choose a mode that suits your specific taste and needs for
the photo at hand. By combining two different curves in tone curve 1 and
2 you can further fine-tune the look.

The choice of working profile has an influence on the effect of the
curves in all modes except for perceptual - in that mode, changing the
working profile will not alter the effect of the curve.

#### Standard

This is the most basic mode (and the only one available in older
versions of RawTherapee and is found in some shape or form in most
image-related software): the values of each RGB channel are modified by
the curve in a basic "correspondence" method, that is the same curve is
applied to all channels.

The drawback of this mode is that e.g. considering an S-curve shape to
get more contrast, an orange color with a high value of red and green
and a low value of blue will tend to shift toward yellow, because the
red and green component will be raised, while the blue one will be
lowered.

In general an S-curve will increase separation of the channels and thus
increase saturation, which is a similar behavior to how color film
reacts to contrast. This together with the simplicity of implementation
has made the curve type popular in raw converters in general and is
often the only alternative available in less flexible software.

#### Weighted Standard

You can use this method to limit the color shift of the standard curve,
even if it won't suppress it entirely. Keeping the previous example,
this method will raise the first component (red), and will also linearly
alter the green and blue component by raising them too. We end up with 3
values (R, g and b) while we have only processed the red component.

This process is then done for the green and blue component, so at the
end of the process, we end up with 9 values (R,g,b / r,G,b / r,g,B).
Values of the same component are then mixed together, which will produce
the resulting color with a smaller color shift.

#### Film-Like

The film-like curve provides a result highly similar to the standard
type (that is strong saturation increase with increased contrast), but
the RGB-HSV hue is kept constant - that is, there are less color-shift
problems. This curve type was designed by Adobe as a part of DNG and is
thus the one used by Adobe Camera Raw and Lightroom.

#### Saturation and Value Blending

This mode is typically better suited for high-key shots, but can be used
for creative effect in other photos as well. The average value of the
three component is computed, and then the curve is applied to this
value, giving a positive or negative gain. The color is converted to its
Hue, Saturation and Value representation, then if the gain is positive,
the pixel is linearly targeting Value = 1 and Saturation = 0, the Hue is
preserved. If the gain is negative, the pixel is linearly targeting
Value = 0, Saturation and Hue are preserved.

The result is highly similar to a luminance curve in Lab space (that is
change contrast without affecting hue or saturation). For
contrast-increasing curves the look will typically be slightly
desaturated. This is not really because the curve desaturates the colors
but because that in human vision contrast and saturation is tightly
coupled, so the same image with higher contrast requires higher
saturation to appear to have the same.

#### Luminance

Each component of the pixel is boosted by the same factor so color and
saturation is kept stable, that is the result is very true to the
original color. However contrast-increasing curves can still lead to a
slightly desaturated look for the same reason as described for the
Saturation and Value Blending curve mode. If you want to manually
counter-act the desaturation, using the L\*a\*b\* Chromaticity slider is
a more neutral way of compensating for it than using the RGB-based
saturation slider.

Despite showing the R, G and B histogram (merged) in the background of
the curve, the curve operates on luminance values, where [Relative Luminance](https://en.wikipedia.org/wiki/Relative_luminance) Y =
R\*0.2126729 + G\*0.7151521 + B\*0.0721750 First the relative luminance
value of a pixel is obtained, then the curve is applied to that value,
the multiplication factor between before and after luminance is
calculated, and then this factor is applied to each R, G and B
component. This is in contrast to the other methods where the curve is
applied to each R, G and B component separately.

#### Perceptual

This mode will keep the original color appearance concerning hue and
saturation, that is if you for example apply an S-curve the image will
indeed get increased contrast, but the hues will stay the same and the
image doesn't look more or less saturated than the original. It's
specifically useful to establish a pleasing baseline contrast without
distorting the colors provided by a camera profile which doesn't apply a
curve itself (if you use a third-party profile that does apply a curve
it's typically already perceptually mapped with similar techniques as
described here).

The choice of working profile has an influence on the effect of the
curves in all modes except for perceptual - in this mode, changing the
working profile will not alter the effect of the curve.

The algorithm works the following way: it analyses the curve to get a
contrast value, which is used as base to scale chroma (saturation) such
that more contrast leads to more saturation and the other way around. As
contrast and saturation is tightly coupled in human vision this scaling
is necessary to make saturation *appear* constant. There are further
fine-tunings such as increase saturation more in the shadows, and less
for colors that are already highly saturated, also this corresponds to
human vision phenomena so the net effect it that the colors appear
constant. In the extreme highlights close to the white point the
algorithm blends over to white (like the standard curves) which is less
true to color but more practical for real output as the brightest color
of the output media (screen or paper) is white.

However do keep in mind that the perceptual model is not perfect and
cannot be perfect. This is only a curve, image content is not analyzed
and no localized changes are made. This means for example that for an
S-curve a large flat blue sky (low local contrast) may appear slightly
more saturated than the original. If you want to make A/B comparisons
don't compare side by side as the eye will then be confused by the two
contrast levels viewed simultaneously and then saturation will not
appear the same, but instead swap and let the eye adapt for a few
seconds.

If you want to further fine-tune the saturation manually it's generally
best to use the L\*a\*b\* chroma tools.

Due to the many components in the algorithm it's considerably slower
than the other curve modes so refresh rate may suffer.
