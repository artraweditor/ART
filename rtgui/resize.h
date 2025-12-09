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
#include "guiutils.h"
#include "toolpanel.h"
#include <gtkmm.h>

class Resize final: public ToolParamBlock,
                    public AdjusterListener,
                    public FoldableToolPanel,
                    public rtengine::SizeListener {
public:
    Resize();
    ~Resize() override;

    Gtk::Box *getPackBox() { return packBox; }

    void read(const rtengine::procparams::ProcParams *pp) override;
    void write(rtengine::procparams::ProcParams *pp) override;
    void
    setDefaults(const rtengine::procparams::ProcParams *defParams) override;
    void adjusterChanged(Adjuster *a, double newval) override;
    void adjusterAutoToggled(Adjuster *a, bool newval) override;
    void entryWChanged();
    void entryHChanged();
    void appliesToChanged();
    void methodChanged();
    void specChanged();
    void update(bool isCropped, int cw, int ch, int ow = 0, int oh = 0);
    void setGUIFromCrop(bool isCropped, int cw, int ch);
    void sizeChanged(int w, int h, int ow, int oh) override;
    void setDimensions();
    void enabledChanged() override;

    void trimValues(rtengine::procparams::ProcParams *pp) override;

    void toolReset(bool to_initial) override;

private:
    void fitBoxScale();
    int getComputedWidth();
    int getComputedHeight();
    void notifyBBox();
    void updateGUI();
    void allowUpscalingChanged();
    void ppiChanged();
    void unitChanged();
    void updateInfoLabels();
    void setRanges();

    double from_px(int p, rtengine::procparams::ResizeParams::Unit u);
    double from_px(int p);
    int to_px(double p, rtengine::procparams::ResizeParams::Unit u);
    int to_px(double p);

    rtengine::ProcEvent EvResizeAllowUpscaling;
    rtengine::ProcEvent EvUnit;
    rtengine::ProcEvent EvPPI;
    Adjuster *scale;
    Gtk::VBox *sizeBox;
    MyComboBoxText *appliesTo;
    MyComboBoxText *spec;
    MySpinButton *w;
    MySpinButton *h;
    Gtk::CheckButton *allowUpscaling;
    MyComboBoxText *unit;
    MySpinButton *ppi;
    Gtk::Label *size_info_1;
    Gtk::Label *size_info_2;
    Gtk::HBox *unitBox;
    Gtk::VBox *endBox;
    int maxw, maxh;
    int cropw, croph;
    rtengine::procparams::ResizeParams::Unit prev_unit;
    sigc::connection sconn, aconn, wconn, hconn;
    sigc::connection unitconn, ppiconn;
    bool wDirty, hDirty;
    ToolParamBlock *packBox;
    IdleRegister idle_register;

    static constexpr int MAX_SCALE =
        16; // 16 to match the main preview max scale of 1600%

    rtengine::procparams::ResizeParams initial_params;
};
