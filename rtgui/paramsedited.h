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
#pragma once

#include <glibmm.h>
#include <vector>

class ParamsEdited {
public:
    enum { False = 0, True, Undef };

    bool blackwhite;
    bool cacorrection;
    bool chmixer;
    bool coarse;
    bool commonTrans;
    bool crop;
    bool darkframe;
    bool defringe;
    bool dehaze;
    bool demosaic;
    bool denoise;
    bool distortion;
    bool exif;
    bool exposure;
    bool fattal;
    bool filmNegative;
    bool filmSimulation;
    bool flatfield;
    bool general;
    bool gradient;
    bool grain;
    bool hotDeadPixelFilter;
    bool hsl;
    bool icm;
    bool impulseDenoise;
    bool iptc;
    bool labCurve;
    bool lensProf;
    bool logenc;
    bool metadata;
    bool pcvignette;
    bool perspective;
    bool prsharpening;
    bool rawBlack;
    bool rawCA;
    bool rawPreprocessing;
    bool rawWhite;
    bool resize;
    bool rgbCurves;
    bool rotate;
    bool saturation;
    bool sharpening;
    bool softlight;
    bool spot;
    bool toneCurve;
    bool toneEqualizer;
    bool vignetting;
    bool wb;

    unsigned colorcorrection;
    unsigned smoothing;
    unsigned localContrast;
    unsigned textureBoost;

    explicit ParamsEdited(bool value = false);

    void set(bool v);
    void set_append(bool v);
};
