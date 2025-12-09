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

#include "../rtengine/cache.h"
#include "../rtengine/coord.h"
#include "guiutils.h"
#include "histogrampanel.h"
#include <gtkmm.h>

class InspectorBuffer;
class FileCatalog;

class InspectorArea: public Gtk::DrawingArea {
public:
    InspectorArea();
    ~InspectorArea() override;

    /** @brief Mouse movement to a new position
     * @param pos Location of the mouse, in percentage (i.e. [0;1] range)
     * relative to the full size image ; -1,-1 == out of the image
     * @param transform H/V flip and coarse rotation transformation
     */
    void mouseMove(rtengine::Coord2D pos, int transform);

    /** @brief A new image is being flown over
     * @param fullPath Full path of the image that is being hovered inspect, or
     * an empty string if out of any image.
     */
    void switchImage(const Glib::ustring &fullPath, bool recenter = false,
                     rtengine::Coord2D newcenter = rtengine::Coord2D(-1, -1));

    /** @brief Set the new coarse rotation transformation
     * @param transform A semi-bitfield coarse transformation using #defines
     * from iimage.h
     */
    // void setTransformation (int transform);

    /** @brief Use this method to flush all image buffer whenever the Inspector
     * panel is hidden
     */
    void flushBuffers();

    /** @brief Set the inspector on/off
     * @param state true if to activate the Inspector, false to disable it and
     * flush the buffers
     */
    void setActive(bool state);

    /** @brief Get the on/off state
     */
    bool isActive() const { return active_; }

    void setInfoText(const Glib::ustring &txt);
    void infoEnabled(bool yes);
    //    void setZoomFit(bool yes);
    void setFocusMask(bool yes);

    Gtk::SizeRequestMode get_request_mode_vfunc() const override;
    void get_preferred_height_vfunc(int &minimum_height,
                                    int &natural_height) const override;
    void get_preferred_width_vfunc(int &minimum_width,
                                   int &natural_width) const override;
    void
    get_preferred_height_for_width_vfunc(int width, int &minimum_height,
                                         int &natural_height) const override;
    void
    get_preferred_width_for_height_vfunc(int height, int &minimum_width,
                                         int &natural_width) const override;

    sigc::signal<void> signal_ready() { return sig_ready_; }
    sigc::signal<void> signal_active() { return sig_active_; }
    sigc::signal<void, rtengine::Coord2D> signal_moved() { return sig_moved_; }
    sigc::signal<void, rtengine::Coord2D> signal_pressed()
    {
        return sig_pressed_;
    }
    sigc::signal<void> signal_released() { return sig_released_; }

    void setHighlight(bool yes) { highlight_ = yes; }

    void preloadImage(const Glib::ustring &fullPath);

private:
    bool on_draw(const ::Cairo::RefPtr<Cairo::Context> &cr) override;
    bool onMouseMove(GdkEventMotion *evt);
    bool onMousePress(GdkEventButton *evt);
    bool onMouseRelease(GdkEventButton *evt);

    void deleteBuffers();
    bool doSwitchImage(bool recenter, rtengine::Coord2D newcenter);
    void updateHistogram();
    std::shared_ptr<InspectorBuffer>
    doCacheImage(const Glib::ustring &fullPath);

    rtengine::Coord center;
    rtengine::Cache<Glib::ustring, std::shared_ptr<InspectorBuffer>>
        cache_; // std::vector<InspectorBuffer*> images;
    // InspectorBuffer* currImage;
    std::shared_ptr<InspectorBuffer> cur_image_;
    // double zoom;
    bool active_;
    bool first_active_;
    bool highlight_;
    bool has_focus_mask_;

    sigc::connection delayconn_;
    Glib::ustring next_image_path_;

    Glib::ustring info_text_;
    BackBuffer info_bb_;

    sigc::signal<void> sig_ready_;
    sigc::signal<void> sig_active_;
    sigc::signal<void, rtengine::Coord2D> sig_moved_;
    sigc::signal<void, rtengine::Coord2D> sig_pressed_;
    sigc::signal<void> sig_released_;
    rtengine::Coord prev_point_;

    HistogramArea hist_bb_;
};

class Inspector: public Gtk::VBox {
public:
    Inspector(FileCatalog *filecatalog);

    void mouseMove(rtengine::Coord2D pos, int transform);
    void switchImage(const Glib::ustring &fullPath);
    void flushBuffers();
    void setActive(bool state);
    bool isActive() const;
    sigc::signal<void> signal_ready();

    void toggleShowInfo();
    void toggleUseCms();
    void toggleShowHistogram();
    enum class DisplayMode {
        JPG,
        RAW_LINEAR,
        RAW_FILM_CURVE,
        RAW_SHADOW_BOOST,
        RAW_CLIP_WARNING
    };
    void setDisplayMode(DisplayMode m);
    void setZoomFit(bool yes);

    bool handleShortcutKey(GdkEventKey *event);

private:
    Gtk::HBox *get_toolbar();
    Glib::ustring get_info_text(size_t i);
    void info_toggled();
    void mode_toggled(Gtk::ToggleButton *b);
    void zoom_toggled(Gtk::ToggleButton *b);
    void cms_toggled();
    bool keyPressed(GdkEventKey *evt);
    void onGrabFocus(GdkEventButton *evt, size_t i);
    void onInspectorResized(Gtk::Allocation &a);
    void split_toggled();
    void histogram_toggled();
    void focus_mask_toggled();
    void on_moved(rtengine::Coord2D pos);
    void on_pressed(rtengine::Coord2D pos);
    void on_released();
    void do_toggle_zoom(Gtk::ToggleButton *b,
                        rtengine::Coord2D pos = rtengine::Coord2D(-1, -1));

    FileCatalog *filecatalog_;

    Gtk::HBox ibox_;
    std::array<Glib::ustring, 2> cur_image_;
    std::array<size_t, 2> cur_image_idx_;
    std::array<InspectorArea, 2> ins_;
    std::array<Gtk::Allocation, 2> ins_sz_;
    size_t active_;
    size_t num_active_;

    Gtk::HBox *toolbar_;
    Gtk::ToggleButton *split_;
    Gtk::ToggleButton *info_;
    Gtk::ToggleButton *histogram_;
    Gtk::ToggleButton *focusmask_;
    Gtk::ToggleButton *jpg_;
    Gtk::ToggleButton *rawlinear_;
    Gtk::ToggleButton *rawfilm_;
    Gtk::ToggleButton *rawshadow_;
    Gtk::ToggleButton *rawclip_;
    Gtk::ToggleButton *zoomfit_;
    Gtk::ToggleButton *zoom11_;
    Gtk::ToggleButton *cms_;

    RTImage focusmask_on_;
    RTImage focusmask_off_;

    sigc::connection jpgconn_;
    sigc::connection rawlinearconn_;
    sigc::connection rawfilmconn_;
    sigc::connection rawshadowconn_;
    sigc::connection rawclipconn_;
    sigc::connection zoomfitconn_;
    sigc::connection zoom11conn_;
    sigc::connection delayconn_;

    bool temp_zoom_11_;

    IdleRegister idle_register_;
};
