/*
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
#include <algorithm>
#include <cmath>
#include <cstring>
#include <glib.h>
#include <glib/gstdio.h>
#include <memory>
#include <vector>
#ifdef _OPENMP
#include <omp.h>
#endif

#include "rt_math.h"

#include "LUT.h"
#include "array2D.h"
#include "ciecam02.h"
#include "color.h"
#include "curves.h"
#include "iccstore.h"
#include "linalgebra.h"
#include "mytime.h"
#include "opthelper.h"

#undef CLIPD
#define CLIPD(a) ((a) > 0.0f ? ((a) < 1.0f ? (a) : 1.0f) : 0.0f)

namespace rtengine {

namespace curves {

const std::vector<double> filmcurve_def = {
    DCT_Spline, 0, 0, 0.11, 0.09, 0.32, 0.47, 0.66, 0.87, 1, 1};

} // namespace curves

bool sanitizeCurve(std::vector<double> &curve)
{
    // A curve is valid under one of the following conditions:
    // 1) Curve has exactly one entry which is D(F)CT_Linear
    // 2) Number of curve entries is > 3 and odd
    // 3) curve[0] == DCT_Parametric and curve size is >= 8 and curve[1] ..
    // curve[3] are ordered ascending and are distinct
    if (curve.empty()) {
        curve.push_back(DCT_Linear);
        return true;
    } else if (curve.size() == 1 && curve[0] != DCT_Linear) {
        curve[0] = DCT_Linear;
        return true;
    } else if ((curve.size() % 2 == 0 || curve.size() < 5) &&
               curve[0] != DCT_Parametric) {
        curve.clear();
        curve.push_back(DCT_Linear);
        return true;
    } else if (curve[0] == DCT_Parametric) {
        if (curve.size() < 8) {
            curve.clear();
            curve.push_back(DCT_Linear);
            return true;
        } else {
            // curve[1] to curve[3] must be ordered ascending and distinct
            for (int i = 1; i < 3; i++) {
                if (curve[i] >= curve[i + 1]) {
                    curve[1] = 0.25f;
                    curve[2] = 0.5f;
                    curve[3] = 0.75f;
                    break;
                }
            }
        }
    }
    return false;
}

Curve::Curve()
    : N(0), ppn(0), x(nullptr), y(nullptr), mc(0.0), mfc(0.0), msc(0.0),
      mhc(0.0), hashSize(1000 /* has to be initialized to the maximum value */),
      ypp(nullptr), x1(0.0), y1(0.0), x2(0.0), y2(0.0), x3(0.0), y3(0.0),
      firstPointIncluded(false), increment(0.0), nbr_points(0)
{
}

void Curve::AddPolygons()
{
    if (firstPointIncluded) {
        poly_x.push_back(x1);
        poly_y.push_back(y1);
    }

    for (int k = 1; k < (nbr_points - 1); k++) {
        double t = k * increment;
        double t2 = t * t;
        double tr = 1. - t;
        double tr2 = tr * tr;
        double tr2t = tr * 2 * t;

        // adding a point to the polyline
        poly_x.push_back(tr2 * x1 + tr2t * x2 + t2 * x3);
        poly_y.push_back(tr2 * y1 + tr2t * y2 + t2 * y3);
    }

    // adding the last point of the sub-curve
    poly_x.push_back(x3);
    poly_y.push_back(y3);
}

void Curve::fillDyByDx()
{
    dyByDx.resize(poly_x.size() - 1);

    for (unsigned int i = 0; i < poly_x.size() - 1; i++) {
        double dx = poly_x[i + 1] - poly_x[i];
        double dy = poly_y[i + 1] - poly_y[i];
        dyByDx[i] = dy / dx;
    }
}

void Curve::fillHash()
{
    hash.resize(hashSize + 2);

    unsigned int polyIter = 0;
    double const increment = 1. / hashSize;
    double milestone = 0.;

    for (unsigned short i = 0; i < (hashSize + 1);) {
        while (poly_x[polyIter] <= milestone) {
            ++polyIter;
        }

        hash.at(i).smallerValue = polyIter - 1;
        ++i;
        milestone = i * increment;
    }

    milestone = 0.;
    polyIter = 0;

    for (unsigned int i = 0; i < hashSize + 1u;) {
        while (poly_x[polyIter] < (milestone + increment)) {
            ++polyIter;
        }

        hash.at(i).higherValue = polyIter;
        ++i;
        milestone = i * increment;
    }

    hash.at(hashSize + 1).smallerValue = poly_x.size() - 1;
    hash.at(hashSize + 1).higherValue = poly_x.size();

    /*
     * Uncomment the code below to dump the polygon points and the hash table in
    files if (poly_x.size() > 500) { printf("Files generated (%d points)\n",
    poly_x.size()); FILE* f = fopen ("hash.txt", "wt"); for (unsigned int i=0;
    i<hashSize;i++) { unsigned short s = hash.at(i).smallerValue; unsigned short
    h = hash.at(i).higherValue; fprintf (f, "%d: %d<%d (%.5f<%.5f)\n", i, s, h,
    poly_x[s], poly_x[h]);
        }
        fclose (f);
        f = fopen ("poly_x.txt", "wt");
        for (size_t i=0; i<poly_x.size();i++) {
            fprintf (f, "%d: %.5f, %.5f\n", i, poly_x[i], poly_y[i]);
        }
        fclose (f);
    }
    */
}

