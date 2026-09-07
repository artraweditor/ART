---
title: Resize
contributors:
  - Ingo
  - DrSlony
  - XavAL
  - Thanatomanic
tags:
  - 'Tool Description'
  - 'Transform Tab'
toc: true
---


![](resize_tool_5.4-dev.png "resize_tool_5.4-dev.png")


## Resize

Resizing is one of the last things to happen when saving an image - this
tool runs after most other tools and transformations. As downscaling an
image involves a certain loss of detail, this tool includes a
"Post-Resize Sharpening" component which you can use to make the
downscaled image crisp.

The effects of the Resize tool will not be shown in the preview. The
saved image will of course be resized.

Size limits:

- The downscaling limit is 32x32px.
- The upscaling limit is 16-times the image's size as of RawTherapee
  version 5.5. In versions prior to and including 5.4, it was 4-times
  the image's size.

### Applies To

You will typically want the width and height to apply to the cropped
area, however you can also have it apply to the full image. The crop
will be scaled accordingly.


![](resize-interpolation.png "resize-interpolation.png")


### Method

Resizing can be performed using various algorithms, each with its own
strengths and weaknesses. We narrowed the choice down to the two most
significant:

- Nearest


The "nearest neighbor" method of interpolation is meant to be used when
you want to upscale an image in order to make some detail larger without
introducing smooth blending between pixels. It preserves the pixels as
they are. It is usually used when upscaling by squares of 2: 2x, 4x, 8x,
16x, etc.. The result will be very sharp, but also very blocky. This
method is not meant to be used for everyday photography.

- Lanczos


Lanczos is the default method. It is meant to be used for everyday
photography and in all cases except for the one described above. It
results in a smooth yet sharp and high quality image. Use it to resize
by any amount.

### Specify

You can specify what dimensions to scale to or by:

- Scale


Uniformly scale the image by a factor. Ranges from 0.01 (100 times
smaller) to 16 (16 times bigger).

- Width / Height


Set the desired absolute width or height in pixels, regardless of the
image orientation. The other dimension is updated according to the
aspect ratio.

- Bounding Box


Resize to fit the image within a box of certain width and height in
pixels, regardless of the image orientation. The image aspect ratio is
kept. E.g. for a bounding box of 1920×1080 px and a landscape image of
3000×2000 px the resulting image will be 1620×1080 px. For the same
bounding box and a portrait image of 2000×3000 px the resulting image
will be 720×1080 px.

- Long Edge / Short Edge (Since RawTherapee 5.9)


Set the desired size of either the long or short edge of the image in
pixels. The other dimension is updated according to the aspect ratio.
E.g. setting the long edge to 1500 px for a landscape image of
3000×2000 px gives a resulting image of 1500×1000 px. Setting the short
edge to 1500 px for the same image gives a resulting image of
2250×1500 px. Setting the long edge to 1500 px for a portrait image of
2000×3000 px gives a resulting image of 1000×1500 px.

### Allow Upscaling

This setting controls whether the resize tool will also upscale your
images, or only downscale. If it is disabled and the combination of
image dimensions, crop dimensions and resize dimensions is such that the
image would need to be upscaled to meet your target, this setting will
prevent it from being upscaled - it will remain at its current size.

This setting was introduced in RawTherapee 5.5. If in RawTherapee 5.5
you use a sidecar file from an older version of RawTherapee, this
setting will automatically be disabled.

## Post-Resize Sharpening

Resizing an image often leads to a loss of sharpness, so it is common
practice to sharpen the image again after having resized it. With the
Post-Resize Sharpening tool you can save crystal-clear images straight
away with no further hassle. Because this tool works on the image after
it is resized, you cannot use the preview to see what it will do, though
this is not a problem because the procedure for finding the right values
is straightforward.

The default values work great, but if you want to change them, here's
how to preview what the image will look like:

1.  Tweak your image as you usually would and enable the Resize tool,
    e.g. downscale using the Lanczos method to a 900x900px bounding box.
2.  Save the image to a lossless format such as TIFF.
3.  Open that saved TIFF in RawTherapee, apply the Neutral processing
    profile if that wasn't done automatically, and enable the
    [Sharpening](sharpening) tool in the Detail tab.
4.  Zoom to 100% (1:1) and tweak the Sharpening tool's parameters until
    you get a result that satisfies you. These are the values you should
    use in the Post-Resize Sharpening tool.
5.  Go back to the raw image, enable the Post-Resize Sharpening tool and
    set it up with the values from the previous step.

The Post-Resize Sharpening tool is only available when you use the
"Lanczos" resizing method.

As the Post-Resize Sharpening tool works identically to the standard
Sharpening tool (except that it takes place right at the end of
processing), refer to the [Sharpening](sharpening) article to
learn more about how the sharpening tools work.
