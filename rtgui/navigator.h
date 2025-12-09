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
#ifndef _NAVIGATOR_
#define _NAVIGATOR_

#include "../rtengine/iccstore.h"
#include "options.h"
#include "pointermotionlistener.h"
#include "previewwindow.h"
#include <gtkmm.h>

class Navigator: public Gtk::Frame, public PointerMotionListener {
private:
    Options::NavigatorUnit currentRGBUnit;
    Options::NavigatorUnit currentLCHUnit;
    void cycleUnitsRGB(GdkEventButton *event);
    void cycleUnitsLCH(GdkEventButton *event);

protected:
    Gtk::Label *metaInfo;
    Glib::ustring dimension;
    Gtk::Label *position;
    Gtk::Label *R, *G, *B;
    Gtk::Label *L, *C, *H;
    Gtk::Label *LAB_A, *LAB_B, *LAB_L;

    Gtk::Label *lR, *lG, *lB;
    Gtk::Label *lL, *lC, *lH;
    Gtk::Label *lLAB_A, *lLAB_B, *lLAB_L;

public:
    PreviewWindow *previewWindow;

    Navigator();

    // pointermotionlistener interface
    //  void pointerMoved (bool validPos, int x, int y, int r, int g, int b);
    void pointerMoved(bool validPos, const Glib::ustring &profile,
                      const Glib::ustring &profileW, int x, int y, int r, int g,
                      int b, bool raw = false) override;
    void setInvalid(int fullWidth = -1, int fullHeight = -1);
    void setMetaInfo(const rtengine::FramesMetaData *idata);

    void getRGBText(int r, int g, int b, Glib::ustring &sR, Glib::ustring &sG,
                    Glib::ustring &sB, bool isRaw = false) override;
    void getLCHText(float l, float c, float h, Glib::ustring &sL,
                    Glib::ustring &sC, Glib::ustring &sH) override;
    void getLABText(float l, float a, float b, Glib::ustring &sL,
                    Glib::ustring &sA, Glib::ustring &sB) override;
};

#endif
