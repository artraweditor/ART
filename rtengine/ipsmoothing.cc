/* -*- C++ -*-
 *
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2018 Alberto Griggio <alberto.griggio@gmail.com>
 *
 *  RawTherapee is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RawTherapee is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RawTherapee.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <complex.h>
#include <fftw3.h>

#include "alignedbuffer.h"
#include "curves.h"
#include "gauss.h"
#include "guidedfilter.h"
#include "improcfun.h"
#include "ipdenoise.h"
#include "masks.h"
#include "nlmeans.h"
#include "wavelet.h"
#include "rescale.h"
#include "rt_algo.h"
#include "rt_math.h"
#include "sleef.h"
#include <iostream>
#include <queue>

// #include <random>
#include "rng.h"

#define BENCHMARK
#include "StopWatch.h"

#ifdef _OPENMP
#include <omp.h>
#endif

extern Options options;

namespace rtengine {

namespace { enum class Channel { L, C, LC }; }

namespace gpu { namespace ops {

bool wavelet_smoothing(Imagefloat *rgb,
                       const TMatrix &ws, float strength, int levels,
                       float gamma, double scale, Channel chan,
                       Context *ctx, BufferPool *pool);

bool nlmeans_smoothing(Imagefloat *rgb,
                       const TMatrix &ws, const TMatrix &iws, Channel chan,
                       int strength, int detail, int iterations, double scale,
                       Context *ctx, BufferPool *pool);

}} // namespace gpu::ops

namespace {

// code adapted from darktable, src/iop/blurs.c
/*
    This file is part of darktable,
    Copyright (C) 2021 darktable developers.

    darktable is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    darktable is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with darktable.  If not, see <http://www.gnu.org/licenses/>.
*/
void blur_2D_Bspline(const array2D<float> &src, array2D<float> &dst)
{
    const int height = src.height();
    const int width = src.width();
    constexpr int FSIZE = 5;
    constexpr float filter[FSIZE] = {1.0f / 16.0f, 4.0f / 16.0f, 6.0f / 16.0f,
                                     4.0f / 16.0f, 1.0f / 16.0f};

    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            float acc = 0.f;

            for (int ii = 0; ii < FSIZE; ++ii) {
                for (int jj = 0; jj < FSIZE; ++jj) {
                    const int row =
                        LIM(i + (ii - (FSIZE - 1) / 2), 0, height - 1);
                    const int col =
                        LIM(j + (jj - (FSIZE - 1) / 2), 0, width - 1);

                    acc += filter[ii] * filter[jj] * src[row][col];
                }
            }

            dst[j][i] = acc;
        }
    }
}

void create_lens_kernel(array2D<float> &buffer, const float n, const float m,
                        const float k, const float rotation)
{
    const int width = buffer.width();
    const int height = buffer.height();

    // n is number of diaphragm blades
    // m is the concavity, aka the number of vertices on straight lines (?)
    // k is the roundness vs. linearity factor
    //   see https://math.stackexchange.com/a/4160104/498090
    // buffer sizes need to be odd

    // Spatial coordinates rounding error
    const float eps = 1.f / (float)width;
    const float radius = float(width / 2);

    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            // get normalized kernel coordinates in [-1 ; 1]
            const float x = (i - 1) / radius - 1;
            const float y = (j - 1) / radius - 1;

            // get current radial distance from kernel center
            const float r = hypotf(x, y);

            // get the radial distance at current angle of the shape envelope
            const float M =
                cosf((2.f * asinf(k) + RT_PI_F * m) / (2.f * n)) /
                cosf((2.f * asinf(k * cosf(n * (atan2f(y, x) + rotation))) +
                      RT_PI_F * m) /
                     (2.f * n));

            // write 1 if we are inside the envelope of the shape, else 0
            buffer[i][j] = (M >= r + eps);
        }
    }
}

void create_motion_kernel(array2D<float> &buffer, const float angle,
                          const float curvature, const float offset)
{
    const int width = buffer.width();
    // const int height = buffer.height();

    buffer.fill(0.f);

    // Compute the polynomial params from user params
    const float A = curvature / 2.f;
    const float B = 1.f;
    const float C = -A * offset * offset + B * offset;
    // Note : C ensures the polynomial arc always goes through the central pixel
    // so we don't shift pixels. This is meant to allow seamless connection
    // with unmasked areas when using masked blur.

    // Spatial coordinates rounding error
    const float eps = 1.f / (float)width;

    const float radius = float(width / 2);
    const float corr_angle = -RT_PI_F / 4.f - angle;

    // Matrix of rotation
    const float M[2][2] = {{cosf(corr_angle), -sinf(corr_angle)},
                           {sinf(corr_angle), cosf(corr_angle)}};

    for (int i = 0; i < 8 * width; ++i) {
        // Note : for better smoothness of the polynomial discretization,
        // we oversample 8 times, meaning we evaluate the polynomial
        // every eighth of pixel

        // get normalized kernel coordinates in [-1 ; 1]
        const float x = (i / 8.f - 1) / radius - 1;
        // const float y = (j - 1) / radius - 1; // not used here

        // build the motion path : 2nd order polynomial
        const float X = x - offset;
        const float y = X * X * A + X * B + C;

        // rotate the motion path around the kernel center
        const float rot_x = x * M[0][0] + y * M[0][1];
        const float rot_y = x * M[1][0] + y * M[1][1];

        // convert back to kernel absolute coordinates ± eps
        const int y_f[2] = {int(roundf((rot_y + 1) * radius - eps)),
                            int(roundf((rot_y + 1) * radius + eps))};
        const int x_f[2] = {int(roundf((rot_x + 1) * radius - eps)),
                            int(roundf((rot_x + 1) * radius + eps))};

        // write 1 if we are inside the envelope of the shape, else 0
        // leave 1px padding on each border of the kernel for the anti-aliasing
        for (int l = 0; l < 2; l++) {
            for (int m = 0; m < 2; m++) {
                if (x_f[l] > 0 && x_f[l] < width - 1 && y_f[m] > 0 &&
                    y_f[m] < width - 1) {
                    buffer[y_f[m]][x_f[l]] = 1.f;
                }
            }
        }
    }
}

float compute_norm(array2D<float> &buffer)
{
    const int width = buffer.width();
    const int height = buffer.height();

    float norm = 0.f;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            norm += buffer[y][x];
        }
    }
    return norm;
}

void normalize(array2D<float> &buffer, const float norm)
{
    const int width = buffer.width();
    const int height = buffer.height();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            buffer[y][x] /= norm;
        }
    }
}

bool build_blur_kernel(array2D<float> &out, const SmoothingParams &params,
                       int region, double scale)
{
    const auto &r = params.regions[region];

    int radius = std::ceil(r.radius / scale);
    if (radius < 1) {
        return false;
    }

    int kersz = 2 * radius + 1;
    array2D<float> buf(kersz, kersz);
    out(kersz, kersz);
    constexpr float torad = RT_PI_F / 180.f;

    if (r.mode == SmoothingParams::Region::Mode::MOTION) {
        create_motion_kernel(buf, (r.angle + 90) * torad, r.curvature,
                             r.offset);
    } else {
        assert(r.mode == SmoothingParams::Region::Mode::LENS);
        create_lens_kernel(buf, r.numblades, 1.f, 1.f,
                           r.angle * torad + RT_PI_F);
    }
    blur_2D_Bspline(buf, out);
    float norm = compute_norm(out);
    normalize(out, norm);

    return true;
}


