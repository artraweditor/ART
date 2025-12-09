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
#ifndef _RGBCURVES_H_
#define _RGBCURVES_H_

#include "adjuster.h"
#include "colorprovider.h"
#include "curveeditor.h"
#include "curveeditorgroup.h"
#include "toolpanel.h"
#include <gtkmm.h>

class RGBCurves: public ToolParamBlock,
                 public FoldableToolPanel,
                 public CurveListener,
                 public ColorProvider,
                 public CurveBackgroundProvider {
private:
    CurveEditorGroup *curveEditorG;
    DiagonalCurveEditor *Rshape;
    DiagonalCurveEditor *Gshape;
    DiagonalCurveEditor *Bshape;

    rtengine::procparams::RGBCurvesParams initial_params;

public:
    RGBCurves();
    ~RGBCurves() override;

    void read(const rtengine::procparams::ProcParams *pp) override;
    void write(rtengine::procparams::ProcParams *pp) override;
    void setEditProvider(EditDataProvider *provider) override;
    void autoOpenCurve() override;

    void curveChanged(CurveEditor *ce) override;
    void updateCurveBackgroundHistogram(
        const LUTu &histToneCurve, const LUTu &histLCurve,
        const LUTu &histCCurve, const LUTu &histLCAM, const LUTu &histCCAM,
        const LUTu &histRed, const LUTu &histGreen, const LUTu &histBlue,
        const LUTu &histLuma, const LUTu &histLRETI);
    void enabledChanged() override;

    void setDefaults(const rtengine::procparams::ProcParams *def) override;
    void toolReset(bool to_initial) override;

    void renderCurveBackground(int caller_id,
                               Glib::RefPtr<Gtk::StyleContext> style,
                               Cairo::RefPtr<Cairo::Context> cr, double x,
                               double y, double w, double h) override;
};

#endif
