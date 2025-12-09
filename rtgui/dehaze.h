/** -*- C++ -*-
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
#pragma once

#include "adjuster.h"
#include "curveeditor.h"
#include "curveeditorgroup.h"
#include "mycurve.h"
#include "toolpanel.h"
#include <gtkmm.h>

class Dehaze: public ToolParamBlock,
              public AdjusterListener,
              public FoldableToolPanel,
              public CurveListener {
public:
    Dehaze();

    void read(const rtengine::procparams::ProcParams *pp) override;
    void write(rtengine::procparams::ProcParams *pp) override;
    void
    setDefaults(const rtengine::procparams::ProcParams *defParams) override;
    void adjusterChanged(Adjuster *a, double newval) override;
    void enabledChanged() override;
    void adjusterAutoToggled(Adjuster *a, bool newval) override {}
    void curveChanged() override;
    void autoOpenCurve() override;
    void setEditProvider(EditDataProvider *provider) override;

    void toolReset(bool to_initial) override;

private:
    void showDepthMapChanged();
    void luminanceChanged();

    FlatCurveEditor *strength;
    // Adjuster *strength;
    Adjuster *depth;
    Gtk::CheckButton *showDepthMap;
    MyComboBoxText *luminance;
    Adjuster *blackpoint;

    rtengine::ProcEvent EvDehazeEnabled;
    rtengine::ProcEvent EvDehazeStrength;
    rtengine::ProcEvent EvDehazeDepth;
    rtengine::ProcEvent EvDehazeShowDepthMap;
    rtengine::ProcEvent EvDehazeLuminance;
    rtengine::ProcEvent EvDehazeBlackpoint;

    rtengine::procparams::DehazeParams initial_params;
};