void lens_motion_blur(ImProcData &im, Imagefloat *rgb,
                      const array2D<float> &mask, int region)
{
    Imagefloat tmp;
    Imagefloat *src = rgb;

    const int W = rgb->getWidth();
    const int H = rgb->getHeight();

    array2D<float> kernel;
    if (build_blur_kernel(kernel, im.params->smoothing, region, im.scale)) {
        // prevent the blur from bleeding outside the mask
        // idea taken from https://discuss.pixls.us/t/difficulty-with-blurs-and-masks/50274/21
        // credit to @kofa (István Kovács)
        //
        // Method: 
        // - create a mask
        // - blurred_image_mask = blur(image * mask)
        // - blurred_mask = blur(mask)
        // - blurred_image = blurred_image_mask / blurred_mask
        // - apply blurred_image to base image via mask.
        //
        // Rationale:
        // - where the mask is white, far from the hole/subject → mask
        //   is white → no change, keep the blurred pixel;
        // - where the mask is grey, close to the edge → pixel is
        //   darkened by the blurred mask bleeding into the pixel, but
        //   blurred mask < 1 → darkened pixel divided by e.g. 0.5 →
        //   restores brightness;
        // - where the mask is black: we don’t care.
        
        tmp.allocate(W, H);
#ifdef _OPENMP
#       pragma omp parallel for if (im.multiThread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                const float f = LIM01(mask[y][x]);
                tmp.r(y, x) = rgb->r(y, x) * f;
                tmp.g(y, x) = rgb->g(y, x) * f;
                tmp.b(y, x) = rgb->b(y, x) * f;
            }
        }
        rgb = &tmp;
        
        Convolution conv(kernel, rgb->getWidth(), rgb->getHeight(),
                         im.multiThread);
        conv(rgb->r.ptrs, rgb->r.ptrs);
        conv(rgb->g.ptrs, rgb->g.ptrs);
        conv(rgb->b.ptrs, rgb->b.ptrs);

        array2D<float> bmask(W, H);
        conv(mask, bmask);
#ifdef _OPENMP
#       pragma omp parallel for if (im.multiThread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                if (bmask[y][x] > 1e-6f) {
                    rgb->r(y, x) /= bmask[y][x];
                    rgb->g(y, x) /= bmask[y][x];
                    rgb->b(y, x) /= bmask[y][x];
                }
            }
        }
    }

    if (src != rgb) {
#ifdef _OPENMP
#pragma omp parallel for if (im.multiThread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                const float b = LIM01(mask[y][x]);
                src->r(y, x) = intp(b, rgb->r(y, x), src->r(y, x));
                src->g(y, x) = intp(b, rgb->g(y, x), src->g(y, x));
                src->b(y, x) = intp(b, rgb->b(y, x), src->b(y, x));
            }
        }
    }
}

//-----------------------------------------------------------------------------

void guided_smoothing(array2D<float> &R, array2D<float> &G, array2D<float> &B,
                      const TMatrix &ws, const TMatrix &iws, Channel chan,
                      int radius, float epsilon, double scale, bool multithread)
{
    const auto rgb2yuv = [&](float R, float G, float B, float &Y, float &u,
                             float &v) -> void {
        Color::rgb2yuv(R, G, B, Y, u, v, ws);
    };

    const auto yuv2rgb = [&](float Y, float u, float v, float &R, float &G,
                             float &B) -> void {
        Color::yuv2rgb(Y, u, v, R, G, B, ws);
    };

    int r = max(int(round(radius / scale)), 0);
    if (r > 0) {
        const int W = R.width();
        const int H = R.height();

        array2D<float> iR(W, H, R, ARRAY2D_ALIGNED);
        array2D<float> iG(W, H, G, ARRAY2D_ALIGNED);
        array2D<float> iB(W, H, B, ARRAY2D_ALIGNED);

        const bool rgb = (chan == Channel::LC);
        const bool luminance = (chan == Channel::L);

        if (rgb) {
            rtengine::guidedFilterLog(10.f, R, r, epsilon, multithread);
            rtengine::guidedFilterLog(10.f, G, r, epsilon, multithread);
            rtengine::guidedFilterLog(10.f, B, r, epsilon, multithread);
        } else {
            array2D<float> guide(W, H, ARRAY2D_ALIGNED);
#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
            for (int y = 0; y < H; ++y) {
                for (int x = 0; x < W; ++x) {
                    float l =
                        Color::rgbLuminance(R[y][x], G[y][x], B[y][x], ws);
                    guide[y][x] = xlin2log(max(l, 0.f), 10.f);
                }
            }
            rtengine::guidedFilterLog(guide, 10.f, R, r, epsilon, multithread);
            rtengine::guidedFilterLog(guide, 10.f, G, r, epsilon, multithread);
            rtengine::guidedFilterLog(guide, 10.f, B, r, epsilon, multithread);

#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
            for (int y = 0; y < H; ++y) {
                for (int x = 0; x < W; ++x) {
                    float rr = R[y][x];
                    float gg = G[y][x];
                    float bb = B[y][x];
                    float ir = iR[y][x];
                    float ig = iG[y][x];
                    float ib = iB[y][x];

                    float iY, iu, iv;
                    float oY, ou, ov;
                    rgb2yuv(ir, ig, ib, iY, iu, iv);
                    rgb2yuv(rr, gg, bb, oY, ou, ov);
                    if (luminance) {
                        ou = iu;
                        ov = iv;
                    } else {
                        float bump = oY > 1e-5f ? iY / oY : 1.f;
                        ou *= bump;
                        ov *= bump;
                        oY = iY;
                    }
                    yuv2rgb(oY, ou, ov, R[y][x], G[y][x], B[y][x]);
                }
            }
        }
    }
}

void find_region(const array2D<float> &mask, int &min_x, int &min_y, int &max_x,
                 int &max_y)
{
    const int W = mask.width();
    const int H = mask.height();

    min_x = W - 1;
    min_y = H - 1;
    max_x = 0;
    max_y = 0;

    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            if (mask[y][x] > 0.f) {
                min_x = std::min(min_x, x);
                max_x = std::max(max_x, x);
                min_y = std::min(min_y, y);
                max_y = std::max(max_y, y);
            }
        }
    }

    ++max_x;
    ++max_y;
}

void gaussian_smoothing(array2D<float> &R, array2D<float> &G, array2D<float> &B,
                        float *buf, const TMatrix &ws, Channel chan,
                        double sigma, double scale, bool multithread)
{
    // static constexpr double HIGH_PRECISION_THRESHOLD = 2.0;

    double s = sigma / scale;
    const int W = R.width();
    const int H = R.height();

    array2D<float> kernel;
    const bool high_precision = s > 0; // && s < HIGH_PRECISION_THRESHOLD;
    std::unique_ptr<Convolution> conv;
    if (high_precision) {
        build_gaussian_kernel(s, kernel);
        conv.reset(new Convolution(kernel, W, H, multithread));
    }

    const auto blur = [&](array2D<float> &a) -> void {
        if (high_precision) {
            (*conv)(a, a);
        } else {
#ifdef _OPENMP
#pragma omp parallel if (multithread)
#endif
            gaussianBlur(a, a, W, H, s, buf);
        }
    };

    if (chan == Channel::LC) {
        blur(R);
        blur(G);
        blur(B);
    } else {
        const bool luminance = (chan == Channel::L);
        array2D<float> iR(W, H, R, ARRAY2D_ALIGNED);
        array2D<float> iG(W, H, G, ARRAY2D_ALIGNED);
        array2D<float> iB(W, H, B, ARRAY2D_ALIGNED);

        blur(R);
        blur(G);
        blur(B);

#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                float iY, iu, iv;
                float oY, ou, ov;
                Color::rgb2yuv(iR[y][x], iG[y][x], iB[y][x], iY, iu, iv, ws);
                Color::rgb2yuv(R[y][x], G[y][x], B[y][x], oY, ou, ov, ws);
                if (luminance) {
                    Color::yuv2rgb(oY, iu, iv, R[y][x], G[y][x], B[y][x], ws);
                } else {
                    Color::yuv2rgb(iY, ou, ov, R[y][x], G[y][x], B[y][x], ws);
                }
            }
        }
    }
}