/** @ brief Return the number of control points of the curve
 * This method return the number of control points of a curve. Not suitable for
 * parametric curves.
 * @return number of control points of the curve. 0 will be sent back for
 * Parametric curves
 */
int Curve::getSize() const { return N; }

/** @ brief Return the a control point's value
 * This method return a control points' value. Not suitable for parametric
 * curves.
 * @param cpNum id of the control points we're interested in
 * @param x Y value of the control points, or -1 if invalid
 * @param y Y value of the control points, or -1 if invalid
 */
void Curve::getControlPoint(int cpNum, double &x, double &y) const
{
    if (this->x && cpNum < N) {
        x = this->x[cpNum];
        y = this->y[cpNum];
    } else {
        x = y = -1.;
    }
}

//
void ToneCurve::Reset() { lutToneCurve.reset(); }

// Fill a LUT with X/Y, ranged 0xffff
void ToneCurve::Set(const Curve &pCurve, float whitecoeff)
{
    this->whitecoeff = whitecoeff;
    this->curve = &pCurve;
    this->whitept = 65535.f * whitecoeff;
    lutToneCurve(65536);

    for (int i = 0; i < 65536; i++) {
        lutToneCurve[i] = (float)pCurve.getVal(float(i) / 65535.f) * 65535.f;
    }
}

// this is a generic cubic spline implementation, to clean up we could probably
// use something already existing elsewhere
void PerceptualToneCurve::cubic_spline(const float x[], const float y[],
                                       const int len, const float out_x[],
                                       float out_y[], const int out_len)
{
    int i, j;

    float **A = (float **)malloc(2 * len * sizeof(*A));
    float *As = (float *)calloc(1, 2 * len * 2 * len * sizeof(*As));
    float *b = (float *)calloc(1, 2 * len * sizeof(*b));
    float *c = (float *)calloc(1, 2 * len * sizeof(*c));
    float *d = (float *)calloc(1, 2 * len * sizeof(*d));

    for (i = 0; i < 2 * len; i++) {
        A[i] = &As[2 * len * i];
    }

    for (i = len - 1; i > 0; i--) {
        b[i] = (y[i] - y[i - 1]) / (x[i] - x[i - 1]);
        d[i - 1] = x[i] - x[i - 1];
    }

    for (i = 1; i < len - 1; i++) {
        A[i][i] = 2 * (d[i - 1] + d[i]);

        if (i > 1) {
            A[i][i - 1] = d[i - 1];
            A[i - 1][i] = d[i - 1];
        }

        A[i][len - 1] = 6 * (b[i + 1] - b[i]);
    }

    for (i = 1; i < len - 2; i++) {
        float v = A[i + 1][i] / A[i][i];

        for (j = 1; j <= len - 1; j++) {
            A[i + 1][j] -= v * A[i][j];
        }
    }

    for (i = len - 2; i > 0; i--) {
        float acc = 0;

        for (j = i; j <= len - 2; j++) {
            acc += A[i][j] * c[j];
        }

        c[i] = (A[i][len - 1] - acc) / A[i][i];
    }

    for (i = 0; i < out_len; i++) {
        float x_out = out_x[i];
        float y_out = 0;

        for (j = 0; j < len - 1; j++) {
            if (x[j] <= x_out && x_out <= x[j + 1]) {
                float v = x_out - x[j];
                y_out = y[j] +
                        ((y[j + 1] - y[j]) / d[j] -
                         (2 * d[j] * c[j] + c[j + 1] * d[j]) / 6) *
                            v +
                        (c[j] * 0.5) * v * v +
                        ((c[j + 1] - c[j]) / (6 * d[j])) * v * v * v;
            }
        }

        out_y[i] = y_out;
    }

    free(A);
    free(As);
    free(b);
    free(c);
    free(d);
}

