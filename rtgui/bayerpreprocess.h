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
#ifndef _BAYERPREPROCESS_H_
#define _BAYERPREPROCESS_H_

#include "../rtengine/rawimage.h"
#include "adjuster.h"
#include "toolpanel.h"
#include <gtkmm.h>

class BayerPreProcess: public ToolParamBlock,
                       public AdjusterListener,
                       public FoldableToolPanel {

protected:
    Adjuster *lineDenoise;
    MyComboBoxText *lineDenoiseDirection;
    Adjuster *greenEqThreshold;
    Gtk::CheckButton *pdafLinesFilter;
    Gtk::CheckButton *dynamicRowNoiseFilter;

    rtengine::ProcEvent EvLineDenoiseDirection;
    rtengine::ProcEvent EvPDAFLinesFilter;
    rtengine::ProcEvent EvDynamicRowNoiseFilter;

    rtengine::procparams::RAWParams::BayerSensor initial_params;

public:
    BayerPreProcess();

    void read(const rtengine::procparams::ProcParams *pp) override;
    void write(rtengine::procparams::ProcParams *pp) override;
    void
    setDefaults(const rtengine::procparams::ProcParams *defParams) override;

    void adjusterChanged(Adjuster *a, double newval) override;
    void adjusterAutoToggled(Adjuster *a, bool newval) override;

    void hotDeadPixelChanged();
    void trimValues(rtengine::procparams::ProcParams *pp) override;
    void lineDenoiseDirectionChanged();
    void pdafLinesFilterChanged();
    void dynamicRowNoiseFilterChanged();

    void toolReset(bool to_initial) override;
};

#endif
