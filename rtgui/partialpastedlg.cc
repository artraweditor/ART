/*
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
#include "partialpastedlg.h"
#include "guiutils.h"
#include "multilangmgr.h"
#include "paramsedited.h"

namespace {

const std::vector<std::string> groups = {
    "PARTIALPASTE_EXPOSUREGROUP", "PARTIALPASTE_DETAILGROUP",
    "PARTIALPASTE_EFFECTSGROUP",  "PARTIALPASTE_COLORGROUP",
    "PARTIALPASTE_LENSGROUP",     "PARTIALPASTE_COMPOSITIONGROUP",
    "PARTIALPASTE_LOCALGROUP",    "PARTIALPASTE_RAWGROUP",
    "PARTIALPASTE_METAGROUP"};

struct ToggleInfo {
    std::string label;
    bool *edited;
    int index;
    unsigned *edited3;
};

std::vector<ToggleInfo> get_toggles(ParamsEdited &pedited)
{
    return {
        {"PARTIALPASTE_EXPOSURE", &pedited.exposure, 0, nullptr},
        {"PARTIALPASTE_TONE_EQUALIZER", &pedited.toneEqualizer, 0, nullptr},
        {"PARTIALPASTE_TONECURVE", &pedited.toneCurve, 0, nullptr},
        {"PARTIALPASTE_TM_FATTAL", &pedited.fattal, 0, nullptr},
        {"PARTIALPASTE_TM_LOG", &pedited.logenc, 0, nullptr},

        {"PARTIALPASTE_SPOT", &pedited.spot, 1, nullptr},
        {"PARTIALPASTE_SHARPENING", &pedited.sharpening, 1, nullptr},
        {"PARTIALPASTE_DIRPYRDENOISE", &pedited.denoise, 1, nullptr},
        {"PARTIALPASTE_IMPULSEDENOISE", &pedited.impulseDenoise, 1, nullptr},
        {"PARTIALPASTE_DEFRINGE", &pedited.defringe, 1, nullptr},

        {"PARTIALPASTE_CHANNELMIXERBW", &pedited.blackwhite, 2, nullptr},
        {"PARTIALPASTE_FILMSIMULATION", &pedited.filmSimulation, 2, nullptr},
        {"PARTIALPASTE_SOFTLIGHT", &pedited.softlight, 2, nullptr},
        {"PARTIALPASTE_PCVIGNETTE", &pedited.pcvignette, 2, nullptr},
        {"PARTIALPASTE_GRADIENT", &pedited.gradient, 2, nullptr},
        {"PARTIALPASTE_DEHAZE", &pedited.dehaze, 2, nullptr},
        {"PARTIALPASTE_GRAIN", &pedited.grain, 2, nullptr},
        {"PARTIALPASTE_FILMNEGATIVE", &pedited.filmNegative, 2, nullptr},

        {"PARTIALPASTE_WHITEBALANCE", &pedited.wb, 3, nullptr},
        {"PARTIALPASTE_ICMSETTINGS", &pedited.icm, 3, nullptr},
        {"PARTIALPASTE_SATURATION", &pedited.saturation, 3, nullptr},
        {"PARTIALPASTE_CHANNELMIXER", &pedited.chmixer, 3, nullptr},
        {"PARTIALPASTE_HSLEQUALIZER", &pedited.hsl, 3, nullptr},
        {"PARTIALPASTE_RGBCURVES", &pedited.rgbCurves, 3, nullptr},
        {"PARTIALPASTE_LABCURVE", &pedited.labCurve, 3, nullptr},

        {"PARTIALPASTE_DISTORTION", &pedited.distortion, 4, nullptr},
        {"PARTIALPASTE_CACORRECTION", &pedited.cacorrection, 4, nullptr},
        {"PARTIALPASTE_VIGNETTING", &pedited.vignetting, 4, nullptr},
        {"PARTIALPASTE_LENSPROFILE", &pedited.lensProf, 4, nullptr},

        {"PARTIALPASTE_COARSETRANS", &pedited.coarse, 5, nullptr},
        {"PARTIALPASTE_ROTATION", &pedited.rotate, 5, nullptr},
        {"PARTIALPASTE_CROP", &pedited.crop, 5, nullptr},
        {"PARTIALPASTE_RESIZE", &pedited.resize, 5, nullptr},
        {"PARTIALPASTE_PRSHARPENING", &pedited.prsharpening, 5, nullptr},
        {"PARTIALPASTE_PERSPECTIVE", &pedited.perspective, 5, nullptr},
        {"PARTIALPASTE_COMMONTRANSFORMPARAMS", &pedited.commonTrans, 5,
         nullptr},

        {"PARTIALPASTE_COLORCORRECTION", nullptr, 6, &pedited.colorcorrection},
        {"PARTIALPASTE_SMOOTHING", nullptr, 6, &pedited.smoothing},
        {"PARTIALPASTE_LOCALCONTRAST", nullptr, 6, &pedited.localContrast},
        {"PARTIALPASTE_TEXTUREBOOST", nullptr, 6, &pedited.textureBoost},

        {"PARTIALPASTE_RAW_DEMOSAIC", &pedited.demosaic, 7, nullptr},
        {"PARTIALPASTE_RAW_BLACK", &pedited.rawBlack, 7, nullptr},
        {"PARTIALPASTE_RAW_WHITE", &pedited.rawWhite, 7, nullptr},
        {"PARTIALPASTE_RAW_PREPROCESSING", &pedited.rawPreprocessing, 7,
         nullptr},
        {"PARTIALPASTE_RAWCA", &pedited.rawCA, 7, nullptr},
        {"PARTIALPASTE_HOT_DEAD_PIXEL_FILTER", &pedited.hotDeadPixelFilter, 7,
         nullptr},
        {"PARTIALPASTE_DARKFRAME", &pedited.darkframe, 7, nullptr},
        {"PARTIALPASTE_FLATFIELD", &pedited.flatfield, 7, nullptr},

        {"PARTIALPASTE_METADATA", &pedited.metadata, 8, nullptr},
        {"PARTIALPASTE_EXIFCHANGES", &pedited.exif, 8, nullptr},
        {"PARTIALPASTE_IPTCINFO", &pedited.iptc, 8, nullptr}};
}

} // namespace

PartialPasteDlg::PartialPasteDlg(const Glib::ustring &title,
                                 Gtk::Window *parent)
    : Gtk::Dialog(title, *parent, true), allow_3way_(false)
{
    set_default_size(700, 600);

    everything_ =
        Gtk::manage(new Gtk::CheckButton(M("PARTIALPASTE_EVERYTHING")));
    everything_->set_name("PartialPasteHeader");
    everything_conn_ = everything_->signal_toggled().connect(sigc::bind(
        sigc::mem_fun(*this, &PartialPasteDlg::toggled), everything_));

    Gtk::VBox *vboxes[9];
    Gtk::HSeparator *hseps[9];

    for (int i = 0; i < 9; i++) {
        vboxes[i] = Gtk::manage(new Gtk::VBox());
        vboxes[i]->set_name("PartialPasteGroupContainer");
        hseps[i] = Gtk::manage(new Gtk::HSeparator());
        hseps[i]->set_name("PartialPasteHeaderSep");
    }

    Gtk::VBox *vbCol1 = Gtk::manage(new Gtk::VBox());
    Gtk::VBox *vbCol2 = Gtk::manage(new Gtk::VBox());
    Gtk::VBox *vbCol3 = Gtk::manage(new Gtk::VBox());

    for (int i = 0; i < 3; i++) {
        vbCol1->pack_start(*vboxes[i], Gtk::PACK_SHRINK, 2);
    }

    for (int i = 3; i < 6; i++) {
        vbCol2->pack_start(*vboxes[i], Gtk::PACK_SHRINK, 2);
    }

    for (int i = 6; i < 9; i++) {
        vbCol3->pack_start(*vboxes[i], Gtk::PACK_SHRINK, 2);
    }

    Gtk::VBox *vbtop = Gtk::manage(new Gtk::VBox());
    vbtop->pack_start(*everything_, Gtk::PACK_SHRINK, 2);

    Gtk::Dialog::get_content_area()->pack_start(*vbtop, Gtk::PACK_SHRINK, 2);

    Gtk::HBox *hbmain = Gtk::manage(new Gtk::HBox());
    hbmain->pack_start(*vbCol1);
    Gtk::VSeparator *vsep1 = Gtk::manage(new Gtk::VSeparator());
    setExpandAlignProperties(vsep1, false, true, Gtk::ALIGN_CENTER,
                             Gtk::ALIGN_FILL);
    hbmain->pack_start(*vsep1);
    hbmain->pack_start(*vbCol2);
    Gtk::VSeparator *vsep2 = Gtk::manage(new Gtk::VSeparator());
    setExpandAlignProperties(vsep2, false, true, Gtk::ALIGN_CENTER,
                             Gtk::ALIGN_FILL);
    hbmain->pack_start(*vsep2);
    hbmain->pack_start(*vbCol3);

    Gtk::ScrolledWindow *scrolledwindow =
        Gtk::manage(new Gtk::ScrolledWindow());
    scrolledwindow->set_name("PartialPaste");
    scrolledwindow->set_can_focus(true);
    scrolledwindow->set_shadow_type(Gtk::SHADOW_NONE);
    scrolledwindow->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    scrolledwindow->property_window_placement().set_value(Gtk::CORNER_TOP_LEFT);

    scrolledwindow->add(*hbmain);

    Gtk::Dialog::get_content_area()->pack_start(*scrolledwindow,
                                                Gtk::PACK_EXPAND_WIDGET, 2);

    std::vector<Gtk::CheckButton *> gbtns;
    size_t i = 0;
    for (const auto &g : groups) {
        auto b = Gtk::manage(new Gtk::CheckButton(M(g)));
        gbtns.push_back(b);
        b->set_name("PartialPasteHeader");
        vboxes[i]->pack_start(*b, Gtk::PACK_SHRINK, 2);
        vboxes[i]->pack_start(*hseps[i], Gtk::PACK_SHRINK, 2);
        ++i;

        auto conn = b->signal_toggled().connect(
            sigc::bind(sigc::mem_fun(*this, &PartialPasteDlg::toggled), b));
        buttons_[b] = {conn, {}, true, nullptr, nullptr};
    }

    for (const auto &t : get_toggles(pedited_)) {
        auto b = Gtk::manage(new Gtk::CheckButton(M(t.label)));
        auto conn = b->signal_toggled().connect(
            sigc::bind(sigc::mem_fun(*this, &PartialPasteDlg::toggled), b));
        int i = t.index;
        auto master = gbtns[i];
        buttons_[b] = {conn, {master}, false, t.edited, t.edited3};
        buttons_[master].related.push_back(b);
        vboxes[i]->pack_start(*b, Gtk::PACK_SHRINK, 2);
    }

    hbmain->show();
    scrolledwindow->show();

    add_button(M("GENERAL_OK"), Gtk::RESPONSE_OK);
    add_button(M("GENERAL_CANCEL"), Gtk::RESPONSE_CANCEL);
    set_response_sensitive(Gtk::RESPONSE_OK);
    set_default_response(Gtk::RESPONSE_OK);
    show_all_children();
}

void PartialPasteDlg::toggled(Gtk::CheckButton *which)
{
    if (which == everything_) {
        which->set_inconsistent(false);
        for (auto &p : buttons_) {
            auto b = p.first;
            ConnectionBlocker blocker(p.second.conn);
            b->set_active(which->get_active());
        }
    } else {
        auto &g = buttons_[which];
        if (!which->get_active()) {
            ConnectionBlocker blocker(everything_conn_);
            everything_->set_active(false);
        }
        if (allow_3way_ && g.edited3) {
            ConnectionBlocker blocker(g.conn);
            if (which->get_inconsistent()) {
                which->set_inconsistent(false);
                which->set_active(false);
            } else if (!which->get_active()) {
                which->set_inconsistent(true);
                which->set_active(true);
            }
        }
        if (g.is_master || !which->get_active()) {
            for (auto c : g.related) {
                auto &ci = buttons_[c];
                ConnectionBlocker blocker(ci.conn);
                c->set_active(which->get_active());
            }
        }
    }
}

ParamsEdited PartialPasteDlg::getParamsEdited()
{
    for (auto &p : buttons_) {
        if (p.second.edited) {
            *p.second.edited = p.first->get_active();
        } else if (p.second.edited3) {
            *p.second.edited3 = p.first->get_inconsistent()
                                    ? ParamsEdited::Undef
                                    : p.first->get_active();
        }
    }
    return pedited_;
}