// generic function for finding minimum of f(x) in the a-b range using the
// interval halving method
float PerceptualToneCurve::find_minimum_interval_halving(
    float (*func)(float x, void *arg), void *arg, float a, float b, float tol,
    int nmax)
{
    float L = b - a;
    float x = (a + b) * 0.5;

    for (int i = 0; i < nmax; i++) {
        float f_x = func(x, arg);

        if ((b - a) * 0.5 < tol) {
            return x;
        }

        float x1 = a + L / 4;
        float f_x1 = func(x1, arg);

        if (f_x1 < f_x) {
            b = x;
            x = x1;
        } else {
            float x2 = b - L / 4;
            float f_x2 = func(x2, arg);

            if (f_x2 < f_x) {
                a = x;
                x = x2;
            } else {
                a = x1;
                b = x2;
            }
        }

        L = b - a;
    }

    return x;
}

struct find_tc_slope_fun_arg {
    const ToneCurve *tc;
};

float PerceptualToneCurve::find_tc_slope_fun(float k, void *arg)
{
    struct find_tc_slope_fun_arg *a = (struct find_tc_slope_fun_arg *)arg;
    float areasum = 0;
    const int steps = 10;

    for (int i = 0; i < steps; i++) {
        float x = 0.1 + ((float)i / (steps - 1)) *
                            0.5; // testing (sRGB) range [0.1 - 0.6], ie ignore
                                 // highligths and dark shadows
        float y = Color::gamma2(a->tc->lutToneCurve[Color::igamma2(x) * 65535] /
                                65535.0);
        float y1 = k * x;

        if (y1 > 1) {
            y1 = 1;
        }

        areasum +=
            (y - y1) * (y - y1); // square is a rough approx of (twice) the
                                 // area, but it's fine for our purposes
    }

    return areasum;
}

float PerceptualToneCurve::get_curve_val(float x, float range[2], float lut[],
                                         size_t lut_size)
{
    float xm = (x - range[0]) / (range[1] - range[0]) * (lut_size - 1);

    if (xm <= 0) {
        return lut[0];
    }

    int idx = (int)xm;

    if (idx >= static_cast<int>(lut_size) - 1) {
        return lut[lut_size - 1];
    }

    float d = xm - (float)idx; // [0 .. 1]
    return (1.0 - d) * lut[idx] + d * lut[idx + 1];
}

// calculate a single value that represents the contrast of the tone curve
float PerceptualToneCurve::calculateToneCurveContrastValue() const
{

    // find linear y = k*x the best approximates the curve, which is the linear
    // scaling/exposure component that does not contribute any contrast

    // Note: the analysis is made on the gamma encoded curve, as the LUT is
    // linear we make backwards gamma to
    struct find_tc_slope_fun_arg arg = {this};
    float k =
        find_minimum_interval_halving(find_tc_slope_fun, &arg, 0.1, 5.0, 0.01,
                                      20); // normally found in 8 iterations
    // fprintf(stderr, "average slope: %f\n", k);

    float maxslope = 0;
    {
        // look at midtone slope
        const float xd = 0.07;
        const float tx[] = {0.30, 0.35, 0.40,
                            0.45}; // we only look in the midtone range

        for (size_t i = 0; i < sizeof(tx) / sizeof(tx[0]); i++) {
            float x0 = tx[i] - xd;
            float y0 =
                Color::gamma2(lutToneCurve[Color::igamma2(x0) * 65535.f] /
                              65535.f) -
                k * x0;
            float x1 = tx[i] + xd;
            float y1 =
                Color::gamma2(lutToneCurve[Color::igamma2(x1) * 65535.f] /
                              65535.f) -
                k * x1;
            float slope = 1.0 + (y1 - y0) / (x1 - x0);

            if (slope > maxslope) {
                maxslope = slope;
            }
        }

        // look at slope at (light) shadows and (dark) highlights
        float e_maxslope = 0;
        {
            const float tx[] = {0.20, 0.25, 0.50,
                                0.55}; // we only look in the midtone range

            for (size_t i = 0; i < sizeof(tx) / sizeof(tx[0]); i++) {
                float x0 = tx[i] - xd;
                float y0 =
                    Color::gamma2(lutToneCurve[Color::igamma2(x0) * 65535.f] /
                                  65535.f) -
                    k * x0;
                float x1 = tx[i] + xd;
                float y1 =
                    Color::gamma2(lutToneCurve[Color::igamma2(x1) * 65535.f] /
                                  65535.f) -
                    k * x1;
                float slope = 1.0 + (y1 - y0) / (x1 - x0);

                if (slope > e_maxslope) {
                    e_maxslope = slope;
                }
            }
        }
        // fprintf(stderr, "%.3f %.3f\n", maxslope, e_maxslope);
        //  midtone slope is more important for contrast, but weigh in some
        //  slope from brights and darks too.
        maxslope = maxslope * 0.7 + e_maxslope * 0.3;
    }
    return maxslope;
}

namespace {

inline float scurve(const float x) // x must be in 0..1 range
{
    if (x < 0.5f) {
        return 2.f * SQR(x);
    } else {
        return 1.f - 2.f * SQR(1.f - x);
    }
}

} // namespace

