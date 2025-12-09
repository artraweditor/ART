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

#include "../rtengine/rawimage.h"
#include "adjuster.h"
#include "guiutils.h"
#include "toolpanel.h"
#include <gtkmm.h>
#include <memory>

class FFProvider {
public:
    virtual ~FFProvider() {}
    virtual rtengine::RawImage *getFF() = 0;
    virtual bool hasEmbeddedFF() = 0;
    virtual Glib::ustring GetCurrentImageFilePath() = 0;
    // add other info here
};

class FlatField: public ToolParamBlock,
                 public AdjusterListener,
                 public FoldableToolPanel,
                 public rtengine::FlatFieldAutoClipListener {

protected:
    MyFileChooserButton *flatFieldFile;
    Gtk::Label *ffLabel;
    Gtk::Label *ffInfo;
    Gtk::Button *flatFieldFileReset;
    Gtk::CheckButton *flatFieldAutoSelect;
    Adjuster *flatFieldClipControl;
    Adjuster *flatFieldBlurRadius;
    MyComboBoxText *flatFieldBlurType;
    Gtk::HBox *hbff;
    Gtk::VBox *vbff;
    bool ffChanged;
    bool lastFFAutoSelect;
    bool lastFFAutoClipCtrl;
    FFProvider *ffp;
    sigc::connection flatFieldFileconn, flatFieldAutoSelectconn,
        flatFieldBlurTypeconn;
    Glib::ustring lastShortcutPath;
    bool b_filter_asCurrent;
    bool israw;
    Gtk::CheckButton *embedded;

    IdleRegister idle_register;

    rtengine::procparams::RAWParams initial_params;

    rtengine::ProcEvent EvEmbedded;

public:
    FlatField();
    ~FlatField() override;

    void read(const rtengine::procparams::ProcParams *pp) override;
    void write(rtengine::procparams::ProcParams *pp) override;
    void trimValues(rtengine::procparams::ProcParams *pp) override;
    void
    setDefaults(const rtengine::procparams::ProcParams *defParams) override;

    void adjusterChanged(Adjuster *a, double newval) override;
    void adjusterAutoToggled(Adjuster *a, bool newval) override;
    void flatFieldFileChanged();
    void flatFieldFile_Reset();
    void flatFieldAutoSelectChanged();
    void flatFieldBlurTypeChanged();
    void setShortcutPath(const Glib::ustring &path);
    void setFFProvider(FFProvider *p) { ffp = p; }
    void flatFieldAutoClipValueChanged(int n = 0) override;
    void embeddedToggled();

    void toolReset(bool to_initial) override;
};
