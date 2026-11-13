/* -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright 2023 Alberto Griggio <alberto.griggio@gmail.com>
 *
 *  ART is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  ART is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with ART.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "../engine/clutparams.h"
#include "adjuster.h"
#include "colorprovider.h"
#include "curvelistener.h"
#include "edit.h"
#include "thresholdadjuster.h"
#include "toolpanel.h"
#include <gtkmm.h>

namespace art { namespace gui {


class CLUTParamsPanel: public Gtk::VBox,
                       public AdjusterListener,
                       public CurveListener,
                       public ColorProvider,
                       public ThresholdAdjusterListener {
public:
    CLUTParamsPanel();
    ~CLUTParamsPanel() override;

    void setParams(const art::engine::CLUTParamDescriptorList &params);
    void setValue(const art::engine::CLUTParamValueMap &val);
    art::engine::CLUTParamValueMap getValue() const;

    /// Pipette support for the curves that declare it (see @ART-pipette in
    /// the CTL scripts). base is the payload of the edit IDs (see
    /// art::utils::ctlEditBase), to which the curve id is added
    void setEditProvider(EditDataProvider *provider);
    void setEditIDBase(unsigned int base);

    sigc::signal<void> signal_changed() { return sig_changed_; }

    void adjusterChanged(Adjuster *a, double v) override { emit_signal(); }
    void adjusterAutoToggled(Adjuster *a, bool v) override {}
    void curveChanged() override { emit_signal(); }
    void curveChanged(CurveEditor *ce) override { emit_signal(); }

    // hue/saturation parameters (float[2] in CTL scripts)
    void adjusterChanged(ThresholdAdjuster *a, double newBottom,
                         double newTop) override
    {
        emit_signal();
    }
    void adjusterChanged(ThresholdAdjuster *a, double newBottomLeft,
                         double newTopLeft, double newBottomRight,
                         double newTopRight) override {}
    void adjusterChanged(ThresholdAdjuster *a, int newBottom,
                         int newTop) override
    {
        emit_signal();
    }
    void adjusterChanged(ThresholdAdjuster *a, int newBottomLeft, int newTopLeft,
                         int newBottomRight, int newTopRight) override {}
    void adjusterChanged2(ThresholdAdjuster *a, int newBottomL, int newTopL,
                          int newBottomR, int newTopR) override {}
    void colorForValue(double valX, double valY,
                       enum ColorCaller::ElemType elemType, int callerId,
                       ColorCaller *caller) override;

    Gtk::SizeRequestMode get_request_mode_vfunc() const override;

private:
    void emit_signal();
    void apply_preset();
    void unsubscribe_pipettes();
    void setup_pipette(const art::engine::CLUTParamDescriptor &d,
                       CurveEditor *ce);

    sigc::signal<void> sig_changed_;
    bool sig_blocked_;
    art::engine::CLUTParamDescriptorList params_;
    std::vector<void *> widgets_;
    sigc::connection presets_conn_;
    MyComboBoxText *presets_combo_;
    EditDataProvider *edit_provider_;
    unsigned int edit_base_;
};


} } // namespace art::gui