void PerceptualToneCurve::BatchApply(
    const size_t start, const size_t end, float *rc, float *gc, float *bc,
    const PerceptualToneCurveState &state) const
{
    const AdobeToneCurve &adobeTC =
        static_cast<const AdobeToneCurve &>((const ToneCurve &)*this);
    const StandardToneCurve &stdTC =
        static_cast<const StandardToneCurve &>((const ToneCurve &)*this);

    const float strength = state.strength;

    const auto to_prophoto = [&state](float &r, float &g, float &b) -> void {
        if (!state.isProphoto) {
            float newr = state.Working2Prophoto[0][0] * r +
                         state.Working2Prophoto[0][1] * g +
                         state.Working2Prophoto[0][2] * b;
            float newg = state.Working2Prophoto[1][0] * r +
                         state.Working2Prophoto[1][1] * g +
                         state.Working2Prophoto[1][2] * b;
            float newb = state.Working2Prophoto[2][0] * r +
                         state.Working2Prophoto[2][1] * g +
                         state.Working2Prophoto[2][2] * b;
            r = CLIP(newr);
            g = CLIP(newg);
            b = CLIP(newb);
        }
    };

    const auto to_working = [&state](float &r, float &g, float &b) -> void {
        if (!state.isProphoto) {
            float newr = state.Prophoto2Working[0][0] * r +
                         state.Prophoto2Working[0][1] * g +
                         state.Prophoto2Working[0][2] * b;
            float newg = state.Prophoto2Working[1][0] * r +
                         state.Prophoto2Working[1][1] * g +
                         state.Prophoto2Working[1][2] * b;
            float newb = state.Prophoto2Working[2][0] * r +
                         state.Prophoto2Working[2][1] * g +
                         state.Prophoto2Working[2][2] * b;
            r = CLIP(newr);
            g = CLIP(newg);
            b = CLIP(newb);
        }
    };

    for (size_t i = start; i < end; ++i) {
        float r = CLIP(rc[i]);
        float g = CLIP(gc[i]);
        float b = CLIP(bc[i]);
        // float r = rc[i];
        // float g = gc[i];
        // float b = bc[i];

        to_prophoto(r, g, b);

        { // fix out of gamut blues. Apply a variation of this trick:
          // https://acescentral.com/t/colour-artefacts-or-breakup-using-aces/520/8
          // matrix hand-tuned by visual inspection (!!)
            // [ 1.0 0.0  0.0
            //   0.0 0.94 0.06
            //   0.0 0.0  1.0 ]
            float hue, sat, val;
            Color::rgb2hsv(r, g, b, hue, sat, val);
            hue *= 360.f;
            constexpr float blue_hue = 250.f;
            constexpr float blue_hue_inner = 20.f;
            constexpr float blue_hue_outer = 40.f;
            constexpr float blue_sat_lower = 0.65f;
            float dist = std::abs(hue - blue_hue);
            if (dist <= blue_hue_outer && sat >= blue_sat_lower) {
                float gg = intp(0.94f, g, b);
                float d = std::max(dist - blue_hue_inner, 0.f);
                float x = 1.f - d / (blue_hue_outer - blue_hue_inner);
                x = scurve(x);
                float xx = (sat - blue_sat_lower) / (1.f - blue_sat_lower);
                xx = scurve(xx);
                g = intp(x * xx, gg, g);
            }
        }

        float std_r = r;
        float std_g = g;
        float std_b = b;
        stdTC.Apply(std_r, std_g, std_b);
        to_working(std_r, std_g, std_b);

        float ar = r;
        float ag = g;
        float ab = b;
        adobeTC.Apply(ar, ag, ab);

        if (ar >= 65535.f && ag >= 65535.f && ab >= 65535.f) {
            // clip fast path, will also avoid strange colours of clipped
            // highlights
            // rc[i] = gc[i] = bc[i] = 65535.f;
            rc[i] = 65535.f;
            gc[i] = 65535.f;
            bc[i] = 65535.f;
            continue;
        }

        if (ar <= 0.f && ag <= 0.f && ab <= 0.f) {
            // rc[i] = gc[i] = bc[i] = 0;
            rc[i] = 0.f;
            gc[i] = 0.f;
            bc[i] = 0.f;
            continue;
        }

        // ProPhoto constants for luminance, that is xyz_prophoto[1][]
        constexpr float Yr = 0.2880402f;
        constexpr float Yg = 0.7118741f;
        constexpr float Yb = 0.0000857f;

        // we use the Adobe (RGB-HSV hue-stabilized) curve to decide luminance,
        // which generally leads to a less contrasty result compared to a pure
        // luminance curve. We do this to be more compatible with the most
        // popular curves.
        const float oldLuminance = r * Yr + g * Yg + b * Yb;
        const float newLuminance = ar * Yr + ag * Yg + ab * Yb;
        const float Lcoef = newLuminance / oldLuminance;
        r = LIM<float>(r * Lcoef, 0.f, 65535.f);
        g = LIM<float>(g * Lcoef, 0.f, 65535.f);
        b = LIM<float>(b * Lcoef, 0.f, 65535.f);

        // move to JCh so we can modulate chroma based on the global
        // contrast-related chroma scaling factor
        float x, y, z;
        Color::Prophotoxyz(r, g, b, x, y, z);

        float J, C, h;
        Ciecam02::xyz2jch_ciecam02float(
            J, C, h, aw, fl, x * 0.0015259022f, y * 0.0015259022f,
            z * 0.0015259022f, xw, yw, zw, c, nc, pow1, nbb, ncb, cz, d);

        if (!std::isfinite(J) || !std::isfinite(C) || !std::isfinite(h)) {
            // this can happen for dark noise colours or colours outside human
            // gamut. Then we just return the curve's result.
            to_working(r, g, b);
            rc[i] = CLIP(intp(strength, r, std_r));
            gc[i] = CLIP(intp(strength, g, std_g));
            bc[i] = CLIP(intp(strength, b, std_b));

            continue;
        }

        float cmul = state.cmul_contrast; // chroma scaling factor

        // depending on color, the chroma scaling factor can be fine-tuned below

        {
            // decrease chroma scaling slightly of extremely saturated colors
            float saturated_scale_factor = 0.95f;
            constexpr float lolim =
                35.f; // lower limit, below this chroma all colors will keep
                      // original chroma scaling factor
            constexpr float hilim =
                60.f; // high limit, above this chroma the chroma scaling factor
                      // is multiplied with the saturated scale factor value
                      // above

            if (C < lolim) {
                // chroma is low enough, don't scale
                saturated_scale_factor = 1.f;
            } else if (C < hilim) {
                // S-curve transition between low and high limit
                float x = (C - lolim) /
                          (hilim - lolim); // x = [0..1], 0 at lolim, 1 at hilim

                if (x < 0.5f) {
                    x = 2.f * SQR(x);
                } else {
                    x = 1.f - 2.f * SQR(1 - x);
                }

                saturated_scale_factor = (1.f - x) + saturated_scale_factor * x;
            } else {
                // do nothing, high saturation color, keep scale factor
            }

            cmul *= saturated_scale_factor;
        }

        {
            // increase chroma scaling slightly of shadows
            float nL =
                Color::gamma2curve[newLuminance]; // apply gamma so we make
                                                  // comparison and transition
                                                  // with a more perceptual
                                                  // lightness scale
            float dark_scale_factor = 1.20f;
            // float dark_scale_factor = 1.0 + state.debug.p2 / 100.0f;
            constexpr float lolim = 0.15f;
            constexpr float hilim = 0.50f;

            if (nL < lolim) {
                // do nothing, keep scale factor
            } else if (nL < hilim) {
                // S-curve transition
                float x = (nL - lolim) /
                          (hilim - lolim); // x = [0..1], 0 at lolim, 1 at hilim

                if (x < 0.5f) {
                    x = 2.f * SQR(x);
                } else {
                    x = 1.f - 2.f * SQR(1 - x);
                }

                dark_scale_factor = dark_scale_factor * (1.0f - x) + x;
            } else {
                dark_scale_factor = 1.f;
            }

            cmul *= dark_scale_factor;
        }

        {
            // to avoid strange CIECAM02 chroma errors on
            // close-to-shadow-clipping colors we reduce chroma scaling
            // towards 1.0 for black colors
            float dark_scale_factor = 1.f / cmul;
            constexpr float lolim = 4.f;
            constexpr float hilim = 7.f;

            if (J < lolim) {
                // do nothing, keep scale factor
            } else if (J < hilim) {
                // S-curve transition
                float x = (J - lolim) / (hilim - lolim);

                if (x < 0.5f) {
                    x = 2.f * SQR(x);
                } else {
                    x = 1.f - 2.f * SQR(1 - x);
                }

                dark_scale_factor = dark_scale_factor * (1.f - x) + x;
            } else {
                dark_scale_factor = 1.f;
            }

            cmul *= dark_scale_factor;
        }

        C *= cmul;

        Ciecam02::jch2xyz_ciecam02float(x, y, z, J, C, h, xw, yw, zw, c, nc,
                                        pow1, nbb, ncb, fl, cz, d, aw);

        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) {
            // can happen for colours on the rim of being outside gamut, that
            // worked without chroma scaling but not with. Then we return only
            // the curve's result.
            to_working(r, g, b);

            // rc[i] = r;
            // gc[i] = g;
            // bc[i] = b;
            rc[i] = intp(strength, r, std_r);
            gc[i] = intp(strength, g, std_g);
            bc[i] = intp(strength, b, std_b);

            continue;
        }

        Color::xyz2Prophoto(x, y, z, r, g, b);
        r *= 655.35f;
        g *= 655.35f;
        b *= 655.35f;
        r = LIM<float>(r, 0.f, 65535.f);
        g = LIM<float>(g, 0.f, 65535.f);
        b = LIM<float>(b, 0.f, 65535.f);

        {
            // limit saturation increase in rgb space to avoid severe clipping
            // and flattening in extreme highlights

            // we use the RGB-HSV hue-stable "Adobe" curve as reference. For
            // S-curve contrast it increases saturation greatly, but desaturates
            // extreme highlights and thus provide a smooth transition to the
            // white point. However the desaturation effect is quite strong so
            // we make a weighting
            const float as = Color::rgb2s(ar, ag, ab);
            const float s = Color::rgb2s(r, g, b);

            const float sat_scale =
                as <= 0.f ? 1.f
                          : s / as; // saturation scale compared to Adobe curve
            float keep = 0.2f;
            constexpr float lolim =
                1.00f; // only mix in the Adobe curve if we have increased
                       // saturation compared to it
            constexpr float hilim = 1.20f;

            if (sat_scale < lolim) {
                // saturation is low enough, don't desaturate
                keep = 1.f;
            } else if (sat_scale < hilim) {
                // S-curve transition
                float x = (sat_scale - lolim) /
                          (hilim - lolim); // x = [0..1], 0 at lolim, 1 at hilim

                if (x < 0.5f) {
                    x = 2.f * SQR(x);
                } else {
                    x = 1.f - 2.f * SQR(1 - x);
                }

                keep = (1.f - x) + keep * x;
            } else {
                // do nothing, very high increase, keep minimum amount
            }

            if (keep < 1.f) {
                // mix in some of the Adobe curve result
                r = intp(keep, r, ar);
                g = intp(keep, g, ag);
                b = intp(keep, b, ab);
            }
        }

        to_working(r, g, b);
        // rc[i] = r;
        // gc[i] = g;
        // bc[i] = b;
        rc[i] = CLIP(intp(strength, r, std_r));
        gc[i] = CLIP(intp(strength, g, std_g));
        bc[i] = CLIP(intp(strength, b, std_b));
    }
}

