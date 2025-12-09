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
#include "navigator.h"
#include "../rtengine/color.h"
#include "../rtengine/curves.h"
#include "../rtengine/iccmatrices.h"
#include "../rtengine/iccstore.h"
#include "../rtengine/rt_math.h"
#include "options.h"
#include "toolpanel.h"
#include <iomanip>

extern Options options;

using namespace rtengine;

Navigator::Navigator()
    : currentRGBUnit(options.navRGBUnit), currentLCHUnit(options.navLCHUnit)
{

    set_label(M("MAIN_MSG_NAVIGATOR"));
    set_label_align(0.025, 0.5);
    Gtk::VBox *mbox = Gtk::manage(new Gtk::VBox());
    mbox->set_name("Navigator");
    previewWindow = Gtk::manage(new PreviewWindow());
    mbox->pack_start(*previewWindow, Gtk::PACK_SHRINK, 2);
    metaInfo = Gtk::manage(new Gtk::Label());
    mbox->pack_start(*metaInfo, Gtk::PACK_SHRINK, 2);

    position = Gtk::manage(new Gtk::Label());
    mbox->pack_start(*position, Gtk::PACK_SHRINK, 2);

    // labels
    lR = Gtk::manage(new Gtk::Label(M("NAVIGATOR_R")));
    lG = Gtk::manage(new Gtk::Label(M("NAVIGATOR_G")));
    lB = Gtk::manage(new Gtk::Label(M("NAVIGATOR_B")));
    lL = Gtk::manage(new Gtk::Label(M("NAVIGATOR_L")));
    lC = Gtk::manage(new Gtk::Label(M("NAVIGATOR_C")));
    lH = Gtk::manage(new Gtk::Label(M("NAVIGATOR_H")));
    lLAB_A = Gtk::manage(new Gtk::Label(M("NAVIGATOR_LAB_A")));
    lLAB_B = Gtk::manage(new Gtk::Label(M("NAVIGATOR_LAB_B")));
    lLAB_L = Gtk::manage(new Gtk::Label(M("NAVIGATOR_LAB_L")));

    // left-align labels
    lR->set_alignment(Gtk::ALIGN_START);
    lG->set_alignment(Gtk::ALIGN_START);
    lB->set_alignment(Gtk::ALIGN_START);
    lL->set_alignment(Gtk::ALIGN_START);
    lC->set_alignment(Gtk::ALIGN_START);
    lH->set_alignment(Gtk::ALIGN_START);
    lLAB_A->set_alignment(Gtk::ALIGN_START);
    lLAB_B->set_alignment(Gtk::ALIGN_START);
    lLAB_L->set_alignment(Gtk::ALIGN_START);

    // values
    R = Gtk::manage(new Gtk::Label());
    G = Gtk::manage(new Gtk::Label());
    B = Gtk::manage(new Gtk::Label());
    L = Gtk::manage(new Gtk::Label());
    C = Gtk::manage(new Gtk::Label());
    H = Gtk::manage(new Gtk::Label());
    LAB_A = Gtk::manage(new Gtk::Label());
    LAB_B = Gtk::manage(new Gtk::Label());
    LAB_L = Gtk::manage(new Gtk::Label());

    // right-align values
    R->set_alignment(Gtk::ALIGN_END);
    G->set_alignment(Gtk::ALIGN_END);
    B->set_alignment(Gtk::ALIGN_END);
    L->set_alignment(Gtk::ALIGN_END);
    C->set_alignment(Gtk::ALIGN_END);
    H->set_alignment(Gtk::ALIGN_END);
    LAB_A->set_alignment(Gtk::ALIGN_END);
    LAB_B->set_alignment(Gtk::ALIGN_END);
    LAB_L->set_alignment(Gtk::ALIGN_END);

    // set font family and size
    /*
    Glib::ustring fontname;

#ifdef WIN32
    fontname = "Droid Sans Mono Slashed"; // font file is provided in the source
tree in rtdata/fonts to be installed by the windows installer #endif

#ifdef __linux__
    fontname = "Monospace";
#endif

#ifdef __APPLE__
    fontname = "Menlo";
#endif

    if (0) { // (fontname!=""){
        Glib::RefPtr<Gtk::CssProvider> cssProvider = Gtk::CssProvider::create();

        if (cssProvider) {
            cssProvider->load_from_data("Label { font-name: " + fontname + "
}"); R->get_style_context()->add_provider(cssProvider,
GTK_STYLE_PROVIDER_PRIORITY_USER);
            G->get_style_context()->add_provider(cssProvider,
GTK_STYLE_PROVIDER_PRIORITY_USER);
            B->get_style_context()->add_provider(cssProvider,
GTK_STYLE_PROVIDER_PRIORITY_USER);
            H->get_style_context()->add_provider(cssProvider,
GTK_STYLE_PROVIDER_PRIORITY_USER);
            S->get_style_context()->add_provider(cssProvider,
GTK_STYLE_PROVIDER_PRIORITY_USER);
            V->get_style_context()->add_provider(cssProvider,
GTK_STYLE_PROVIDER_PRIORITY_USER);
            LAB_A->get_style_context()->add_provider(cssProvider,
GTK_STYLE_PROVIDER_PRIORITY_USER);
            LAB_B->get_style_context()->add_provider(cssProvider,
GTK_STYLE_PROVIDER_PRIORITY_USER);
            LAB_L->get_style_context()->add_provider(cssProvider,
GTK_STYLE_PROVIDER_PRIORITY_USER);

            lR->get_style_context()->add_provider(cssProvider,
GTK_STYLE_PROVIDER_PRIORITY_USER);
            lG->get_style_context()->add_provider(cssProvider,
GTK_STYLE_PROVIDER_PRIORITY_USER);
            lB->get_style_context()->add_provider(cssProvider,
GTK_STYLE_PROVIDER_PRIORITY_USER);
            lH->get_style_context()->add_provider(cssProvider,
GTK_STYLE_PROVIDER_PRIORITY_USER);
            lS->get_style_context()->add_provider(cssProvider,
GTK_STYLE_PROVIDER_PRIORITY_USER);
            lV->get_style_context()->add_provider(cssProvider,
GTK_STYLE_PROVIDER_PRIORITY_USER);
            lLAB_A->get_style_context()->add_provider(cssProvider,
GTK_STYLE_PROVIDER_PRIORITY_USER);
            lLAB_B->get_style_context()->add_provider(cssProvider,
GTK_STYLE_PROVIDER_PRIORITY_USER);
            lLAB_L->get_style_context()->add_provider(cssProvider,
GTK_STYLE_PROVIDER_PRIORITY_USER);

            position->get_style_context()->add_provider(cssProvider,
GTK_STYLE_PROVIDER_PRIORITY_USER);
        }
    }
    */

    // setup the tables
    Gtk::Table *table0 = Gtk::manage(
        new Gtk::Table(1, 3)); // rows, cols The main table container
    // let's pack tables1,2-3 into table0

    // RGB
    Gtk::EventBox *evBox1 = Gtk::manage(new Gtk::EventBox());
    Gtk::HBox *hbox1 = Gtk::manage(new Gtk::HBox()); // container
    Gtk::Table *table1 = Gtk::manage(new Gtk::Table(3, 2));

    table1->attach(*lR, 0, 1, 0, 1, Gtk::SHRINK, Gtk::SHRINK, 4, 0);
    table1->attach(*R, 1, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK, 0, 0);

    table1->attach(*lG, 0, 1, 1, 2, Gtk::SHRINK, Gtk::SHRINK, 4, 0);
    table1->attach(*G, 1, 2, 1, 2, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK, 0, 0);

    table1->attach(*lB, 0, 1, 2, 3, Gtk::SHRINK, Gtk::SHRINK, 4, 0);
    table1->attach(*B, 1, 2, 2, 3, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK, 0, 0);

    evBox1->add(*table1);
    evBox1->signal_button_release_event().connect_notify(
        sigc::mem_fun(*this, &Navigator::cycleUnitsRGB));

    hbox1->pack_start(*evBox1, Gtk::PACK_EXPAND_WIDGET, 4);
    hbox1->pack_start(*Gtk::manage(new Gtk::VSeparator()), Gtk::PACK_SHRINK, 4);
    table0->attach(*hbox1, 0, 1, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK, 0,
                   0);

    // HSV
    Gtk::EventBox *evBox2 = Gtk::manage(new Gtk::EventBox());
    Gtk::HBox *hbox2 = Gtk::manage(new Gtk::HBox()); // container
    Gtk::Table *table2 = Gtk::manage(new Gtk::Table(3, 2));

    table2->attach(*lL, 0, 1, 0, 1, Gtk::SHRINK, Gtk::SHRINK, 4, 0);
    table2->attach(*L, 1, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK, 0, 0);

    table2->attach(*lC, 0, 1, 1, 2, Gtk::SHRINK, Gtk::SHRINK, 4, 0);
    table2->attach(*C, 1, 2, 1, 2, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK, 0, 0);

    table2->attach(*lH, 0, 1, 2, 3, Gtk::SHRINK, Gtk::SHRINK, 4, 0);
    table2->attach(*H, 1, 2, 2, 3, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK, 0, 0);

    evBox2->add(*table2);
    evBox2->signal_button_release_event().connect_notify(
        sigc::mem_fun(*this, &Navigator::cycleUnitsLCH));

    hbox2->pack_start(*evBox2, Gtk::PACK_EXPAND_WIDGET, 4);
    hbox2->pack_start(*Gtk::manage(new Gtk::VSeparator()), Gtk::PACK_SHRINK, 4);
    table0->attach(*hbox2, 1, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK, 0,
                   0);

    // LAB
    Gtk::HBox *hbox3 = Gtk::manage(new Gtk::HBox()); // container
    Gtk::Table *table3 = Gtk::manage(new Gtk::Table(3, 2));

    table3->attach(*lLAB_L, 0, 1, 0, 1, Gtk::SHRINK, Gtk::SHRINK, 4, 0);
    table3->attach(*LAB_L, 1, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK, 0,
                   0);

    table3->attach(*lLAB_A, 0, 1, 1, 2, Gtk::SHRINK, Gtk::SHRINK, 4, 0);
    table3->attach(*LAB_A, 1, 2, 1, 2, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK, 0,
                   0);

    table3->attach(*lLAB_B, 0, 1, 2, 3, Gtk::SHRINK, Gtk::SHRINK, 4, 0);
    table3->attach(*LAB_B, 1, 2, 2, 3, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK, 0,
                   0);

    hbox3->pack_start(*table3, Gtk::PACK_EXPAND_WIDGET, 4);
    hbox3->pack_start(*Gtk::manage(new Gtk::HBox()), Gtk::PACK_SHRINK, 2);
    table0->attach(*hbox3, 2, 3, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK, 0,
                   0);

    table0->set_homogeneous(
        true); // all cells will be the same size as the largest cell.

    mbox->pack_start(*table0, Gtk::PACK_EXPAND_WIDGET, 2);
    add(*mbox);

    setInvalid();
    show_all();
}

