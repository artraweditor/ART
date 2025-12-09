/* -*- C++ -*-
 *
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2019 Jean-Christophe FRISCH <natureh.510@gmail.com>
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

#include "../rtengine/procparams.h"
#include "../rtengine/tweakoperator.h"
#include "adjuster.h"
#include "edit.h"
#include "toolpanel.h"
#include <gtkmm.h>

/**
 * @brief Let the user create/edit/delete points for Spot Removal tool
 *
 * User Interface:
 *
 * For the rest of this documentation, T represent a "target" point (where the
 * image is edited) and S represent the "source" location (where the edition
 * takes its source data).
 *
 * When the edit button is active, all T points are shown by a small "dot". When
 * the user move the cursor over one of them, a circle is displayed to show the
 * radius of the brush, as well as a second circle representing the source data
 * (S point). The user can then use the left mouse button over the icon to drag
 * the T point. The left mouse button can be used over the S circle or the right
 * mouse button can be used over the T point to move the S point.
 *
 * Using the left mouse button over the circle of the T point will let the user
 * adjust its radius.
 *
 * Using the left mouse button over the feather circle will let the user adjust
 * its radius by setting a coefficient (0.0 = same radius than the inner circle
 * ; 1.0 = 2 times the inner radius).
 *
 * To create a new point, just move over a free area, and press the left mouse
 * button while holding the CTRL key. This will create a new S and T pair of
 * points. The CTRL key can be released, but keep the left mouse button pressed
 * and move away to position the S point.
 *
 * To delete a point, move your mouse over any of its geometry press the middle
 * or right mouse button (the point will be deleted on button release).
 */

class Spot: public ToolParamBlock,
            public FoldableToolPanel,
            public rtengine::TweakOperator,
            public EditSubscriber,
            public AdjusterListener {

private:
    enum class DraggedSide { NONE, SOURCE, TARGET };

    DraggedSide draggedSide; // tells which of source or target is being dragged
    int lastObject;          // current object that is hovered
    int activeSpot;          // currently active spot, being edited
    std::vector<rtengine::procparams::SpotEntry> spots; // list of edited spots
    OPIcon sourceIcon;          // to show the source location
    Circle sourceCircle;        // to show and change the Source radius
    Circle sourceMODisc;        // to change the Source position
    OPIcon targetIcon;          // to show the target location
    Circle targetCircle;        // to show and change the Target radius
    Circle targetMODisc;        // to change the Target position
    Circle sourceFeatherCircle; // to show the Feather radius at the Source
                                // position
    Circle targetFeatherCircle; // to show the Feather radius at the Target
                                // position
    Line link; // to show the link between the Source and Target position
    std::unique_ptr<Geometry>
        whole_area_rectangle; // dummy rectangle to always set a custom cursor

    RTImage lightPipelineOnImage;
    RTImage lightPipelineOffImage;

    OPIcon *getActiveSpotIcon();
    void lightPipelineToggled();
    void updateGeometry();
    void createGeometry();
    void addNewEntry();
    void deleteSelectedEntry();
    void resetPressed();

protected:
    Gtk::HBox *labelBox;
    Gtk::CheckButton *editedCheckBox;
    Gtk::Label *countLabel;
    Gtk::ToggleButton *edit;
    Gtk::ToggleButton *lightPipeline;
    Gtk::Button *reset;
    sigc::connection editConn, editedConn, lightPipelineConn;

    void editToggled();
    void editedToggled();
    Geometry *getVisibleGeometryFromMO(int MOID);

    Gtk::Frame *spot_frame;
    Adjuster *source_x;
    Adjuster *source_y;
    Adjuster *target_x;
    Adjuster *target_y;
    Adjuster *radius;
    Adjuster *feather;
    Adjuster *opacity;
    Adjuster *detail;
    std::vector<Adjuster *> spot_adjusters;

    rtengine::procparams::SpotParams initial_params;

    void reset_adjusters();
    void on_fold(GdkEventButton *event);
    void on_hide() override;

public:
    Spot();
    ~Spot();

    void read(const rtengine::procparams::ProcParams *pp) override;
    void write(rtengine::procparams::ProcParams *pp) override;

    void enabledChanged() override;
    void toolReset(bool to_initial) override;
    void setDefaults(const rtengine::procparams::ProcParams *pp) override;

    void setEditProvider(EditDataProvider *provider) override;

    // EditSubscriber interface
    CursorShape getCursor(int objectID, int xPos, int yPos) override;
    bool mouseOver(int modifierKey) override;
    bool button1Pressed(int modifierKey) override;
    bool button1Released() override;
    bool button2Pressed(int modifierKey) override;
    bool button3Pressed(int modifierKey) override;
    bool button3Released() override;
    bool drag1(int modifierKey) override;
    bool drag3(int modifierKey) override;
    bool pick2(bool picked) override;
    bool pick3(bool picked) override;
    void switchOffEditMode() override;

    // TweakOperator interface
    void tweakParams(rtengine::procparams::ProcParams &pparams) override;

    void adjusterChanged(Adjuster *a, double newval) override;

    rtengine::ProcEvent EvSpotEnabled;
    rtengine::ProcEvent EvSpotEnabledOPA; // used to toggle-on the Spot 'On
                                          // Preview Adjustment' mode
    rtengine::ProcEvent EvSpotEntry;
    rtengine::ProcEvent EvSpotEntryOPA;
};