float PerceptualToneCurve::cf_range[2];
float PerceptualToneCurve::cf[1000];
float PerceptualToneCurve::f, PerceptualToneCurve::c, PerceptualToneCurve::nc,
    PerceptualToneCurve::yb, PerceptualToneCurve::la, PerceptualToneCurve::xw,
    PerceptualToneCurve::yw, PerceptualToneCurve::zw;
float PerceptualToneCurve::n, PerceptualToneCurve::d, PerceptualToneCurve::nbb,
    PerceptualToneCurve::ncb, PerceptualToneCurve::cz, PerceptualToneCurve::aw,
    PerceptualToneCurve::wh, PerceptualToneCurve::pfl, PerceptualToneCurve::fl,
    PerceptualToneCurve::pow1;

void PerceptualToneCurve::init()
{

    // init ciecam02 state, used for chroma scalings
    xw = 96.42f;
    yw = 100.0f;
    zw = 82.49f;
    yb = 20;
    la = 20;
    f = 1.00f;
    c = 0.69f;
    nc = 1.00f;

    Ciecam02::initcam1float(yb, 1.f, f, la, xw, yw, zw, n, d, nbb, ncb, cz, aw,
                            wh, pfl, fl, c);
    pow1 = pow_F(1.64f - pow_F(0.29f, n), 0.73f);

    {
        // init contrast-value-to-chroma-scaling conversion curve

        // contrast value in the left column, chroma scaling in the right.
        // Handles for a spline. Put the columns in a file (without commas) and
        // you can plot the spline with gnuplot: "plot 'curve.txt' smooth
        // csplines" A spline can easily get overshoot issues so if you
        // fine-tune the values here make sure that the resulting spline is
        // smooth afterwards, by plotting it for example.
        const float p[] = {
            0.60, 0.70,                                     // lowest contrast
            0.70, 0.80, 0.90, 0.94, 0.99, 1.00, 1.00, 1.00, // 1.0 (linear
                                                            // curve) to 1.0, no
                                                            // scaling
            1.07, 1.00, 1.08, 1.00, 1.11, 1.02, 1.20, 1.08, 1.30,
            1.12, 1.80, 1.20, 2.00, 1.22 // highest contrast
        };

        const size_t in_len = sizeof(p) / sizeof(p[0]) / 2;
        float in_x[in_len];
        float in_y[in_len];

        for (size_t i = 0; i < in_len; i++) {
            in_x[i] = p[2 * i + 0];
            in_y[i] = p[2 * i + 1];
        }

        const size_t out_len = sizeof(cf) / sizeof(cf[0]);
        float out_x[out_len];

        for (size_t i = 0; i < out_len; i++) {
            out_x[i] = in_x[0] +
                       (in_x[in_len - 1] - in_x[0]) * (float)i / (out_len - 1);
        }

        cubic_spline(in_x, in_y, in_len, out_x, cf, out_len);
        cf_range[0] = in_x[0];
        cf_range[1] = in_x[in_len - 1];
    }
}

