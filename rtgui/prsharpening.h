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
#ifndef _PRSHARPENING_H_
#define _PRSHARPENING_H_

#include "adjuster.h"
#include "thresholdadjuster.h"
#include "toolpanel.h"
#include <gtkmm.h>

class PrSharpening: public ToolParamBlock,
                    public ThresholdAdjusterListener,
                    public AdjusterListener,
                    public FoldableToolPanel {

protected:
    Adjuster *contrast;
    MyComboBoxText *method;
    Adjuster *dradius;
    Adjuster *damount;
    Gtk::VBox *usm;
    Gtk::VBox *rld;

    Adjuster *radius;
    Adjuster *amount;
    Adjuster *eradius;
    Adjuster *etolerance;
    Adjuster *hcamount;
    Gtk::VBox *edgebin;
    Gtk::VBox *hcbin;
    Gtk::VBox *edgebox;
    Gtk::VBox *hcbox;
    ThresholdAdjuster *threshold;
    Gtk::CheckButton *edgesonly;
    bool lastEdgesOnly;
    sigc::connection eonlyConn;
    Gtk::CheckButton *halocontrol;
    bool lastHaloControl;
    sigc::connection hcConn;
    rtengine::ProcEvent EvPrShrContrast;

    rtengine::procparams::SharpeningParams initial_params;

public:
    PrSharpening();
    ~PrSharpening() override;

    void read(const rtengine::procparams::ProcParams *pp) override;
    void write(rtengine::procparams::ProcParams *pp) override;
    void
    setDefaults(const rtengine::procparams::ProcParams *defParams) override;
    void adjusterChanged(Adjuster *a, double newval) override;
    void adjusterAutoToggled(Adjuster *a, bool newval) override;
    void enabledChanged() override;
    void edgesonly_toggled();
    void halocontrol_toggled();
    void method_changed();

    void adjusterChanged(ThresholdAdjuster *a, double newBottom,
                         double newTop) override;
    void adjusterChanged(ThresholdAdjuster *a, double newBottomLeft,
                         double newTopLeft, double newBottomRight,
                         double newTopRight) override;
    void adjusterChanged(ThresholdAdjuster *a, int newBottom,
                         int newTop) override;
    void adjusterChanged(ThresholdAdjuster *a, int newBottomLeft,
                         int newTopLeft, int newBottomRight,
                         int newTopRight) override;
    void adjusterChanged2(ThresholdAdjuster *a, int newBottomL, int newTopL,
                          int newBottomR, int newTopR) override;

    void trimValues(rtengine::procparams::ProcParams *pp) override;

    void toolReset(bool to_initial) override;
};

#endif
