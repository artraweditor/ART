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
#include "colortemp.h"
#include "rtengine.h"
#include "sleef.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

namespace rtengine {

namespace {

static const double cie_colour_match_jd[97][3] =
    { // 350nm to 830nm   5 nm J.Desmis 2° Standard Observer.
        {0.0000000, 0.000000, 0.000000},
        {0.0000000, 0.000000, 0.000000},
        {0.0001299, 0.0003917, 0.0006061},
        {0.0002321, 0.000006965, 0.001086},
        {0.0004149, 0.00001239, 0.001946},
        {0.0007416, 0.00002202, 0.003846},
        {0.001368, 0.000039, 0.006450001},
        {0.002236, 0.000064, 0.01054999},
        {0.004243, 0.000120, 0.02005001},
        {0.007650, 0.000217, 0.036210},
        {0.014310, 0.000396, 0.06785001},
        {0.023190, 0.000640, 0.110200},
        {0.043510, 0.001210, 0.207400},
        {0.077630, 0.002180, 0.371300},
        {0.134380, 0.004000, 0.645600},
        {0.214770, 0.007300, 1.0390501},
        {0.283900, 0.011600, 1.385600},
        {0.328500, 0.016840, 1.622960},
        {0.348280, 0.023000, 1.747060},
        {0.348060, 0.029800, 1.782600},
        {0.336200, 0.038000, 1.772110},
        {0.318700, 0.048000, 1.744100},
        {0.290800, 0.060000, 1.669200},
        {0.251100, 0.073900, 1.528100},
        {0.195360, 0.090980, 1.287640},
        {0.142100, 0.112600, 1.041900},
        {0.095640, 0.139020, 0.8129501},
        {0.05795001, 0.169300, 0.616200},
        {0.032010, 0.208020, 0.465180},
        {0.014700, 0.258600, 0.353300},
        {0.004900, 0.323000, 0.272000},
        {0.002400, 0.407300, 0.212300},
        {0.009300, 0.503000, 0.158200},
        {0.029100, 0.608200, 0.111700},
        {0.063270, 0.710000, 0.07824999},
        {0.109600, 0.793200, 0.05725001},
        {0.165500, 0.862000, 0.042160},
        {0.2257499, 0.9148501, 0.029840},
        {0.290400, 0.954000, 0.020300},
        {0.359700, 0.980300, 0.013400},
        {0.43344990, 0.9949501, 0.008749999},
        {0.5120501, 1.000000, 0.005749999},
        {0.594500, 0.995000, 0.003900},
        {0.678400, 0.978600, 0.002749999},
        {0.762100, 0.952000, 0.002100},
        {0.842500, 0.915400, 0.001800},
        {0.916300, 0.870000, 0.001650001},
        {0.978600, 0.816300, 0.001400},
        {1.026300, 0.757000, 0.001100},
        {1.056700, 0.694900, 0.001000},
        {1.062200, 0.631000, 0.000800},
        {1.045600, 0.566800, 0.000600},
        {1.002600, 0.503000, 0.000340},
        {0.938400, 0.441200, 0.000240},
        {0.8544499, 0.381000, 0.000190},
        {0.751400, 0.321000, 0.000100},
        {0.642400, 0.265000, 0.00004999999},
        {0.541900, 0.217000, 0.000030},
        {0.447900, 0.175000, 0.000020},
        {0.360800, 0.138200, 0.000010},
        {0.283500, 0.107000, 0.000000},
        {0.218700, 0.081600, 0.000000},
        {0.164900, 0.061000, 0.000000},
        {0.121200, 0.044580, 0.000000},
        {0.087400, 0.032000, 0.000000},
        {0.063600, 0.023200, 0.000000},
        {0.046770, 0.017000, 0.000000},
        {0.032900, 0.011920, 0.000000},
        {0.022700, 0.008210, 0.000000},
        {0.015840, 0.005723, 0.000000},
        {0.01135916, 0.004102, 0.000000},
        {0.008110916, 0.002929, 0.000000},
        {0.005790346, 0.002091, 0.000000},
        {0.004109457, 0.001484, 0.000000},
        {0.002899327, 0.001047, 0.000000},
        {0.00204919, 0.000740, 0.000000},
        {0.001439971, 0.000520, 0.000000},
        {0.0009999493, 0.0003611, 0.000000},
        {0.0006900786, 0.0002492, 0.000000},
        {0.0004760213, 0.0001719, 0.000000},
        {0.0003323011, 0.000120, 0.000000},
        {0.0002348261, 0.0000848, 0.000000},
        {0.0001661505, 0.000060, 0.000000},
        {0.000117413, 0.0000424, 0.000000},
        {0.00008307527, 0.000030, 0.000000},
        {0.00005870652, 0.0000212, 0.000000},
        {0.00004150994, 0.00001499, 0.000000},
        {0.00002935326, 0.0000106, 0.000000},
        {0.00002067383, 0.0000074657, 0.000000},
        {0.00001455977, 0.0000052578, 0.000000},
        {0.00001025398, 0.0000037029, 0.000000},
        {0.000007221456, 0.00000260778, 0.000000},
        {0.000005085868, 0.0000018366, 0.000000},
        {0.000003581652, 0.0000012934, 0.000000},
        {0.000002522525, 0.00000091093, 0.000000},
        {0.000001776509, 0.00000064153, 0.000000},
        {0.000001251141, 0.00000045181, 0.000000}};

/*
    Calculate Planck's radiation
*/
// calculate spectral data for blackbody at temp!
double blackbody_spect(double wavelength, double temperature)
{
    double wlm = wavelength * 1e-9; /* Wavelength in meters */
    return (3.7417715247e-16 /
            pow(wlm, 5)) / // 3.7417..= c1 = 2*Pi*h*c2  where h=Planck constant,
                           // c=velocity of light
           (xexp(1.438786e-2 / (wlm * temperature)) -
            1.0); // 1.4387..= c2 = h*c/k  where k=Boltzmann constant
}

double daylight_spect(double wavelength, double m1, double m2)
{
    // Values for Daylight illuminant: s0 s1 s2
    // s0
    static const double s0[97] = {
        61.80,  61.65,  61.50,  65.15,  68.80,  66.10,  63.40,  64.60,  65.80,
        80.30,  94.80,  99.80,  104.80, 105.35, 105.90, 101.35, 96.80,  105.35,
        113.90, 119.75, 125.60, 125.55, 125.50, 123.40, 121.30, 121.30, 121.30,
        117.40, 113.50, 113.30, 113.10, 111.95, 110.80, 108.65, 106.50, 107.65,
        108.80, 107.05, 105.30, 104.85, 104.40, 102.20, 100.00, 98.00,  96.00,
        95.55,  95.10,  92.10,  89.10,  89.80,  90.50,  90.40,  90.30,  89.35,
        88.40,  86.20,  84.00,  84.55,  85.10,  83.50,  81.90,  82.25,  82.60,
        83.75,  84.90,  83.10,  81.30,  76.60,  71.90,  73.10,  74.30,  75.35,
        76.40,  69.85,  63.30,  67.50,  71.70,  74.35,  77.00,  71.10,  65.20,
        56.45,  47.70,  58.15,  68.60,  66.80,  65.00,  65.50,  66.00,  63.50,
        61.00,  57.15,  53.30,  56.10,  58.90,  60.40,  61.90};
    // s1
    static const double s1[97] = {
        41.60,  39.80,  38.00,  40.70,  43.40,  40.95,  38.50,  36.75,  35.00,
        39.20,  43.40,  44.85,  46.30,  45.10,  43.90,  40.50,  37.10,  36.90,
        36.70,  36.30,  35.90,  34.25,  32.60,  30.25,  27.90,  26.10,  24.30,
        22.20,  20.10,  18.15,  16.20,  14.70,  13.20,  10.90,  8.60,   7.35,
        6.10,   5.15,   4.20,   3.05,   1.90,   0.95,   0.00,   -0.80,  -1.60,
        -2.55,  -3.50,  -3.50,  -3.50,  -4.65,  -5.80,  -6.50,  -7.20,  -7.90,
        -8.60,  -9.05,  -9.50,  -10.20, -10.90, -10.80, -10.70, -11.35, -12.00,
        -13.00, -14.00, -13.80, -13.60, -12.80, -12.00, -12.65, -13.30, -13.10,
        -12.90, -11.75, -10.60, -11.10, -11.60, -11.90, -12.20, -11.20, -10.20,
        -9.00,  -7.80,  -9.50,  -11.20, -10.80, -10.50, -10.60, -10.15, -9.70,
        -9.00,  -8.30,  -8.80,  -9.30,  -9.55,  -9.80};
    // s2
    static const double s2[97] = {
        6.70,  6.00,  5.30,  5.70,  6.10,  4.55,  3.00,  2.10,  1.20,  0.05,
        -1.10, -0.80, -0.50, -0.60, -0.70, -0.95, -1.20, -1.90, -2.60, -2.75,
        -2.90, -2.85, -2.80, -2.70, -2.60, -2.60, -2.60, -2.20, -1.80, -1.65,
        -1.50, -1.40, -1.30, -1.25, -1.20, -1.10, -1.00, -0.75, -0.50, -0.40,
        -0.30, -0.15, 0.00,  0.10,  0.20,  0.35,  0.50,  1.30,  2.10,  2.65,
        3.65,  4.10,  4.40,  4.70,  4.90,  5.10,  5.90,  6.70,  7.00,  7.30,
        7.95,  8.60,  9.20,  9.80,  10.00, 10.20, 9.25,  8.30,  8.95,  9.60,
        9.05,  8.50,  7.75,  7.00,  7.30,  7.60,  7.80,  8.00,  7.35,  6.70,
        5.95,  5.20,  6.30,  7.40,  7.10,  6.80,  6.90,  7.00,  6.70,  6.40,
        5.95,  5.50,  5.80,  6.10,  6.30,  6.50};

    int wlm = (int)((wavelength - 350.) / 5.);
    return (s0[wlm] + m1 * s1[wlm] + m2 * s2[wlm]);
}

/*
The next 3 methods are inspired from:

   a) Colour Rendering of Spectra  by John Walker
      http://www.fourmilab.ch/
      This program is in the public domain.

   b) Bruce Lindbloom
      Adapted to RawTherapee by J.Desmis

this values are often called xBar yBar zBar and are characteristics of a color /
illuminant

values cie_colour_match[][3] = 2° Standard Observer x2, y2, z2
E.g. for 380nm: x2=0.001368  y2=0.000039  z2=0.006451  round in J.Walker to
0.0014  0.0000 0.0065 above I have increase precision used by J.Walker  and pass
to 350nm to 830nm
*/

void spectrum_to_xyz_daylight(double _m1, double _m2, double &x, double &y,
                              double &z)
{
    int i;
    double lambda, X = 0, Y = 0, Z = 0, XYZ;

    for (i = 0, lambda = 350.; lambda < 830.1; i++, lambda += 5.) {
        double Me = daylight_spect(lambda, _m1, _m2);
        X += Me * cie_colour_match_jd[i][0];
        Y += Me * cie_colour_match_jd[i][1];
        Z += Me * cie_colour_match_jd[i][2];
    }

    XYZ = (X + Y + Z);
    x = X / XYZ;
    y = Y / XYZ;
    z = Z / XYZ;
}

void spectrum_to_xyz_blackbody(double _temp, double &x, double &y, double &z)
{
    int i;
    double lambda, X = 0, Y = 0, Z = 0, XYZ;

    for (i = 0, lambda = 350.; lambda < 830.1; i++, lambda += 5.) {
        double Me = blackbody_spect(lambda, _temp);
        X += Me * cie_colour_match_jd[i][0];
        Y += Me * cie_colour_match_jd[i][1];
        Z += Me * cie_colour_match_jd[i][2];

        assert(X >= 0);
        assert(Y >= 0);
        assert(Z >= 0);
    }

    XYZ = (X + Y + Z);
    x = X / XYZ;
    y = Y / XYZ;
    z = Z / XYZ;

    assert(x >= 0);
    assert(y >= 0);
    assert(z >= 0);
}

void temp2mulxyz(double temp, double &Xxyz, double &Zxyz)
{
    double x, y, z;

    // otherwise we use the Temp+Green generic solution
    if (temp <= INITIALBLACKBODY) {
        // if temperature is between 2000K and 4000K we use blackbody, because
        // there will be no Daylight reference below 4000K... of course, the
        // previous version of RT used the "magical" but wrong formula of
        // U.Fuchs (Ufraw).
        spectrum_to_xyz_blackbody(temp, x, y, z);
    } else {
        // from 4000K up to 25000K: using the D illuminant (daylight) which is
        // standard
        double x_D, y_D;

        if (temp <= 7000) {
            x_D = -4.6070e9 / (temp * temp * temp) + 2.9678e6 / (temp * temp) +
                  0.09911e3 / temp + 0.244063;
        } else if (temp <= 25000) {
            x_D = -2.0064e9 / (temp * temp * temp) + 1.9018e6 / (temp * temp) +
                  0.24748e3 / temp + 0.237040;
        } else /*if (temp > 25000)*/ {
            x_D = -2.0064e9 / (temp * temp * temp) + 1.9018e6 / (temp * temp) +
                  0.24748e3 / temp + 0.237040 -
                  ((temp - 25000) / 25000) *
                      0.025; // Jacques empirical adjustment for very high temp
                             // (underwater !)
        }

        y_D = -3.0 * x_D * x_D + 2.87 * x_D - 0.275; // modify blue / red action
        // calculate D -daylight in function of s0, s1, s2 and temp ==> x_D y_D
        // S(lamda)=So(lambda)+m1*s1(lambda)+m2*s2(lambda)
        double interm = 0.0241 + 0.2562 * x_D - 0.734 * y_D;
        double m1 = (-1.3515 - 1.7703 * x_D + 5.9114 * y_D) / interm;
        double m2 = (0.03 - 31.4424 * x_D + 30.0717 * y_D) / interm;
        spectrum_to_xyz_daylight(m1, m2, x, y, z);
    }

    Xxyz = x / y;
    Zxyz = (1.0 - x - y) / y;
}

} // namespace

ColorTemp::ColorTemp()
    : mode_(MULTIPLIERS), temp(-1.), green(-1.), equal(1.), clipped_(false)
{
    mult_[0] = mult_[1] = mult_[2] = 1;
}

ColorTemp::ColorTemp(double e)
    : mode_(TEMP_TINT), temp(-1.), green(-1.), equal(e), clipped_(false)
{
    mult_[0] = mult_[1] = mult_[2] = 1;
}

ColorTemp::ColorTemp(double t, double g, double e, const std::string &m)
    : mode_(TEMP_TINT), temp(t), green(g), equal(e), mult_{0}, clipped_(false)
{
    clip(temp, green, equal);
}

void ColorTemp::clip(double &temp, double &green) const
{
    if (temp < MINTEMP || temp > MAXTEMP) {
        clipped_ = true;
    }
    temp = rtengine::LIM(temp, MINTEMP, MAXTEMP);
    if (green < MINGREEN || green > MAXGREEN) {
        clipped_ = true;
    }
    green = rtengine::LIM(green, MINGREEN, MAXGREEN);
}

void ColorTemp::clip(double &temp, double &green, double &equal) const
{
    clip(temp, green);
    equal = rtengine::LIM(equal, MINEQUAL, MAXEQUAL);
}

ColorTemp::ColorTemp(double mulr, double mulg, double mulb, double e)
    : mode_(TEMP_TINT), equal(e), mult_{0}, clipped_(false)
{
    mul2temp(mulr, mulg, mulb, equal, temp, green);
}

ColorTemp::ColorTemp(double mulr, double mulg, double mulb)
    : mode_(MULTIPLIERS), temp(-1), green(-1), equal(1), clipped_(false)
{
    mult_[0] = mulr;
    mult_[1] = mulg;
    mult_[2] = mulb;
    mul2temp(mulr, mulg, mulb, equal, temp, green);
}

void ColorTemp::mul2temp(const double rmul, const double gmul,
                         const double bmul, const double equal, double &temp,
                         double &green) const
{
    double maxtemp = MAXTEMP, mintemp = MINTEMP;
    double tmpr, tmpg, tmpb;
    temp = (maxtemp + mintemp) / 2;

    while (maxtemp - mintemp > 1) {
        temp2mul(temp, 1.0, equal, tmpr, tmpg, tmpb);

        if (tmpb / tmpr > bmul / rmul) {
            maxtemp = temp;
        } else {
            mintemp = temp;
        }

        temp = (maxtemp + mintemp) / 2;
    }

    green = (tmpg / tmpr) / (gmul / rmul);
    clip(temp, green);
}

void ColorTemp::temp2mul(double temp, double green, double equal, double &rmul,
                         double &gmul, double &bmul) const
{
    clip(temp, green, equal);
    double Xwb, Zwb;
    temp2mulxyz(temp, Xwb, Zwb);

    float adj = 1.f;

    if (equal < 0.9999 || equal > 1.0001) {
        adj = (100.f + (1000.f - (1000.f * (float)equal)) / 20.f) / 100.f;
    }

    // recalculate channels multipliers with new values of XYZ tue to
    // whitebalance
    rmul = std::max(sRGBd65_xyz[0][0] * Xwb * adj + sRGBd65_xyz[0][1] +
                        sRGBd65_xyz[0][2] * Zwb / adj,
                    1e-05); // Jacques' empirical modification 5/2013
    gmul = std::max(sRGBd65_xyz[1][0] * Xwb + sRGBd65_xyz[1][1] +
                        sRGBd65_xyz[1][2] * Zwb,
                    1e-05);
    bmul = std::max(sRGBd65_xyz[2][0] * Xwb * adj + sRGBd65_xyz[2][1] +
                        sRGBd65_xyz[2][2] * Zwb / adj,
                    1e-05);
    //};
    gmul /= green;
    // printf("rmul=%f gmul=%f bmul=%f\n",rmul, gmul, bmul);
    double maxRGB = rtengine::max(rmul, gmul, bmul);

    rmul /= maxRGB;
    gmul /= maxRGB;
    bmul /= maxRGB;

    assert(rmul >= 0);
    assert(gmul >= 0);
    assert(bmul >= 0);

    rmul /= gmul;
    bmul /= gmul;
    gmul = 1.0;
}

void ColorTemp::update(const double rmul, const double gmul, const double bmul,
                       const double equal)
{
    mode_ = TEMP_TINT;
    this->equal = equal;
    clipped_ = false;
    mul2temp(rmul, gmul, bmul, this->equal, temp, green);
}

void ColorTemp::useDefaults(const double equal)
{
    mode_ = TEMP_TINT;
    temp = 6504; // Values copied from procparams.cc
    green = 1.0;
    this->equal = equal;
    clipped_ = false;
}

void ColorTemp::getMultipliers(double &mulr, double &mulg, double &mulb) const
{
    if (mode_ == TEMP_TINT) {
        temp2mul(temp, green, equal, mulr, mulg, mulb);
    } else {
        mulr = mult_[0];
        mulg = mult_[1];
        mulb = mult_[2];
    }
}

bool ColorTemp::operator==(const ColorTemp &other) const
{
    if (mode_ == other.mode_) {
        if (mode_ == TEMP_TINT) {
            return fabs(temp - other.temp) < 1e-10 &&
                   fabs(green - other.green) < 1e-10;
        } else {
            return mult_ == other.mult_;
        }
    } else {
        return false;
    }
}

bool ColorTemp::operator!=(const ColorTemp &other) const
{
    return !(*this == other);
}

} // namespace rtengine
