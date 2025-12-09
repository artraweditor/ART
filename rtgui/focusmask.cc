/* -*- C++ -*-
 *
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2004-2010 Gabor Horvath <hgabor@rawtherapee.com>
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

#include "../rtengine/rt_math.h"
#include <cairomm/cairomm.h>
#include <gtkmm.h>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

void addFocusMask(const unsigned char *src, unsigned char *dst, int W, int H,
                  int src_stride, int dst_stride, int src_offset,
                  int dst_offset)
{
    using rtengine::SQR;
    auto pix = dst;
    auto pixWrkSpace = const_cast<unsigned char *>(src);

    const int pixRowStride = dst_stride;
    const int pixWSRowStride = src_stride;

    const int bHeight = H;
    const int bWidth = W;

    const int chans = pixRowStride / bWidth;

    const int blur_radius2 = 1; // radius of small kernel. 1 => 3x3 kernel
    const int blur_dim2 = 2 * blur_radius2 + 1; // dimension of small kernel
    const int blur_radius = (blur_dim2 * blur_dim2) / 2; // radius of big kernel
    const float kernel_size =
        SQR(2.f * blur_radius + 1.f); // count of pixels in the big blur kernel
    const float rkernel_size =
        1.0f / kernel_size; // reciprocal of kernel_size to avoid divisions
    const float kernel_size2 = SQR(
        2.f * blur_radius2 + 1.f); // count of pixels in the small blur kernel
    const float rkernel_size2 =
        1.0f / kernel_size2; // reciprocal of kernel_size to avoid divisions

    // aloocate buffer for precalculated Luminance
    const size_t bufsz = bHeight * bWidth;
    std::vector<float> tmpLv(bufsz), tmpLsumv(bufsz), tmpLsumSqv(bufsz),
        tmpstdDev2v(bufsz);
    ;
    // float* tmpL = (float*)malloc(bHeight * bWidth * sizeof(float) );
    // // aloocate buffers for sums and sums of squares of small kernel
    // float* tmpLsum = (float*)malloc((bHeight) * (bWidth) * sizeof(float) );
    // float* tmpLsumSq = (float*)malloc((bHeight) * (bWidth) * sizeof(float) );
    // float* tmpstdDev2 = (float*)malloc((bHeight) * (bWidth) * sizeof(float)
    // );
    float *tmpL = &tmpLv[0];
    float *tmpLsum = &tmpLsumv[0];
    float *tmpLsumSq = &tmpLsumSqv[0];
    float *tmpstdDev2 = &tmpstdDev2v[0];
    float maxstdDev_L2 = 0.f;

#ifdef _OPENMP
#pragma omp parallel
#endif
    {
#ifdef _OPENMP
#pragma omp for
#endif

        // precalculate Luminance
        for (int i = 0; i < bHeight; i++) {
            guint8 *currWS = pixWrkSpace + i * pixWSRowStride;
            float *currL = tmpL + i * bWidth;

            for (int j = 0; j < bWidth; j++) {
                *currL = 0.299f * (currWS)[0] + 0.587f * (currWS)[1] +
                         0.114f * (currWS)[2];
                currL++;
                currWS += chans; // 3;
            }
        }

        float maxthrstdDev_L2 = 0.f;
#ifdef _OPENMP
#pragma omp for nowait
#endif

        // precalculate sum and sum of squares of small kernel
        for (int i = blur_radius2; i < bHeight - blur_radius2; i++) {
            for (int j = blur_radius2; j < bWidth - blur_radius2; j++) {
                float sumL = 0.f;
                float sumLSqu = 0.f;

                for (int kh = -blur_radius2; kh <= blur_radius2; kh++) {
                    for (int kw = -blur_radius2; kw <= blur_radius2; kw++) {
                        float curL = tmpL[(i + kh) * bWidth + j + kw];
                        sumL += curL;
                        sumLSqu += SQR(curL);
                    }
                }

                tmpLsum[i * bWidth + j] = sumL;
                tmpLsumSq[i * bWidth + j] = sumLSqu;
                float stdDev_L2 =
                    rkernel_size2 * sqrtf(sumLSqu * kernel_size2 - sumL * sumL);

                if (stdDev_L2 > maxthrstdDev_L2) {
                    maxthrstdDev_L2 = stdDev_L2;
                }

                tmpstdDev2[i * bWidth + j] = stdDev_L2;
            }
        }

#ifdef _OPENMP
#pragma omp critical
#endif
        {
            if (maxthrstdDev_L2 > maxstdDev_L2) {
                maxstdDev_L2 = maxthrstdDev_L2;
            }
        }
    }

    const float focus_thresh = 80.f;
    maxstdDev_L2 = std::min(maxstdDev_L2, focus_thresh);
    const float focus_threshby10 = focus_thresh / 10.f;
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic, 16)
#endif

    for (int i = blur_radius + 1; i < bHeight - blur_radius; i++) {
        guint8 *curr = pix + i * pixRowStride + 3 * (blur_radius + 1);
        guint8 *currWs =
            pixWrkSpace + i * pixWSRowStride + 3 * (blur_radius + 1);

        for (int j = blur_radius + 1; j < bWidth - blur_radius; j++) {

            //*************
            // Copyright (c) 2011 Michael Ezra michael@michaelezra.com
            // determine if pixel is in the sharp area of the image using
            // standard deviation analysis on two different scales
            // float focus_thresh2;
            // float opacity = 0.9;//TODO: implement opacity
            // TODO: evaluate effects of altering sampling frequency

            // TODO: dynamically determine appropriate values based on image
            // analysis

            // calculate average in +-blur_radius pixels area around the current
            // pixel speed up: calculate sum of squares in the same loops

            float sum_L = 0.f;
            float sumsq_L = 0.f;

            // use precalculated values of small kernel to reduce number of
            // iterations
            for (int kh = -blur_radius + blur_radius2;
                 kh <= blur_radius - blur_radius2; kh += blur_dim2) {
                float *currLsum =
                    &tmpLsum[(i + kh) * bWidth + j - blur_radius + 1];
                float *currLsumSqu =
                    &tmpLsumSq[(i + kh) * bWidth + j - blur_radius + 1];

                for (int k = -blur_radius + blur_radius2;
                     k <= blur_radius - blur_radius2; k += blur_dim2,
                         currLsum += blur_dim2, currLsumSqu += blur_dim2) {
                    sum_L += *currLsum;
                    sumsq_L += *currLsumSqu;
                }
            }

            // float sum_L2 = tmpLsum[i * bWidth + j];
            // float sumsq_L2 = tmpLsumSq[i * bWidth + j];
            //*************
            //  averages
            //  Optimized formulas to avoid divisions
            float stdDev_L =
                rkernel_size * sqrtf(sumsq_L * kernel_size - sum_L * sum_L);
            float stdDev_L2 = tmpstdDev2[i * bWidth + j];
            //                          float stdDev_L2 = rkernel_size2 *
            //                          sqrtf(sumsq_L2 * kernel_size2 - sum_L2 *
            //                          sum_L2);

            // TODO: try to normalize by average L of the entire (preview) image

            // detection method 1: detect focus in features
            // there is no strict condition between stdDev_L and stdDev_L2
            // themselves
            /*                                if (stdDev_L2>focus_thresh2
                                              && (stdDev_L <focus_thresh)){ //
               this excludes false positives due to high contrast edges

                                              curr[1]=255;
                                              curr[0]=0;
                                              curr[2]=0;

                                              }*/

            // detection method 2: detect focus in texture
            //  key point is std deviation on lower scale is higher than for the
            //  larger scale plus some boundary conditions
            if (focus_thresh >=
                    stdDev_L2 // TODO: could vary this to bypass noise better
                && stdDev_L2 > stdDev_L // this is the key to select fine detail
                                        // within lower contrast on larger scale
                && stdDev_L > focus_threshby10 // options.highlightThreshold
            ) {
                // transpareny depends on sdtDev_L2 and maxstdDev_L2
                float transparency =
                    1.f - std::min(stdDev_L2 / maxstdDev_L2, 1.0f);
                // first row of circle
                guint8 *currtmp = &curr[0] + (-3 * pixRowStride);
                guint8 *currtmpWS = &currWs[0] + (-3 * pixWSRowStride);

                for (int jj = -chans; jj <= chans; jj += chans) {
                    guint8 *currtmpl = currtmp + jj + dst_offset;
                    guint8 *currtmpWSl = currtmpWS + jj + src_offset;
                    // transparent green
                    currtmpl[0] = transparency * currtmpWSl[0];
                    currtmpl[1] = transparency * currtmpWSl[1] +
                                  (1.f - transparency) * 255.f;
                    currtmpl[2] = transparency * currtmpWSl[2];
                }

                // second row of circle
                currtmp = &curr[0] + (-2 * pixRowStride);
                currtmpWS = &currWs[0] + (-2 * pixWSRowStride);

                for (int jj = -2 * chans; jj <= 2 * chans; jj += chans) {
                    guint8 *currtmpl = currtmp + jj + dst_offset;
                    guint8 *currtmpWSl = currtmpWS + jj + src_offset;
                    // transparent green
                    currtmpl[0] = transparency * currtmpWSl[0];
                    currtmpl[1] = transparency * currtmpWSl[1] +
                                  (1.f - transparency) * 255.f;
                    currtmpl[2] = transparency * currtmpWSl[2];
                }

                // three middle row of circle
                for (int ii = -1; ii <= 1; ii++) {
                    currtmp = &curr[0] + (ii * pixRowStride);
                    currtmpWS = &currWs[0] + (ii * pixWSRowStride);

                    for (int jj = -3 * chans; jj <= 3 * chans; jj += chans) {
                        guint8 *currtmpl = currtmp + jj + dst_offset;
                        guint8 *currtmpWSl = currtmpWS + jj + src_offset;
                        // transparent green
                        currtmpl[0] = transparency * currtmpWSl[0];
                        currtmpl[1] = transparency * currtmpWSl[1] +
                                      (1.f - transparency) * 255.f;
                        currtmpl[2] = transparency * currtmpWSl[2];
                    }
                }

                // second last row of circle
                currtmp = &curr[0] + (2 * pixRowStride);
                currtmpWS = &currWs[0] + (2 * pixWSRowStride);

                for (int jj = -2 * chans; jj <= 2 * chans; jj += chans) {
                    guint8 *currtmpl = currtmp + jj + dst_offset;
                    guint8 *currtmpWSl = currtmpWS + jj + src_offset;
                    // transparent green
                    currtmpl[0] = transparency * currtmpWSl[0];
                    currtmpl[1] = transparency * currtmpWSl[1] +
                                  (1.f - transparency) * 255.f;
                    currtmpl[2] = transparency * currtmpWSl[2];
                }

                // last row of circle
                currtmp = &curr[0] + (3 * pixRowStride);
                currtmpWS = &currWs[0] + (3 * pixWSRowStride);

                for (int jj = -chans; jj <= chans; jj += chans) {
                    guint8 *currtmpl = currtmp + jj + dst_offset;
                    guint8 *currtmpWSl = currtmpWS + jj + src_offset;
                    // transparent green
                    currtmpl[0] = transparency * currtmpWSl[0];
                    currtmpl[1] = transparency * currtmpWSl[1] +
                                  (1.f - transparency) * 255.f;
                    currtmpl[2] = transparency * currtmpWSl[2];
                }
            }

            curr += chans;   // 3;
            currWs += chans; // 3;
        }
    }

    // free(tmpL);
    // free(tmpLsum);
    // free(tmpLsumSq);
    // free(tmpstdDev2);
}
