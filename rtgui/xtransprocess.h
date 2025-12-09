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
#ifndef _XTRANSPROCESS_H_
#define _XTRANSPROCESS_H_

#include "adjuster.h"
#include "checkbox.h"
#include "guiutils.h"
#include "toolpanel.h"
#include <gtkmm.h>

class XTransProcess: public ToolParamBlock,
                     public AdjusterListener,
                     public CheckBoxListener,
                     public FoldableToolPanel,
                     public rtengine::AutoContrastListener {

protected:
    MyComboBoxText *method;
    Gtk::HBox *borderbox;
    Adjuster *border;
    Adjuster *ccSteps;
    Gtk::VBox *dualDemosaicOptions;
    Adjuster *dualDemosaicContrast;
    bool lastAutoContrast;

    int oldSelection;
    sigc::connection methodconn;
    IdleRegister idle_register;

    rtengine::ProcEvent EvDemosaicBorder;
    rtengine::ProcEvent EvDemosaicAutoContrast;
    rtengine::ProcEvent EvDemosaicContrast;

    rtengine::procparams::RAWParams::XTransSensor initial_params;

public:
    XTransProcess();
    ~XTransProcess() override;

    void read(const rtengine::procparams::ProcParams *pp) override;
    void write(rtengine::procparams::ProcParams *pp) override;
    void
    setDefaults(const rtengine::procparams::ProcParams *defParams) override;

    void methodChanged();
    void autoContrastChanged(double autoContrast) override;
    void adjusterChanged(Adjuster *a, double newval) override;
    void checkBoxToggled(CheckBox *c, CheckValue newval) override;
    void adjusterAutoToggled(Adjuster *a, bool newval) override;

    void toolReset(bool to_initial) override;
};

#endif