void nlmeans_smoothing(Imagefloat *rgb,
                       const TMatrix &ws, const TMatrix &iws, Channel chan,
                       int strength, int detail, int iterations, double scale,
                       bool multithread, gpu::Context *ctx, gpu::BufferPool *pool)
{
    MyTime t1p, t2p;
    t1p.set();
    if (gpu::ops::nlmeans_smoothing(rgb, ws, iws, chan, strength, detail,
                                    iterations, scale, ctx, pool)) {
        if (settings->verbose) {
            t2p.set();
            std::cout << "nlmeans_smoothing: executed on the GPU in "
                      << t2p.etime(t1p) << " usec" << std::endl;
        }
        return;
    }

    array2D<float> iY;
    const int W = rgb->getWidth();
    const int H = rgb->getHeight();

    array2D<float> R(W, H, rgb->r.ptrs, ARRAY2D_BYREFERENCE);
    array2D<float> G(W, H, rgb->g.ptrs, ARRAY2D_BYREFERENCE);
    array2D<float> B(W, H, rgb->b.ptrs, ARRAY2D_BYREFERENCE);    

    if (chan == Channel::L) {
        iY(W, H, ARRAY2D_ALIGNED);
#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
        for (int y = 0; y < H; ++y) {
            float u, v;
            for (int x = 0; x < W; ++x) {
                Color::rgb2yuv(R[y][x], G[y][x], B[y][x], iY[y][x], u, v, ws);
            }
        }
        for (int i = 0; i < iterations; ++i) {
            denoise::NLMeans(iY, 1.f, strength, detail, scale, multithread);
        }
#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
        for (int y = 0; y < H; ++y) {
            float Y, u, v;
            for (int x = 0; x < W; ++x) {
                Color::rgb2yuv(R[y][x], G[y][x], B[y][x], Y, u, v, ws);
                Color::yuv2rgb(iY[y][x], u, v, R[y][x], G[y][x], B[y][x], ws);
            }
        }
    } else {
        if (chan == Channel::C) {
            iY(W, H, ARRAY2D_ALIGNED);
#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
            for (int y = 0; y < H; ++y) {
                float u, v;
                for (int x = 0; x < W; ++x) {
                    Color::rgb2yuv(R[y][x], G[y][x], B[y][x], iY[y][x], u, v,
                                   ws);
                }
            }
        }

        for (int i = 0; i < iterations; ++i) {
            denoise::NLMeans(R, 1.f, strength, detail, scale, multithread);
            denoise::NLMeans(G, 1.f, strength, detail, scale, multithread);
            denoise::NLMeans(B, 1.f, strength, detail, scale, multithread);
        }

        if (chan == Channel::C) {
#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
            for (int y = 0; y < H; ++y) {
                float Y, u, v;
                for (int x = 0; x < W; ++x) {
                    Color::rgb2yuv(R[y][x], G[y][x], B[y][x], Y, u, v, ws);
                    Color::yuv2rgb(iY[y][x], u, v, R[y][x], G[y][x], B[y][x],
                                   ws);
                }
            }
        }
    }

    if (settings->verbose) {
        t2p.set();
        std::cout << "nlmeans_smoothing: executed on the CPU in "
                  << t2p.etime(t1p) << " usec" << std::endl;
    }
}

void add_noise(array2D<float> &R, array2D<float> &G, array2D<float> &B,
               const TMatrix &ws, int strength, int coarseness, double scale,
               Channel chan, bool multithread, int oX, int oY, int fW, int fH)
{
    BENCHFUN

    const int W = R.width();
    const int H = R.height();

    const float sf =
        LIM01(float(strength) / (chan == Channel::L ? 200.f : 100.f)) / scale;
    const float radius = (0.5f + 1.75f * float(coarseness) / 100.f) / scale;

    RandomNumberGenerator rng(42 + int(chan) + coarseness + oY * fW + oX);

    array2D<float> kernel;
    {
        const int sz = int(std::ceil(radius)) * 2 + 1;
        kernel(sz, sz);
        const int c = sz / 2;
        double totd = 0.0;
        for (int i = 0; i < sz; ++i) {
            for (int j = 0; j < sz; ++j) {
                float r = std::sqrt(SQR(i - c) + SQR(j - c));
                float d = r - radius;
                kernel[i][j] = d < 0.f ? 1.f : std::max(1.f - d, 0.f);
                totd += kernel[i][j];
            }
        }
        const float tot = totd;
        for (int i = 0; i < kernel.height(); ++i) {
            for (int j = 0; j < kernel.width(); ++j) {
                kernel[i][j] /= tot;
            }
        }
    }
    Convolution conv(kernel, W, H, multithread);

    constexpr uint32_t normd_size = 5233;
    float normd[normd_size];
    NormalDistribution d;
    for (uint32_t i = 0; i < normd_size; ++i) {
        normd[i] = d(rng);
    }

    const auto noise = [&](array2D<float> &a, int chan) -> void {
        constexpr float chan_sd[5] = {1.f, 0.7f, 1.f, 1.3f};
        const float c01 = float(coarseness) / 100.f;
        const float c = 655.35f / (20.f + std::pow(c01, 0.5f) * 80.f);
        const float sd = chan_sd[chan];

        array2D<float> noisebuf(W, H);

#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                float v = a[y][x];
                // float mu = LIM01(v) * c;
                float mu = std::max(v, 0.f) * c;
                float r = normd[rng.randint(normd_size)] * sd;
                float m = mu + sqrtf(mu) * r;
                noisebuf[y][x] = m / c - v;
            }
        }

        conv(noisebuf, noisebuf);

#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                float n = noisebuf[y][x];
                // a[y][x] += sf * n;
                a[y][x] = std::max(a[y][x] + sf * n, 0.f);
            }
        }
    };

    if (chan == Channel::LC) {
        noise(R, 1);
        noise(G, 2);
        noise(B, 3);
    } else if (chan == Channel::L) {
#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                Color::rgb2yuv(R[y][x], G[y][x], B[y][x], G[y][x], R[y][x],
                               B[y][x], ws);
            }
        }

        noise(G, 0);

#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                Color::yuv2rgb(G[y][x], R[y][x], B[y][x], R[y][x], G[y][x],
                               B[y][x], ws);
            }
        }
    } else {
        array2D<float> Y(W, H);
#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                float u, v;
                Color::rgb2yuv(R[y][x], G[y][x], B[y][x], Y[y][x], u, v, ws);
            }
        }
        noise(R, 1);
        noise(G, 2);
        noise(B, 3);
#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                float l, u, v;
                Color::rgb2yuv(R[y][x], G[y][x], B[y][x], l, u, v, ws);
                Color::yuv2rgb(Y[y][x], u, v, R[y][x], G[y][x], B[y][x], ws);
            }
        }
    }
}

// adapted from https://github.com/hotgluebanjo/halation-dctl
void halation(array2D<float> &R, array2D<float> &G, array2D<float> &B, int size,
              float color, bool multithread)
{
    if (size <= 0) {
        return;
    }

    array2D<float> kernel(2 * size + 1, 2 * size + 1);
    float radius = size;

    for (int i = -size, y = 0; i <= size; ++i, ++y) {
        for (int j = -size, x = 0; j <= size; ++j, ++x) {
            float dist = SQR(i) + SQR(j);
            float e = dist == 0.f ? 1.f : 1.f / dist;
            kernel[y][x] =
                e * std::max((radius - std::sqrt(dist)) / radius, 0.f);
        }
    }

    normalize(kernel, compute_norm(kernel));

    const int W = R.width();
    const int H = R.height();

    Convolution conv(kernel, W, H, multithread);
    array2D<float> hR(W, H);
    array2D<float> hG(W, H);
    array2D<float> hB(W, H);
    conv(R, hR);
    conv(G, hG);
    conv(B, hB);

    constexpr float cR = 0.7f;
    const float cG = 1.f - color / 3.f;
    constexpr float cB = 1.f;

    const auto halate = [](float &rgb, float blurred, float color) -> void {
        float halated = (rgb - blurred) * color;
        rgb = halated + blurred;
    };

#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            halate(R[y][x], hR[y][x], cR);
            halate(G[y][x], hG[y][x], cG);
            halate(B[y][x], hB[y][x], cB);
        }
    }
}

