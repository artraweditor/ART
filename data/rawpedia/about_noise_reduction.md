---
title: About Noise Reduction
contributors:
  - Jdc
  - DrSlony
tags:
  - 'General Information'
toc: true
---


## Preamble

To put the Local Adjustments denoise tools into context, it is useful to
summarize the other denoise tools available elsewhere in RawTherapee
(Detail and Wavelets tabs) and to provide some additional information on
the algorithms used. This information is complementary to the existing
documentation, which can be found in the relevant sections of Rawpedia.

Tools initially designed by Emil Martinec in 2012 located in
"Ftblockdn.cc".

- Wavelets in RGB mode
- Fourier Transform (DCT)

Other tools

- Guided filter
- Median filters
- Bilateral filter
- Gaussian blur
- Dark frame
- Hot/Dead pixels, etc.

We will look at the first two: Wavelets & Fourier.

## The original module designed by E. Martinec between 2008 & 2012

It is composed of basic functions (Wavelets, DCT) which can also be
called by other programs, such as Wavelet Levels and Local Adjustments.
These functions were originally designed to provide:

- Wavelet processing with the following characteristics:
  - Global action, i.e. identical for all decomposition levels and for
    the whole range of luminance or chroma values.
  - Global noise assessment - per decomposition level - using MAD
    (median absolute deviation).
  - One or two passes ("conservative" and "aggressive").
- Fourier processing for luminance noise using DCT (Discrete Cosine
  Transform) to process the residual noise (equal to the difference
  between the original image and the wavelet-denoised image).

These 2 functions were contained in a general module (Noise Reduction in
the Detail tab):

- Using luminance and chrominance with a single slider for each
- Using automatic tiling for both wavelet and DCT operations to reduce
  memory requirements.

Note that in the beginning, the Noise Reduction module was at the end of
the process (this was also the case for the PerfectRAW product which I
worked on with E.Martinec and M.LLorens).

## Enhancements to original features carried out between 2012 & 2020

Several improvements were made by Ingo Weyrich and Jacques Desmis:

1.  Possibility of denoising by level of decomposition.
2.  Ability to denoise at pixel level by taking into account the
    luminance and chrominance.
3.  Ability to extend the DCT Fourier processing to chrominance.
4.  Addition of a DCT threshold to take into account the edge effect
    (origin ART ).

## Improvements to the general noise reduction module (Noise Reduction - Detail tab)

- Automatic calculation of noise suppression settings.
- 2 additional curves to process more finely the luminance and
  chrominance noise - 'y' axis = amplitude, 'x' axis = luminance or
  chrominance intensity.
- Ability to use L\*a\*b\* mode instead of RGB mode.
- Addition of median-filter denoising.

## Improvements with "Non-local means" - also called patch-based denoise (Local Adjustments tab) February 2021.

