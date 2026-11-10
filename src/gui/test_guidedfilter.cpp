/** -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright (c) 2018-2026 Alberto Griggio <alberto.griggio@gmail.com>
 *
 *  ART is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  ART is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with ART.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include "../engine/color.h"
#include "../engine/guidedfilter.h"
#include "../engine/imagefloat.h"
#include "../engine/stdimagesource.h"
#include <stdio.h>

using namespace art::engine;

void save(array2D<float> &img, const char *filename)
{
    Imagefloat im(img.width(), img.height());
#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (int y = 0; y < im.getHeight(); ++y) {
        for (int x = 0; x < im.getWidth(); ++x) {
            im.r(y, x) = im.g(y, x) = im.b(y, x) = img[y][x] * 65535.f;
        }
    }

    im.saveTIFF(filename, 16);
}

int main(int argc, const char **argv)
{
    Gio::init();
    Options::load(true);

    int err = 0;
    InitialImage *ii = InitialImage::load(argv[1], false, &err);
    if (err) {
        fprintf(stderr, "ERROR!\n");
        return 1;
    }

    fprintf(stderr, "loaded\n");
    fflush(stderr);

    ImageSource *src = ii->getImageSource();

    int w, h;
    src->getFullSize(w, h);
    Imagefloat im(w, h);
    src->getImage(ColorTemp(), TR_NONE, &im, art::engine::PreviewProps(0, 0, w, h, 1),
                  ExposureParams(), RAWParams());

    fprintf(stderr, "before saving\n");

    im.saveTIFF("/tmp/input.tif", 16);

    array2D<float> Y(im.getWidth(), im.getHeight());
#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (int y = 0; y < im.getHeight(); ++y) {
        for (int x = 0; x < im.getWidth(); ++x) {
            Y[y][x] =
                1.f - Color::rgbLuminance(im.r(y, x), im.g(y, x), im.b(y, x)) /
                          65535.f;
        }
    }

    save(Y, "/tmp/Y.tif");

    array2D<float> mask(im.getWidth(), im.getHeight());
#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (int y = 0; y < im.getHeight(); ++y) {
        for (int x = 0; x < im.getWidth(); ++x) {
            mask[y][x] = 1.f - ((1.f - Y[y][x]) < 0.25f ? 0.f : 1.f);
        }
    }

    save(mask, "/tmp/mask.tif");

    int r = 60;
    if (argc >= 3) {
        r = atoi(argv[2]);
    }
    float eps = SQR(0.001);
    if (argc >= 4) {
        eps = atof(argv[3]);
    }
    int scale = 1;
    if (argc >= 5) {
        scale = atoi(argv[4]);
    }
    fprintf(stderr, "\n*** RUNNING WITH r = %d, eps = %f, scale = %d\n\n", r,
            eps, scale);

    array2D<float> blurred(im.getWidth(), im.getHeight());
    guidedFilter(Y, mask, blurred, r, eps, true, scale);
    // for (int y = 0; y < im.getHeight(); ++y) {
    //     for (int x = 0; x < im.getWidth(); ++x) {
    //         if (blurred[y][x] < 0.f) {
    //             std::cerr << "BAD NEG!!" << std::endl;
    //             return 1;
    //         }
    //         if (blurred[y][x] > 1.f) {
    //             std::cerr << "BAD POS!!" << std::endl;
    //             return 1;
    //         }
    //     }
    // }

    save(blurred, "/tmp/guided.tif");

    return 0;
}