void wavelet_smoothing(Imagefloat *rgb,
                       const TMatrix &ws, float strength, int levels,
                       float gamma, double scale, Channel chan,
                       bool multithread,
                       gpu::Context *ctx, gpu::BufferPool *pool)
{
    if (strength <= 0.1f) {
        return;
    }

    MyTime t1p, t2p;
    t1p.set();
    if (gpu::ops::wavelet_smoothing(rgb, ws, strength, levels, gamma, scale,
                                    chan, ctx, pool)) {
        if (settings->verbose) {
            t2p.set();
            std::cout << "wavelet_smoothing: executed on the GPU in "
                      << t2p.etime(t1p) << " usec" << std::endl;
        }
        return;
    }

    const int W = rgb->getWidth();
    const int H = rgb->getHeight();

    array2D<float> R(W, H, rgb->r.ptrs, 0);
    array2D<float> G(W, H, rgb->g.ptrs, 0);
    array2D<float> B(W, H, rgb->b.ptrs, 0);
    
    const int nlevels = std::max(levels - int(std::log2(scale)), 2);
    constexpr float eps = 0.01f;
    const float s = SQR(float(strength) / 125.f * (1.f + strength / 25.f));

    const auto mad = [&](float *coeffs, int len) -> float {
        float med = 0.f;
        AlignedBuffer<float> buf(len);
#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
        for (int i = 0; i < len; ++i) {
            buf.data[i] = std::abs(coeffs[i]);
        }
        findMinMaxPercentile(buf.data, len, 0.5f, med, 0.5f, med, multithread);
        return med / 0.6745f;
    };

    const auto wav = [&](float *data) -> void {
#ifdef _OPENMP
        int nthreads = multithread ? omp_get_num_procs() : 1;
#else
        int nthreads = 1;
#endif
        wavelet_decomposition wd(data, W, H, nlevels, 1, 1, nthreads);
        for (int lvl = 0; lvl < wd.maxlevel(); ++lvl) {
            for (int dir = 1; dir < 4; ++dir) {
                const int lW = wd.level_W(lvl);
                const int lH = wd.level_H(lvl);
                float **coeffs = wd.level_coeffs(lvl);
                float m = SQR(mad(coeffs[dir], lW * lH) * 65535.f);
                float level_factor = m * 5.0 / float(lvl + 1);
                const int n = lW * lH;

#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
                for (int i = 0; i < n; ++i) {
                    float mag = SQR(coeffs[dir][i] * 65535.f);
                    float sf = mag / (mag +
                                      level_factor * s *
                                          xexpf(-mag / (9 * level_factor * s)) +
                                      eps);
                    float f = SQR(sf) / (sf + eps);
                    coeffs[dir][i] *= f;
                }
            }
        }
        wd.reconstruct(data);
    };

    if (gamma > 1.f) {
#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                R[y][x] = pow_F(std::max(R[y][x], 0.f), 1.f / gamma);
                G[y][x] = pow_F(std::max(G[y][x], 0.f), 1.f / gamma);
                B[y][x] = pow_F(std::max(B[y][x], 0.f), 1.f / gamma);
            }
        }
    }

    if (chan == Channel::LC) {
        wav(R);
        wav(G);
        wav(B);
    } else {
#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                Color::rgb2yuv(R[y][x], G[y][x], B[y][x], G[y][x], R[y][x],
                               B[y][x], ws);
            }
        }

        if (chan == Channel::L) {
            wav(G);
        } else {
            wav(R);
            wav(B);
        }

#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                Color::yuv2rgb(G[y][x], R[y][x], B[y][x], R[y][x], G[y][x],
                               B[y][x], ws);
            }
        }
    }

    if (gamma > 1.f) {
#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                rgb->r(y, x) = pow_F(std::max(R[y][x], 0.f), gamma);
                rgb->g(y, x) = pow_F(std::max(G[y][x], 0.f), gamma);
                rgb->b(y, x) = pow_F(std::max(B[y][x], 0.f), gamma);
            }
        }
    } else {
#ifdef _OPENMP
#pragma omp parallel for if (multithread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                rgb->r(y, x) = R[y][x];
                rgb->g(y, x) = G[y][x];
                rgb->b(y, x) = B[y][x];
            }
        }
    }

    if (settings->verbose) {
        t2p.set();
        std::cout << "wavelet_smoothing: executed on the CPU in "
                  << t2p.etime(t1p) << " usec" << std::endl;
    }
}

} // namespace

namespace denoise {

void denoiseGuidedSmoothing(ImProcData &im, Imagefloat *rgb)
{
    if (!im.params->denoise.smoothingEnabled ||
        im.params->denoise.guidedChromaRadius == 0) {
        return;
    }

    rgb->normalizeFloatTo1(im.multiThread);

    const int W = rgb->getWidth();
    const int H = rgb->getHeight();
    array2D<float> R(W, H, rgb->r.ptrs, ARRAY2D_BYREFERENCE);
    array2D<float> G(W, H, rgb->g.ptrs, ARRAY2D_BYREFERENCE);
    array2D<float> B(W, H, rgb->b.ptrs, ARRAY2D_BYREFERENCE);

    TMatrix ws = ICCStore::getInstance()->workingSpaceMatrix(
        im.params->icm.workingProfile);
    TMatrix iws = ICCStore::getInstance()->workingSpaceInverseMatrix(
        im.params->icm.workingProfile);

    const float c_eps = 0.001f;

    guided_smoothing(R, G, B, ws, iws, Channel::C,
                     im.params->denoise.guidedChromaRadius, c_eps, im.scale,
                     im.multiThread);

    rgb->normalizeFloatTo65535(im.multiThread);
}

} // namespace denoise