void Navigator::setInvalid(int fullWidth, int fullHeight)
{
    if (fullWidth > 0 && fullHeight > 0) {
        dimension = Glib::ustring::compose("%1 x %2", fullWidth, fullHeight);
    }
    position->set_text(
        Glib::ustring::compose("%1   (%2)", M("NAVIGATOR_XY_NA"), dimension));

    R->set_text(M("NAVIGATOR_NA"));
    G->set_text(M("NAVIGATOR_NA"));
    B->set_text(M("NAVIGATOR_NA"));
    L->set_text(M("NAVIGATOR_NA"));
    C->set_text(M("NAVIGATOR_NA"));
    H->set_text(M("NAVIGATOR_NA"));
    LAB_A->set_text(M("NAVIGATOR_NA"));
    LAB_B->set_text(M("NAVIGATOR_NA"));
    LAB_L->set_text(M("NAVIGATOR_NA"));
}

void Navigator::setMetaInfo(const rtengine::FramesMetaData *idata)
{
    Glib::ustring infoString = Glib::ustring::compose(
        "%1 %2   %3 sec   f/%4   %5 mm", M("QINFO_ISO"), idata->getISOSpeed(),
        Glib::ustring(idata->shutterToString(idata->getShutterSpeed())),
        Glib::ustring(idata->apertureToString(idata->getFNumber())),
        Glib::ustring::format(std::setw(3), std::fixed, std::setprecision(2),
                              idata->getFocalLen()));

    Glib::ustring expcomp =
        Glib::ustring(idata->expcompToString(idata->getExpComp(), true));

    if (!expcomp.empty()) {
        infoString = Glib::ustring::compose("%1   %2 ev", infoString, expcomp);
    }

    metaInfo->set_text(infoString);
}

