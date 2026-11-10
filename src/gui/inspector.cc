/* -*- C++ -*-
 *  This file is part of ART.
 *
 *  Copyright (c) 2004-2010 Gabor Horvath <hgabor@rawtherapee.com>
 *  Copyright (c) 2018-2026 Alberto Griggio <alberto.griggio@gmail.com>
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

#include <iostream>
#include <gtkmm.h>
#include <iomanip>

#include "../engine/imagedata.h"
#include "../engine/previewimage.h"
#include "cursormanager.h"
#include "filecatalog.h"
#include "focusmask.h"
#include "guiutils.h"
#include "inspector.h"
#include "shortcutsmanager.h"
#include "../utils/multilangmgr.h"
#include "options.h"
#include "rtwindow.h"

namespace art { namespace gui {
using namespace utils;


extern Options options;

//-----------------------------------------------------------------------------
// InspectorBuffer
//-----------------------------------------------------------------------------

class InspectorBuffer {
public:
    BackBuffer imgBuffer;
    Glib::ustring imgPath;
    std::array<art::engine::LUTu, 3> histogram;

    explicit InspectorBuffer(const Glib::ustring &imgagePath, int width = -1,
                             int height = -1);
};

InspectorBuffer::InspectorBuffer(const Glib::ustring &imagePath, int width,
                                 int height)
{
    if (!imagePath.empty() &&
        Glib::file_test(imagePath, Glib::FILE_TEST_EXISTS) &&
        !Glib::file_test(imagePath, Glib::FILE_TEST_IS_DIR)) {
        imgPath = imagePath;

        // generate thumbnail image
        Glib::ustring ext = getExtension(imagePath);

        if (ext == "") {
            imgPath.clear();
            return;
        }

        const auto load =
            [&]() -> Cairo::RefPtr<Cairo::ImageSurface>
            {
                art::engine::PreviewImage pi(imagePath, ext, width, height,
                                          options.thumbnail_inspector_enable_cms,
                                          options.thumbnail_inspector_show_histogram);
                Cairo::RefPtr<Cairo::ImageSurface> imageSurface = pi.getImage();
                pi.getHistogram(histogram[0], histogram[1], histogram[2]);
                return imageSurface;
            };
        
        auto imageSurface = load();
        if (!imageSurface && options.rtSettings.thumbnail_inspector_mode == art::engine::Settings::ThumbnailInspectorMode::JPEG) {
            art::engine::TempVarSetter<art::engine::Settings::ThumbnailInspectorMode> setraw(options.rtSettings.thumbnail_inspector_mode, art::engine::Settings::ThumbnailInspectorMode::RAW);
            art::engine::TempVarSetter<art::engine::Settings::ThumbnailInspectorRawCurve> setfilmcurve(options.rtSettings.thumbnail_inspector_raw_curve, art::engine::Settings::ThumbnailInspectorRawCurve::FILM);
            imageSurface = load();
        }

        if (imageSurface) {
            imgBuffer.setSurface(imageSurface);
        } else {
            imgPath.clear();
        }
    }
}

//-----------------------------------------------------------------------------
// InspectorArea
//-----------------------------------------------------------------------------

InspectorArea::InspectorArea()
    : cache_(std::max(options.maxInspectorBuffers, 1)), cur_image_(nullptr),
      active_(false), first_active_(true), highlight_(false),
      has_focus_mask_(false), info_text_(""), hist_bb_(nullptr, false)
{
    Glib::RefPtr<Gtk::StyleContext> style = get_style_context();
    set_name("Inspector");
    add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK |
               Gdk::POINTER_MOTION_MASK);
    signal_button_press_event().connect(
        sigc::mem_fun(*this, &InspectorArea::onMousePress), false);
    signal_button_release_event().connect(
        sigc::mem_fun(*this, &InspectorArea::onMouseRelease), false);
    signal_motion_notify_event().connect(
        sigc::mem_fun(*this, &InspectorArea::onMouseMove), false);
    prev_point_.set(-1, -1);

    hist_bb_.hide();
    hist_bb_.updateOptions(true, true, true, false, false, 1,
                           Options::ScopeType::HISTOGRAM_RAW, false);
}

InspectorArea::~InspectorArea() { deleteBuffers(); }

namespace {

void show_focus_mask(Cairo::RefPtr<Cairo::ImageSurface> surface)
{
    addFocusMask(surface->get_data(), surface->get_data(), surface->get_width(),
                 surface->get_height(), surface->get_stride(),
                 surface->get_stride(), 1, 1);
}

} // namespace

bool InspectorArea::on_draw(const ::Cairo::RefPtr<Cairo::Context> &cr)
{
    Glib::RefPtr<Gdk::Window> win = get_window();

    if (!win) {
        return false;
    }

    if (!active_) {
        sig_active_.emit();
        active_ = true;
    }

    // cleanup the region

    if (cur_image_ && cur_image_->imgBuffer.surfaceCreated()) {
        int scale = getImageDisplayScale();

        // compute the displayed area
        art::engine::Coord availableSize;
        art::engine::Coord topLeft;
        // art::engine::Coord displayedSize;
        art::engine::Coord dest(0, 0);
        availableSize.x = win->get_width();
        availableSize.y = win->get_height();
        int imW = cur_image_->imgBuffer.getWidth() / scale;
        int imH = cur_image_->imgBuffer.getHeight() / scale;

        art::engine::Coord center(center_.x * imW, center_.y * imH);

        if (imW < availableSize.x) {
            // center the image in the available space along X
            topLeft.x = 0;
            dest.x = (availableSize.x - imW) / 2;
        } else {
            // partial image display
            // double clamp
            topLeft.x = center.x + availableSize.x / 2;
            topLeft.x = art::engine::min<int>(topLeft.x, imW);
            topLeft.x -= availableSize.x;
            topLeft.x = art::engine::max<int>(topLeft.x, 0);
        }

        if (imH < availableSize.y) {
            // center the image in the available space along Y
            topLeft.y = 0;
            dest.y = (availableSize.y - imH) / 2;
        } else {
            // partial image display
            // double clamp
            topLeft.y = center.y + availableSize.y / 2;
            topLeft.y = art::engine::min<int>(topLeft.y, imH);
            topLeft.y -= availableSize.y;
            topLeft.y = art::engine::max<int>(topLeft.y, 0);
        }

        // printf("center: %d, %d   (img: %d, %d)  (availableSize: %d, %d)
        // (topLeft: %d, %d)\n", center.x, center.y, imW, imH, availableSize.x,
        // availableSize.y, topLeft.x, topLeft.y);

        // define the destination area
        auto dw = art::engine::min<int>(availableSize.x - dest.x, imW);
        auto dh = art::engine::min<int>(availableSize.y - dest.y, imH);
        cur_image_->imgBuffer.setDrawRectangle(win, dest.x, dest.y, dw, dh,
                                               false);
        cur_image_->imgBuffer.setSrcOffset(topLeft.x, topLeft.y);

        if (!cur_image_->imgBuffer.surfaceCreated()) {
            return false;
        }

        // Draw!
        RTScalable::setDisplayScale(cur_image_->imgBuffer.getSurface(), scale);

        Gdk::RGBA c;
        Glib::RefPtr<Gtk::StyleContext> style = get_style_context();
        style->render_background(cr, 0, 0, get_width(), get_height());

        if (has_focus_mask_) {
            int sw = std::min(win->get_width(), imW);
            int sh = std::min(win->get_height(), imH);
            BackBuffer surf(sw, sh); 
            cur_image_->imgBuffer.setDestPosition(0, 0);
            cur_image_->imgBuffer.copySurface(&surf);
            show_focus_mask(surf.getSurface());
            surf.setDestPosition(dest.x, dest.y);
            surf.copySurface(win);
        } else {
            cur_image_->imgBuffer.copySurface(win);
        }

        // draw the frame
        c = highlight_ ? style->get_color(Gtk::STATE_FLAG_SELECTED)
                       : style->get_background_color(Gtk::STATE_FLAG_NORMAL);
        cr->set_source_rgb(c.get_red(), c.get_green(), c.get_blue());
        cr->set_line_width(3);
        cr->rectangle(1.5, 1.5, availableSize.x - 2.5, availableSize.y - 2.5);
        cr->stroke();

        if (options.thumbnail_inspector_show_info && info_text_ != "") {
            info_bb_.copySurface(cr);
        }

        if (options.thumbnail_inspector_show_histogram) {
            auto s = 1;
            int dscale = RTScalable::getDisplayScale(this);
            double border = 4 * s;
            const int hist_W = hist_bb_.getWidth() / dscale;
            const int hist_H = hist_bb_.getHeight() / dscale;
            Gdk::Rectangle rect(border + 8 * s,
                                availableSize.y - hist_H - 8 * s -
                                    border,
                                hist_W, hist_H);

            // cr->set_operator(Cairo::OPERATOR_OVER);
            cr->set_source_rgba(0., 0., 0., 0.75);
            cr->rectangle(rect.get_x() - border, rect.get_y() - border,
                          rect.get_width() + border * 2,
                          rect.get_height() + border * 2);
            cr->fill();

            hist_bb_.copySurface(cr, &rect);
        }
    } else {
        Gdk::RGBA c;
        Glib::RefPtr<Gtk::StyleContext> style = get_style_context();
        style->render_background(cr, 0, 0, get_width(), get_height());

        // draw the frame
        c = highlight_ ? style->get_color(Gtk::STATE_FLAG_SELECTED)
                       : style->get_background_color(Gtk::STATE_FLAG_NORMAL);
        cr->set_source_rgb(c.get_red(), c.get_green(), c.get_blue());
        cr->set_line_width(3);
        cr->rectangle(1.5, 1.5, win->get_width() - 2.5,
                      win->get_height() - 2.5);
        cr->stroke();
    }

    if (first_active_) {
        first_active_ = false;
        sig_ready_.emit();
    }

    return true;
}

void InspectorArea::mouseMove(art::engine::Coord2D pos, int transform)
{
    if (!active_) {
        return;
    }

    if (cur_image_) {
        center_.set(art::engine::LIM01(pos.x), art::engine::LIM01(pos.y));
    } else {
        center_.set(0, 0);
    }

    queue_draw();
}

void InspectorArea::switchImage(const Glib::ustring &fullPath, bool recenter,
                                art::engine::Coord2D newcenter, int w, int h)
{
    if (!active_) {
        return;
    }

    if (delayconn_.connected()) {
        delayconn_.disconnect();
    }

    next_image_path_ = fullPath;
    if (!options.inspectorDelay) {
        doSwitchImage(recenter, newcenter, w, h);
    } else {
        delayconn_ = Glib::signal_timeout().connect(
            sigc::bind(sigc::mem_fun(*this, &InspectorArea::doSwitchImage),
                       recenter, newcenter, w, h),
            options.inspectorDelay);
    }
}

int InspectorArea::getImageDisplayScale()
{
    if (options.thumbnail_inspector_hidpi_preview) {
        return RTScalable::getDisplayScale(this);
    } else {
        return 1;
    }
}
    

bool InspectorArea::doSwitchImage(bool recenter, art::engine::Coord2D newcenter,
                                  int w, int h)
{
    Glib::ustring fullPath = next_image_path_;

    if (fullPath.empty()) {
        cur_image_.reset();
    } else {
        cur_image_ = doCacheImage(fullPath, w, h);
    }

    if (cur_image_ && recenter) {
        if (newcenter.x >= 0 && newcenter.y >= 0) {
            center_.set(art::engine::LIM01(newcenter.x),
                        art::engine::LIM01(newcenter.y));
        } else {
            center_.set(0.5, 0.5);
        }
    }

    if (cur_image_ && options.thumbnail_inspector_show_histogram) {
        updateHistogram();
    }

    queue_draw();

    return true;
}

std::shared_ptr<InspectorBuffer>
InspectorArea::doCacheImage(const Glib::ustring &fullPath, int w, int h)
{
    std::shared_ptr<InspectorBuffer> res;
    if (!cache_.get(fullPath, res)) {
        int width = -1, height = -1;
        if (options.thumbnail_inspector_zoom_fit) {
            Glib::RefPtr<Gdk::Window> win = get_window();
            int scale = getImageDisplayScale();
            if (w > 0 && h > 0) {
                width = w * scale;
                height = h * scale;
            } else if (win) {
                width = win->get_width() * scale;
                height = win->get_height() * scale;
            }
        }

        // Loading a new image
        res = std::make_shared<InspectorBuffer>(fullPath, width, height);

        // and add it to the tail
        if (res->imgPath.empty()) {
            res.reset();
        } else {
            cache_.set(fullPath, res);
        }
    }
    return res;
}

void InspectorArea::preloadImage(const Glib::ustring &fullPath)
{
    // std::cout << "PRELOAD: " << fullPath << std::endl;
    doCacheImage(fullPath, -1, -1);
}

void InspectorArea::deleteBuffers()
{
    cache_.clear();
    cur_image_.reset();
}

void InspectorArea::flushBuffers()
{
    if (!active_) {
        return;
    }

    deleteBuffers();
}

void InspectorArea::setActive(bool state)
{
    if (!state) {
        flushBuffers();
    }

    active_ = state;
    if (!active_) {
        first_active_ = true;
    }
}

Gtk::SizeRequestMode InspectorArea::get_request_mode_vfunc() const
{
    return Gtk::SIZE_REQUEST_CONSTANT_SIZE;
}

void InspectorArea::get_preferred_height_vfunc(int &minimum_height,
                                               int &natural_height) const
{
    minimum_height = 50 * 1;
    natural_height = 300 * 1;
}

void InspectorArea::get_preferred_width_vfunc(int &minimum_width,
                                              int &natural_width) const
{
    minimum_width = 50 * 1;
    natural_width = 200 * 1;
}

void InspectorArea::get_preferred_height_for_width_vfunc(
    int width, int &minimum_height, int &natural_height) const
{
    get_preferred_height_vfunc(minimum_height, natural_height);
}

void InspectorArea::get_preferred_width_for_height_vfunc(
    int height, int &minimum_width, int &natural_width) const
{
    get_preferred_width_vfunc(minimum_width, natural_width);
}

void InspectorArea::setInfoText(const Glib::ustring &text)
{
    info_text_ = text;

    Glib::RefPtr<Pango::Context> context = get_pango_context();
    Pango::FontDescription fontd(get_style_context()->get_font());

    // update font
    fontd.set_weight(Pango::WEIGHT_BOLD);
    fontd.set_size(options.fontSize * Pango::SCALE);
    context->set_font_description(fontd);

    // create text layout
    Glib::RefPtr<Pango::Layout> ilayout = create_pango_layout("");
    ilayout->set_markup(text);

    // get size of the text block
    int iw, ih;
    ilayout->get_pixel_size(iw, ih);

    // create BackBuffer
    int scale = RTScalable::getDisplayScale(this);
    info_bb_.setDrawRectangle(Cairo::FORMAT_ARGB32, 0, 0, (iw + 16) * scale,
                              (ih + 16) * scale, true);
    info_bb_.setDestPosition(8, 8);
    RTScalable::setDisplayScale(info_bb_.getSurface(), scale);

    Cairo::RefPtr<Cairo::Context> cr = info_bb_.getContext();

    // cleaning the back buffer (make it full transparent)
    cr->set_source_rgba(0., 0., 0., 0.);
    cr->set_operator(Cairo::OPERATOR_CLEAR);
    cr->paint();
    cr->set_operator(Cairo::OPERATOR_OVER);

    // paint transparent black background
    cr->set_source_rgba(0., 0., 0., 0.5);
    cr->paint();

    // paint text
    cr->set_source_rgb(1.0, 1.0, 1.0);
    cr->move_to(8, 8);
    ilayout->add_to_cairo_context(cr);
    cr->fill();
}

void InspectorArea::infoEnabled(bool yes)
{
    if (options.thumbnail_inspector_show_info != yes) {
        options.thumbnail_inspector_show_info = yes;
        queue_draw();
    }
}

void InspectorArea::setFocusMask(bool yes)
{
    if (has_focus_mask_ != yes) {
        has_focus_mask_ = yes;
        queue_draw();
    }
}

void InspectorArea::updateHistogram()
{
    Glib::RefPtr<Gdk::Window> win = get_window();
    if (!win || !cur_image_) {
        return;
    }

    art::engine::LUTu dummy_lut(1);
    array2D<int> dummy_arr;
    hist_bb_.update(dummy_lut, dummy_lut, dummy_lut, dummy_lut, dummy_lut,
                    cur_image_->histogram[0], cur_image_->histogram[1],
                    cur_image_->histogram[2], 1, dummy_arr, dummy_arr, 1,
                    dummy_arr, dummy_arr, dummy_arr, dummy_arr);

    int hist_w = 1 * 300;
    int hist_h = 1 * 200;

    hist_bb_.updateBackBuffer(hist_w, hist_h);
}

bool InspectorArea::onMouseMove(GdkEventMotion *evt)
{
    if (active_ && cur_image_ && prev_point_.x >= 0) {
        int scale = getImageDisplayScale();
        double w = cur_image_->imgBuffer.getWidth() / scale;
        double h = cur_image_->imgBuffer.getHeight() / scale;
        if (w > 0 && h > 0) {
            constexpr double gain = 4.0;
            double dx = (center_.x * w) - (evt->x - prev_point_.x) * gain;
            double dy = (center_.y * h) - (evt->y - prev_point_.y) * gain;
            sig_moved_.emit(art::engine::Coord2D(dx / w, dy / h));
        }
        prev_point_.set(evt->x, evt->y);
    }
    return false;
}

bool InspectorArea::onMousePress(GdkEventButton *evt)
{
    if (active_ && evt->button == 1) {
        prev_point_.set(evt->x, evt->y);
        CursorManager::setWidgetCursor(get_window(), CSHandClosed);
        if (cur_image_) {
            double w = cur_image_->imgBuffer.getWidth();
            double h = cur_image_->imgBuffer.getHeight();
            auto win = get_window();
            if (w > 0 && h > 0) {
                int ww = win->get_width();
                int hh = win->get_height();
                int ox = w / 2 - ww / 2;
                int oy = h / 2 - hh / 2;
                double x = (evt->x + ox) / w;
                double y = (evt->y + oy) / h;
                sig_pressed_.emit(art::engine::Coord2D(x, y));
            }
        }
    } else {
        prev_point_.set(-1, -1);
        CursorManager::setWidgetCursor(get_window(), CSArrow);
    }
    return false;
}

bool InspectorArea::onMouseRelease(GdkEventButton *evt)
{
    prev_point_.set(-1, -1);
    CursorManager::setWidgetCursor(get_window(), CSArrow);
    sig_released_.emit();
    return false;
}

//-----------------------------------------------------------------------------
// Inspector
//-----------------------------------------------------------------------------

Inspector::Inspector(FileCatalog *filecatalog)
    : filecatalog_(filecatalog), focusmask_on_("focusscreen-on.svg"),
      focusmask_off_("focusscreen-off.svg")
{
    bindShortcuts();
    ibox_.pack_start(ins_[0], Gtk::PACK_EXPAND_WIDGET, 3);
    ibox_.pack_start(ins_[1], Gtk::PACK_EXPAND_WIDGET, 3);
    pack_start(ibox_);
    pack_start(*get_toolbar(), Gtk::PACK_SHRINK, 2);
    removeIfThere(&ibox_, &ins_[1]);
    show_all_children();

    signal_key_press_event().connect(
        sigc::mem_fun(*this, &Inspector::keyPressed));

    cur_image_idx_[0] = 0;
    cur_image_idx_[1] = 0;

    active_ = 0;
    num_active_ = 1;
    temp_zoom_11_ = false;
    for (size_t i = 0; i < 2; ++i) {
        ins_[i].set_can_focus(true);
        ins_[i].add_events(Gdk::BUTTON_PRESS_MASK);
        ins_[i].signal_button_press_event().connect_notify(
            sigc::bind(sigc::mem_fun(*this, &Inspector::onGrabFocus), i));
        //        ins_[i].signal_size_allocate().connect(sigc::mem_fun(*this,
        //        &Inspector::onInspectorResized));
        ins_[i].signal_active().connect(
            sigc::bind(sigc::mem_fun(*this, &Inspector::setActive), true));
        ins_[i].signal_moved().connect(
            sigc::mem_fun(*this, &Inspector::on_moved));
        ins_[i].signal_pressed().connect(
            sigc::mem_fun(*this, &Inspector::on_pressed));
        ins_[i].signal_released().connect(
            sigc::mem_fun(*this, &Inspector::on_released));
    }
    signal_size_allocate().connect(
        sigc::mem_fun(*this, &Inspector::onInspectorResized));

    property_scale_factor().signal_changed().connect(
        [this]() {
            hidpi_->set_visible(RTScalable::getDisplayScale(this) > 1);
        });
}

void Inspector::mouseMove(art::engine::Coord2D pos, int transform)
{
    for (size_t i = 0; i < num_active_; ++i) {
        ins_[i].mouseMove(pos, transform);
    }
}

void Inspector::on_moved(art::engine::Coord2D pos) { mouseMove(pos, 0); }

void Inspector::on_pressed(art::engine::Coord2D pos)
{
    if (options.thumbnail_inspector_zoom_fit) {
        temp_zoom_11_ = true;
        ConnectionBlocker block1(zoom11conn_);
        zoom11_->set_active(true);
        do_toggle_zoom(zoom11_, pos);
    }
}

void Inspector::on_released()
{
    if (temp_zoom_11_) {
        temp_zoom_11_ = false;
        ConnectionBlocker block1(zoomfitconn_);
        zoomfit_->set_active(true);
        do_toggle_zoom(zoomfit_);
    }
}

void Inspector::flushBuffers()
{
    for (size_t i = 0; i < 2; ++i) {
        ins_[i].flushBuffers();
    }
}

void Inspector::setActive(bool state)
{
    if (!state) {
        toolbar_->hide();
    } else {
        toolbar_->show();
    }
    for (size_t i = 0; i < num_active_; ++i) {
        ins_[i].setActive(state);
    }
}

bool Inspector::isActive() const { return ins_[0].isActive(); }

sigc::signal<void> Inspector::signal_ready()
{
    return ins_[active_].signal_ready();
}

bool Inspector::keyPressed(GdkEventKey *evt)
{
    if (filecatalog_) {
        return filecatalog_->handleShortcutKey(evt);
    }
    return false;
}

void Inspector::switchImage(const Glib::ustring &fullPath)
{
    if (!isActive()) {
        return;
    }
    
    cur_image_[active_] = fullPath;
    if (info_->get_active()) {
        ins_[active_].setInfoText(get_info_text(active_));
    }
    ins_[active_].switchImage(fullPath);
    auto &root = getToplevelWindow(this);
    if (RTWindow *w = dynamic_cast<RTWindow *>(&root)) {
        w->set_title_decorated(fullPath);
    }
    auto &entries = filecatalog_->fileBrowser->getEntries();
    size_t j = entries.size();
    size_t ilo = std::min(cur_image_idx_[active_], j-1);
    size_t ihi = ilo;
    while (ilo > 0 || ihi < entries.size()) {
        if (ilo > 0) {
            if (!entries[ilo - 1]->filtered &&
                entries[ilo - 1]->filename == fullPath) {
                j = ilo - 1;
                break;
            }
            --ilo;
        }
        if (ihi < entries.size()) {
            if (!entries[ihi]->filtered && entries[ihi]->filename == fullPath) {
                j = ihi;
                break;
            }
            ++ihi;
        }
    }
    if (j < entries.size()) {
        cur_image_idx_[active_] = j;
        Glib::ustring prev, next;
        if (options.maxInspectorBuffers > 2) {
            for (size_t i = j; i > 0; --i) {
                if (!entries[i - 1]->filtered) {
                    prev = entries[i - 1]->filename;
                    break;
                }
            }
        }
        if (options.maxInspectorBuffers > 1) {
            for (size_t i = j + 1; i < entries.size(); ++i) {
                if (!entries[i]->filtered) {
                    next = entries[i]->filename;
                    break;
                }
            }
        }
        idle_register_.add([this, prev, next]() -> bool {
            if (!next.empty()) {
                ins_[active_].preloadImage(next);
            }
            if (!prev.empty()) {
                ins_[active_].preloadImage(prev);
            }
            return false;
        });
    }
}

Gtk::HBox *Inspector::get_toolbar()
{
    Gtk::HBox *tb = Gtk::manage(new Gtk::HBox());
    toolbar_ = tb;
    tb->pack_start(*Gtk::manage(new Gtk::Label("")), Gtk::PACK_EXPAND_WIDGET,
                   2);

    const auto add_tool = [&](const char *icon,
                              const char *tip =
                                  nullptr) -> Gtk::ToggleButton * {
        Gtk::ToggleButton *ret = Gtk::manage(new Gtk::ToggleButton());
        ret->set_image(*Gtk::manage(new RTImage(icon)));
        ret->set_relief(Gtk::RELIEF_NONE);
        if (tip) {
            ret->set_tooltip_markup(M(tip));
        }
        tb->pack_start(*ret, Gtk::PACK_SHRINK, 2);
        return ret;
    };

    split_ = add_tool("beforeafter.svg", "INSPECTOR_SPLIT");
    tb->pack_start(*Gtk::manage(new Gtk::VSeparator()), Gtk::PACK_SHRINK, 4);

    info_ = add_tool("info.svg", "INSPECTOR_INFO");
    histogram_ = add_tool("histogram.svg", "INSPECTOR_HISTOGRAM");
    focusmask_ = add_tool("focusscreen-off.svg", "INSPECTOR_FOCUS_MASK");
    tb->pack_start(*Gtk::manage(new Gtk::VSeparator()), Gtk::PACK_SHRINK, 4);

    jpg_ = add_tool("wb-camera.svg", "INSPECTOR_PREVIEW");
    rawlinear_ = add_tool("raw-linear-curve.svg", "INSPECTOR_RAW_LINEAR");
    rawfilm_ = add_tool("raw-film-curve.svg", "INSPECTOR_RAW_FILM");
    rawshadow_ = add_tool("raw-shadow-curve.svg", "INSPECTOR_RAW_SHADOW");
    rawclip_ = add_tool("raw-clip-curve.svg", "INSPECTOR_RAW_CLIP");

    tb->pack_start(*Gtk::manage(new Gtk::VSeparator()), Gtk::PACK_SHRINK, 4);

    hidpi_ = add_tool("hidpi.svg", "HIDPI_PREVIEW");

    zoomfit_ = add_tool("magnifier-fit.svg", "INSPECTOR_ZOOM_FIT");
    zoom11_ = add_tool("magnifier-1to1.svg", "INSPECTOR_ZOOM_11");

    tb->pack_start(*Gtk::manage(new Gtk::VSeparator()), Gtk::PACK_SHRINK, 4);

    cms_ = add_tool("gamut-softproof.svg", "INSPECTOR_ENABLE_CMS");

    //------------------------------------------------------------------------

    split_->signal_toggled().connect(
        sigc::mem_fun(*this, &Inspector::split_toggled));

    info_->set_active(options.thumbnail_inspector_show_info);
    info_->signal_toggled().connect(
        sigc::mem_fun(*this, &Inspector::info_toggled));

    histogram_->set_active(options.thumbnail_inspector_show_histogram);
    histogram_->signal_toggled().connect(
        sigc::mem_fun(*this, &Inspector::histogram_toggled));

    focusmask_->signal_toggled().connect(
        sigc::mem_fun(*this, &Inspector::focus_mask_toggled));

    bool use_jpg = options.rtSettings.thumbnail_inspector_mode ==
                   art::engine::Settings::ThumbnailInspectorMode::JPEG;
    jpg_->set_active(use_jpg);
    rawlinear_->set_active(
        !use_jpg && options.rtSettings.thumbnail_inspector_raw_curve ==
                        art::engine::Settings::ThumbnailInspectorRawCurve::LINEAR);
    rawfilm_->set_active(
        !use_jpg && options.rtSettings.thumbnail_inspector_raw_curve ==
                        art::engine::Settings::ThumbnailInspectorRawCurve::FILM);
    rawshadow_->set_active(
        !use_jpg &&
        options.rtSettings.thumbnail_inspector_raw_curve ==
            art::engine::Settings::ThumbnailInspectorRawCurve::SHADOW_BOOST);
    rawclip_->set_active(
        !use_jpg &&
        options.rtSettings.thumbnail_inspector_raw_curve ==
            art::engine::Settings::ThumbnailInspectorRawCurve::RAW_CLIPPING);

    zoomfit_->set_active(options.thumbnail_inspector_zoom_fit);
    zoom11_->set_active(!options.thumbnail_inspector_zoom_fit);

    cms_->set_active(options.thumbnail_inspector_enable_cms);
    hidpi_->set_active(options.thumbnail_inspector_hidpi_preview);

    jpgconn_ = jpg_->signal_toggled().connect(
        sigc::bind(sigc::mem_fun(*this, &Inspector::mode_toggled), jpg_));
    rawlinearconn_ = rawlinear_->signal_toggled().connect(
        sigc::bind(sigc::mem_fun(*this, &Inspector::mode_toggled), rawlinear_));
    rawfilmconn_ = rawfilm_->signal_toggled().connect(
        sigc::bind(sigc::mem_fun(*this, &Inspector::mode_toggled), rawfilm_));
    rawshadowconn_ = rawshadow_->signal_toggled().connect(
        sigc::bind(sigc::mem_fun(*this, &Inspector::mode_toggled), rawshadow_));
    rawclipconn_ = rawclip_->signal_toggled().connect(
        sigc::bind(sigc::mem_fun(*this, &Inspector::mode_toggled), rawclip_));

    zoomfitconn_ = zoomfit_->signal_toggled().connect(
        sigc::bind(sigc::mem_fun(*this, &Inspector::zoom_toggled), zoomfit_));
    zoom11conn_ = zoom11_->signal_toggled().connect(
        sigc::bind(sigc::mem_fun(*this, &Inspector::zoom_toggled), zoom11_));

    cms_->signal_toggled().connect(
        sigc::mem_fun(*this, &Inspector::cms_toggled));

    hidpi_->signal_toggled().connect(
        sigc::mem_fun(*this, &Inspector::hidpi_toggled));

    return tb;
}

void Inspector::info_toggled()
{
    if (!info_->get_active()) {
        for (size_t i = 0; i < num_active_; ++i) {
            ins_[i].infoEnabled(false);
        }
    } else {
        for (size_t i = 0; i < num_active_; ++i) {
            ins_[i].setInfoText(get_info_text(i));
            ins_[i].infoEnabled(true);
        }
    }
}

Glib::ustring Inspector::get_info_text(size_t i)
{
    art::engine::FramesData meta(cur_image_[i]);

    Glib::ustring infoString;
    Glib::ustring expcomp;

    if (meta.hasExif()) {
        infoString = Glib::ustring::compose(
            "%1 + %2\n<span size=\"small\">f/</span><span "
            "size=\"large\">%3</span>  <span size=\"large\">%4</span><span "
            "size=\"small\">s</span>  <span size=\"small\">%5</span><span "
            "size=\"large\">%6</span>  <span size=\"large\">%7</span><span "
            "size=\"small\">mm</span>",
            Glib::ustring(meta.getMake() + " " + meta.getModel()),
            Glib::ustring(meta.getLens()),
            Glib::ustring(meta.apertureToString(meta.getFNumber())),
            Glib::ustring(meta.shutterToString(meta.getShutterSpeed())),
            M("QINFO_ISO"), meta.getISOSpeed(),
            Glib::ustring::format(std::setw(3), std::fixed,
                                  std::setprecision(2), meta.getFocalLen()));

        expcomp = Glib::ustring(meta.expcompToString(meta.getExpComp(), true));

        if (!expcomp.empty()) {
            infoString = Glib::ustring::compose(
                "%1  <span size=\"large\">%2</span><span "
                "size=\"small\">EV</span>",
                infoString, expcomp);
        }

        infoString = Glib::ustring::compose(
            "%1\n<span size=\"small\">%2</span><span>%3</span>", infoString,
            escapeHtmlChars(Glib::path_get_dirname(cur_image_[i])) +
                G_DIR_SEPARATOR_S,
            escapeHtmlChars(Glib::path_get_basename(cur_image_[i])));

        int ww = -1, hh = -1;
        meta.getDimensions(ww, hh);
        if (ww > 0 && hh > 0) {
            // megapixels
            infoString = Glib::ustring::compose(
                "%1\n<span size=\"small\">%2 MP (%3x%4)</span>", infoString,
                Glib::ustring::format(std::setw(4), std::fixed,
                                      std::setprecision(1),
                                      (float)ww * hh / 1000000),
                ww, hh);
        }
    } else {
        infoString = M("QINFO_NOEXIF");
    }
    return infoString;
}

void Inspector::mode_toggled(Gtk::ToggleButton *b)
{
    ConnectionBlocker blockj(jpgconn_);
    ConnectionBlocker blockl(rawlinearconn_);
    ConnectionBlocker blockf(rawfilmconn_);
    ConnectionBlocker blocks(rawshadowconn_);
    ConnectionBlocker blockc(rawclipconn_);

    if (!b->get_active()) {
        b->set_active(true);
    } else {
        jpg_->set_active(false);
        rawlinear_->set_active(false);
        rawfilm_->set_active(false);
        rawshadow_->set_active(false);
        rawclip_->set_active(false);
        b->set_active(true);

        if (jpg_->get_active()) {
            options.rtSettings.thumbnail_inspector_mode =
                art::engine::Settings::ThumbnailInspectorMode::JPEG;
        } else if (rawlinear_->get_active()) {
            options.rtSettings.thumbnail_inspector_mode =
                art::engine::Settings::ThumbnailInspectorMode::RAW;
            options.rtSettings.thumbnail_inspector_raw_curve =
                art::engine::Settings::ThumbnailInspectorRawCurve::LINEAR;
        } else if (rawfilm_->get_active()) {
            options.rtSettings.thumbnail_inspector_mode =
                art::engine::Settings::ThumbnailInspectorMode::RAW;
            options.rtSettings.thumbnail_inspector_raw_curve =
                art::engine::Settings::ThumbnailInspectorRawCurve::FILM;
        } else if (rawshadow_->get_active()) {
            options.rtSettings.thumbnail_inspector_mode =
                art::engine::Settings::ThumbnailInspectorMode::RAW;
            options.rtSettings.thumbnail_inspector_raw_curve =
                art::engine::Settings::ThumbnailInspectorRawCurve::SHADOW_BOOST;
        } else if (rawclip_->get_active()) {
            options.rtSettings.thumbnail_inspector_mode =
                art::engine::Settings::ThumbnailInspectorMode::RAW;
            options.rtSettings.thumbnail_inspector_raw_curve =
                art::engine::Settings::ThumbnailInspectorRawCurve::RAW_CLIPPING;
        }

        for (size_t i = 0; i < num_active_; ++i) {
            ins_[i].flushBuffers();
            ins_[i].switchImage(cur_image_[i]);
        }
    }
}

void Inspector::zoom_toggled(Gtk::ToggleButton *b) { do_toggle_zoom(b); }

void Inspector::do_toggle_zoom(Gtk::ToggleButton *b, art::engine::Coord2D pos)
{
    ConnectionBlocker blockf(zoomfitconn_);
    ConnectionBlocker block1(zoom11conn_);

    if (!b->get_active()) {
        b->set_active(true);
    } else {
        zoomfit_->set_active(false);
        zoom11_->set_active(false);
        b->set_active(true);

        options.thumbnail_inspector_zoom_fit = zoomfit_->get_active();

        for (size_t i = 0; i < num_active_; ++i) {
            ins_[i].flushBuffers();
            ins_[i].switchImage(cur_image_[i], true, pos);
        }
    }
}

void Inspector::cms_toggled()
{
    options.thumbnail_inspector_enable_cms = cms_->get_active();
    for (size_t i = 0; i < num_active_; ++i) {
        ins_[i].flushBuffers();
        ins_[i].switchImage(cur_image_[i]);
    }
}

void Inspector::hidpi_toggled()
{
    options.thumbnail_inspector_hidpi_preview = hidpi_->get_active();
    for (size_t i = 0; i < num_active_; ++i) {
        ins_[i].flushBuffers();
        ins_[i].switchImage(cur_image_[i]);
    }
}

void Inspector::toggleShowInfo() { info_->set_active(!info_->get_active()); }

void Inspector::toggleUseCms() { cms_->set_active(!cms_->get_active()); }

void Inspector::toggleShowHistogram()
{
    histogram_->set_active(!histogram_->get_active());
}

void Inspector::setDisplayMode(DisplayMode m)
{
    switch (m) {
    case DisplayMode::JPG:
        jpg_->set_active(true);
        break;
    case DisplayMode::RAW_LINEAR:
        rawlinear_->set_active(true);
        break;
    case DisplayMode::RAW_FILM_CURVE:
        rawfilm_->set_active(true);
        break;
    case DisplayMode::RAW_SHADOW_BOOST:
        rawshadow_->set_active(true);
        break;
    case DisplayMode::RAW_CLIP_WARNING:
        rawclip_->set_active(true);
        break;
    }
}

void Inspector::setZoomFit(bool yes)
{
    if (yes) {
        zoomfit_->set_active(true);
    } else {
        zoom11_->set_active(true);
    }
}

// void Inspector::setFocusMask(bool yes)
// {
//     focusmask_->set_active(yes);
// }

void Inspector::onGrabFocus(GdkEventButton *evt, size_t i)
{
    if (evt->button == 1) {
        ins_[active_].setHighlight(false);
        ins_[i].setHighlight(true);
        active_ = i;
        queue_draw();
    }
}

void Inspector::split_toggled()
{
    if (split_->get_active()) {
        active_ = 1;
        ibox_.pack_start(ins_[1], Gtk::PACK_EXPAND_WIDGET, 3);
        ins_[1].show();
        ins_[1].setActive(false);
        num_active_ = 2;
        ins_[0].setHighlight(false);
        ins_[1].setHighlight(true);
    } else {
        active_ = 0;
        ins_[1].setActive(false);
        removeIfThere(&ibox_, &ins_[1]);
        num_active_ = 1;
        ins_[0].setHighlight(false);
        ins_[1].setHighlight(false);
    }
    queue_draw();
}

void Inspector::histogram_toggled()
{
    options.thumbnail_inspector_show_histogram = histogram_->get_active();
    for (size_t i = 0; i < num_active_; ++i) {
        ins_[i].flushBuffers();
        ins_[i].switchImage(cur_image_[i]);
    }
}

void Inspector::focus_mask_toggled()
{
    if (focusmask_->get_active()) {
        focusmask_->set_image(focusmask_on_);
    } else {
        focusmask_->set_image(focusmask_off_);
    }
    for (size_t i = 0; i < num_active_; ++i) {
        ins_[i].setFocusMask(focusmask_->get_active());
    }
}

void Inspector::bindShortcuts()
{
    auto sm = ShortcutsManager::getInstance();
    const auto run = [this, sm](const char *id, std::function<void()> f) {
        sm->bind(this, id, [f](GdkEventKey *) {
            f();
            return true;
        });
    };

    run("inspector.toggle_histogram", [this]() { toggleShowHistogram(); });
    run("inspector.toggle_cms", [this]() { toggleUseCms(); });
    run("inspector.zoom_100", [this]() { setZoomFit(false); });
    run("inspector.zoom_fit", [this]() { setZoomFit(true); });
    run("inspector.mode_jpeg",
        [this]() { setDisplayMode(Inspector::DisplayMode::JPG); });
    run("inspector.mode_raw_linear",
        [this]() { setDisplayMode(Inspector::DisplayMode::RAW_LINEAR); });
    run("inspector.mode_raw_film_curve",
        [this]() { setDisplayMode(Inspector::DisplayMode::RAW_FILM_CURVE); });
    run("inspector.mode_raw_shadow_boost", [this]() {
        setDisplayMode(Inspector::DisplayMode::RAW_SHADOW_BOOST);
    });
    run("inspector.mode_raw_clip_warning", [this]() {
        setDisplayMode(Inspector::DisplayMode::RAW_CLIP_WARNING);
    });
    run("inspector.toggle_split",
        [this]() { split_->set_active(!split_->get_active()); });
    sm->bind(this, "inspector.switch_pane", [this](GdkEventKey *) {
        if (split_->get_active()) {
            ins_[active_].setHighlight(false);
            active_ = 1 - active_;
            ins_[active_].setHighlight(true);
            queue_draw();
            return true;
        }
        return false;
    });
    run("inspector.toggle_focus_mask",
        [this]() { focusmask_->set_active(!focusmask_->get_active()); });
    run("inspector.toggle_info", [this]() { toggleShowInfo(); });
}


Inspector::~Inspector()
{
    ShortcutsManager::getInstance()->unbindOwner(this);
}

void Inspector::onInspectorResized(Gtk::Allocation &a)
{
    if (zoomfit_->get_active()) {
        if (delayconn_.connected()) {
            delayconn_.disconnect();
        }

        const auto doit = [this]() -> bool {
            for (size_t i = 0; i < num_active_; ++i) {
                ins_[i].flushBuffers();
                ins_[i].switchImage(cur_image_[i]);
            }
            return false;
        };

        delayconn_ = Glib::signal_timeout().connect(sigc::slot<bool>(doit),
                                                    options.adjusterMaxDelay);
    }
    hidpi_->set_visible(RTScalable::getDisplayScale(this) > 1);
}


void Inspector::popover(const ThumbBrowserEntryBase *entry)
{
    auto w = entry->getParent();
    if (!w || !entry->thumbnail) {
        return;
    }
    Gtk::Popover p(*w);
    p.set_pointing_to(
        Gdk::Rectangle(entry->getX(), entry->getY(),
                       entry->getEffectiveWidth(), entry->getEffectiveHeight()));

    auto &top = getToplevelWindow(const_cast<ThumbBrowserBase *>(w));
    int topw, toph;
    top.get_size(topw, toph);

    art::engine::TempVarSetter<bool> setzoomfit(options.thumbnail_inspector_zoom_fit, true);
    art::engine::TempVarSetter<bool> setnohist(options.thumbnail_inspector_show_histogram, false);
    art::engine::TempVarSetter<art::engine::Settings::ThumbnailInspectorMode> setmode(options.rtSettings.thumbnail_inspector_mode, art::engine::Settings::ThumbnailInspectorMode::JPEG);
    art::engine::TempVarSetter<art::engine::Settings::ThumbnailInspectorRawCurve> setcurve(options.rtSettings.thumbnail_inspector_raw_curve);

    // fit the image into the window area scaled by the preference (using the
    // shorter window side for the longer image side made landscape images
    // much smaller than portrait ones), but never larger than the room that
    // is available in the toplevel window next to the thumbnail
    const float pct = float(options.quick_inspect_popup_size_percent) / 100.f;
    const float maxw = topw * pct, maxh = toph * pct;

    int tw, th;
    entry->thumbnail->getOriginalSize(tw, th, true);
    const float ratio = th > 0 ? float(tw) / float(th) : 1.f;
    const auto fit = [&](float bw, float bh, int &fw, int &fh) {
        bw = std::max(bw, 1.f);
        bh = std::max(bh, 1.f);
        fw = bw;
        fh = fw / ratio + 0.5f;
        if (fh > bh) {
            fh = bh;
            fw = fh * ratio + 0.5f;
        }
        fw = std::max(fw, 1);
        fh = std::max(fh, 1);
    };

    int aw, ah;
    fit(maxw, maxh, aw, ah);

    // A popover is constrained to the toplevel window, and may be placed on
    // any side of the thumbnail: pick the side with the most room and size
    // the image to what fits there.
    {
        // measure the space the popover itself adds around its content
        // (border, padding, shadow), plus an allowance for the arrow gap that
        // GTK puts between the popover and the thumbnail
        int chrome_w = 48, chrome_h = 48;
        {
            Gtk::Popover probe(*w);
            Gtk::Box content;
            content.set_size_request(100, 100);
            probe.add(content);
            content.show();
            int mw = 0, nw = 0, mh = 0, nh = 0;
            probe.get_preferred_width(mw, nw);
            probe.get_preferred_height(mh, nh);
            chrome_w = std::max(nw - 100, 0) + 24;
            chrome_h = std::max(nh - 100, 0) + 24;
        }
        int tx = 0, ty = 0;
        if (const_cast<ThumbBrowserBase *>(w)->translate_coordinates(
                top, entry->getX(), entry->getY(), tx, ty)) {
            // the thumbnail may be partly scrolled out of view: only the
            // visible part counts
            const int x0 = std::max(tx, 0), y0 = std::max(ty, 0);
            const int x1 = std::min(tx + entry->getEffectiveWidth(), topw);
            const int y1 = std::min(ty + entry->getEffectiveHeight(), toph);
            const float AW = topw - chrome_w;
            const float AH = toph - chrome_h;
            // when the popover goes beside the thumbnail it also needs the
            // arrow gap along that axis, which is part of chrome_*
            const float left = x0 - chrome_w;
            const float right = topw - x1 - chrome_w;
            const float above = y0 - chrome_h;
            const float below = toph - y1 - chrome_h;

            struct Cand {
                Gtk::PositionType pos;
                float w, h;
            };
            const Cand cands[] = {
                { Gtk::POS_RIGHT, right, AH },
                { Gtk::POS_LEFT, left, AH },
                { Gtk::POS_BOTTOM, AW, below },
                { Gtk::POS_TOP, AW, above },
            };
            long best = -1;
            for (auto &c : cands) {
                if (c.w < 1 || c.h < 1) {
                    continue;
                }
                int fw, fh;
                fit(std::min(maxw, c.w), std::min(maxh, c.h), fw, fh);
                if (long(fw) * fh > best) {
                    best = long(fw) * fh;
                    aw = fw;
                    ah = fh;
                    p.set_position(c.pos);
                }
            }
        } else {
            // unknown geometry: at least fit in the window
            fit(std::min(maxw, float(topw - chrome_w)),
                std::min(maxh, float(toph - chrome_h)), aw, ah);
        }
    }

    InspectorArea a;
    a.setActive(true);
    a.setInfoText("");
    a.switchImage(entry->filename, false, art::engine::Coord2D(-1, -1), aw, ah);
    a.set_size_request(aw, ah);
    p.add(a);

    bool done = false;
    p.signal_closed().connect([&]() { done = true; });
    p.add_events(Gdk::KEY_PRESS_MASK|Gdk::BUTTON_PRESS_MASK);

    const auto toggle_mode =
        [&](Inspector::DisplayMode m) -> bool
        {
            auto &mode = options.rtSettings.thumbnail_inspector_mode;
            auto &curve = options.rtSettings.thumbnail_inspector_raw_curve;

            mode = art::engine::Settings::ThumbnailInspectorMode::RAW;

            switch (m) {
            case Inspector::DisplayMode::JPG:
                mode = art::engine::Settings::ThumbnailInspectorMode::JPEG;
                break;
            case Inspector::DisplayMode::RAW_LINEAR:
                curve = art::engine::Settings::ThumbnailInspectorRawCurve::LINEAR;
                break;
            case Inspector::DisplayMode::RAW_FILM_CURVE:
                curve = art::engine::Settings::ThumbnailInspectorRawCurve::FILM;
                break;
            case Inspector::DisplayMode::RAW_SHADOW_BOOST:
                curve = art::engine::Settings::ThumbnailInspectorRawCurve::SHADOW_BOOST;
                break;
            case Inspector::DisplayMode::RAW_CLIP_WARNING:
                curve = art::engine::Settings::ThumbnailInspectorRawCurve::RAW_CLIPPING;
                break;
            }

            a.flushBuffers();
            a.switchImage(entry->filename, false, art::engine::Coord2D(-1, -1), aw, ah);
            a.queue_draw();
            
            return true;
        };
    
    // the keys that switch the display mode: anything else closes the popover
    const std::pair<const char *, Inspector::DisplayMode> modes[] = {
        {"inspector.popover.mode_jpeg", Inspector::DisplayMode::JPG},
        {"inspector.popover.mode_raw_linear",
         Inspector::DisplayMode::RAW_LINEAR},
        {"inspector.popover.mode_raw_film_curve",
         Inspector::DisplayMode::RAW_FILM_CURVE},
        {"inspector.popover.mode_raw_shadow_boost",
         Inspector::DisplayMode::RAW_SHADOW_BOOST},
        {"inspector.popover.mode_raw_clip_warning",
         Inspector::DisplayMode::RAW_CLIP_WARNING}};
    for (auto &m : modes) {
        const auto mode = m.second;
        ShortcutsManager::getInstance()->bind(
            &p, m.first,
            [&toggle_mode, mode](GdkEventKey *) { return toggle_mode(mode); });
    }

    p.signal_key_press_event().connect(
        [&](GdkEventKey *event)
        {
            if (ShortcutsManager::getInstance()->dispatch(
                    event, ShortcutPhase::After,
                    {{"inspector-popover", &p}})) {
                return true;
            }
            done = true;
            p.hide();
            return true;
        }
        );
    a.set_can_focus(true);

    bool zoomfit = true;

    const auto toggle_zoom =
        [&](art::engine::Coord2D pos)
        {
            zoomfit = !zoomfit;
            art::engine::TempVarSetter<bool> setzoomfit(options.thumbnail_inspector_zoom_fit, zoomfit);
            a.flushBuffers();
            a.switchImage(entry->filename, true, pos, aw, ah);
            a.queue_draw();
        };
    
    a.signal_pressed().connect([&](art::engine::Coord2D pos) { toggle_zoom(pos); });
    a.signal_released().connect([&]() { toggle_zoom(art::engine::Coord2D(-1, -1)); });
    a.signal_moved().connect([&](art::engine::Coord2D pos) { a.mouseMove(pos, 0); });

    p.show_all_children();
    p.set_modal(true);
    p.show();

    while (!done) {
        gtk_main_iteration();
    }
    ShortcutsManager::getInstance()->unbindOwner(&p);
}


} } // namespace art::gui
