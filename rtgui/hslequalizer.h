/* -*- C++ -*-
 *
 *  This file is part of RawTherapee.
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
 *
 *  2010 Ilya Popov <ilia_popov@rambler.ru>
 */

#pragma once

#include "adjuster.h"
#include "colorprovider.h"
#include "curveeditor.h"
#include "curveeditorgroup.h"
#include "guiutils.h"
#include "toolpanel.h"
#include <gtkmm.h>

class HSLEqualizer: public ToolParamBlock,
                    public FoldableToolPanel,
                    public CurveListener,
                    public ColorProvider,
                    public AdjusterListener {
public:
    HSLEqualizer();
    ~HSLEqualizer() override;

    void read(const rtengine::procparams::ProcParams *pp) override;
    void write(rtengine::procparams::ProcParams *pp) override;
    void curveChanged(CurveEditor *ce) override;
    void setEditProvider(EditDataProvider *provider) override;
    void autoOpenCurve() override;
    void colorForValue(double valX, double valY,
                       enum ColorCaller::ElemType elemType, int callerId,
                       ColorCaller *caller) override;

    void enabledChanged() override;
    void adjusterChanged(Adjuster *a, double newval) override;
    void adjusterAutoToggled(Adjuster *a, bool newval) override;

    void setDefaults(const rtengine::procparams::ProcParams *def) override;
    void toolReset(bool to_initial) override;

private:
    rtengine::ProcEvent EvHSLSmoothing;

    CurveEditorGroup *curveEditorG;
    FlatCurveEditor *hshape;
    FlatCurveEditor *sshape;
    FlatCurveEditor *lshape;
    Adjuster *smoothing;

    rtengine::procparams::HSLEqualizerParams initial_params;
    std::vector<double> default_flat_curve_;
};
