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
#ifndef _PREPROCESS_H_
#define _PREPROCESS_H_

#include <gtkmm.h>
// #include "adjuster.h"
#include "../rtengine/rawimage.h"
#include "adjuster.h"
#include "guiutils.h"
#include "toolpanel.h"

class PreProcess: public ToolParamBlock,
                  public AdjusterListener,
                  public FoldableToolPanel {

protected:
    Gtk::CheckButton *hotPixel;
    Gtk::CheckButton *deadPixel;
    bool lastHot, lastDead;
    sigc::connection hpixelconn;
    sigc::connection dpixelconn;
    Adjuster *hdThreshold;

    rtengine::procparams::RAWParams initial_params;

public:
    PreProcess();

    void read(const rtengine::procparams::ProcParams *pp) override;
    void write(rtengine::procparams::ProcParams *pp) override;

    void hotPixelChanged();
    void deadPixelChanged();
    void adjusterChanged(Adjuster *a, double newval) override;
    void adjusterAutoToggled(Adjuster *a, bool newval) override;

    void setDefaults(const rtengine::procparams::ProcParams *def) override;
    void toolReset(bool to_initial) override;
};

#endif
