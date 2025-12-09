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

#include "toolpanel.h"
#include <gtkmm.h>
#include <unordered_set>

class ExifPanel: public Gtk::VBox, public ToolPanel {

private:
    const rtengine::FramesMetaData *idata;
    rtengine::procparams::ExifPairs changeList;
    rtengine::procparams::ExifPairs defChangeList;

    class ExifColumns: public Gtk::TreeModelColumnRecord {
    public:
        Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>> icon;
        Gtk::TreeModelColumn<std::string> key;
        Gtk::TreeModelColumn<Glib::ustring> label;
        Gtk::TreeModelColumn<Glib::ustring> value;
        Gtk::TreeModelColumn<Glib::ustring> value_nopango;
        Gtk::TreeModelColumn<bool> editable;
        Gtk::TreeModelColumn<bool> edited;
        Gtk::TreeModelColumn<bool> active;
        Gtk::TreeModelColumn<bool> is_group;
        Gtk::TreeModelColumn<bool> activatable;
        Gtk::TreeModelColumn<Glib::ustring> tooltip;

        ExifColumns()
        {
            add(key);
            add(label);
            add(value);
            add(icon);
            add(edited);
            add(value_nopango);
            add(editable);
            add(active);
            add(is_group);
            add(activatable);
            add(tooltip);
        }
    };
    Glib::RefPtr<Gdk::Pixbuf> editicon;
    Glib::RefPtr<Gdk::Pixbuf> open_icon_;
    Glib::RefPtr<Gdk::Pixbuf> closed_icon_;

    ExifColumns exifColumns;
    Gtk::TreeView *exifTree;
    Gtk::ScrolledWindow *scrolledWindow;
    Glib::RefPtr<Gtk::TreeStore> exifTreeModel;

    Gtk::Button *add;
    Gtk::Button *reset;
    Gtk::Button *resetAll;
    Gtk::Button *activate_all_;
    Gtk::Button *activate_none_;

    Gtk::CellRendererToggle exif_active_renderer_;
    Gtk::TreeView::Column exif_active_column_;

    std::vector<std::pair<std::string, Glib::ustring>> editable_;

    std::unordered_set<std::string> initial_active_keys_;
    std::unordered_set<std::string> cur_active_keys_;

    rtengine::ProgressListener *pl_;

    void addTag(const std::string &key,
                const std::pair<Glib::ustring, Glib::ustring> &label,
                const Glib::ustring &value, bool editable, bool edited,
                bool activatable = true);
    void refreshTags();
    void resetIt(Gtk::TreeModel::iterator iter);
    void resetPressed();
    void resetAllPressed();
    void addPressed();
    void activateAllPressed();
    void activateNonePressed();

    void setKeyActive(Gtk::CellRenderer *renderer,
                      const Gtk::TreeModel::iterator &it);
    void onKeyActiveToggled(const Glib::ustring &path);

    bool all_keys_active() const;
    std::unordered_set<std::string> get_active_keys() const;

    void onExifTreeClick(GdkEventButton *event);
    void onExifRowExpanded(const Gtk::TreeModel::iterator &it,
                           const Gtk::TreeModel::Path &path);
    void onExifRowCollapsed(const Gtk::TreeModel::iterator &it,
                            const Gtk::TreeModel::Path &path);

    void setExifTagValue(Gtk::CellRenderer *renderer,
                         const Gtk::TreeModel::iterator &it);
    void onEditExifTagValue(const Glib::ustring &path,
                            const Glib::ustring &value);

public:
    ExifPanel();
    ~ExifPanel() override;

    void read(const rtengine::procparams::ProcParams *pp) override;
    void write(rtengine::procparams::ProcParams *pp) override;
    void
    setDefaults(const rtengine::procparams::ProcParams *defParams) override;

    void setImageData(const rtengine::FramesMetaData *id);

    void exifSelectionChanged();
    // void row_activated (const Gtk::TreeModel::Path& path,
    // Gtk::TreeViewColumn* column);

    void notifyListener();

    void setProgressListener(rtengine::ProgressListener *pl);
};
