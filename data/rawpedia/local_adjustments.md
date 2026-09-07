---
title: Local Adjustments
date: 2026-04-01
contributors:
  - Jdc - Jacques Desmis
  - DrSlony
tags:
  - 'Tool Description'
  - 'Selective Editing'
toc: true
---


## Introduction

Local editing (Selective Editing) in RawTherapee is based on RT-spots, which are similar in
principle to the U-Point concept originally used in Nikon Capture NX2
and subsequently in the Nik Collection, DxO PhotoLab and Capture NXD.
RT-spots use algorithms developed specifically for RawTherapee by
Jacques Desmis.
This approach is completely different to the more familiar local editing
methods used in applications such as GIMP, Photoshop, etc., which
primarily use selection tools such as lassos, magic wands etc.,
associated with brushes, layers and blend masks. These methods can be
time consuming and difficult to use accurately when complex shapes are
involved.
An RT-spot consists of either an ellipse or a rectangle with a
variable-diameter circle at the center. The shapes have four control
points, which can be adjusted independently or symmetrically. The
rectangle spot can also be used in full-image mode which automatically
sets the control points outside the image preview area. Future
developments will provide enhanced shape manipulation.

<figure>
<img src="ellipse.jpg" title="ellipse.jpg" width="200" />
<figcaption>Ellipse</figcaption>
</figure>
<figure>
<img src="rectangle.jpg" title="rectangle.jpg" width="200" />
<figcaption>Rectangle</figcaption>
</figure>

The RT-spot algorithm uses shape detection based on ΔE (the change in
the visual perception of two given colors) to select the parts of the
image to be modified inside the ellipse or rectangle. The reference
values used for the shape detection algorithm are based on the average
of the hue, chroma and luminance values inside the variable-diameter
circle. This means that in full-image mode (and also in Normal or
Excluding modes) these values and the subsequent shape detection can
vary depending on the position of the circle.

The extent to which these modifications are applied can be finely
controlled allowing for very precise selections. Further refinement is
possible with additional parametric masks but the shape-detection
algorithms should be sufficient for the vast majority of local editing
requirements. RT-spots can also be used in Excluding mode to prevent the
algorithm from influencing certain parts of the image. The modifications
that can be carried out are extensive and incorporate most of the
functions available in RawTherapee's global adjustment tools along with
some additional tools available only in the Local Adjustments tab.

Note: provided the checkbox “Avoid color shift” in the Settings module
has not been disabled, the following operations will be carried out on
the data before and after any RT Spot is activated.

- A relative colorimetric correction to keep the data within gamut.
- A Munsell correction using LUTs to ensure that the data remains linear
  and avoid hue shifts.

### The Tools

The tools are grouped in the following modules (Tool name - position in
pipeline):

#### Color & Light - 11

Adjust color, lightness, contrast and correct small defects such as
red-eye, sensor dust etc. Other functions include a graduated filter,
L\*a\*b\* curves and blend modes.

#### Shadows/Highlights & Tone Equalizer - 6

Adjust shadows & highlights with either the shadows/highlights sliders,
a Tone Equalizer or a Tone Response Curve (TRC). Can be used instead of,
or in conjunction with the Exposure module. Can also be used as a
graduated filter.

#### Vibrance & Warm/Cool - 5

Adjust vibrance (essentially the same as the global adjustment). Carry
out the equivalent of a white-balance adjustment using a CIECAM
algorithm.

#### Log Encoding - 0

Adjust underexposed or high-dynamic-range images using a log-encoded
algorithm.

#### Dynamic Range & Exposure - 10

Modify exposure in L\*a\*b\* space using Laplacian PDE algorithms to
take into account deltaE and minimize artifacts. Laplacian operators are
used because they are particularly good at detecting fine details but
you do not need to understand how they work to use this tool!

#### Common Color Mask - 12

A tool in its own right. Can be used to adjust the image appearance
(chrominance, luminance, contrast) and texture as a function of Scope.

#### Soft Light & Original Retinex - 7

Apply a Soft-light blend (identical to the global adjustment). Carry out
dodge and burn using the original Retinex algorithm.

#### Blur/Grain & Denoise - 1

Can be used to blur backgrounds, soften skin, add film grain and
denoise.

#### Tone Mapping - 2

Same as the tone mapping tool in the main menu. The main menu tool must
be deactivated if this tool is used.

#### Dehaze & Retinex - 3

Dehaze and Retinex (Advanced mode only). Useful for dehaze, local
contrast with high values and simulation of "clarity".

#### Sharpening - 8

Uses RL deconvolution sharpening. View at 1:1 or Capture Deconvolution

#### Local Contrast & Wavelets - 8

- Local Contrast: basically the same functions as Local Contrast in the
  Detail tab.
- Wavelets: based on Wavelet Levels in the Advanced tab with essentially
  the same features (clarity, contrast, blur, etc., see documentation).
  Additional features such as a Graduated Filter, Tone mapping, etc.
  have also been included. Its use in Local Adjustments provides
  additional possibilities such as the removal of large blemishes,
  grease stains etc.

#### Contrast By Detail Levels - 4

Contrast by detail levels. Can be used to remove sensor or lens marks.

#### Color appearance(Cam16 & Jzcz) - 12

This module is a simplified version of the Color Appearance &
Lighting(Ciecam02/16) module in the Advanced tab in the main menu, which
has been adapted to the specific requirements of Local Adjustments. It
has also been extended to take into account HDR Peak Luminance and
includes an experimental JzCzHz function (in Advanced mode) to improve
HDR processing.

Each tool module can be toggled between Basic, Standard & Advanced
modes. The default mode can be set in RawTherapee's Preferences window.

## Getting started

The examples in the following sections are designed to give an overview
of some of the ways the various tools can be used for local adjustments.
However, if you prefer to explore the possibilities by yourself, check
that the “Default complexity for Local Adjustments” in the Preferences
module is set to Basic and uncheck the "Show additional settings"
checkbox at the top of the Local Adjustments module. This will give you
a simplified yet powerful version of Local Adjustments.

The Basic mode, which has no masks and in most cases no curves, is the
closest to the original intention of Jacques Desmis when development
started back in 2015.

Explore the capabilities of the Color & Light, "Shadows/Highlights &
Tone Equalizer" and "Vibrance & Warm/Cool" tools to start with and don't
hesitate to try out the additional functionality by manually setting the
complexity mode to Standard (in the combobox in the module you are
working on).

The Color & Light tool is extremely powerful and includes functions from
both the "Color Toning \> Color correction regions" module in the
main-menu Color tab as well as the L\*a\*b\* curves available in the
Exposure tab

Please note: the screenshots in the following examples are currently
being updated to take into account the latest developments. Because of
this, some of the slider and module names will be different from the
text.

### First steps

#### Activating Local Adjustments

- In the Tab tool bar, select the "hand" icon (Local Adjustments tab).
- Turn on the Local Adjustments power button (if it is not already
  activated) and expand the Settings module.
- Select Add.


<figure>
<img src="startspot.jpg" title="startspot.jpg" width="600" />
<figcaption>startspot.jpg</figcaption>
</figure>


#### Choose the type of Spot


[The four types of RT-spot]({{% relref "#the-four-types-of-rt-spot" %}})


#### Preparation

Position the RT-spot at the desired location. In this case, we want to
increase the saturation of the red flower and reduce the luminance
(lightness) without affecting the rest of the image.

- Move the center of the RT-spot so that it is located on an area
  representative of what you want to change.
- Position the 4 delimiters well beyond the flower.
- Select the Lockable Color Picker and locate 3 colors: a) one on the
  red flower, b) one on the blue sky, c) one on a green leaf.
- In the example the 3 colors are:
  - Red flower L=48.6 a=74.4 b=47.0
  - Blue sky: L=68.6 a=-3.1 b=-16.6
  - Green leaf: L=48.3 a=-28.3 b=51.4