void Navigator::getRGBText(int r, int g, int b, Glib::ustring &sR,
                           Glib::ustring &sG, Glib::ustring &sB, bool isRaw)
{
    if (isRaw) {
        sR = Glib::ustring::format(std::fixed, std::setprecision(0), r);
        sG = Glib::ustring::format(std::fixed, std::setprecision(0), g);
        sB = Glib::ustring::format(std::fixed, std::setprecision(0), b);
    } else {
        switch (currentRGBUnit) {
        case (Options::NavigatorUnit::R0_1):
            sR = Glib::ustring::format(std::fixed, std::setprecision(4),
                                       r / 255.f);
            sG = Glib::ustring::format(std::fixed, std::setprecision(4),
                                       g / 255.f);
            sB = Glib::ustring::format(std::fixed, std::setprecision(4),
                                       b / 255.f);
            break;
        case (Options::NavigatorUnit::R0_255):
            sR = Glib::ustring::format(std::fixed, std::setprecision(0), r);
            sG = Glib::ustring::format(std::fixed, std::setprecision(0), g);
            sB = Glib::ustring::format(std::fixed, std::setprecision(0), b);
            break;
        case (Options::NavigatorUnit::PERCENT):
        default:
            sR = Glib::ustring::format(std::fixed, std::setprecision(1),
                                       r * 100.f / 255.f) +
                 Glib::ustring("%");
            sG = Glib::ustring::format(std::fixed, std::setprecision(1),
                                       g * 100.f / 255.f) +
                 Glib::ustring("%");
            sB = Glib::ustring::format(std::fixed, std::setprecision(1),
                                       b * 100.f / 255.f) +
                 Glib::ustring("%");
        }
    }
}

