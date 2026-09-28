---
title: Rawtherapee Processing Challenge
contributors:
  - Jdc
tags:
  - 'Tutorials'
toc: true
---

## Introduction - Who this document is for ?

This Feedback Report (Retour d'Expériences : « Rex » in french) was
produced for the Rawtherapee Processing Challenge, which took place in
March and April 2024. The aim of this challenge was twofold: a) to allow
you to familiarise yourself with Cam16 and the tools that have been
integrated into the Selective Editing (SE) "Color appearance (CAM16 &
JzCzHz)" module. b) to validate the changes before merging into "dev".
This feedback, « Rex », can help you to better understand Rawtherapee,
its architecture and its tools, and to improve the quality of your
images as quickly as possible by benefiting from the experience of
others.

Who this document is for ? The target audience is anyone with an
interest in image processing (I'm excluding the absolute beginner, as he
or she doesn't have the minimum knowledge required to get started):
beginners or experienced users, experts or not, amateur or professional
photographers, developers, algorithm designers, and so on.

## The images

We have chosen 6 images that are already well known on the forums. They
are all 'difficult' in that simple processing is rarely satisfactory.
Brief characteristics of these images, which will have a major impact on
processing:

- IMG_0080.CR2: sunset, underexposed image, digital noise, high dynamic
  range 12 Ev.
- 2010_MONTR_033.NEF: "The blue horse". Shaded and sunlit areas, deep
  whites and blacks, isolated green tree, sky reduced to a small part of
  the image. In the centre the horse, its blue reflection and the
  carriage.
- S7_00463.ARW: Very under-exposed background framing the dance scene,
  artificial lighting, digital noise, high dynamic range 16Ev.
- B.dng: isolated house in the countryside, overall dull appearance,
  lack of contrast (dynamic) and color.
- IMGP2426.DNG: sunset, strong under-exposure to 'keep the sun', high
  Dynamic 14Ev.
- 2024-03-22_14-28-41.75_DSC5286.NEF - Quantum of the seas - blur - low
  contrast.

I have added 2 images outside the "Challenge" because they were cited
for Cam16's use :

- hagalund-test01.DNG - Smooth highlights example,
- Sweep-sRGB_Linear_Half_Zip.tif , for Dynamic Range of 25Ev.

All these images are CC BY-NC-SA 4.0

## How do you approach this problem? The how

The 6 images were processed by people with different experience. I'll
leave it to you to discover - there's no "right solution" - how each of
the testers treated the images. You'll be able to get ‘how and why they
did ?’, and I hope to benefit from their experience to help us improve.

I've attached the 6 images. For each:

- the Raw file ;
- the thumbnail of the image as it appears unprocessed - first ;
- a thumbnail preview of what can be achieved - second;
- all the pp3 files - with a very brief summary of 'how' the tester did
  each one.

In a few cases, you'll see a 'pp3' ending in 'jd'. As part of the 'why',
I have added one or two additional 'Selective Editing' to an existing
treatment (arbitrary choice) for educational purposes.

### Image n°1 IMG_0080.CR2

- Raw file(Common Attribution-share Alike 4.0):
  [1](https://drive.google.com/file/d/1KSXYpFMljPTcYGPFb_yNsgO3I-0-GrTM/view?usp=drive_link)

#### Original image without processing


![](g_0080-thumb0.jpg "g_0080-thumb0.jpg")
<figcaption>G_0080-thumb0.jpg</figcaption>


#### Image with possible processing


![](g_0080-thumb.jpg "g_0080-thumb.jpg")
<figcaption>G_0080-thumb.jpg</figcaption>

#### Some treatments carried out by the testers

##### Tester A

- IMG_0080.CR2-Priort.pp3
  [2](https://drive.google.com/file/d/1NaJUD8I4DISspIkUWL9NpAT6z8dv2anh/view?usp=sharing)
- Treatment Summary: Exposure, Tone-Mapping, Impulse & Noise Reduction,
  Haze Removal, White Balance 7733K, Vibrance, Retinex, Color Appearance
  & Lighting, LA 1 spot Dynamic Range & Exposure, Graduated Filter.

##### Tester B

- IMG_0080.CR2-Nosle.pp3
  [3](https://drive.google.com/file/d/15wnkXYQCsZj7QZG6D69o_DsnYDpR-iAd/view?usp=sharing)

- Treatment Summary: Exposure, Temperature correlation, Selective
  Editing : a) spot 1 – Cam16 & Denoise – b) spot 2 - Denoise

##### Tester C

- IMG_0080.CR2-Hiram.pp3
  [3](https://drive.google.com/file/d/1ZsWTJcVNc8FwBgZFBXQYWXgXQYiF_x0b/view?usp=sharing)
- Treatment Summary: Noise Reduction – Selective Editing : 9 spots –
  Cam16 - Mosaic

##### Tester D

- IMG_0080.CR2-Paul.pp3
  [4](https://drive.google.com/file/d/1If1gHH5s6jSttFLcUskXl-A61R54jrZn/view?usp=sharing)
- Treatment Summary: Selective Editing : 1 Spot Cam16 - Resize

##### Tester E

- IMG_0080.CR2-PD1.pp3
  [5](https://drive.google.com/file/d/1Vvm2tL5C2KNKS2COmcy2CeDUQUBVLo3E/view?usp=sharing)
- Treatment Summary : Exposure, Color Appearance & Lighting

##### Tester F

- IMG_0080.CR2-Asil.pp3
  [6](https://drive.google.com/file/d/1f0EzuriNy7OyaDLBueUz2FifeAHKIbbc/view?usp=sharing)
- Treatment Summary : Exposure, Tone-mapping, Noise Reduction, Selective
  Editing : 1 spot – Cam16 & Contrast by Detail Levels

##### Tester F + jd

- IMG_0080.CR2-Asil-Exclud-jd
  [7](https://drive.google.com/file/d/1Q8HDOJqFLVcQZUBq87kHbkZlmhoe2Ju5/view?usp=sharing)
- Treatment Summary : As above (Asil), then : Selective Editing : 1
  Excluding spot

### Image n°2 – 2010_MONTR_033.NEF

- Raw file(Common Attribution-share Alike 4.0):
  [8](https://drive.google.com/file/d/1rRcFYYihDjW0ZHadA9S0nNyt3vwxG1Yu/view?usp=drive_link)

#### Original image without processing


![](2010_montr_033-thumb.jpg "2010_montr_033-thumb.jpg")

<figcaption>2010_MONTR_033-thumb.jpg</figcaption>


#### Image with possible processing


![](2010_montr_033-thumb1.jpg "2010_montr_033-thumb1.jpg")
<figcaption>2010_MONTR_033-thumb1.jpg</figcaption>


#### Some treatments carried out by the testers

##### Tester A

- 2010_MONTR_033.NEF-Asi.pp3[9](https://drive.google.com/file/d/1CqLFUvbaIYqHqjJmG_NMCYV9POG999kk/view?usp=sharing)
- Treatment Summary: Selective Editing : 1 spot - Cam16

##### Tester B

- 2010_MONTR_033.NEF-PD1.pp3
  \[<https://drive.google.com/file/d/1Au6D3z01y0eISLKkAIOAftv9wg7sVlud/view?usp=drive_link>)
- Treatment Summary:Exposure, Tone Equalizer, Noise Reduction, Color
  Appearance & Lighting,

##### Tester C

- 2010_MONTR_033.NEF-Priort.pp3
  [10](https://drive.google.com/file/d/1woSyy7CBfYl2OJcoLqc850xWSzWJWK7Q/view?usp=sharing)
- Treatment Summary:Exposure, Tone-mapping, Dynamic Range, Sharpening,
  Noise Reduction, Haze Removal, Color Toning, Selective Editing : 1
  spot Cam16

##### Tester D

- 2010_MONTR_033.NEF-Nosle.pp3
  [11](https://drive.google.com/file/d/1G91-dWcAfEZYesjcwDvAJ0YOoh3qswOS/view?usp=sharing)
- Treatment Summary:Exposure, Temperature correlation, Selective Editing
  1 spot : Cam16 and Blur/grain & Denoise

##### Tester E

- 2010_MONTR_033.NEF-Hiram.pp3
  [12](https://drive.google.com/file/d/1UXMkYHnseF5Z_5PY5XNMNNF1Mc78PLM9/view?usp=sharing)
- Treatment Summary:Exposure, Selective Editing – 28 spots Cam16 – 1
  Blur/grain & Denoise

##### Tester B + jd

2010_MONTR_033.NEF-PD1-LAjd.pp3
[13](https://drive.google.com/file/d/1njVQKKj8mhj0gVsnT3_-jBGk2-o-ukK2/view?usp=sharing)

- Treatment Summary: As PD1 then 2 Spots Selective Editing : Exposure,
  Tone Equalizer, Noise Reduction, Color Appearance & Lighting, LA : 2
  spots Cam16, Wavelets

### Image n°3 S7_00463.ARW

- Raw file(Common Attribution-share Alike 4.0):
  [14](https://drive.google.com/file/d/1FztHcLponYicyKe7VOOpp5RpUE4pp5Ax/view?usp=drive_link)

#### Original image without processing


![](s7-00463.jpg "s7-00463.jpg")
<figcaption>S7-00463.jpg</figcaption>


#### Image with possible processing


![](s7_00463-thumb.jpg "s7_00463-thumb.jpg")
<figcaption>S7_00463-thumb.jpg</figcaption>


#### Some treatments carried out by the testers

##### Tester A

- S7_00463.ARW-PD1.pp3[15](https://drive.google.com/file/d/1hI4tVqmChgdOe1uV0DP9Ioyo1g2NgVwv/view?usp=sharing)
- Treatment Summary:Exposure, Tone Equalizer, Sharpening, Color
  Appearance & Lighting

##### Tester B

- S7_00463.ARW-Nosle.pp3
  [16](https://drive.google.com/file/d/1Se2P60iUXCnxiYKpS7pVqx30k70tGlMv/view?usp=sharing)
- Treatment Summary:Defringe, Temperature correlation, Selective Editing
  2 spots – Cam16 and Denoise

##### Tester C

- S7_00463.ARW-Priort.pp3
  [17](https://drive.google.com/file/d/145J6BRhANqYVl1BQfCklkfundkTjpiEs/view?usp=sharing)
- Treatment Summary : Shadows/Highlights, Tone Equalizer, Sharpening,
  Noise Reduction, Defringe, Contrast by Detail Levels, Selective
  Editing 1 spot Cam16, Resize

##### Tester D

- S7_00463.ARW-Asi.pp3
  [18](https://drive.google.com/file/d/1VbLxfRgAszUlBsF0qZyO42NybXQLxIeW/view?usp=sharing)
- Treatment Summary : Selective Editing 1 spot Cam16 – Crop - Resize

##### Tester E

- S7_00463.ARW-Hiram.pp3
  [19](https://drive.google.com/file/d/1UMwXB8eWKGY-LoHSVKWGzHWaMcx_MnR0/view?usp=sharing)
- Treatment Summary : Selective Editing 5 spots Cam16 – Crop – Resize –
  Post Resize Sharpening

##### Jdc

- S7_00463.ARW-jdc1-n.pp3
  [20](https://drive.google.com/file/d/1drlf7vNl0LCGJI6KlAx2GQ1lrSbDFlsq/view?usp=sharing)
- Treatment Summary : Neutral, Defringe, Temperature correlation,
  Selective Editing :
  - First Spot Global Cam16, Scene conditions Surround, Source Data
    Adjustments: Gamma, Slope, Midtones, Highlight Attenuation & Levels
    : Levels, Cam16 image adjustments
  - Same spot - Denoise
  - Second Spot - Wavelet Local contrast and Clarity

###### Processing step by step

I chose this image for educational purposes. This process combines:

- Minimal processing outside “Selective Editing”, based on the non-use
  of Tone-curves (Exposure Tab), on the use of Defringe (Detail Tab),
  and Temperature correlation for White Balance (Color Tab)
  ![](s7-00463-expos.jpg "s7-00463-expos.jpg")

- Using Selective Editing with:
  - First Spot with Global Spot method: Color Appearance , Denoise
    (Blur/Grain & Denoise)
  - Second Spot with Normal Spot method : Wavelets (Local Contrast &
    Wavelets)

###### First Spot Color Appearance and Denoise

- Spot method = Global
- Add tool to current spot : Color Appearance (Cam16 & JzCzHz)
- choose "Basic"
- Scene conditions \> Surround \> Extremely Dark (Cutsheet)

Then:
![](s7-00463-cam16.jpg "s7-00463-cam16.jpg")

Note :

- Source Data Adjustments :
  - Gamma = 6, Slope = 21, Midtones = 30
  - Highlight Attenuation & Levels : Levels - Red and Green Slope =
    1.08, Blue Slope = 1.0

Then :

- Add tool to current spot : Blur/Grain & Denoise \> Denoise

![](s7-00463-denoise.jpg "s7-00463-denoise.jpg")
 Note:

- Mode Conservative
- Non-Local Means: Strength = 2
- Wavelets: Luminance denoise
- Wavelets: Chrominance - Fine Chroma = 30

###### Second Spot Wavelet

![](s7-00463-wav.jpg "s7-00463-wav.jpg")

Note:

- Scope = 81
- Wavelet levels slider
- Local Contrast Curve and Attenuation response
- Clarity & Sharp mask : Merge luma = 20

###### Why this process?

This image must have been taken at some distance from the scene (a
hypothesis I can't confirm), framed wide. The focus was probably on the
part limited to the dancers. Obviously, ignoring the parts that are in
shadow due to under-exposure is an easy solution that probably reflects
the photographer's wishes. But I'm going to pretend that he wanted to
render the overall point of view. In this case, either it would have
been possible to reduce the image to be processed to the bright part
(crops), or voluntarily leave it in the shadows. This is not the choice
I've made, and so I'm trying to recreate the dancers' scene and its
immediate environment as best I can (for educational purposes).

One of the questions to ask is what is the lighting? Is it a system
based on tungsten lamps, or an LED system, or a mixture of the two? I
leave in the ignorance that lighting is a source that responds to the
laws of “blackbody” illuminants, and so to optimize colorimetry I use
“Temperature correlation” for white balance.

Once I've validated this hypothesis, I'm going to ignore the “usual”
processes - based on RT's usage habits. So I'm going to ignore pretty
much everything in “Exposure tab”, disabling “Auto-matched Tone Curve”
and forbidding myself any action on the “Exposure compensation”
components. I could have disabled “Inpaint Opposed”. I try to be as
early as possible in the process and avoid on the one hand what can
increase noise and on the other hand use Ciecam for colorimetry
(contrast, lightness, saturation...) instead of traditional RGB or Lab
tools. I keep "Capture Sharpening" which is located at the start of the
process and takes into account the local contrast.

When you examine the image and the histogram, it's obvious that the
image is under-exposed: the maximum value is around 75 (Lab) and the 3
RGB channels in the lowlights are very under-exposed. What's more, Exif
data is non-existent, so we know nothing about the lens used to take the
picture, or the conditions under which it was taken.

I then make the (arbitrary) choices:

- I assume that the noise is evenly distributed, so the treatment will
  be global. The deep underexposure in the darker parts is likely to
  lead to artifacts when we try to lighten, hence the need for
  differentiated processing combining the use of “Non-local means,
  luminance and chrominance wavelets”. The image is noisy, notably due
  to the soften of shadows. Draperies show little interest in detail,
  hence the use of a combined demosaicing method, such as Amaze+VNG4.
- One of the goals is for the user to have almost all of the tools in
  the same tab, and thus minimize the back and forth. This is why I used
  Selective Editing (2 Spots) with the minimum of tools and the simplest
  complexity mode appropriate.
- I set up a Spot (Selective Editing) in Global mode to both:
  - Soften shadows in a linear way to avoid noise build-up as much as
    possible, as well as taking into account eye/brain perception
    through the use of gamma (TRC - Tone response Curve) and midtone
    adjustments.
  - Smooth the transition in the highlights by using “Highlight
    Attenuation & Levels : Levels” (Tone mapping tool) to control the
    progressivity of highlights and attenuate the blue channel (perhaps
    due to the lighting).
  - Then it is almost necessarily to give contrast and saturation using
    Cam16 Images Adjustments.

<!-- -->

- Then I create a second spot in normal mode to give a little dynamic to
  the central part of the image - which is very dull. For this I use the
  "Wavelet" tool with a "Local contrast" and the "Clarity" tool.
  - "Local contrast" examines the local contrast already present and
    amplifies or reduces it depending on its initial intensity. You can
    obviously act on the curve, but also on the extent of the
    decomposition levels (Wavelet Levels slider) and "Attenuation
    response" to better localize the action and avoid artifacts.
  - Clarity & Sharp mask adjust the contrast of your image targeting
    only midtones.

Understanding the intellectual approach I've implemented should enable
the user to adapt it to his or her needs. This can then lead to the
saving of directly selectable Processing Profiles in RT, which will only
require adaptation in known and repetitive areas.

###### Reusing the configuration (Bundled profiles - pp3) - a necessary adaptation?

As you may have noticed, the treatment of this image is quite specific
to show scenes (dance, music, theater, etc.). Why not reuse the basics
of the pp3 file, adapting them to your needs. You can use Andy Atsbury's
excellent video to show you how
[21](https://www.youtube.com/watch?v=mb96_8BWPyA&t=12s)

Of course, you must adapt the pp3 file to your needs by customizing it.

- Demosaicing method (Raw tab)
- Highlight reconstruction method, Tone curve mode,.. (Exposure tab)
- White Balance method (Color tab)

And of course each tool and settings of the 2 Selective Editing Spot. In
particular, the setting values. Spot 1 : Color Appearance Cam16:

- Scene conditions : Surround
- Source Data Adjustments : gamma, slope, midtones, Highlight
  Attenuation & Levels
- Cam16 Image Adjustments : Local contrast, Contrast, Saturation, etc.

Spot 1: Blur/Grain & Denoise:

- Non-local Means: Luminance
- Wavelets Luminance
- Wavelet Chrominance

Spot 2: Local Contrast & Wavelets

- Spot size and location
- Local contrast curve
- Clarity & Sharp mask

This means you'll have an adaptable tool, where you'll have your
reference points. With 2 Spot in Selective Editing, the back-and-forth
to find the “right” settings will be reduced, making it easier to
understand and act.

### Image n°4 B.DNG

- Raw file(Common Attribution-share Alike 4.0):
  [22](https://drive.google.com/file/d/1dKa8TmFVsFKqN-h5Yn_BBcm9ogdRxLGH/view?usp=sharing)

#### Original image without processing


![](b-0.jpg "b-0.jpg")
<figcaption>B-0.jpg</figcaption>

#### Image with possible processing


![](b-thumb.jpg "b-thumb.jpg")
<figcaption>B-thumb.jpg</figcaption>

#### Some treatments carried out by the testers

##### Tester A

- B.DNG-Hiram.pp3
  [23](https://drive.google.com/file/d/1e3Tg-pLzKcY4fKqVYba9pQyCxGYQIfSH/view?usp=sharing)
- Treatment Summary : Exposure, Graduated Filter, Selective Editing – 7
  spots Cam16, Resize, Post Resize Sharpening

##### Tester B

- B.DNG-PD1.pp3
  [24](https://drive.google.com/file/d/1ooudqwLBA4vzF1ByjyHfHceCUykwy09M/view?usp=sharing)
- Treatment Summary : Exposure, Shadows/Highlights, Graduated Filter,
  Color Appearance & Lighting.

##### Tester C

- B.DNG-Asi.pp3
  [25](https://drive.google.com/file/d/1GTcfpAJBMh0ZVhiyhTjPosut3KF2MDN4/view?usp=sharing)
- Treatment Summary : Defringe, Selective Editing 1 spot Cam16, Resize

##### Tester D

- B.DNG-Priort.pp3
  [26](https://drive.google.com/file/d/1PPrDsJizaYulcHIcNEJb9RwsvXpSrjWn/view?usp=sharing)
- Treatment Summary : Exposure, Tone Mapping, Sharpening, Edges,
  Microcontrast, Noise Reduction, Contrast by Details Levels, Haze
  Removal, Vibrance, Selective Editing 2 spots : Cam16 – Dehaze &
  Retinex, Color & Light, Resize

### Image n°5 IMGP2426.DNG

- Raw file(Common Attribution-share Alike 4.0):
  [27](https://drive.google.com/file/d/145J6BRhANqYVl1BQfCklkfundkTjpiEs/view?usp=sharing)

#### Original image without processing


![](imgp2426-thumb0.jpg "imgp2426-thumb0.jpg")
<figcaption>IMGP2426-thumb0.jpg</figcaption>


#### Image with possible processing

![](imgp2426-thumb.jpg "imgp2426-thumb.jpg")
<figcaption>IMGP2426-thumb.jpg</figcaption>

#### Some treatments carried out by the testers

##### Tester A

- IMGP2426.DNG-Priort.pp3
  [28](https://drive.google.com/file/d/1Wmw9giOOIJfS27kd0hcpgNBlmrqpyDbA/view?usp=sharing)
- Treatment Summary : Exposure, Shadows/Highlights, Tone Equalizer, Tone
  Mapping, Dynamic Range, Sharpening, Local contrast, Noise reduction,
  HSV Equalizer, Color Toning, Selective Editing – 1 spot Log Encoding,
  Resize

##### Tester B

- IMGP2426.DNG-Hiram.pp3
  [29](https://drive.google.com/file/d/1EjnwERaaJC_JHvDbzIilTNEnaeSkLX3V/view?usp=sharing)
- Treatment Summary : Exposure, Selective Editing – 3 spots Cam16,
  Resize, Post Resize Sharpening

##### Tester C

- IMGP2426.DNG-PD1.pp3
  [30](https://drive.google.com/file/d/1FaWE8j6_GCtYH0xlSx7X3Bw6lbE8poGE/view?usp=sharing)
- Treatment Summary : Exposure, Tone Equalizer, Graduated Filter, Noise
  Reduction, Color Appearance & Lighting

##### Tester D

- IMGP2426.DNG-Asi.pp3
  [31](https://drive.google.com/file/d/1j0_ZXnt4BohvAt70CFuam88GQcvyHKDX/view?usp=sharing)
- Treatment Summary : Selective Editing – 1 spot Cam16 : Slope based,
  Viewing : Yb, Chromatic adaptation/Cat16

### Image n°6 2024-03-22_14-28-41.75_DSC5286.NEF

- Raw file(Common Attribution-share Alike 4.0):
  [32](https://drive.google.com/file/d/1w6sn1BQ8FQ7uSTd3quceJcyNU5K_nCCZ/view?usp=sharing)

#### Original image without processing


![](dsc5286.jpg "dsc5286.jpg")
<figcaption>DSC5286.jpg</figcaption>


#### Image with possible processing


![](dsc5286-thumb.jpg "dsc5286-thumb.jpg")
<figcaption>DSC5286-thumb.jpg</figcaption>


#### Some treatments carried out by the testers

##### Tester A

- DSC5286.nef-PD1.pp3
  [33](https://drive.google.com/file/d/16HVp5w3_Oas9qzEj_2KHYSpKClpzsTWK/view?usp=sharing)
- Treatment Summary : Exposure, Tone Equalizer, Tone Mapping, Graduated
  Filter, Local Contrast, Noise Reduction, Haze Removal, Color
  Appearance & Lighting, Wavelets levels, Selective Editing – 1 spot
  Color & Light, Crop

##### Tester B

- DSC5286.nef-Asi.pp3
  [34](https://drive.google.com/file/d/10_Sl0mdxsU38-jG_-z66DkIvEjNbOYCW/view?usp=sharing)
- Treatment Summary : Selective Editing 1 spot – Cam16 with ‘Viewing
  conditions’

##### Tester C

- DSC5286.nef-Priort.pp3
  [35](https://drive.google.com/file/d/1BNS7A4KdqpJqZX0Rieb5_6poO01g2WEX/view?usp=sharing)
- Treatment Summary : Exposure, Spot Removal, Edges, Noise Reduction,
  Haze Removal, WB RGB grey, Vibrance, HSV Equalizer, SElective Editing
  1 Spot – Soft Light & Original Retinex, Tone Mapping, Dehaze &
  Retinex, Local Contrast & Wavelets, Contrast by Details Levels, Resize

##### Jdc

- DSC5286.nef_jdc_n.pp3
  [36](https://drive.google.com/file/d/1pc34f1awD85OYVW-ZvYN48YQkFa5enJY/view?usp=sharing)
- Treatment Summary : Neutral, Haze Removal, Temperature correlation,
  Selective Editing - 5 spots : 1) Cam16 & Wavelets - 2)Color &
  Light - 3) Color & Light and Denoise - 4) Wavelet (contrast by levels)
  for Spots - 5) add Noise for Spots.

###### Processing step by step

I chose this image for educational purposes. This process combines:

- Minimal processing outside “Selective Editing”, based on the non-use
  of Tone-curves (Exposure Tab), on the use of Haze Removal (Detail
  Tab), and Temperature correlation for White Balance (Color Tab)
- Using Selective Editing with:
  - First Spot with Global Spot method: Color Appearance , Local
    contrast & Wavelets (Wavelets)
  - Second Spot with Normal Spot method : Color & Light, to increase the
    saturation of greens
  - Third Spot with Normal Spot method : Color & Light, Blur/Grain &
    Denoise (Denoise), to improve the sky
  - Fourth spot with Normal Spot method : Local Contrast & Wavelet
    (Wavelet - Pyramid2)- Contrast by levels to remove spots in the sky
  - Fifth spot with Normal Spot method : Blur/Grain & Denoise - Blur &
    noise - to add some noise after wavelet action.

###### First Spot with Global Spot method - Color Appearance


![](dsc5286-ca.jpg "dsc5286-ca.jpg")
<figcaption>DSC5286-CA.jpg</figcaption>


Note:

- Source Data Adjustments : Gamma = 5.75, Slope = 25.18, Midtones = -10,
  Highlight Attenuation \> Slope based \> Gray balance (slope) = 0.7
- Cam16 Image Adjustments : Local Contrast = 74, Contrast (Q) = 90,
  Saturation = 50

###### First Spot with Global Spot method - Local Contrast & Wavelets


![](dsc286-wav.jpg "dsc5286-wav.jpg")
<figcaption>DSC5286-wav.jpg</figcaption>

Note:

- Local contrast curve
- Wavelets levels close to the maximum
- Clarity & Sharp mask : merge luma = 40

###### Fourth spot with Normal Spot method - Wavelet - Remove Spots in the sky

In this image "Spot Removal" does not work correctly.Instead I use
“Wavelets” with high values:

- Scope = 100;
- Wavelet levels = maximum
- Pyramid2 - Contrast by levels with :
  - Curve to the minimum
  - Attenuation response = 2.5
  - Offset = 1.66


![](dsc5286-spot.jpg "dsc5286-spot.jpg")
<figcaption>DSC5286-spot.jpg</figcaption>


###### Why this process?

This image is extremely difficult to process. It seems that the image
must have been taken through glass, giving both a hazy appearance and at
the same time spots of dirt disturbing the sky. It is sorely lacking in
contrast and saturation, and seems lost in opacity. One might be tempted
to believe that a treatment combining Dehaze and Spot removal would
bring about an improvement. But this is not the case.

I've deliberately deactivated almost all the “Exposure” tools to focus
on Color Appearance and Wavelets. I keep Haze Removal with low values.

The problem of blotches can of course be solved by croping the image,
which for educational purposes is an easy solution. I'm going to solve
this problem by using high values of “Contrast by levels” Wavelets, then
adding noise to restore the image to its original appearance.

I then make the (arbitrary) choices:

- I set up a Spot (Selective Editing) in Global mode to both:
  - Enhance the perceptual contrast, as well as taking into account
    eye/brain perception through the use of gamma (TRC - Tone response
    Curve), Slope and midtone adjustments.
  - Smooth the transition in the highlights by using “Highlight
    Attenuation & Levels : Slope based”
  - Then it is almost necessarily to give contrast and saturation using
    Cam16 Images Adjustments.

Wavelet is then a possible solution for greatly increasing local
contrast, as well as Clarity.

It also seems necessary to increase the saturation of the foreground
(green grass), as well as that of the sky, with 2 normal spots.

In the end, this image has become watchable, but not satisfying.
Contrast and saturation could be increased, but the overall effect will
be “artificial”.

### Image hagalund-test01.DNG

- Raw file(Common Attribution-share Alike 4.0):
  [37](https://drive.google.com/file/d/1LQPKH0xqTchYLYG9rHXtJazCHGArJy5L/view?usp=sharing)

#### Original image without processing


![](hagalund-0.jpg "hagalund-0.jpg")
<figcaption>hagalund-0.jpg</figcaption>


#### Some treatments carried out by the testers

##### Jdc

- hagalund-test01.dng-jdc0.pp3
  [38](https://drive.google.com/file/d/1z8Wwkei9l93MAHEQClOFqtEI8Oy7hxc7/view?usp=sharing)

<!-- -->

- image with 3 possible processing:
  - Spot 1 Global – Cam16 – Gamma based
  - Spot 2 - Color & Light – Merge File – Screen
  - Spot 3 - Original Retinex

You can combine or not these 3 spots.

###### Why this process and how?

This image is an excellent example of highlights. However, the problem
of highlight reconstruction seems marginal. On the one hand, there's an
excess of luminance, and on the other, the lights in the white part of
the building are rather unsightly.

The treatment, even if most of it concerns the entire image, will focus
on the building. I've added Lockable Color pickers to better visualize
the differences.

1\) Image appearance with basic profile 'Auto-Matched Curve ISO Low and
activation of Color Propagation
![](haga-auto-cp.jpg "haga-auto-cp.jpg")

The partial view of the building shows
exposure values at L=100 a=0 b=0, reflecting excess luminance and an
entirely white background. Other parts of the building retain a
predominantly light blue color, but with significant disparities.
Examination of the histogram shows an excess of blue in the highlights.

I will deal with this problem in two stages:

- Bring Lab values within acceptable limits, with a progressivity that
  is more pleasing to the eye, so that details can be better perceived.
- Attenuate disparities to obtain an ensemble with less pronounced
  transitions

2\) Bring Lab values within acceptable limits
![](haga-cam16-0.jpg "haga-cam16-0.jpg")

I'm going to use several features that are
actually internal functions rather similar to a Tone-mapper.

- The first is to act on “Whites distribution” to modify the link
  between the white point (White Ev) and Mean luminance (Yb) - Scene
  conditions. This first vector will lower the highest highlight values
  a little.
- The second is to act on the “Source Data Adjustments” module upstream
  of Cam16, by modifying
  - Tone response Curve & Midtones : Gamma, Slope, Midtones to change
    the distribution of light and shadow over the entire image
  - Highlight Attenuation & Levels : Slope based, which allows finer
    adjustment of highlight transitions than Ev Based and Gamma based.
- The third is to increase image contrast a little with Cam16 Images
  Adjustments

3\) Attenuate disparities to obtain an ensemble with less pronounced
transitions
![](haga-reti-0.jpg "haga-reti-0.jpg")


For this I'm going to use a tool that allows a
form of “Dodge and Burn”:

- Soft Light & Original Retinex \> Original retinex.
- Action on 'Strength' and 'Laplacian threshold deltaE' to dose
  equalization action.

The user will notice that there is a third Spot that is not activated.
I've added it for educational purposes, to give the user a more complete
view of the tools available to deal with the highlights problem.

- Its action uses a tool familiar to Photoshop and Gimp users. It allows
  you to merge an image with a layer - in the case of Rawtherapee
  Selective Editing, the tool merges the retouched image with the
  original (default choice): Color & Light (advanced) \> Merge file \>
  Original Image
- I chose the 'Screen mode', which softens transitions. You can control
  'Scope', Opacity, Contrast Threshold

In summary, the proposed method can be used in principle for all
Highlight problems:

- You can choose the 'Highlight reconstruction' method, which here is
  set to 'Color Propagation'; the default choice is 'Inpaint Opposed'
- You may or may not activate 'Auto-matched Tone-Curve', and select a
  Curve mode other than 'Film like'
- In 'Scene Conditions':
  - You can adjust 'White and Black distribution' to allow the
    'Tone-mapper' algorithm to operate more in line with image
    realities.
  - You can set 'Surround' to have a strong effect on shadow-highlight
    differentiation.
- In 'Source Data Adjustment':
  - You can adjust gamma, slope and midtones according to the nature of
    the image and your perception.
  - In 'Highlight Attenuation & Levels', you can choose one of the 4
    methods : Ev based, Gamma based, Slope based, Levels.
- In Cam16 Image Adjustments, you can set the usual image parameters
  (lightness, brightness, contrast, saturation, etc.) using a color
  appearance model (Ciecam).
- You can complete the treatment by seeking to reduce the differences,
  either with 'Original Retinex', or with 'Screen mode' (Color & Light)

### Image Sweep-sRGB_Linear_Half_Zip.tif

- TIF file(Common Attribution-share Alike 4.0)
  [39](https://drive.google.com/file/d/1vAzFY7Qh8MdJ882J_4JeO_cnpGELzD8D/view?usp=drive_link)

[An evaluation of the Dynamic-Range capabilities of tools in Selective
Editing](Local_Adjustments#An_evaluation_of_the_dynamic-range_capabilities_of_tools_in_Selective_Editing.md)

## The new features tested in this challenge

- The 'Source Data Adjustments' - (SDA) module with its components: a)
  'Tone Response Curve & Midtones', b) 'Highlight attenuation & Levels'.
  In 'Advanced' mode you have access to: c) the ability to act on
  primaries and illuminants, d) Log encoding. Together, they form a tool
  similar to Abstract Profile, located at the end of the pipeline. This
  SDA module, located 'before' Cam16, improves the processing of images
  with high dynamic range or blocked shadows.
- The 'Ciecam - Cam16' part, which is a simplified version of 'Color
  Appearance & Lighting', is a CAM (Color Appearance Model) that takes
  into account not only the Cartesian aspects of photography, but also
  the physiological aspect (eye-brain relationship).
- You can also try out the JzCzHz experimental module in 'Advanced'
  mode, which is an attempt at an HDR approach.
- And of course, this challenge was (is) for some of you an opportunity
  to get to know Rawtherapee better.

[Color appearance Cam16 and
HDR](Local_Adjustments#Using_the_Cam16_and_HDR_functions.md)

## Why a new module ?: there's Sigmoid and Filmic

Communication is one of the foundations of success. In this respect,
other software (paid or free) does considerably better than Rawtherapee.
They have succeeded in capturing the attention of users, through
tutorials, videos, forums, etc., where the response from contributors
(developers, testers, etc.) is rapid.

The example of one of Rawtherapee's “competitors” is particularly
striking. Based on a totally false assertion (Lab is bad), the whole
thing was rewritten. This doesn't mean that the result of this system
isn't good, but it sows (bad) doubt in the minds of those who haven't
switched. Admittedly, Lab isn't perfect, especially when it comes to
color drift when changing saturation, but RT solves it perfectly with
Munsell corrections.

Reservations about Lab and its limitation to 7 or 8 Ev are unfounded.
Image processing - after RGB\>Lab and then Lab\>RGB conversion - shows
that Lab easily passes 25 Ev, more than enough to process all images.
[An evaluation of the dynamic-range capabilities of tools in Selective
Editing](Local_Adjustments#An_evaluation_of_the_dynamic-range_capabilities_of_tools_in_Selective_Editing.md)

I'm not saying that Filmic (cf Log encoding) or Sigmoid are bad modules,
these are tone-mappers - you'll find them in RT - , but either they use
non-linear conversions that disrupt colorimetry, or they can't both
release deep shadows and soften highlights.

I set out to find a credible alternative that would be as no
color-destructive as possible, while at the same time being able to
play:

- on lowlights in a linear way
- on highlights in a parabolic way (gamma) as our eye does

This first step, based on the TRC (Tone Response Curve) and the virtual
profile, has often been misunderstood (lack of communication), and
yet...easily solves the problem of many images.

Over the years, I've added several additions to it:

- complete the action with a midtones slider.
- soften highlights (highlight attenuation) when they are too centered
  on the limit value, thus losing essential detail.
- improve dynamics processing (DR) without the need for complex or
  destructive functions (Log, Fourier transform, Laplacians, etc.).
- provide access (in advanced mode) to editing of primaries,illuminants
  and dominant colors, using a module that may not be very intuitive,
  but is nonetheless powerful.

Together with “Scene conditions” and "Color Appearance Cam16", this is
part of an overall process that takes maximum account of the linearity
of the data being processed (and to be processed), and of physiological
aspects. We find the name "Scene referred and Display referred" which
here are combined.

In summary, this module does not use logarithmic conversion, sigmoid,
Laplacian, or Fourier transform. This does not mean that it is simple,
it is different, and restores 23 to 24Ev. What RawTherapee lacks is,
above all, good communication. RT seems to some to be software for
engineers, while others seem to be software for photographers. This
image linked to the presence of functionalities which do not exist
elsewhere (or in an embryonic state) is false - you are not obliged to
use the advanced functions of wavelets, ciecam, etc., but maintained by
(bad) communication internal and external.

## Rawtherapee processing and interface

Each software package (Lightroom, Darktable, DxO, Rawtherapee, etc.) has
a different ergonomic approach. This approach is linked to its history,
to the first developers who worked on it and who have different
experiences and cultures, resulting in very different systems. There is
no one right approach. For the user trying to switch from one to the
other, the result is confusion, even rejection, in the face of apparent
complexity. Rawtherapee was created in 2006 by a single person - Gabor
Horvatz - and the architecture of the system has remained the same.

New modules have been added over the last 18 years. By design, these new
modules are not highlighted when the system is opened. As Rawtherapee
has developed, new tools and algorithms have been added, in several
existing "Tabs", or even in new Tabs, which are "behind" them. Let's
take an example: a user wants to lighten the shadows in an image.
Depending on their experience, they will first go to the first 'Tab' and
the first 'Exposure' tools... You really need to be curious and
well-informed to process the image with the Surround function in 'Color
Appearance & Lighting' - Tab advanced.

The testers who took part in this 'challenge' each had their own profile
(experience, culture...) - which evolved over the course of the
challenge, so you learn from yourself and from others. This led to some
very different image processing. There is no "one" good solution, but a
process that matches each person's profile.

## The impact of RT's internal design - the pipeline

Knowing at least the sequence of the main tools enables you to
understand the impact of each tool on the other, and thus to induce or
not induce a change in the way you approach and process an image. This
summary is not intended to replace Rawpedia or the help available on the
forum, or the videos, etc., nor to impose a particular method.

In order - summary of the main methods and tools :

- Demosaicing method, more or less precise, more or less adapted to
  noisy images ;
- Input profile: manufacturer's matrix, ICC profile or DCP;
- White balance: camera - auto – manual;
- Spot Removal ;
- Highlight recovery ;
- Noise reduction ;
- Dehaze ;
- Dynamic Range Compression ;
- Selective Editing: which can be used in local mode or for the entire
  image (Full image or Global). These include tools similar to those in
  the main process, as well as 'new features' (Log encoding, Cam16,
  JzCzHz, Original Retinex, etc.) and masks;
- Auto-match tone curve ;
- Tone response curve and the entire RGB process : Exposure, Shadows,
  Highlight, Black, Color toning, HSV equalizer, etc.;
- Lab process: Shadows/highlight, Lab adjustments, Graduated filter,
  Tone-mapping, etc. ;
- Specific tools for a more complex approach: Wavelets, Abstract
  profile, Color Appearance & Lighting ;
- Crop, Resize ;
- Final output conversion.

An upstream tool will not be influenced by a downstream tool, but the
reverse is true. You will notice that there is no relationship between
the order of operations in the pipeline and the position in the
graphical interface (GUI).

## A possible answer to why? Testers' comments

### Tester 1

One way we can gain that kind of old-hand experience is learning how
photos are developed by hand. When you view the image under the exposer,
you separate out where the lighting zones of the photo exist, and
determining how you are going to affect the exposure to various areas of
the photo using your hands, other small hand tools used for modifying
the light hitting the paper, and various timing methods. These areas are
very much like Selective Editing zones, so looking at a photo in zones
from the outset definitely invites a deeper understanding of how a
Selective Editing spot could help the area in question, and affect its
overall balance within the rest of the photo. These are all editorial
decisions, a bit more than just knowing what every knob in the software
does, because you are intending to affect how the image is perceived by
acting as a director for the viewers eyes. The editorial content starts
with the photographic composition, and follows the image thru exposure
developement. The photo editor can tell a story with an image using all
this know how, and the better they know their skills, tools, and
appreciate the realities of retinal color sensation, visual cortex
perception, cultural expectation, they will attain the desired result
with greater ease and efficiency.

### Tester 2

Frankly, I don’t see myself competent to comment as I am relatively new
to RT. However, I can say that my focus is on how the picture develops
from a visual perspective. So typically I start with the exposure and
move on to the others, which make meaning to me. I found the module
“Colour Appearance and Lighting” very useful as it was able to handle a
wide range of adjustments in a single module. This was also, I
understand the focus of the challenge, and I’d say I used it in almost
all the pictures to get results to my satisfaction.As I am an amateur, I
use the basic configuration of a laptop for my photo development, so
many of the finer details I miss, as I don’t have a high-resolution
monitor of a respectable size, required by someone who is a serious
post-processing pro. Because of this, I don’t see how I can use some of
the other modules, as I am not sure how it can help. I am, what can be
called an enthusiastic amateur, and someone, who is not very
knowledgeable in the technical aspects.I hope this helps. I was
hesitating giving feedback as I did not see how my input could help,
being at a basic level

### Tester 3

As far as the tool itself is concerned, it is capable of giving results
which are as good as, if not better than other software (Foss and
non-Foss). The downside is that it takes more time “fiddling” compared
to some of the alternatives. This is not a problem of complexity per se
but complexity in so far as it affects the efficient use of the
software. This goes beyond the tool itself, which could be described as
CAM (Color Appearance Model) with a bit of Log encoding. However, we
also have Log Encoding with a bit of CAM in Selective Editing and CAM in
the main menu, each with a set of sliders that can react differently
depending on the tool.

### Tester 4

I only test if there are bugs.

### Tester 5

I gathered from the discussion and the documentation in Rawpedia on the
Color Appearance & Lighting (CAL) that it is a single module that can be
used to process most of the pictures. No other module is required The
exceptions are with some specialized modules like Crop or Spot removal
etc. Is it true, yes I found it so when I used it once again on three
images (with different conditions) and purposefully did not touch any
other. The output IS satisfying for all of them.For people like me, who
process an image visually, and not technically, I found the Rawpedia
article wanting, it wanders far too deep into the technicalities. It
does not talk about the various sliders (the why) and what they do and
maybe gives situations when they could/should be used. I was left
experimenting, not knowing what to expect. Except in the beginning, when
some examples were given, the rest of the article was all Greek and
Latin for me. (excuse the expression). It is from these examples I got
the idea of the ‘power’ of the module.The Rawpedia article needs to
change considerably if you want the user to use the module effectively,
and keep the technicalities in addendum as links. Overall it is an
excellent module, and when used effectively, would obviate the need to
use any other module (except for the exceptions stated earlier), as long
as the user wants to process a picture with a balanced tonal quality.
For artistic expression, other modules would be required. I used the
latest version in zip format, which is downloaded from the dev page.
Hope my rambling is of use to you and other members

### Tester 6

I interpreted the challenge to be mostly about local edits and the
ciecam module. I use these for my own work so it wasn’t much of a change
although I refrained from using other tools , such as tone equalizer, in
one or two occasions. When I have an image to process i look at it and
quickly assess what obvious issues it may have. Personally it’s often
exposure and dynamic range management that needs adjustment. This comes
largely from the digital requirement to expose to avoid clipping.
Several of the images in this challenge had this issue to manage. I
rarely strive to change the image in terms of relative colour
relationships and focus on exposure and detail. To get a feel for the
technical limitations of the file I often dramatically change exposure
to observe clipping, noise and other issues. I also often do this using
other tools such as Color Appearance. This gives me some understanding
how the particular image reacts to particular tools and hints and when I
need to stack multiple tools that handle the same issue but behave
slightly differently. My starting point is often auto match tone curve.
I’ve set my own cameras to a modest sooc profile so it gives good
results with auto matched. I usually leave this on as a sort of look
curve. I find it gives good results for overall tone and even leave it
on when editing files from other people. Naturally I change the curve
when needed but only after finding that it interacts badly with other
tools. For the typical underexposed image I know that setting « Color
Appearance \> Scene Conditions \> Surround to dim » does a good job as
an initial boost of shadows. Dark rarely looks good but I’ve found
photos where it does so I often check. For the horse image I needed to
boost shadows quite a bit both changing surround and using source data
adjustment, this always affects contrast in a problematic way. To
counter this there is a variety of options but in this instance i choose
Ciecam contrast. As the title of the file suggests and the nature of
shaded areas on a sunny day determine the horse was now a bit too blue.
This could be solved by chromatic adjustments of the shaded areas. I
found that making the horse too warm looked odd so maintained a bluish
tint. When files have been pushed this much noise tend to become an
issue which I tackled via a local edit noise reduction tool. The other
images followed a similar approach with changes in how noise was handled
and how much dynamic range was flattened. The goal with all the edits
was to make the images look good without radically changing the look or
atmosphere. In the dancers image I left the people almost black and
focused on good tones and contrast on the dancers themselves.

## Reusing the configuration (Bundles profiles - pp3) - a necessary adaptation?

Each of the pp3 files, for the images supplied, is a rich resource for
the user. Each of the “experts” has supplied “pp3 profile” files that
ensure highly efficient image processing every time.

The number, complexity and variety of images chosen for the challenge
cover a large part of the usual requirements.

You can try out the treatment that best suits your personality with the
images on offer, and then try out these pp3 profiles on your own images.
It will probably be necessary to adapt these pp3s, but you can add them
to the existing “Bundled profiles” by adapting them. The comments added
by these experts should also help guide your choices.

You can use Andy Atsbury's excellent video to show you how
[40](https://www.youtube.com/watch?v=mb96_8BWPyA&t=12s)