void PerceptualToneCurve::initApplyState(
    PerceptualToneCurveState &state, const Glib::ustring &workingSpace) const
{
    state.strength = 1.f;

    // Get the curve's contrast value, and convert to a chroma scaling
    const float contrast_value = calculateToneCurveContrastValue();
    state.cmul_contrast =
        get_curve_val(contrast_value, cf_range, cf, sizeof(cf) / sizeof(cf[0]));
    // fprintf(stderr, "contrast value: %f => chroma scaling %f\n",
    // contrast_value, state.cmul_contrast);

    // Create state for converting to/from prophoto (if necessary)
    if (workingSpace == "ProPhoto") {
        state.isProphoto = true;
    } else {
        state.isProphoto = false;
        TMatrix Work =
            ICCStore::getInstance()->workingSpaceMatrix(workingSpace);
        memset(state.Working2Prophoto, 0, sizeof(state.Working2Prophoto));

        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                for (int k = 0; k < 3; k++) {
                    state.Working2Prophoto[i][j] +=
                        prophoto_xyz[i][k] * Work[k][j];
                }

        Work = ICCStore::getInstance()->workingSpaceInverseMatrix(workingSpace);
        memset(state.Prophoto2Working, 0, sizeof(state.Prophoto2Working));

        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                for (int k = 0; k < 3; k++) {
                    state.Prophoto2Working[i][j] +=
                        Work[i][k] * xyz_prophoto[k][j];
                }
    }
}

