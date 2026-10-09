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
#include "clutparamspanel.h"
#include "guiutils.h"
#include "toolpanel.h"
#include <glibmm.h>
#include <gtkmm.h>
#include <memory>

namespace art { namespace gui {


class ClutComboBox: public SearchableTreeCombo {
public:
    explicit ClutComboBox(const std::vector<Glib::ustring> &paths);
    // int fillFromDir (const Glib::ustring& path);
    int foundClutsCount() const;
    std::pair<Glib::ustring, Glib::ustring> getSelectedClut();
    void setSelectedClut(Glib::ustring filename);

    static void cleanup();

private:
    class ClutColumns: public Gtk::TreeModel::ColumnRecord {
    public:
        Gtk::TreeModelColumn<Glib::ustring> label;
        Gtk::TreeModelColumn<Glib::ustring> clutFilename;
        ClutColumns();
    };

    class ClutModel {
    public:
        Glib::RefPtr<Gtk::TreeStore> m_model;
        ClutColumns m_columns;
        int count;
        explicit ClutModel(const std::vector<Glib::ustring> &paths);
        int parseDir(const std::vector<Glib::ustring> &paths);
    };

    Glib::RefPtr<Gtk::TreeStore> &m_model();
    ClutColumns &m_columns();

    Gtk::TreeIter findRowByClutFilename(Gtk::TreeModel::Children childs,
                                        Glib::ustring filename);

    // we use a shared TreeModel for all the combo boxes, to save time
    // (no need to reparse the clut dir multiple times)
    static std::unique_ptr<ClutModel> cm; 
};

class FilmSimulation: public ToolParamBlock,
                      public AdjusterListener,
                      public FoldableToolPanel {
public:
    FilmSimulation();

    void adjusterChanged(Adjuster *a, double newval) override;
    void adjusterAutoToggled(Adjuster *a, bool newval) override;
    void read(const art::engine::procparams::ProcParams *pp) override;
    void write(art::engine::procparams::ProcParams *pp) override;
    void trimValues(art::engine::procparams::ProcParams *pp) override;

    void setDefaults(const art::engine::procparams::ProcParams *pp) override;
    void toolReset(bool to_initial) override;

private:
    void onClutSelected();
    void onClutParamsChanged();
    void enabledChanged() override;
    void updateDisable(bool value);
    void afterToneCurveToggled();

    ClutComboBox *m_clutComboBox;
    sigc::connection m_clutComboBoxConn;
    // Glib::ustring m_oldClutFilename;

    Adjuster *m_strength;
    Gtk::CheckButton *after_tone_curve_;
    Gtk::HBox *after_tone_curve_box_;

    CLUTParamsPanel *lut_params_;

    art::engine::procparams::FilmSimulationParams initial_params;

    art::engine::ProcEvent EvAfterToneCurve;
    art::engine::ProcEvent EvClutParams;
};


} } // namespace art::gui
