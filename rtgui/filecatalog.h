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

#include "coarsepanel.h"
#include "exiffiltersettings.h"
#include "filebrowser.h"
#include "fileselectionchangelistener.h"
#include "fileselectionlistener.h"
#include "filterpanel.h"
#include "multilangmgr.h"
#include "previewloader.h"
#include "threadutils.h"
#include "toolbar.h"
#include <giomm.h>
#include <set>

class FilePanel;
class BatchQueue;
/*
 * Class:
 *   - handling the list of file (add/remove them)
 *   - handling the thumbnail toolbar,
 *   - monitoring the directory (for any change)
 */
class FileCatalog: public Gtk::VBox,
                   public PreviewLoaderListener,
                   public FilterPanelListener,
                   public FileBrowserListener {
public:
    typedef sigc::slot<void, const Glib::ustring &> DirSelectionSlot;

private:
    FilePanel *filepanel;
    Gtk::HBox *hBox;
    Glib::ustring selectedDirectory;
    int selectedDirectoryId;
    int refresh_counter_;
    bool enabled;
    bool inTabMode; // Tab mode has e.g. different progress bar handling
    Glib::ustring imageToSelect_fname;
    Glib::ustring
        refImageForOpen_fname; // Next/previous for Editor's perspective
    eRTNav actionNextPrevious;

    FileSelectionListener *listener;
    FileSelectionChangeListener *fslistener;
    DirSelectionSlot selectDir;

    Gtk::HBox *buttonBar;
    Gtk::HBox *hbToolBar1;
    MyScrolledToolbar *hbToolBar1STB;

    Gtk::HBox *fltrRankbox;
    Gtk::HBox *fltrLabelbox;
    Gtk::VBox *fltrVbox1;

    Gtk::HBox *fltrEditedBox;
    Gtk::HBox *fltrRecentlySavedBox;
    Gtk::VBox *fltrVbox2;

    Gtk::VSeparator *vSepiLeftPanel;

    Gtk::ToggleButton *tbLeftPanel_1;
    Gtk::ToggleButton *tbRightPanel_1;
    Gtk::ToggleButton *bFilterClear;
    Gtk::ToggleButton *bUnRanked;
    Gtk::ToggleButton *bRank[5];
    Gtk::ToggleButton *bUnCLabeled;
    Gtk::ToggleButton *bCLabel[5]; // color label
    Gtk::ToggleButton *bEdited[2];
    Gtk::ToggleButton *bRecentlySaved[2];
    Gtk::ToggleButton *bTrash;
    Gtk::ToggleButton *bNotTrash;
    Gtk::ToggleButton *bOriginal;
    Gtk::ToggleButton *categoryButtons[20];
    Gtk::ToggleButton *exifInfo;
    // PopUpButton *thumbOrder;
    Gtk::MenuButton *thumbOrder;
    std::vector<Gtk::MenuItem *> thumbOrderItems;
    std::vector<Glib::ustring> thumbOrderLabels;
    sigc::connection bCateg[20];
    Gtk::Image *iFilterClear, *igFilterClear;
    Gtk::Image *iranked[5], *igranked[5], *iUnRanked, *igUnRanked;
    Gtk::Image *iCLabeled[5], *igCLabeled[5], *iUnCLabeled, *igUnCLabeled;
    Gtk::Image *iEdited[2], *igEdited[2];
    Gtk::Image *iRecentlySaved[2], *igRecentlySaved[2];
    Gtk::Image *iTrashShowEmpty, *iTrashShowFull;
    Gtk::Image *iNotTrash, *iOriginal;
    Gtk::Image *iRefreshWhite, *iRefreshRed;
    Gtk::Image *iLeftPanel_1_Show, *iLeftPanel_1_Hide, *iRightPanel_1_Show,
        *iRightPanel_1_Hide;
    Gtk::Image *iQueryClear;

    Gtk::Entry *BrowsePath;
    Gtk::Button *buttonBrowsePath;
    Gtk::ToggleButton *button_recurse_;
    sigc::connection recurse_conn_;
    bool is_toggling_recurse_;
    Gtk::Button *button_session_add_;
    Gtk::Button *button_session_remove_;
    Gtk::Button *button_session_load_;
    Gtk::Button *button_session_save_;
    Glib::RefPtr<Gtk::EntryCompletion> browsePathCompletion;

    Gtk::Entry *Query;
    Gtk::Button *buttonQueryClear;

    Gtk::Label *selection_counter_;

    double hScrollPos[18];
    double vScrollPos[18];
    int lastScrollPos;

    Gtk::VBox *trashButtonBox;

    Gtk::Button *zoomInButton;
    Gtk::Button *zoomOutButton;

    MyMutex dirEFSMutex;
    ExifFilterSettings dirEFS;
    ExifFilterSettings currentEFS;
    bool hasValidCurrentEFS;

    FilterPanel *filterPanel;
    bool filter_panel_update_;

    int previewsToLoad;
    int previewsLoaded;

    std::vector<Glib::ustring> fileNameList;
    std::unordered_set<std::string> file_name_set_;

    std::set<Glib::ustring> editedFiles;
    guint modifierKey; // any modifiers held when rank button was pressed

    static const unsigned int DIR_REFRESH_DELAY = 2000;
    Glib::RefPtr<Gio::FileMonitor> dirMonitor;
    sigc::connection dir_refresh_conn_;

    IdleRegister idle_register;

    BatchQueue *bqueue_;
    std::vector<Thumbnail *> to_open_;

    void addAndOpenFile(const Glib::ustring &fname, bool force = false);
    void addFile(const Glib::ustring &fName);
    std::vector<Glib::ustring> getFileList(bool recursive);
    BrowserFilter getFilter();
    void trashChanged();

    void onBrowsePathChanged();
    Glib::ustring getBrowsePath();

    void removeFromBatchQueue(const std::vector<FileBrowserEntry *> &tbe);

    void on_dir_changed(const Glib::RefPtr<Gio::File> &file,
                        const Glib::RefPtr<Gio::File> &other_file,
                        Gio::FileMonitorEvent event_type);

public:
    // thumbnail browsers
    FileBrowser *fileBrowser;

    FileCatalog(FilePanel *filepanel);
    ~FileCatalog() override;
    void dirSelected(const Glib::ustring &dirname,
                     const Glib::ustring &openfile);
    void closeDir();
    void refreshEditedState(const std::set<Glib::ustring> &efiles);

    // previewloaderlistener interface
    void previewReady(int dir_id, FileBrowserEntry *fdn) override;
    void previewsFinished(int dir_id) override;
    void previewsFinishedUI();
    void _refreshProgressBar();

    void setInspector(Inspector *inspector)
    {
        if (fileBrowser) {
            fileBrowser->setInspector(inspector);
        }
    }
    void disableInspector();
    void enableInspector();

    // filterpanel interface
    void exifFilterChanged() override;

    Glib::ustring lastSelectedDir() { return selectedDirectory; }
    void setEnabled(bool e);         // if not enabled, it does not open image
    void enableTabMode(bool enable); // sets progress bar

    // accessors for FileBrowser
    void redrawAll();
    void refreshThumbImages();
    void refreshHeight();

    void filterApplied() override;
    void openRequested(const std::vector<Thumbnail *> &tbe) override;
    void deleteRequested(const std::vector<FileBrowserEntry *> &tbe,
                         bool onlySelected) override;
    void copyMoveRequested(const std::vector<FileBrowserEntry *> &tbe,
                           bool moveRequested) override;
    void developRequested(const std::vector<FileBrowserEntry *> &tbe,
                          bool fastmode) override;
    // void renameRequested(const std::vector<FileBrowserEntry*>& tbe) override;
    void selectionChanged(const std::vector<Thumbnail *> &tbe) override;
    void clearFromCacheRequested(const std::vector<FileBrowserEntry *> &tbe,
                                 bool leavenotrace) override;
    bool isInTabMode() const override;

    void emptyTrash();
    bool trashIsEmpty();

    void setFileSelectionListener(FileSelectionListener *l) { listener = l; }
    void setFileSelectionChangeListener(FileSelectionChangeListener *l)
    {
        fslistener = l;
    }
    void setDirSelector(const DirSelectionSlot &selectDir);

    void setFilterPanel(FilterPanel *fpanel);
    void exifInfoButtonToggled();
    void categoryButtonToggled(Gtk::ToggleButton *b, bool isMouseClick);
    bool capture_event(GdkEventButton *event);
    void filterChanged();
    void runFilterDialog();

    void on_realize() override;
    void reparseDirectory();
    void _openImage();

    void zoomIn();
    void zoomOut();

    void buttonBrowsePathPressed();
    void browsePathRefresh();

    void sessionAddPressed();
    void sessionRemovePressed();
    void sessionLoadPressed();
    void sessionSavePressed();

    bool BrowsePath_key_pressed(GdkEventKey *event);
    void buttonQueryClearPressed();
    void executeQuery();
    bool Query_key_pressed(GdkEventKey *event);
    void updateFBQueryTB(bool singleRow);
    void updateFBToolBarVisibility(bool showFilmStripToolBar);

    void tbLeftPanel_1_toggled();
    void tbLeftPanel_1_visible(bool visible);
    void tbRightPanel_1_toggled();
    void tbRightPanel_1_visible(bool visible);

    void openNextImage() { fileBrowser->openNextImage(); }
    void openPrevImage() { fileBrowser->openPrevImage(); }
    void selectImage(Glib::ustring fname, bool clearFilters);
    bool isSelected(const Glib::ustring &fname) const;
    void openNextPreviousEditorImage(Glib::ustring fname, bool clearFilters,
                                     eRTNav nextPrevious);

    bool handleShortcutKey(GdkEventKey *event);

    bool CheckSidePanelsVisibility();
    void toggleSidePanels();
    void toggleLeftPanel();
    void toggleRightPanel();
    void setupSidePanels();

    void showToolBar();
    void hideToolBar();

    void setBatchQueue(BatchQueue *bq) { bqueue_ = bq; }
};

inline void
FileCatalog::setDirSelector(const FileCatalog::DirSelectionSlot &selectDir)
{
    this->selectDir = selectDir;
}