<img src="prepare1.jpg" title="prepare1.jpg" width="600"
alt="prepare1.jpg" /> Raw file (Jacques Desmis - Creative Common
Attribution-share Alike 4.0):
[1](https://drive.google.com/file/d/1X2g73FqzQl7-WRfzhF7zHa7XWwnKcwtx/view?usp=sharing)


#### Adding The Color And Light Tool

In the settings menu, choose "Add tool to current spot".

- You will see a list of choices: Color & Light, Shadows/Highlights &
  Tone Equalizer etc. For each RT-spot you can associate one or more
  tools from the list. The processing order in the pipeline corresponds
  to the number at the end of the tool description as described in the
  Introduction: Log Encoding - 0 is first (if it is activated), Color &
  Light -11 is the last. This is also the case for the associated masks.
- Select Color & Light.

<figure>
<img src="addtoolcolorandlight1.jpg" title="addtoolcolorandlight1.jpg"
width="300" />
<figcaption>addtoolcolorandlight1.jpg</figcaption>
</figure>

#### Adjusting luminance (lightness) and chrominance

- Set Lightness to -70
- Set Chrominance to 130

<!-- -->

- Review the results.
- The red flower now has a new color L= 41.3, a = 66.0, b = 50.4
- The sky is unchanged.
- The green leaf is unchanged.

<figure>
<img src="lightchro1.jpg" title="lightchro1.jpg" width="600" />
<figcaption>lightchro1.jpg</figcaption>
</figure>

#### Color Tool Scope and Transition Value

In the Settings module:

- Observe the effect of moving the "Scope (color tools)" slider.
  - If you reduce the value (default 30) only a part of the reds will be
    affected.
  - If you increase the value, the sky, then the green leaf, then the
    whole image will be taken into account (Scope=100).

Leave the Scope value at 100 and in the Settings module select "Show
additional settings".

- Observe the effect of moving the "Transition value" slider:
  - Reduce the value to 5.
  - Increase the value to 100 and see the result.

#### Previewing the adjustment area using deltaE (ΔE)

You can preview the areas of the image that will be affected by any
changes. The preview does not show the changes themselves or the
transitions, but allows you to set the scope of any adjustments.

There are two possibilities.

- Use the Preview ΔE button located in Settings. This will only work
  properly if you have activated one (and only one) of the color tools
  in "Add tool to current spot" menu.
- Use the Preview ΔE option in the "Mask and modifications" menu
  associated with a particular tool (standard and advanced modes only).
  In this case the GUI takes into account any adjustments made with the
  tool and works regardless of the number of activated tools.

You can vary the intensity and color of this preview with "ΔE preview
color " in the "Shape detection" section of the Settings module. The
preview will also let you see the effect of varying the other sliders in
the “Shape detection” section.

<figure>
<img src="previewdeltae1.jpg" title="previewdeltae1.jpg" width="600" />
<figcaption>previewdeltae1.jpg</figcaption>
</figure>

#### Viewing the changes

To see the changes:

- Go to "Mask and modifications" \> "Show modifications without mask"
  (don't forget to select either Standard or Advanced in the Color &
  Light combobox) .

<!-- -->

- You can see the effects of any changes to luminance, contrast, color
  and saturation, as well as any changes to the texture or structure of
  the image.
- You can also see the effect of the transition settings.
  - "Transition value": percentage of the area that will receive the
    full effect of any adjustments before dropping off to zero.
  - "Transition decay": the rate with which the zone of influence
    decreases.
  - "Transition differentiation XY": difference in coverage between
    abscissa and ordinate.

Try out the following and observe the effect.

- Change the "Scope (color tools)". Remember that the scope slider acts
  on deltaE.
- Transition settings.
- Tool settings (luminance, chroma, etc.).

<figure>
<img src="showmodif.jpg" title="showmodif.jpg" width="600" />
<figcaption>showmodif.jpg</figcaption>
</figure>

#### Working on the full image using an Excluding spot

Local adjustments are not limited to local touch-ups. You can also use
the Local Adjustments tool to process the full image.

In Settings enable "Show additional settings":

- Set the "RT-spot shape" to “Full image”
- This will set the 4 delimiters outside of the preview
- It will also set the transition to 100 (you can use another value if
  you wish to generate a gradient, bearing in mind that there are other
  tools for making gradients). You are now ready to use all the tools in
  full-image mode.

<figure>
<img src="fullim1.jpg" title="fullim1.jpg" width="600" />
<figcaption>fullim1.jpg</figcaption>
</figure>

#### Combining tools in a single spot

Most of the local-adjustment tools can be used together in the same
RT-spot. However, combinations of Log Encoding, Tone Mapping and Retinex
should be avoided. This is because the output TIFF or JPG may not
correspond to the Preview particularly when the Preview has been
magnified using the zoom function. Associating any one of the above
tools with the other local-adjustment tools such as Color & Light, does
not pose a problem. If you do wish to use combinations of the 3 tools
mentioned above you can simply add another RT-spot in close proximity to
the first one. For example, the first RT-spot could be dedicated to Log
Encoding, and the second dedicated to Tone Mapping or Retinex. Other
tools can be added to either of the two RT-spots as required.

### Worked examples

#### Example: changing the color of all the green leaves, except for one

##### Changing the color of the leaves

- You can use the "a" and "b" components of "Lab" in the "Color
  correction grid ", by choosing Direct (combobox under the grid) and a
  high value of Strength. Moving the dots on the grid as shown will
  change the color of all the leaves.
- You can adjust if necessary with "Scope (color tools)".
- The other colors in the flower, sky etc., are not modified.

<figure>
<img src="colorleav1.jpg" title="colorleav1.jpg" width="600" />
<figcaption>colorleav1.jpg</figcaption>
</figure>

##### Restoring the green color to one of the leaves

- Add a second RT-spot (Add in the Settings module).
- Choose "Spot method" = "Excluding spot".
- Move the RT-spot to the leaf to be changed and expand the spot well
  beyond the edges of the leaf.
- Adjust Scope (under the Excluding heading in Settings) until you get
  the desired effect.
- If you wish, you can use the Excluding spot in the same way as a
  normal RT-spot and add tools such as Denoise, Blur, etc. (i.e. it not
  only “excludes" the effect of the adjacent spot, but it also allows
  you to use it in the same way as normal spot for the area it
  encompasses).

<figure>
<img src="excluding1.jpg" title="excluding1.jpg" width="600" />
<figcaption>excluding1.jpg</figcaption>
</figure>

#### Correcting red-eye and removing sensor defects

3 steps: preparation, RT-spot adjustment, red-eye removal.

##### Preparation

- Choose a large area around the eye.
- Put the RT-spot on the red area of the eye (pupil).
- Set 4 Lockable Color Pickers so that you can see the changes.

<figure>
<img src="redeye_prepare1.jpg" title="Redeye_prepare1.jpg"
width="600" />
<figcaption>Redeye_prepare1.jpg</figcaption>
</figure>

##### Adjusting the RT-spot

- Add the Color & Light tool.
- Press the "Preview deltaE" button in Settings.
- Adjust the RT-spot to obtain the desired level of selection.
  - In this example we have chosen to reduce the spot size to 14.
  - "Scope (color tools)" = 18.

<figure>
<img src="redeye_previewde1.jpg" title="Redeye_previewdE1.jpg"
width="600" />
<figcaption>Redeye_previewdE1.jpg</figcaption>
</figure>

##### Removing the red color

- In the Color and Light tool, reduce the chrominance to -100.
- Observe the result.
  - The pupil of the eye has almost no dominant color anymore.
  - The iris, cornea and facial skin are unchanged.
  - You may need to change the "Transition value" (lower it) and
    "Transition decay" (increase it) in "Settings" depending on the
    case.

<figure>
<img src="redeye1.jpg" title="Redeye1.jpg" width="600" />
<figcaption>Redeye1.jpg</figcaption>
</figure>

##### Removing sensor defects or spots

The principle is the same as above for removing small sensor faults but
in this example we will use different tools.

- Either CBDL (Contrast By Detail Levels),
- or Wavelet Pyramid2 \> “Contrast by level” (Advanced).
- In both cases, reduce the contrast for the lower levels of
  decomposition.
- Adjust "Blur levels" if necessary (Wavelet Pyramid1).
- Use a low "Transition value" (less than 20) and high "Transition
  decay" (greater than 15) in the Settings module.
- The minimum size of the RT-spot for the CBDL and Wavelet Pyramid2
  decomposition to function is 32x32 pixels. There are workarounds such
  as the use of transitions and deltaE to deal with defects smaller than
  the spot.

Example: removing multiple spots using Wavelet Pyramid2.

- Looking at the image below, we can see that it is blotchy.

<figure>
<img src="blotches.jpg" title="Blotches.jpg" width="600" />
<figcaption>Blotches.jpg</figcaption>
</figure>

- A possible solution:
  - Activate the tool Local Contrast & Wavelets.
  - Choose Advanced in the first combobox and then Wavelets in the
    second combobox.
  - Adjust Scope to 20.
  - Go to Pyramid2 and activate "Contrast by level".
  - Set high values of "Attenuation response", Offset and "Chroma
    levels" (if necessary).
  - Activate the "Contrast by level" curve and reduce the contrast for
    the lower levels.

<figure>
<img src="blotchesless1.jpg" title="Blotchesless1.jpg" width="600" />
<figcaption>Blotchesless1.jpg</figcaption>
</figure>

#### Dodging and Burning

In many portraits, or photos where light falls directly on the skin, an
unpleasant contrast-enhancement phenomenon occurs. Some parts of the
skin are slightly overexposed, while others are slightly underexposed.

- Traditionally this problem is treated with masks and layers and there
  are numerous tutorials for doing this with the GIMP and Photoshop (c).
  You could probably use RawTherapee's Local Adjustments masks also.
- Here we are going to use the Original Retinex concept (based on Ipol
  research). It was developed in the 1970s and was originally designed
  for this sort of application and not for the way it has been
  subsequently used in Rawtherapee and elsewhere. We are going to:
  - Use one or more adjustable-threshold Laplacian functions (see note
    below).
  - Solve the Poisson equation (PDE - Partial Derivative Equation).
  - Balance the luminance values.

Note: Laplacian operators are used because they are particularly good at
detecting fine details and Poisson equations are used to solve the
partial differential equation generated by the Laplacian and make the
tool usable. But you do not need to understand how they work to use this
tool!

There are 3 steps: Preparation, Laplacian settings and preview, Result

##### Preparation

- The deltaE adjustment, the Scope (make sure you use the Original
  Retinex Scope) and the transition adjustment principles are identical
  to the previous examples and won't be repeated here.
- The portrait we are going to use has had the eyes masked for
  confidentiality reasons.
- Choose "Add tool to current spot" \> Soft Light & Original Retinex \>
  Advanced \> Original Retinex.

<figure>
<img src="dodgeburn1.jpg" title="Dodgeburn1.jpg" width="600" />
<figcaption>Dodgeburn1.jpg</figcaption>
</figure>

##### Adjusting the Laplacian threshold and viewing the changes

- Adjust the Strength slider (which takes into account the threshold of
  the first Laplacian operator).
- Adjust the "Laplacian threshold deltaE" slider (which takes into
  account the deltaE of the image to act on a second Laplacian
  operator). This processing is upstream of the Scope algorithms and can
  take into account differences in the background.
- View the modifications by choosing: "Show Fourier process" \> "Show
  modifications without mask".

<figure>
<img src="dodgeburnshow1.jpg" title="Dodgeburnshow1.jpg" width="600" />
<figcaption>Dodgeburnshow1.jpg</figcaption>
</figure>

##### Results

<img src="dodgeburnmodif1.jpg" title="Dodgeburnmodif1.jpg" width="600"
alt="Dodgeburnmodif1.jpg" /> A similar algorithm is used in the Dynamic
Range & Exposure tool. It can be used to process images with large
differences in exposure that are often globally underexposed.

#### Making A Graduated Filter Based On Luminance Chrominance And Hue

##### Preparation

- Choose the flower image that was used in the first example.
- Identify 7 points with the "Lockable color picker".
- Add the Color & Light tool to the current spot and select “Advanced”
  mode.

<figure>
<img src="gradprepa1.jpg" title="gradprepa1.jpg" width="600" />
<figcaption>gradprepa1.jpg</figcaption>
</figure>

Raw file (Creative Common Attribution-share Alike 4.0):
[2](https://drive.google.com/file/d/1X2g73FqzQl7-WRfzhF7zHa7XWwnKcwtx/view?usp=sharing)

##### Making a graduated filter

Arbitrarily we have chosen the following settings:

- Luminance gradient strength = -0.44
- Chrominance gradient strength = -1.13
- Hue gradient strength = 2.69
- Gradient angle = -87.6
- Scope (color tools) = 30
- Feather gradient(settings) = 25

<figure>
<img src="gradlch1.jpg" title="gradLCH1.jpg" width="600" />
<figcaption>gradLCH1.jpg</figcaption>
</figure>

##### Changing the default settings

- Try to gradually change "Scope (color tools)" by increasing the value
  to 70 then 75, 80, 85, 90 and 100.
- Change "Feather gradient" in the Settings \> Transition Gradient
  module and note the variations.
- You can also change the values of the gradients (L, C, H, angle) in
  the Graduated Filter section of the Color & Light tool).
- If you wish, you can also change the values of Color & Light.

<figure>
<img src="gradlchscopefeather1.jpg" title="gradLCHScopeFeather1.jpg"
width="600" />
<figcaption>gradLCHScopeFeather1.jpg</figcaption>
</figure>

#### How to change Local Contrast & Clarity and Sharp Mask

I will only elaborate on the concept of Local contrast with wavelets. This tool is a combination of several wavelet-based tools. It's an "all-in-one" solution. It doesn't use any masks or layers and relies solely on signal processing.

I started with the algorithm I developed around 2012 or 2013 (which has been adopted by other open-source software) and the "Contrast enhancement" version in Abstract Profiles.

[See Contrast Enhancement - Abstract Profiles](color_management/#contrast-enhancement)

The essential improvement is the ability, if necessary, to modify the amplitude (the gradient) of the local contrast according to the level of decomposition. With the default value (1) the settings should be suitable in the vast majority of cases. However, for example, to accentuate the dramatic effect of a sky, you can increase this gradient on the higher decomposition levels up to 512x512 pixels or 1024x1024 pixels. Or, of course, do the opposite.  

The general problem with any algorithm that uses wavelets is that of halos and artifacts. I'm not saying they don't exist at all, but I think they are well controlled.

Note that this algorithm is an "all-in-one" solution... It combines (and improves upon) what you can find in Contrast By Details Levels, Wavelet Levels (with edge sharpness), and Contrast Enhancement (AP). You will notice that the "Clarity" function is directly accessible.  

The 3 key points for using the set are:
* the double slider "Wavelet levels"
* the curve "Local contrast"
* merge luma (Clarity).

<figure>
<img src="sewavcont.jpg" title="sewavcont.jpg" width="600" />
<figcaption>Local contrast using Wavelets</figcaption>
</figure>

##### Wavelets levels

Range of wavelet levels used throughout the Wavelets module.

The numbers associated with Top-left and Bottom-left represent the minimum decomposition levels used. The numbers associated with Top-right and Bottom-right represent the maximum decomposition levels used.

If the respective values ​​of Top-left and Bottom-left, or Top-right and Bottom-right, are different, a transition is applied.
* 0 - means a 2x2 pixels decomposition. 
* 1 - means a 4x4 pixels decomposition.
* 2 - means a 8x8 pixels decomposition.
* 3 - means a 16x16 pixels decomposition.
* 4 - means a 32x32 pixels decomposition.
* 5 - means a 64x64 pixels decomposition.
* 6 - means a 128x128 pixels decomposition.
* 7 - means a 256x256 pixels decomposition.
* 8 - means a 512x512 pixels decomposition.
* 9 - means a 1024x1024 pixels decomposition.

The values ​​8 and 9 assume, in order to be perceived in the Preview, that the size of the Preview is sufficient.

##### Show modified areas with Wavelets

Wavelet decomposition: Shows the changes made by the wavelet settings (only luminance).\n\nYou see the changes made to the decomposed image, factoring in the deltaE and transitions, when you modify the wavelet settings. Note that this is different from Wavelet levels in the Advanced tab in which you always see the results of the decomposition.

##### Prevent decomposition contrast high & low light

Allows you to avoid, for all 'wavelet' functions which implement contrast, modifying the very low lights and the very high lights.

##### Settings

**Attenuation response**

The effect of the local contrast adjustment is stronger for medium-contrast details and weaker for high and low-contrast details.

This slider controls how quickly the effect dampens towards the extreme contrasts.

The higher the value of the slider, the wider the range of contrasts that will receive the full effect of the local contrast adjustment and the higher the risk of generating artifacts.\nThe lower the value, the more the effect will be pinpointed towards a narrow range of contrast values.

**Offset**

Offset modifies the balance between low contrast and high contrast details.

High values will amplify contrast changes to the higher contrast details, whereas low values will amplify contrast changes to low contrast details.

By using a low Attenuation response value you can select which contrast values will be enhanced.

**Gradient levels**

Adjust local contrast for high and medium decomposition levels.
* Medium corresponds to a Top-right value greater than 3. 
* High corresponds to a Top-right value greater than 5.

##### Clarity & Sharp Mask

This algorithm combines two decompositions of the same image and recombines them using different processes, It involves significant system resources.
* From levels 0 to Bottom-right = 4 (included) 'Sharp mask' is enabled.
* Starting from levels 5 – Top-right or Bottom-right and above: 'Clarity' is enabled.
* In the case of the 'Sharp Mask' the change of level is similar to changing the radius in the 'Unsharp Mask'. 
* For 'Clarity' a level change results in a greater or lesser three-dimensional volume effect on the image. 

**Merge luma**

'Merge luma' is used to select the intensity of the desired effect on luminance.

**Merge chroma**

*'Merge chroma' is used to select the intensity of the desired effect on chrominance.

##### An example

<figure>
<img src="contex.jpg" title="contex.jpg" width="800" />
<figcaption>Local contrast using Wavelets Example</figcaption>
</figure>

Raw file (Creative Common Attribution-share Alike 4.0):
[14](https://drive.google.com/file/d/1GdqejdnbW1kJFNY6y9sdQDlF2rCEGMCu/view?usp=sharing)

pp3 file: [Pagodas 0 pp3](dsc1629-0.pp3 "dsc1629-0.pp3")

[See Tutorial Game Changer - Pagodas](/tutorials/#pagodas---how-to-process-local-contrast---clarity)

###### Settings 

#### Six ways to change the exposure and lift the shadows

This example is for demonstration purposes only so that we can see the
various (non-exhaustive) possibilities for adjusting exposure. The
settings are arbitrary.

- The image is a difficult one with deep shadows and a central area that
  is almost overexposed.
- Five possible methods are shown with arbitrary settings:
  - Shadows/Highlights
  - Tone Equalizer
  - TRC (Tone Response Curve)
  - Log Encoding
  - Exposure (PDE algorithms & Exposure)
  - Generalize Hyperbolic Strech (GHS)

We could also have used:

- Contrast curves,
- or lifted the shadows with "Lightness" (Color and Light),
- or used a graduated luminance filter.

##### Preparation

- Add a spot as shown in the image below.
- Set the "Scope (color tools)" slider to 50 (this value will be used by
  the Shadows/Highlights tool when it is added and we will use the
  separate Scope sliders for each of the Log Encoding and Exposure
  tools).
- Try varying this value between 20 and 100

<figure>
<img src="shadows-prepa1.jpg" title="shadows-prepa1.jpg" width="600" />
<figcaption>shadows-prepa1.jpg</figcaption>
</figure>

Raw file (Rawtherapee - Creative Common Attribution-share Alike 4.0):
[3](https://drive.google.com/file/d/1ziux382pWgdYa4jySimwKaKnK_KdDhno/view?usp=sharing)

##### Using Shadows/Highlights

- “Add tool to current spot” \> "Shadows/Highlights & Tone Equalizer" \>
  "Standard"
- Select Shadows/Highlights in the combobox.
- Try changing "Shadows tonal width" and "Highlights".

<figure>
<img src="shadows-sh1.jpg" title="shadows-sh1.jpg" width="600" />
<figcaption>shadows-sh1.jpg</figcaption>
</figure>

##### Using the Tone Equalizer

- “Add tool to current spot” \> "Shadows/Highlights & Tone Equalizer".
- Select Tone Equalizer in the combobox.
- Try also sliders 2, 3 et 4.

<figure>
<img src="shadows-toneeq2.jpg" title="shadows-toneeq2.jpg"
width="600" />
<figcaption>shadows-toneeq2.jpg</figcaption>
</figure>

##### Using the Tone Response Curve (TRC)

- “Add tool to current spot” \> "Shadows/Highlights & Tone Equalizer".
- Select TRC.
- Increase the "Slope" to 150 and then come back to 60.
- Try reducing and then increasing the gamma and observe the effect.

<figure>
<img src="shadows-trc1.jpg" title="shadows-trc1.jpg" width="600" />
<figcaption>shadows-trc1.jpg</figcaption>
</figure>

##### Using Log Encoding

- “Add tool to current spot” \> Log Encoding.
- Note that the Scope slider in this case is in the Log Encoding tool:
  set Scope to 50.
- Click on the Automatic button.
- Adjust the “Target gray point” (now called “Mean luminance (Yb%)” in
  the Viewing Conditions panel).

<figure>
<img src="shadows-elog1.jpg" title="shadows-elog1.jpg" width="600" />
<figcaption>shadows-elog1.jpg</figcaption>
</figure>

##### Using Exposure

- “Add tool to current spot” \> "Dynamic Range & Exposure".
- Select Standard.
- Adjust "Exposure compensation ƒ" (a Laplacian and a Fourier transform
  will be applied).
- Set the Exposure Tools sliders to Black = -1500, Shadows = 50.
- By default "Highlight compression" is 20. Vary it to see the effect.
- Play around with the above settings to get a feeling for how the tool
  works.

<figure>
<img src="shadows-expo1.jpg" title="shadows-expo1.jpg" width="600" />
<figcaption>shadows-expo1.jpg</figcaption>
</figure>

##### Using Generalize Hyperbolic Stretch - GHS
[GHS]({{% relref "#generalized-hyperbolic-stretch-and-michaelis-menten" %}})
##### Recommendations

For portraits and images with low color contrast:

- Use the Exposure slider with care because the algorithm (which is
  similar to the one used in the Exposure tab) is not well adapted to
  cases such as portraits which have subtle color variations in skin
  tones. The algorithm was improved recently (July 5, 2020) by the
  addition of a Laplacian operator to resolve the differences in
  contrast but it is still not the best solution for these cases.

Despite the improvements and in general:

- The performance of the Exposure algorithm isn’t optimal. However,
  users are used to it so I have implemented some workarounds to improve
  the behaviour.
- Some simple alternatives include:
  - The Tone Equalizer - in Shadows/Highlights & Tone Equalizer
  - The Tone Response Curve (TRC) - also in Shadows/Highlights & Tone
    Equalizer (with the Equalizer in Standard mode). You can increase
    the Slope to linearly open up the shadows. You can also use the
    Gamma slider to lighten the bright areas.

If you do use Exposure, then it is recommended (but not mandatory) to
change the parameters of "Shape detection" in Settings as follows:

- Increase "ΔE-scope threshold".
- Reduce "ΔE decay".
- Set "ab-L balance (ΔE)" to L.
- Adjust "Scope (color tools)" if necessary.

##### An Evaluation Of The Dynamic-range Capabilities Of Tools In Selective Editing

TIF file (Creative Common Attribution-share Alike 4.0):
[4](https://drive.google.com/file/d/1vAzFY7Qh8MdJ882J_4JeO_cnpGELzD8D/view?usp=sharing)

###### Original image - 25Ev - unprocessed

Note:

- The shadows lack detail.
- Approximately 40% of the image is 100% white.
- The restored black-to-white dynamic range is around 12 to 13 Ev.

<figure>
<img src="sweep-rgb-neutral.jpg" title="sweep-rgb-neutral.jpg"
width="600" />
<figcaption>sweep-rgb-neutral.jpg</figcaption>
</figure>

###### Image with Local Adjustments - Log Encoding

Note:

- The shadows and highlights occupy the entire visible range from L=1 to
  L=99.8 (scale 0 – 100).
- The colors appear evenly distributed as a function of luminance. The
  use of this L\*a\*b\* tool does not impact the dynamic range.
- The dynamic range of the blacks, whites and color is restored to 25Ev.
- Note the use of "White distribution = 90".

<figure>
<img src="sweep-rgb-log.jpg" title="sweep-rgb-log.jpg" width="600" />
<figcaption>sweep-rgb-log.jpg</figcaption>
</figure>

###### Image with Local Adjustments - Color Appearance - Cam16

Note:

- The shadows and highlights occupy part of the visible range from L=1
  to L=80.9 (scale 0 – 100).
- The colors appear evenly distributed as a function of luminance. The
  use of this L\*a\*b\* tool does not impact the dynamic range.
- The dynamic range of the blacks, whites and color is restored to 25Ev.
- Note the use of "White distribution = 100" and "Black distribution =
  60".

Note the difference with Log Encoding, which changes the luminance
distribution due to the action of Ciecam.

- Using the TRC Gamma slider substantially restores the Log Encoding
  image above.
- Alternatively, you can use the Contrast J and Luminance J sliders.

<figure>
<img src="sweep-rgb-cie.jpg" title="sweep-rgb-cie.jpg" width="600" />
<figcaption>sweep-rgb-cie.jpg</figcaption>
</figure>

###### Image with Local Adjustments - Tone Equalizer

Note:

- The shadows and highlights occupy part of the visible range from L=1
  to L=82 (scale 0 – 100).
- The colors do not appear to be evenly distributed as a function of
  luminance. The use of this L\*a\*b\* tool, slightly impacts the
  dynamic range.
- The dynamic range of the rendered blacks, whites and color is
  approximately 15Ev to 18Ev.
- Note the use of a fifth slider to handle very bright highlights:
  5(lightest) = -100.

<figure>
<img src="sweep-rgb-te.jpg" title="sweep-rgb-te.jpg" width="600" />
<figcaption>sweep-rgb-te.jpg</figcaption>
</figure>

###### Image with Local Adjustments - Dynamic Range & Exposure

Note:

- The shadows and highlights occupy part of the visible range from L=4
  to L=95.8 (scale 0 – 100).
- The colors do not seem to be evenly distributed in accordance with the
  luminance and color (color shift in the blues). The use of this
  L\*a\*b\* tool slightly impacts the dynamic range.
- The dynamic range of the rendered blacks, whites and color is
  approximately 17Ev to 20Ev.
- Note the complexity and lack of intuitiveness of the adjustments and
  the long processing time.

<figure>
<img src="sweep-rgb-drexp1.jpg" title="sweep-rgb-drexp1.jpg"
width="600" />
<figcaption>sweep-rgb-drexp1.jpg</figcaption>
</figure>

###### Image with Local Adjustments - Dehaze

Note:

- The shadows and highlights occupy part of the visible range from L=1
  to L=88 (scale 0 – 100). There is not much detail in the shadows
- The colors do not appear to be evenly distributed in accordance with
  the luminance. The use of this L\*a\*b\* tool, impacts the dynamic
  range.
- The dynamic range of the rendered blacks, whites and color is
  approximately 15Ev to 16Ev.

<figure>
<img src="sweep-rgb-deha.jpg" title="sweep-rgb-deha.jpg" width="600" />
<figcaption>sweep-rgb-deha.jpg</figcaption>
</figure>

###### Image with Local Adjustments - Shadows/Highlights

Note:

- The shadows and highlights occupy part of the visible range from L=1
  to L=70 (scale 0 – 100).
- The colors do not appear to be evenly distributed as a function of the
  luminance. The use of this L\*a\*b\* tool, impacts the dynamic range.
- The dynamic range of the rendered blacks, whites and color is
  approximately 12Ev to 14Ev.

<figure>
<img src="sweep-rgb-sh.jpg" title="sweep-rgb-sh.jpg" width="600" />
<figcaption>sweep-rgb-sh.jpg</figcaption>
</figure>

###### Image with Local Adjustments - Tone Response Curve - TRC - and TRC Cam16

Note:

- The shadows and highlights occupy part of the visible range from L=1
  to L=95.6 (scale 0 – 100).
- The colors do not appear to be evenly distributed as a function of
  luminance. The use of this L\*a\*b\* tool only slightly impacts the
  dynamic range.
- The dynamic range of the rendered blacks, whites and color is
  approximately 12Ev to 16Ev.

<figure>
<img src="sweep-rgb-trc.jpg" title="sweep-rgb-trc.jpg" width="600" />
<figcaption>sweep-rgb-trc.jpg</figcaption>
</figure>

- TRC Cam16

Note:

- The shadows and highlights occupy part of the visible range from L=1
  to L=98.8 (scale 0 – 100).
- The colors appear evenly distributed as a function of luminance. The
  use of this L\*a\*b\* tool only slightly impacts the dynamic range.
- The dynamic range of the rendered blacks, whites and color is
  approximately 20Ev to 22Ev.
- Note the use of "White distribution = 80" and "Smooth highlights".

<figure>
<img src="sweep-rgb-trc-cam16.jpg" title="sweep-rgb-trc-cam16.jpg"
width="600" />
<figcaption>sweep-rgb-trc-cam16.jpg</figcaption>
</figure>

###### Image with Local Adjustments - Tone Response Curve and Highlight attenuation - Slope based - Cam16

- Slope based

Note:

- The shadows and highlights occupy part of the visible range from L=2.4
  to L=98.3 (scale 0 – 100).
- The colors appear evenly distributed as a function of luminance. The
  use of this L\*a\*b\* tool only slightly impacts the dynamic range.
- The dynamic range of the rendered blacks, whites and color is
  approximately 23Ev to 24Ev.
- Note the use of "White distribution = 100" and "Black distribution =
  89"

<figure>
<img src="sweep-rgb-trc-slope1-based.jpg"
title="sweep-rgb-trc-slope1-based.jpg" width="600" />
<figcaption>sweep-rgb-trc-slope1-based.jpg</figcaption>
</figure>

###### Image with Local Adjustments - Tone Response Curve and Highlight attenuation - Sigmoid based - Cam16

- Sigmoid based

Note:

- The shadows and highlights occupy part of the visible range from L=3.0
  to L=98.1 (scale 0 – 100).
- The colors appear evenly distributed as a function of luminance. The
  use of this L\*a\*b\* tool only slightly impacts the dynamic range.
- The dynamic range of the rendered blacks, whites and color is
  approximately 21.5Ev to 22.5Ev.
- Note the use of "White distribution = 100" and "Black distribution =
  89"

<figure>
<img src="sweep-rgb-trc-sigmoid-based.jpg"
title="sweep-rgb-trc-sigmoid-based.jpg" width="600" />
<figcaption>sweep-rgb-trc-sigmoid-based.jpg</figcaption>
</figure>

###### Image with Local Adjustments - Dynamic Range & Exposure: Exposure only

Note:

- The shadows and highlights occupy part of the visible range from L=1
  to L=70.9 (scale 0 – 100).
- The colors, as a function of luminance, don't appear to be faithful or
  evenly distributed. The use of this L\*a\*b\* tool severely impacts
  the dynamic range.
- The dynamic range of the restored blacks, whites and color is
  approximately 11Ev to 14Ev.

<figure>
<img src="sweep-rgb-exp.jpg" title="sweep-rgb-exp.jpg" width="600" />
<figcaption>sweep-rgb-exp.jpg</figcaption>
</figure>

###### Image with Local Adjustments - Tone Mapping

Note:

- The shadows and highlights occupy part of the visible range from L=1
  to L=72.4 (scale 0 – 100).
- The colors do not appear to be evenly distributed as a function of the
  luminance. The use of L\*a\*b\*, severly impacts the dynamic range.
- The dynamic range of the restored blacks, whites and color is
  approximately 10Ev to 14Ev.

<figure>
<img src="sweep-rgb-tm.jpg" title="sweep-rgb-tm.jpg" width="600" />
<figcaption>sweep-rgb-tm.jpg</figcaption>
</figure>

###### Summary

- Tools using the log-encoding algorithm, such as the local-adjustments
  Log Encoding or Color-Appearance Cam16 tools, fully reproduce the
  color and dynamic range but they are not intuitive to use.
- The local-adjustments TRC tool in Cam16 almost completely reproduces
  the color and dynamic range. The tool should be suitable for a number
  of cases. Operation is quite simple and intuitive.
- The local-adjustments Tone Equalizer ensures fairly good restitution
  for this image. The tool should be suitable for most camera images.
  It's intuitive, but requires at least 5 settings.
- The local-adjustments Dynamic Range & Exposure tool has very good
  dynamic range reproduction, although some colors drift. Adjustments
  are not very intuitive, and the system is slow.
- The local-adjustments TRC tool provides average rendition for this
  image but should be suitable in a number of cases. Operation is simple
  and intuitive.
- The local-adjustments Slope tools, and Sigmoid tools provides average
  or very good rendition for this image. These tools should be suitable
  in a number of cases. Operation is quite simple, but not very
  intuitive.They require the use of an upstream tool like TRC tools.
- The local-adjustments Dehaze tool can have a significant effect on the
  dynamic range when it is used for its intended purpose i.e. dehaze.
- The other local-adjustment tools Exposure and Shadows/Highlights are
  of little interest in so far as dynamic range is concerned
- Curiously, one of the tools that is "dedicated" (in theory) to dynamic
  range reduction, i.e. Tone Mapping, doesn't give good results.

#### Processing a hazy image

We are going to process a very hazy image by first applying the global
Haze Removal tool in the Detail tab and then touch up the sky and
horizon using Retinex in Local Adjustments.

##### Original Image

<figure>
<img src="haze.jpg" title="haze.jpg" width="600" />
<figcaption>haze.jpg</figcaption>
</figure>

Raw file:(Pixls.us - Creative Common Attribution-share Alike 4.0):
[5](https://drive.google.com/file/d/1tc9TxHGwYnVQ2OwiTZIiszDOEXfDjkh6/view?usp=sharing)

##### Processing with the Haze Removal tool (Detail tab in the main menu)

We could have used the Dehaze tool in Local Adjustments but when you
look at the image there is a lot of haze in the background and the hills
so it is better to use a two-step approach.

<figure>
<img src="haze-dehaze.jpg" title="haze-dehaze.jpg" width="600" />
<figcaption>haze-dehaze.jpg</figcaption>
</figure>

##### Additional processing with local Retinex

- Select "Add tool to current spot" \> Dehaze & Retinex \> Advanced.
- Try varying the settings.
- Adjust the "Transmission map" curve in Advanced Retinex Tools if
  necessary by increasing the attenuation on the right side of the
  curve.
- Now look at the difference in the hills and the sky on the horizon!

<figure>
<img src="haze-reti1.jpg" title="haze-reti1.jpg" width="600" />
<figcaption>haze-reti1.jpg</figcaption>
</figure>

#### Using the Denoise module

There are several ways of using this tool:

- On selected areas to refine any denoising adjustments carried out with
  the Noise Reduction module in the Detail tab. In this case keep the
  Detail tab noise reduction to a minimum.
- By processing the whole image using the Denoise module in Local
  Adjustments and excluding parts of the image with an Excluding spot.
- By using it on its own to reduce noise in low-noise images. For
  example, to remove noise from the sky or a face.
- By using it on its own to reduce the noise in a selected area and
  deliberately leaving the noise in the rest of the image for artistic
  purposes.

We are going to look at an example using this last case.

The image of the young girl is particularly noisy and has strong
chromatic noise.

<figure>
<img src="denoise-prepa1.jpg" title="denoise-prepa1.jpg" width="600" />
<figcaption>denoise-prepa1.jpg</figcaption>
</figure>

Raw file (Creative Common Attribution-share Alike 4.0):
[6](https://drive.google.com/file/d/1poZporRNILcYfab1Y_f1i-SIEB4acn6L/view?usp=sharing)

##### Zoom 100%

<figure>
<img src="denoise-zoom1.jpg" title="denoise-zoom1.jpg" width="600" />
<figcaption>denoise-zoom1.jpg</figcaption>
</figure>

##### Which settings should we use for denoising?

- The position of the spot and its size are important. We are going to
  choose a part of the face with strong chromatic noise and use a large
  “Spot size” for the RT-spot .
- The choice of the Scope parameter is also important. In this case,
  where the noise occupies almost the whole color spectrum (red, green,
  blue, yellow), a high Scope value must be chosen (90 in this case). If
  on the other hand the image to be processed has mainly luminance
  noise, then the "usual" Scope value should be chosen, i.e. around 30,
  to allow the algorithm to differentiate the action according to the
  colors.
- There are several differences in the Local Adjustments denoise
  function compared to the global Noise Reduction module in the Detail
  tab.
  - You can use a curve to adjust the luminance noise level as a
    function of the level of detail (from 0 to 6 depending on the
    position on the abscissa of the curve).
  - A distinction is made depending on the level of detail i.e. if
    levels 3 and above are greater than 20% of the ordinate of the
    curve, the luminance noise reduction will be more aggressive.
  - The "white - black" differentiation for luminance is handled by an
    equalizer, rather than gamma.
  - There is the possibility to distinguish between "Fine chroma"
    (impulse noise and low chrominance noise for levels 0 to 4) and
    "Coarse chroma" (packets of noise, blotches for levels 5 and 6) .
  - There is a "red-green/blue -yellow" equalizer which can be useful
    for low-noise images.
  - There is an extra "Chroma detail recovery" slider using DCT (a
    Fourier-related discrete cosine transform).
  - There is an added "Luminance & chroma detail threshold (DCT)" slider
    to differentiate the action based on edges ("Edge detection").

<figure>
<img src="denoise1.jpg" title="denoise1.jpg" width="600" />
<figcaption>denoise1.jpg</figcaption>
</figure>

##### A complex noise reduction problem: how to differentiate between uniform areas and areas with texture or detail?

Isolating a subject against a background is a common problem in
photography. The subject can be an animal, a plant, a person and the
background the sky, grass, a forest, a wall etc. The problem is a
complex one for noise reduction software because the algorithm usually
"ignores" the difference between the subject and the background. This
means that removing noise in the background will cause a loss of detail,
contrast and color in the subject.

###### An example using Andy Astbury's harvest mouse image

I chose this image, with the agreement of its author Andy Astbury,
because not only is it excellent (the animal stands out very well
against a gray background) but it is also slightly noisy. Removing the
noise using only the Noise Reduction tool in the Detail tab will
inevitably lead to a loss of detail and a reduction in contrast and
saturation in the mouse.

Raw file: (Copyright Andy Astbury - Creative Common Attribution-share
Alike 4.0)
[7](https://drive.google.com/file/d/1uND8pqgfxxaBhs554RnCvOS5NI3KsWT5/view?usp=sharing)

PP3 file
[8](https://drive.google.com/file/d/1qwslSsbXlM4Ns8A_2t_8_QSMWfsIuAR8/view?usp=sharing)
The pp3 file is provided as a guide to the tools and possible settings
that can be used in cases like this. They are not necessarily the
"right" settings.

Traditionally noise is removed in Rawtherapee using the Noise Reduction
module (Detail tab). If we try to remove the luminance and chrominance
noise in the background we end up with settings (not shown in the
screenshot) in the order of:

- Luminance slider = 65
- Chrominance slider - Master = 20

Certainly the background will be perfect, but our little harvest mouse
will become dull and washed out. So how can we go about denoising this
image?

Method outline:

- We will use a two-step approach.
  - In the first step we will remove the noise in the details that we
    want to preserve (the harvest mouse) using the Noise Reduction
    module in the Detail tab, paying particular attention to its eye and
    tail. Note that in other images this step may also reduce large
    noise packets.
  - Because the perception of noise is similar to the principles of
    color appearance models, it will be more visible on a gray
    background than on a darker background (especially the chrominance
    noise). The same principle applies for the brighter parts of the
    image. It is therefore advisable to adjust the tonal contrast in
    conjunction with noise reduction. This will enhance the image and
    reduce the perceived noise at the same time.
  - In the second step, we will treat the noise with some of the tools
    available in Local Adjustments and in particular, the five tools
    outlined below:
    - The mask, which will allow us to differentiate between the
      detailed parts of the image (mouse, vegetation) and the
      background.
    - The “Denoise hue equalizer” which will allow us to differentiate
      the denoising between the color of the mouse and the background.
    - The Scope slider (i.e. use deltaE) which allows us to
      differentiate the action based on differences in color.
    - The "Luminance detail recovery (DCT)" slider (abbreviated to “Luma
      detail recovery” in the current interface) and the “Luminance -
      Chroma detail theshold” (abbreviated to “Luma-chro detail
      threshold” in the current interface) slider in Edge Detection,
      which uses a noise reduction technique (Fourier) based on the
      difference between the original image and the image that has been
      denoised using wavelets.
    - Patch-based denoising (also called non-local means), is another
      denoising algorithm based on pixel and patch similarity. It allows
      you to differentiate the denoise between areas with detail and
      texture (e.g. field mouse, vegetation etc.) and uniform areas
      (background).
  - Finally we will adjust the saturation, local contrast, etc.

Note that this document is for didactic purposes and the settings are
designed to clearly demonstrate the different steps rather than to
produce a beautiful image.

###### First step: noise reduction and tonal contrast adjustment

The image below only shows the tone equalizer settings and not the
luminance and chrominance denoise settings that were made in the Noise
Reduction module (Detail tab). Lockable Color Pickers have been placed
on the eye, the fur, the vegetation, and the tail.

- Add a new RT-spot and choose "Full image" in settings. Also in the
  Settings panel, go to "Mask and merge" and set "Background color/luma
  mask" to 0 (this will make it easier to distinguish the variations in
  luminance values).
- Click on "Add tool to current spot" and select Shadows/Highlights &
  Tone Equalizer in the drop-down menu. Leave the tool in the default
  Basic mode.
- Position the center of the RT-spot on the gray background.
- Adjust the equalizer sliders to get the best compromise, while at the
  same time adjusting the two sliders in the "Noise Reduction" module
  (Detail tab). Here I used: luminance = 4, chrominance = 6.5 (Method:
  Manual \> Chrominance-Master).

<figure>
<img src="mulot_first.jpg" title="mulot_first.jpg" width="600" />
<figcaption>mulot_first.jpg</figcaption>
</figure>

###### Second step: Local Adjustments, Blur/Grain & Denoise module

- Click on "Add tool to current spot" and select Blur/Grain & Denoise
  and then Denoise. Set the tool to Advanced mode.
- Use the "Luminance denoise" curve.
- For educational purposes and in order to see the effectiveness of the
  various tools you can set this curve to maximum and activate
  Aggressive. You will of course have to bring it back to normal values
  afterwards before continuing.

Familiarize yourself with each of the 5 tools mentioned above, one by
one. For example to see the action of the "Denoise hue equalizer", set
Scope to 100, set the slider “Recovery threshold" to 1 in “Recovery
based on luminance mask" and leave the 3 other sliders at their default
values.

- Adjust the "Denoise hue equalizer" by increasing the noise level for
  the background and decreasing it for the mouse.
- Adjust "Fine chroma" slightly.
- Review the results.

<figure>
<img src="mulot_levelhue.jpg" title="mulot_levelhue.jpg" width="600" />
<figcaption>mulot_levelhue.jpg</figcaption>
</figure>

Next:

- Make a mask (Blur/Grain & Denoise\> Denoise \> Mask and
  modifications).

This mask will be used to differentiate the denoise between the
background and the rest of the image i.e. the harvest mouse and the
vegetation. In this case I used a simple L(L) curve, a gamma adjustment
and the contrast curve, but other images may need to use the LC(H)
curve, “Structure mask strength”, “Smooth radius”, etc...

<figure>
<img src="mulot_mask.jpg" title="mulot_mask.jpg" width="600" />
<figcaption>mulot_mask.jpg</figcaption>
</figure>

<figure>
<img src="mulot_mask2.jpg" title="mulot_mask2.jpg" width="300" />
<figcaption>mulot_mask2.jpg</figcaption>
</figure>

- Activate the mask.
- Expand the "Recovery based on luminance mask" tool.
- Adjust "Recovery threshold" to reveal detail. Note that this tool is
  inactive when set to its default value 1.0. As soon as you move the
  slider, you will see maximum detail and noise which can then be
  reduced by moving the slider to the right.

For other images it may be necessary to adjust:

- "Dark area luminance threshold". The denoise is progressively
  increased from 0% at the threshold setting to 100% at the maximum
  black value (determined by the mask).
- "Light area luminance threshold", The denoise is progressively
  decreased from 100% at the threshold setting to 0% at the maximum
  white value (determined by the mask). In this example, the adjustment
  will allow us to denoise the vegetation as a function of luminance.
- Decay allows you to manage the progressiveness of any changes.
- The two "Gray area" sliders allow you to reapply noise reduction if
  necessary in the "protected" mid-tone area of the mask.

<figure>
<img src="mulot_recovery.jpg" title="mulot_recovery.jpg" width="600" />
<figcaption>mulot_recovery.jpg</figcaption>
</figure>

- Using Scope: here we are on familiar ground. In "Mask and
  modifications" you can use the two selections "Show modified areas
  with mask" and "Show modified areas without mask" to see the effect of
  Scope. Or you can simply adjust Scope and see the effect. In this
  image, with "Equalizer hue" disabled and "Recovery based on luminance
  mask" disabled (“Recovery threshold” set to 1) , the Scope action is
  sensitive between 50 and 100.
- Use of the two sliders "Luminance detail recovery" (now labeled as
  “Luma detail recovery”) and "Luminance & chroma detail threshold" (now
  labeled as “Luma-chro detail threshold”).
  - Gradually increase "Luma detail recovery".
  - Adjust the "Luma-chro detail threshold"in parallel. You will see the
    details reappear.
  - 2 algorithms are possible. The first one uses an internal mask and
    the second one a Laplacian. Each one has its particularities: the
    Laplacian is more selective, but less progressive.

<!-- -->

- Using patch-based denoise (non-local means).
  - What is patch-based denoise? Contrary to the usual filters that
    reduce noise by averaging the values of groups of pixels located
    around a target pixel, non-local means filters average the values of
    all the pixels in the image and weight them according to their
    similarity with the target pixel. This type of filtering reduces the
    loss of detail compared to filters that use local averaging.

<figure>
<img src="mulot_nlmeans.jpg" title="mulot_nlmeans.jpg" width="300" />
<figcaption>mulot_nlmeans.jpg</figcaption>
</figure>

To familiarize yourself with this method it is recommended to:

- Activate "Non-local means only" in Denoise \> Mode.
- Deactivate the mask.
- Set Scope to 100.

In Advanced mode you have 5 sliders:

- Strength
- Detail recovery: allows you to make a preliminary selection between
  uniform and textured areas. The higher the values, the more the
  details will be selected.
- Gamma: allows you to further refine the selection between uniform and
  textured areas. Lower gamma values will reveal more detail and
  texture.
- Maximum patch size: allows you to adapt the size of the "patch" to the
  size of the objects. In theory, the more noisier the image, the larger
  this value should be. In practice, you should look for and minimize
  any artifacts in the transitions between the uniform and textured
  areas.
- Maximum radius size: higher values will theoretically give better
  noise reduction at the expense of increased processing time.

###### Final Adjustment - Saturation and Local Contrast

Add a new RT-spot, centered on the mouse.

Then add 2 tools.

- “Add tool to current spot” \> Vibrance & Warm/Cool \> Basic.
  - Move the Vibrance slider until you get the desired increase in
    saturation.
- “Add tool to current spot” \> Local Contrast & Wavelets \> Wavelets
  \>Advanced.
  - Use "Contrast by level" in Wavelet pyramid 2 , giving priority to
    the first levels.

<figure>
<img src="mulot_wav.jpg" title="mulot_wav.jpg" width="600" />
<figcaption>mulot_wav.jpg</figcaption>
</figure>

###### Other methods and tools

Other methods can be used for the same purpose.

- Using Local Adjustments
  - "Denoise based on luminance mask". Uses the same mask as "Recovery
    based on luminance mask" but increases or decreases the wavelet
    denoise. It acts prior to denoise (as does the "Denoise hue
    equalizer") whereas "Recovery based on luminance mask" acts after
    denoise by comparing the original noisy image and the denoised
    image.
  - "Equalizer white-black" and "Equalizer blue-yellow red-green"are the
    equivalent of the "Luminance curve" in "Noise Reduction" and not
    very efficient here.
  - Guided Filter in Blur/grain & Denoise \> Blur & Noise uses the same
    mask and the same process as "Recovery based on luminance mask" with
    negative values of the Detail slider.
  - Excluding spots allow you to restore the image to the settings prior
    to activating the "Full image" RT-spot.
  - Median in Blur/Grain & Denoise \> Blur & Noise is not very efficient
    here.
  - "Blur levels" in Local Contrast & Wavelets \> Wavelets \> Pyramid1:
    if you want to blur a part of the image according to the level of
    detail.

<!-- -->

- Other Rawtherapee methods (not developed here)
  - Noise Reduction: the "Luminance control" and chrominance curves
    allow some form of selection, but they are not sufficient in this
    particular case.
  - Wavelet levels noise reduction, which includes a "Denoise hue
    equalizer" and makes use of local contrast.

Comparison of Denoise tools
[Comparison of the 3 Rawtherapee noise reduction tools](comparison_of_the_3_rawtherapee_noise_reduction_tools)

###### Summary

Thanks once again to Andy Astbury for this excellent image, which allows
us to demonstrate 5 ways of differentiating noise reduction between
uniform areas and areas with detail.

- Denoise hue equalizer
- Recovery based on luminance mask
- Scope - deltaE
- DCT - Edge detection
- Non-local means

In a difficult image it will probably be necessary to activate all 5
methods to try and find the right balance. The result is a matter of
individual taste and is quite subjective.

It also depends on:

- The background, which is uniform in this example, but may pose
  problems if it contains detail or texture.
- The colors, which are well separated here, but will be more difficult
  to distinguish if they are "mixed".
- DeltaE, which can be affected by chromatic noise, or when the
  separation of the colors is less distinct.
- "Edge detection" which will also be affected by high luminance noise.

###### Selective Editing -  Blur/grain & Denoise > Denoise

Allows you to differentiate the denoising action as a function of image detail and uniform background.

The resulting action is a combination of the noise reduction and that of the mask created by 'Contrast threshold'

On very noisy images you can reduce the mask noise.

You can also differentiate the action with the curve 'Equalizer denoise mask'.

The ratio flat-structured areas allows you to correct the action between the two.

On noisy images and in Raw format, you should use the 'Presharpening denoise' and ‘Postsharpening denoise’ functions for more effective mask action.

<img src="denoise-contrast.jpg" title="Denoise contrast mask" width="300"
alt="Denoise-contrast.jpg" />

###### Locks MadL noise evaluation

Allows you to lock the noise evaluation used for Wavelet denoise.

Wavelet denoise uses MadL (Median absolute deviation luminance) to act on the noise based on a slider or a curve.

Rawtherapee's pipeline design results in different MadL values depending on the preview position and leads to differences in output.

If you enable this checkbox, the value used for global processing will be the preview at that time. This will allow you to prioritize a specific area, background, or face for noise reduction, rather than an overall average value. Of course, you should only check the box once you've selected the area.

###### Behavior when the complexity level is set to 'Advanced'

Shows the actual values of Median Absolute Deviation (MadL).

If 'Locks MadL noise evaluation' is not enabled you can manually change these values which will affect the entire image.

The first levels 0,1,2,3,4,5,6 for the Horizontal direction,7,8,9,10 for the Vertical direction and 14,15,16,17 for the Diagonal, act on the detail noise. The higher levels have more effect on banding noise.
Luminance noise reduction is a function that implements MadL (depending on level and direction) divided by the level of detail (high, medium, low frequency), the actual value of the decomposition at that location (depending on level and direction) and the curve applied by the user according to the level of wavelet decomposition.

######  The checkbox ‘Manual settings’

Allows you to manually control MadL values. Be careful: this function assumes mastery of the concepts of noise reduction using wavelets. Make sure that the initial values for the selected area have been set automatically.

Be careful not to confuse the action on MadL which is a noise estimation only used in Wavelet Denoise where the strength is set in Selective Editing using a curve and a selected level (Aggressive / Conservative). The MadL scale is exponential and not linear.
The first levels of wavelet decomposition provide visible noise reduction, whereas the higher levels have more affect on banding.


<img src="denoise-manual.jpg" title="Denoise Manual settings" width="300"
alt="Denoise-manual.jpg" />

######  Selective Editing – Blur/Grain & Denoise > Blur & Noise

You can blur the background using either:
- Gaussian blur.
- Median
- Guided Filter

To use the functions below you must either use a 'Normal Spot' or 'Full image' - therefore not available in 'Global' mode, because it requires a mask created by the use.
You need to use the functions of the "Recovery based on luminance mask" tool, in conjunction with the chosen blur function. 
A possible example is in Rawpedia (not for blur) ‘A complex noise reduction problem: how to differentiate between uniform areas and areas with texture or detail?’


#### A moment of madness - try wavelets!

##### An example … (don't run away, it isn't as difficult as all that)

Original image, with “Exposure compensation” = +1.5

<img src="amsterdam15.jpg" title="Amsterdam15.jpg" width="600"
alt="Amsterdam15.jpg" /> 

Raw file (Rawtherapee - Creative Common
Attribution-share Alike 4.0):
[9](https://drive.google.com/file/d/1dJ5yiqF-XdLQdKizCDseUxHf34y4AKZ6/view?usp=sharing)

##### The same image with Wavelet level tone mapping

- Leave all settings at their default values.
- Enable the Local Contrast & Wavelets tool (Advanced mode), select
  Wavelets in the combobox just under the “Overall strength” slider (the
  default is Unsharp Mask) and then select Pyramid2.
- Set Scope (Wavelets) to 80.
- Use the settings visible on the screenshot.
- Of course the appearance is subjective so feel free to change the
  settings.
- This version of tone mapping is different from the other algorithms
  implemented in Rawtherapee (Mantiuk for both Tone mapping and Log
  Encoding and Fattal for Dynamic Range Compression) and is specific to
  Rawtherapee Wavelets.

<figure>
<img src="amsterdam15_wavtm1.jpg" title="Amsterdam15_wavtm1.jpg"
width="600" />
<figcaption>Amsterdam15_wavtm1.jpg</figcaption>
</figure>

#### Three ways of increasing texture

For demonstration purposes we will use:

- Tone Mapping (Mantiuk)
- Retinex
- Wavelets

##### Preparation – original image - Venice

<figure>
<img src="texture-normal1.jpg" title="texture-normal1.jpg"
width="600" />
<figcaption>texture-normal1.jpg</figcaption>
</figure>

Raw file (Copyright Sébastien Guyader - Creative Common
Attribution-share Alike 4.0):
[10](https://drive.google.com/file/d/1DASGpHfl_9RDRhbq2_JQVgypgKyrdiBk/view?usp=sharing)

##### Using Tone Mapping

- Create an elliptical RT-spot as shown then “Add tool to current spot”
  \> Tone Mapping.
- Note that the option "Normalize luminance" is checked. This ensures
  that the average and variance of the luminance values are the same as
  in the original image.
- Use Advanced mode and adjust "Edge stopping" and Scale.

<figure>
<img src="texture-tm1.jpg" title="texture-tm1.jpg" width="600" />
<figcaption>texture-tm1.jpg</figcaption>
</figure>

##### Using Retinex

- Using the same RT-spot as above, turn off the Tone Mapping tool and
  then “Add tool to current spot” \> Dehaze & Retinex \> Advanced. Then
  use the settings in the screenshot.
- Note that the option "Normalize luminance" is checked. This ensures
  that the average and variance of the luminance values are the same as
  in the original image.
- Note also that you can enable the "Use Fast Fourier Transform".

<figure>
<img src="texture-reti1.jpg" title="texture-reti1.jpg" width="600" />
<figcaption>texture-reti1.jpg</figcaption>
</figure>

##### Using Wavelets

- Using the same RT-spot as above, turn off the Dehaze & Retinex tool
  and then “Add tool to current spot” \> Local Contrast & Wavelets \>
  Advanced \> Wavelets \> Pyramid2 and then use the settings in the
  screenshot.
- Note the use of "Compression by level", the values of "Attenuation
  response", "Balance threshold" and "Compress residual image".
- Try "Contrast by level".
- Also try "Directional contrast",
- or a combination of these parameters.

<figure>
<img src="texture-wav1.jpg" title="texture-wav1.jpg" width="600" />
<figcaption>texture-wav1.jpg</figcaption>
</figure>

#### Merging Layers Using Blend Modes

You can use "Merge file" in the Color & Light tool (Advanced mode) to
simulate the effect of merging layers. Each RT-spot can be thought of as
a layer and the "Merge file" function allows you to merge up to 2
RT-spots with the original image.

- The first "layer" is called Original and corresponds (in the same way
  as an Excluding spot) to the image data prior to any local adjustments
  being carried out.
- When you stack RT-spots on top of each other, for example 6:
  - If the current Spot is number 6, "Merge file" will merge the 6th
    layer, either with the 5th (Previous Spot), or with the Original
    Image (the original data), or with a color defined in Background
    depending on the option chosen in the combobox.
  - If the current spot is number 3 out of the 6, then "Merge file" will
    merge spot 3 either with the 2nd spot (Previous Spot), or with
    Original (the original data) or with a color defined in Background.
  - For each of these merges you have 21 blend modes inspired by those
    of Photoshop© (Normal, Difference, Soft light, Overlay, etc.).
  - For each blend mode you can adjust the opacity, deltaE, and a
    Contrast Threshold (except in the case of Background).
  - The Graduated Filter (Luminance, Chrominance, Hue), which is located
    in Color & Light, also works with "Merge file".

As an example we will use these features to create a variable blur (of
course this isn't the only application).

##### Preparation

Add an RT-spot as in the previous examples and then add the Blur/Grain &
Denoise tool in Advanced mode using "Add tool to current spot".

- Set the RT-spot to Inverse mode (using the checkbox which will appear
  when you click on the Blur & Noise expander).
- Choose Scope = 90 or 100 depending on the desired effect.
- Set Radius to a high value (2000 or more and check the FFTW option),
  set Blur mode to Luminance & Chrominance (in the combobox at the
  bottom of the tool panel). .

<figure>
<img src="mergeblurinv_2.jpg" title="mergeblurinv_2.jpg" width="600" />
<figcaption>mergeblurinv_2.jpg</figcaption>
</figure>

Raw file (Rawtherapee - Creative Common Attribution-share Alike 4.0):
[11](https://drive.google.com/file/d/1dJ5yiqF-XdLQdKizCDseUxHf34y4AKZ6/view?usp=sharing)

##### Adding a second RT-spot

Use "Add tool to current spot" to add the Color & Light tool and set it
to Advanced mode.

- Set "Scope (color tools) " to 100.

<figure>
<img src="mergetwo1.jpg" title="mergetwo1.jpg" width="600" />
<figcaption>mergetwo1.jpg</figcaption>
</figure>

##### First merge in Normal blend mode

- Go to the "Merge file" expander in the Color & Light module.
- In the combobox choose one of the options in the list that starts with
  None. The options are:
  - Original Image.
  - Previous Spot, which merges with the previous RT-spot (or the
    Original Image if there is only one RT-spot).
  - Background, which allows you to merge with a colored background.

<!-- -->

- Then choose the blend mode (under the heading "Merge with
  Original/Previous/Background") and adjust the settings: "Merge
  background", Opacity, Contrast Threshold.
- The other parameters in the Color & Light module can also be used if
  you wish (e.g. Lightness, Contrast, Saturation etc.).

<figure>
<img src="mergeorignrmal1.jpg" title="mergeorignrmal1.jpg"
width="600" />
<figcaption>mergeorignrmal1.jpg</figcaption>
</figure>

##### Second merge using the Soft Light blend mode

In the list of merge modes try “Soft Light (legacy)" or a different mode
if you wish.

- Try adjusting the settings (e.g. Opacity, etc.) to see what difference
  they make.
- Switch from the Original Image option to Previous Spot and see what
  difference that makes.

<figure>
<img src="mergeorigsoftlight1.jpg" title="mergeorigsoftlight1.jpg"
width="600" />
<figcaption>mergeorigsoftlight1.jpg</figcaption>
</figure>

#### Using A Simple Mask To Improve Color Selection

##### Preparation

- We are going to use an image of the salt mountain in Pammukale
  (Turkey).
- It is a difficult image to process, because of the subtle differences
  in color between the sky and the mountain. Moreover, the mountain
  contains many irregularities.
- The preliminary steps are the same as for previous examples. Note the
  setting of "Scope (color tools)" to 40 which is a compromise but
  necessary if we are going to process the mountain correctly.
- For the purposes of this example, we are going to strongly increase
  the luminance (lightness) and the chrominance of the mountain (this is
  not an artistic objective) and see if we can avoid affecting the sky
  in the process.
- We could have used Excluding spots (or in a future GUI release, a
  polygon), but for now, we are going to use a simple mask. We could
  also have used several curves for the mask, or created several masks
  by duplicating the RT-spot.
- 2 types of mask are available in Local Adjustments.
  - 1\) Those that don't add or subtract the mask from the image. The
    aim in this case is to improve the quality of the deltaE selection.
  - 2\) Those that make use of the resulting differences when they are
    added to, or subtracted from the image.
  - We are going to use the first case (selection improvement).

<figure>
<img src="masksimpleprepa1.jpg" title="masksimpleprepa1.jpg"
width="600" />
<figcaption>masksimpleprepa1.jpg</figcaption>
</figure>

Raw file (Jacques Desmis - Creative Common Attribution-share Alike 4.0):
[12](https://drive.google.com/file/d/1azCxu1midw6dcuN7SbvbAiJH4pxX5BTA/view?usp=sharing)

##### Strongly increase Lightness and Chrominance

- Observe the result: there is color bleed and the sky has been affected
  by the changes, which is what we wanted to avoid.

<figure>
<img src="masksimplelum250chro1401.jpg"
title="masksimplelum250chro1401.jpg" width="600" />
<figcaption>masksimplelum250chro1401.jpg</figcaption>
</figure>

##### Creating a simple mask

- We are going to use only one of the 3 LCH curves (in this case L).
- Examine the L(L) curve closely. You will see that the point of
  inflection is located at the transition between the gray areas. This
  "transition" corresponds to the 3 references of the RT-spot (chroma,
  luma, hue) and is common to all the curves -- C(C), L(L), LC(H).
- Avoid using Blend to ensure that only the shape detection is improved.
- You can also use "Show modifications with mask" in "Mask and
  modifications".

<figure>
<img src="masksimpleshow1.jpg" title="masksimpleshow1.jpg"
width="600" />
<figcaption>masksimpleshow1.jpg</figcaption>
</figure>

##### Fine tuning the result

- Set "Mask and modifications" to "Show modified image".
- Activate "Enable mask".
- If necessary, adjust the "Smooth radius" mask tool.
- Retouch the "Contrast curve" mask and L(L) curves if necessary.
- Switch to Advanced mode and try the Gamma, Slope, and Laplacian
  threshold masks (instead of the "Smooth radius" mask).
- Certainly it is not perfect, but it is much better -- the goal is to
  discover how the masks work.

To improve the mask performance, you have 2 solutions:

- Duplicate the RT-spot: if you duplicate the RT-spot, and place it
  alongside the previous one, the slight change in the position of the
  center (references), will allow the second mask to "correct" the
  "anomalies or incompleteness" of the previous mask. Moreover this
  option allows you to readjust certain parameters if necessary in the
  second spot (in this case Lightness & Chrominance) to give a more
  homogeneous result.
- Use the mask of another open tool (if of course the tool is equipped
  with a mask). In this case you keep the same references (luma, chroma,
  hue) to create the masks and to take into account the deltaE (Scope).

DeltaE considerations:

- You can disable the core function of Local Adjustments i.e. the Scope
  function, which takes into account deltaE, if you want to work
  entirely with masks and ignore Scope. In this case set Scope=100. You
  can see that the Scope function has been disabled and you will be able
  to use just the Blend function to combine the mask and the image.
- When you use the Mask Tools sliders available in "Mask &
  modifications" (Contrast curve mask, chroma mask, gamma mask, etc.),
  you must remember that they are sensitive to the specific deltaE
  settings for the mask i.e. "deltaE Image mask" in the Mask & Merge
  panel in Settings.

<figure>
<img src="masksimple1.jpg" title="masksimple1.jpg" width="600" />
<figcaption>masksimple1.jpg</figcaption>
</figure>

##### Improved result with "Recovery based on luminance mask"

Normally we use masks in RawTherapee to:

- Improve detection (without blend).
- Improve detection and add or subtract the mask to or from the image
  (with blend).

In this example we will use the light and dark areas of the mask to
select the parts of the image that will be modified by the Color & Light
settings and then combine them with the unmodified image as follows:

- The dark and black areas of the mask will remain as close as possible
  to the original image.
- The very bright or white areas of the mask will also remain as close
  as possible to the original image.
- The intermediate zone will correspond to the settings in the Color &
  Light tool.

The area between the dark and light areas can be adjusted with the
"Recovery based on luminance mask" slider.

Note:

- To ensure that the L\*a\*b\* values of the Lockable Color Pickers
  correspond to the real values you need to set: Local Adjustments \>
  Settings \> Mask and Merge \> "Background color/luma mask” = 0 (slider
  labeled as “Background color for luminance and color masks” in
  screenshot).

Image with Color & Light settings - without mask

File pp3;
[13](https://drive.google.com/file/d/1BWpBUd5qjpDHv_stdF5ord8qFGhxC0J0/view?usp=sharing)

<figure>
<img src="mask_recov0.jpg" title="mask_recov0.jpg" width="600" />
<figcaption>mask_recov0.jpg</figcaption>
</figure>

###### Mask

Note the use of Blur Mask with "Contrast threshold" and Radius. This
increases the gray value on the right-hand part of the salt mountain and
reduces the effect of the Color & Light adjustments.

<figure>
<img src="mask_recov.jpg" title="mask_recov.jpg" width="600" />
<figcaption>mask_recov.jpg</figcaption>
</figure>

###### Recovery of the original image characteristics

- Make sure that the mask is enabled in "Mask and modifications" and
  that the "Enable mask" box is checked.
- Open the "Recovery based on luminance mask" expander.
- Set the "Recovery threshold": the closer the slider is to the value
  "2", the more the dark and very bright areas of the mask will be taken
  into account and brought back to the original image values.
- Use the "Dark area luminance threshold" and "Light area luminance
  threshold" sliders to include or exclude parts of the image. The
  corresponding values are the two limits below and above which the
  effect of the mask will be progressively taken into account (in this
  case the dark-area threshold = 32.1 and the light-area threshold =
  85).

<!-- -->

- If necessary, use “Decay strength" to adjust the rate of the decay.
- Try disabling the mask in "Mask and modifications" ("Enable mask" box
  unchecked) to see the effect.
- With the mask enabled ("Enable mask" box checked) try to reset the
  "Recovery threshold" slider to 1 in "Mask and modifications".
- Try varying other mask settings as well as the four
  "recovery"settings.

<figure>
<img src="mask_recovend.jpg" title="mask_recovend.jpg" width="600" />
<figcaption>mask_recovend.jpg</figcaption>
</figure>

#### What can you do when the mask has a salt-and-pepper appearance?

When you use the LC(h) curve to create a mask, the image of the mask can
sometimes be covered with black and white dots (salt and pepper noise)
that prevent the mask from functioning correctly. The image used in the
above example does not demonstrate this effect very well because it is
not very noisy, so I have chosen a different image with significant
chromatic noise.
<img src="masknoisechroma.jpg" title="masknoisechroma.jpg" width="600"
alt="masknoisechroma.jpg" />

##### Recovery

Processing steps: In Settings \> "Show additional settings" \> "Mask and
Merge", there is a slider labeled "Denoise chroma mask". Adjust the
slider until you get the desired effect
<img src="masknoisechromaden.jpg" title="masknoisechromaden.jpg"
width="600" alt="masknoisechromaden.jpg" />

##### Chroma noise can also have other effects on a mask.

For example, if you use the C curve to create a mask, artifacts (clumps
of grayish spots) may appear when the mask is merged with the original
image. It is likely that this is also due to chromatic noise, in which
case you can use the above procedure to correct the problem.

#### Blending a mask with the original image

In this example, we want to increase the impression of perspective
(relief) of the Pagodas.

##### Preparation

- We could have used specific tools here to give a heightened impression
  of relief e.g. CBDL (Contrast by Detail Levels) or a Wavelet pyramid.
- But for the purposes of this demonstration, we will use a mask with
  "blend".
- The preparation is identical to previous examples with "Scope (color
  tools)" set to 40 (arbitrary), and Color & Light in "Advanced" mode.

<figure>
<img src="maskblendprepa1.jpg" title="maskblendprepa1.jpg"
width="600" />
<figcaption>maskblendprepa1.jpg</figcaption>
</figure>

Raw file (Creative Common Attribution-share Alike 4.0):
[14](https://drive.google.com/file/d/1GdqejdnbW1kJFNY6y9sdQDlF2rCEGMCu/view?usp=sharing)

##### Mask settings: what not to do

- For the purpose of this demonstration we will use 2 features.
  - The LC(H) curve to select the colors.
  - A Blur Mask which combines a contrast threshold and a blur function.
- Note the checkbox FFTW, which although it consumes resources,
  increases the quality of the results. Without FFTW the radius is
  limited to 100 whereas with FFTW it is increased to 1000.

<figure>
<img src="maskblendshow2.jpg" title="maskblendshow2.jpg" width="600" />
<figcaption>maskblendshow2.jpg</figcaption>
</figure>

###### Results

- Once again, activate "Enable mask".
- Set the value of Blend to whatever you like.
- Adjust the "Smooth radius" mask if necessary.
- If you have enabled the non-mask settings of Color & light (lightness,
  contrast, etc.), the "Spot structure" slider will have an effect.
- You can see that the image now has a dominant color. This is caused by
  using the Blend function with the LC(H) curve.
  - Switch the curve to Linear mode and you will see that the dominant
    color disappears.
  - To overcome this problem avoid combining several mask settings,
    irrespective of whether they use Blend or not.
  - If you need to combine these settings, it is advisable, as in the
    simple mask case above, to create either a second (or several)
    RT-spot(s) using the Duplicate function and setting one to Blend and
    the other without (or with different Blend values). You can also use
    another mask associated with another tool.

##### The right approach

As seen above, we need to take a two-step approach, for example by
creating 2 spots.

- The first one to take into account the LC(H) curve.
- The second to act on the structure.

###### Working on the structure

There are several mask-type tools (in Advanced mode) that allow you to
modify the structure:

- Blur Mask which includes a contrast threshold and a blur function.
- Structure Mask which acts directly on the structure.
- When using these two tools, make sure that the LC(H) curve has not
  been activated (i.e. no curve).
- However if you wish to use the LC(H) curve, you can associate the L(L)
  curve with it.
- The Blur Mask and Structure Mask tools can also be associated with
  each other.
- "Local contrast” (by wavelet level) and "Wavelet level selection", can
  be associated with the L(L) mask curve and generate a local-contrast
  effect.

Reminder:

- Activate “Enable mask”.
- Set Blend to whatever value you like.
- Adjust "Smooth radius" mask if necessary.

<figure>
<img src="maskblend2.jpg" title="maskblend2.jpg" width="600" />
<figcaption>maskblend2.jpg</figcaption>
</figure>

#### How to use the Common Color Mask and an example of how to blend 2 RT-spots

This mask does not work in exactly the same way the other Local
Adjustments masks. It does not allow you to modify the behaviour of an
existing tool like the mask in Color & light for example, but is a tool
in its own right. You can use it to change the appearance of an image
e.g. contrast, luminance, color, as well as its texture.

- It consists of the 3 curves C(C), L(L), LC(H), (or in Advanced mode, 3
  curves plus Structure Mask & Blur Mask), which will generate
  differences in the color or structure of the image when compared to
  the original image.
- These "differences" are similar to the differences generated by the
  Lightness, or Chrominance functions in Color & Light.
- The color differences between the mask image and the original image
  are taken into account by the deltaE (ΔE) and transition parameters.
- Of course you can also use it in conjunction with other tools in the
  same RT-spot.
- The simple example that follows allows you to understand how it works.
  It is based on the same ΔE principles as the other tools in Local
  Adjustments.

##### Preparation

Repeat the preliminary steps outlined in previous examples and add the
tool to the RT-spot.

- "Add tool to current spot" \> Common Color Mask \> Standard. For the
  purposes of the demonstration, do not open any other tools.
- To create the mask, we will simplify the exercise as much as possible
  by using only 2 curves C(C) and L(L) to take into account the
  references of the RT-spot.
- Note that the 2 sliders "Add/subtract luminance mask " and
  "Add/subtract chrominance mask" are not set to zero, so that the user
  is not confused by a lack of response from the system; the two values
  -10 are arbitrary and low.

<figure>
<img src="common-maskprepa1.jpg" title="common-maskprepa1.jpg"
width="600" />
<figcaption>common-maskprepa1.jpg</figcaption>
</figure>

Raw file (Creative Common Attribution-share Alike
4.0):[15](https://drive.google.com/file/d/1aWvYbW-rDPQaLWoRzbK5HAcz9dU0GHFU/view?usp=sharing)

##### Luminance Mask

The curve makes a small change to the luminance.

- Note the position of the top of the curve on the gray transition. This
  means that the Luminance mask matches the reference value of the
  RT-spot.

<figure>
<img src="common-mask-showl1.jpg" title="common-mask-showL1.jpg"
width="600" />
<figcaption>common-mask-showL1.jpg</figcaption>
</figure>

##### Chrominance Mask

- Notice the position of the top of the curve on the gray transition.
  This means that the Chrominance mask matches the reference value of
  the RT-spot.

<figure>
<img src="common-mask-showc11.jpg" title="Common-mask-showC11.jpg"
width="600" />
<figcaption>Common-mask-showC11.jpg</figcaption>
</figure>

##### Preview ΔE

From here you can play with the deltaE (ΔE) between "Image + Mask" and
Original Image.

- Try increasing or decreasing the Scope (make sure you use the Common
  Color Mask slider and not the slider in the Settings module above).
- Try adjusting the parameters in the Settings "Shape detection" panel:
  "Threshold ΔE-scope", "ΔE decay", "ab-L balance (ΔE)", "C-H balance
  (ΔE)".

<figure>
<img src="common-mask-previewde11.jpg"
title="common-mask-previewdE11.jpg" width="600" />
<figcaption>common-mask-previewdE11.jpg</figcaption>
</figure>

##### Show modifications

Go to "Show modifications with mask".

- Adjust "Add/subtract luminance mask " and "Add/subtract chrominance
  mask " (these sliders could also have been called "opacity").

<figure>
<img src="common-mask-modif11.jpg" title="common-mask-modif11.jpg"
width="600" />
<figcaption>common-mask-modif11.jpg</figcaption>
</figure>

##### Result

You can:

- Change the Scope (the Common Color Mask slider which acts on ΔE).
- Activate the "Smooth radius" mask, which will try to reduce the
  artifacts caused by the fact that the mask has been generated by 3
  different curves - C(C), L(L), LC(H).
- Change the Chroma mask.
- Adjust the "Contrast curve" mask.
- Try "Scope (ΔE image mask)" in Settings: this slider acts on the mask
  and takes into account the deltaE of the mask compared to the center
  of the RT-spot. It is different from Scope (the first slider of the
  Common Color Mask), which acts on the difference between the original
  image and the mask you have created.

Switch to Advanced mode.

- Adjust the Soft Radius slider which will reduce any artifacts arising
  from differences between the original image and the one obtained after
  "adding" the mask. The default value is 1 even in Standard mode and
  produces a small variation - even without the mask – which can be seen
  in "Show modifications".
- Try the "Laplacian threshold" mask, and note the difference compared
  to the "Smooth radius" mask.
- Try the Gamma and Slope masks.
- Try to change the structure with one of the tools provided: Structure
  Mask, Blur Mask, "Local contrast” (by wavelet level) mask.
- Try the Graduated Filter Mask.

<figure>
<img src="common-mask11.jpg" title="common-mask11.jpg" width="600" />
<figcaption>common-mask11.jpg</figcaption>
</figure>

Now we are going to enhance the Common Color Mask image with the "Merge
file" tool in Color & Light.

##### Adding a new RT-spot Color & Light - Advanced mode

For demonstration (and not artistic) purposes, we will use 3 of the 21
possible blend modes.

- Add a new RT-spot.
- Add a Color & Light tool in Advanced mode.
- Set the "Scope (color tools)" correctly (using Preview ΔE).
- Choose 3 settings to increase the luminance, contrast and chrominance

<figure>
<img src="common-color1.jpg" title="common-color1.jpg" width="600" />
<figcaption>common-color1.jpg</figcaption>
</figure>

##### Preparing the "merge"

- Choose "Previous spot". We are now going to merge the new RT-spot
  (Color & Light) with the previous one (Common Color Mask).

<figure>
<img src="common-color-prepa1.jpg" title="common-color-prepa1.jpg"
width="600" />
<figcaption>common-color-prepa1.jpg</figcaption>
</figure>


##### First merge using Normal blend mode

- Choose the Normal blend mode.
- We arbitrarily choose 3 settings: “Merge background” = 54.2 (takes
  into account the deltaE between the 2 layers), Opacity = 54.2 (about
  50% for each), Contrast Threshold = 12.5 (takes into account the
  differences between uniform and textured areas).
- The two identical values of 54.2 are arbitrary and you can choose
  other values 43, 68, etc.

<figure>
<img src="common-color-normal1.jpg" title="common-color-normal1.jpg"
width="600" />
<figcaption>common-color-normal1.jpg</figcaption>
</figure>

##### Second merge using "Soft Light (legacy)" blend mode

- Change the blend mode and choose "Soft Light (legacy)".

<figure>
<img src="common-color-softphot1.jpg" title="common-color-softphot1.jpg"
width="600" />
<figcaption>common-color-softphot1.jpg</figcaption>
</figure>

##### Third merge using Color Burn blend mode

- Change the blend mode and choose Color Burn (the choice is completely
  arbitrary).
- Note the differences in luminance and chrominance.

<figure>
<img src="common-color-colburn1.jpg" title="common-color-colburn1.jpg"
width="600" />
<figcaption>common-color-colburn1.jpg</figcaption>
</figure>

##### Additional information

Of course, you can create as many Common Color Masks as you want. Simply
duplicate the mask and place it close to the previous one and use
similar settings.

Some important points about the mask curves C(C), L(L), LC(H).

- These curves are used to create the mask.
- For each of the curves, the vertical dark-gray/light-gray separation
  line represents the 3 references of the RT-spot: luminance, chroma and
  hue.
- For the first curve shown below (with the highest point of the curve
  on the selection – L in this case), the deltaE selection is improved.
- For the second curve shown below, the hue used by the mask corresponds
  to the hue reference of the RT-spot (the peak of the curve is on the
  selection – H in this case). Pulling the curve downwards will
  progressively mask (or reduce the impact) of whatever adjustment has
  been applied to the selected hue (or L, or C depending on which of the
  curves you are using).
- For the third curve shown below, the hue selection for the mask does
  not match the hue reference of the RT-spot. In this case, pulling the
  curve downwards will progressively mask whatever adjustment (luminance
  and chrominance) has been applied to that particular color.

Note that the effect of the combined LC (H) curve can be visualised by
referring to the spatial representation of the Lch coordinates i.e. as
you move up and down the vertical L axis, there will be a corresponding
increase or decrease in the chroma values.

<figure>
<img src="mask-curve.jpg" title="mask-curve.jpg" width="600" />
<figcaption>mask-curve.jpg</figcaption>
</figure>

- For this demonstration we used an image with two dominant colors:
  magenta (flower) and green (foliage). Images that have more varied
  color, luminance and chrominance (e.g. sky, sea, mountains, houses,
  fields, flowers, portraits etc.) would require more elaborate masks.
- We stay with the "philosophy" of Local Adjustments, by only relying on
  the references of the RT-spot. We could of course have used just the
  curves outlined above but this would have given a completely different
  result.
- Similarly, for the merge we chose the same color range as for the
  mask. We could have made another choice for the second RT-spot, by
  positioning it on the foliage but the result of the merge would have
  been different, with less variation in the flowers.

#### Correcting an underexposed portrait and improving grainy skin (Mairi)

The portrait of Mairi gives us the opportunity to use several
local-adjustment tools. We are going to:

- Increase the Exposure of the image to make it lighter.
- Use CBDL to soften the skin and Clarity to lighten the face.
- Make a Graduated Filter to open up the shadows of the face on the
  right-hand side of the image.
- Use 3 Excluding spots to "exclude" the eyes and lips from the
  adjustments.
- Use an LC(H) mask to "exclude" the hair from the softening adjustments
  (to avoid losing definition).
- Compare the result "before" and "after".
- Remark: the settings have been given as an indication and are a matter
  of individual taste.

<figure>
<img src="mairi.jpg" title="mairi.jpg" width="600" />
<figcaption>mairi.jpg</figcaption>
</figure>

Raw file (Copyright Pat David - Creative Common Attribution-share Alike
4.0):
[16](https://drive.google.com/file/d/1m4UBhES2AVe_sJNqMVSz5jA-Qg-s7LHt/view?usp=sharing)

##### Increasing exposure

- Exposure + 0.5
- Note: we could have used an RT-spot to limit the exposure increase to
  a particular area instead of applying an overall increase in exposure.

<figure>
<img src="mairiexp05.jpg" title="mairiexp05.jpg" width="600" />
<figcaption>mairiexp05.jpg</figcaption>
</figure>

##### Using CBDL

- Create an RT-spot with a large "Spot size" = 47
- Make a gradual contrast reduction for levels 0 to 4
- Set Clarity to 60
- Set Scope to 40

<figure>
<img src="mairi-cbdl_1.jpg" title="mairi-cbdl_1.jpg" width="600" />
<figcaption>mairi-cbdl_1.jpg</figcaption>
</figure>

##### Graduated Filter

- Create another Color & Light RT-spot.
- Graduated Filter settings: Luminance = -0.6; “Gradient angle” = 71.5
- You can also play with the chrominance settings (Advanced mode).

<figure>
<img src="mairi-grad1.jpg" title="mairi-grad1.jpg" width="600" />
<figcaption>mairi-grad1.jpg</figcaption>
</figure>

##### Excluding the eyes and lips

- Create 3 Excluding RT-spots on the eyes and lips.
- Adjust the Scope (excluding) to obtain the desired result.

<figure>
<img src="mairi-excluding1.jpg" title="mairi-excluding1.jpg"
width="600" />
<figcaption>mairi-excluding1.jpg</figcaption>
</figure>

##### Hair exclusion mask

- Go back to the first RT-spot.
- Go to "Mask and modifications".
- Select "Show mask".
- Open the LC(H) curve.
- Identify the color of the skin (the boundary between the light and
  dark gray areas on the graph).
- Lower the curve as shown in the graph (or similar).
- Adjust the "Smooth radius" mask.
- Adjust the Gamma, Slope, Contrast-curve masks if necessary.

<figure>
<img src="mairi-mask_1.jpg" title="mairi-mask_1.jpg" width="600" />
<figcaption>mairi-mask_1.jpg</figcaption>
</figure>

##### Result

- Set the mask to "Show image with modifications".
- Check the "Enable mask" checkbox.

<figure>
<img src="mairi-fin1.jpg" title="mairi-fin1.jpg" width="600" />
<figcaption>mairi-fin1.jpg</figcaption>
</figure>

##### Before and After Comparison

<figure>
<img src="mairi-befaft1.jpg" title="mairi-befaft1.jpg" width="600" />
<figcaption>mairi-befaft1.jpg</figcaption>
</figure>

##### An alternative - replace CBDL with Wavelets “Contrast by level".

- The Wavelets module is more powerful than CBDL (Contrast By Detail
  Levels) and may seem more complex given the number of options.
- However, it allows you to target the CBDL effect by using the
  "Attenuation Response" (Damper) and Offset sliders. This means that
  instead of applying the changes linearly to the wavelet decomposition
  signal, they will be adjusted depending on the actual value of the
  signal to avoid amplifying defects such as noise.
- The wavelet option also has a Clarity function.

<figure>
<img src="mairi-wav_1.jpg" title="mairi-wav_1.jpg" width="600" />
<figcaption>mairi-wav_1.jpg</figcaption>
</figure>

##### Using a mask with wavelets (yes it is possible!)

<figure>
<img src="mairi-wavmask1.jpg" title="mairi-wavmask1.jpg" width="600" />
<figcaption>mairi-wavmask1.jpg</figcaption>
</figure>

#### Add a border to an image

##### General

Local Adjustments can be used to add a white, black, gray or colored
border to an image.

The corresponding .pp3 presets can be downloaded from the following
links: White border
[17](https://drive.google.com/file/d/1fDIQl7Vp82SA4iY4TwwDazFpu97en0Fz/view?usp=sharing)
Gray border
[18](https://drive.google.com/file/d/1mXM1zRaA8w8yai7UGgkae_lg7VggjmCK/view?usp=sharing)
Colored border
[19](https://drive.google.com/file/d/1Ztj8y1XFwlJ7fH0WXco8WRRKa6G4BzQm/view?usp=sharing)
Black border
[20](https://drive.google.com/file/d/1KND7KTPfHybarkcUiSibtJ3FqISRUl9q/view?usp=sharing)

The borders are added inside the existing image area, unlike other
programs, which usually increase the image size by adding the border
outside the image area.

You can customize the border presets by changing the width (vertical or
horizontal borders) or the color. You can also use "partial paste" to
apply any modifications to the presets to other images.

I want to thank Arturo Isilvia (@aruroisilvia) for his contribution to
these presets.

##### Two RT-spots

These presets are based on two RT-spots:

- The first spot uses full-image mode and the Color & Light tool in
  Advanced mode to make the image background either black, white, gray
  or colored.
  - - For black, white or gray borders, the RGB Tone Curve in the Color
      & Light tool is used.
    - For colored borders, the "Color correction grid" is used.
- The second spot is a rectangle used in Excluding mode to define the
  border boundaries.

##### First RT-spot

###### First RT-spot settings for black, gray and white borders

<figure>
<img src="borderrgb.jpg" title="Borderrgb.jpg" width="600" />
<figcaption>Borderrgb.jpg</figcaption>
</figure>

- The Color and Light tool is set to Advanced mode.
- The changes are made using the RGB Tone Curve tool. The setting
  corresponds to the above example (gray border).
- Set Transition to 100 in the Settings module.

###### First RT-spot settings for colored borders

<figure>
<img src="bordergrid.jpg" title="Bordergrid.jpg" width="600" />
<figcaption>Bordergrid.jpg</figcaption>
</figure>

- Lightness, Contrast, Chrominance are set to -100; Gamma to 0.5
- Choose the color of your choice in the "Color correction grid". In the
  example, the basic setting is green.
- In Settings, set: Transition = 100, Scope = 100, “DeltaE scope
  threshold” = 10.

##### Second RT-spot

<img src="borderexcluding.jpg" title="Borderexcluding.jpg" width="600"
alt="Borderexcluding.jpg" /> Note the following:

- Scope = 100, Transition = 100, DeltaE scope threshold = 10
- The “Shape method” is set to "Symmetrical (mouse + sliders)" (in “Show
  additional settings” \> “Specific cases” \> “Shape method”). You can
  easily change the values of Right and Bottom to adjust the width of
  the border:
  - "Right" changes the vertical part.
  - "Bottom" changes the horizontal part.

## HDR to SDR: A First Approach (Log Encoding - CAM16 - JzCzHz - Sigmoid)

High dynamic range images are one of the recurring problems in image
processing. There are already several algorithms in RawTherapee that can
be used to reduce the dynamic range, with more or less success:

- Dynamic Range Compression and Shadows/Highlights in the Exposure tab.
- Dynamic Range & Exposure, Tone Equalizer, Tone Response Curve,
  Wavelets, etc., in the Local Adjustments tab.

To address some of the limitations of these algorithms, two additional
modules have been added to the Local Adjustments tab to help with the
processing of HDR images:

- Log Encoding
- Color Appearance (CAM & JzCzHz)

The Color Appearance module is a simplified version of the Color
Appearance & Lighting (Ciecam02/16) module found in the Advanced tab of
the main menu. It uses the same CAM16 and HDR functions adapted to the
specific requirements of Local Adjustments. It can take into account HDR
Peak Luminance and also includes an experimental JzCzHz function (in
Advanced mode) to improve HDR processing.

Please note that some parts the following explanations may be
controversial, especially for JzCzHz and part of Cam16 because:

- There is a lack of documentation for JzCzHz.
- The basic JzCzHz algorithm as published by the researchers does not
  function correctly (saturation defects, poor behavior with deep
  shadows, etc.).
- The solutions to these problems are my personal interpretation of how
  the algorithm should behave.

### **Log Encoding**

The code used in this part of RawTherapee is similar to:

- The Log Tone Mapping module in ART, designed by Alberto Griggio.
- The Filmic module in darktable, designed by Aurélien Pierre.

Both are inspired by the work on logarithmic coding developed by the
Academy Color Encoding System (ACES).

#### The algorithm is based on a 3-step process:

- The first step for a given image (HDR or otherwise) involves
  calculating the deviation from the theoretical mean gray value (18%
  gray) of the darkest blacks and the brightest whites. This is
  expressed in photographic Ev units (luminosity index, which is related
  to the brightness of the scene). The black and white Ev values, along
  with the average or mean luminance of the scene (Yb%) are used by the
  algorithm (either automatically or with manual override) to modify the
  balance of the RGB values, thereby reducing contrasts, enhancing
  shadows and reducing highlights, without overly distorting the image
  rendering.

<!-- -->

- In the second and third steps, the data is manually corrected by the
  user to increase local contrast (which has been reduced by the "Log"
  conversion) and adjust the viewing conditions for the intended output
  device.

#### Chromatic Adaptation Example

The first step is to find an almost mathematically perfect white balance
using White Balance \> Auto \> “Temperature correlation” (same example
and settings as in the “Ciecam Advanced tab” tutorial).

Raw file (Rawtherapee - Creative Common Attribution-share Alike 4.0):
[21](https://drive.google.com/file/d/1CiQ2t4KyD3tdCiNNhskqUG2cH9LT2ly7/view?usp=sharing)

<figure>
<img src="catatwb.jpg" title="catATwb.jpg" width="600" />
<figcaption>catATwb.jpg</figcaption>
</figure>

##### Choose a suitable Local Adjustments setting - Preparation

- Select the Local Adjustments tab.
- Add a “Full image” spot.
- Set some lockable color pickers as shown.

<figure>
<img src="catlaset.jpg" title="catLAset.jpg" width="600" />
<figcaption>catLAset.jpg</figcaption>
</figure>

##### Select Log Encoding

Choose: "Add tool to current spot" \> Log Encoding
<img src="catlalog.jpg" title="catLAlog.jpg" width="600"
alt="catLAlog.jpg" />

- Try changing the position of the center of the RT-spot.
- Try changing the values of Scope e.g. 40 - 60 - 80 – 100.
- Observe the results.
- Note that the image still has a yellow cast.

##### Modify Chromatic adaptation - cat02

<figure>
<img src="catlalogadap.jpg" title="catLAlogadap.jpg" width="600" />
<figcaption>catLAlogadap.jpg</figcaption>
</figure>

- Make the image cooler by moving the “Chromatic adaptation cat02"
  slider to the left.
- Reducing the slider value by 10 units corresponds to a 300K drop in
  illuminant temperature.
- Try with a value of -23.

#### High dynamic range image + Ciecam

The image is a difficult one (the same that was used for the CIECAM
example in the Advanced tab). It has very marked shadows and strong
sunlit backlighting. Use the default RawTherapee settings and position
the lockable color pickers as shown so that you can see the changes when
processing.

Raw file (Pixls.us - Creative Common Attribution-share Alike 4.0):
[22](https://drive.google.com/file/d/1ctjOWX2lVmgcAzJtBwt69FGpxZOq-LyP/view?usp=sharing)
<img src="ciecam_light_prepa.jpg" title="ciecam_light_prepa.jpg"
width="600" alt="ciecam_light_prepa.jpg" />

##### Using Log Encoding + Ciecam

Create a full-image spot and then "Add tool to current spot" \> Log
Encoding. For this example, and for comparison with
[Lift the shadows]({{% relref "#using-log-encoding" %}}), the following
arbitrary settings are used:

- Set the value of Scope = 79
- Complexity = Advanced.
- Press the Automatic button.

<figure>
<img src="ciecamlog1.jpg" title="ciecamlog1.jpg" width="600" />
<figcaption>ciecamlog1.jpg</figcaption>
</figure>

- Move the RT-spot and observe the effect.
- Change the Scope settings and observe the effect.

##### Adjusting the Ciecam settings in the Log Encoding module

- In the Scene Conditions panel choose Surround = Dim. The image will
  become lighter.
- In the Image Adjustments panel, set the Saturation(s) = 30 and
  Contrast (J) = -10.
- Observe the effect on the shadows.

<figure>
<img src="ciecamlog_cie.jpg" title="ciecamlog_cie.jpg" width="600" />
<figcaption>ciecamlog_cie.jpg</figcaption>
</figure>

Now open the "All tools" expander.

- Try Colorfulness (M) instead of Saturation (s).
- Try Contrast (Q) instead of Contrast (J).
- Adjust the Lightness.

#### Log encoding - Dodge and Burn - Ciecam

##### Preparation

This is another way to Dodge and Burn using Log Encoding and Ciecam.
Position an Rt-spot on the face and set 2 "Lockable color pickers" as
shown.

Raw file (Copyright - Jean Christophe Frisch - Creative Common
Attribution-share Alike 4.0):
[23](https://drive.google.com/file/d/1oyPu-U6CD1DjWuO8LuqJdIdDau1-2yY1/view?usp=sharing)
<img src="dblogciecamprepa.jpg" title="Dblogciecamprepa.jpg" width="600"
alt="Dblogciecamprepa.jpg" />

##### Using Log Encoding in manual mode with Ciecam

- "Add tool to current spot" \> Log Encoding
- Complexity = Advanced
- Click on Automatic
- Slightly increase the White Ev value until you get the desired effect
  (in this example from 3.0 to 5.0).
- Slightly increase the Saturation (s).
- Click on "All tools" and slightly reduce Brightness (Q).
- Try the other settings e.g. Surround = Dim, Lightness (J), etc.
- You can also adjust the Scope and move the RT-spot and observe the
  variations in the result.

<figure>
<img src="dblogciecam.jpg" title="Dblogciecam.jpg" width="600" />
<figcaption>Dblogciecam.jpg</figcaption>
</figure>

### **Log Encoding and Highlight Recovery**

The use of Log Encoding can sometimes lead to unexpected results. If the
image contains highlights that were overexposed during shooting, then
they need to be recovered or reconstructed. However, if this is the
case, the Log Encoding module will "overwrite" the reconstructed
highlights, resulting in unpleasant effects (e.g. unexpected changes in
luminosity, hue and saturation). We will use 2 methods to overcome this
problem and preserve the highlights.

- Using a mask and a recovery process.
- Using excluding spots.

#### Preparation

Raw file (Pixls.us Jonathan Dumaine - Creative Common Attribution-share
Alike 4.0):
[24](https://drive.google.com/file/d/1I-Y6xqFoYMaxj9v3uEcvXfZomuJxyX6R/view?usp=sharing)

PP3 File
[25](https://drive.google.com/file/d/1JQG52i76FxFWUWqi4Mz_5seWpLD-rvT6/view?usp=sharing)

Preliminary adjustments

- Adjust the white balance (Color tab). Without the knowledge of the
  person who took this image we don't know anything about the lighting
  conditions. Are they LED or incandescent lamps? How was the foreground
  lit? In this case:
  - You can either leave the image as is,
  - or use the "Temperature correlation" automatic white balance method.
- Reconstruct the highlights (Exposure tab). I have used the excellent
  Color Propagation algorithm designed by Emil Martinec.

Preparation of the RT-spot

- Choose Rectangle.
- Position the delimiters outside the preview area.
- Set the transition to a fairly high value.
- Position a series of Lockable Color Pickers on the image.
- Ensure that the L\*a\*b\* values of the Lockable Color Pickers match
  the actual values by setting Mask and Merge \> "Background color/luma
  mask” = 0 in the Settings module.

<figure>
<img src="loghigh-prepa.jpg" title="loghigh-prepa.jpg" width="600" />
<figcaption>loghigh-prepa.jpg</figcaption>
</figure>

#### Apply Log Encoding

- Add the Log Encoding tool using "Add tool to current spot".
- Select Advanced (or Standard).
- Click on the Automatic button.
- Set Scope to a high value i.e. 80 or more.

Log Encoding will automatically adjust the image and in particular the
foreground, but the colors in the previously "reconstructed" highlights
in the background will be desaturated and the brightness will be
reduced.

Not only that, but the overall image is too saturated and the changes in
exposure are badly distributed.

<figure>
<img src="loghigh-log.jpg" title="loghigh-log.jpg" width="600" />
<figcaption>loghigh-log.jpg</figcaption>
</figure>

#### Elaboration of the mask

We will create a different sort of mask compared to what we normally use
in RawTherapee. This mask will be used "live" to combine two images
processed with and without Log Encoding.

Depending on the settings of "Recovery based on luminance mask":

- The dark and black areas of the mask will result in a combined image
  that is as close as possible to the original.
- The very bright or white areas of the mask will also result in a
  combined image that is as close as possible to the original.
- The intermediate area will be modified by the settings of Log
  Encoding.

In this case I used the LC(H) curve but other images may require the
L(L) curve. Note that the C(C) curve has no effect on the "mix" but can
be used to improve the selection.

<figure>
<img src="loghigh-mask.jpg" title="loghigh-mask.jpg" width="600" />
<figcaption>loghigh-mask.jpg</figcaption>
</figure>

#### Partial recovery of highlights with mask

- Make sure that the mask is enabled i.e. the "Enable mask" box is
  checked.
- Open the expander "Recovery based on luminance mask".
- Set "Recovery threshold". The closer the slider is to "2", the more
  the dark and very bright areas of the mask will be taken into account
  and restored to the original image values.
- Use the "Dark area luminance threshold" and "Light area luminance
  threshold" sliders to include or exclude parts of the image. The
  corresponding values (in this case dark = 25.5 and Light = 98.3) are
  the two limits below and above which the effect of the mask will be
  progressively taken into account.
- If necessary, use “Decay strength” to adjust the rate of decay.

<figure>
<img src="loghigh-recov.jpg" title="loghigh-recov.jpg" width="600" />
<figcaption>loghigh-recov.jpg</figcaption>
</figure>

#### Highlight Recovery using an Excluding spot

We can also use one of the strong points of Local Adjustments by using
Excluding spots. The adjustments are arbitrary.

<figure>
<img src="loghigh-exclu.jpg" title="loghigh-exclu.jpg" width="600" />
<figcaption>loghigh-exclu.jpg</figcaption>
</figure>

#### Final adjustment with Ciecam16

Once the various parameters have been set, we can refine the result. In
this case I have chosen several Ciecam settings (using Ciecam 2016 in
this case) to:

- Increase the contrast.
- Reduce the saturation, especially for the skin.
- Change the chromatic adaptation, to make the image a little "colder".

Of course, everything is quite arbitrary and depends on one's
perception.

A final remark.

- The image is particularly noisy and will need to be denoised. However,
  to keep things simple, I have excluded this from the example.

<figure>
<img src="loghigh-ciecam.jpg" title="loghigh-ciecam.jpg" width="600" />
<figcaption>loghigh-ciecam.jpg</figcaption>
</figure>

### **Other Examples Log Encoding**

- In this example we are going to use the Log Encoding tool (derived
  from the darktable filmic module and adapted by A. Griggio for ART).
  The tool has undergone further adaptation by J. Desmis for use in
  RawTherapee Local Adjustments.
- To demonstrate the possibilities of this module, we are going to use
  it to make a luminance gradient, without using the Graduated Filter in
  the Log Encoding menu.
- There are three steps: preparation, automatic settings, adjustments.

#### Preparation

- Set the RT-spot so that:
  - The center is at the bottom left corner of the image.
  - The upper right corner is at the limits of the image.
- Go to "Add tool to current spot" then select Log Encoding (the tool
  has been deliberately disabled in the screenshot).

<figure>
<img src="encodlogprepa.jpg" title="encodlogprepa.jpg" width="600" />
<figcaption>encodlogprepa.jpg</figcaption>
</figure>

Raw file (Copyright - Roberto Posadas - Creative Common
Attribution-share Alike 4.0):
[26](https://drive.google.com/open?id=1RXoXp-AHWzo6mzbd-VyRTRvmRlFtyZDq)

#### Automatic settings

- Press the Automatic button.
- The image will brighten.
- Click the Automatic button again to see the settings more clearly.
- The values Black Ev = -6.7, White Ev = 6.9, indicate a large dynamic
  range = 13.6 EV.
- “Source gray point” = 1.2 (slider changed to “Mean luminance (Yb%)” in
  the Scene Conditions

### **Using the Cam16 and HDR functions**

#### Preamble

The Cam16 color appearance module (Cam16 & JzCzHz) can also take into
account certain aspects of HDR images. However, we need to keep in mind
that Log Encoding, JzCzHz and Cam16 are only part of the overall HDR
processing chain. Other aspects such as the HDR monitor characteristics
(including the LCMS code) will need to be included in the future.

#### Differences compared to the Color Appearance & Lighting (Ciecam02/16) module (Advanced tab)

- Takes into account the specific aspects of Local Adjustments (deltaE,
  transitions, masks etc.).
- Has a simpler chromatic adaptation process.
- Has fewer curves.
- Has a simplified PQ (Perceptual Quantizer) function: PQ, which is
  positioned at the beginning of the Cam16 processing pipeline, changes
  the way the Brightness (Q), Lightness (J), Colorfulness (M),
  Saturation (s) and Chroma (C) values are calculated internally.
- Allows Log Encoding Q or Sigmoid Q to be used with or without the
  Black Ev and White Ev values (dynamic range).

#### Some important points

- The scene-condition values are calculated by the Cam16 algorithms and
  a process located upstream of Local Adjustments, just after the input
  profile, the demosaicing, and the choice of the working profile, all
  of which are in linear mode.
- The RGB=\>Lab conversions at the beginning of the Local Adjustments
  (LA) process and Lab=\>RGB at the end of the LA process do not change
  the dynamic range (DR). If an image has a DR of 15Ev before the LA
  module, it has approximately the same value after the LA module (any
  differences will be due to user adjustments).
  [Jzazbz - a new experimental CAM? (Cam16 & JzCzHz)](CIECAM02#Jzazbz_-_a_new_experimental_CAM_(Cam16_&_JzCzHz) "wikilink").
  However, while Lab allows low light levels (less than 0.005 cd/m2) to
  be taken into account, this is not the case for highlights \> 120
  cd/m2 because of the use of gamma. Nevertheless, Lab preserves
  highlights with values \>120 cd/m2 and can be thought of as a form of
  data compression. Future developments will have to take into account
  the higher highlight values (HDR-Lab, output process, etc.).
- If in Settings you check the 2 boxes "Avoid color shift" and "Munsell
  correction only", the whole local-adjustments process will maintain a
  constant hue whatever the changes made to the saturation.

#### Cam16 with HDR pre-processing

A new integrated processing approach, allows the user to:

- Better exploit the potential of Cam16, recognised for its ability to
  take account of the physiological aspects of image processing.
- Integrate HDR processing upstream of Ciecam – Cam16 if need be.

If this is your first contact with Cam16, or if you're put off by
technical details, you can skip the next paragraph.

##### There are two major difficulties implementing Cam: variables in 6 dimensions and the specificities of the brightness function Q

###### Cam and the 6 dimensions

It's necessary to touch on the programming aspects of Cam16 (the same
applies to Cam02) to understand the difficulties and constraints. During
the development of Ciecam, around 2011-2012, it quickly became apparent
that unlike working in RGB, XYZ or Lab mode, which requires
3-dimensional variable declarations, Ciecam requires 6 dimensions: J
(luminance), Q (brightness), C (chromaticity), S (saturation), M
(colorfulness) and h (hue). This requires considerable memory and
significant processing times. It was therefore decided to process the
variables sequentially, one at a time, in a global loop to limit the
memory requirements. However this presents a major problem because
optimising a procedure called in the middle of the loop is practically
impossible. As a result, optimisation of functions such as contrast,
sigmoid, etc. is limited and global.

Cam16 also introduces the ability to:

- Lift the shadows (Lighting).
- Carry out perceptual processing of the highlights, which complicates
  the use of HDR processing techniques such as log encoding.

###### Particularities of Q - Brightness

- By design, Q (brightness), unlike J (lightness), L (L\*a\*b\*) and
  luminance calculations in RGB, HSV and other modes, is calculated from
  absolute luminance and has no fixed limits (0..1, 0..65535).
- When J is in the interval \[0, 1\], the value of Q can reach values of
  5 or 10, depending on the scene conditions. This is a very strong
  constraint on the references used by the algorithms (average,
  thresholds, etc.) and makes it difficult to automate them.
- However, in the majority of cases, Q results in a more natural
  rendering, closer to human perception.

###### Removal of the HDR modules - "Log encoding Q" and "Sigmoid Q" - previously integrated into Cam16, Image Adjustments

![](sourcedata1.jpg)

In view of the difficulties
described in the two previous paragraphs, it was decided:

- To place the HDR or colorimetry tools in the Source Data Adjustments
  module, located upstream of the Ciecam – Cam16 process. It is
  integrated in the Color Appearance GUI so that the user does not have
  to switch tools. This module contains a number of functions that are
  more or less useful depending on the nature of the image:
  - Log encoding: it encodes the data logarithmically to allow
    high-dynamic-range images to be processed correctly. Of course you
    can use this tool for all images, but it is only really useful if
    the dynamic range is in the order of 10 Ev or more. Using this tool
    when it isn’t necessary can lead to a change in the overall
    colorimetry that is not always easy to recover. The "Brightness
    compression" slider is used to limit the action for high luminance
    values. [An evaluation of the dynamic-range capabilities of tools]({{% relref "#an-evaluation-of-the-dynamic-range-capabilities-of-tools-in-selective-editing" %}}).
  - Tone Response Curve (TRC) & Midtones: in the majority of cases, this
    module allows you to modify underexposed images (with deep shadows),
    or images where there are problems with luminance balance. This
    approach is identical to the other modules using this TRC in
    Rawtherapee.
    - Gamma: modifies the effect on the highlights.
    - Slope: allows you to lighten the shadows.
    - Midtones: adjusts midtones.
      - To avoid color shifts, you can choose where this algorithm is located: a) before gamma and slope; b) after gamma and slope; c) after the entire CAM16 process
    - Attenuation threshold: completes the processing carried out by
      gamma, slope and midtones by causing a slight lowering of
      highlights. Please note this does not replace Highlight
      reconstruction.
    - Tone Mapping Operators : Complements the processing performed by Gamma, Slope and Midtones by applying either:
      * asymptotic highlight attenuation
      * or tone mapping.
        Please note that this does not replace highlight reconstruction.
      * Ev based: attenuates highlights using an exponential function.
      * Gamma based: attenuates highlights using a hyperbolic function and partial tone mapping.
      * Slope based: uses tone mapping. Slope (gray balance) acts on the dynamic range and affects shadows and contrast. 'Scale Yb scene' is a function of White Ev.
      * RGB channel slope: similar to 'Slope based', but differentiable for each R, G, B channel. Attenuation threshold modifies mid gray and shadow/highlight balance.
      * Sigmoid based: applies Sigmoid tone mapping with the Contrast and Skew sliders. Display white point (cd/m2) adjusts the maximum screen luminance. This new version of Sigmoid is incompatible with versions 5.11 and earlier..
      * TRC based: applies a simplified Tone Response Curve (TRC) to each individual R, G, B channel. Modifies the gamma by adjusting the shadow/highlight distribution (levels). 'Relative gamma' mode is a simplified method that takes into account the Slope. 'Highlight attenuation threshold' reduces the effect on the highlights.
      * For the RGB operators 'RGB channel slope' and 'TRC based', the 'Invert color' checkbox simulates a film negative function.

<!-- -->

- - Primary & Illuminant lets you:
    - Adjust the colorimetry of images where for example, a mixture of
      illuminants leads to visible drift (e.g. LEDs), or where the
      shooting conditions (stdA illuminant, predominantly red flowers,
      etc.) lead the system to unsatisfactory responses (exaggerated
      saturation of blues or reds, etc.).
    - Be careful if you change the Working Profile (Color Tab), which is
      Prophoto by default, you will need to adapt the "Destination
      primaries" (this is not automatic).
    - Modify the colors or saturation of an image locally or globally:
      - The system calculates the xy values of the dominant color of the
        image and displays them (gray point) on the CIExy diagram;
      - Moving the "Refine colors (white-point)" slider will modify the
        saturation of the image.
      - Changing the position of this dominant colour using the "Shift
        x" and "Shift y" sliders will allow you to add or remove a
        dominant colour (hue and saturation) without changing the
        primaries.
    - Gamut control: provides gamut control, particularly if the
      primaries have been changed.
    - Matrix adaptation: 4 choices - Bradford, Cat16, Cat02, Von Kries,
      XYZ scale - provides chromatic adaptation when the primaries are
      changed.

Some of these tools are similar to the Abstract Profile concept.
[Abstract Profiles]({{% relref "Color_Management#abstract-profiles" %}})
##### Source Data Adjustments and Scene conditions

I won't go back over what Scene Conditions does (see the tutorials on
Ciecam), even though this concept is more complex and misunderstood than
many users think. As a brief reminder, Scene Conditions' (or “scene
referred”, or “source”, depending on usage) takes into account the
characteristics of the image at the time of shooting. This is to be
distinguished from Viewing Conditions (or “display referred”, depending
on usage) which takes into account the viewing environment. For example,
the system used to display the image, its luminance and contrast (HDR or
standard monitor, television, projector, printer, etc.), or the
luminance of the background (in full sunlight, in a dark room, etc.). Be
careful not to misuse the Viewing Conditions settings to compensate for
imperfect Scene Conditions or Source Data Adjustments settings.

[Tutorial Color Appearance & Lighting (CIECAM02/16) and Color Appearance (Cam16 &  JzCzHz)]({{% relref "CIECAM02#color-appearance--lighting-ciecam0216-et-color-appearance-cam16--jzczhz---tutorial" %}})

###### What principles do "Log encoding" and Scene Conditions use to render an image prior to Cam16?

<img src="whiteblackdisr.jpg" title="whiteblackdisr.jpg" width="400"
alt="whiteblackdisr.jpg" />Cam16 needs information on:

- Absolute luminance: this is calculated from the Exif data at the time
  of shooting: speed, aperture, exposure compensation, etc.
- Mean luminance, also known as "gray point". This calculation is
  carried out as described in the paragraph below.

To process high-dynamic-range images, you need to calculate the values
required for the logarithmic conversion:

- BlackEv and WhiteEv, as well as the "gray point" mentioned above.

Where should these values be taken, and how should they be calculated?
The logic of Scene Conditions leads us to place ourselves as far
upstream as possible in the processing process while still having access
to useful data. ART originally chose to take a copy of the image data
just after demoisaicing. The disadvantage of this choice is that by
definition, the image is not processed and the colour and light
distribution is poorly known and unreliable. The difficulty introduced
by logarithmic conversion to compress the data (Log 2) is that it is not
linear.

Digital noise must therefore be taken into account and the minimum and
maximum RGB values as well as the gray point need to be estimated. These
adjustments should also take into account possible subsequent changes in
luminance. More or less empirical formulas for calculating average
luminance and the resulting gray point have been developed by the
original designers. Nevertheless, the result can be disappointing.

In practice, once the calculations have been made manual adjustments are
still required for at least BlackEv, WhiteEv, “Mean luminance Yb%” (gray
point) for the source data, and or “Mean luminance Yb%” (gray point )
for the Viewing Conditions (not to be confused with source data
adjustments), and other subsequent luminance adjustments. Hence the
difficulty of obtaining a result intuitively.

To try and solve this problem, I chose to add two extreme white and
black adjustment settings. I've called them “Whites distribution” and
“Blacks distribution”. There are obviously no general rules for
processing an image, you weaken or strengthen the most exposed or
darkest part of the image copy. The internal algorithm recalculates the
BlackEv, WhiteEv and Mean luminance (gray point) values, which must be
consistent. The advantage is (I hope) a more intuitive system. There may
be other ways (?) of achieving this; this one has the merit of working.

These controls are only available if the Automatic check box in Scene
Conditions is ticked.

##### The application GUI has been simplified and made more intuitive, in particular through the use of Expanders

<figure>
<img src="cam16exp.jpg" title="Cam16exp.jpg" width="600" />
<figcaption>Cam16exp.jpg</figcaption>
</figure>

##### CAM16 Image Adjustments - settings
<figure>
<img src="cam16imadju.jpg" title="cam16imadju.jpg" width="600" />
<figcaption>Cam16image adjustments</figcaption>
</figure>

CAM16 Image adjustments has similar features to CIECAM02, but with fewer options. 

[Tutorial CIECAM](ciecam02/#color-appearance--lighting-ciecam0216-et-color-appearance-cam16--jzczhz---tutorial)

The 'Red Green Blue' function offers the same functionality, allowing for deltaE and local corrections.

[Red green blue](ciecam02/#red---green---blue---hue-rotation-h---saturation-s---brightness-curve-q)

##### CAM16 Final Gain & Gamut Compression
<figure>
<img src="cam16final.jpg" title="cam16final.jpg" width="600" />
<figcaption>Cam16 Final Gain & Gamut Compression</figcaption>
</figure>

**Target Gamut**
Placed at the end of Selective Editing > Color Appearance (CAM16 & JzCzHz), takes into account, only for CAM16 & JzCzHz, everything preceding the current Spot, this tool allows you to compress the data to fit into the output profile.

This output gamut compression is much simpler than at the beginning of the processing pipeline because unless the user has exaggerated the settings, the gamut will already be close to the Working Profile, hence the simplified graphical interface.

More often than not, you will not see any changes to the image or the histogram when using Gamut Compression. This is because it does not convert the data from Working profile (often Rec2020) to Target Gamut (often sRGB), but compresses it to fit the Target gamut (often sRGB) while remaining in Working profile (often Rec2020), and the upstream treatments must have preserved the gamut. The effect is visible on Led’s, flowers with high saturation. On a normal landscape or portrait, there is most often no change.

**Gain (Ev)**
Modifies global image appearance, similar to the Exposure slider, by applying a positive or negative Ev adjustment to the RGB data. This allows for better control of either the color gamut or the saturation; for the latter, small values ​​of plus or minus 0.1 to 0.4 are sufficient. Higher gain values ​​allow for future use with HDR displays.

**Note**
The term 'final' can be misleading. It all depends on the user's actual process. If 'Selective Editing > Color Appearance (CAM16 & JzCzHz)' is the last processing step, then 'final' truly signifies the end of the process, and the values ​​displayed are the final ones. If the user chooses to use other processing steps such as 'Exposure', 'Abstract Profile', or 'Color Appearance & Lighting', etc., then 'final' only signifies the end of the 'CAM16' process.


##### Cam16 tutorial with an HDR image

I have chosen an image of the Alexandre III bridge in Paris, taken at
the end of October 2021.

- Nikon Z6 II
- Raw file (Jacques Desmis - Creative Common Attribution-share Alike
  4.0):
  [27](https://drive.google.com/file/d/17G1H-7S2uCyDa92CiJf9g35jhegvCZu_/view?usp=sharing)
- pp3 file [Alex3-NEF.pp3](alexandre3-1.pp3 "Alex3-NEF.pp3")

This image has a high dynamic range of just over 13.5Ev, with some areas
of high exposure (luminance L \>= 100).

The aim of this tutorial is not to demonstrate the 'best' way of
processing this image, but to show that by integrating a pre-processing
module upstream of Ciecam, you can process images with a very high
dynamic range so that they can then be easily processed by Ciecam
(Cam16).

I've added a second example, unrelated to the Alexander III bridge, to
show the possible use of the Tone Response Curve (TRC).

###### Preparation: establishing a good starting point and identifying the problems to be solved

I decided to start with the following settings:

- Apply a Neutral profile.
- Apply White Balance: Auto - Temperature correlation.
- Activate “Highlight reconstruction”: Inpaint Opposed - “Gain
  threshold” = 0.82

With these basic settings:

- The shadows in the upper right-hand part of the steel structure are
  completely lacking in detail.
- The highlights are acceptable (L = 97), with no apparent colour drift.
- The histogram shows that the red channel is out of bounds.
- The rendered image is not very appealing.

<figure>
<img src="alex3-neutr.jpg" title="Alex3-neutr.jpg" width="600" />
<figcaption>Alex3-neutr.jpg</figcaption>
</figure>

###### Apply Nikon Auto-Matched Tone Curve rendering

By default Rawtherapee loads the Nikon Z6 II rendering profile.

Observation:

- The image looks more pleasing, more balanced.
- The shadows have even less detail.
- The highlights are out of range (L \>= 100).

<figure>
<img src="alex3-automatch.jpg" title="Alex3-automatch.jpg"
width="600" />
<figcaption>Alex3-automatch.jpg</figcaption>
</figure>

###### Correct the dynamic range

- Activate the Source Data Adjustments expander.
- Activate Log Encoding.

Examine the result:

- High Dynamic Range: 13.6Ev (according to the calculations).
- Detail has been restored to the shadows (probably too much)!
- The highlights are within acceptable limits L=98.
- The histogram shows an overshoot in the reds.
- The overall image is too bright.

<figure>
<img src="alex3-logencode.jpg" title="Alex3-logencode.jpg"
width="600" />
<figcaption>Alex3-logencode.jpg</figcaption>
</figure>

To increase the shadows and reduce the highlights:

- Set (Scene Conditions) “Whites distribution” to -74 and “Black
  distribution” to -59: these choices are fairly arbitrary and depend on
  the perception of the user.
- Set Midtones (Source Data Adjustments) to -70, to restore a more
  acceptable average luminance.
- Increase “Brightness compression” (Source Data Adjustments - Log
  encoding) to 0.90 to lower the brightness a little.

<figure>
<img src="alex3-dynamic1.jpg" title="Alex3-dynamic1.jpg" width="600" />
<figcaption>Alex3-dynamic1.jpg</figcaption>
</figure>

###### Make the Seine look more “summery”

The autumn weather (and the cleanliness of the Seine?) make the water
look unflattering.

We are going to make it a little more pleasant. Let's create a second
RT-Spot (Scope = 60).

In Source Data Adjustments, Tone Response Curves: Midtones = -70.

In Primaries & Illuminant - go to “Dominant color”:

- Use the 2 sliders “Shift x” and “Shift y” to move the gray point
  representing the dominant colour.
- Shift x = 0.2, Shift y = -0.0454
- So that the white point, the gray point (dominant colour) and the
  black point representing the red primary are roughly aligned.
- By moving “Refine colors (white-point)”, we're going to change both
  the hue towards more blue/green and the saturation. Of course I could
  have made other choices.

<figure>
<img src="alex3-seine1.jpg" title="Alex3-seine1.jpg" width="600" />
<figcaption>Alex3-seine1.jpg</figcaption>
</figure>

##### A second example - Using the TRC or Surround - Truck under a tunnel

- Raw file(Copyright - Roberto Posadas - Common Attribution-share Alike
  4.0):
  [28](https://drive.google.com/open?id=1RXoXp-AHWzo6mzbd-VyRTRvmRlFtyZDq)
- pp3 file [truck-tunnel-1.pp3](truck-tunnel-1.pp3 "truck-tunnel.pp3")
- pp3 file [truck-tunnel-2.pp3 - 2 spots](truck-tunnel-2.pp3 "truck-tunnel.pp3")
- pp3 file [truck-tunnel-3.pp3 - surround](truck-tunnel-3.pp3 "truck-tunnel.pp3")

###### Original Image

<figure>
<img src="van-tunnel-0.jpg" title="Van-tunnel-0.jpg" width="600" />
<figcaption>Van-tunnel-0.jpg</figcaption>
</figure>

###### Image with Source Data Adjustments - Tone Response Curve & Midtones

- Scope = 60;
- Note that the settings are simplified and intuitive:
  - Gamma = 2.90: brightens highlights.
  - Slope = 80: lightens shadows.

<figure>
<img src="van-tunnel.jpg" title="Van-tunnel.jpg" width="600" />
<figcaption>Van-tunnel.jpg</figcaption>
</figure>

###### Image with Scene Conditions - Surround

Here, it's even simpler.

- Scope = 60;

Scene conditions:

- Surround = Dark.

Source Data Adjustments:

- Smooth Highlights = enabled

Cam16 Image Adjustments:

- Contrast J = 24.

<figure>
<img src="van-tunnel-surround.jpg" title="Van-tunnel-surround.jpg"
width="600" />
<figcaption>Van-tunnel-surround.jpg</figcaption>
</figure>

##### Comparison between the two TRC (tone response curve) tools in RawTherapee and the impact of Ciecam

There are two TRC tools in RawTherapee, one in the Abstract Profile
module in Color Management, and the other in the Source Data Adjustments
module in the Local Adjustments, Color Appearance tool.

The Abstract Profile and Source Data Adjustments modules are nearly
identical except for an additional log-encoding function integrated into
the latter. This tool should be reserved for extreme cases that can’t be
solved any other way. This is because the way they are rendered can be
unpredicatable and often disturbs the balance of light and colour.

They are both based on the use of a virtual profile to modify the data.
They include:

- A Tone Response Curve with a Gamma slider for adjusting the highlights
  and a Slope slider for the shadows.
- Two additional sliders or settings - checkbox or combobox (Source Data
  Adjustments - SDA) - for adjusting the midtones and the highlights:
  - Midtones
  - Highlight Attenuation & RGB Channels
    - Ev based: which can be used to soften strong highlights with Ev
      values between 0 and +12.
    - Gamma based (Source Data Adjustments): Can be used to soften
      strong highlights by using the Mean Scene luminance Yb% of an
      asymptotic polynomial function.
    - Slope based (Source Data Adjustments): Combine Gamma based with a
      tone mapping function using Slope. The system is capable of
      restoring 23 to 24 Ev. The Slope slider allows you to adjust the
      balance between the shadows and the highlights. The Scene value of
      Yb% is taken into account to scale the algorithm.
    - Sigmoid based (Source Data Adjustments): inspired by Darktable
      code and modified to be as automatic as possible, taking into
      account the calculated values ​​of Black Ev, White Ev, Mean
      Luminance (Yb) Scene, and Display White point (cd /m2). It has 3
      sliders including Contrast with lower base values ​​than in ART and
      Darktable (position in the process, taking into account calculated
      data, etc.), Skew which positions the sigmoid between the shadows
      and the lights and Display White point.
    - RGB Channel Slope (Source Data Adjustments): similar to 'Slope
      based', allowing you to act separately on the 3 channels R, G,
      and B. Three controls have been added to a) take into account Yb%
      value in viewing conditions; b) adapt the rendering by maintaining
      the overall brightness; c) adapt the point (near Mean Luminance
      Yb) from which the attenuation of the highlights occurs.

<img src="highattenuation1.jpg" title="highattenuation1.jpg" width="300"
alt="highattenuation1.jpg" />

- Primaries & Illuminant, for adjusting the image colorimetry. The user
  can choose the illuminant and adjust the dominant colour.

<img src="primdom.jpg" title="Primdom.jpg" width="300"
alt="Primdom.jpg" />

- Adjusting the dominant color can be done by moving the white point
  towards the calculated color dominant (gray point on the diagram) via
  Refine colors (white-point), or moved by changing the values of Shift
  x and Shift y.
- You can use the Primaries module to create a Channel Mixer and switch
  to black and white

###### Since 5.12, Sigmoid based has been replaced by a Darktable-like version of Sigmoid

This new version is based on the Darktable (and ART) code, but differs
in that it takes different account of Mean luminance, White-Ev and
Black-Ev.

- First - it uses 3 sliders
  - Contrast, which acts on the slope of the Sigmoid.
  - Skew, which moves the Sigmoid between whites and blacks.
  - Display White point (cd/m2): Allows you to adapt the white point to
    the peak luminance of the display device. Its position in the
    processing pipeline is not optimal so use with caution.

<!-- -->

- Second - You'll also find it in CAM16 Image adjustments.
  - In this case, the 3 RGB variables are no longer used, but Q - CIECAM
    brightness. The results are different

For the sake of compatibility and to keep a historical record
(feedback), versions 5.11 are still available.

###### Others several improvements have been made - 2025

**Saturation**

Modifies the image saturation whenever the gamma/slope ratio is different from 2.4/12.92. It aims to compensate for the desaturation caused by significant luminance variations.
Values ​​between 0.2 and 1.0 are often sufficient.

**Midtones**

To try and mitigate a possible color shift when using Midtones, a combobox allows you to choose where the Midtones action is positioned:
- Before Gamma & Slope (default)
- After Gamma & Slope
- After CAM16 - so after the adjustments 'CAM16 Image Adjustments'

**Tone Mapping Operators**

Complements the processing performed by Gamma, Slope and Midtones by applying either:
- asymptotic highlight attenuation,
- or tone mapping.
Please note that this does not replace highlight reconstruction.

- Ev based: attenuates highlights using an exponential function.
- Gamma based: attenuates highlights using a hyperbolic function and partial tone mapping.
- Slope based: uses tone mapping. Slope (gray balance) acts on the dynamic range and affects shadows and contrast. 'Scale Yb scene' is a function of White Ev.
- RGB channel slope: similar to 'Slope based', but differentiable for each R, G, B channel. Attenuation threshold modifies mid gray and shadow/highlight balance.
- Sigmoid based: applies Sigmoid tone mapping with the Contrast and Skew sliders. Display white point (cd/m2) adjusts the maximum screen luminance. This new version of Sigmoid is incompatible with versions 5.11 and earlier
- TRC based: applies a simplified Tone Response Curve (TRC) to each individual R, G, B channel. Modifies the gamma by adjusting the shadow/highlight distribution (levels). 'Relative gamma' mode is a simplified method that takes into account the Slope. 'Highlight attenuation threshold' reduces the effect on the highlights.
- For the RGB operators 'RGB channel slope' and 'TRC based', the 'Invert color' checkbox simulates a film negative function.

**Screenshot**

Screenshot showing the interface with 'Invert color' enabled (The base image is not a negative).

I've tried to address user requests. You can use 'Invert color' in either 'TRC based' or 'RGB channel slope'. In both cases, you have access to a separate setting for each RGB channel.
- TRC based: The Red balance (TRC), Green balance (TRC), and Blue balance (TRC) sliders directly affect gamma. Relative gamma mode adds a slope variation.
- RGB channel slope: Red slope, Green slope, Blue slope modify the distribution of the tone-mapper between the low and high lights for each R, G and B channel. We cannot strictly speak of gamma, but the result is mathematically quite similar.

<img src="negative-cam16.jpg" title="Sreenshot Negative & CAM16" width="600"
alt="Negative-cam16.jpg" />


###### Position in the pipeline

The Abstract Profile is at the end of the process, just before Color
Appearance & Lighting.The Source Data Adjustments module, which is
integrated into the Color Appearance (CAM16 & JzCzHz) tool in Local
Adjustments, is located just after white balance, near the beginning of
the pipeline. The difference in the respective positions of these two
modules will lead to a difference in behaviour with regard to shadows
and highlights:

- Source Data Adjustments is located prior to Exposure, the Auto-matched
  Tone Curve, and the other sliders and curves located in the Exposure
  tab.
- It uses demosaiced RGB data for internal and scene condition
  calculations.
- The Abstract Profile uses RGB data values calculated at the end of the
  pipeline.

The shadow and highlight distribution will therefore depend on prior
user adjustments and the action of the Auto-matched Tone curve, which
will in turn vary with the type of image and camera.

The behaviour of the Gamma and Slope sliders will also be different in
the two cases: In general, lower values will be needed in the Source
Data Adjustments module compared to the Abstract Profile module. The
choice of primaries and illuminants will also give different results,
notably with respect to the dominant color.

###### The Influence of Ciecam

The Abstract Profile is independent of the Ciecam module , which comes
just after it in the pipeline (Color Apearance & Lighting in the Color
tab). If you activate Ciecam, the parameters of the Abstract Profile
module will be taken into account. The Source Data Adjustments module is
integrated into the Local Adjustments Ciecam module (Color Appearance).
The Source Data Adjustment settings will therefore be affected by the
ensuing colour appearance model (CAM).

###### What does Ciecam do?

We're not going to repeat the Ciecam tutorial here, but in simple terms
Ciecam, as used in both Color Appearance and Lighting and Local
Adjustments, is a software model that tries to take into account the
physiological aspects due to the perception of the eye and the brain.

Further information, albeit incomplete, on the effects taken into
account by the two versions of Ciecam implemented in RT can be found
here: [Ciecam History - effects]({{% relref "ciecam02#introduction---history" %}})

Most of the effects are automatically taken into account by the
software. Particularly in Source Data Adjustments.

The Stevens effect is a special case. It is produced by the two
'surround' comboboxes in the scene and viewing conditions modules. For
example the options in 'Scene conditions' are: Average, Dim, Dark,
Extremely Dark.

When these effects are taken into account, the image is modified in
depth to bring it closer to the conditions in which it was shot and
viewed. Of course, only the person who took the shot can fully recreate
the context.

A few comments on the impact of some of the effects that are taken into
account:

- Surround (Stevens effect) will lighten the shadows. The higher the
  surround effect (e.g. Dark), the more it will be necessary to reduce
  the action of the Tone Response Curve and in particular, the Slope.
- Simultaneous contrast, which is all the more important when the
  luminance differences in the image are large, will be taken into
  account automatically and will increase color saturation. This means
  that images where there is an imbalance between highlights and
  shadows - whether natural or artificially obtained by a mismatch
  between gamma (highlights) and slope (shadows) - will result in an
  exaggerated increase in saturation.
- The Hunt effect will (if Exif data is taken into account) be based on
  Absolute Luminance values.

The settings in the Cam16 Image Adjustments module (in Local
Adjustments) also take these effects into account. For example:

- Brightness Q (and contrast Q) are based on the Absolute Luminance
  value to differentiate them from Lightness (J) and Contrast (J).
- Saturation (s), will increase the sensation of colour in a
  differentiated way and have less effect in the shadows compared to
  Chroma (C).

To summarise for Source Data Adjustments:

- the more you act on Surround (Scene), the more you need to reduce the
  Slope and Gamma parameters compared with those in the Abstract Profile
  module.
- Colour saturation will be directly affected by large values of Slope
  and Gamma. You can adjust this if necessary using the Saturation
  slider (Cam16 Image Adjustments). The system is under your control,
  there is no AI (Artificial Intelligence).
- the fact that CAM effects are taken into account when adjusting the
  parameters in the Source Data Adjustments module means that there will
  be an impact on the distribution of light and shadow and on the
  colours. It is impossible to transpose Abstract Profile settings (at
  least those relating to luminance: gamma, slope, midtones, etc.) to
  Source Data Adjustments and vice versa other than reducing their
  overall values.

###### Summary

Ciecam, particularly in its latest version Cam16 (2020, 2022) is a very
high quality tool (even if it's not perfect). To my knowledge, it's the
only one to combine almost all the concepts of Colour Appearance Models
(CAM), which is not the case with other systems such as CieLab OKLab,
Jzazbz, etc.:

- Separation into 3 processes: Scene conditions, Image Adjustments,
  Viewing conditions;
- Tools and processes to take into account the physiological aspects of
  the human eye-brain pair: simultaneous contrast, surround, chromatic
  adaptation, etc. ;
- Use of the 6 variables required for a CAM: lightness (J), brightness
  (Q), chroma (C), saturation (s), colourfulness (M), hue (h).

However, it does have one shortcoming, which can be a handicap in the
case of difficult images (with high dynamic range, for example): it
doesn't take into account the overall mapping of the image, because
Cam16 works pixel by pixel.

The Source Data Adjustments module provides a partial solution to this
problem. It performs a global analysis of tones and colours using the
associated tools (Log encoding, Tone Response Curve with Midtones,
etc.). It is partially associated with Cam16. In other words, changes
induced by Source Data Adjustments will be taken into account by Cam16,
but without any judgement being made as to their relevance. For some
images, Source Data Adjustments settings or downstream processes
(exposure, etc.), may mean that the image becomes too contrasty in
certain areas, or too saturated, particularly in the shadows.

It is up to the user to use the tools available in Image Adjustments
(saturation, brightness, etc.) either on the entire image, or with the
help of an additional RT-spot on the areas concerned. Similarly, the
extensive processing of shadows often leads to an increase in noise,
which needs to be controlled. The denoise tool in Blur/Grain & Denoise
should help to remedy this.

##### Rawtherapee Processing Challenge feedback

Evaluation of the Source Data Adjustments 'SDA' module in Rawtherapee
Processing Challenge -March and April 2024.

[Evaluation of the SDA module](rawtherapee_processing_challenge_feedback)

### **Ciecam -JzCzHz Tutorial**

[Tutorial Color Appearance & Lighting (CIECAM02/16) and Color Appearance (Cam16 &  JzCzHz)]({{% relref "ciecam02#color-appearance--lighting-ciecam0216-et-color-appearance-cam16--jzczhz---tutorial" %}})

### **An experimental JzCzHz module**

For a brief explanation of this module, see the following:
[Experimental tool - JzCzHz]({{% relref "ciecam02#jzazbz---a-new-experimental-cam-cam16--jzczhz" %}})

#### Understanding the CAM - SDR - HDR settings - General

For a brief explanation of this module, see the following:
[CIECAM Understanding CAM SDR HDR]({{% relref "ciecam02#understanding-the-cam---sdr---hdr-settings---general" %}})

#### Understanding the CAM - SDR - HDR settings -Introduction

For a brief explanation of this module, see the following
[CIECAM Understanding CAM SRD HDR Introduction]({{% relref "ciecam02#understanding-the-cam---sdr---hdr-settings---introduction" %}})

[Experimental tool - JzCzHz]({{% relref "ciecam02#understanding-the-cam---sdr---hdr-settings---general" %}})

##### Jz in practice

<img src="jzsettings.jpg" title="Jzsettings.jpg" width="600"
alt="Jzsettings.jpg" /> There are 6 settings that interact with Jz:

- "Mean luminance (Yb%)" is the average value (expressed in % of gray)
  of the image just after demosaicing. Moving the slider to the right
  will progressively darken the final result. This slider also affects
  the reference value used by Log encoding Jz and will change the
  apparent contrast of the image before the logarithmic conversion.
- "Absolute luminance", La (described above), allows you to correct the
  value if necessary as follows:
  - To act on the common CAM (Color Appearance Model) module to take
    into account the actual value used by Jz and Q.
  - To modify the useful amplitude of the Jz values in the code (which
    we have seen were very low), by interacting non-linearly with the
    "PU adaptation" (Perceptual uniform adaptation) slider (my
    interpretation). High values of La will increase the internal values
    of Jz, whose rendering is not linear because of the PQ function.
- "PU adaptation" allows the user to adjust the internal values of Jz
  independently of the values of La.
- "Jz reference 100 cd/m2". This slider, which should be left as is for
  the moment, corresponds to the 8 bit/10 bit ratio of Jz values between
  SDR (100 cd/m2) and HDR (10000 cd/m2 chosen by Dolby). It has been
  included because it may be necessary when the module is used with a
  true HDR monitor.
- “PQ-Peak luminance” translates the value of "Peak luminance" used
  internally by the PQ function. This value, which is used internally
  for all HDR calculations upstream of the monitor, is not the same as
  the value that can be allocated to the HDR monitor (when available).
  Try adjusting PQ and you will see a variation in the shadows and
  highlights as well as the saturation.
- These last 3 "Jz remapping" settings improve the adaptation of the
  internal Jz values. In order for the curves, sliders and tools to
  react correctly, a second correction is applied temporarily to these
  tools, to put Jz in the range \[0..1\].
- Surround (Average, Dim, Dark), takes into account the environmental
  conditions at the time of shooting. Was the background around the
  scene normal, rather dim, or very dark? Adjusting this setting will
  change the appearance of the image by gradually lightening it.
- Try adjusting these 6 settings but be careful because some of them
  only have an effect when used in conjunction with Jz adjustments (Log
  Encoding, Sigmoid Jz, JzczHz adjustments).
- You can see the impact of these settings in the console, for example:

La=250.0 PU_adap=1.6 maxi=0.018249 mini=0.000016 mean=0.002767,
avgm=0.249170 to_screen=42.941518 Max_real=0.783651 to_one=1.276078
Where:

- "to_screen" is the multiplier applied to Jz, for the actual
  processing.
- "to_one" is the second multiplier allowing the curves and various
  functions to work in the range \[0..1\].

#### HDR tools Sigmoid Jz and Log Encoding

The Jz version of Log Encoding is similar in concept to the Log Encoding Module
The main difference is the way luminance is evaluated. In Log Encoding,
the module works in RGB mode and can produce hue shifts if the
evaluation is not perfect.

In the case of Log Encoding Jz the luminance uses Jz and behaves like a
CAM. The only color variations are those that have been introduced in
the conversion matrices by the researchers. The evaluations carried out
on Jz give very good results, similar to Cam16, and do not introduce hue
shifts.

The choice of Jz for the luminance component (instead of RGB) also
changes the way the values of "Mean luminance (Yb%)" and "Viewing
luminance (Yb%)" are taken into account, in particular with respect to
the calculation of the 2 gray points.

For Log encoding Jz and Sigmoid Jz, a calculation using image data just
after demosaicing evaluates the values of: a) Black Ev (darkest point of
the image), b) White Ev (lightest point of the image), c) "Mean
luminance (Yb%)".

- These values are used in logarithmic mode and luminance Jz becomes
  "(log2(Jz) - black Ev)/Dynamic Range)".
- For Log encoding Jz a unique solution to the logarithmic equation is
  calculated from the values of a), b) and "Viewing mean luminance
  (Yb%)" to linearize (or re-linearize) the values. The value of c)
  influences the distribution of the contrast, without interfering with
  the calculation. We will see that it is different with "Sigmoid Jz".

**Sigmoid Jz**

<img src="sigmoidjz1.jpg" title="Sigmoidjz1.jpg" width="600"
alt="Sigmoidjz1.jpg" />

- See [Sigmoid Jz - principles]({{% relref "ciecam02#sigmoid-jz" %}})

- Raw file (Rawtherapee - Creative Common Attribution-share Alike 4.0):
  [29](https://drive.google.com/file/d/1ziux382pWgdYa4jySimwKaKnK_KdDhno/view?usp=sharing)

As previously mentioned, fitting an HDR image (in this case 14.8 Ev with
an absolute luminance of 2000 cd/m2) into an SDR monitor (7 Ev, 120
cd/m2) without compromising anything is an impossible mission. With Log
encoding Jz or Sigmoid Jz the hue will be preserved, but the settings
for the luminance distribution between the deepest shadows, the
highlights and the midtones will depend on the user and the choices he
makes. For Log encoding Jz, the "Mean luminance (Yb%)" has a strong
impact on the result by modifying the contrast. "Viewing mean luminance
(Yb%)" will affect the overall luminance of the image. The other two
settings Black Ev and White Ev, which are logarithmic, will, like "Mean
luminance (Yb%)", affect the distribution between the shadows and the
highlights. It is obvious that the responses and therefore the settings
are dependent on the image, the monitor, and the 6 settings

### Generalized Hyperbolic Stretch and Michaelis-Menten

[Game changer](/tutorials/)

#### Introduction

**GHS - Generalized Hyperbolic Strectch**, brings a new way of processing
images. The vocabulary used is different from what we're used to
(contrast, brightness, etc.), so I'd like to take this opportunity to
make a (personal) point about the different conceptions of software
(free or paid).

This document was originally produced in French, but I won't be putting
the French version online, as it is said that in the near future, the
English version will be the basis, with translations to follow. As I'm
not an English speaker, I used DeepL to create this version. I am open
to possible amendments.

In addition, a large part of the information is contained in the
Tooltips. Their content is directly inspired by that of PixInsight
(authors of the algorithm). When I deem it necessary to avoid
redundancy, I will quote from the Tooltips.

**MM - Michaelis-Menten**, I added the (MM) algorithm to Selective Editing. This algorithm has the advantage of being simple for the user and is very efficient in most cases. It’s a **tone mapper** using the Michaelis-Menten equation, which is borrowed from biochemistry to describe enzyme kinetics .

**It has the same role as GHS, that of a 'pre-tone mapper**' which prepares the work for other 'Game changer' tools. Of course, like GHS, it can be sufficient on its own for certain images. Schematically, it's essentially the same as GHS (on the goals), but simpler, with slightly fewer options for (very) difficult images. In mathematical terms, comparing MM to GHS is like comparing the curriculum for the Baccalaureate exam to a Master's degree in Mathematics. If we put ourselves in the "mission impossible" situation, the asymptotic possibilities in the highlights are superior for GHS, but at the cost of greater complexity for the user.

#### Background on the history of digital photography - GHS && MM and Selective Editing

##### Masks and U-points / Rt-spot

There are many different algorithms for processing images. Each of them
seeks to solve contradictory problems linked to the difference between
human perception - our eye/brain pair - and the data recorded on a
camera's digital sensor. Some of these algorithms were born at the
beginning of the digital era, around 2000. A major innovation is the
work of Adobe, which designed Photoshop, taking its inspiration from
silver photography without actually writing it down, by creating an
architecture based on layers and masks. Hence the easy and favorable
welcome given to users of black and white film (Kodak, Ilford...) who
'played' with chemistry and enlargers (masks, papers with different
grades, developers, etc.). These 'photographic' users are mostly
fifty-somethings or more, who have found in this architecture a kind of
continuity.

Users have (re)discovered the notions of contrast, luminance, masks (in
a new form), blending and so on. Today, this conceptual basis is still
the basis of many paid (Capture One, Lightroom...) and free (Darktable,
Gimp, ART, ...) software programs. Most of these programs share the same
basic logic - even if their ergonomics, processing order and function
names differ. Other software packages, such as Capture NX2 (from Nik
Software), DxO and Rawtherapee, have chosen a slightly different path,
in principle more 'natural' - the notion of 'U-point' linked to color
and luminance differences (deltaE) - which, if you look closely, uses
the same principle as masks, but in a direct way. In digital
photography - unlike film - we don't use the inverted colors used in
film photography (negative, enlargers, etc.), but the direct colors
recorded on the sensor - or at least their translation into digital
values. If you wish to learn more about how softwares convert values 
from a digital sensor to an image, feel free to give [Demosaicing](https://github.com/RawTherapee/RawPedia/blob/master/content/Demosaicing/index.md) a read. 
Since the 1930s, researchers have been asking the question: "How can we 
model physical and physiological data in digital concepts that can be 
used by a photographer or a scientist, in the simplest possible way?"

##### Several concepts

Several concepts were born and are now used in (almost) all software,
often originating from CIE (originating from CIE ("Commission Internationale 
de l'Éclairage" in French, aka "International Commission on Illumination")):

- RGB data (Red, Green, Blue) - physical data actually present on
  sensors and inside digital devices (TVs, computers, set-top boxes...)
  and basic processing .
- XYZ data - and its xyY variant - which takes into account the colors
  actually perceived by the human eye, combined with photographers'
  intuitive / empirical datas: direct light (totality of the CIExy
  diagram) and reflected light (Pointer gamut). Gamut (color gamut) is a
  notion closely linked to these data, both for hardware (camera,
  monitor, printer, etc.) and for human beings.
- White balance, i.e. color balance, based on exposure conditions (sun,
  shade, type of lighting, etc.) and chromatic adaptation.
- Linear data and gamma - to express the difference between the
  luminance of the data present on the sensor and that perceived by the
  eye.
- Lab data (also used to represent gamut) - originally designed to
  measure the differences between different colors - but often used as
  the first model of color appearance. In the Lab domain, the contrast,
  luminance and chromaticity sliders react perceptually in much the same
  way as the eye.
- CIECAM data and processing (as well as its variants with similar
  concepts, etc.) - color appearance models (CAM), attempting to make use of 
  the work of researchers to solve some of the problems
  unsolvable by mathematics alone (notion of simultaneous contrast,
  surround, etc.). CIECAM is (very) often misunderstood, considered to
  be a researcher's fad, and yet anyone who has tried and understood it
  won't want to do without it....for the rest of us, it's a useless
  gizmo.

##### Photographers and engineers

Mathematics and science naturally came to the frontline to improve digital
processing, with problems similar to those encountered with film
(noise/grain, color drift and respect, sharpness, distortion, dynamic
range, etc.). These mathematics and sciences, always present, are more
or less accessible (transparent) to the user and more or less developed:
Wavelets, Fourier transform, logarithms, exponentials, hyperbolics,
sigmoid, Béziers curves, Laplacians, matrix calculations, convolution,
deconvolution, etc. When the user doesn't have direct access, or when a
simplified vocabulary is used, or when this notion is generally
assimilated, we often speak of software for photographers. As if a
photographer were, on principle, an artist with minimal knowledge of the
physical sciences and phenomena that surround him or her.

When the user has access to it - if only through the vocabulary - we
speak of software for engineers. As if an engineer were in principle a
scientist, devoid of any artistic or photographic sense (I don't think
I'm devoid of artistic tastes...). Nevertheless, certain concepts have
(almost) passed into common parlance, even if underneath this used
(understood?) vocabulary lies complex principles: Sigmoid, Log encoding,
Denoise, Blur, HDR/SDR, Gamut, etc. Are we sure that the photographer of
the 1980s-2000s even understands these terms? The complexity is above
all - beyond understanding the scientific phenomenon - a problem of
acceptance linked to habits of use, exchanges on forums and discovery of
the process by oneself (or with the help of a friend). Little by little,
we make a concept our own - without really understanding its
foundations. Who among photographers has a clear vision of what gamut or
the sigmoid function is (for a doctor, the sigmoid is the terminal part
of the intestine)?

Some of the major problems currently addressed by GHS / MM and other advanced
modules such as Sigmoid, Filmic, TRC, Log Encoding, etc., include:

- The gap between the dynamics perceived by human beings - in just a few
  seconds, our eye/brain pair is capable of adapting to the darkness of
  a church interior and the brightness of sunlit stained-glass windows,
  and to the nature of the light (sun, tungsten, LED...), and the
  dynamics linearly present on the camera sensor.
- Preservation of general contrast, local contrast and saturation when
  using these algorithms. It's normal for these functions to affect
  local contrast and saturation as a result of data stretching and
  compression.
- The discrepancy between these same dynamics and those of the media
  used to represent images: smartphones, TVs, monitors, printers.
- Failures during shooting, either due to technological shortcomings
  (dynamic range, sensor gamut, etc.), or user-related malfunctions
  (incorrect settings, blurring, etc.).
- etc.

In short, you need to use the digital data you have on display, the
photographic equipment you have at your disposal, and your own knowledge
and skills, to arrive as quickly and easily as possible (without too
much manipulation and to-ing and fro-ing), at a result that's acceptable
to yourself or your interlocutors. Depending on all these factors, some
software programs are supposed to be simple and solve (almost)
everything with a single click, while others are more complex in their
approach, solving a few extra problems. Is the game of complexity worth
the effort? To quote a well-known adage: 'A problem is only difficult
when you don't know the answer'..., or 'Practice makes perfect'.
Generalized Hyperbolic Strech (GHS) or Michaelis-Menten (MM), integrated with Selective Editing
(SE), breaks new ground. GHS algorithms is particularly innovative (I
love it). I chose to integrate it in Selective Editing, to work in
direct mode. As we'll see later, (GHS) + (SE) makes it easy to combine
several 'stretches' in normal GHS or inverse GHS, in 'Global', 'Full
image' or 'Normal spot' mode. MM is much simpler mathematically and has a simpler user interface. It gives the correct result (almost) with the default settings (or with few changes). It should be suitable in most cases. However, it lacks an Inverse mode and is less efficient in extreme cases. There's nothing stopping you from mixing them, a first RT-spot with MM, and a second with GHS, for example in 'Inverse' mode to mitigate the effects in an area that's too heavily modified (sky, sun...).

#### Hyperbolics and others asymptotiques
Both GHS and MM use hyperbolic functions, but of a different nature. Michaelis' function can work without problems in any data range (it's best to put it in [0, 1]). GHS assumes the data is in [0, 1] otherwise it clips the data, which is unfortunate. This has significant consequences in terms of code and for users. This data is in 32-bit real format, and nothing prevents the action of GHS from causing the limits to be exceeded. The output of GHS is unbounded.

There are several types of hyperbolic functions, all sharing, despite their different mathematical principles, the common characteristic of implementing asymptotic functions that approach a value without ever reaching it (a typical example being highlights):
* Those that use the usual functions 'sinh' (hyperbolic sinus) , 'cosh' (hyperbolic cosinus) , 'tanh' (hyperbolic tangent), or even compositions of functions exp(x) and exp(-x). GHS falls into this category. Its distinctive feature (hence the term 'Generalized') is that the formula changes seamlessly depending on the settings... this is what makes it remarkable. I've attached the ‘Desmos’ demonstration (feel free to expand the left panel to see the formulas). But it is complex to implement and for users  [GHS - Desmos](https://www.desmos.com/calculator/xufftbzks6?lang=fr)
* Sigmoid can also be considered a hyperbolic function for the 'highlight' part, even if its use is more in the field of statistics or AI, basically it is a model of the normal Gaussian function.
* Michaelis-Menten : this function uses the Michaelis-Menten equation, which is borrowed from biochemistry to describe enzyme kinetics. It is simple and comes in the form : (S * x) / (K +  x). Where ‘S’ is ‘Output scale’ and ‘K’ is ‘Knee strength’. It has the advantage of being easy to use and gives good results, more easily than GHS. However, it is not reversible.
* A similar function is used in 'Gamma based' and 'Slope based' in Selective Editing > Color Appearance (CAM16 & JzCzHz). In the form: dr * (x / (x + b)) + c) * kmid.  With (dr  = dynamic range)

The special case of **Log encoding** functions. 
* These are not asymptotic functions (but they do compress the data). 
* In short, we measure the minimum and maximum values ​​of the data (linearly), divide the minimum value by 2 (minVal), and multiply the maximum value by 1.5 (maxVal) to give the logarithmic function some 'margin'. 
* We try to find (not easy) the value of the real Middle Grey – which is often around 0.04 or 0.07, far from the reference value of 0.18. From the modified real data, we obtain a Dynamic Range (DR), a Middle Grey (MG source), and we take a reference Middle Grey (MG destination), which by default is 0.18. These three values ​​(DR, MG source, MG destination) are used to calculate the logarithmic base. We apply it before processing, and then its 'inverse to linear' after. The only drawback is that for complex images, there is no asymptote. Hence the addition of a hyperbolic tangent function with the 'Brightness compression' slider.
* In the case of Rawtherapee, these calculations are not performed on data just before the 'logarithmic' conversion, but much earlier. Hence the potential for significant differences (especially if processing has been done in between).
* The values ​​are expressed in Ev, which is simply a base-2 logarithm transformation:

```
    const float dynamic_range = -xlogf(minVal / maxVal) / log2;
```


#### Generalized Hyperbolic Strech - GHS - origin

I believe (to be verified) that two people came up with a new approach
to the general problems seen above. Most of the methods we know today 
are derived from the work of the film industry by ACES or in the continuity
of the precursive work of Adobe (I can cite - non-exhaustively -
functions such as: Log encoding, Sigmoid, Gamut compress, etc.). However, 
two people, David Payne and Mike Cranfield, have come up with 'something
else' with a vocabulary that is a little 'disturbing'. The system uses
foreign concepts such as 'Symmetry Point', 'Stretch' and 'Local
Intensity'. The internal functions are quite complex, working at pixel
level. They are continuous functions that modify pixel intensity and,
according to their authors, enable:

- initial stretching (and compression) of pixel values from a linear
  state.
- add contrast to key areas of the image.
- general brightening or darkening of the image.
- adjust the dynamic range of the image.

I've also added the possibility of using several modes (RGB, Lch,
Saturation, Hue) to modify the image as desired: maintaining overall
balance while preserving contrasts, etc., or acting as a Color-toning.

These algorithms and functions have found a home in astrophotography
software (Siril, Pixlnsight). Examination of the tutorials and code for
these two programs (Siril code only) shows that the initial scope is
broader than astrophotography, and can be extended to general
photography. The major difference - whether foreseen by the
authors or not - is the extension to highlights and high dynamic range, 
rarely found in astrophotography.

#### GHS - available settings
<figure>
<img src="ghs-settings.jpg" title="GHS-settings.jpg" width="600" />
<figcaption>GHS_settings.jpg</figcaption>
</figure>

##### Simplified operation - Principle

Five settings act directly on GHS:

- Stretch factor (D): controls the extent of the stretch. It can only have positive values.
  'Negative space' can be implemented with 'Inverse GHS'.
- Local intensity (b) - linear factor: controls the degree of the stretch 
  that focuses around the Symmetry point (SP), by modifying the
  shape of the transformation itself.
- Symmetry point (SP): defines the focus point around which the stretch
  is applied - the contrast will be distributed symmetrically with
  respect to (SP). While (b) determines the degree of focus of the
  stretch, (SP) determines where this focus is. (SP) should
  generally be placed close to a histogram peak(s) so that the stretch
  widens and lowers the peak(s), adding the most contrast to the stretch
  at this point.
- Protect shadows (LP): defines a value below which the stretching would
  be modified to preserve contrast in shadows and lowlights. To achieve
  this, a linear transformation of the data is performed below the (LP)
  level, reserving the contrast of the rest of the image. Among other
  things, this makes it possible to better control noise.
- Protect highlights (HP): defines a value above which the stretching would
  be modified to preserve contrast in the highlights. To do this, a linear
  transformation of the data is performed above the HP level, reserving
  the contrast of the rest of the image. This allows you to better
  control the progression of highlights.

In simplified terms, by assimilation, the system functions as an
'S-curve' (or inverted S-curve) that modifies the image while preserving
contrast, highlights and lowlights through the use of asymptotic
functions. The inflection point (where it exists) is defined by the
symmetry point (SP), e.g. SP=0.5 will generate a symmetrical 'S-curve'
for RGB values below the symmetry point (SP) or above. The Stretch
factor (D) will make this curve more or less pronounced, with very
progressive asymptotes in the low and high light ranges. The linear
factor - Local intensity (b) - modifies the shape of the 'S', reducing
or increasing the 'length' of the asymptotic parts.

These three factors can be used to modify image contrast, by attempting
to preserve local contrast overall, filling in valleys and reducing
peaks.

##### Inverse GHS
Only available in Standard and Advanced modes.

Inverse GHS checkbox: Inverse GHS is useful for reducing overall contrast or 
recovering your original image. It adds contrast on the far right and left 
of the histogram, brightening the deep shadows, darkening the bright highlights, 
and squeezing the histogram towards the middle.

The Generalized Hyperbolic Inverse transformation allows you to recover 
your original image, subject to mathematical precision, but you can also 
use it for other purposes. This feature is more like the 'Excluding spot' 
but more powerful.

If the White point and Black point were changed in normal mode (automatic 
or not), they will likely need to be readjusted. The white point often needs 
to be readjusted lower. There is interaction between the two settings. In the 
majority of cases, especially when used as a second GHS Spot, the adjustments 
will be minimal, or even unnecessary.

It can be used in a second GHS spot after a first GHS spot in Global or 'Full image' 
mode, most often with 'Spot method = Normal spot', to, for example, darken sky or sunset.

To allow Inverse GHS to work the checkbox 'Auto Black point & White point' should be disabled. 


##### The need to fine-tune White Point (linear WP) and Black Point (linear BP)

The algorithm developed by David Payne and Mike Cranfield assumes that
all image data to be processed is in the interval \[0 ,1\] (32-bit real format)

- Negative values or values greater than 1 will be ignored (clipped).
- If shooting conditions or upstream processing reduce the practical
  interval to different values, e.g. [0.2, 0.9], the algorithm will
  only partially fill them. It is therefore advisable to find the values
  of the black and white points to obtain an interval of data before GHS
  processing in the range as close as possible to [0, 1].
- Move the White point (linear WP) slider slightly. Move this slider
  until you obtain a Clipped White point value close to 0. You will also
  see the values of data that may have been lost or misused.
- Move the Black point (linear BP) slider slightly to the right
  (positive values). Move this slider until you obtain a minimum Clipped
  Black point value. If no upstream processing has been performed, the
  first value to be retained will be Clipped Black point = -1.
- The 'Highlight reconstruction' method has a very strong impact on the
  White-point value. and therefore on the amplitude of the RGB data which
  will then be brought back into the interval [0. 1]. It seems obvious that
  the values ​​of Stretch factor (D), Local intensity (b), Symmetry point (SP)...
  will be (depending on the action of the reconstruction process) profoundly impacted. 

###### Associated Tooltips
**Black point/White point**: Sets the 'Black point (linear BP)', 'White point (linear WP)' for a linear stretch.

* 'Black point (linear BP)' and 'White point (linear WP)' settings are sensitive to the general settings upstream of GHS i.e. highlight reconstruction, white balance and RAW processes.
* To have relevant 'Black Point (linear BP)' and 'White Point (linear WP)' settings, the entire image must be analyzed. It is recommended to use 'Fit whole image to screen - Shortcut Alt-f'.
* To allow Inverse GHS or Matrix conversion (AgX, Jz, Cat16) to work the 'Auto Black point & White point' must be disabled. 

**Black point** Sets the black point for a linear image stretch.
* In normal GHS mode, negative slider values lift the shadows linearly to avoid excessive noise and facilitate 
* For positive slider values, the histogram is shifted to the left. For raw images, you can also use the more precise Raw Black Points > Dehaze in the Raw tab.
* Increases in contrast due to the linear stretch will be evenly distributed over the image.
* You can adjust a linear black point offset to either: a)account for noise in the deep shadows. b) adjust the histogram.
* The label 'Clipped pixel count Shadows:x Highlights:y' shows the number of pixels that would be clipped without adjusting the two sliders.
* The label ‘Pixel values - Darkest:w Lightest:z’ shows you the minimum and maximum values in the range [0, 1].
* The label 'RGB values - R:r G:g B:b' shows you the maximum values for each channel.
* The label 'Dynamic Range GHS (Ev)' shows you the Dynamic Range, which uses the value of the highest and the lowest of the 3 RGB channels, instead of the luminance.

* In  Inverse GHS mode the behavior is reversed and there are possible interactions with the White point. In the majority of cases, especially when using a second GHS Spot, the adjustments will be small, or even unnecessary.
* To activate the Inverse GHS checkbox, the checkbox 'Auto Black point & White point' must be disabled. 

**White point** Sets the White point for a linear stretch of the image. Any pixel with value greater than the white point input will be clipped and the data lost.
* Contrast gained by performing the linear stretch will be evenly distributed over the image, which will be brightened. Pixels with values greater than the white point will appear white and have a value of 1.0.
* Setting this parameter to a value greater than 1 will extend the dynamic range at the high end.
* The 'Highlight reconstruction' method has a very strong impact on the white-point value.

* The label 'Clipped pixel count - Shadows:x Highlights=y' shows you the number of pixels that would be clipped without adjusting the two sliders.
* The label 'Pixel values - Darkest:w Lightest:z' shows you the minimum and maximum values in the range [0, 1].
* The label 'RGB values - R:r G:g B:b' shows you the maximum values for each channel.
* The label 'Dynamic Range GHS (Ev)' shows you the Dynamic Range, which uses the value of the highest and the lowest of the 3 RGB channels, instead of the luminance.

* In Inverse GHS mode the behavior is reversed and there are possible interactions with the black point.
* To allow Inverse GHS to work the checkbox 'Auto Black point & White point' must be disabled.

**Auto Black point & White point** Automatically adjust the 'Black point' and 'White point' values.
* Not available in Inverse GHS mode and must be disabled to allow Inverse GHS to work. It must be disabled (and re-enabled) in order to change 'Matrix conversion (AgX, JzAzBz, Cat16)'.
* Generally, the automatic settings will be satisfactory.
* If necessary, you can subsequently adjust the two points:
* The 'Black point': using negative slider values may help when the shadows are particularly blocked.
* Manually increase the ‘White point’ value if necessary, especially if it is less than 1, to give the system more flexibility by avoiding possible gamut overflows due to GHS or subsequent adjustments.


##### Recommendations

<figure>
<img src="./gamu-profil1.jpg" title="Gamu-profil1.jpg" width="600" />
<figcaption>Gamu-profil1.jpg</figcaption>
</figure>

To make these settings easier, set the 'gamut' knob so that the data and
histogram use the same values as those - linear + working profile - used
by GHS. While setting (linear WP) and (linear BP), observe the
histogram. On the left, the histogram should be close to the vertical
axis, with no off-gamut values (linear BP). On the right, the histogram
should be close to the vertical axis, with no out-of-gamut values (linear WP). The setting of (linear WP) is particularly influenced by the
activation of Highlight reconstruction. In 'Inverse GHS' mode, the White
point (linear WP) and Black point (linear BP) settings must be
different. For example, a value of 1.8 for (linear WP) will be required
in GHS mode and perhaps 0.8 in Inverse GHS mode. In addition, there is
interaction between the two settings.

##### Matrix
Use the LMS (Long, Medium, Short) transformation, depending on the user's selection. LMS is a color space transform which represents the response of the three types of cone cells of the human eye, named for their responsivity (sensitivity) peaks at long, medium, and short wavelengths.

These transformations, in the form of matrix, modify each R, G and B channel for each pixel, giving priority to certain dominant colors according to your choice.
* None - leaves the data unchanged (default).
* AgX - accentuates each R, G, B channel quite strongly so that the LMS data is perceived as more colorful.
* JzAzBz (XYZ High dynamic) - modifies the LMS distribution, by acting on X, Y, Z to better account for images with a high ‘linear WP’.
* Cat16 (XYZ harmonious) - modifies the LMS distribution, by acting, on X, Y, Z - ensures a more harmonious distribution of colors while respecting the eye/brain interaction.
* JzAzBz (RGB High dynamic) - modifies the LMS distribution, by acting, like AgX, on the 3 channels R, G, B to better account for images with a high ‘linear WP’.\n * Cat16 (RGB harmonious) - modifies the LMS distribution, by acting, like AgX, on the 3 channels R, G and B - ensures a more harmonious distribution of colors while respecting the eye/brain interaction.

For JzAzBz and Cat16, the XYZ mode : JzAzBz (XYZ High dynamic) and Cat16 (XYZ harmonious) should ensure better color and luminance fidelity.

This transformation only affects the modifications made by GHS: 'linear WP', 'linear BP', 'Symmetry point (SP)' and of course the action on Stretch factor (D) and Local intensity (b). It is imperative to review the GHS settings each time the matrix is ​​changed. It is strongly recommended to activate 'Auto Black point and White point' (except in Inverse mode). Observe the histogram in the two modes 'Working profile - linear' and 'Gamma - corrected output profile'.

Be careful :
* Not to confuse Cat16 with CIECAM. Cat16 is only the conversion matrix, while CIECAM (Color Appearance & Lighting) is a complete colorimetry process that takes physiological aspects into account.
* The JzAzBz matrix here, does not take into account the notions of 'Absolute luminance' and so does not use the notion of PQ (Perceptual Quantizer). Do not confuse with the module in Selective Editing > JzCzHz.

**Comments**
Performs the entire GHS processing, including Matrix conversion either AgX or JzAzBz or Cat16. For JzAzBz and Cat16 you have the choice between a transformation in RGB mode or in XYZ mode (preferably).
* To allow this matrix to be enabled or disabled the 'Auto Black point and White point' must be disabled.
* You need to re-enable 'Auto Black point and White point' to recalculate the values ​​of 'linear BP', 'linear WP' and 'Symmetry point (SP)'.



###### First image in Neutral mode, without Black Point (linear BP) and White Point (linear WP) retouching

Raw image: D200_20070802_2087.NEF

- Raw file (Creative Common Attribution-share Alike 4.0):
  [30](https://drive.google.com/file/d/1mSRiKwmnLoKGaGE4pa_vnA80gzPOobYX/view?usp=drive_link)

Look at the histogram: on the left, the values start far from the
origin; the Black Point needs to be adjusted. Look at the data Clipped
pixel counts Highlights = 230 and Pixel values Darkest:0.12 and
Lightest=1.08 confirm the need to adjust the Black point, but also the
White point.

<figure>
<img src="bp-wp-1.jpg" title="BP-WP-1.jpg" width="600" />
<figcaption>BP-WP-1.jpg</figcaption>
</figure>

###### First image in Neutral mode with Black point and White point retouching

- retouch the Black point (BP linear), shift the slider to the right,
  until the histogram on the left is close to the vertical axis, and
  Clipped pixel count and Pixel value - Darkest are at 0. In the
  example, the value found is 0.1247.
- retouch the White-point, until Clipped pixel count Highlight is 0, and
  Pixel values Lightest is 1. In the example, the value found is 1.0809.

<figure>
<img src="bp-wp-2.jpg" title="BP-WP-2.jpg" width="600" />
<figcaption>bp-wp-2.jpg</figcaption>
</figure>

###### Second image in Neutral mode, without Black Point (linear BP) and White Point (linear WP) retouching

Raw image: 5D3_0104.CR2

- Raw file (Creative Common Attribution-share Alike 4.0):
  [31](https://drive.google.com/file/d/1tsmOPbyfnya-mLtHmKE6U0Dba0f2cOaC/view?usp=drive_link)

Look at the histogram: the right-hand part is far from the right-hand
vertical axis, and Pixel values Lightest is at 0.63, so the white point
needs adjusting. <img src="bp-wp-3.jpg" title="BP-WP-3.jpg" width="600"
alt="BP-WP-3.jpg" />

- retouch the White point (linear WP) until Clipped pixel count Highlight
  is 0 and Pixel values Lightest is 1.
- check that the values set (here 0) are correct by moving the Black
  point on the right-hand side of the slider (linear BP). If you move
  the slider a little to the right and the values for Clipped pixel
  count Shadows and Pixel value - Darkest are no longer zero, you should
  not change the Black point setting (linear BP).

You will be able to see on the images that require a 'Highlight
reconstruction', the effectiveness of the various methods proposed, by
looking at the White point (linear WP) value. For example:

- None: 1.1
- Inpaint Opposed: 2.1
- Color Propagation: 3.2

##### Particularities of Black Point (linear BP) and Highlight Attenuation checkbox

- If Black Point (linear BP) is used with negative values, where
  possible (Black Point indicators at zero), the black point will not be
  modified, but the luminance of very low lights will be increased. This
  action will facilitate the work of the GHS algorithm by raising the
  Symmetry point (SP).
- The Highlight Attenaution checkbox completes the processing of
  highlights - Protect Highlights (HP) - in cases of high Dynamic Range,
  the GHS setting may be insufficient. This action on the checkbox will,
  on one hand, attenuate highlights after GHS and, on the other,
  take into account the Protect Highlights (HP) setting to increase the
  effect of this attenuation.

##### The essential role of Symmetry point (SP)

This is probably the key factor in understanding how GHS works. It is
around this point that image transformations will take place. As this
algorithm is designed to process highly dynamic and often under-exposed
images, I have chosen a default value of 0.015 (arbitrarily). The system
works on linear data, in the Working profile (default: Prophoto) - the
values to be taken into account are naturally offset from the image
rendered in the Preview (which by default takes into account a gamma and
the monitor profile). As with the (WP linear) and (BP linear) settings,
it is therefore advisable to toggle the histogram and gamut button
readouts to linear and working profile mode. By observing the histogram
in RGB mode, you can assess the position of the luminance peak(s). It's
around this value(s) that you need to adjust (SP). The % position of the
histogram peak on the 'x' axis gives a good approximation of the (SP)
setting.

If the image is very underexposed, or has two significant luminance
peaks, it is advisable to proceed in 2 (or more) steps with smaller
Stretch factor (D) values. If the Black point setting (BP linear) allows
(no use of positive values to re-establish contrast), you can slightly
open up the shadows with negative Black point values, to enable better
evaluation of the Symmetry point (SP). This action is performed before
GHS. It is illusory to believe that it is possible (as in astrophoto -
unless you can show me otherwise) to automatically adjust (SP) from a
value captured on the image or histogram. This is certainly relevant in
astrophoto, for example using the luminance value of a nebula, but
unrealistic here. The position of the peak(s) in the histogram seems to
me sufficient, more relevant and sufficient.

**The 'Auto' checkbox**
Try setting an automatic Symmetry Point (SP) estimation, only in RGB mode. May require manual adjustment.


###### Associated Tooltip

Default 0.015 to avoid the zero value.

- This is the key balance value of the GHS system.
- Sets the focus point around which the stretch is applied - contrast
  will be distributed symmetrically about (SP).
- While (b) provides the degree of focus of the stretch, (SP) determines
  where that focus is applied.
- (SP) should generally be placed within a histogram peak so that the
  stretch will widen and lower the peak by adding the most contrast in
  the stretch at that point. Pixel values will move away from the (SP)
  location.

###### Automatic Black point & White point - Estimation Symmetry point (SP)
To make GHS more intuitive and easier to use, I added 2 features:
- Automatically calculate Black point (linear) and White point (linear) - not in Inverse GHS mode. This allows to compensate for example (as poor Dehaze does) the black point in foggy images and take into account the reconstruction of highlights. As a reminder, unlike other Tone-mappers, these 2 points are in linear mode. The aim of the operation is to bring the data back into the interval [0 1]. Of course it is possible to retouch in manual mode, for example negative values ​​(depending on the images) to open up overly pronounced shadows and help GHS.
- Provide an evaluation of the Symmetry Point (SP). This value, which has nothing to do with 'middle gray' is essential to understanding GHS and the results. 0.015 is chosen as the default value. The evaluation seems good to me in RGB Luminance mode, acceptable in RGB mode and it is not provided for the other modes, because it has no value. 
- It’s just an estimation of the symmetry point from the data in linear mode. This point corresponds to the maximum of the histogram in linear mode and with the working profile. It is up to the user to choose and adapt this value by adjusting the slider (SP).
- If the checkbox added to the slider (SP) is checked: Try setting an automatic Symmetry Point (SP) estimation, only in RGB mode. May require manual adjustment.

<img src="ghs-bw-sym2.jpg" title="Black point - White point - SP" width="600"
alt="Ghs-bw-sym2.jpg" />

##### Incidence of Local intensity (b)

High positive values of (b) can be considered as histogram wideners,
i.e. they lead to a wider spread of the histogram around the point of
focus (SP). On the other hand, lower values of (b) tend to shift the
histogram towards a brighter (or darker) rendering without affecting its
width too significantly. As a general rule, the level of (b) used
decreases as more stretching sequences are added, when using a second
spot.

###### Associated Tooltip

This parameter controls how tightly focused the stretch is around the
Symmetry point (SP) by changing the form of the transform itself:

- For concentrated stretches (such as initial stretches on linear
  images) a large (b) factor should be employed to focus a stretch
  within a histogram peak while de-focusing the stretch away from the
  histogram peak.
- For adjustment of non-linear images, lower (b) parameters should be
  employed to distribute contrast and brightness more evenly.
- Large positive values of (b) can be thought of as a histogram widener,
  i.e. spreading the histogram wider about the focus point, SP.
- By contrast, lower values of (b) tend to shift the histogram to a
  brighter (or dimmer) position without affecting its width too greatly.
- As a general rule, the level of (b) employed will decrease as a
  stretch sequence nears completion, although larger (b) values can
  still be employed for precise placement of additional contrast.
  
###### Some comments to better understand GHS
To help the user understand what GHS and its inverse function are (a little bit better), here's a few comments.

As you can see:
- 'Stretch factor (D)' only has positive values.
- There are no 'contrast' or 'lightness' sliders, no more 'middle grey' or 'pivot'.


The problem (not just that of labels and tooltips - labels and tooltips are difficult to write because you have to try to convey something else) is complex. Wanting to create a dynamic system that calculates (BP), (WP), and (SP) in real time and allows for inverse operation (Inverse GHS) is difficult; I'm not trying to be pretentious. However, it's completely different from anything being done elsewhere. I'm not saying that what is done elsewhere (in RawTherapee or other software...) is less good or bad, but it is very different.

As a reminder, because I've already mentioned it:

- (BP) and (WP) calculations are in linear mode, not with Ev.

- It's important, even on ordinary images, to enable 'Color Propagation' (even if you can disable it later).

- Combining the two allows you to retrieve out-of-gamut colors (including ACES-P0). When the system finds a (WP) of 10, it means that within the limits of the usual data gamut (0 - 65535), the values ​​taken into account are about 655000...when the illuminant consists of, for example, LEDs.

The system acts as a gamut compressor (a bit like 'Gamut Compression', but for the 3 RGB channels) and 'fits' the data into the gamut.
 
Adjusting the (WP) (for example, reducing it) will limit the maximum value. Adjusting the ‘Stretch Factor (D)’ and ‘Local Intensity (b)’ will allow you to focus on the area of ​​the image to be enhanced. If necessary to refine the colorimetry, you can use ‘Abstract profiles’, in particular the 'Custom (CIExy diagram)' part, Refine colors, Illuminant.

And of course, if you create a second Spot, the (BP) and (WP) values ​​are recalculated and often close to 0 and 1, and (SP) recalculated.

The 'Inverse GHS' function is (somewhat) similar to 'Excluding spot', but more sophisticated, to put it modestly.

It allows you to 'undo' previous actions in a specific area of ​​the image, for example 'removing' or ‘reduce’ Stretch in sky or sunset.

But it also allows you to work in 'negative space', that is, to treat the 'Stretch factor (D)' as if it had negative values ​​(which is not normally possible). In 'Inverse GHS' mode, the contrasts will be reduced overall (instead of being increased). The entire system will operate in reverse. Look at 'GHS curve visualization' and you'll see that when you increase 'Stretch factor (D)', the curve curves downwards...  


###### Mathematical principles used for transformations depending on the value of (b)

- \(b\) = -1: logarithmic;
- \(b\) \< 0: modified hyperbolic;
- \(b\) = 0: exponential;
- \(b\) \> 0: hyperbolic.

Where conditions permit:

- (LP) less than (SP);
- (HP) greater than (SP).

and the user acts on the Protect shadows (LP) or Protect highlights (HP)
sliders, the functions mentioned above for (b) are progressively
transformed into linear functions. These two adaptations are a key
factor in the control of lowlights and noise, and the progressiveness of
highlights. I've adapted the GUI so that the range of possible settings
for (LP) and (HP) is a function of (SP).
<img src="ghs-visu.jpg" title="Ghs-visu.jpg" width="600"
alt="Ghs-visu.jpg" />

Representation with Desmos
[32](https://www.desmos.com/calculator/xufftbzks6)

##### One recommendation: use GHS in Neutral mode

It's important to control GHS under optimum conditions. This means
eliminating any parameters that could disrupt the analysis. I recommend
working in Neutral mode. You can (must) activate:

- Highlight reconstruction: which has a very strong impact on the
  calculation of the White point (linear WP). In most cases, 'Color propagation' is preferred - with or without Clip out-of-gamut colors, depending on the nature of the image. Generally, uncheck 'Clip out-of-gamut colors'.
- Denoise - if necessary.
- White Balance: Auto - Temperature correlation is preferred in most cases except for illuminants, which are far from daylight or blackbody (LEDs, Halogen...).
- Capture Sharpening and raw functions as Raw Black Points (Dehaze): Check if the sliders are not zero. If they remain at zero, disable "Dehaze".
- Gamut Compression: observe the effect on the histogram after selecting the 'Target Compression Gamut' (often sRGB). Slightly adjust the three threshold sliders to see (or not) the changes in the histogram and image. Adjust 'Power'. 
- Toggle histogram and data display to 'Working profile - linear' mode.

In Selective Editing, activate GHS as the first tool, in the first Spot
you create.

Once GHS has been set up, you can activate additional contrast,
luminance and saturation functions if required, paying particular
attention to those upstream of GHS, such as Contrast By Detail Levels or
Haze Removal, which will modify the data received by GHS - and retouch,
if necessary, the black point (linear BP) and white point (linear WP).

- Toggle histogram and data to 'Output profile – gamma'.

- Abstract Profile - Fine-tune the image settings, primarily adjusting Gamma and Slope. Enable Contrast Enhancement : The system uses Wavelets with a complex algorithm based solely on signal processing. You can modify the 'local contrast' according to two main principles: a) Contrast profile, which allows you to change the extent of contrast detail to be reduced or amplified, from 2x2 pixels up to 1024x1024; b) a curve that distributes the effect around the average contrast for each level of decomposition. Primaries & Illuminant: It's a difficult tool to master, especially the primaries. But you can safely modify the illuminant (for example, going from D65 to D41) or use 'Dominant color'.

- Color Appearance & Lighting : It uses CIECAM, which is probably, to my knowledge, the best tool for managing color. It has three processes (Scene, Image Adjustments, Viewing). It uses six variables: J - lightness, Q - brightness, C - chroma,  S -saturation, M - colorfullness, h - hue rotation.It takes into account the shooting and viewing conditions, as well as numerous physiological factors. In addition to the usual settings, you can separately adjust the 3 R, G, B channels for hue rotation, saturation and brightness. 

##### Various data processing modes - RGB, Lab, HSL

6 modes are available:

- RGB Standard (default setting): the 3 RGB channels are modified in the same way.
- RGB Luminance: the 3 RGB channels are modified by
  taking into account the average luminance obtained from the working
  profile's XYZ data. Recommended mode.
- Luminance & chromaticity (Lab): these 2 values, L (luminance) and C
  (chromaticity) after RGB \> Lab transformation, have different peaks
  for L (luminance) and C (chromaticity), which seems obvious. I've
  chosen to let the user - with the help of the histogram - manage this
  difference by adjusting the Chromaticity Lab factor (C) slider, and/or
  the Slope Lab factor (S) slider in Advanced mode. These settings
  modify the GHS function in different ways for L and C, and modify the
  linear part of the Lab transform (which is in fact a TRC Tone Response
  Curve). Acting on GHS in Lab mode is no longer a linear mode, and will
  generally have the following consequences, compared to RGB mode:
  - increase the Symmetry Point (SP).
  - reduce the Stretch factor (D).
- Luminance (HSL): after RGB \> HSL conversion - is a linear
  transformation. Only the luminance channel is taken into account. It's
  likely that the Saturation (HSL) component will need to be modified
  with another Spot.
- Saturation (HSL): after RGB \> HSL conversion - is a linear
  transformation. Only the saturation channel is taken into account. It
  is likely that the Luminance (HSL) component will then need to be
  adjusted with another Spot.
- Hue (HSL): after RGB \> HSL conversion, is a linear transformation.
  Only the Hue channel is affected. The GHS transform will profoundly
  modify the image colors with Color Toning effects.

I haven't chosen to propose a separate action on each R, G and B
channel, but of course it's possible.

##### Stretch Regularization & Midtones

It is normal when stretching or compressing an image that the local
contrast and saturation are changed, it cannot be otherwise.

- the Value (LC) slider, allows to compensate the local contrast (the
  saturation part is "automatic" only in RGB Luminance mode). The way
  this compensation acts depends on the data processing mode, the most
  efficient is RGB Luminance. But the action on this slider does not
  replace a user's wish to increase the Local contrast, it will be up to
  him to act for example with the Selective Editing Wavelet tool - Local
  contrast and/or Clarity.
- the Midtones slider, allows to retouch the midtones, only if
  necessary, after running GHS to restore the average luminance values.

#### A simple example of using GHS

Raw image: IMGP2426.DNG

- Raw file (Creative Common Attribution-share Alike 4.0):
  [33](https://drive.google.com/file/d/17R3iBq08s71DuDRiv9T6TzlJVsxqBISo/view?usp=drive_link)

##### First step:

- - Neutral – Histogram in ‘working profile – linear’ mode.
  - Highlight reconstruction – Color Propagation
  - White Balance Auto - Temperature correlation
  - Selective Editing: Global
  - Equalization & Pre-Tone Mapping
  - Generalized Hyperbolic Stretch (GHS)

<img src="ghs-example1.jpg" title="Ghs-example1.jpg" width="600"
alt="Ghs-example1.jpg" /> In this first step:

- the White point (linear WP) has a high value, mainly due to the use of
  Highlight reconstruction – Color propagation.
- the Black point (linear BP) is used with a negative value, to slightly
  open up the shadows, reduce the need for a high Stretch factor (D) and
  slightly increase the value of Symmetry point (SP).
- Note the values ​​of Protect shadows (LP) at 0.0 and Protect Highlights
  (HP) at 1.0 and the non-use of Highlight attenuation.
- The Graduated filter is activated with the Spot positioned in the
  middle of the sky, to reduce the brightness of the upper part of the
  image.

##### Second step:

- Histogram in ‘output profile – gamma’ mode.
- To give more contrast and saturation to the image – for educational purposes, 
  the necessity can be debated – I activated in the ‘Exposure’ module –
  Auto-Matched Tone Curve. Other choices like Selective Editing – Wavelet –
  Local Contrast also works.
- In order to have a better rendering of the sky, I activated Highlight
  Attenuation and set Protect Highlights (HP) to 0.9.

<figure>
<img src="ghs-example2.jpg" title="Ghs-example2.jpg" width="600" />
<figcaption>Ghs-example2.jpg</figcaption>
</figure>

###### Recommendations and other comments

**Certain choices regarding the composition of GHS categories**

- Saturation adjustment, often necessary after stretching, is performed by default without user access. There are other places in 
 Selective Editing, CIECAM, etc., where saturation can be modified. However, if users wish, a 'saturation' slider can be added.

- The GHS module contains the minimum tools required for operation. It will rarely function completely independently. 
  It would have been possible to add components found elsewhere, for example in Selective Editing > Color Appearance (CAM16) or
  Abstract Profile (Color Tab), to adjust the balance of light and shadow (TRC) or Contrast Enhancement, but I preferred to
  perform these adjustments elsewhere than in GHS.

- Stretch Regularization & Midtones: The Value (LC) slider affects local contrast using a Guided Filter. It can introduce artifacts 
  or unwanted behavior in some images (LEDs, etc.). You can disable or reduce its effect.

**Recommendations**

- It is preferable to use GHS as the first Global RT-spot; this will allow us to resolve the problem of equilibria due to deviations from [0 1] (32-bit real format)
  of the values ​​of the Black point (linear BP) and White point (linear WP). In fact, GHS behaves like a pre-tone-mapper. Black point correction
  (linear BP) in Raw corresponds roughly to Raw Black Point > Dehaze (Raw Tab), while White point correction (linear WP) can be considered
  a better use of Raw White Points (Raw Tab). Ideally, GHS would have been developed using raw data, but this option would have two major
  drawbacks: a) the assignment of the Working Profile and White Balance would not have been performed, resulting in significant color shifts;
  b) it would be impossible to process non-raw images (TIFF/JPG).

- To avoid high stretch values ​​that can lead to weakening of local contrast and saturation, you can use two (or more) RT-spots, 
  one after the other with lower stretch values.

- When the White Point (linear WP) and Black Point (linear BP) values ​​are low, for example [0.1 - 0.4], the image contrast will be 
  significantly increased. In these extreme cases, it can be useful to create a second 'Inverse GHS' spot to reduce contrast.

- When the White Point (linear) values ​​are high (3 to 5) or very high (10 or more), the system forces colors present on the sensor,
  which may be far beyond the visible spectrum, into the gamut perceived by the human eye. Adjust the White Point (linear WP), Stretch (D),
  and Local Intensity (b) values ​​as needed. Feel free to use the Gamut Compression module, or the Primaries & Illuminant section of
  Abstract Profile, to try to obtain colors acceptable to our perceptions.

- Feel free to use the Graduated Filter (at the bottom of the GHS menu) in 'Fit to screen' mode. This allows you to easily, for example,
  make skies more realistic.

#### Michaelis-Menten - MM - origin  
 This tone mapper uses the Michaelis-Menten equation, which is borrowed from biochemistry to describe enzyme kinetics. In image processing, it creates a smooth, saturating curve that is useful for tone mapping.

##### MM Settings
<figure>
<img src="mm-settings.jpg" title="MM-settings.jpg" width="300" />
<figcaption>MM-settings.jpg</figcaption>
</figure>

###### **Basic algorithm**
* **Exposure**: Adjusts the input image brightness.
* **Output scale (S)**: Controls the maximum asymptotic value of the curve; essentially the output white level.
* **Knee strength (K)**: Determines the “knee” of the curve. Lower values result in a sharper transition to the compressed highlight region.
* **Saturation**: Adjusts color saturation post-tone mapping.
* **Output max clamp**: Sets the final clipping point for the output values.

###### **JDx Matrix**
The JDx checkbox allows the user to choose either the default or the JDx LMS transformations. LMS is a color space transform which represents the response (sensitivity) of the three types of cone cell in the human eye at long, medium, and short wavelengths.
These transformations are matrices that modify each of the R G B channels for each individual pixel, giving priority to certain dominant colors depending on the selected option.
* Unchecked: leaves the data unchanged (default).
* JDx: accentuates each of the R G B channels by acting on X Y Z such that the LMS data is perceived as being more colorful.

JDx Matrix: How it works ? I'm attaching part of the code.
```
Matrix lms_mat = {{//JDx - Jacques Desmis Matrix XYZ -> LMS - Simple matrix that amplifies the current channel.
                        { 0.83, 0.1, 0.07 },//Red (L) predominant in LMS with a little more green
                        { 0.12, 0.78, 0.1 },//Green (M) almost neutral, with a little more red
                        { 0.11, 0.09, 0.80 }//Blue (S) almost neutral, with a little more red
                 }};
```
As you can see, it's a kind of table (a matrix with 9 numbers). Its use is simple. If you observe the pixels of each image, they will have (we'll stick with a simple case) values ​​like this for a pixel: X=0.4, Y=0.2, Z=0.7.
To obtain the value of 'L', we multiply the coefficients of the first line with the values ​​X, then Y, then Z respectively, by adding them together
This will give for ‘L’ : 0.83 * 0.4 + 0.1 * 0.2 + 0.07 * 0.7 = 0.401. This value will become the new data used. We do the same for 'M' and 'S' with the second and third lines.

It is important to note that I was not referring to RGB data, but XYZ data. In 1931, researchers from the CIE (Commission Internationale de l’Eclairage – in french). They imagined a reference space to free themselves from the constraints of RGB spaces (they did this with a panel of volunteers) which led, in particular, to the CIExy diagram (the limits of human vision). It is therefore important that the matrix be independent of the Working Profile and remain the same; for example, if the user chooses 'AdobeRGB' instead of 'Rec2020', the matrix will remain the same. Of course, the RGB data must first be converted to XYZ (and vice versa).
Of course, for the user it's completely transparent. The inverse matrix is ​​calculated... I chose a simple matrix (there are only 2 significant numbers), giving more pleasing images and lowering (a little) the white point in out-of-gamut images.

###### **Black point & White point  & Attenuation threshold**
The concept is completely similar to that of GHS; I've just simplified the interface to create a system that's easier to use than GHS.
**Subtract linear black & Linear dynamic range** 
- Subtract linear black: reduces the linear black point values ​​to almost zero. This makes optimal use of the available data by increasing overall contrast and reducing haze effects.
- Linear dynamic range: allows you to include any data outside the color gamut (in the case of sunsets for example). Enabling ‘Highlight reconstruction’ > Color Propagation can provide a significant improvement.

**Attenuation threshold**: uses an exponential function to reduce highlights likely to cause gamut overshoot.

**The label at the bottom of the page**
Allows you to see the effect of the two checkboxes:
* Subtract black: reduces the value of the linear black point to almost zero by subtracting the indicated value.
* White point: after subtracting the black point, the RGB values ​​are divided by the ‘Linear dynamic range’ (i.e. the displayed white point value minus the old black point).

Reducing the black point to almost zero and taking into account the linear dynamic range as described allows the algorithm to produce better contrast and better consideration of the real gamut).

Activating JDx Matrix’will reduce the ‘Linear dynamic range’ and change the color balance.


**The Primaries and Illuminant functions - possible uses**

When to use this function - whether in 'Abstract Profile' (Color Tab) or 'Selective Editing > Color Appearance (CAM16)' - depending on 
whether you want to affect the entire image or just a part of it - isn't always straightforward. The system for modifying primaries, 
illuminants, or adjusting the dominant color using CIExy components is neither intuitive nor user-friendly. Nevertheless, it's often 
the only way to make in-depth color correction adjustments.

To simplify the explanation, I'll use the module located in Abstract Profiles (Color Tab), which is (slightly) more comprehensive and easier to use.

<figure>
<img src="ap-prim-dom.jpg" title="Primaries - Illuminants - Dominant Color" width="300" />
<figcaption>AP-prim-dom.jpg</figcaption>
</figure>

To modify the primaries or adjust the 'Dominant colors', you must change the selection in the 'Destination Primaries' combobox and choose either:
- Custom (sliders) - usable for Abstract Profile and Selective Editing.
- Custom (CIExy Diagram) - usable for Abstract Profle only.

- Usage of 'Dominant Color' only with Custom (sliders): This adaptation is primarily intended for situations where the illuminant
  is known to be of the 'Daylight' or 'Blackbody' type. Gamut overshoot is often due to incorrect exposure of the highlights,
  resulting in overexposure. In this case, the 'Color Dominant' module is the most appropriate.
-- The gray dot represents the calculated dominant color of the image. The white dot represents the position of the illuminant's white point
  (in the example, Prophoto - D50). Moving 'Refine color' to the right will increase the dominant color, primarily in the form of saturation
  (moving it to the left will decrease it).
-- If you want to change the color of the changes, simply move 'Shift x' and 'Shift y' to the color area you want.
-- The movements of the white and gray dots appear directly on the CIExy diagram.
-- You can also change the illuminant. In the example, you can change D50 to D80. Observe the change in the image's colorimetry
   with all the settings (Refine Colors, Shift x, Shift y set to zero). 
      The white point has moved, and naturally, the effects of Refine Colors (White point), Shift x, and Shift y will be amplified (or reduced depending on the illuminant chosen).

- Usage of 'Primaries': Of course, you can also change the color balance by modifying the primaries. This exercise is not at all intuitive
- and often results in effects opposite to those desired. This feature is best suited for processing images with unusual illuminants
- (LED, Halogen, etc.). You have two ways to modify the primaries.
-- Directly modify the values ​​of Rx and Ry for red, Gx and Gy for green, and Bx and By for blue. Moving the red dot closer to the
   white point will increase saturation (and vice versa) if, for example, the red dot is moved along the original white/red point line.
   Moving away from this original white/red point line will change the hue.
-- Modify the primaries using the graphical interface by directly changing the red, green, or blue points on the CIExy diagram.
   To do this, change 'Custom (sliders)' to 'Custom (CIE xy Diagram)' in the menu (only for Abstract Profile and not available for
   Selective Editing). The actions will be the same as above, but the system is more intuitive to use. In this case, you do not
   have access to the 'Dominant colors' setting. If you wish to make further adjustments to 'Dominant colors', you will need to
   return to the Custom (sliders) option.


**GHS, a pre-tone-mapper and a game-changer for processing difficult images?**

- A fundamental question is whether GHS is a tone-mapper, and why I placed it at the beginning of the process. This deserves discussion.
  I think we can call it a pre-tone mapper. It combines actions similar to 'Raw Black Points > Dehaze' by allowing for the precise
  positioning of the Black point (BP linear) – thus increasing apparent contrast and, in most cases, resolving the problem of hazy images.
  For the White point (WP linear), can be considered a better use of Raw White Points,  its action is close but not the same as 'Gamut compression',
  bringing out-of-gamut data (from the working profile) into that profile. GHS settings allow for the recovery of a usable image, either immediately,
  depending on the values ​​of these parameters (BP linear) and (WP linear) and the illuminant. Its position immediately after demosaicing is crucial.
- Generally speaking, without addressing issues related to raw processing (demosaicing, Capture Sharpening, etc.), 
  nor those related to geometry or crops or denoise, I recommend the following steps (indicative) for an efficient workflow:

- Set profile to Neutral.
- Enable Highlight reconstruction > Color Propagation, in order to optimize the White point (WP linear) settings (see below) in GHS. 
  Check the impact of this choice on the (WP linear) setting (see below), disable if there is no impact, to save resources.
- Activate GHS using the 'Auto Black Point & White point', and Automatic Symmetry Point (SP) settings, adjust the automatic settings as needed,
  especially for (WP linear) and (SP). Find a balanced image by adjusting Stretch Factor (D) and Local Intensity (b). You can also use the other
  available settings: Protect Shadows (LP), Protect Highlights (HP), Stretch Regularization & Midtones, and Highlight Attenuation. The goal is to
  obtain an acceptable image for subsequent enhancement; of course, it can be excellent after GHS processing. The general objectives are:
  a) data optimization to fit within the interval [0, 1] (32-bit real format) - with no data out of gamut, the most important; b) a histogram without major defects;
  b) a balance of luminance, contrast, and color (gamut, colorimetry, etc.).
- Then use Abstract Profile, specifically: a) Gamma/Slope to adjust tones; b) Contrast Enhancement to adjust local contrast; 
  c) Primaries & Illuminant if necessary to correct colors (purity, saturation, dominant color), in particular if the illuminants are
  very far from the 'Daylight' or 'BlackBody' references, for example LED or Halogen. Of course, you can also use Selective Editing >
  Color Appearance (CAM16) and Selective Editing > Local contrast & Wavelets > Wavelets, especially if the modifications only concern
  a part of the image, or a second GHS Spot, in 'Normal Spot'.
- You can also use Color Appearance & Lighting (CIECAM) to take into account the physiological aspects of human vision and viewing conditions.

#### Generalized Hyperbolic Stretch & Michaelis-Menten : what type of data to use ?

The question is not trivial and concerns other algorithms such as Sigmoid, or Slope based, etc.

#### What data is taken into account?
The important point to consider is that most of the time we ignore the distribution and nature of the data once we step outside the Working Profile (and in some cases, even within the Working Profile).
+ One point to consider is that a camera, however sophisticated, is not the human eye. 

##### Our eye
Our eye is made up of cones and rods. It is capable of adapting to strong variations in brightness in a few seconds, and its Dynamic Range is around 20 to 25 Ev, but dynamically, meaning that this area 'adapts' to the environment.
+ It has characteristics known and measured since 1931, leading, for example, to LMS representations, or to the concept of CIE XYZ standard observer color-matching functions (Observer 2° - Observer 10°)
+ The colors perceived by our eye depend on 3 factors - which are expressed as matrices of spectral data:
+ The true colors (measured with a spectrograph) of a flower, an object, or skin. They are expressed between 350 nm and 830 nm. For example for a Red Petunia:
```
    const double ColorTemp::ColorRedpetunia_spect[97] = {
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        0.1890, 0.1097,  0.0855,  0.0899,  0.0987,  0.0881,  0.0807,  0.0804,  0.0787,  0.0691,  0.0643,  0.0549,  0.0465,  0.0404,  0.0385,
        0.0302,  0.0244,  0.0195,  0.0165,  0.0159,  0.0123,  0.0129,  0.0108,  0.0111,  0.0114,  0.0126,  0.0126,  0.0134,  0.0162,  0.0170,
        0.0213,  0.0248,  0.0279,  0.0351,  0.0412,  0.0566,  0.0752,  0.0909, 0.1069, 0.1270, 0.1526, 0.1707, 0.1858, 0.1999, 0.2112,
        0.2293, 0.2422, 0.2471, 0.2611, 0.2718, 0.2710, 0.2778, 0.2807, 0.2825, 0.2856, 0.2909, 0.2901, 0.2974, 0.3042, 0.3044, 0.3075,
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0
    };
```
+ The illuminant's value can be a complex calculation, such as daylight or a Solux studio lamp. Here, for example, are the specifications for Solux 4700K.
```
    //spectral data for Solux lamp : near Daylight (for example  "Musée d'Orsay") - 4700K
    const double ColorTemp::Solux4700_spect[97] = {
        0.4590, 0.83, 1.2011, 1.53, 1.8647, 2.15, 2.5338, 3.06, 3.5809, 3.99, 4.4137, 4.82, 5.2228, 5.63, 6.0387, 6.53, 6.9944, 7.55, 8.0266, 8.475, 8.9276, 8.90, 9.7840, 10.20, 10.6390, 11.00, 11.3600, 11.75, 12.1340, 12.36, 12.5880, 12.74, 12.8790,
        13.07, 13.2560, 13.38, 13.5220, 13.41, 13.3070, 13.35, 13.3990, 13.37, 13.3420, 13.39, 13.4220, 13.65, 13.2710, 13.25, 13.2330, 13.12, 13.0110, 12.93, 12.8470, 12.805, 12.7630, 12.66, 12.5760, 12.563, 12.5490,
        12.59, 12.6330, 12.617, 12.6010, 12.616, 12.6310, 12.6275, 12.6240, 12.70, 12.7710, 12.776, 12.7810, 12.786, 12.7950, 12.74, 12.6850, 12.64, 12.5950, 12.55, 12.5420, 12.43, 12.3180, 12.07, 11.8340, 11.72, 11.6190, 11.55, 11.5020,
        11.32, 11.1510, 11.05, 10.9530, 10.80, 10.6550, 10.495, 10.4390, 10.31, 10.1790
    };
```
+ The Observer is the human-eye representation of the CIExy diagram. Note that scientists say it would be better to use Observer 10° rather than Observer 2°, but since equipment (cameras, TVs, etc.) uses Observer 2°, we stick with the latter. This Observer has three components. I'm not giving the spectral values ​​because that would be a lot of data.
+ The color perceived by our eye in XYZ data (It's a simplification, a systemic model, but it works nonetheless), is given by a matrix calculation with the code below (one case among others)
```
    //calculate XYZ from spectrum data (color) and illuminant : J.Desmis December 2011
    void ColorTemp::spectrum_to_color_xyz_preset(const double* spec_color, const double* spec_intens, double &xx, double &yy, double &zz, const color_match_type &color_match)
    {
        int i;
        double lambda, X = 0, Y = 0, Z = 0, Yo = 0;

        for (i = 0, lambda = 350; lambda < 830.1; i++, lambda += 5) {

            double Me;
            double Mc;

            Me = get_spectral_color(lambda, spec_color);
            Mc = get_spectral_color(lambda, spec_intens);
            X += Mc * color_match[i][0] * Me;
            Y += Mc * color_match[i][1] * Me;
            Z += Mc * color_match[i][2] * Me;
        }

        for (i = 0, lambda = 350; lambda < 830.1; i++, lambda += 5) {

            double Ms;

            Ms = get_spectral_color(lambda, spec_intens);
            Yo += color_match[i][1] * Ms;
        }

        xx = X / Yo;
        yy = Y / Yo;
        zz = Z / Yo;
    }
```
+ This is the type of calculation used in White Balance auto (Temperature correlation), with a (somewhat...very) complex algorithm that compares up to 237 colors in the image and 429 spectral data points.

##### The Camera
Cameras have a dynamic range that, depending on their age and shooting conditions, is around 10 to 15 EV. But they are incapable of adapting like our eyes. It's up to the photographer to master these things... if they can.
+ It functions 'like' our eye... and performs calculations
+ Their 'Observer' is different from the human eye and varies depending on the brand and camera body. Furthermore, what happens when you exceed the sensor's limits, either in the shadows or the highlights? The data is 'lost'... This is where the software can try to repair it, particularly with 'Raw Black Point' and 'Highlight Reconstruction'. In both cases, it tries to reconstruct what no longer exists.
+ The performance of the algorithms varies considerably. The most efficient is the one devised by Emil Martinec in 2012. It works in the majority of cases, and allows, for example, for a sunset, it to 'retrieve' data up to 4 times the limit of the Working Profile (in comparison, Inpaint Opposed, which is less resource-intensive, only retrieves 2.5 times).
+ Another point to consider is that the manufacturer or those who have carried out tests provide a matrix to convert the sensor data into usable values. For example, for a Canon EOS77d:
```
        "dcraw_matrix": [ 6969,-512,-968,-4425,12161,2553,-739,1981,5601 ], // DNG v13.2
```
+ A very important point to note is that these values ​​are for a D65 illuminant and a Observer 2°. What happens if you are not in these conditions at the time of shooting? The calculations are incorrect.

##### Algorithms

The key point of all algorithms (GHS, Michaelis-Menten, Sigmoid, etc., and also AgX, not included in Rawtherapee) is: how to take the data into account, how to evaluate it, how to ensure that shadows and highlights are taken into account without disturbing the 'normal' colors.

The problem is not simple – beyond the location of the calculations, which is 'THE' fundamental point, and the processing algorithm, what is taken into account as systemic modeling? Do we use:
+ Each of the 3 channels R, G and B equally, but simultaneously.
+ Each of the three channels (R, G, and B) is processed independently. This leads to the problem of out-of-gamut colors and asymptotes in the very highlights.
+ Try to find a "Luminance" channel, which is actually the one we're interested in for the asymptotic function. As a reminder, the most advanced models to date are Cam16 and JzAzBz which use, albeit in a different way, the 'Brightness' channel (Q for Cam16), (Jz for JzAzBz)
+ In RawTherapee, all three cases are available. For example, the R, G, and B channels are separated in Selective Editing > Color Appearance (CAM16) > RGB channel Slope or TRC based
+ The simplest solution to implement (but not necessarily the most effective) is the first '3 channels R, G and B equally'

In GHS you will find several possible solutions:
+ RGB Standard: The 3 channels R, G and B equally.
+ RGB Luminance : The system is 'guided' by an equivalent luminance (I'll come back to that) to better account for very low light and out-of-gamut light.
+ Lightness and Chromaticity: we use Lab data.. Certainly this is good in theory (I think PixInsight uses it) but how does 'L' behave out of gamut?
+ Luminance HSL: I think, perhaps wrongly, that it's the least good. RGB to HSL conversion has never been designed for data more than 4 times greater than 'normal'.
+ The problem is, without sophisticated equipment - I have nothing, no more than the photographer in the field who isn't going to use sophisticated devices to measure the spectral data of the flowers or the illuminant, so I have nothing to assess what's going on (maybe some research laboratories do).
+ How I did it. There are two formulas that are widely available. The equivalent luminance using the RGB > XYZ conversion matrix in the working profile.
    - Taking the components of the matrix diagonal and applying them to the data... A small problem: this worked as expected in 1931, but not for data outside the Working Profile (here Rec2020).
       ```
       constexpr double xyz_rec2020[3][3] = {
        {0.6734241,  0.1656411,  0.1251286},
        {0.2790177,  0.6753402,  0.0456377},
        { -0.0019300,  0.0299784, 0.7973330}
        };
       ```
    - try to take into account what is outside, for example with:
        ```
        // taken from darktable
        inline float power_norm(float r, float g, float b)
        {
            r = std::abs(r);
            g = std::abs(g);
            b = std::abs(b);

            float r2 = SQR(r);
            float g2 = SQR(g);
            float b2 = SQR(b);
  
            float d = r2 + g2 + b2;
            float n = r * r2 + g * g2 + b * b2;

            return n / std::max(d, 1e-12f);
        }
        ```
        But it works reasonably well (very difficult to verify) for 'reasonable' Gamut overshoots.
    - Trying to solve the squaring of the circle with my latest (finalized) attempts after user feedback
        ```
        inline float norm_3(float r, float g, float b, TMatrix ws, float raplim)//lowers the equivalent luminance if the white point is high
            {
                constexpr float hi = std::numeric_limits<float>::max() / 100.f;
                float pwn = 0.5f;//standard repartition between XYZ luminance and Out of gamut values 
                if (raplim < 1.2f) {//raplim : ratio between the normal value 'reasonable_limit_white_point' and reality
                    pwn = 0.55f;//Tested on images with WP linear close to 4 - Near Sunset
                } else if (raplim < 1.5f) {//Very high White point
                    pwn = 0.75f;//Tested on images with WP linear close to 5 or 6
                } else {
                    pwn = 0.85f;//Tested on images with WP linear close to 6 and above //LEDs
                }    
                return std::min(hi, (1.f - pwn) * power_norm(r, g, b) + pwn * Color::rgbLuminance(r, g, b, ws));//I reversed the action of the two components to better account for what happens out of gamut.
            }
        ```
##### Summary - what are the key takeaways?
The matter is anything but simple. 
The "pre-tone mapper" principle in RawTherapee allows for the separation of processing steps, within the concept of a "game changer." We try to make the data "acceptable" as early as possible in the process, but all data is taken into account. In this sense, "GHS" is superior to "MM," which, without using the "Subtract Linear Black" and "Linear Dynamic Range" functions, would be unable to process certain images where the black point is "misplaced" (these are two features I added to original MM). For these images, this is also the case for Log encoding, Sigmoid,.. which do (very) poorly.

The concept of linear processing is very often misinterpreted; I think it stems from a confusion. A few years ago (but I have no proof, no code...) Adobe used a suffix 'Melissa' for its Raw processing (I think from the developer's first name), which in fact, instead of using the linear 'Working profile', used the same one with the sRGB gamma (Slope=12.92 gamma=2.4).

As for the rest, Rawtherapee's code is entirely 32-bit or 64-bit, and I think (I'm sure) that it makes little to no difference. As soon as anything is done (white balance, input profiles, etc.), the initial data is changed... is it important to 'monitor' the level of change? I don't think so. The important thing is not to lose any data, and to ensure that the black point is as close to zero as possible and that the white point has an asymptotic function. These two values ​​should be calculated as close as possible to their actual use and not assigned arbitrarily.

The debate surrounding middle grey needs to be understood by users. We're not talking about the same thing when analyzing data 'before' processing, or when using it for output. The differences can be enormous. GHS elegantly resolves the issue by referring to the 'Symmetry Point' (the peak of the histogram in linear mode).

Game changer allows you to separate the data retrieval phase from the final processing phase. The two algorithms located at the end of the process allow:
+ With Abstract profile (which is not a gadget...) you can adjust/balance shadows and lights, increase 'local contrast' (even though I don't like this term), act if necessary (and only if necessary) on the Illuminants and primaries, which are delicate to implement.
+ With Color Appearance & Lighting (CIECAM), you take into account the concepts of 'scene' (source) and 'display' (viewing). You consider a significant number of physiological aspects, and above all, you take into account the shooting conditions (finally!), and the conditions under which you view your images (examining them in the dark is not the same as viewing them in a well-lit office...). Furthermore, in the final stage, without having to return to the beginning, you can adjust each R, G, and B channel, much like the process of primary colors or a film simulation.


## General principles and settings

### The RT-spot object

As mentioned previously, the approach used for local adjustments is
similar to the method developed by Nik Software. There are however some
major differences:

- Each RT-spot can be considered as an object with several event fields.
  There are about 640 such fields consisting of cursors, curves,
  combo-boxes, check-boxes, drop-down menus, masks, etc. In Basic mode,
  the number of event fields is limited to 160.
- The fields are organized in groups, which can be activated by the user
  and have variable values depending on the type of event. The groups
  are arranged in user-coherent sets: Color & Light, Shadows/Highlights
  & Tone Equalizer, Vibrance & Warm/Cool, Log Encoding, Color appearance
  (Cam16 & JzCzHz), Dynamic Range & Exposure, Common Color Mask, Soft
  Light & Original Retinex, Blur/Grain & Denoise, Tone Mapping, Dehaze &
  Retinex, Sharpening, Local Contrast & Wavelets, Contrast by Detail
  Levels. There is also a Cam16 & JzCzHz Color Appearance module. See
  [Tutorial Color Appearance & Lighting (CIECAM02/16) and Color Appearance (Cam16 &  JzCzHz)]({{% relref "ciecam02#color-appearance--lighting-ciecam0216-et-color-appearance-cam16--jzczhz---tutorial" %}})

- Each RT-spot creates an additional "layer" similar to the way layers
  are added in bit-map editors. Each new RT-spot is transparent and
  allows the user to see any previous modifications. The Excluding Spot
  allows the user to access the original unmodified image (prior to any
  local adjustments) and can be used to exclude certain areas or
  simulate an inverse mode.
- All RT-spots in the Local Adjustments tab work exclusively in
  L\*a\*b\* mode unlike the other tabs in RawTherapee which can use
  either RGB or L\*a\*b\* after demosaicing.
  - You may ask “Why use L\*a\*b\* when it is supposedly limited to
    7EV”? In fact the 7 EV limitation only applies to 8-bit ICC monitor
    profiles. You may also have heard that L\*a\*b\* causes color shifts
    (blue-purple and red-orange). However this is only true if no
    correction is applied.
  - If L\*a\*b\* is used in 32-bit Real Mode along with RawTherapee’s
    Munsell correction (to correct color shifts), then in the vast
    majority of cases only the brightest highlights in HDR images will
    pose a problem. Tests on HDR images have shown that L\*a\*b\* is
    capable of handling over 15Ev which is well beyond the range of
    ordinary monitors. If an HDR monitor is available then the JzCzHz
    module can be used as a first approach to HDR processing.
  - For more information go to [Tool sequence in the pipeline - General colorimetry](toolchain_pipeline)
- RT-spot objects are managed in a "for" loop (creation, modification,
  follow-up).
- There is no code duplication. For example the Denoise tool in
  "Blur/Grain & Denoise", uses the functions of the Noise Reduction
  module in the main-menu Detail tab adapted to increase the number of
  wavelet decompositions. The same applies to the Retinex functions etc.
  Some modules do however have different code e.g. Color & Light.
- Many of the tools in Local Adjustments are similar to those in the
  other main-menu tabs but with additional functions, such as:
  - Tone Equalizer or Tone Response Curve (TRC) with Shadows/Highlights,
    which allows fine and differentiated exposure adjustment.
  - Original Retinex, which can be used to simulate a "dodge and burn"
    function.
  - Log Encoding, which is a form of tone-mapping using simple
    logarithmic encoding.
  - Warm/Cool (Vibrance & Warm/Cool) which allows the user to simulate
    color temperature changes similar to White Balance.
  - Blur/Grain (Blur/Grain & Denoise) allows local blur, grain or noise
    to be added to the image.
  - JzCzHz and CAM16 with Sigmoid and other advanced functions such as
    Color Appearance (Cam16 & JzCzHz) to prepare the way for future HDR
    functions.
  - Wavelets with many new tools (tone mapping, decompression, clarity,
    etc.).

### Overview of the RT-spot area

When the user selects an RT-spot, the image on the screen shows:

- A center C consisting of an adjustable circle whose size (diameter)
  and position can be varied using the mouse or cursors.
- Four horizontal and vertical delimiting points T (top), B (bottom), L
  (left), R (right) whose positions can be varied with the mouse or
  cursors.

This gives:

- An area (either an ellipse or rectangle) bounded by 4 delimiting
  points T, B, L, R. The various algorithms (Color & Light, Dynamic
  Range & Exposure, Retinex, Shadow/Highlight & Tone Equalizer, etc.)
  operate within this area. It is also where the color-detection and
  structure-detection algorithms are calculated and applied (except of
  course in Inverse mode).
- The orientation of the RT-spot cannot be varied. This may be possible
  in the future but it would add considerably to the complexity of the
  algorithms and a significant amount of work to adapt the GUI.

It is possible to extend the boundaries of the above areas outside the
preview area. This is done automatically when the user selects
full-image mode. Note that masks (with 1 or 2 Spots) can be used to
compensate for the fact that the RT-Spots cannot be rotated. They also
allow the user to further refine the processing, depending on the
requirements.

### The 3 types of gradient

The RT-spot object is based on 3 types of gradient:

- Dissymetry: a natural gradient inside the area bounded by the RT-spot
  can be created by placing the four points TBLR dissymetrically around
  the center C.
- Transition: this is an adjustable gradient that operates from the
  center C to the periphery of the spot.
- Color (deltaE): the Scope slider adjusts the extent of any applied
  adjustments as a function of deltaE (ΔE). Lower values limit the color
  deviations (L, C, H) that will be taken into account whereas higher
  values (Scope = 80 and above) will allow the tool to act on a wider
  range of values. When the Scope value reaches 100 there is no longer
  any differentiation and all colors will be adjusted equally.

The three types of gradient described can be used in conjunction with
any of the tools (including Contrast By Detail Levels, Retinex, Tone
Mapping, etc.), many of which also have their own graduated filter
function. In all cases the center C of the RT-spot is the reference
point for the start of the gradient.

### The Four Types Of RT-spot

Four types of RT-spot are available:

- Normal: each new spot takes into account any adjustments that may
  previously have been applied by other spots to the same image area.
  The action is recursive, rather like overlapping layers in a bit-map
  editor.
- Excluding: can be used to cancel the action of a Normal spot by
  resetting the selected part of the image to whatever state it was in
  prior to applying any previous spots. It uses the previous adjustment
  data to reverse the effect. It can also be used to invert the behavior
  of certain tools.
  - In principle it is similar to the Capture NX2© “counter point”. It
    allows the user to correct changes that have strayed into unwanted
    areas or to prevent certain parts of the image being affected. For
    example if the user wants to increase the saturation of a portrait
    without affecting the eyes or some other part of the image, which
    has been exposed differently.
  - The algorithm used for the Excluding spot is similar to those used
    elsewhere in Local Adjustments. It is based on differences in ΔE and
    on the image structure (using a Sobel Canny transform). It is not
    perfect, but should satisfy 70% of the cases.
  - How to use an Excluding spot
    - Simply place the new spot on the area to be excluded.
    - Choose “Excluding spot” in the Settings \> “Spot method” menu.
    - Then set Scope, Transition Gradient, "Spot size" and the 4 area
      limiters to obtain the desired effect. If necessary, you can
      further refine the adjustments by adding a tool such as Color &
      Light (as for a normal spot).
    - In some cases (when there is only a small difference in ΔE) it may
      be necessary to reduce the "ΔE scope threshold" slider with
      respect to its default value.
  - How to use an excluding spot to simulate an “inverse” mode
    - Some (but not all) of the tool modules contain an Inverse mode.
      This basically allows you to apply the adjustment outside the
      RT-spot area instead of applying it inside. For example, if you
      use a spot to lighten the area inside the spot and then tick the
      Inverse box, it will lighten everything outside the spot and leave
      the area inside the spot untouched.
    - This effect can also be achieved using an Excluding spot. The
      advantage is that it works with all of the tools and is not
      restricted to those with a built-in Inverse mode.The procedure is
      simple:
      - Create an Rt-spot that extends the TBLR points beyond the limits
        of the image either by using the Full-image option, or by using
        a Normal spot and extending the points manually.
      - Taking the Inverse mode example above, lighten the area inside
        the spot. Because the spot boundaries extend beyond the image
        boundaries, the whole image will be lightened.
      - Now place an Excluding spot over the area where you want to
        cancel out the modifications carried out in the previous step.
        Adjust the Scope if necessary.
      - The result is almost the same as the first example above using
        Inverse mode.There two things to note however when using this
        method:
        - The way the algorithm works for the Excluding spot means that
          the result will not be exactly the same as if the Inverse mode
          in the tool had been used (when available). This is because
          the transition zones are different.
        - The use of the Excluding spot (as opposed to using Inverse
          mode) also means that the area it covers will be reset to the
          original image data and any adjustments carried out in that
          area prior to applying the Excluding spot will be lost.
  - When there are uniform areas in the image, the "Spot structure"
    slider (when available in the particular tool being used) can help
    restrict the effect.
- Full image
  - Allows you to work on the whole image similar to the rest of
    RawTherapee, with the added advantage of being able to use the ΔE
    function for more precise adjustments. If you don't want to use the
    ΔE function then you can effectively disable it by setting the tool
    Scope to 100 and it will behave in the same way as any other of the
    RawTherapee tools in the main menu used to work on the global image.
  - The RT-spot is a rectangle with the delimiters automatically placed
    well beyond the image boundaries. These can be moved around
    depending on the desired effect.
  - The “Transition value” is set to 100. This can also be changed,
    especially if you want to create gradients.
  - The use of ΔE means that the image rendering will depend on the
    position of the center of the RT-spot (in the same way as Normal and
    Excluding spots). The position of the center of the Rt-spot and its
    size determine the basic parameters for calculating ΔE.
  - Please note that some tools may have high memory requirements
    (superior to 8 or even 16 Mb) in full-image mode, especially with
    very large images. Examples include the denoise function in
    Blur/Grain & Denoise, the wavelet function in Local contrast &
    Wavelets or the retinex function in Dehaze & Retinex. The use of
    FFTW may also increase the processing time.
- Global
  - Allows you to work on the whole image similar to the rest of
    RawTherapee (except masks).
  - But it cancels out the notion of deltaE and transition. In this
    mode, the overall behavior is similar to that of Rawtherapee's other
    tools.
  - Some tools, such as the “Graduated filter”, are guided by the
    position of the RT-spot center.
  - When you choose a “Global” RT-spot, all the tools you open will
    operate in this mode.
  - You can open several other RT-spots, in the mode of your choice, to
    adapt the image locally. Either with “Normal spot”, or with
    “Excluding spot” to undo a change made in Global mode, or with “Full
    image”.

<!-- -->

- In Preferences \> General \> Define Spot method for Local Adjustments,
  you can choose the default mode you prefer.

### The three levels of complexity

Each tool (Color & Light, Tone mapping, etc.), has a complexity level
selector with 3 possible options: Basic, Standard, Advanced

- Basic is the default mode, which should be sufficient for the vast
  majority of local editing requirements. It has no masks and few
  curves.
- The Standard mode has been enhanced with additional functions and
  settings including graduated filters, curves and simplified masks.
- Advanced mode contains advanced functions for experienced users
  wishing to have maximum control over the various algorithms. Some
  examples of additional functionality include:
  - A "Merge file" module in Color & Light that allows users to use
    blend modes (Difference, Multiply, Soft Light, Overlay, etc) to
    blend Rt-spots similar to the way layers can be blended in Gimp or
    Photoshop.
  - Masks with additional functions such as a Structure Mask, a Blur
    Mask, etc.

It should be noted that there are effectively only two options for
Dehaze & Retinex, Basic and Advanced (the Basic and Standard options are
identical). Modifying the GUI to reflect this seems to be difficult at
this stage.

### Hue, chroma, luminance references and the principle of the algorithm

The powerful shape detection algorithm is based on the following:

- The area of the central circle C serves as a reference. Depending on
  the diameter chosen by the user, the system calculates the average of
  the hue, chroma and luminance (when using Denoise, the values are
  blurred slightly prior to processing to reduce the impact of noise).
- The choice of the diameter of the central zone depends on the use. For
  foliage for example, the user should choose a low value in order to
  select only the green color of the foliage. Conversely, for skin the
  user should increase the diameter in order to avoid taking into
  account spurious data (noise, eyelashes, etc.).
- The RT-spot area is divided into four quadrants. In each of these
  quadrants (of the ellipse or rectangle) the algorithm proceeds as
  follows:
  - Calculates the ΔE (perceived difference between 2 colors taking into
    account hue, chroma and luminance) between the central zone and the
    current pixel.
  - Attenuates the intensity of the adjustment based on the above ΔE
    value using either a linear or a power law (parabolic, cubic, etc.)
    depending on the setting of "ΔE decay" in Settings (1 = linear law,
    2 = parabolic, etc.).

The extent of this action depends on the setting of the Scope slider.
This allows the user to differentiate the action according to the
criteria mentioned above. For example, if the central circle is placed
on some foliage, the action can be limited to just the foliage contained
in the area covered by the RT-spot, without affecting the background
e.g. the sky (impossible with a lasso).

- The “Transition value” slider allows the user to vary the action
  inside the RT-spot area. If the slider is set to 50, then the full
  adjustment will be applied to half of the area (working outwards from
  the center C). The adjustment is then attenuated progressively until
  the limits of the spot boundary are reached. The fall-off is by
  default linear but the user can modify this to use a non-linear power
  function using the “Transition decay (linear-log)” slider.
- If you increase the value of Scope the whole area contained within the
  RT-spot will be gradually taken into account whatever the hue, chroma
  or luminance.
- If we reduce the value of Scope, the action will be limited to those
  pixels close (in terms of ΔE) to the pixels in the reference area (the
  circle C).
- If the Scope is greater than 80, ΔE will have progressively less
  effect and will cease to be taken into account when Scope = 100. Using
  the Scope slider between 80 and 100 is not generally recommended
  except for specific applications e.g. creating luminance gradients or
  disabling the ΔE detection in full-image mode.

The shape detection algorithm is designed primarily for use in Normal
mode. It can be used in Inverse mode but with some restrictions. For
example in the Color & Light tool:

1.  There is no color correction grid.
2.  There are fewer curves.
3.  “Merge file” is not possible.

#### Recursive updating of references

If you enable the "Recursive references" checkbox under the “Specific
cases” drop-down menu:

- The hue, chroma, luma and Sobel references will be dynamically updated
  for each module used for the same RT-spot, and for each RT-spot.
- The references that appear in the C(C), L(L) and LC(h) masks and h(H)
  will also be update.

#### Preview of selected areas (Preview ΔE)

If you select the Preview ΔE button in Settings, you will get a preview
of the areas of the image that have been affected by the selection (the
effects of any transition gradients are not taken into account). The "ΔE
preview color - intensity" button in Settings allows you to select
either a green or blue preview color and adjust its intensity. Only
luminance adjustments are visible in the ΔE preview. The preview will
not show the effect of any sliders or curves that affect color. Scope is
the most sensitive setting since it acts directly on ΔE. Please note
that the Scope setting located in Settings is only applicable to the
Color & Light, Shadows/Highlights & Tone Equalizer, and Vibrance &
Warm/Cool tools. All other tools have their own Scope slider. The
effects of the 4 sliders located in Settings i.e: “ΔE scope threshold”,
“ΔE decay”, “ab-L balance (ΔE)” and “C-H balance (ΔE)” are also visible
in the preview.

### Summary of the different options in Settings

Settings includes everything that is common to RT-spot management, for
example:

- The transitions will be identical whichever tool is selected (Color &
  Light, Dynamic Range & Exposure, Bur/Grain & Denoise, etc.)
- Anything that is specific to a particular tool is handled in the tool
  module itself. For example the management of ΔE (Scope) for tools
  other than Color & Light, Shadows/Highlights & Tone Equalizer and
  Vibrance & Warm/Cool.

#### RT-spot management

The first set of parameters in the Local Adjustments interface provide
the following options for managing spots:

- Add
- Delete
- Duplicate (creates a new spot with the same shape characteristics as
  the previous spot)
- Rename (allows the user to customize spot names)
- Show/Hide (hide or show the selected spot depending on whether you are
  previewing or editing the image)

#### RT-spot shape

Activate the menu options by ticking the checkbox “Show additional
settings”. The drop-down menu “RT-spot shape” allows you to select
either an ellipse or a rectangle for the Rt-spot shape. Ellipse is the
default mode.

#### Spot method

The “Spot method” drop-down menu allows you to choose between a “Normal
spot” an “Excluding spot” or a “Full-image” spot. The default is a
“Normal spot”. Any of the available tools in the drop-down tool list
“Add tool to current spot” can be used in any of the spots.

#### Spot size

The default size should be suitable in most cases. Smaller values can be
used in conjunction with the Scope slider to select just the leaves of a
tree for example. Larger values will make the calculation of the
reference values (hue, chroma, luma) less sensitive to small variations.
This can be useful when processing skin.

#### Transition Gradient

It can be adjusted with three sliders as follows:

- "Transition value”: allows you to chose the extent of the action
  inside the RT-spot. For a selected value "x", the algorithm is applied
  100% from the center to x% of the selected area. It then decays to 0
  at the periphery of the spot. With this transition we end up with 3
  zones (note that in Inverse mode, these zones are reversed).
  - zone 0: located outside the selection encompassed by the RT-spot
    (rectangle or ellipse). The algorithm has no effect.
  - zone 1: the zone located inside the RT-spot where the action of the
    transition cursors is taken into account. The algorithm is applied
    degressively.
  - zone 2: the zone located close to the center C, where the action of
    the transition cursors is not taken into account. The algorithm is
    applied 100%.

Note: very small values can be used to help reduce defects.

- "Transition decay (linear - log)": adjusts the transition decay as a
  function of distance: 1 = linear, 2 = parabolic, 3 to 10 = cubic (max.
  exponent value =25) for very heavy decay. This makes it possible to
  choose a relatively large Rt-spot to correct certain defects without
  the risk of the spilling over into unwanted parts of the image.
- "Transition differentiation XY": for all values other than zero, the
  slider creates a differential gradient between the abscissa and the
  ordinate. Negative values reduce the transition area on the ordinate
  and positive values increase it.
- "Feather gradient (Grad. Filters)”: this is a common slider which
  works in conjunction with the graduated filters when they have been
  activated in individual tools. The gradient width is a percentage of
  the spot diagonal.

#### Shape detection

- "ΔE decay”: adjusts the fall-off in intensity as a function of ΔE
  using a power function (1 = linear up to 10 for very high decay). The
  higher values are designed to be used for images with a very wide
  gamut.
- "ΔE scope threshold": allows you to fine tune the ΔE values to reduce
  or increase the sensitivity of the Scope slider. Higher values are
  designed to be used with high-gamut images (flowers, artificial
  colors, etc.).
- "ab-L balance (ΔE)": determines the weighting of the three components
  L\* a\* b\* when determining the value of ΔE. A slider value =1 means
  that they have equal weighting. Higher values will increase the weight
  of L\* and lower values will increase the weight of the color
  components.
- "C-H balance (ΔE)": with a slider value = 1, the ΔE calculation
  applies equal weighting to the chromaticity and hue components. Higher
  values will increase the action of H (hue) and vice versa.
- " ΔE preview color - intensity": this allows you to change the color
  of the “Preview ΔE ” function. The default the color is green. Moving
  the slider to the left will change the color to blue. Pushing the
  slider to the extremes will increase the intensity of either the blue
  or green color.

#### Avoid Color Shift

Tries to put the colors back into the working-profile gamut prior to
carrying out a Munsell correction. In most cases, it is recommended to
select "Avoid color shift", in conjunction with "Munsell correction
only". This ensures that the L\*a\*b\* processing will stay neutral in
terms of hue when changing the saturation, and avoids the gamut control
which can cause artifacts in overexposed images.

#### All changes forced in black and white

When the user has used a black and white module or a black and white
film simulation prior to applying any local adjustments, this check box
will ensure that color will not reappear when local adjustment settings
are applied.

#### Recursive references

Forces the algorithm to recalculate the ΔE references in the center C
after each tool has been applied.

#### Shape method

You can choose the way the RT-spot shape is manipulated from among the
following options:

- Independent (mouse) - default setting
- Symmetrical (mouse)
- Independent (mouse + cursors)
- Symmetrical (mouse + cursors)

#### Wavelet Edge performance

An image that has been decomposed into its components by the Daubechies
method can have up to 10 scales of coefficients ranging from D2 (which
corresponds to the Haar decomposition) to D20. In RawTherapee the
coefficients D2 (low), D4 (standard), D6 (standard plus), D10 (medium)
and D14 (high) are used. The more coefficients there are, the more
detail the wavelet decomposition will distinguish, albeit with a slight
increase in processing time (often negligible). Although there is no
direct relationship between the resulting quality and the number of
coefficients (depending on the original image), choosing the right
number of coefficients will help refine the quality of the lower levels
and the residual image:

- In some cases, the best results for edge detection are obtained with
  D2
- In other cases better results can be obtained with D6 or D14

#### Mask and Merge

Used by masks - LC(h) curves etc. - when enabled (Standard and Advanced
modes only).

- Enable “ΔE Image mask” in the check box. For all masks:
  - Takes into account the ΔE of the image to avoid modifying the
    selection area when the following mask tools are used: Gamma, Slope,
    Chroma, Contrast Curve, Local Contrast (by wavelet level), Blur Mask
    and Structure Mask (if enabled).
  - Disabled when Inverse mode is used.
- Scope (ΔE Image mask): allows ΔE to be taken into account when
  creating masks. This setting improves mask selection without affecting
  the various Scope tools. It assumes that one of the mask tools is
  activated (e.g. chroma mask, contrast curve mask, gamma mask, slope
  mask etc.).
- Denoise chroma mask: allows the user to control the chromatic noise of
  the mask. Useful for a better control of the chrominance noise and to
  avoid artefacts, especially when using the LC(h) curve.
- Background color/luma mask: adjusts the shade of gray or color of the
  mask background in "Show mask" (in the "Mask and modifications" menu
  in individual tools).

### Complementary algorithm - Soft radius

This feature can be used to soften the agressive action that can
sometimes occur when using the following algorithms (with the exception
of Basic mode):

- Color & Light
- Dynamic Range & Exposure
- Contrast By Detail Levels

The result is softened by using a guided filter on the differences in
luminance between the original and the modified images.

Note: This algorithm is also an integral part of the Shadows/Highlights
& Tone Equalizer design.

Currently only the luminance component is processed but there is nothing
to prevent future versions taking the color components into account
using either Chroma (C \*) or the 2 components a \* and b \*

### Complementary algorithm - Graduated Filter

Based on the Graduated Filter tool used in the Exposure tab (luminance
gradient), this tool adds two additional gradients:

- Chrominance: allows you to vary the chrominance and produce chroma
  gradients (e.g. in the sky or in portraits).
- Hue: allows you to produce color gradients by varying the hue
  (landscapes, fine adjustments, special effects etc.).

Example using a Graduated Filter to produce variations in luminance,
chrominance and hue: [Example in Getting started - Graduated Filter
based on Luminance Chrominance and
Hue]({{% relref "#making-a-graduated-filter-based-on-luminance-chrominance-and-hue" %}})


The following tools include a graduated filter module:

- Color & Light: luminance, chrominance and hue
- Dynamic Range & Exposure: luminance
- Shadows/Highlights & Tone Equalizer: luminance
- Vibrance & Warm/Cool: luminance, chrominance, hue
- Local Contrast & Wavelets: wavelet-based local contrast only

The feather slider, which controls the gradient fall-off in the RT-spot
area, can be found in Settings \> Transition Gradient.

The gradient can only be rotated using the "Gradient angle" slider at
this stage. There is no mouse assistance.

Applying a gradient does not affect the Transition or Scope settings.

### Complementary algorithm – "Merge file"

Example using the merge-file function (Color & Light): [Example in Getting Started Merge File]({{% relref "#merging-layers-using-blend-modes" %}})
 In Color & light there are 4 options in the "Merge file" drop-down menu:

- None: All of the usual Rawtherapee functions, including masks, operate
  normally.
- Original Image: Blends the current image as shown on the screen with
  the original unmodified image.
- Previous Spot: Blends the current image with the previous spot.
- Background: Allows the user to choose an image background and vary its
  hue, chroma and luminance.

#### Using the Background option to simulate a brush

Choose:

- A small Rt-spot size,
- A very low "Transition value" setting,
- A high "Transition decay" setting,
- Then duplicate the spot.

#### Several types of merge are possible with or without a mask

The scope and transition functions remain unaltered.

Merge modes – similar to blend modes in Photoshop © and the GIMP:

- Normal, Subtract, Difference, Multiply, Addition, Divide, Soft Light
  (legacy), Soft Light Illusion, Soft Light W3C, Hard Light, Overlay,
  Screen, Darken Only, Lighten Only, Exclusion, Hue, Saturation, Color,
  Luminosity.
- The amount of blend can be adjusted with the Opacity slider.
- Merge background (deltaE): takes into account deltaE when blending (a
  sort of equivalent to Scope)

#### "Merge file" example

The following example is an illustration of one of the many possible
applications of "Merge file". We are going to create a variable gradient
on a blurred background.

- Add the first RT-spot set to: "Normal spot", Ellipse.
  - Check the Inverse checkbox.
  - Select the area to be excluded from the blurring function by using
    the 4 spot delimiters.
  - Select: Blur/Grain & Denoise \> Blur & Noise \> "Gaussian Blur -
    Noise – Grain".
  - Set the Radius to a value between 1000 and 10000 (uses a Fourier
    FFTW function).
  - Select Noise if you wish to add some luminance noise.

<!-- -->

- Add a second RT-spot set to: "Normal spot", Ellipse, "Transition
  value" = 60 (you can change these settings later if you wish).
  - Put the center of this new RT-spot inside the first spot (in the
    unblurred area).
  - Using the four delimiters, select the area where you want to vary
    the blur. This will be outside the area of the first spot. Note that
    if you extend the 4 delimiters beyond the image boundaries then you
    will only be able to produce a simple gradient.
  - Activate the Color & Light tool and set to Advanced mode.
  - Set the Scope to a high value (at or near 100).
  - In "Merge file", select Original Image.
  - In "Merge with Original/Previous/Background" select the Normal blend
    mode (you can also experiment with other blend modes such as Soft
    Light if you wish).
    - Set "Merge background (deltaE)" to around 50 depending on the
      desired effect.
    - Set the Opacity to around 50 to adjust the effect.
    - Adjust the Contrast Threshold depending on taste.

<figure>
<img src="doubleblur.jpg" title="doubleblur.jpg" width="300" />
<figcaption>doubleblur.jpg</figcaption>
</figure>

### The RT-spot and layers

Each new RT-spot is made up of two "layers". The first layer contains
the original unmodified image data in case the user wishes to use the
"Merge file" blend modes with the original image or use the spot in
Excluding mode. The second layer contains a copy of the previous image
information including all edits, even if the actual spot size is smaller
than the image itself. This allows the user to apply the blend modes
described above to either the previous image or to the original image.

The merge or blend modes are sensitive to the individual RT-spot
settings and in some cases will only work if the Scope has been set to
100.

### Complementary algorithm - Recovery based on luminance mask

Certain tools such as Color & Light, Shadows/Highlights & Tone
Equalizer, etc., include an algorithm that allows the user to modulate
the effect of the masking curves L(L) or LC(H) in "Mask and
modifications" and is based on the luminance values. The two threshold
sliders, Light and Dark, set the range of luminance values beyond which
the parameters of the current tool will gradually diminish until they no
longer have any effect. Between the two thresholds, the parameters of
the current tool operate normally.

The mask and at least one of the curves must be activated for this
function to work.

### Complementary algorithm - Mask and Modifications

Example using a mask:
[Example in Getting started to improve color selection]({{% relref "#using-a-simple-mask-to-improve-color-selection" %}})
#### Preamble

The mask concepts used in Local Adjustments have been designed by
Jacques Desmis based on the three L, C, H masks used in "Color Toning \>
Color correction regions" in the main-menu Color tab. There are however
some significant differences which may surprise users used to masks in
other software. They also have some limitations due to the current GUI
implementation in Local Adjustments i.e. they cannot be tilted or
modified using Bezier curves or polygons. Some tools do however have
functions generally not available elsewhere (in Advanced mode):

- - Structure Mask, which allows the image structure to be used either
    on its own or in combination with other mask functions. This allows
    the user to reinforce the differentiation between light and dark
    areas.
  - Blur Mask, which varies the contrasts of the mask again allowing the
    user to accentuate the difference between light and dark areas.
  - "Wavelet level selection", which modifies the mask as a function of
    the maximum and minimum wavelet levels.

Masks are only available in Standard or Advanced mode for the tools
concerned.

#### Introduction

Masks are available for the following 11 modules: Color & Light, Dynamic
Range & Exposure, Shadows/Highlights & Tone Equalizer, Vibrance &
Warm/cool, Log encoding, Color Appearance (Cam16 & JzCzHz), Contrast by
Detail Levels, Local Contrast & Wavelets", Tone mapping, Dehaze &
Retinex, Blur/Grain & Denoise (these two modules have a common mask).

- The three tools Color & Light, Dynamic Range & Exposure and
  Shadows/Highlights & Tone Equalizer can be used in Inverse mode (with
  certain restrictions).
- The Blur/Grain and the Denoise tools have a common mask.
- In the case of Tone Mapping and Dehaze & Retinex, the mask can be
  applied either before or after adjustments carried out in the current
  tool are applied.

You can adjust the background luminance of the masks in Settings. The
scale ranges from 0 to 30 and is set to a value of 10 by default.
Reducing the value improves the visibility of the modifications but
reduces the visible detail of the image and vice versa.

#### Mask and modifications

The drop-down menu has several options, which vary depending on the tool
being used.

- Show modified image: shows the modified image including the effect of
  any adjustments and masks.
- Show modifed areas without mask: shows the modifications before any
  masks are applied. Shows the effect of the transition settings.
- Show modified areas with mask: shows the modifications after the mask
  or masks have been applied. Shows the effect of the transition
  settings.
- Show mask: shows the aspect of the mask including any curves or
  filters. It does not show any transitions or the ΔE itself. However,
  if "ΔE Image mask" has been checked in Settings (Mask & Merge), Show
  Mask will show any modifications made to the mask by any of the
  following mask tools: Gamma, Slope, Contrast Curve, Local Contrast (by
  wavelet level), Blur mask and Structure mask. The scope of these tools
  can be fine tuned using the "Scope (Δ Image mask)" slider.
- "Show Spot structure (Advanced) ": allows you to see the effect of the
  "Spot structure" slider when available.
- Preview ΔE: allows you to see the image and the ΔE no matter which
  tool is being used.

#### These masks have two main objectives:

1.  Increase the detection sensitivity to improve object selection and
    avoid modifying unwanted parts of the image. This is useful in the
    rare cases when the ΔE algorithm alone is insufficient.
2.  Create special effects by combining the image used for the mask with
    the original image.

#### Functionality

##### LCH Curves

The curves included in certain tools generate masks which modify the
image before the actual tool adjustments are applied, except in the case
of Tone Mapping and the Retinex function in Dehaze & Retinex, where the
masks can be applied either before or after the tool adjustments. These
masks and any modifications that have been made to them using the mask
tools, take into account the Local Adjustments shape-detection and
transition algorithms etc., irrespective of where they are applied in
the processing pipleline. Of course the overall result will differ
depending on where they are positioned. Curves are available in the
following tools: Color & Light, Dynamic Range & Exposure,
Shadows/Highlights & Tone Equalizer, Vibrance & Warm/Cool, Log Encoding,
Color Appearance (Cam16 & JzCzHz), Contrast by Detail Levels, Tone
Mapping, Blur/Grain & Denoise and Local Contrast & Wavelets. There are
three curves all set to 1 (maximum) at startup:

- C=f(C): chrominance varies as a function of chrominance. The user can
  reduce the chrominance to improve the selection.
- L=f(L): luminance varies as a function of luminance. The user can
  reduce the luminance to improve the selectivity.
- L and C = f(H): the luminance and the chrominance can be varied as a
  function of hue. The user can reduce both the luminosity and the
  chroma to improve the selection.

If the user positions the curves near the zero point on the y-axis, the
effect of the mask will be inverted.

##### Structure Mask

A mask based on the image structure to differentiate low-contrast areas
(uniform areas such as skies etc.) from high-contrast areas (buildings,
hilly terrain etc.). It can be used in two ways:

- Structure Mask ("Structure mask strength » slider) with the checkbox
  "Structure mask as tool" unchecked:

In this case, a mask showing the structure will be generated even if
none of the 3 curves have been activated. Take it easy when you use this
slider!

- Structure Mask ("Structure mask strength" slider) with the checkbox
  "Structure mask as tool" checked:

In this case a mask showing the structure will be generated after having
activated the Mask curves L(L) or LC(h). In this case the structure mask
behaves like the other mask tools such as Gamma, Slope etc. It allows
you to differentiate the action based on the image structure.

- "Structure mask" is available for the Blur/Grain & Denoise and Color &
  Light masks.

##### Blur Mask

The Blur mask, which generates a large-radius blur, allows the user to
vary the image contrast and lighten or darken selected parts of the
image (using Color & Light).

- Contrast threshold: allows you to select the parts of the image that
  will be affected based on the texture.
- Radius: allows you to vary the radius of the Gaussian blur in the
  range 0 to 500.
- FFTW checkbox: uses a Fourier transform to improve the quality
  (increases processing time and memory requirements).

Note that this checkbox is only available in Advanced mode. In other
modes without FFTW, the radius is limited to 100.

##### Smooth Radius et Laplacian Theshold

The simultaneous use of these two sliders is not recommended (the PDE
equation cannot be solved).

###### Smooth Radius

The "Smooth radius" slider uses a guided filter to help reduce artifacts
and transitions.

###### Laplacian Threshold

Allows the user to increase the luminance in the highlights.

##### Gamma, Slope, Chroma, Blend

You can modify the mask (producing the opposite effect on the image)
with the following three sliders:

- Chroma: allows you to adapt the strength of the mask chroma as a
  function of the observed chroma in the image, which depends on several
  factors including the size of the workspace (sRGB, Prophoto, etc.).
- Gamma & Slope: Gamma and Slope are based on the same principle as sRGB
  gamma. The function acts on the Luminance L\* channel with a linear
  part for the dark areas and a parabolic curve thereafter. The curve is
  continuous without any discontinuties.

##### Blend

- Blend allows you to vary the extent to which the mask is blended with
  the current image.

##### Contrast curve, Wavelet local contrast, Hue curve

- All masks have a "Contrast curve" function.
- Two masks (Blur/Grain & Denoise and Color & Light) are equipped with a
  wavelet-based local contrast function with wavelet level selection.
- Color and Light includes an H=f(H) function which allows the user to
  carry out fine color retouching, for example for skin.

##### Applications and use of the luminance and color masks

- Gamma and Slope: allow a smooth and artifact-free transformation of
  the mask by varying the luminance component L progressively avoiding
  discontinuities.
- Contrast curve: can be used similarly to Gamma and Slope, but in this
  case the action can be restricted to certain parts of the mask
  (usually the lightest parts) by using a curve to exclude the darker
  parts.
- Wavelet local contrast: allows you to decrease or increase the action
  on a particular level of detail of the mask as a function of the
  luminosity (usually the brightest areas).
- Hue curve: allows the user to fine tune the colors present in the
  mask.

##### "ΔE Image mask" Checkbox and "Scope (ΔE Image Mask)" in Settings

These two functions take into account the ΔE of the image to avoid
modifying the selection area covered by the RT-spot:

- For each individual mask.
- For sliders and curves which modify the masks after they have been
  created:
  - Sliders: Gamma, Chroma and Slope.
  - Curves: "Contrast curve", "Local contrast" by levels (when present),
    "Hue curve" (when present).
- Lower values of Slope increase the differentiation.
- The function is disabled in Inverse mode.

#### In practice

To achieve the first objective mentioned above, i.e. *increase the
detection sensitivity to improve object selection and avoid modifying
unwanted parts of the image*, it is important to avoid modifying the
zone covered by the circle at the center of the RT-spot. This circle
contains the reference values L, C & H, which can visualised on the
graph itself in the form of a dark gray- light gray boundary. The user
should therefore position the top of the curve on this boundary to avoid
modifying the parameters inside the circle.

- Please note that if the option "Recursive references" has been
  selected (checkbox in Settings \> "Specific cases") this can interfere
  with the position of the light-gray dark-gray boundary on the graph.
  It is therefore recommended to temporarily deactivate this option
  while you are adjusting the mask for the particular tool you happen to
  be working on e.g. if you are adjusting the "Exposure compensation" in
  the Dynamic Range & Exposure tool.

<!-- -->

- You can now adjust the mask to reduce the chroma or luma values
  depending on the required result. Reducing the y-axis value by 10% to
  20% should be sufficient in the majority of cases as shown in the
  example below.

<figure>
<img src="mask_sensi_obj1.jpg" title="mask_sensi_obj1.jpg"
width="600" />
<figcaption>mask_sensi_obj1.jpg</figcaption>
</figure>

- The Blend cursor should be left at zero when the objective is to
  increase detection sensitivity.

As far as the second objective is concerned *(create special effects by
combining the image used for the mask with the original image)*, the
approach depends on what you are trying to achieve. It is recommended
that in the first instance you use the same method as above but with
higher attenuation values (up to 100% if necessary). It should be noted
that the mask in this case will also increase the selection sensitivity
as for the first objective, but in this case the effect will be greater.
The Blend slider is adjusted as follows:

- Negative slider values will subtract a complement of the mask image
  from the original image. In the case of a luminance mask, the image
  will become darker.
- Positive slider values will add a complement of the mask image to the
  original image. In the case of a luminance mask, the image will become
  lighter.

There are several visual aids to help when adjusting the masks:

- a\) Show mask
- b\) Show modifications without the mask (except in Inverse mode).
- c\) Show modifications with the mask (except in Inverse mode).

There is also an option to see the structure mask (except in Inverse
mode). Masks have to be generated individually (it is not possible to
adjust several masks simultaneously), but once generated, they can be
used together.

You can change the visual appearance of the modifications with and
without the mask in Settings \> "Shape detection".

- "ΔE preview color - intensity"
  - Positive values of the cursor do not modify the color appearance.
  - Negative values of the cursor add the L\* a\* b\* "b" component to
    improve the visibility of changes in luminance.

Note that masks can be memory intensive when there is a large selection
area. Note also that contrary to the usual image adjustments, any
adjustments made to a mask will have the inverse effect on the image
itself.

#### Using masks by themselves

When a tool includes masking functions, these can be used without having
to make any adjustments with the tool itself. For example:

- Log Encoding, which is the first tool before Denoise in the
  local-adjustments pipeline.
- Any module upstream of the current module (for example use the mask in
  Contrast by Detail Levels if the current module is Color & Light).
- Using the masks in this way (without using any of the tool sliders,
  curves etc.), allows you to modify the image (e.g. the gamma) at the
  beginning of the local-adjustments pipeline.
- Similarly "Color appearance (Cam16 & JzCzHz)", which is the last
  module in the local-adjustments pipeline, can be used to modify the
  image after all the other local-adjustment modifications have been
  carried out.

The masks in other modules (when equipped) can also be used with the
exception of Retinex.

#### Using several masks

When a tool with masking functions (e.g. Tone mapping, Dynamic Range &
Exposure, etc.) is used in association with a given RT-spot, only one
instance of the tool mask can be used with that particular spot. However
it is simple to use additional masks if required.

Note, the additional mask will take into account the results of the
preceding mask.

##### Using several masks with the same tool

To add a second mask to a given tool, you only have to activate another
tool with masking (e.g. Contrast by Detail Levels), enable the mask and
then adjust it. You don’t have to use the tool itself.

The masks are applied in the same order as the tools in the
local-adjustments pipeline.

- In Advanced mode, you can make adjustments to the masks in Color &
  Light and Blur/Grain & Denoise based on local contrast (using
  wavelets), on the shadows and midtones (Shadows slider in Color &
  Light, Blur/Grain & Denoise) and on the highlights (Highlights slider
  in Blur/Grain & Denoise).
- The Color & Light mask mask has a hue curve (hue = f(hue)).
- The Vibrance & Warm/Cool mask is a special case because there is no
  effect on the luminance when using vibrance (apart from it being used
  for gamut control). The luminance response will therefore be
  negligeable.

These features can be combined with "Merge file" and "ΔE Image mask".

##### Use several masks with the same tool by duplicating the RT-spot

Duplicate the RT-spot and adjust its position and size. This allows you
to:

- Take into account different reference values (hue, chroma, luminance).
- Modify the affected area.
- Change the mask order.

#### Retinex – a special case

The Retinex module is similar to the other tools, however:

- You can only satisfy objective 1 in Normal mode (it doesn’t use the
  Transmission Map).
- If you select "Use transmission map" it does not behave as expected:
  - Instead, the Retinex function will rely heavily on local contrast
    effects. Note that the function behaves the same for both positive
    and negative values of Blend.
  - With "Use transmission map", the light-gray dark-gray boundary
    indicating the reference hue, chroma and luminance values is
    completely erroneous.

### Common Color Mask

Example using a Common Color Mask:
[Example in First Steps, Common Color Mask]({{% relref "#how-to-use-the-common-color-mask-and-an-example-of-how-to-blend-2-rt-spots" %}})

The tools in this mask have the same characteristics as the other masks
mentioned above. However the mask is based on different principles and
the way it functions is different.

- The usual masks add additional functionality to the tool by either
  improving the selection or by changing the image aspect after it has
  been modified by the tool itself (e.g. Color & Light, Vibrance &
  Warm/Cool, etc.).

In the case of the Common Color Mask, the mask behaves as if it were a
tool in its own right:

- The three curves C(C), L(L), LC(H) and in Advanced mode, Structure
  Mask and Blur Mask, will generate color and structure differences with
  respect to the original image.
- This "difference" is of the same type as that generated by the
  Lightness or Chrominance functions in Color & Light.

To make use of this particular mask behaviour, there are three
additional common sliders:

- Scope: allows you to adjust the ΔE references depending on the
  position of the RT-spot and the parameters in Settings (all other
  Scope cursors are deactivated in this case).
- "Add/subtract luma mask": allows you to add or subtract the luminance
  mask from the original image.
- "Add/subtract chroma mask": allows you to add or subtract the
  chrominance mask from the original image.
  - Both of these cursors are inactive when in the zero position.
- Scope acts on the differences generated by the two "add" and
  "subtract" cursors.

### Avoid color shift

Available in Settings \> "Specific cases"

This has two functions:

- Ensures that the colors in the current working profile are within
  gamut using relative colorimetric.
- Adjusts the colors using Munsell correction when the L\*a\*b\*
  saturation has been changed significantly, particularly for red-orange
  and blue-purple.

Difficulties:

- There are no limits when using L\*a\*b\* mode, which means that when a
  color is just within gamut in RGB, it can be transformed into an
  out-of-gamut imaginary color in L\*a\*b\* if the user changes the
  saturation significantly. For this reason it is necessary to control
  the gamut.
- The "Avoid color shift" algorithm does not handle clipped highlights
  very well. For example it will destroy any highlight reconstruction
  carried out in the Exposure tab.

Therefore, for images that have colors close to the gamut limits (e.g.
flowers), that have reconstructed clipped highlights and that have
undergone an increase in saturation, you can either:

- Not use "Avoid color shift", in which case the blue-violet and
  orange-red colors will shift.
- Use "Avoid color shift" as well as "Munsell corrrection only". This
  can generate imaginary colors not visible to the human eye.
- Use "Avoid color shift", which by default will control the gamut and
  apply Munsell correction, then use an Excluding spot on the areas with
  reconstructed highlights that you wish to preserve.

### Fast Fourier Transform

The Fast Fourier Transform in the form "FFTW real DCT" is used in
Rawtherapee and particularly in Local Adjustments in 3 ways:

- To create a Gaussian blur.
- To resolve the Poisson PDE equation after the use of a Laplace
  function.
- To reduce noise.

When processing large images or using Full-Image mode, the use of these
functions can lead to longer processing times.

#### Gaussian Blur

A “Use Fast Fourier Transform” checkbox has been added to two of the
local-adjustments tools. The Fast Fourier Transform (FFT) is used to
generate the blur required for MultiScale Retinex (not used in
full-image mode) and also in the Local Contrast & Wavelets Unsharp Mask.
Note that Multiscale Retinex is invoked using the Scale slider. Higher
values correspond to more iterations and a stronger Retinex effect.
Increasing Scale is resource hungry especially when the blur function is
used, with or without FFTW.

- The Gaussian function is used to generate the blur and is applied
  after the Fourier transform and prior to the inverse transform.
- Gaussian function G(x,y) = (1/2\*PI\*sigma) \* exp(-(x^2 + y^2) / 2\*
  sigma^2).
- The Fourier version is G(x,y) = exp((-sigma^2)\*(PI \* x^2 + PI \*
  y^2))

This function is applicable for all radius values of sigma.

Note that for the blur function, the use of a FFT gives a better quality
result than the approach used elsewhere in RawTherapee, which is based
on a series of approximations. For MultiScale Retinex it is possible
(but not recommended) to change the variable Fftwsigma=true in
RawTherapee’s “options” file to “false”. In this case the FFT algorithm
will be modified in an attempt to emulate the “older” equation without
FFTW.

#### Resolving the Poisson PDE

This concerns the following:

- Original Retinex used to attenuate luminance differences especially
  for portraits.
- Dynamic Range & Exposure with the following two modules: a) PDE Ipol
  Contrast attenuator; b) Fattal PDE - Dynamic Range Compression
  (similar to Dynamic Range Compression used in full-image mode).

#### Noise reduction

In Local Adjustements the FFT is used in conjunction with wavelets to
reduce luminance and chrominance noise (not implemented for the
chrominance component in Detail \> Noise Reduction).

#### FFT optimisation

The processing time for the Fast Fourier Transform (FFT) depends on the
area of the image to be processed and the number of times it is called
in the algorithm (governed for example by the Scale slider in Multiscale
Retinex ). The application of the Gaussian function is almost
instantaneous and independent of the radius. It should be noted that the
FFT is optimised when the dimensions (H, W) of the area to be processed
correspond to a wavelet decomposition using prime factors only: 2^n,
3^p, 5^q, 7^r, 11^a, 13^b (with a + b = 0 or 1).

- The code uses a table to select the appropriate dimensions, which can
  currently extend to a value of L or H = 18144 pixels.
- The result of the optimisation is not visible in the preview.
- The improvement in processing time can range from a factor of 2 to 10.
  It will be 10 if the dimensions of the RT-spot are a multiple of 2^n
  and it will be 2 if there is a large combination of prime factors.
  Because of this the FFTW could in effect take more time for a small
  spot than for a large spot.
- Note that if you select Full Image, the area processed by the FFTW
  will be slightly bigger than the image size (by a few pixels) to
  ensure that the entire image is processed.

#### Calculation precision

The FFTW uses float precision, which after testing seems to be
sufficient. The errors measured on a full image after carrying out a
transform followed by an inverse transform were in the order of 1/1000
and affected only a few isolated pixels. Overall there was no measurable
difference.

If necessary, it should be possible to install the double precision
version of FFTW in GitHub.

## Differences between L\*a\*b\* in Local Adjustments and RawTherapee in general

There are differences in the way some of the L\*a\*b\* functions in
Local Adjustments are implemented compared to the equivalent L\*a\*b\*
functions in the rest of RawTherapee.

### Color & Light

- The luminance and contrast algorithms are different to those used by
  L\*a\*b\* Adjustments in the main-menu, which may lead to some
  differences in rendering.

An example with Color & Light:

[Example in first steps with Color & Light]({{% relref "#adding-the-color-and-light-tool" %}})

<img src="colorspace_flowers.jpg" title="Colorspace_flowers.jpg"
width="300" alt="Colorspace_flowers.jpg" />
<img src="colorspace_flowers-grid2.jpg"
title="Colorspace_flowers-grid2.jpg" width="300"
alt="Colorspace_flowers-grid2.jpg" />

- The *Color correction grid* has two options:

Color Toning and Direct

- Color Toning

1.  In this case luminance is taken into account when varying chroma.
2.  It is equivalent to an H=f(H) function if the black dot only is
    varied on the grid and the white dot stays in the default position
    (note that the two dots are superposed in the default position).
3.  It is equivalent to the Color Toning function in the main-menu Color
    tab if you vary both the black and white dots.

- Direct

1.  In this case the chroma is affected directly.

You can vary the effect using the Strength and the other available
sliders (depending on the level of complexity). In particular, you can
limit the extent of the action using the Scope slider to isolate a
particular color for example.

- The Inverse mode, which now has a Scope function, can be used for
  creating gradients, vignetting, special effects, or simulating a
  graduated border around the image. In the case of a graduated border ,
  if you set Lightness to -100, reduce the Chrominance and select a
  Scope value greater than 75, then the "border" will be black.

Specific settings:

- Gamma (in Advanced mode): Changes the gamma used by L\*a\*b\* from the
  default gamma =3 to linear if you want to work in HDR mode. A reverse
  gamma function is applied at the end of this process.
- "Spot structure": uses the Sobel-Canny algorithm to improve the delatE
  detection by taking into account the differences in structure.
- "Blur shape detection":slightly blurs the result of the delatE
  detection to reduce possible artifacts.

#### Curves

- An L=f(L) curve and a C=f(C) curve allows the luminance or chrominance
  for each particular RT-spot to be modulated according to the luminance
  or chrominance (Standard mode).
- An L=f(H) curve (Advanced mode) allows the luminance for each RT-spot
  to be modulated according to the hue.
- A C=f(H) curve (Advanced mode) allows chrominance modulation for each
  RT-spot as a function of hue.
- An H=f(H) curve (Advanced mode) allows you to modulate the hue for
  each RT-spot according to the hue.
- An L=f(C) curve (Advanced mode) allows the luminance for each RT-spot
  to be modulated according to the chrominance.
- A C=f(L) curve (Advanced mode) allows the chrominance for each RT-spot
  to be modulated according to the luminance.

To activate them, select the Normal option in the "Curve type"
drop-down.

Depending on the level of complexity, there are several additional
features in Color & Light, for example: masking and structure detection.

In Inverse mode the following curves are not implemented; L=f(H),
H=f(H), L=f(C) & C=f(L). There is also no modification preview.

- RGB Tone Curves (Advanced mode)
  - There are 4 options for the RGB curves: Standard, Weighted Standard,
    Luminance & Film-like
  - There is also a "Special use of RGB curves" checkbox. The design of
    Local Adjustments is such that the RGB tone-curve algorithm will
    compare the output to the original, which in certain cases can give
    unexpected results, especially if the curve is inverted to create a
    negative effect. The checkbox allows you to isolate the RGB Tone
    Curve processing by removing all other adjustments carried out using
    the Scope and other sliders, masks etc with the exception of the
    transition settings. Nevertheless, should you wish to further adjust
    the image after having checked the checkbox, you can do so by
    creating another RT-spot in the same position or nearby as the case
    may be.

#### Merge Files

You can merge 2 Rt-Spots or 1 RT-spot with a background similar to the
way you would use blend modes in Photoshop, GIMP etc.

- There are 21 blend modes: Normal, Substract, Addition, Multiply, etc.
- There are 3 sliders to control the action: "Merge background
  (deltaE)", Opacity & "Contrast threshold".

### Dynamic Range & Exposure

Slightly different from the main menu with 3 identical sliders: Amount,
Detail and Anchor, which also work slightly differently. The following
additional functions are also available:

- Gamma (in Advanced mode): Changes the gamma used by L\*a\*b\* from the
  default gamma =3 to linear for working in HDR mode. A reverse gamma
  function is applied at the end of this process.
- "Spot structure": uses the Sobel-Canny algorithm to improve the deltaE
  detection by taking into account the differences in structure.
- "Blur shape detection":slightly blurs the result of the delatE
  detection to reduce possible artifacts.

Dynamic Range & Exposure also has masking functions (limited in Inverse
mode).

#### Partial Differential Equation (PDE) Algorithms

##### Contrast attenuator (Ipol algorithm modified by Jacques Desmis)

- This algorithm carries out contrast attenuation, which is different
  from dynamic range compression. It has 4 sliders:
  - "Laplacian threshold", which performs a convolution ignoring values
    below the threshold.
  - Linearity, which increases the luminance for below-average values.
  - "Laplacian balance", which balances the result by mixing the PDE
    result with a reference standard (1 = 100% PDE).
  - Gamma, which modifies the luminance distribution before and after
    the Laplacian function.
  - The drop-down menu allows you to choose whether or not to denoise
    before the Laplacian function is applied. Because the denoising
    option in this menu ‘median) is rather destructive, it is preferable
    to use the Denoise module (Blur/Gain & Denoise), which by default
    will be applied prior to the Laplacian. If you wish to apply it
    after the Lapalcian, you can do this by creating an additional
    RT-spot.

PDE solves the Poisson equation (Laplacian + Fourier) after a Fourier
transform.

#### Exposure (part of Dynamic Range & Exposure)

This module is similar to the Exposure module in global RGB mode,
however:

- It works entirely in L\*a\*b\* mode, hence the differences in
  rendering.
- It does not have the Lightness, Saturation and Contrast sliders as
  these functions can be found in Color & Light.
- There is a "Chroma compensation" slider, which is a feature of the
  L\*a\*b\* mode and avoids apparent saturation variations. The default
  setting should be adequate in most cases.
- There is an additional Shadows slider (in Exposure Tools), which uses
  the same algorithm as Shadows/Highlights & Equalizer.
- There is a single Tone curve similar to the L=F(L) curve in Color &
  Light. The rendering of this curve will of course be different from
  the the L=f(L) curve in RGB mode. If you wish, you can activate both
  L=f(L) curves in Color & Light and Exposure.
- The selection can be refined by making small adjustments with the
  "Highlight compression" slider.

The Shadows/Highlights & Tone Equalizer module can be used as an
alternative to the Dynamic Range & Exposure module if the RT-spot is in
a shaded area.

- Note: placing the RT-spot in areas of very low luminance may give
  unexpected results.

Tips:

- Although the exposure compensation algorithm used in RawTherapee’s
  Exposure tab has several shortcomings, users are familiar with it and
  so it has has been included in Local Adjustments, albeit with some
  modifications to improve the behaviour.
- Better alternatives include (there are others in the First Steps
  section):
  - The Tone Equalizer in Shadows/Highlights & Tone Equalizer.
  - The Tone Response Curve (TRC) also in Shadows/Highlights & Tone
    Equalizer (Standard mode). You can use the Slope slider to linearly
    lift the shadows and the Gamma slider to lift the mid-tones and
    highlights.

### Shadows/Highlights & Tone Equalizer

Example using Shadows/Highlights & Tone Equalizer \> Tone Response
Curve(TRC):
[Example in First Steps 2]({{% relref "#six-ways-to-change-the-exposure-and-lift-the-shadows" %}})

The module also includes:

- A Graduated Filter (Standard/Advanced modes) and a "Recovery based on
  luminance mask" function.
- When working on areas with deep shadows, it may be necessary to use
  the demoise function Blur/Grain & Denoise.

#### Shadows/Highlights

- Available in L\*a\*b\* mode only.

#### Tone equalizer

This module was first developed for darktable and adapted to RawTherapee
by Alberto Griggio. It allows you to progressively adjust the tonality
as a function of the exposure values (in EV).

- There are 5 sliders covering the range from the deepest shadows to the
  brightest highlights as well as a Detail slider for fine tuning the
  luminance range e.g. only lift the shadows in the -16 to -18 Ev range.

#### Tone Response Curve (TRC)

This module allows you to adjust the image gamma and slope:

- A TRC has been used rather than simple gamma controls to help reduce
  artifacts and improve the color rendering.
- The default settings with gamma = 2.4 and slope = 12.92 (sRGB)
  correspond to the default RawTherapee output settings (screen or
  sRGB). Note that all internal processing in RawTherapee uses gamma =1.
- Other settings can give a similar overall result but with different
  effects in the shadows and highlights.
  - BT709: gamma = 2.22, slope = 4.5
  - L\*a\*b\*: gamma = 3.0, slope = 9.02
- This module allows you to adjust:
  - Mid-tones and highlights using the Gamma slider (you can use high
    gamma values if necessary).
  - Shadows using Slope (you can use high slope values if necessary).

### Vibrance & Warm/Cool

Overview:

- Masking in both Standard and Advanced modes.
- Graduated Filter
  - Luminance filter in Standard mode.
  - Luminance, Chroma & Hue in Advanced mode.
- Recovery based on luminance mask

#### Vibrance

Similar to the Vibrance module in the main-menu Color tab.

#### Warm/Cool

A single slider that allows you to:

- Vary the "warmth" of the selected area.
- Reduce or eliminate certain color artifacts, for example when there
  are multiple illuminants.

Note that this slider is not equivalent to a white balancing slider even
though it may resemble it. The algorithm uses the CAT02 process, which
is apart of CIECAM02 and is probably the best chromatic adaptation
process currently available. When you make the selected area of the
image warmer with respect to the D50 reference, the slider will lower
the temperature of the viewing conditions. If you make the selected area
cooler, it will increase the temperature of the viewing conditions. Note
that you can obtain a similar result for the overall image by using the
CIECAM02 module (Advanced tab) with the following settings:

- Scene conditions: "WP model"==\> "Free temp + tint + Cat02/16 +
  \[output\], Temperature = 5000K, Surround = Average, Adaptation = 100,
  Yb %=18, "Absolute luminance" = 400
- No change to "Image adjustements"
- Viewing Conditions: Adaptation = 100, "Absolute luminance" = 400,
  Surround = Average, Yb%=18 and the Temperature set to the desired
  value.

### Local Contrast & Wavelets

There are two options:

- Unsharp mask: the algorithm is similar to the Unsharp Mask in the main
  menu Detail tab (Sharpening).
- Wavelets (in Standard or Advanced mode): the algorithm is similar to
  that used in Wavelets in the main menu (Advanced tab). However it does
  not include the denoise function, which is in a separate module in
  Local Adjustments. The functionality is similar in both the global and
  local-adjustments versions. Several improvements have been made to the
  latter which of course has the additional deltaE capability.
  - In Standard mode, there is a simplified version of wavelets, which
    includes an algorithm similar to the "Final local contrast"and
    "Contrast curve" algorithms (used in the Residual Image section of
    the global wavelets module). These algorithms can provide a clarity
    function when combined.
  - In Advanced mode there are two drop-down wavelet menus Pyramid 1 &
    Pyramid 2, which include the following functions:
    - Graduated Filter, Edge Sharpness, Blur.
    - Contrast by level, Tone mapping & Directional contrast.

Both modules are equipped with masks and "Recovery based on luminance
mask" in Standard and Advanced modes.

#### Unsharp Mask

The rendering with this tool will be different because of its position
in the pipeline.

The addition of a Use Fast Fourier checkbox allows the use of a Fast
Fourier Transform (FFT) to generate the blur needed for local contrast.
The Gaussian function is used to generate the blur and is applied after
the Fourier transform and prior to the inverse transform.

- A Gaussian blur is applied after the transform and prior to the
  inverse transform.
- The Gaussian function G(x,y) = (1/2\*PI\*sigma) \* exp(-(x^2 + y^2) /
  2\* sigma^2)
- The Fourier version is G(x,y) = exp((-sigma^2)\*(PI \* x^2 + PI \*
  y^2))

This function is applicable for all radius values of sigma.

Note that for the blur function, the use of a FFT gives a better quality
result than the approach used elsewhere in RawTherapee, which is based
on a series of approximations. The processing time for the Fast Fourier
Transform (FFT) depends on the area of the image to be processed and the
number of times it is called in the algorithm, whereas the application
of the Gaussian function is almost instantaneous and independent of the
radius. It should be noted that the FFT is optimised when the dimensions
(H, W) of the area to be processed correspond to a wavelet decomposition
using prime factors only: 2^n, 3^p, 5^q, 7^r, 11^a, 13^b (with a + b = 0
or 1).

#### Wavelets

A simple application of wavelets (without using the pyramid):
[A moment of madness - try Wavelets]({{% relref "#a-moment-of-madness---try-wavelets" %}})

Available controls:

##### Wavelet levels

- A wavelet-level selector with threshold which allows you to select a
  range of levels rather than simply choosing a maximum number of
  levels. The algorithm automatically reduces the number of levels if
  the selected area (in pixels) is insufficient: for level 9 a minimum
  of 1024 pixels is required. For level 8 the minimum = 512 pixels,
  level 7 = 256 pixels, level 6 = 128 pixels, level 5 = 64 pixels, level
  4 = 32 pixels, level 3 = 16 pixels, level 2 = 8 pixels, level 1 = 4
  pixels and level 0 = 2 pixels.

##### Local contrast

- A Local contrast curve acts on the contrast (luminance). Attention we
  do not act directly on the luminance but on the wavelet decompositions
  of the luminance. They translate the local contrast for each level of
  detail, from 2x2 pixels to 1024\*1024 pixels depending on the settings
  and are obtained from variations in the luminance of the undecomposed
  image. For example, uniform areas will result in zero local contrast.
  The levels of detail mentioned above correspond to the levels of
  decomposition from 0 to 9. The higher levels (typically 8 and 9) are
  only possible if the size of the RT-Spot and the Preview are
  sufficient.

<!-- -->

- This curve, along with the level selector, allows you to modify the
  micro contrast as a function of the luminance. You can for example,
  increase the contrast for the midtones and lower the contrast in the
  shadows. If necessary, you can also add another RT-spot to make other
  adjustments.

##### Residual Image

- A slider for adjusting the contrast of the residual image.
- A slider for adjusting the saturation (chroma) of the residual image.
- Four sliders that allow you to adjust the shadows and highlights of
  the residual image, with the possibility of using negative values.
- A Gamma and a Slope slider to adjust the shadows, mid-tones and
  highlights.

##### Clarity & Sharp Mask/Blend & Soften Images (details)

- The level selector allows you to choose between Clarity and Sharp
  Mask. Sharp Mask is used when the selected levels are equal to or less
  than 4 and Clarity is used when the selected levels are 5 and above.
- Merge Luma allows you to select the intensity of the effect on the
  luminance.
- Merge chroma allows you to select the intensity of the effect on the
  chroma.
- The checkbox "Merge only with original image", can be used to prevent
  other wavelet-pyramid adjustments being merged with the Clarity and
  Sharp Mask adjustments to avoid unwanted interactions.
- Note: "Merge luma" and "Merge chroma" , take into account all wavelet
  adjustments, which can be limited to just Clarity if required (see
  above).
- "Soft radius"
  - A "Soft radius" slider (using a guided filter algorithm) allows you
    to reduce halos and irregularities for all wavelet-pyramid
    adjustments including Sharp Mask and Clarity. To deactivate it, set
    the slider to zero.

##### Gamma

The "Gamma (wavelet pyramids)" slider modifies the default L\*a\*b\*
gamma value (= 3) and sets it to gamma = 1 (linear mode) for working
with HDR images. The action is reversed at the end of the process.

##### Wavelet Pyramids

###### Graduated filter & Local Contrast

- Allows you to apply a variable angle gradient to the local
  contrast.The gradient affects the luminance variations and not the
  luminance itself.

###### Edge sharpness

This module has the same purpose and the same adjustments as its
equivalent in the main-menu "Wavelet levels" (Advanced tab) and has a
similar degree of complexity. It targets the local-contrast effect on
edges and has the same controls as its main-menu equivalent.

- [Edge sharpness](wavelet_levels#edge_sharpness_module)

In addition, it benefits from the specific functions in Local
Adjustments such as scope, transitions, multiple selections etc.

###### Blur Levels

- The "Blur residual image" slider allows you to blur the residual
  image. If you try varying the number of wavelet levels, you will see
  that using intermediate values (top right and bottom right settings on
  the threshold graph) can give some interesting results.
- The "Blur by level" curve allows you to blur any individual or range
  of levels. The horizontal axis of the graph represents the
  wavelet-decomposition level. The left-hand side corresponds to the
  finer details ( 2, 4, 8 pixels). The "Maximum blur level" slider
  allows you to set the maximum amount of blur irrespective of the
  "Wavelet levels" settings. The "Chroma levels" slider adjust the
  chroma as a percentage ( plus or minus) of the luminance.

###### Contrast by Level

- The "Contrast by level" graph in Pyramid 2 is the counterpart of
  Contrast By Detail Levels and also of the Contrast module in the
  main-menu "Wavelet levels" (Advanced tab). Once again, the horizontal
  axis represents the wavelet decomposition levels and the vertical axis
  the amount the contrast is increased or decreased.
- The "Attenuation response" slider attenuates the effect of the
  contrast adjustments made using the "Contrast by level" graph. The
  adjustment will be stronger for medium-contrast details and weaker for
  high and low contrast details. The slider controls how quickly the
  effect dampens towards the extreme contrasts. The higher the value of
  the slider, the wider the range of contrast values that will receive
  the full effect of the adjustment and the higher the risk of
  generating artifacts. Lower values will pinpoint the adjustment
  towards a narrower range of contrast values.
- The Offset slider moves the mean value towards either the shadows or
  the highlights.
- Chroma levels: acts on the L\*a\*b\* "a" and "b" components as a
  percentage of the luminance settings.

With this slider you can adjust the apparent contrast by reinforcing or
attenuating the details (with or without using the "Contrast by level"
curve). Values less than 1 reduce the effect and values greater than 1
increase it.

###### Directional Contrast

You can use this module for creating a tone-mapping effect. It operates
on the differences in the three directions of the decomposition:
horizontal, vertical and diagonal. The method relies on the difference
between the diagonal and the horizontal/vertical cross section to
operate on the edges. The action of the curve is based on the luminance.

- The "Attenuation response" slider allows you to concentrate the action
  around the mean contrast values by attenuating the effect on the high
  and low contrast values. Moving the slider to the right will increase
  the extend of the effect beyond the mean contrast values.
- Delta balance allows you to target the processing towards the higher
  or lower levels depending on the position of the slider. Moving the
  slider to the left accentuates the lower levels whereas moving it to
  the right reduces the lower levels and accentuates the higher levels.

###### Wavelet level tone mapping

Example to enhance the texture: [Example in Getting started - with Wavelets Tone mapping]({{% relref "#three-ways-of-increasing-texture" %}})

This module uses wavelets only with a compression algorithm that can be
applied to each decomposition level and the residual image. A guided
filter helps attenuate any artifacts (see below).

- "Attenuation response" allows you to concentrate the action around the
  mean contrast values by attenuating the effect on the high and low
  contrast values. Moving the slider to the right will increase the
  extent of the effect beyond the mean contrast values.
- "Balance threshold" allows you to balance the effect and can in
  certain cases be used to lift the shadows (default value =1.4).
- Negative values compress the data and give a tone-mapping effect that
  is different from the usual algorithms (Mantiuk, Fattal etc.). The
  design is such that it will be more sensitive to areas with detail and
  less sensitive to uniform areas in the image.
  - Positive values will reduce the apparent contrast and give an effect
    similar to Original Retinex. It can be an alternative to simulating
    dodge and burn effects with the Original Retinex tool.
  - "Compress residual image" increases or decreases the contrast in the
    residual image.
- To help reduce artifacts, it is recommended to use Clarity & Sharp
  Mask/Blend & Soften Images if necessary by adjusting the value of
  "Soft radius". The default value is 1 but a smaller value should be
  sufficient in the majority of cases.

##### Note

- Don’t forget that you can use Scope to target the action and/or use an
  Excluding spot to cancel out an action in a selected area. In
  particular if you are using "Wavelet level tone mapping" with
  "Compression by level", an Excluding spot is useful to cancel out any
  pronounced shadow effects.

##### Importance of Attenuation Response

Acts on the standard deviation (the distribution is not Gaussian but is
treated as such for the purposes of the calculations). Taking into
account the mean, the standard deviation and the maximum for each level
in almost all the wavelet-pyramid algorithms, allows us to process the
decomposition non-linearly to avoid artifacts. Micro-contrast values
close to the mean are amplified more than the lower and higher values.

### Tone Mapping

Example increasing texture:
[Example in First step  - increasing texture with Tone mapping]({{% relref "#three-ways-of-increasing-texture" %}})

- Masking and "Recovery based on luminance mask" available in Standard
  and Advanced modes.
- Additional Saturation slider in Advanced mode (to compensate for
  occasional shortcomings in the Mantiuk formula).
- The Strength slider has been renamed "Compression strength", to
  reflect its exact function. The slider range is different: Lab
  Adjustments -1 to 2, Local Adjustments -0.5 to 2
- The Gamma range (Advanced mode) has been changed: Lab Adjustments 0.8
  to 1.5, Local Adjustments 0.4 to 4
- "Reweighting iterates" settings are different: Lab adjustments 0 to 9
  / Local adjustments 0 to 3
- Additional "Normalize luminance" checkbox in Advanced mode. When
  activated the image luminance has the same mean and variance as the
  original image.
- Mask

Tone-Mapping associated with Scope can be used to adjust the image
Clarity in a particular area (among other things).

### Soft Light & Original Retinex

Soft Light is the same as the main-menu function.

#### Original Retinex

Example: [Dodge and Burn example in First Steps]({{% relref "#dodging-and-burning" %}})

Tests on the original Retinex algorithm (based on the work carried out
by IPOL) have shown that it has some useful features for local editing.
The algorithm is different from those used elsewhere, including in
Rawtherapee. It tries to translate the way the human eye perceives large
variations in luminance and deep shadows, which a photographic sensor
has difficulty reproducing. For example if we use a flash or strong
light for a portrait, the face can end up with over-accentuated shadows
and highlights, which can be compensated to a certain degree using dodge
and burn techniques.

Dodging and burning is usually carried out using a brush to lighten or
darken the affected areas but with the original Retinex PDE, this is
carried out automatically.

The IPOL code used can be found here:
<https://www.ipol.im/pub/art/2011/lmps_rpe/> It has been modified and
adapted for use in RawTherapee.

The algorithm is complex and consists of several steps:

1.  Image analysis.
2.  Application of a discrete Laplacian transform with a threshold to
    determine the signal strength. Strength values around 70 give an
    internal Laplacian threshold of around 4 (typical for Laplacian
    transforms).
3.  The "Laplacian threshold deltaE" slider allows the threshold to be
    differentiated as a function of deltaE. For values less than the
    threshold, the full effect of the Laplacian transform is applied.
    For values greater than the threshold, a second Laplacian
    (attenuated by 60%) is added to the first. This mechanism differs
    from the Scope function (which reduces the effect globally), by
    allowing the user to separate the foreground from the background.
4.  Application of a two-dimensional Fourier transform (DCT: Discrete
    Cosinus Transform).
5.  Resolution of the Poisson equation (PDE) to "stabilize" the system.
6.  Inverse two-dimensional Fourier transform.
7.  Luminance normalization with respect to the original image (same
    mean and variance).

The "Show Fourier process" menu allows the user to follow the various
steps i.e.

1.  Determination of the Laplacian variable (first step).
2.  Fourier transform (DCT i.e. discrete cosine transform) of the
    Laplacian.
3.  Resolution of the discrete Poisson equation (PDE) using the Fourier
    decomposition. Note the relative similarity of this image (in its
    Fourier form) to that of the Laplacian.
4.  Inverse transform (not visible in the preview but you can see the
    final result).
5.  Normalization of the luminance (in this case an absence of
    luminosity).

Try it for yourself, in particular on a portrait.

### Dehaze & Retinex

Dehaze is similar to Haze Removal in the Detail tab and can be combined
with Retinex for better results.

#### Retinex: Important differences with the main-menu module

Example using it to increase texture:
[Example in First steps - increasing texture with Retinex]({{% relref "#three-ways-of-increasing-texture" %}})
In Local adjustments, Retinex is similar to the main-menu
implementation, however there are some differences.

Retinex requires stringent conditions to work optimally. It is essential
to have the necessary resources to implement the very large Gaussian
blur radii. RawTherapee’s architecture is such that it doesn’t always
meet the requirements needed to have an accurate image preview. As a
result, there will be differences between the preview on screen and the
TIF or JPG output. These differences will be all the more important
when:

- The RT-spot size is small.
- Scale values are high.
- The Radius is significant.

The algorithm in the Advanced tab, which works on the whole image, is
not subject to these restrictions.

Note also that the memory requirements and processing time required for
implementing Retinex in Local Adjustments increases with:

- The spot size.
- Increasing Radius values.
- Increasing Scale values.
- The use of masks.
- The use of FFTW.

Memory requirements can easily reach 6 or 8 Gbytes! For example, using
Retinex with an 8280\*5512 pixel Nikon D850 image and the following
settings (spot beyond the limits of the image, Radius = 500, Scale = 10,
no masks), will result in a memory requirement of at least 9 Gb .

#### Fast Fourier Transform

A Use Fast Fourier Transform checkbox has been added, which allows the
FFT to be used to generate the blur required for Multi Scale Retinex. It
increases the quality when large radius values are used but it also
increases the processing time significantly.

#### Features

The following changes have been made to the user interface (GUI):

- A Depth slider has been added to Dehaze (previously calculated using
  the Retinex parameters).
- A checkbox allowing the user to choose between linear mode
  (appropriate for working on contrast) and logarithm mode (more
  appropriate for reducing haze). It can also generate more marked local
  contrast effects at the expense of an increase in halos.
- A "Transmission map" curve has been added to help reduce artifacts by
  adjusting the internal transmission parameters.
- The "Transmission map" and reconstructed data is now displayed.
- A "Clip restored data (gain)" slider has been added allowing the user
  to adjust the displayed "Transmission map" values in conjunction with
  the Threshold and Offset sliders.
- A "Reduce ΔE artifacts" slider has been added which acts on the data
  just after the "Transmission map" action.
- The default parameters have been changed.
- Dehaze has been separated out (in the GUI and partly in the
  algorithm).
  - In the main-menu implementation, two separate algorithms are used
    (developed separately in the past) but they both have a similar
    purpose i.e. reduce haze.
  - In Local Adjustments they have been included in the same interface
    so that the advantages of each are more easily accessible.

The Retinex function in Local Adjustments acts at the end of the process
unlike the Retinex standard mode in the main menu (Advanced tab \>
Retinex), which is at the beginning of the Raw process. The number of
settings is also different. It also has the following additional
features:

- A dehaze module, Dehaze & Retinex, which uses the same dehaze
  algorithm as the equivalent function in the main menu. The combination
  of these 2 algorithms (Retinex and Dehaze) provides a powerful tool
  for solving atmospheric haze problems. Each algorithm has its
  strengths: Retinex can differentiate between the foreground and
  background and Dehaze is easier to use overall and generally more
  relevant to a wider variety of images.

The Retinex algorithm is only used if the Retinex Strength slider (in
Advanced mode) is greater than 0.2 .

- If you choose Scale = 1, the Retinex algorithm is partially bypassed
  and the process acts similarly to a local contrast function with the
  possibility of using much higher values than usual. Some functions
  have been removed such as masks and tone mapping and others have been
  assigned different slider values (Radius, Variance, Threshold).
- Darkness and Lightness have no effect if the cursor value is set to 1.
  For other values, the final stage of Multiple Scale Retinex invokes an
  algorithm similar to that used for local contrast. The Darkness and
  Lightness sliders, in conjunction with the Strength slider, allow the
  user to adjust the local contrast upstream.
- The other options, Low, Uniform & High (drop-down menu just below
  Retinex Tools) along with Strength, Radius, Threshold & Contrast are
  in principle similar to the main-menu module even if some of the
  settings and resulting effects may differ slightly.
- When the "Normalize luminance" checkbox is activated, the final image
  luminance will have the same mean and variance as the initial image.

There are optional masks (Retinex only) which are essentially similar to
the other masks in Local Adjustments with some particularities. They are
applied either at the beginning or at the end of the Retinex process
depending on the Transmission Map.

For example, to process an image with a lot of haze:

- The first step is to use the dehaze function in the main menu (Detail
  \> Haze Removal). In some images, the result will have little effect
  and the image will remain hazy.
- Select Dehaze & Retinex in Local Adjustments and position the RT-spot
  over the hazy area.
  - Adjust the amount of Dehaze using the Strength, Depth and Saturation
    sliders.
  - Adjust the Radius using a relatively high value (100 to 150). Note
    that in some cases, lower values may be sufficient.
  - Adjust the Variance (contrast) using a relatively low value (less
    than 100). Note that in some cases higher values may be required.
  - Set the Scale value to 3 or more. Higher Scale values allow you to
    use higher Variance values and reduce the likelihood of artifacts.
  - Adjust Strength and Scope until you get the desired effect.

The results are difficult to predict because sensitivity to the
Threshold slider settings will depend on the image. To sum up, this
module addresses two essential uses, each requiring different settings:

- Processsing hazy images.
- Adding local contrast using high radius values, with the possibility
  of simulating a clarity function.

##### Controlling artifacts and halos

Retinex is very efficient, but: a) it is complex, b) it can generate
artifacts especially at luminance transition areas, c) it often produces
halos.

To reduce artifacts and halos, once the basic settings are chosen
(Radius, Variance, Scale, Darkness, "Transmission gain" and Strength):

- Use the "Transmission map" data dashboard.
- Adjust "Clip restored data (gain)", Offset and Threshold to obtain the
  Min and Max restored-data values, which should be close to 0 and 32768
  respectively. Note that other values may be suitable but it’s
  important to respect the the orders of magnitude. Avoid for example
  values such as Min = -25000 and Max = 90000.
- If the artifacts persist, adjust the "Transmission map" curve by
  pulling down the middle part close to the abscissa, which corresponds
  to the mean data value. You can also try changing the Max and Min
  points on the abscissa – for example by lifting the Min value and
  lowering the Max value.
- You can also adjust the gain of the Transmission curve.
- Another possibility is to adjust the "Reduce deltaE artifacts" slider
  by increasing the value from its default position. (you can also try
  adjusting Scope).
- You can move the RT-spot to change the reference values.
- Of course you can also change the initial settings and start all over
  again!
- It may also help to use a mask (in "Mask and modifications") in
  particular on the luminance component. When Retinex is in
  "Transmission map" mode with its 2 curves "Transmission map" and
  "Transmission gain", you can use the mask-adjustment sliders such as
  Blend, "Soft radius", etc. to optimize the contrast, the halo, the
  haze, etc.

It is certainly a little complicated, but the resulting images can be
well worth the effort, especially when working on the local contrast.

### Sharpening

#### RL deconvolution

- RL deconvolution is available. The results are only visible at 100% zoom (1:1) or greater.

#### Capture deconvolution

Capture Deconvolution, which is an adaptation of Capture Sharpening in the Raw tab for use in ‘Selective Editing’, and allows you to use this algorithm either in standalone mode or after denoising to restore the image's vigor. See the Raw section of Capture Sharpening for more details on understanding. The algorithm is essentially the same, but the code had to be broken down into smaller parts, and of course, the data to be processed is of a different nature.
[Capture Sharpening Raw](/capture_sharpening/)

Capture Sharpening's capabilities allow for finer sharpening control by allowing you to soften the corner sharpness to enhance focus on the main subject. 

But be careful, this algorithm, even though very efficient, does not replace other 'Sharpening' tools, nor anything that falls under the term 'Local contrast'.

The results in the TIF/JPG outputs correspond to those in the 'fit to screen' view.

The minimum size of the RT-spot is 150x150.

<img src="capture-decon.jpg" title="Capture deconvolution" width="300"
alt="Capture-decon.jpg" />

#### Associated Tooltip

Capture Deconvolution, which is an adaptation of Capture Sharpening in the Raw tab for use in Selective Editing, and allows you to use this algorithm either in standalone mode or after denoising. 
Capture Sharpening's capabilities allow for finer sharpening control by allowing you to soften the corner sharpness to enhance focus on the main subject. 
The results in the TIF/JPG outputs correspond to those in the 'fit to screen'view.


### Contrast By Detail Levels

- The minimum area is 64x64 pixels but you can effectively reduce this
  with the help of the Transition Gradient settings.
- Use preferably in 1:1 mode (100% zoom).
- There are no sliders for managing skin tones. They are replaced by the
  system used by the RT-spot.
- There is an additional Chroma slider.
- Access to the residual image has been added with the possibility of
  adjusting Clarity and Contrast.

There are masks and a “Recovery based on luminance mask” module in
Standard and Advanced modes.

The Local Adjustments algorithm introduces several improvements:

- The ability to reduce skin defects.
- The ability to increase the impression of perspective and relief in
  coloured areas with fine detail (as in wavelets), with the possibility
  of limiting the extent of the effect (using the Scope slider).
- Removal of sensor defects (coloured or grey spots).

Note: if the selected area is large and includes several objects with
similar hue, chroma, luma and contrast, the algorithm will select them
while leaving the rest of the image unchanged.

### Blur/Grain & Denoise

#### Blur & Noise

- This module contains three options: Gaussian Blur-Noise-Grain, Median
  and Guided Filter. They can be used in the following modes: Luminance
  only, Chrominance only, or Luminance + Chrominance.

##### Gaussian Blur - Noise - Grain

- Radius: a Gaussian filter is applied. Blur is active only if Radius is
  greater than or equal to 1.6. By reducing the default value of Scope
  and adjusting just the Luminance, it is possible to obtain
  differentiated blurs as a function of the hue Radius.
- Noise: Luminance noise is added to the image.

<!-- -->

- Film Grain:
  - Coarseness with 2 settings.
    - Distribution: simulates the ISO number.
    - Gamma: changes the distribution of the effect. The higher the
      value, the more the effect.
  - Strength: sets the intensity.
  - Scale: the default value is set to 100.

##### Median

- You can choose between 3x3, 5X5, 7X7, 9X9 and the number of passes
  from 1 to 4 (these medians are directly derived from those used in
  Denoise).

##### Guided Filter

- You can select “Soft radius”, Strength and Detail, all of which affect
  the impression of strength.

This module can be used (and in particular the Median & Guided filters)
in addition to the Denoise module for difficult images.

### Denoise

Example: [Example in Getting started - Using Denoise module]({{% relref "#using-the-denoise-module" %}})

This module is quite different from the main-menu noise reduction module
for several reasons:

- It is equipped with a non-local means or piecewise denoising module
  that only deals with luminance noise.
- It uses the same basic wavelets functions as in the main menu Advanced
  tab, modified to increase the number of decomposition levels:
  - The number of luminance levels has been changed from 5 (0 to 4) to 7
    (0 to 6) to better differentiate the noise reduction especially in
    uniform areas (requires more resources).
  - The number of chrominance levels has been changed from 6 (0 to 5) to
    7 (0 to 6).
- The following additional functions have been added:
  - The DCT (Discrete Cosine Transform) algorithm has been extended to
    the chrominance component.
  - The ability to combine the wavelet actions with other tools for
    difficult images. For example, the Guided Filter, which will act a
    bit like a bilateral filter, especially for the chrominance part.
- Two drop-down menus that allow the luminance noise to be adjusted
  based on information contained in the mask:
  - Denoise based on luminance mask
  - Recovery based on luminance mask

It can also be used:

1.  To complement the main-menu noise reduction. For example apply light
    noise reduction to the overall image using the Noise Reduction
    module in the main-menu Detail tab and then carry out targeted noise
    reduction using the Local Adjustments Denoise module.
2.  To take advantage of the fact that Local Adjustments are in the
    middle of the pipeline to reduce noise generated in the intermediate
    processing steps (the main-menu noise reduction is carried out at
    the beginning of the processing pipeline). This can be done by
    simply adding a final RT-spot dedicated to denoising.

#### Mode

There are 4 options: Off, Conservative, Agressive, “Non local means
only”.

- “Non-local means only”: uses patch-based denoising only (no wavelets).
- Conservative and Agressive use wavelets.

With the exception of “Non local means only”, you can combine wavelets
(luminance and/or chrominance) with non-local means.

#### Non-local means (patch-based noise reduction)

What is patch-based noise reduction? Unlike the usual noise-reduction
filters that average the values of a group of pixels located around a
target pixel in order to reduce noise, the non-local-means filter
averages all the pixel values contained in the image, weighted according
to their similarity to the target pixel. This approach reduces loss of
detail compared to filters using local means. There are 5 sliders:

- Strength: governs the intensity of the action (at zero there is no
  action).
- Detail recovery: uses a Laplacian to target the action towards uniform
  areas and preserve the structure or detail.
- Gamma: low values preserve detail and structure, high values increase
  the noise reduction.
- “Maximum patch size”: adapts the noise reduction to the size of the
  objects to be denoised.
- “Maximum radius size”: high values increase the noise reduction at the
  expense of processing time.

#### Wavelets

This module has 6 functions and must be used in 1:1 mode (100% zoom):

1.  Carry out noise reduction on a selected area (as a function of color
    for example) and leave the rest of the image untouched.
2.  Reduce the noise in an area where noise has been increased following
    a significant increase in exposure or after lifting the shadows.
3.  Introduce a Gaussian blur on the lower wavelet levels (for levels 0,
    1 & 2) to simulate a bokeh effect.

- The minimum wavelet action area is 128 pixels \* 128 pixels.
- There are fewer curves but more cursors to select the right level of
  wavelet decomposition and refine the result.

##### Luminance

The luminance noise reduction module has:

- A curve which allows you to distribute the action. The y-axis
  corresponds to the strength and the x-axis to the level of wavelet
  decomposition (fine details from 0 to 2, areas with little detail from
  3 onwards).
- Uses a DCT (Discrete Cosine Transform) function to progressively
  recover details. With the sliders set to 0, the DCT action is maximum.
  To recover details, increase the values.
- “Equalizer white-black”: allows you to carry out more or less noise
  reduction in the shadows or highlights.
- Gamma: low values preserve detail and structure, high values increase
  the noise reduction.
- “Denoise hue equalizer”: curve that allows you to adjust the noise
  reduction according to the hue.

##### Chrominance

- You can differentiate the action on the chrominance according to the
  coarseness of the noise: “Fine chroma (Wav)” corresponds to the first
  4 levels and “Coarse chroma (Wav)” corresponds to levels higher than
  4.
- There is also a DCT slider for chrominance noise: “Chroma detail
  recovery”: The default value is 50%. It is disabled when the value =
  100.
  - The algorithm also uses a Fourier transform for the chrominance.
    With the slider at 0, the DCT has minimum effect. Increasing the
    value will progressively restore the details.
- Chroma quality improvement
  - If you move the "Coarse chroma (Wav)" slider to 1, it activates an
    improved algorithm for chrominance noise reduction. It makes 2
    passes:
  - When it is set to "1" it only improves fine noise.
  - When it is set to "2" the coarse algorithm is activated.
- Equalizer Color: Allows you to direct the chroma noise reduction
  towards either the blue-yellow or the red-green color range.

##### Two Expanders

###### Denoise based on luminance mask

Allows you to reduce the image noise as a function of the luminance
information contained in the L(L) or LC(H) mask (“Mask and
modifications”). The L(L) mask or LC(H) mask must be enabled to use this
function.

- “Renforce dark/light areas”: allows you to target the noise reduction
  in light or dark areas.

The two sliders below act in conjunction with "Renforce dark/light
areas":

- “Dark area luma threshold”: if “Renforce dark/light areas” is greater
  than 1, the noise reduction is gradually increased from 0% for the
  dark threshold setting (default set to 12) to 100% for the maximum
  dark value (determined by the mask).
- “Light area luma threshold”: The noise reduction is gradually reduced
  from 100% for the threshold setting (default set to 85) to 0% for the
  maximum white value (determined by the mask).
- In the area between the two thresholds, the denoise settings are not
  affected by the mask.

###### Recovery based on luminance mask

Allows you to target the noise reduction based on the luminance
information of the image contained in the L(L) or LC(H) masks (“Mask and
modifications”). The L(L) mask or LC(H) mask must be enabled to use this
function.

- If the mask is below the “Dark area luma threshold” (default 12), the
  noise reduction will be applied progressively.
- If the mask is above the “Light area luma threshold” (default 85), the
  noise reduction will be applied progressively.
- Between the two, the image settings without denoise will be maintained
  unless you adjust the “Gray area luminance denoise” or “Gray area
  chrominance denoise” sliders.

##### Other controls

- The Scope slider allows you to vary the action as a function of the
  deltaE of the subject and the RT-spot transition gradient. Noise
  reduction will be maximum in those areas where deltaE is fully taken
  into account and will diminish in those areas where the deltaE
  detection is reduced either because of the Scope slider setting or the
  transition gradient settings or both.
- If you adjust either of the "chroma" sliders, a slight increase in
  saturation will be applied.

#### Combined adjustment with Blur & Noise

In certain noise situations it can be useful to blur the background and
isolate the subject or foreground. You can do this using the following
filters:

- Gaussian Blur-Noise-Grain
- Median
- Guided Filter, which will function similarly to a bilateral filter on
  the chrominance component in particular.

#### Bilateral Filter

The ‘Bilateral filter is a replica of the Impulse Noise Reduction filter
in the main-menu Detail tab. It’s primary function is to reduce the salt
& pepper noise often found in black and white images but it can also
filter other types of impulse noise.

### Log Encoding

#### Some links to Log Encoding

Example showing how it can be used:

[Log Encoding]({{% relref "#log-encoding" %}})

[Log Encoding and highlight recovery]({{% relref "#log-encoding-and-highlight-recovery" %}})

[Other examples using Log Encoding]({{% relref "#other-examples-log-encoding" %}})

#### Introduction

This module is derived from the excellent module developped by Alberto
Aggrigio in ART. It allows you to process underexposed images or images
with a high dynamic range. It encodes the data with a logarithmic scale
to ensure intelligent data compression.

The RGB log encoding module in RawTherapee incorporates some of the
features of the CIECAM modules used elsewhere in Local Adjustments and
in the Advanced tab and shares the same vocabulary and presentation. In
particular, it incorporates some of the adjustment parameters of the CAM
16 module. The first step in the process is to determine the Black Ev
and White Ev values so that the dynamic range can be calculated. The
default value is 15 Ev (White Ev = +10, Black Ev = -5 Ev, “Mean
luminance (Scene conditions)” = 10%). This estimation is done upstream
in the processing pipeline with a copy of the image being made just
after the colorimetric conversion to sRGB or Prophoto etc. It is
therefore prior to any RGB adjustments in the main processing pipeline
and prior to any local adjustments.

The Scope function is incorporated and allows the action to be targeted
to certain areas of the image based on deltaE.

#### Relative Exposure Levels

- If you click on the Automatic button, the system will calculate the
  black Ev and white Ev values.
- The algorithm takes into account the size of the RT-spot and if
  necessary will detect any high contrast or dark areas. If you choose a
  full-image RT-spot and set the Scope to 100, you will obtain similar
  results to those obtained with ART.

You can of course adjust the black Ev and white Ev values directly.

The checkbox "Black-Ev and White-Ev for whole image" is activated by
default.

- With "Black-Ev and White-Ev for whole image" deactivated, positioning
  the RT-spot over a light area may give an abnormally high Mean
  Luminance value (in Scene Conditions). In this case:
  - Position the RT-spot over a darker area.
  - Reduce the "Mean luminance" value manually.
- Or activate the checkbox.

#### Scene Conditions

Leave “Auto mean luminance (Yb%)” checked if you want the algorithm to
automatically take into account the gray point before processing. Two
algorithms are used for the automatic calculation. If the first one used
for the original calculation fails (possible in certain circumstances),
a second calculation based on Yb (mean luminance) is used instead. There
are three adjustments available:

- “Mean luminance (Yb%)”: Yb is the relative luminance of the
  background, expressed in % gray. 18% gray corresponds to a background
  luminance of 50% expressed in CIE L. The data is based on the average
  luminance of the image.
- Absolute luminance: corresponds to the luminance in candelas per m2 at
  the time of shooting, automatically calculated from the exif data.
- Surround: changes the tones and colors to take into account the
  conditions of the scene.
  - Average: average (standard) lighting environment.
  - Slightly Dark (Dim): dark environment. The image becomes slightly
    brighter.
  - Very Dark. The image becomes brighter

#### CAM16 Image Adjustments

- Local contrast: acts mainly on high frequencies (Basic mode)
- Contrast J: contrast using relative luminance (Standard mode)
- Contrast threshold (J & Q): adjusts the midtones of the 2 contrasts J
  & Q (Standard mode)
- Saturation: acts mainly on the midtones and highlights (Standard mode)

In “All tools” (Advanced mode)

- Lightness J = lightness - relative luminance
- Brightness Q: clarity - absolute luminance
- Contrast Q: contrast using absolute luminance
- Chroma: the color of a stimulus relative to the brightness of a
  stimulus that appears white under identical conditions.
- Colorfulness: the perceived amount of tint in relation to gray.

#### Viewing Conditions

- Mean Luminance (Yb%): relative luminance of the background, expressed
  in % gray. 18% gray corresponds to a background luminance of 50%
  expressed in CIE L. The data is based on the average luminance of the
  image (at the desired output)
- Absolute luminance: absolute luminance of the output environment
  (default 16cd/m2)
- Chromatic adaptation: chromatic adaptation allows a color to be
  interpreted according to its spatial and temporal environment. Can be
  used when the white balance deviates significantly from the D50
  reference. Adapts the colors to the lighting of the output device.
- Surround: modifies tones and colors to take into account the
  conditions of the output environment.
  - Average: average (standard) light environment.
  - Slightly Dark (Dim): dark environment. The image becomes slightly
    dark.
  - Very Dark: the image becomes darker
  - Extremely Dark: the image becomes very dark

#### Graduated Filter

At the end of the log processing, you can act on the resulting luminance
with a Graduated Filter equipped with 2 sliders: “Gradient strength” and
“Gradient angle”.

### Color appearance (Cam16 & JzCzHz)

Example using Color Appearance and HDR functions:
[HDR-SDR First approach: Log encoding – Cam16 – JzCzHz – Sigmoid](/local_adjustments/#hdr-to-sdr-a-first-approach-log-encoding---cam16---jzczhz---sigmoid)


The Color appearance module (Cam16 & JzCzHz) is both:

- Easier than Color Appearance & Lighting(Ciecam02/16).
  - It does not have CAM02, only Cam16.
  - No Automatic symmetric, or Mixed mode. So no possibility of
    chromatic adaptation at the end of the process.
  - No choice of white-point model and no choice of illuminant.
  - No separate CAT02/16 adaptation settings (Scene and Viewing
    conditions).
  - No temperature and hue settings in Viewing conditions.
- More complete than Color Appearance & Lighting(Ciecam02/16) and
  depending on the level of complexity (Basic, Standard, Advanced):
  - Can take into account HDR PQ (Peak Luminance): i.e. a first approach
    to HDR processing.
  - Incorporates Sigmoid Q & Log Encoding Q function, taking into
    account Black Ev and White Ev.
  - Includes masks.
  - Includes in Advanced mode, an experimental JzCzHz module (for
    improved HDR processing).

Overall the Color Appearance module (Cam16 and JzCzHz) is simpler (at
least for the Cam16 part), is more intuitive and takes into account the
possibilities of Local Adjustments i.e. deltaE, Scope, Transition
Gradients, Excluding spots, etc.

For an overview of Cam16:
[Using_Cam16_and_HDR_features]({{% relref "#using-the-cam16-and-hdr-functions" %}})

For a presentation of JzCzHz:
[Experimental JzCzHz module]({{% relref "#an-experimental-jzczhz-module" %}})

To see the tutorial:
[Tutorial Color Appearance & Lighting (CIECAM02/16) and Color Appearance (Cam16 &  JzCzHz)]({{% relref "CIECAM02#color-appearance--lighting-ciecam0216-et-color-appearance-cam16--jzczhz---tutorial" %}})
It should be noted that the JzCzHz module contains all the tools
necessary to replace Lab:

- Curves: Jz(Jz), Cz(Cz), Cz(Jz), Jz(Hz), Hz(Hz), Cz(Hz).
- “Shadows/Highlights Jz”.
- Wavelets Jz.
  - “Local contrast”.
  - “Clarity & Sharp mask”.
