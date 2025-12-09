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

#include "../rtengine/procparams.h"
#include "adjuster.h"
#include "options.h"
#include "profilestorecombobox.h"
#include "saveformatpanel.h"
#include <gtkmm.h>
#include <unordered_map>

class SaveAsDialog: public Gtk::Dialog, public FormatChangeListener {
public:
    SaveAsDialog(const Glib::ustring &initialDir, Gtk::Window *parent);

    Glib::ustring getFileName();
    Glib::ustring getDirectory();
    SaveFormat getFormat();
    bool getForceFormatOpts();
    bool getAutoSuffix();
    bool getImmediately();
    bool getToHeadOfQueue();
    bool getToTailOfQueue();
    int getSaveMethodNum();

    void setInitialFileName(const Glib::ustring &iname);
    void setImagePath(const Glib::ustring &imagePath);

    void okPressed();
    void cancelPressed();
    void formatChanged(const Glib::ustring &format) override;
    bool keyPressed(GdkEventKey *event);

    const rtengine::procparams::PartialProfile *getExportProfile();

private:
    Gtk::FileChooserWidget *fchooser;
    Gtk::CheckButton *autoSuffix;
    Gtk::CheckButton *forceFormatOpts;
    SaveFormatPanel *formatOpts;
    Glib::ustring fname;
    std::unordered_map<std::string, Glib::RefPtr<Gtk::FileFilter>> filters_;
    Gtk::RadioButton *saveMethod[3]; /*  0 -> immediately
                                      *  1 -> putToQueueHead
                                      *  2 -> putToQueueTail
                                      */
    Gtk::CheckButton *apply_export_profile_;
    ProfileStoreComboBox *profiles_cb_;
    sigc::connection apply_export_profile_conn_;
    sigc::connection profiles_cb_conn_;

    void forceFmtOptsSwitched();
    void saveImmediatlyClicked();
    void putToQueueClicked();
    void fixExtension(const Glib::ustring &name);
    void exportProfileChanged();
};
