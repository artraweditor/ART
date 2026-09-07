---
title: Image file formats and compression
contributors:
  - DrSlony
  - XavAL
tags:
  - 'General Photography'
toc: true
---

## Obsolete

While the information contained here may have some value, note that it
is obsolete. RawTherapee 5.4 uses an efficient and fast compression
strategy for both TIFF and PNG, and both formats support metadata.

## Intermediate format

An intermediate file is one you use just to pass the image data around
between programs while working on it, e.g. from RawTherapee to GIMP or
Photoshop.

The ideal intermediate format is one which loses no image data or
metadata and is quick to read and write. This pretty much means you
should always use uncompressed 16-bit TIFF as your intermediate format.
Period.

## Final format

Some things to consider when deciding which format to use for storing
non-intermediate files, e.g. final processed images, scanned film, high
dynamic range images, and so forth:

- JPEG is the most widely supported but is lossy and in most
  implementations limited to 8 bits per channel, 24 bits per pixel.
- TIFF and JPEG support metadata, PNG generally does not.
- TIFF can use various compression methods (including the one PNG uses:
  DEFLATE, also known as Zip). Zip is the most efficient compression
  method for real-world photos supported by TIFF.
- PNG is slow and inefficient.
- The images can be converted externally to other more efficient
  formats, such as 16-bit JPEG-2000 or OpenEXR.

## Lossless compression comparison

The input file is a 16-bit 10000x5000 pixel real-world panorama.

To convert the formats I used ImageMagick-6.8.8.10 on an x86_64 Intel(R)
Core(TM) i7 CPU Q 820 @ 1.73GHz, and I scripted the steps in Bash.

### TIFF 16-bit

`for c in RLE None LZW Zip; do time convert foo.tif[0] -monitor -depth 16 -compress "$c" "${c}_16.tif"; ls -lh "${c}_16.tif"; done`

| Format      | Compression | Size \[MB\] | Time \[s\] |
|-------------|-------------|-------------|------------|
| TIFF 16-bit | None        | 382         | 5          |
| TIFF 16-bit | RLE         | 385         | 5          |
| TIFF 16-bit | LZW         | 293         | 8          |
| TIFF 16-bit | Zip         | 243         | 61         |

Yes, the result using RLE was bigger than the uncompressed image.

### TIFF 12-bit

`for c in None LZW Zip; do time convert foo.tif[0] -monitor -depth 12 -compress "$c" "${c}_12.tif"; ls -lh "${c}_12.tif"; done`

| Format      | Compression | Size \[MB\] | Time \[s\] |
|-------------|-------------|-------------|------------|
| TIFF 12-bit | None        | 287         | 4          |
| TIFF 12-bit | LZW         | 248         | 9          |
| TIFF 12-bit | Zip         | 210         | 44         |

GIMP-2.9 and RawTherapee-4.2 do not read 12-bit TIFF files.

### TIFF 8-bit

`for c in None LZW Zip; do time convert foo.tif[0] -monitor -depth 8 -compress "$c" "${c}_8.tif"; ls -lh "${c}_8.tif"; done`

| Format     | Compression | Size \[MB\] | Time \[s\] |
|------------|-------------|-------------|------------|
| TIFF 8-bit | None        | 191         | 3          |
| TIFF 8-bit | LZW         | 51          | 4          |
| TIFF 8-bit | Zip         | 49          | 41         |

### TIFF 16-bit floating-point

`for c in None LZW Zip; do time convert foo.tif[0] -monitor -define quantum:format=floating-point -compress "$c" "${c}_fp.tif"; ls -lh "${c}_fp.tif"; done`

| Format                     | Compression | Size \[MB\] | Time \[s\] |
|----------------------------|-------------|-------------|------------|
| TIFF 16-bit floating-point | None        | 382         | 6          |
| TIFF 16-bit floating-point | LZW         | 153         | 8          |
| TIFF 16-bit floating-point | Zip         | 133         | 72         |

### TIFF 64-bit double-precision floating-point

`for c in None LZW Zip; do time convert foo.tif[0] -monitor -depth 64 -define quantum:format=floating-point -compress "$c" "${c}_64fp.tif"; ls -lh "${c}_64fp.tif"; done`

| Format                            | Compression | Size \[MB\] | Time \[s\] |
|-----------------------------------|-------------|-------------|------------|
| TIFF 64-bit floating-point double | None        | 1525        | 21         |
| TIFF 64-bit floating-point double | LZW         | 1016        | 36         |
| TIFF 64-bit floating-point double | Zip         | 551         | 104        |

### OpenEXR 16-bit floating-point

OpenEXR is a good option; it's modern, efficient and widely supported.

`for c in None Zip PIZ; do time convert foo.tif[0] -monitor -depth 16 -compress "$c" "${c}_16.exr"; ls -lh "${c}_16.exr"; done`

| Format     | Compression | Size \[MB\] | Time \[s\] |
|------------|-------------|-------------|------------|
| EXR 16-bit | None        | 382         | 13         |
| EXR 16-bit | Zip         | 106         | 21         |
| EXR 16-bit | PIZ         | 111         | 7          |

To convert the EXR to a 16-bit TIFF you need to set the colorspace to
sRGB (set it, not transform it):

`convert Zip_16.exr -depth 16 -set colorspace sRGB suchwow.tif`

### PNG 16-bit

Finally, PNG. ImageMagick lets you set the zlib compression level 1-9,
or uses Huffman compression if you set the first value to 0. The second
value sets date encoding filtering, where 5=Adaptive, best for
real-world photos.

`for l in 0 6 9; do time foo.tif[0] -monitor -depth 16 -quality "${l}5" "${l}5_16.png"; ls -lh "${l}5_16.png"; done`

| Format     | Compression | Size \[MB\] | Time \[s\] |
|------------|-------------|-------------|------------|
| PNG 16-bit | level 0     | 310         | 9          |
| PNG 16-bit | level 6     | 233         | 87         |
| PNG 16-bit | level 9     | 233         | 351        |

Notice the same file size of level 6 and 9 despite taking 4 times as
long.

### PNG 8-bit

`for l in 0 6 9; do time foo.tif[0] -monitor -depth 8 -quality "${l}5" "${l}5_8.png"; ls -lh "${l}5_8.png"; done`

| Format    | Compression | Size \[MB\] | Time \[s\] |
|-----------|-------------|-------------|------------|
| PNG 8-bit | level 0     | 137         | 10         |
| PNG 8-bit | level 6     | 49          | 40         |
| PNG 8-bit | level 9     | 46          | 341        |
