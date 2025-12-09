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

#include "dirbrowser.h"
#include "filecatalog.h"
#include "fileselectionlistener.h"
#include "filterpanel.h"
#include "history.h"
#include "placesbrowser.h"
#include "pparamschangelistener.h"
#include "progressconnector.h"
#include "recentbrowser.h"
#include <gtkmm.h>

class RTWindow;

class FilePanel final: public Gtk::HPaned, public FileSelectionListener {
public:
    FilePanel();
    ~FilePanel() override;

    Gtk::Paned *placespaned;
    Gtk::HPaned *dirpaned;

    Gtk::HBox *rightBox;

    DirBrowser *dirBrowser;
    FilterPanel *filterPanel;
    FileCatalog *fileCatalog;
    Gtk::Paned *ribbonPane;

    void setParent(RTWindow *p) { parent = p; }
    void
    init(); // don't call it directly, the constructor calls it as idle source
    void on_realize() override;
    void setAspect();
    void open(const Glib::ustring &d); // open a file or a directory
    void refreshEditedState(const std::set<Glib::ustring> &efiles)
    {
        fileCatalog->refreshEditedState(efiles);
    }
    void loadingThumbs(Glib::ustring str, double rate);

    // call this before closing RT: it saves file browser's related things into
    // options
    void saveOptions();

    // interface fileselectionlistener
    Result fileSelected(Thumbnail *thm) override;
    bool
    addBatchQueueJobs(const std::vector<BatchQueueEntry *> &entries) override;

    void optionsChanged();
    bool imageLoaded(Thumbnail *thm,
                     ProgressConnector<rtengine::InitialImage *> *);

    bool handleShortcutKey(GdkEventKey *event);
    void updateTPVScrollbar(bool hide);

    void showRightBox(bool yes);

    bool isInspectorVisible() const;

    bool on_button_release_event(GdkEventButton *event) override;

    RTWindow *getParent() { return parent; }

private:
    void on_NB_switch_page(Gtk::Widget *page, guint page_num);
    void on_inspector_ready();

    PlacesBrowser *placesBrowser;
    RecentBrowser *recentBrowser;

    Inspector *inspectorPanel;
    RTWindow *parent;
    Gtk::Notebook *rightNotebook;
    sigc::connection rightNotebookSwitchConn;

    struct pendingLoad {
        bool complete;
        ProgressConnector<rtengine::InitialImage *> *pc;
        Thumbnail *thm;
    };
    MyMutex pendingLoadMutex;
    std::vector<struct pendingLoad *> pendingLoads;

    int error;

    IdleRegister idle_register;
    int pane_pos_;
};