bool ImProcFunctions::guidedSmoothing(Imagefloat *rgb)
{
    PlanarWhateverData<float> *editWhatever = nullptr;
    EditUniqueID eid = pipetteBuffer ? pipetteBuffer->getEditID() : EUID_None;

    if ((eid == EUID_Masks_H3 || eid == EUID_Masks_C3 ||
         eid == EUID_Masks_L3) &&
        pipetteBuffer->getDataProvider()
                ->getCurrSubscriber()
                ->getPipetteBufferType() == BT_SINGLEPLANE_FLOAT) {
        editWhatever = pipetteBuffer->getSinglePlaneBuffer();
    }

    if (eid == EUID_Masks_DE3) {
        if (getDeltaEColor(rgb, deltaE.x, deltaE.y, offset_x, offset_y,
                           full_width, full_height, scale, deltaE.L, deltaE.C,
                           deltaE.H)) {
            deltaE.ok = true;
        }
    }

    if (params->smoothing.enabled) {
        if (editWhatever) {
            MasksEditID id = static_cast<MasksEditID>(int(eid) - EUID_Masks_H3);
            fillPipetteMasks(rgb, editWhatever, id, multiThread);
        }

        int n = params->smoothing.regions.size();
        int show_mask_idx = params->smoothing.showMask;
        if (show_mask_idx >= n ||
            (cur_pipeline !=
             Pipeline::PREVIEW /*&& cur_pipeline != Pipeline::OUTPUT*/)) {
            show_mask_idx = -1;
        }
        std::vector<array2D<float>> mask(n);
        if (!generateMasks(
                rgb, "smoothing", linked_mask_mgr_, params->smoothing.masks,
                offset_x, offset_y, full_width, full_height, scale, multiThread,
                show_mask_idx, nullptr, &mask,
                cur_pipeline == Pipeline::NAVIGATOR ? plistener : nullptr)) {
            return true; // show mask is active, nothing more to do
        }

        const int W = rgb->getWidth();
        const int H = rgb->getHeight();

        Imagefloat working;
        rgb->setMode(Imagefloat::Mode::RGB, multiThread);
        rgb->normalizeFloatTo1(multiThread);

        TMatrix ws = ICCStore::getInstance()->workingSpaceMatrix(
            params->icm.workingProfile);
        TMatrix iws = ICCStore::getInstance()->workingSpaceInverseMatrix(
            params->icm.workingProfile);

        for (int i = 0; i < n; ++i) {
            if (!params->smoothing.masks[i].enabled) {
                continue;
            }

            int min_x, min_y, max_x, max_y;
            const auto &blend = mask[i];

            find_region(blend, min_x, min_y, max_x, max_y);
            int ww = max_x - min_x;
            int hh = max_y - min_y;

            auto &r = params->smoothing.regions[i];

            if (ww * hh < (W * H) / 2 &&
                !(r.mode == SmoothingParams::Region::Mode::LENS ||
                  r.mode == SmoothingParams::Region::Mode::MOTION)) {
                working.allocate(ww, hh);
#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif
                for (int y = min_y; y < max_y; ++y) {
                    int yy = y - min_y;
                    for (int x = min_x; x < max_x; ++x) {
                        int xx = x - min_x;
                        working.r(yy, xx) = rgb->r(y, x);
                        working.g(yy, xx) = rgb->g(y, x);
                        working.b(yy, xx) = rgb->b(y, x);
                    }
                }
            } else {
                min_x = 0;
                min_y = 0;
                max_x = W;
                max_y = H;
                ww = W;
                hh = H;

                rgb->copyTo(&working);
            }

            if (r.mode == SmoothingParams::Region::Mode::NLMEANS) {
                nlmeans_smoothing(&working, ws, iws, Channel(int(r.channel)),
                                  r.nlstrength,
                                  r.nldetail, r.iterations, scale, multiThread,
                                  getGPUContext(), getGPUPool());
            } else if (r.mode == SmoothingParams::Region::Mode::WAVELETS) {
                wavelet_smoothing(&working, ws, r.wav_strength, r.wav_levels,
                                  r.wav_gamma, scale, Channel(int(r.channel)),
                                  multiThread,
                                  getGPUContext(), getGPUPool());
            } else {
                const int flags = ARRAY2D_BYREFERENCE;
                array2D<float> R(ww, hh, working.r.ptrs, flags);
                array2D<float> G(ww, hh, working.g.ptrs, flags);
                array2D<float> B(ww, hh, working.b.ptrs, flags);

                const bool glow =
                    r.mode == SmoothingParams::Region::Mode::GAUSSIAN_GLOW;
                Channel ch = glow ? Channel::LC : Channel(int(r.channel));
                if (r.mode == SmoothingParams::Region::Mode::NOISE) {
                    if (cur_pipeline == Pipeline::OUTPUT ||
                        cur_pipeline == Pipeline::PREVIEW) {
                        add_noise(R, G, B, ws, r.noise_strength, r.noise_coarseness,
                                  scale, ch, multiThread,
                                  offset_x, offset_y, full_width, full_height);
                    }
                // } else if (r.mode == SmoothingParams::Region::Mode::NLMEANS) {
                //     nlmeans_smoothing(R, G, B, ws, iws, ch, r.nlstrength,
                //                       r.nldetail, r.iterations, scale, multiThread);
                } else if (r.mode == SmoothingParams::Region::Mode::LENS ||
                           r.mode == SmoothingParams::Region::Mode::MOTION) {
                    ImProcData im(params, scale, multiThread);
                    lens_motion_blur(im, &working, blend, i);
                } else if (r.mode == SmoothingParams::Region::Mode::HALATION) {
                    halation(R, G, B, 50 * r.halation_size / scale,
                             LIM01(r.halation_color + 0.5), multiThread);
//                 } else if (r.mode == SmoothingParams::Region::Mode::WAVELETS) {
//                     wavelet_smoothing(R, G, B, ws, r.wav_strength, r.wav_levels,
//                                       r.wav_gamma, scale, ch, multiThread);
// #ifdef _OPENMP
// #pragma omp parallel for if (multiThread)
// #endif
//                     for (int y = 0; y < R.height(); ++y) {
//                         for (int x = 0; x < R.width(); ++x) {
//                             working.r(y, x) = R[y][x];
//                             working.g(y, x) = G[y][x];
//                             working.b(y, x) = B[y][x];
//                         }
//                     }
                } else if (r.mode != SmoothingParams::Region::Mode::GUIDED) {
                    AlignedBuffer<float> buf(ww * hh);
                    double sigma = r.sigma;
                    for (int i = 0; i < r.iterations; ++i) {
                        gaussian_smoothing(R, G, B, buf.data, ws, ch, sigma, scale,
                                           multiThread);
                        if (glow) {
                            sigma *= 1.5;
                            float f = pow_F(r.falloff, i);
                            float f2 = 1.f + 1.f / f;

#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif
                            for (int y = min_y; y < max_y; ++y) {
                                int yy = y - min_y;
                                for (int x = min_x; x < max_x; ++x) {
                                    int xx = x - min_x;
                                    float &r = R[yy][xx];
                                    float &g = G[yy][xx];
                                    float &b = B[yy][xx];
                                    r = (rgb->r(y, x) + r / f) / f2;
                                    g = (rgb->g(y, x) + g / f) / f2;
                                    b = (rgb->b(y, x) + b / f) / f2;
                                }
                            }
                        }
                    }
                } else {
                    const float epsilon =
                        std::max(0.001f * std::pow(2, -r.epsilon), 1e-6);
                    int radius = r.radius;
                    for (int i = 0; i < r.iterations; ++i) {
                        guided_smoothing(R, G, B, ws, iws, ch, radius, epsilon,
                                         scale, multiThread);
                    }
                }
            }

#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif
            for (int y = min_y; y < max_y; ++y) {
                int yy = y - min_y;
                for (int x = min_x; x < max_x; ++x) {
                    int xx = x - min_x;
                    float r = rgb->r(y, x);
                    float g = rgb->g(y, x);
                    float b = rgb->b(y, x);
                    float wr = working.r(yy, xx);
                    float wg = working.g(yy, xx);
                    float wb = working.b(yy, xx);
                    rgb->r(y, x) = intp(blend[y][x], wr, r);
                    rgb->g(y, x) = intp(blend[y][x], wg, g);
                    rgb->b(y, x) = intp(blend[y][x], wb, b);
                }
            }
        }

        rgb->normalizeFloatTo65535(multiThread);
    } else if (editWhatever) {
        editWhatever->fill(0.f);
    }

    return false;
}

} // namespace rtengine


#ifdef ART_USE_VULKAN

#include "gpu/gpu.h"
#include "gpu/vk_pass.h"
#include "gpu/plane_io.h"
#include "gpu/ops.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <mutex>

