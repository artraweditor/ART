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

#include "adjuster.h"
#include "curveeditor.h"
#include "curveeditorgroup.h"
#include "guiutils.h"
#include "mycurve.h"
#include "toolpanel.h"
#include <gtkmm.h>

class Exposure: public ToolParamBlock,
                public AdjusterListener,
                public FoldableToolPanel {
protected:
    MyComboBoxText *hrmode;
    rtengine::ProcEvent EvBlack;
    rtengine::ProcEvent EvHRBlur;

    Gtk::HBox *abox;
    Adjuster *expcomp;
    Adjuster *black;
    Adjuster *hrblur;

    rtengine::procparams::ExposureParams initial_params;

    void showHRBlur();

public:
    Exposure();

    void read(const rtengine::procparams::ProcParams *pp) override;
    void write(rtengine::procparams::ProcParams *pp) override;
    void
    setDefaults(const rtengine::procparams::ProcParams *defParams) override;
    void trimValues(rtengine::procparams::ProcParams *pp) override;

    void adjusterChanged(Adjuster *a, double newval) override;
    void adjusterAutoToggled(Adjuster *a, bool newval) override;

    void setRaw(bool raw);

    void hrmodeChanged();

    void toolReset(bool to_initial) override;
    void registerShortcuts(ToolShortcutManager *mgr) override;
};