- This algorithm (origin Ipol 2014
  [1](https://pdfs.semanticscholar.org/a262/269fa4b44b64d750c72fad3887f9d88817ce.pdf))
  was ported to ART by Alberto Griggio and improved by Ingo Weyrich,
  Jacques Desmis adapted it to RawTherapee.

<!-- -->

- What is patch-based denoise? Contrary to the usual filters that reduce
  noise by averaging the values of groups of pixels located around a
  target pixel, non-local means filters average the values of all the
  pixels in the image and weight them according to their similarity with
  the target pixel. This type of filtering reduces the loss of detail
  compared to filters that use local averaging.

## A few remarks

Denoising is subject to a lot of debate, often controversial. There is
also a lot of dogma and quarrels surrounding the methods and tools. My
position on the following subjects is to be pragmatic because what
counts is the final result.

## Where should the denoise functions be located?

At the beginning, or at the end of the process? Each has its advantages
and disadvantages.

- At the beginning: denoise is carried out in linear mode, but does not
  take into account subsequent processing (sharpening, exposure,
  saturation etc.) that may increase the noise or its appearance. This
  results in a tendency to overcompensate for noise.
- At the end: denoise is carried out in non-linear mode and takes into
  account all the processing operations including any noise artifacts
  that appeared early in the pipeline and were amplified by subsequent
  processing.
- The ideal would be to combine the two: minimal denoise at the
  beginning, then additional denoising at the end.

## RGB mode or Lab mode?

Each method has its advantages and disadvantages, again we need to be
pragmatic.

- The linear RGB mode should theoretically be better, mainly because of
  the superiority of MAD noise evaluation. However, we need to keep in
  mind that the way our eyes and brain react to noise can be modeled in
  the same way as CAMs (colored appearance models) i.e. the same noise
  level will be perceived differently according to whether the
  background is black, gray or white and varies with color and
  saturation.
- The Lab mode , which requires the addition of contrast and luminance
  management tools (gamma, curves, sliders etc.), can be better in many
  cases. In addition, Lab seems to discriminate color noise better.

## How many wavelet levels?

- It is often said that only the first 4 levels are useful. This is true
  enough for luminance noise when the CAM effect is taken into account.
- However, if we apply noise reduction to the luminance components above
  level 4 we can definitely see a difference.
- For chrominance noise, the first 4 levels are suitable for the noise
  which generates distributed colored pixels. For packets of noise
  (blotches), we can only get rid of these by going up to level 7.

## Main characteristics of the various RawTherapee noise reduction tools

### Noise Reduction (Detail tab)

- No differentiation by level of decomposition.
- Pixel differentiation for luminance.
- 5 decomposition levels for luminance, 6 for chrominance.
- DCT used for luminance only.
- No DCT threshold.
- Automatic tiling for wavelets.
- Automatic adjustment option.

### Denoise & Refine (Wavelet Levels, Advanced tab)

- Differentiation by level of decomposition for Luminance and
  Chrominance (fine and coarse).
- Pixel differentiation for luminance and possibility of taking hue into
  account.
- 6 decomposition levels used for luminance, 6 for chrominance.
- Differentiation using local contrast.
- No DCT.
- Tiling option.

### Blur/Grain & Denoise (Local Adjustments tab)

- Differentiation by level of decomposition for luminance and
  chrominance (fine and coarse).
- Pixel differentiation for luminance and possibility of taking hue into
  account.
- 7 decomposition levels used for luminance, 7 for chrominance.
- DCT for luminance and chrominance.
- DCT threshold for luminance and chrominance.
- Recovery of all or part of the original image (prior to denoise) with
  the help of masks.
- Local differential selection (Rtspot).
- Use of deltaE and masks to improve selection (RTspot).
- Use of an Excluding spot to recover noisy areas.
- No tiling: this means that when you work in full-image mode and the
  image is \> ~30Mb, you will need more than 8Gb of RAM to work without
  crashing.
- Since February 2021 - Non-local means (Patch-based denoise).

## Which modules should you use?

It is very difficult to answer this question, unless you want to denoise
just part of the image. In this case you need to use Blur/Grain &
Denoise in the Local Adjustments tab. Nevertheles :

- With noisy or very noisy raw images from older cameras with smaller
  sensors, the Noise Reduction module in the Detail tab is essential and
  easy to use.
- With recent low-noise raw files, the Blur/Grain & Denoise module in
  the Local Adjustments tab is by far the most comprehensive even if it
  is a little complex.
- The Denoise & Refine module in Wavelet Levels (Advanced tab) has the
  advantage of being part of the same interface as the other
  wavelet-processing tools thereby making it easier to tweak the
  settings in conjunction with the other wavelet parameters.
- There is no point in using all 3 together. However, for noisy images
  the combination of Noise Reduction and one of the other modules either
  in Wavelet Levels or Local Adjustments is recommended.

## Denoising with Local adjustments

### Steps in the process

(in the order they appear in the menu)

These tools allow:

- Denoise by level of detail
- Differentiation between flat areas and areas with structure.

#### Aggressive / Conservative mode (uses wavelet processing)

The "aggressive" setting is not necessarily more destructive than the
"conservative” setting. It all depends on the Strength settings for
luminance and chrominance. In some cases two passes with a lower
Strength setting will lead to a better result. However, the processing
time will be significantly increased in this case.

#### Luminance denoise by level (uses wavelet processing)

The abscissa ("x" axis) represents the levels of decomposition (from 0
to 6, left to right). The y-axis represents the amount of denoise
applied at each level.

#### Luma detail recovery (DCT f - uses Fourier transform and wavelets)

This slider uses a Fourier transform to take into account the difference
between the L (L\*a\*b\*) component of the wavelet-denoised image and
the original image. It is important not to set this slider to zero
because in this case, the Fourier transform will have a overly strong
effect on the noise and prevent the 3 equalizers from having any real
influence. The more the slider is moved to the right, the weaker the DCT
action will be and the more the details will be apparent.

#### Equalizer white-black (uses data prior to wavelet processing)

This slider corresponds to the luminance curve of the Noise Reduction
module in simplified form. It allows you to focus the action on either
the dark or light tones of the image. The action of the “Luminance
denoise” curve will be modulated according to the position of the
slider.

#### Denoise hue equalizer (uses data prior to wavelet processing)

This curve allows you to increase or reduce the action of the "Luminance
denoise" curve as a function of the color (hue) of the image, .

#### Denoise based on luminance mask (uses data prior to wavelet processing)

- The mask in Mask and modifications (Blur & Denoise ) must be
  activated.
- One or both of the two curves L(L) - LC(H) must be activated.
- The slider "Dark area luma threshold" allows you to choose the
  luminance level. below which the action of the "Luminance denoise by
  level" curve will be reinforced.
- The slider "Light area luma threshold" allows you to choose the
  luminance level above which the action of the curve "Luminance
  denoise" will be reinforced.
- Between the two values, the denoise action will depend on the curve
  settings.
- Reinforce denoise in dark and light areas.

Depends on the individual image and whether or not there are uniform
dark and/or light areas. If the image contains an almost perfectly
uniform dark area, the background of the corresponding mask will be
completely black (L = 0). If the dark area is not completely uniform
this value will vary. The slider will progressively increase the action
of the denoise curve between the dark or light threshold and the
completely uniform area. Values less than 1 decrease the action of the
curve.

#### "Edge detection - DCT" \> Luma and chroma detail threshold (DCT f - uses Fourier transforms and wavelets)

This slider tries to take into account the edge effect of the original
image in order to differentiate the action between uniform areas and
areas with detail. Two algorithms are available (both from ART)

- "buildblendmask" - similar function to that used to differentiate
  demosaicing, for example.
- "Laplacian"
- The first will generally be more progressive, the second more
  selective, especially when the cursor is very close to the maximum.

#### Non-local means - patch-based denoise

Can be used in conjunction with Wavelets and DCT or alone. It allows
very efficient luminance noise reduction. It will notably differentiate
between flatness and texture.

#### Fine chroma (uses wavelets)

This slider acts on the first 4 levels of decomposition and will
generally reduce or remove dots of colored noise.

#### Coarse chroma (uses wavelets)

This slider acts on the last 3 decomposition levels and will generally
reduce or remove the blotches or packets of colored noise.

#### Equalizer Blue-yellow /Red Green (uses wavelets)

Changes the distribution of color noise processing.

#### Chroma detail recovery (DCT – uses Fourier transforms and wavelets)

This slider uses a Fourier transform to take into account the difference
between the "a" and "b" (L\*a\*b\*) components of the wavelet-denoised
image and the original image. It is important not to set this slider to
zero because the Fourier transform will have an overly strong effect on
the noise and prevent the edge detection from having any real influence.
The more the slider is moved to the right, the weaker the DCT action
will be and the more the color details will be apparent.

#### Recovery based on luminance mask (uses data before and after denoise processing)

This module acts on the difference between the original noisy image and
the denoised image processed with the above tools. Completely black
areas in the mask will retain the denoise settings whereas completely
white areas will retain the image settings prior to denoising.

- The mask in Mask and modifications (Blur & Denoise ) must be
  activated.
- One or both of the two curves L(L) - LC(H) must be activated.
- The "Dark area luma threshold" slider allows you to choose the
  luminance level below which the denoise settings will be progressively
  applied.
- The "Light area luma threshold" slider allows you to choose the
  luminance level above which the denoise settings will be progressively
  applied.
- Between the two values, the image will be preserved without denoising,
  unless the "Gray area luma denoise" slider is greater than 0; this can
  be useful for example to attenuate any unsightly chromatic noise.
- The "Recovery threshold" slider allows you to choose the level of
  denoise that will be applied below or above the "dark" and "light"
  thresholds respectively.
- The "Decay strentgh" slider allows you to choose the rate at which the
  attenuation or amplification will be progressively applied.

### Note on masks

- You can use a lockable Color Picker (LCP) to help identify the extent
  to which the different parts of the mask will attenuate or maintain
  the denoise settings. To ensure that the LCP information corresponds
  to the slider values, the “Background color/luma mask” must be set to
  zero in Settings.
- You can use the Mask Tools (see below) in association with the LCP, to
  adjust the gray areas and make them lighter or darker, thereby
  adjusting the amount of denoise that is applied.

Mask Tools: "Structure mask", "Smooth radius", "Gamma", "Slope",
"Shadows", "Curve L(L)", "Local contrast(L)" curve, curve using
wavelets.

### Conclusion

The Local Adjustments module allows you to :

- Make local adjustments to the noise and differentiate the action by
  luminance and color.
- Process the entire image and use the unique features of the local
  adjustments module:
  - deltaE
  - Excluding spot
  - Transitions
  - Masks.

### Guided Filter

When the values of the Blur & Noise \> Guided Filter (in the combobox)
\> Detail slider are negative, this tool generates a blur that will mask
the luminance and chrominance noise to give a denoising effect.

- "Recovery based on luminance mask" works similarly to the equivalent
  function in Denoise.
