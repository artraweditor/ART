/** -*- C++ -*-
 *
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2017 Alberto Griggio <alberto.griggio@gmail.com>
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
#include "toolpanel.h"
#include <gtkmm.h>

class LogEncoding: public ToolParamBlock,
                   public AdjusterListener,
                   public rtengine::AutoLogListener,
                   public FoldableToolPanel {
public:
    LogEncoding();

    void read(const rtengine::procparams::ProcParams *pp) override;
    void write(rtengine::procparams::ProcParams *pp) override;
    void
    setDefaults(const rtengine::procparams::ProcParams *defParams) override;

    void adjusterChanged(Adjuster *a, double newval) override;
    void adjusterAutoToggled(Adjuster *a, bool newval) override;
    void enabledChanged() override;

    void logEncodingChanged(
        const rtengine::procparams::LogEncodingParams &params) override;
    void autocomputeToggled();

    void toolReset(bool to_initial) override;
    void registerShortcuts(ToolShortcutManager *mgr) override;

private:
    void satcontrolChanged();

    Gtk::ToggleButton *autocompute;
    Adjuster *gain;
    Adjuster *targetGray;
    Adjuster *blackEv;
    Adjuster *whiteEv;
    Adjuster *regularization;
    Gtk::CheckButton *satcontrol;
    Adjuster *highlightCompression;

    rtengine::ProcEvent EvEnabled;
    rtengine::ProcEvent EvAuto;
    rtengine::ProcEvent EvAutoGainOn;
    rtengine::ProcEvent EvAutoGainOff;
    rtengine::ProcEvent EvAutoBatch;
    rtengine::ProcEvent EvGain;
    rtengine::ProcEvent EvGainAuto;
    rtengine::ProcEvent EvTargetGray;
    rtengine::ProcEvent EvBlackEv;
    rtengine::ProcEvent EvWhiteEv;
    rtengine::ProcEvent EvRegularization;
    rtengine::ProcEvent EvSatControl;
    rtengine::ProcEvent EvHLCompression;

    sigc::connection autoconn;

    rtengine::procparams::LogEncodingParams initial_params;
};