void Navigator::getLCHText(float l, float c, float h, Glib::ustring &sL,
                           Glib::ustring &sC, Glib::ustring &sH)
{
    switch (currentLCHUnit) {
    case (Options::NavigatorUnit::R0_1):
        sL = Glib::ustring::format(std::fixed, std::setprecision(4), l);
        sC = Glib::ustring::format(std::fixed, std::setprecision(4), c);
        sH = Glib::ustring::format(std::fixed, std::setprecision(4), h);
        break;
    case (Options::NavigatorUnit::R0_255):
        sL = Glib::ustring::format(std::fixed, std::setprecision(0), l * 255);
        sC = Glib::ustring::format(std::fixed, std::setprecision(0), c * 255);
        sH = Glib::ustring::format(std::fixed, std::setprecision(0), h * 255);
        break;
    case (Options::NavigatorUnit::PERCENT):
    default:
        sL =
            Glib::ustring::format(std::fixed, std::setprecision(1), l * 100.f) +
            Glib::ustring("%");
        sC =
            Glib::ustring::format(std::fixed, std::setprecision(1), c * 100.f) +
            Glib::ustring("%");
        sH =
            Glib::ustring::format(std::fixed, std::setprecision(1), h * 360.f) +
            Glib::ustring("\xc2\xb0");
    }
}

