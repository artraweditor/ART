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
#include "paramsedited.h"

ParamsEdited::ParamsEdited(bool value) { set(value); }

void ParamsEdited::set(bool v)
{
    general = v;
    exposure = v;
    saturation = v;
    toneCurve = v;
    labCurve = v;
    localContrast = v;
    rgbCurves = v;
    sharpening = v;
    prsharpening = v;
    wb = v;
    defringe = v;
    denoise = v;
    textureBoost = v;
    fattal = v;
    logenc = v;
    impulseDenoise = v;
    toneEqualizer = v;
    crop = v;
    coarse = v;
    commonTrans = v;
    rotate = v;
    distortion = v;
    lensProf = v;
    perspective = v;
    gradient = v;
    pcvignette = v;
    cacorrection = v;
    vignetting = v;
    chmixer = v;
    blackwhite = v;
    hsl = v;
    resize = v;
    icm = v;
    demosaic = v;
    rawBlack = v;
    rawWhite = v;
    rawPreprocessing = v;
    hotDeadPixelFilter = v;
    darkframe = v;
    flatfield = v;
    rawCA = v;
    filmSimulation = v;
    softlight = v;
    dehaze = v;
    grain = v;
    smoothing = v;
    colorcorrection = v;
    filmNegative = v;
    metadata = v;
    exif = v;
    iptc = v;
    spot = v;
}

void ParamsEdited::set_append(bool v)
{
    if (v) {
        if (colorcorrection) {
            colorcorrection = Undef;
        }
        if (smoothing) {
            smoothing = Undef;
        }
        if (localContrast) {
            localContrast = Undef;
        }
        if (textureBoost) {
            textureBoost = Undef;
        }
    } else {
        if (colorcorrection == Undef) {
            colorcorrection = true;
        }
        if (smoothing == Undef) {
            smoothing = true;
        }
        if (localContrast == Undef) {
            localContrast = true;
        }
        if (textureBoost == Undef) {
            textureBoost = true;
        }
    }
}
