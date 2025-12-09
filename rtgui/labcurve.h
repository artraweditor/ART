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
#ifndef _LABCURVE_H_
#define _LABCURVE_H_

#include "adjuster.h"
#include "colorprovider.h"
#include "curveeditor.h"
#include "curveeditorgroup.h"
#include "toolpanel.h"
#include <gtkmm.h>

class LabCurve: public ToolParamBlock,
                public AdjusterListener,
                public FoldableToolPanel,
                public CurveListener,
                public ColorProvider {

protected:
    CurveEditorGroup *curveEditorG;
    Adjuster *brightness;
    Adjuster *contrast;
    Adjuster *chromaticity;
    DiagonalCurveEditor *lshape;
    DiagonalCurveEditor *ashape;
    DiagonalCurveEditor *bshape;

    rtengine::procparams::LabCurveParams initial_params;

public:
    LabCurve();
    ~LabCurve() override;

    void read(const rtengine::procparams::ProcParams *pp) override;
    void write(rtengine::procparams::ProcParams *pp) override;
    void
    setDefaults(const rtengine::procparams::ProcParams *defParams) override;
    void autoOpenCurve() override;
    void setEditProvider(EditDataProvider *provider) override;
    void trimValues(rtengine::procparams::ProcParams *pp) override;

    void curveChanged(CurveEditor *ce) override;
    void adjusterChanged(Adjuster *a, double newval) override;
    void adjusterAutoToggled(Adjuster *a, bool newval) override;

    void updateCurveBackgroundHistogram(
        const LUTu &histToneCurve, const LUTu &histLCurve,
        const LUTu &histCCurve, const LUTu &histLCAM, const LUTu &histCCAM,
        const LUTu &histRed, const LUTu &histGreen, const LUTu &histBlue,
        const LUTu &histLuma, const LUTu &histLRETI);

    void enabledChanged() override;
    void toolReset(bool to_initial) override;
    void registerShortcuts(ToolShortcutManager *mgr) override;
};

#endif
