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
#ifndef _BLACKWHITE_H_
#define _BLACKWHITE_H_

#include "adjuster.h"
#include "colorprovider.h"
#include "curveeditor.h"
#include "curveeditorgroup.h"
#include "guiutils.h"
#include "mycurve.h"
#include "thresholdadjuster.h"
#include "toolpanel.h"
#include <gtkmm.h>

class BlackWhite final: public ToolParamBlock,
                        public AdjusterListener,
                        public FoldableToolPanel,
                        public ColorProvider,
                        public ThresholdAdjusterListener {
public:
    BlackWhite();
    ~BlackWhite() override;

    void read(const rtengine::procparams::ProcParams *pp) override;
    void write(rtengine::procparams::ProcParams *pp) override;
    void
    setDefaults(const rtengine::procparams::ProcParams *defParams) override;
    void neutral_pressed();

    void updateRGBLabel(bool from_preset = true);
    void adjusterChanged(Adjuster *a, double newval) override;
    void adjusterAutoToggled(Adjuster *a, bool newval) override;
    void trimValues(rtengine::procparams::ProcParams *pp) override;
    void enabledChanged() override;
    void filterChanged();
    void settingChanged();

    Glib::ustring getSettingString();
    Glib::ustring getFilterString();

    void adjusterChanged(ThresholdAdjuster *a, double newBottom,
                         double newTop) override;
    void adjusterChanged(ThresholdAdjuster *a, double newBottomLeft,
                         double newTopLeft, double newBottomRight,
                         double newTopRight) override
    {
    }
    void adjusterChanged(ThresholdAdjuster *a, int newBottom,
                         int newTop) override
    {
    }
    void adjusterChanged(ThresholdAdjuster *a, int newBottomLeft,
                         int newTopLeft, int newBottomRight,
                         int newTopRight) override
    {
    }
    void adjusterChanged2(ThresholdAdjuster *a, int newBottomL, int newTopL,
                          int newBottomR, int newTopR) override
    {
    }

    void colorForValue(double valX, double valY,
                       enum ColorCaller::ElemType elemType, int callerId,
                       ColorCaller *caller) override;

    void toolReset(bool to_initial) override;

private:
    void showFilter();
    void hideFilter();

    rtengine::ProcEvent EvColorCast;

    Gtk::Label *RGBLabels;

    Adjuster *mixerRed;
    Adjuster *mixerGreen;
    Adjuster *mixerBlue;
    Adjuster *gammaRed;
    Adjuster *gammaGreen;
    Adjuster *gammaBlue;
    ThresholdAdjuster *colorCast;
    Gtk::HBox *filterHBox;
    Gtk::HSeparator *filterSep;
    MyComboBoxText *filter;
    sigc::connection filterconn;
    Gtk::HBox *settingHBox;
    MyComboBoxText *setting;
    sigc::connection settingconn;
    Gtk::VBox *mixerVBox;
    Gtk::Frame *gammaFrame;

    Gtk::Image *imgIcon[11];

    sigc::connection enaccconn, tcmodeconn, tcmodeconn2, autoconn, neutralconn;

    double nextredbw;
    double nextgreenbw;
    double nextbluebw;
    int nextcount = 0;

    IdleRegister idle_register;

    rtengine::procparams::BlackWhiteParams initial_params;
};

#endif
