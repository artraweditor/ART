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

// adapted from the "color correction" module of Darktable. Original copyright
// follows
/*
    copyright (c) 2009--2010 johannes hanika.

    darktable is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    darktable is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with darktable.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "edit.h"
#include "eventmapper.h"
#include "toolpanel.h"
#include <gtkmm.h>

class ColorWheelArea: public Gtk::DrawingArea, public BackBuffer {
public:
    ColorWheelArea(
        bool enable_low = true); // rtengine::ProcEvent evt, const Glib::ustring
                                 // &msg, bool enable_low=true);

    void getParams(double &x, double &y) const;
    void setParams(double x, double y, bool notify);
    void setDefault(double x, double y, double s);
    void setEdited(bool yes);
    bool getEdited() const;
    void reset(bool toInitial);
    // void setListener(ToolPanelListener *l);

    void setScale(double s, bool notify);
    double getScale() const;

    bool on_draw(const ::Cairo::RefPtr<Cairo::Context> &crf) override;
    void on_style_updated() override;
    bool on_button_press_event(GdkEventButton *event) override;
    bool on_button_release_event(GdkEventButton *event) override;
    bool on_motion_notify_event(GdkEventMotion *event) override;
    Gtk::SizeRequestMode get_request_mode_vfunc() const override;
    void get_preferred_width_vfunc(int &minimum_width,
                                   int &natural_width) const override;
    void
    get_preferred_height_for_width_vfunc(int width, int &minimum_height,
                                         int &natural_height) const override;

    sigc::signal<void> signal_changed() { return sig_changed_; }
    sigc::signal<void> signal_right_click() { return sig_right_click_; }

private:
    // rtengine::ProcEvent evt;
    // Glib::ustring evtMsg;

    double low_a;
    double x_;
    double low_b;
    double y_;

    double defaultLow_a;
    double default_x_;
    double defaultLow_b;
    double default_y_;

    ToolPanelListener *listener;
    bool edited;
    bool point_active_;
    bool is_dragged_;
    bool lock_angle_;
    bool lock_radius_;
    sigc::connection delayconn;
    static const int inset = 5;

    double scale;
    double defaultScale;

    sigc::signal<void> sig_changed_;
    sigc::signal<void> sig_right_click_;

    bool notifyListener();
    void getLitPoint();
};

class ColorWheel: public Gtk::HBox, public EditSubscriber {
public:
    ColorWheel(bool use_scale = true);

    void getParams(double &x, double &y, double &s) const;
    void setParams(double x, double y, double s, bool notify);
    void setDefault(double x, double y, double s);
    void setEdited(bool yes) { grid.setEdited(yes); }
    bool getEdited() const { return grid.getEdited(); }
    void reset(bool toInitial) { grid.reset(toInitial); }
    sigc::signal<void> signal_changed() { return grid.signal_changed(); }

    CursorShape getCursor(int objectID) override;
    bool mouseOver(int modifierKey) override;
    bool button1Pressed(int modifierKey) override;
    void subscribe() override;
    void unsubscribe() override;

protected:
    virtual void onResetPressed() {}
    virtual void onRightClickPressed();
    bool resetPressed(GdkEventButton *event);
    void scaleChanged();

    ColorWheelArea grid;
    Gtk::VScale *scale;
    Gtk::ToggleButton *edit_;
    Gtk::VBox *scalebox_;
    sigc::connection scaleconn;
    sigc::connection timerconn;
    sigc::connection editconn_;
    std::array<double, 3> savedparams_;
};

class HueSatColorWheel: public ColorWheel {
public:
    HueSatColorWheel(double sat_scale = 1);
    ~HueSatColorWheel();

    void getParams(double &hue, double &sat) const;
    void setParams(double hue, double sat, bool notify);
    void setDefault(double hue, double sat);

private:
    void onResetPressed();
    void onRightClickPressed();

    double satscale_;
};