NeutralToneCurve::ApplyState::ApplyState(const Glib::ustring &workingSpace,
                                         const Glib::ustring &outprofile,
                                         const Curve *base)
{
    basecurve = base;

    auto work = ICCStore::getInstance()->workingSpaceMatrix(workingSpace);
    auto iwork =
        ICCStore::getInstance()->workingSpaceInverseMatrix(workingSpace);

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            ws[i][j] = work[i][j];
            iws[i][j] = iwork[i][j];
        }
    }

    Mat33<float> om;
    if (ICCStore::getInstance()->getProfileMatrix(outprofile, om)) {
        auto iom = inverse(om);
        to_out = dot_product(iom, work);
        to_work = dot_product(iwork, om);
    } else {
        to_out = identity<float>();
        to_work = identity<float>();
    }

    float j, c;
    const auto hws = xyz_rec2020;
    Color::rgb2jzczhz(1, 0, 0, j, c, rhue, hws);
    Color::rgb2jzczhz(0, 0, 1, j, c, bhue, hws);
    Color::rgb2jzczhz(1, 1, 0, j, c, yhue, hws);
    float ohue;
    Color::rgb2jzczhz(1, 0.5, 0, j, c, ohue, hws);
    yrange = std::abs(ohue - yhue) * 0.8f;
    rrange = std::abs(ohue - rhue);
    brange = rrange;
}

