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
#include "lensgeomlistener.h"
#include "toolpanel.h"
#include <gtkmm.h>

class ControlLineManager;

class PerspCorrectionPanelListener {
public:
    virtual ~PerspCorrectionPanelListener() = default;
    virtual void controlLineEditModeChanged(bool active) = 0;
};

class PerspCorrection: public ToolParamBlock,
                       public AdjusterListener,
                       public FoldableToolPanel {
public:
    PerspCorrection();

    void read(const rtengine::procparams::ProcParams *pp) override;
    void write(rtengine::procparams::ProcParams *pp) override;
    void
    setDefaults(const rtengine::procparams::ProcParams *defParams) override;
    void adjusterChanged(Adjuster *a, double newval) override;
    void adjusterAutoToggled(Adjuster *a, bool newval) override;
    void trimValues(rtengine::procparams::ProcParams *pp) override;

    void setLensGeomListener(LensGeomListener *l) { lgl = l; }
    void setPerspCorrectionPanelListener(PerspCorrectionPanelListener *l)
    {
        panel_listener = l;
    }
    void autoPressed(Gtk::Button *which);

    void setRawMeta(bool raw, const rtengine::FramesMetaData *meta);

    void toolReset(bool to_initial) override;

    void setEditProvider(EditDataProvider *provider) override;
    void switchOffEditMode();
    void lineChanged();

    void setControlLineEditMode(bool active);
    void requestApplyControlLines();

private:
    void do_set_metadata(const rtengine::FramesMetaData *meta);
    void applyControlLines();
    void linesApplyButtonPressed();
    void linesEditButtonPressed();
    void linesEraseButtonPressed();

    Adjuster *horiz;
    Adjuster *vert;
    Adjuster *angle;
    Adjuster *shear;
    Adjuster *flength;
    Adjuster *cropfactor;
    Adjuster *aspect;
    Gtk::Button *auto_horiz;
    Gtk::Button *auto_vert;
    Gtk::Button *auto_both;
    LensGeomListener *lgl;
    PerspCorrectionPanelListener *panel_listener;

    std::unique_ptr<ControlLineManager> lines;
    Gtk::Button *lines_button_apply;
    Gtk::ToggleButton *lines_button_edit;
    Gtk::Button *lines_button_erase;

    rtengine::ProcEvent EvPerspCorrLens;
    rtengine::ProcEvent EvPerspRender;
    rtengine::ProcEvent EvPerspControlLines;
    const rtengine::FramesMetaData *metadata;

    rtengine::procparams::PerspectiveParams initial_params;
};