namespace rtengine { namespace gpu { namespace ops {

namespace {

/* Decimation only happens once, at level 0's FIR step -- the a-trous Haar
 * levels never resample -- so every level's hi1/hi2/hi3 subband is the same
 * w x h size, and WaveletBandsGPU packs all `levels` of one direction into
 * one buffer (ops_internal.h). That means the per-subband MAD-estimation
 * work below (reduce max, histogram, shrink) can run as one dispatch across
 * every level of a direction, instead of one dispatch per (level,
 * direction) pair -- reduceMaxAbsFusedDispatch/histAbsFusedDispatch/
 * waveletShrinkFusedDispatch below are the fused counterparts of what used
 * to be per-subband reduceMaxAbsDispatch/histAbsDispatch/
 * waveletShrinkSubband calls, each looped over all nlevels*3 subbands
 * individually. */
constexpr int kMaxFusedLevels = 8; // matches smoothing.cc's wav_levels Adjuster (2-8)
constexpr unsigned kHistBuckets = 4096;

/* Split into a dispatch half (recorded into the caller's shared Pass, no
 * submit) and a finish half (reads back the per-level partial-max buffer
 * once the Pass has been submitted and waited on). One call covers every
 * level of one direction (hi1, hi2 or hi3).
 *
 * Each level is padded to `paddedN` (a multiple of the workgroup size) so
 * that no workgroup ever straddles a level boundary -- required by the
 * shared-memory tree reduction in reduce_max_abs_fused.comp, where every
 * thread in a workgroup must belong to the same level. */
struct FusedReduceJob {
    Buffer dst;
    unsigned int lx = 0, groupsPerLevel = 0;
};

bool reduceMaxAbsFusedDispatch(Context &ctx, Pass &pass, Buffer &band,
                               int levels, size_t n, FusedReduceJob &job)
{
    unsigned int lx = 256;
    if (lx > ctx.caps().max_workgroup_invocations) {
        lx = ctx.caps().max_workgroup_invocations;
    }
    if (lx > ctx.caps().max_workgroup_size[0]) {
        lx = ctx.caps().max_workgroup_size[0];
    }
    if (!lx) {
        return false;
    }
    job.lx = lx;
    job.groupsPerLevel = (unsigned int)((n + lx - 1) / lx);
    const size_t totalGroups = (size_t)job.groupsPerLevel * (size_t)levels;

    job.dst = ctx.createBuffer(totalGroups * sizeof(float), true);
    if (!job.dst.valid()) {
        return false;
    }
    struct { unsigned int n, paddedN, levels; } pc{
        (unsigned)n, job.groupsPerLevel * lx, (unsigned)levels};
    std::vector<Pass::Binding> b;
    b.push_back(Pass::Binding(&band, false));
    b.push_back(Pass::Binding(&job.dst, true));
    const size_t totalElems = (size_t)job.groupsPerLevel * lx * levels;
    return pass.dispatch1D("reduce_max_abs_fused", b, &pc, sizeof(pc),
                          totalElems);
}

bool reduceMaxAbsFusedFinish(Context &ctx, FusedReduceJob &job, int levels,
                             std::vector<float> &outPerLevel)
{
    const size_t totalGroups = (size_t)job.groupsPerLevel * (size_t)levels;
    const size_t totalBytes = totalGroups * sizeof(float);
    std::vector<float> readback(totalGroups);
    if (!downloadFromBuffer(ctx, &ctx.stagingPoolForThisThread(), job.dst, 0,
                            readback.data(), totalBytes)) {
        return false;
    }
    outPerLevel.resize(levels);
    for (int lvl = 0; lvl < levels; ++lvl) {
        float m = 0.f;
        for (unsigned g = 0; g < job.groupsPerLevel; ++g) {
            m = std::max(m, readback[(size_t)lvl * job.groupsPerLevel + g]);
        }
        outPerLevel[lvl] = m;
    }
    return true;
}

/* ipsmoothing.cc's `mad()` lambda (ipsmoothing.cc:776-787): median(abs(x))
 * / 0.6745, approximated via a histogram over [0, max(abs(x))] rather than
 * an exact order statistic (GPU order statistics need a selection
 * algorithm this project has no existing primitive for; a histogram is the
 * idiomatic GPU substitute, in the same accepted-divergence family as
 * every other approximation in this port). */
struct FusedHistJob {
    Buffer hist;
    std::vector<float> maxAbsPerLevel;
};

bool histAbsFusedDispatch(Context &ctx, Pass &pass, Buffer &band, int levels,
                          size_t n, const std::vector<float> &maxAbsPerLevel,
                          FusedHistJob &job)
{
    job.maxAbsPerLevel = maxAbsPerLevel;
    const size_t totalBuckets = (size_t)kHistBuckets * (size_t)levels;
    job.hist = ctx.createBuffer(totalBuckets * sizeof(unsigned), true);
    if (!job.hist.valid()) {
        return false;
    }
    if (job.hist.mapped()) {
        std::memset(job.hist.mapped(), 0, totalBuckets * sizeof(unsigned));
        job.hist.flush(0, totalBuckets * sizeof(unsigned));
    } else {
        /* Discrete GPU, unmapped: zero it on-device instead, through the
         * same Pass (and hence the same barrier tracking) the atomic
         * increments below are recorded into -- see Pass::fillBuffer's own
         * doc comment, which names this exact histogram as its motivating
         * case. */
        if (!pass.fillBuffer(job.hist)) {
            return false;
        }
    }

    struct {
        unsigned int n, numBuckets, levels;
        float invMaxAbs[kMaxFusedLevels];
    } pc{};
    pc.n = (unsigned)n;
    pc.numBuckets = kHistBuckets;
    pc.levels = (unsigned)levels;
    for (int lvl = 0; lvl < levels; ++lvl) {
        pc.invMaxAbs[lvl] =
            maxAbsPerLevel[lvl] > 0.f ? 1.f / maxAbsPerLevel[lvl] : 0.f;
    }
    std::vector<Pass::Binding> b;
    b.push_back(Pass::Binding(&band, false));
    b.push_back(Pass::Binding(&job.hist, true));
    return pass.dispatch1D("wav_hist_abs_fused", b, &pc, sizeof(pc),
                          n * (size_t)levels);
}

bool madEstimateFusedFinish(Context &ctx, FusedHistJob &job, size_t n,
                            std::vector<float> &madOut)
{
    const int levels = (int)job.maxAbsPerLevel.size();
    madOut.resize(levels);
    const size_t histBytes = (size_t)kHistBuckets * levels * sizeof(unsigned);
    std::vector<unsigned> readback((size_t)kHistBuckets * levels);
    if (!downloadFromBuffer(ctx, &ctx.stagingPoolForThisThread(), job.hist, 0,
                            readback.data(), histBytes)) {
        return false;
    }
    const unsigned *h = readback.data();
    const size_t target = n / 2;
    for (int lvl = 0; lvl < levels; ++lvl) {
        if (job.maxAbsPerLevel[lvl] <= 0.f) {
            madOut[lvl] = 0.f;
            continue;
        }
        const unsigned *hb = h + (size_t)lvl * kHistBuckets;
        size_t cum = 0;
        float median = job.maxAbsPerLevel[lvl];
        for (unsigned i = 0; i < kHistBuckets; ++i) {
            cum += hb[i];
            if (cum >= target) {
                median =
                    (float(i) + 0.5f) / float(kHistBuckets) * job.maxAbsPerLevel[lvl];
                break;
            }
        }
        madOut[lvl] = median / 0.6745f;
    }
    return true;
}

bool waveletShrinkFusedDispatch(Pass &pass, Buffer &band, size_t n,
                                int levels,
                                const std::vector<float> &levelFactorS,
                                const std::vector<float> &nineLevelFactorS,
                                float eps)
{
    struct {
        unsigned int n, levels;
        float eps;
        float levelFactorS[kMaxFusedLevels];
        float nineLevelFactorS[kMaxFusedLevels];
    } pc{};
    pc.n = (unsigned)n;
    pc.levels = (unsigned)levels;
    pc.eps = eps;
    for (int lvl = 0; lvl < levels; ++lvl) {
        pc.levelFactorS[lvl] = levelFactorS[lvl];
        pc.nineLevelFactorS[lvl] = nineLevelFactorS[lvl];
    }
    std::vector<Pass::Binding> b;
    b.push_back(Pass::Binding(&band, true));
    return pass.dispatch1D("wav_shrink_fused", b, &pc, sizeof(pc),
                          n * (size_t)levels);
}

/* ipsmoothing.cc's wavelet_smoothing's `wav` lambda (ipsmoothing.cc:789-820):
 * decompose, MAD-based soft-shrink every detail subband at every level,
 * reconstruct in place.
 *
 * Batched into exactly 3 submissions total, each covering all three
 * directions' worth of fused per-level work in one call apiece (down from
 * one submit->wait per primitive dispatch -- ~9 per subband, i.e.
 * ~9*3*nlevels for a whole channel -- which measured as ~85% pure
 * per-submission overhead rather than GPU compute on a 5-level/1200x800
 * test; fusing every level's reduce/hist/shrink into one dispatch per
 * direction on top of that cut per-dispatch overhead further, see the
 * doc comment above). The only reason this can't be one submission is that
 * the CPU genuinely needs two intermediate results back mid-pipeline: each
 * level's maxAbs (to scale its histogram's bucket range) and each level's
 * median (to compute its shrinkage threshold) -- both small per-direction
 * vectors, both cheap to read back, but both real GPU->CPU->GPU round
 * trips. Everything else -- decompose, every direction's reduction,
 * histogram and shrink (each fused across levels), and reconstruct -- has
 * no CPU-side dependency and is batched into the pass that surrounds it:
 *   pass 1: decompose + reduceMaxAbsFused (each direction)  -> read maxAbs
 *   pass 2: histAbsFused (each direction)                    -> read median
 *   pass 3: shrinkFused (each direction) + reconstruct
 */
bool waveletShrink(Context &ctx, Buffer &data, int W, int H, int nlevels,
                   float s, float eps)
{
    if (nlevels > kMaxFusedLevels) {
        logOnce("GPU: wavelet levels exceeds fused-shrink budget; using the "
               "CPU");
        return false;
    }

    WaveletBandsGPU bands;
    Buffer *ll = nullptr;
    int llW, llH;
    /* One pool for the whole op: the decompose block's scratch comes back
     * for the reconstruct block, and callers that run several channels get
     * the allocation amortised across them.  It outlives every Pass below,
     * which is the requirement (nothing may be recycled before a
     * submitAndWait), and nothing here recycles at all. */
    BufferPool pool(ctx);
    FusedReduceJob reduceJobs[3];
    FusedHistJob histJobs[3];
    std::vector<float> maxAbs[3], mad[3];

    {
        Pass pass(ctx, "wavelet:decompose+reduce");
        if (!pass.valid()) {
            return false;
        }
        if (!waveletDecompose(ctx, pass, pool, data, W, H, nlevels, bands,
                              ll, llW, llH)) {
            return false;
        }
        Buffer *dirs[3] = {bands.hi1, bands.hi2, bands.hi3};
        const size_t n = (size_t)bands.w * bands.h;
        for (int d = 0; d < 3; ++d) {
            if (!reduceMaxAbsFusedDispatch(ctx, pass, *dirs[d], nlevels, n,
                                          reduceJobs[d])) {
                return false;
            }
        }
        if (!pass.submitAndWait()) {
            return false;
        }
        pass.reportTimings();
    }
    for (int d = 0; d < 3; ++d) {
        if (!reduceMaxAbsFusedFinish(ctx, reduceJobs[d], nlevels, maxAbs[d])) {
            return false;
        }
    }

    {
        Pass pass(ctx, "wavelet:hist");
        if (!pass.valid()) {
            return false;
        }
        Buffer *dirs[3] = {bands.hi1, bands.hi2, bands.hi3};
        const size_t n = (size_t)bands.w * bands.h;
        for (int d = 0; d < 3; ++d) {
            if (!histAbsFusedDispatch(ctx, pass, *dirs[d], nlevels, n,
                                     maxAbs[d], histJobs[d])) {
                return false;
            }
        }
        if (!pass.submitAndWait()) {
            return false;
        }
        pass.reportTimings();
    }
    for (int d = 0; d < 3; ++d) {
        if (!madEstimateFusedFinish(ctx, histJobs[d], (size_t)bands.w * bands.h,
                                    mad[d])) {
            return false;
        }
    }

    {
        Pass pass(ctx, "wavelet:shrink+reconstruct");
        if (!pass.valid()) {
            return false;
        }
        Buffer *dirs[3] = {bands.hi1, bands.hi2, bands.hi3};
        const size_t n = (size_t)bands.w * bands.h;
        for (int d = 0; d < 3; ++d) {
            std::vector<float> levelFactorS(nlevels), nineLevelFactorS(nlevels);
            for (int lvl = 0; lvl < nlevels; ++lvl) {
                const float m = (mad[d][lvl] * 65535.f) * (mad[d][lvl] * 65535.f);
                const float levelFactor = m * 5.f / float(lvl + 1);
                levelFactorS[lvl] = levelFactor * s;
                nineLevelFactorS[lvl] = 9.f * levelFactor * s;
            }
            if (!waveletShrinkFusedDispatch(pass, *dirs[d], n, nlevels,
                                           levelFactorS, nineLevelFactorS,
                                           eps)) {
                return false;
            }
        }
        if (!waveletReconstruct(ctx, pass, pool, bands, ll, llW, llH, data,
                                W, H)) {
            return false;
        }
        if (!pass.submitAndWait()) {
            return false;
        }
        pass.reportTimings();
    }

    return true;
}


bool waveletGamma(Pass &pass, Buffer &buf, int W, int H, float exponent)
{
    struct { unsigned int w, h; float exponent; } pc{(unsigned)W, (unsigned)H,
                                                     exponent};
    std::vector<Pass::Binding> b;
    b.push_back(Pass::Binding(&buf, true));
    return pass.dispatch2D("wav_gamma", b, &pc, sizeof(pc), W, H);
}


bool waveletRgb2Yuv(Pass &pass, Buffer &R, Buffer &G, Buffer &B, int W,
                    int H, const TMatrix &ws)
{
    return rgb2yuv(pass, R, G, B, W, H, ws, G, R, B);
}

bool waveletYuv2Rgb(Pass &pass, Buffer &R, Buffer &G, Buffer &B, int W,
                    int H, const TMatrix &ws)
{
    return yuv2rgb(pass, G, R, B, W, H, ws, R, G, B);
}


const unsigned int IMAGETOPLANES_DISPATCHES = 3;
const unsigned int GAMMA_DISPATCHES = 1;
const unsigned int RGB2YUV_DISPATCHES = 1;
const unsigned int RGBLUMINANCE_DISPATCHES = 1;
const unsigned int YUVRECOMBINE_DISPATCHES = 1;

bool imageToPlanes(Pass &pass, Imagefloat *rgb, Buffer *R, Buffer *G,
                   Buffer *B, bool from_image)
{
    if (!rgb || !R || !G || !B) {
        return false;
    }

    const int W = rgb->getWidth();
    const int H = rgb->getHeight();

    const size_t planeStride = rgb->getPlaneStride();
    const int strideFloats = int(rgb->getRowStride() / sizeof(float));

    Buffer *resid = rgb->residency().forWrite();
    if (!resid) {
        return false;
    }

    return transferPlane(pass, *R, *resid, 0, planeStride, W, H,
                         strideFloats, !from_image, 1.f) &&
           transferPlane(pass, *G, *resid, planeStride, planeStride, W, H,
                         strideFloats, !from_image, 1.f) &&
           transferPlane(pass, *B, *resid, 2 * planeStride, planeStride, W,
                         H, strideFloats, !from_image, 1.f);
}

} // namespace

bool wavelet_smoothing(Imagefloat *rgb,
                       const TMatrix &ws, float strength, int levels,
                       float gamma, double scale, Channel chan,
                       Context *ctx, BufferPool *pool)
{
    if (!ctx || !pool || !opEnabled("smoothing") || !available()) {
        return false;
    }

    const int W = rgb->getWidth();
    const int H = rgb->getHeight();
    const size_t bytes = size_t(W) * H * sizeof(float);
    
    Buffer *resid = rgb->residency().forWrite();
    if (!resid) {
        return false;
    }

    PoolScope scope(*pool);

    Buffer *wRp = pool->get(bytes);
    Buffer *wGp = pool->get(bytes);
    Buffer *wBp = pool->get(bytes);
    
    if (!wRp || !wGp || !wBp) {
        logOnce("GPU: wavelet_smoothing buffer allocation failed; using the CPU");
        return false;
    }

    /* The plane split, the forward gamma and the RGB->YUV rotation have no
     * CPU step between them, so they go into one chain instead of five
     * submissions.  waveletShrink below genuinely cannot join them: it reads
     * maxAbs and the MAD median back to the host mid-algorithm, which is a
     * real GPU->CPU->GPU dependency, not batching laziness. */
    PassSeq seq(*ctx, "wavelet_smoothing");

    {
        Pass *ps = seq.reserve(IMAGETOPLANES_DISPATCHES);
        if (!ps || !imageToPlanes(*ps, rgb, wRp, wGp, wBp, true)) {
            return false;
        }
    }

    const int nlevels = std::max(levels - (int)std::log2(scale), 2);
    const float s = (strength / 125.f * (1.f + strength / 25.f)) *
                    (strength / 125.f * (1.f + strength / 25.f));
    constexpr float eps = 0.01f;

    auto &wR = *wRp;
    auto &wG = *wGp;
    auto &wB = *wBp;

    if (gamma > 1.f) {
        Pass *ps = seq.reserve(3 * GAMMA_DISPATCHES);
        if (!ps || !waveletGamma(*ps, wR, W, H, 1.f / gamma) ||
            !waveletGamma(*ps, wG, W, H, 1.f / gamma) ||
            !waveletGamma(*ps, wB, W, H, 1.f / gamma)) {
            return false;
        }
    }

    if (chan == Channel::LC) { 
        if (!waveletShrink(*ctx, wR, W, H, nlevels, s, eps) ||
            !waveletShrink(*ctx, wG, W, H, nlevels, s, eps) ||
            !waveletShrink(*ctx, wB, W, H, nlevels, s, eps)) {
            return false;
        }
    } else {
        Pass *ps = seq.reserve(RGB2YUV_DISPATCHES);
        if (!ps || !waveletRgb2Yuv(*ps, wR, wG, wB, W, H, ws)) {
            return false;
        }
        /* waveletShrink opens its own Passes, so everything recorded above
         * has to be on the queue before it runs. */
        if (!seq.flush()) {
            return false;
        }
        if (chan == Channel::L) {
            if (!waveletShrink(*ctx, wG, W, H, nlevels, s, eps)) {
                return false;
            }
        } else { // Channel::C
            if (!waveletShrink(*ctx, wR, W, H, nlevels, s, eps) ||
                !waveletShrink(*ctx, wB, W, H, nlevels, s, eps)) {
                return false;
            }
        }
        Pass *pe = seq.reserve(RGB2YUV_DISPATCHES);
        if (!pe || !waveletYuv2Rgb(*pe, wR, wG, wB, W, H, ws)) {
            return false;
        }
    }

    if (gamma > 1.f) {
        Pass *ps = seq.reserve(3 * GAMMA_DISPATCHES);
        if (!ps || !waveletGamma(*ps, wR, W, H, gamma) ||
            !waveletGamma(*ps, wG, W, H, gamma) ||
            !waveletGamma(*ps, wB, W, H, gamma)) {
            return false;
        }
    }

    {
        Pass *ps = seq.reserve(IMAGETOPLANES_DISPATCHES);
        if (!ps || !imageToPlanes(*ps, rgb, wRp, wGp, wBp, false)) {
            return false;
        }
    }
    if (!seq.flush()) {
        return false;
    }
    
    if (settings->verbose) {
        std::cout << "wavelet_smoothing executing on the GPU" << std::endl;
    }

    rgb->syncCpu();
    return true;
}


bool nlmeans_smoothing(Imagefloat *rgb,
                       const TMatrix &ws, const TMatrix &iws, Channel chan,
                       int strength, int detail, int iterations, double scale,
                       Context *ctx, BufferPool *pool)
{
    if (!ctx || !pool || !opEnabled("smoothing") || !available()) {
        return false;
    }

    const int W = rgb->getWidth();
    const int H = rgb->getHeight();
    const size_t bytes = size_t(W) * H * sizeof(float);
    
    PoolScope scope(*pool);

    Buffer *wRp = pool->get(bytes);
    Buffer *wGp = pool->get(bytes);
    Buffer *wBp = pool->get(bytes);
    if (!wRp || !wGp || !wBp) {
        logOnce("GPU: guidedSmoothing allocation failed; using the CPU");
        return false;
    }

    /* Same shape as wavelet_smoothing: the plane split and the luminance
     * extraction that follows it are one chain, and NLMeans in between is
     * what forces a boundary. */
    PassSeq seq(*ctx, "nlmeans_smoothing");
    {
        Pass *ps = seq.reserve(IMAGETOPLANES_DISPATCHES);
        if (!ps || !imageToPlanes(*ps, rgb, wRp, wGp, wBp, true)) {
            return false;
        }
    }

    auto &wR = *wRp;
    auto &wG = *wGp;
    auto &wB = *wBp;
    
    const auto runPlaneNlmeans = [&](Buffer &plane) -> bool {
        for (int it = 0; it < iterations; ++it) {
            if (!NLMeans(*ctx, *pool, plane, W, H, 1.f, scale,
                         strength, detail)) {
                return false;
            }
        }
        return true;
    };

    if (chan == Channel::L) { 
        Buffer *yBufp = pool->get(bytes);
        if (!yBufp) {
            return false;
        }
        Buffer &yBuf = *yBufp;
        Pass *ps = seq.reserve(RGBLUMINANCE_DISPATCHES);
        // NLMeans opens its own Passes, so flush before handing over
        if (!ps || !rgbLuminance(*ps, wR, wG, wB, W, H, ws, yBuf) ||
            !seq.flush()) {
            return false;
        }
        if (!runPlaneNlmeans(yBuf)) {
            return false;
        }
        Pass *pr = seq.reserve(YUVRECOMBINE_DISPATCHES);
        if (!pr || !yuvRecombine(*pr, yBuf, wR, wG, wB, W, H, false, ws, wR,
                                 wG, wB)) {
            return false;
        }
    } else {
        Buffer *origYp = nullptr;
        if (chan == Channel::C) { // Channel::C
            origYp = pool->get(bytes);
            if (!origYp) {
                return false;
            }
            Pass *ps = seq.reserve(RGBLUMINANCE_DISPATCHES);
            if (!ps || !rgbLuminance(*ps, wR, wG, wB, W, H, ws, *origYp)) {
                return false;
            }
        }
        if (!seq.flush()) {
            return false;
        }
        if (!runPlaneNlmeans(wR) || !runPlaneNlmeans(wG) ||
            !runPlaneNlmeans(wB)) {
            return false;
        }
        if (chan == Channel::C) {
            Pass *pr = seq.reserve(YUVRECOMBINE_DISPATCHES);
            if (!pr || !yuvRecombine(*pr, *origYp, wR, wG, wB, W, H, false, ws,
                                     wR, wG, wB)) {
                return false;
            }
        }
    }

    {
        Pass *ps = seq.reserve(IMAGETOPLANES_DISPATCHES);
        if (!ps || !imageToPlanes(*ps, rgb, wRp, wGp, wBp, false)) {
            return false;
        }
    }
    if (!seq.flush()) {
        return false;
    }

    if (settings->verbose) {
        std::cout << "nlmeans_smoothing executing on the GPU" << std::endl;
    }
    
    rgb->syncCpu();
    return true;
}


}}} // namespace rtengine::gpu::ops

#else // !ART_USE_VULKAN

namespace rtengine { namespace gpu { namespace ops {

bool wavelet_smoothing(Imagefloat *rgb,
                       const TMatrix &ws, float strength, int levels,
                       float gamma, double scale, Channel chan,
                       Context *ctx, BufferPool *pool)
{
    return false;
}

bool nlmeans_smoothing(Imagefloat *rgb,
                       const TMatrix &ws, const TMatrix &iws, Channel chan,
                       int strength, int detail, int iterations, double scale,
                       Context *ctx, BufferPool *pool)
{
    return false;
}

}}} // namespace rtengine::gpu::ops

#endif // ART_USE_VULKAN