void NeutralToneCurve::BatchApply(const size_t start, const size_t end,
                                  float *rc, float *gc, float *bc,
                                  const ApplyState &state) const
{
    Vec3f rgb;
    Vec3f jch;

    const float Lmax = whitept;

    // from https://github.com/jedypod/gamut-compress

    // Distance limit: How far beyond the gamut boundary to compress
    // const Vec3<float> dl(1.147f, 1.264f, 1.312f); // original ACES values
    static const Vec3<float> dl(1.1f, 1.2f, 1.5f); // hand-tuned
    // Amount of outer gamut to affect
    // const Vec3<float> th(0.815f, 0.803f, 0.88f); // original ACES values
    static const Vec3<float> th(0.85f, 0.75f, 0.95f); // hand-tuned

    // Power or Parabolic compression functions:
    // https://www.desmos.com/calculator/nvhp63hmtj
#if 0 // power compression
    constexpr float p = 1.2f;
    const auto scale =
        [](float l, float t, float p) -> float
        {
            return (l - t) / std::pow(std::pow((1.f - t)/(l - t), -p) - 1.f, 1.f/p);
        };
    static const Vec3<float> s(scale(dl[0], th[0], p),
                               scale(dl[1], th[1], p),
                               scale(dl[2], th[2], p));

    const auto compr =
        [&](float x, int i) -> float
        {
            float t = (x - th[i])/s[i];
            return th[i] + s[i] * std::pow(t / (1.f + std::pow(t, p)), 1.f/p);
        };
#else // parabolic compression
    const auto scale = [](float l, float t) -> float {
        return (1.f - t) / std::sqrt(l - 1.f);
    };
    static const Vec3<float> s(scale(dl[0], th[0]), scale(dl[1], th[1]),
                               scale(dl[2], th[2]));

    const auto compr = [&](float x, int i) -> float {
        return s[i] * std::sqrt(x - th[i] + SQR(s[i]) / 4.0f) -
               s[i] * std::sqrt(SQR(s[i]) / 4.0f) + th[i];
    };
#endif // power/parabolic compression

    const auto gauss = [](float x, float b, float c) -> float {
        return xexpf(-SQR(x - b) / (2 * SQR(c)));
    };

    for (size_t i = start; i < end; ++i) {
        rgb[0] = std::max(rc[i] / 65535.f, 0.f);
        rgb[1] = std::max(gc[i] / 65535.f, 0.f);
        rgb[2] = std::max(bc[i] / 65535.f, 0.f);

        Color::rgb2jzczhz(rgb[0], rgb[1], rgb[2], jch[0], jch[1], jch[2],
                          state.ws);

        float ilum = jch[0];
        float hue = jch[2];

        // gamut compression
        float iY = (rgb[0] + rgb[1] + rgb[2]) / 3.f;

        rgb = dot_product(state.to_out, rgb);

        // Achromatic axis
        float ac = max(rgb[0], rgb[1], rgb[2]);

        // Inverse RGB Ratios: distance from achromatic axis
        Vec3<float> d;
        float aac = std::abs(ac);
        if (ac != 0.f) {
            d[0] = (ac - rgb[0]) / aac;
            d[1] = (ac - rgb[1]) / aac;
            d[2] = (ac - rgb[2]) / aac;
        }

        Vec3<float> cd; // Compressed distance
        for (int i = 0; i < 3; ++i) {
            cd[i] = d[i] < th[i] ? d[i] : compr(d[i], i);
        }

        // Inverse RGB Ratios to RGB
        rgb[0] = ac - cd[0] * aac;
        rgb[1] = ac - cd[1] * aac;
        rgb[2] = ac - cd[2] * aac;

        rgb = dot_product(state.to_work, rgb);

        if (state.basecurve) {
            for (int c = 0; c < 3; ++c) {
                rgb[c] = state.basecurve->getVal(rgb[c]);
            }
        } else {
            float oY = (rgb[0] + rgb[1] + rgb[2]) / 3.f;
            if (oY > 0.f) {
                float f = iY / oY;
                rgb[0] *= f;
                rgb[1] *= f;
                rgb[2] *= f;
                Color::filmlike_clip(&rgb[0], &rgb[1], &rgb[2], Lmax);
            }
        }

        // apply the curve
        for (int j = 0; j < 3; ++j) {
            float nt = rgb[j] * 65535.f;
            curves::setLutVal(lutToneCurve, curve, nt);
            rgb[j] = nt / 65535.f;
        }

        Color::rgb2jzczhz(rgb[0], rgb[1], rgb[2], jch[0], jch[1], jch[2],
                          state.ws);

#if 0
        float red_dist = LIM01(std::sqrt(SQR(rgb[0]-whitecoeff) + SQR(rgb[1]) + SQR(rgb[2])));
        float blue_dist = LIM01(std::sqrt(SQR(rgb[0]) + SQR(rgb[1]) + SQR(rgb[2]-whitecoeff)));
        float hue_shift = 15.f * RT_PI_F_180 * (1.f - red_dist);
        hue_shift += -15.f * RT_PI_F_180 * (1.f - blue_dist);
        hue_shift *= LIM01((rgb[0] + rgb[1] + rgb[2]) / (3.f * whitecoeff));
        hue += hue_shift;
#else
        float hue_shift =
            15.f * RT_PI_F_180 * gauss(hue, state.rhue, state.rrange);
        hue_shift += -5.f * RT_PI_F_180 * gauss(hue, state.bhue, state.brange);
        hue_shift *= LIM01((rgb[0] + rgb[1] + rgb[2]) / (3.f * whitecoeff));
        hue += hue_shift;
#endif

        float sat = jch[1];
        if (!state.basecurve) {
            float olum = jch[0];

            float ccf = ilum > 1e-5f
                            ? (1.f - (LIM01((olum / ilum) - 1.f) * 0.2f))
                            : 1.f;
            ccf = LIM01(ccf + 0.5f * gauss(hue, state.yhue, state.yrange));
            sat *= ccf;
        }

        Color::jzczhz2rgb(jch[0], sat, hue, rgb[0], rgb[1], rgb[2], state.iws);

        rc[i] = LIM(rgb[0] * 65535.f, 0.f, whitept);
        gc[i] = LIM(rgb[1] * 65535.f, 0.f, whitept);
        bc[i] = LIM(rgb[2] * 65535.f, 0.f, whitept);
    }
}

} // namespace rtengine