void Navigator::getLABText(float l, float a, float b, Glib::ustring &sL,
                           Glib::ustring &sA, Glib::ustring &sB)
{
    sL = Glib::ustring::format(std::fixed, std::setprecision(1), l);
    sA = Glib::ustring::format(std::fixed, std::setprecision(1), a);
    sB = Glib::ustring::format(std::fixed, std::setprecision(1), b);
}

// if !validPos then x/y contain the full image size
void Navigator::pointerMoved(bool validPos, const Glib::ustring &profile,
                             const Glib::ustring &profileW, int x, int y, int r,
                             int g, int b, bool isRaw)
{

    if (!validPos) {
        setInvalid(x, y);
    } else {
        Glib::ustring s1, s2, s3;

        position->set_text(
            Glib::ustring::compose("x: %1, y: %2   (%3)", x, y, dimension));

        getRGBText(r, g, b, s1, s2, s3, isRaw);
        R->set_text(s1);
        G->set_text(s2);
        B->set_text(s3);
        if (isRaw) {
            L->set_text("--");
            C->set_text("--");
            H->set_text("--");
            LAB_L->set_text("--");
            LAB_A->set_text("--");
            LAB_B->set_text("--");
        } else {
            float l, c, h;
            float LAB_a, LAB_b, LAB_l;

            Color::rgb2lab01(profile, profileW, r / 255.f, g / 255.f, b / 255.f,
                             LAB_l, LAB_a, LAB_b,
                             options.rtSettings.HistogramWorking);
            getLABText(LAB_l, LAB_a, LAB_b, s1, s2, s3);
            LAB_L->set_text(s1);
            LAB_A->set_text(s2);
            LAB_B->set_text(s3);

            Color::lab2lch01(LAB_l, LAB_a, LAB_b, l, c, h);
            getLCHText(l, c, h, s1, s2, s3);
            L->set_text(s1);
            C->set_text(s2);
            H->set_text(s3);
        }
    }
}

void Navigator::cycleUnitsRGB(GdkEventButton *event)
{
    uint16_t v = (uint16_t)currentRGBUnit;
    ++v;
    if (v == (uint16_t)Options::NavigatorUnit::_COUNT) {
        v = 0;
    }
    options.navRGBUnit = currentRGBUnit = (Options::NavigatorUnit)v;

    switch (currentRGBUnit) {
    case Options::NavigatorUnit::R0_1:
        R->set_text("[0-1]");
        G->set_text("[0-1]");
        B->set_text("[0-1]");
        break;
    case Options::NavigatorUnit::R0_255:
        R->set_text("[0-255]");
        G->set_text("[0-255]");
        B->set_text("[0-255]");
        break;
    case Options::NavigatorUnit::PERCENT:
    default:
        R->set_text("[%]");
        G->set_text("[%]");
        B->set_text("[%]");
        break;
    }
    sig_cycle_rgb.emit();
}

void Navigator::cycleUnitsLCH(GdkEventButton *event)
{
    uint16_t v = (uint16_t)currentLCHUnit;
    ++v;
    if (v == (uint16_t)Options::NavigatorUnit::_COUNT) {
        v = 0;
    }
    options.navLCHUnit = currentLCHUnit = (Options::NavigatorUnit)v;

    switch (currentLCHUnit) {
    case Options::NavigatorUnit::R0_1:
        L->set_text("[0-1]");
        C->set_text("[0-1]");
        H->set_text("[0-1]");
        break;
    case Options::NavigatorUnit::R0_255:
        L->set_text("[0-255]");
        C->set_text("[0-255]");
        H->set_text("[0-255]");
        break;
    case Options::NavigatorUnit::PERCENT:
    default:
        L->set_text("[%]");
        C->set_text("[%]");
        H->set_text("[\xc2\xb0]");
        break;
    }
    sig_cycle_lch.emit();
}
