/* -*- C++ -*-
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2017 Alberto Griggio
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

#include "dynamicprofilepanel.h"
#include "../rtengine/dynamicprofile.h"
#include "../rtengine/profilestore.h"
#include "../rtengine/rtengine.h"
#include "multilangmgr.h"
#include <iomanip>
#include <sstream>
#include <string>

//-----------------------------------------------------------------------------
// DynamicProfilePanel::EditDialog
//-----------------------------------------------------------------------------

DynamicProfilePanel::EditDialog::EditDialog(const Glib::ustring &title,
                                            Gtk::Window &parent)
    : Gtk::Dialog(title, parent)
{
    profilepath_ = Gtk::manage(new ProfileStoreComboBox());
    Gtk::HBox *hb = Gtk::manage(new Gtk::HBox());
    hb->pack_start(*Gtk::manage(new Gtk::Label(M("DYNPROFILEEDITOR_PROFILE"))),
                   false, false, 4);
    hb->pack_start(*profilepath_, true, true, 2);
    get_content_area()->pack_start(*hb, Gtk::PACK_SHRINK, 4);

    add_optional(M("EXIFFILTER_CAMERA"), has_camera_, camera_);
    add_optional(M("EXIFFILTER_LENS"), has_lens_, lens_);
    add_optional(M("EXIFFILTER_FILETYPE"), has_filetype_, filetype_);
    add_optional(M("EXIFFILTER_SOFTWARE"), has_software_, software_);

    imagetype_ = Gtk::manage(new MyComboBoxText());
    imagetype_->append(Glib::ustring("(") + M("DYNPROFILEEDITOR_IMGTYPE_ANY") +
                       ")");
    imagetype_->append(M("DYNPROFILEEDITOR_IMGTYPE_RAW"));
    imagetype_->append(M("DYNPROFILEEDITOR_IMGTYPE_NONRAW"));
    imagetype_->set_active(0);
    hb = Gtk::manage(new Gtk::HBox());
    hb->pack_start(*Gtk::manage(new Gtk::Label(M("EXIFFILTER_IMAGETYPE"))),
                   false, false, 4);
    hb->pack_start(*imagetype_, true, true, 2);
    get_content_area()->pack_start(*hb, Gtk::PACK_SHRINK, 4);

    add_range(M("EXIFFILTER_ISO"), iso_min_, iso_max_);
    add_range(M("EXIFFILTER_APERTURE"), fnumber_min_, fnumber_max_);
    add_range(M("EXIFFILTER_FOCALLEN"), focallen_min_, focallen_max_);
    add_range(M("EXIFFILTER_SHUTTER"), shutterspeed_min_, shutterspeed_max_);
    add_range(M("EXIFFILTER_EXPOSURECOMPENSATION"), expcomp_min_, expcomp_max_);

    has_customdata_ =
        Gtk::manage(new Gtk::CheckButton(M("DYNPROFILEEDITOR_CUSTOMDATA")));
    hb = Gtk::manage(new Gtk::HBox());
    hb->pack_start(*has_customdata_, Gtk::PACK_SHRINK, 4);

    customdata_ = Gtk::TextBuffer::create();
    customdata_view_ = Gtk::manage(new Gtk::TextView(customdata_));
    setExpandAlignProperties(has_customdata_, false, false, Gtk::ALIGN_FILL,
                             Gtk::ALIGN_START);
    setExpandAlignProperties(customdata_view_, true, true, Gtk::ALIGN_FILL,
                             Gtk::ALIGN_FILL);
    customdata_view_->set_border_width(1);
    Gtk::ScrolledWindow *sw = Gtk::manage(new Gtk::ScrolledWindow());
    setExpandAlignProperties(sw, true, false, Gtk::ALIGN_FILL,
                             Gtk::ALIGN_START);
    sw->set_min_content_height(100);
    sw->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_ALWAYS);
    sw->add(*customdata_view_);
    hb->pack_start(*sw, true, true, 2);
    get_content_area()->pack_start(*hb, Gtk::PACK_SHRINK, 4);
    customdata_view_->set_tooltip_markup(
        M("DYNPROFILEEDITOR_CUSTOMDATA_TOOLTIP"));

    add_button(M("GENERAL_OK"), 1);
    add_button(M("GENERAL_CANCEL"), 2);

    set_ranges();

    show_all_children();
}

void DynamicProfilePanel::EditDialog::set_rule(const DynamicProfileRule &rule)
{
    iso_min_->set_value(rule.iso.min);
    iso_max_->set_value(rule.iso.max);

    fnumber_min_->set_value(rule.fnumber.min);
    fnumber_max_->set_value(rule.fnumber.max);

    focallen_min_->set_value(rule.focallen.min);
    focallen_max_->set_value(rule.focallen.max);

    shutterspeed_min_->set_value(rule.shutterspeed.min);
    shutterspeed_max_->set_value(rule.shutterspeed.max);

    expcomp_min_->set_value(rule.expcomp.min);
    expcomp_max_->set_value(rule.expcomp.max);

    has_camera_->set_active(rule.camera.enabled);
    camera_->set_text(rule.camera.value);

    has_lens_->set_active(rule.lens.enabled);
    lens_->set_text(rule.lens.value);

    has_software_->set_active(rule.software.enabled);
    software_->set_text(rule.software.value);

    has_filetype_->set_active(rule.filetype.enabled);
    filetype_->set_text(rule.filetype.value);

    if (!rule.imagetype.enabled) {
        imagetype_->set_active(0);
    } else if (rule.imagetype.value == "raw") {
        imagetype_->set_active(1);
    } else if (rule.imagetype.value == "nonraw") {
        imagetype_->set_active(2);
    } else {
        imagetype_->set_active(0);
    }

    has_customdata_->set_active(rule.customdata.enabled);
    std::ostringstream buf;
    for (auto &p : rule.customdata.value) {
        buf << p.first << "=" << p.second << "\n";
    }
    customdata_->set_text(buf.str());

    profilepath_->updateProfileList();

    if (!profilepath_->setActiveRowFromFullPath(rule.profilepath)) {
        profilepath_->setInternalEntry();
    }
}

DynamicProfileRule DynamicProfilePanel::EditDialog::get_rule()
{
    DynamicProfileRule ret;
    ret.iso.min = iso_min_->get_value_as_int();
    ret.iso.max = iso_max_->get_value_as_int();

    ret.fnumber.min = fnumber_min_->get_value();
    ret.fnumber.max = fnumber_max_->get_value();

    ret.focallen.min = focallen_min_->get_value();
    ret.focallen.max = focallen_max_->get_value();

    ret.shutterspeed.min = shutterspeed_min_->get_value();
    ret.shutterspeed.max = shutterspeed_max_->get_value();

    ret.expcomp.min = expcomp_min_->get_value();
    ret.expcomp.max = expcomp_max_->get_value();

    ret.camera.enabled = has_camera_->get_active();
    ret.camera.value = camera_->get_text();

    ret.lens.enabled = has_lens_->get_active();
    ret.lens.value = lens_->get_text();

    ret.software.enabled = has_software_->get_active();
    ret.software.value = software_->get_text();

    ret.filetype.enabled = has_filetype_->get_active();
    ret.filetype.value = filetype_->get_text();

    ret.imagetype.enabled = imagetype_->get_active_row_number() > 0;
    switch (imagetype_->get_active_row_number()) {
    case 1:
        ret.imagetype.value = "raw";
        break;
    case 2:
        ret.imagetype.value = "nonraw";
        break;
    default:
        ret.imagetype.value = "";
    }

    ret.customdata.enabled = has_customdata_->get_active();
    std::istringstream buf(customdata_->get_text());
    std::string line;
    ret.customdata.value.clear();
    while (std::getline(buf, line)) {
        size_t i = 0;
        while (i < line.size() && std::isspace(line[i])) {
            ++i;
        }
        if (i < line.size()) {
            line = line.substr(i);
            auto pos = line.find("=");
            if (pos != std::string::npos) {
                ret.customdata.value.emplace_back(line.substr(0, pos),
                                                  line.substr(pos + 1));
            } else {
                ret.customdata.value.emplace_back(line, "");
            }
        }
    }

    ret.profilepath = profilepath_->getFullPathFromActiveRow();

    return ret;
}

void DynamicProfilePanel::EditDialog::set_ranges()
{
    DynamicProfileRule default_rule;
    iso_min_->set_digits(0);
    iso_max_->set_digits(0);
    iso_min_->set_increments(1, 10);
    iso_max_->set_increments(1, 10);
    iso_min_->set_range(default_rule.iso.min, default_rule.iso.max);
    iso_max_->set_range(default_rule.iso.min, default_rule.iso.max);
    iso_min_->set_value(default_rule.iso.min);
    iso_max_->set_value(default_rule.iso.max);

#define DOIT_(name)                                                            \
    name##_min_->set_digits(1);                                                \
    name##_max_->set_digits(1);                                                \
    name##_min_->set_increments(0.1, 1);                                       \
    name##_max_->set_increments(0.1, 1);                                       \
    name##_min_->set_range(default_rule.name.min, default_rule.name.max);      \
    name##_max_->set_range(default_rule.name.min, default_rule.name.max);      \
    name##_min_->set_value(default_rule.name.min);                             \
    name##_max_->set_value(default_rule.name.max)

    DOIT_(fnumber);
    DOIT_(focallen);
    DOIT_(shutterspeed);
    DOIT_(expcomp);
#undef DOIT_
    shutterspeed_min_->set_digits(4);
    shutterspeed_max_->set_digits(4);

    profilepath_->setInternalEntry();
}

void DynamicProfilePanel::EditDialog::add_range(const Glib::ustring &name,
                                                Gtk::SpinButton *&from,
                                                Gtk::SpinButton *&to)
{
    Gtk::HBox *hb = Gtk::manage(new Gtk::HBox());
    hb->pack_start(*Gtk::manage(new Gtk::Label(name)), false, false, 4);
    from = Gtk::manage(new Gtk::SpinButton());
    to = Gtk::manage(new Gtk::SpinButton());
    from->set_numeric(true);
    to->set_numeric(true);
    hb->pack_start(*from, true, true, 2);
    hb->pack_start(*Gtk::manage(new Gtk::Label(" - ")), false, false, 4);
    hb->pack_start(*to, true, true, 2);
    get_content_area()->pack_start(*hb, Gtk::PACK_SHRINK, 4);
}

void DynamicProfilePanel::EditDialog::add_optional(const Glib::ustring &name,
                                                   Gtk::CheckButton *&check,
                                                   Gtk::Entry *&field)
{
    check = Gtk::manage(new Gtk::CheckButton(name));
    Gtk::HBox *hb = Gtk::manage(new Gtk::HBox());
    hb->pack_start(*check, Gtk::PACK_SHRINK, 4);
    field = Gtk::manage(new Gtk::Entry());
    hb->pack_start(*field, true, true, 2);
    get_content_area()->pack_start(*hb, Gtk::PACK_SHRINK, 4);
    field->set_tooltip_text(M("DYNPROFILEEDITOR_ENTRY_TOOLTIP"));
}

//-----------------------------------------------------------------------------
// DynamicProfilePanel
//-----------------------------------------------------------------------------

DynamicProfilePanel::DynamicProfilePanel()
    : vbox_(Gtk::ORIENTATION_VERTICAL),
      button_up_(M("DYNPROFILEEDITOR_MOVE_UP")),
      button_down_(M("DYNPROFILEEDITOR_MOVE_DOWN")),
      button_new_(M("DYNPROFILEEDITOR_NEW")),
      button_edit_(M("DYNPROFILEEDITOR_EDIT")),
      button_delete_(M("DYNPROFILEEDITOR_DELETE")),
      button_reset_(M("FILEBROWSER_RESETDEFAULTPROFILE"))
{
    add(vbox_);

    treeview_.set_grid_lines(Gtk::TREE_VIEW_GRID_LINES_VERTICAL);
    scrolledwindow_.add(treeview_);

    scrolledwindow_.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    vbox_.pack_start(scrolledwindow_);
    vbox_.pack_start(buttonbox_, Gtk::PACK_SHRINK);

    buttonbox_.pack_start(button_new_, Gtk::PACK_SHRINK);
    buttonbox_.pack_start(button_edit_, Gtk::PACK_SHRINK);
    buttonbox_.pack_start(button_delete_, Gtk::PACK_SHRINK);
    buttonbox_.pack_start(button_up_, Gtk::PACK_SHRINK);
    buttonbox_.pack_start(button_down_, Gtk::PACK_SHRINK);
    buttonbox_.pack_start(button_reset_, Gtk::PACK_SHRINK);
    buttonbox_.set_border_width(5);
    buttonbox_.set_layout(Gtk::BUTTONBOX_END);
    button_up_.signal_clicked().connect(
        sigc::mem_fun(*this, &DynamicProfilePanel::on_button_up));
    button_down_.signal_clicked().connect(
        sigc::mem_fun(*this, &DynamicProfilePanel::on_button_down));
    button_new_.signal_clicked().connect(
        sigc::mem_fun(*this, &DynamicProfilePanel::on_button_new));
    button_edit_.signal_clicked().connect(
        sigc::mem_fun(*this, &DynamicProfilePanel::on_button_edit));
    button_delete_.signal_clicked().connect(
        sigc::mem_fun(*this, &DynamicProfilePanel::on_button_delete));
    button_reset_.signal_clicked().connect(
        sigc::mem_fun(*this, &DynamicProfilePanel::on_button_reset));

    treemodel_ = Gtk::ListStore::create(columns_);
    treeview_.set_model(treemodel_);

#define ADD_COL(which, lbl)                                                    \
    do {                                                                       \
        auto cell = Gtk::manage(new Gtk::CellRendererText());                  \
        int cols_count = treeview_.append_column(M(lbl), *cell);               \
        auto col = treeview_.get_column(cols_count - 1);                       \
                                                                               \
        if (col) {                                                             \
            col->set_cell_data_func(                                           \
                *cell,                                                         \
                sigc::mem_fun(*this, &DynamicProfilePanel::render_##which));   \
        }                                                                      \
    } while (false)

    ADD_COL(profilepath, "DYNPROFILEEDITOR_PROFILE");
    ADD_COL(camera, "EXIFFILTER_CAMERA");
    ADD_COL(lens, "EXIFFILTER_LENS");
    ADD_COL(iso, "EXIFFILTER_ISO");
    ADD_COL(fnumber, "EXIFFILTER_APERTURE");
    ADD_COL(focallen, "EXIFFILTER_FOCALLEN");
    ADD_COL(shutterspeed, "EXIFFILTER_SHUTTER");
    ADD_COL(expcomp, "EXIFFILTER_EXPOSURECOMPENSATION");
    ADD_COL(filetype, "EXIFFILTER_FILETYPE");
    ADD_COL(software, "EXIFFILTER_SOFTWARE");
    ADD_COL(imagetype, "EXIFFILTER_IMAGETYPE");
    ADD_COL(customdata, "DYNPROFILEEDITOR_CUSTOMDATA");

    show_all_children();
    reset();
}

void DynamicProfilePanel::reset()
{
    treemodel_->clear();
    for (auto &r : ProfileStore::getInstance()->getRules()) {
        add_rule(r);
    }
}

void DynamicProfilePanel::update_rule(Gtk::TreeModel::Row row,
                                      const DynamicProfileRule &rule)
{
    row[columns_.iso] = rule.iso;
    row[columns_.fnumber] = rule.fnumber;
    row[columns_.focallen] = rule.focallen;
    row[columns_.shutterspeed] = rule.shutterspeed;
    row[columns_.expcomp] = rule.expcomp;
    row[columns_.camera] = rule.camera;
    row[columns_.lens] = rule.lens;
    row[columns_.imagetype] = rule.imagetype;
    row[columns_.profilepath] = rule.profilepath;
    row[columns_.software] = rule.software;
    row[columns_.filetype] = rule.filetype;
    row[columns_.customdata] = rule.customdata;
}

void DynamicProfilePanel::add_rule(const DynamicProfileRule &rule)
{
    auto row = *(treemodel_->append());
    update_rule(row, rule);
}

DynamicProfileRule DynamicProfilePanel::to_rule(Gtk::TreeModel::Row row,
                                                int serial)
{
    DynamicProfileRule ret;
    ret.serial_number = serial;
    ret.iso = row[columns_.iso];
    ret.fnumber = row[columns_.fnumber];
    ret.focallen = row[columns_.focallen];
    ret.shutterspeed = row[columns_.shutterspeed];
    ret.expcomp = row[columns_.expcomp];
    ret.camera = row[columns_.camera];
    ret.lens = row[columns_.lens];
    ret.profilepath = row[columns_.profilepath];
    ret.imagetype = row[columns_.imagetype];
    ret.software = row[columns_.software];
    ret.filetype = row[columns_.filetype];
    ret.customdata = row[columns_.customdata];
    return ret;
}

void DynamicProfilePanel::render_profilepath(
    Gtk::CellRenderer *cell, const Gtk::TreeModel::iterator &iter)
{
    auto row = *iter;
    Gtk::CellRendererText *ct = static_cast<Gtk::CellRendererText *>(cell);
    auto value = row[columns_.profilepath];
    auto pse = ProfileStore::getInstance()->findEntryFromFullPath(value);

    if (pse != nullptr) {
        ct->property_text() = pse->label;
    } else {
        ct->property_text() = value;
    }
}

#define RENDER_RANGE_(tp, name, tostr)                                         \
    auto row = *iter;                                                          \
    Gtk::CellRendererText *ct = static_cast<Gtk::CellRendererText *>(cell);    \
    DynamicProfileRule::Range<tp> r = row[columns_.name];                      \
    DynamicProfileRule dflt;                                                   \
    if (r.min > dflt.name.min || r.max < dflt.name.max) {                      \
        auto value = tostr(r.min) + " - " + tostr(r.max);                      \
        ct->property_text() = value;                                           \
    } else {                                                                   \
        ct->property_text() = "";                                              \
    }

namespace {

template <class V> Glib::ustring to_str(V n, int precision = 1)
{
    std::ostringstream buf;
    buf << std::setprecision(precision) << std::fixed << n;
    return buf.str();
}

} // namespace

void DynamicProfilePanel::render_iso(Gtk::CellRenderer *cell,
                                     const Gtk::TreeModel::iterator &iter)
{
    RENDER_RANGE_(int, iso, to_str);
}

void DynamicProfilePanel::render_fnumber(Gtk::CellRenderer *cell,
                                         const Gtk::TreeModel::iterator &iter)
{
    RENDER_RANGE_(double, fnumber, [](double f) {
        return std::string("f/") +
               rtengine::FramesMetaData::apertureToString(f);
    });
}

void DynamicProfilePanel::render_focallen(Gtk::CellRenderer *cell,
                                          const Gtk::TreeModel::iterator &iter)
{
    RENDER_RANGE_(double, focallen, to_str);
}

void DynamicProfilePanel::render_shutterspeed(
    Gtk::CellRenderer *cell, const Gtk::TreeModel::iterator &iter)
{
    RENDER_RANGE_(double, shutterspeed,
                  rtengine::FramesMetaData::shutterToString);
}

void DynamicProfilePanel::render_expcomp(Gtk::CellRenderer *cell,
                                         const Gtk::TreeModel::iterator &iter)
{
    RENDER_RANGE_(double, expcomp, to_str);
}

#undef RENDER_RANGE_

#define RENDER_OPTIONAL_(name)                                                 \
    auto row = *iter;                                                          \
    Gtk::CellRendererText *ct = static_cast<Gtk::CellRendererText *>(cell);    \
    DynamicProfileRule::Optional o = row[columns_.name];                       \
    if (o.enabled) {                                                           \
        ct->property_text() = o.value;                                         \
    } else {                                                                   \
        ct->property_text() = "";                                              \
    }

void DynamicProfilePanel::render_camera(Gtk::CellRenderer *cell,
                                        const Gtk::TreeModel::iterator &iter)
{
    RENDER_OPTIONAL_(camera);
}

void DynamicProfilePanel::render_lens(Gtk::CellRenderer *cell,
                                      const Gtk::TreeModel::iterator &iter)
{
    RENDER_OPTIONAL_(lens);
}

void DynamicProfilePanel::render_software(Gtk::CellRenderer *cell,
                                          const Gtk::TreeModel::iterator &iter)
{
    RENDER_OPTIONAL_(software);
}

void DynamicProfilePanel::render_filetype(Gtk::CellRenderer *cell,
                                          const Gtk::TreeModel::iterator &iter)
{
    RENDER_OPTIONAL_(filetype);
}

void DynamicProfilePanel::render_imagetype(Gtk::CellRenderer *cell,
                                           const Gtk::TreeModel::iterator &iter)
{
    auto row = *iter;
    Gtk::CellRendererText *ct = static_cast<Gtk::CellRendererText *>(cell);
    DynamicProfileRule::Optional o = row[columns_.imagetype];
    if (o.enabled) {
        ct->property_text() =
            M(std::string("DYNPROFILEEDITOR_IMGTYPE_") + o.value.uppercase());
    } else {
        ct->property_text() = "";
    }
}

void DynamicProfilePanel::render_customdata(
    Gtk::CellRenderer *cell, const Gtk::TreeModel::iterator &iter)
{
    auto row = *iter;
    Gtk::CellRendererText *ct = static_cast<Gtk::CellRendererText *>(cell);
    DynamicProfileRule::CustomMetadata m = row[columns_.customdata];
    if (m.enabled) {
        ct->property_text() = std::to_string(m.value.size()) + " " +
                              M("DYNPROFILEEDITOR_CUSTOMDATA_TAGS");
    } else {
        ct->property_text() = "";
    }
}

#undef RENDER_OPTIONAL_

void DynamicProfilePanel::on_button_up()
{
    auto s = treeview_.get_selection();

    if (!s->count_selected_rows()) {
        return;
    }

    auto it = s->get_selected();

    if (it != treemodel_->children().begin()) {
        auto it2 = it;
        --it2;
        treemodel_->iter_swap(it, it2);
    }
}

void DynamicProfilePanel::on_button_down()
{
    auto s = treeview_.get_selection();

    if (!s->count_selected_rows()) {
        return;
    }

    auto it = s->get_selected();
    auto it2 = it;
    ++it2;

    if (it2 != treemodel_->children().end()) {
        treemodel_->iter_swap(it, it2);
    }
}

void DynamicProfilePanel::on_button_delete()
{
    auto s = treeview_.get_selection();

    if (!s->count_selected_rows()) {
        return;
    }

    auto it = s->get_selected();
    treemodel_->erase(it);
}

void DynamicProfilePanel::on_button_new()
{
    EditDialog d(M("DYNPROFILEEDITOR_NEW_RULE"),
                 static_cast<Gtk::Window &>(*get_toplevel()));
    int status = d.run();

    if (status == 1) {
        DynamicProfileRule rule = d.get_rule();
        add_rule(rule);
    }
}

void DynamicProfilePanel::on_button_edit()
{
    auto s = treeview_.get_selection();

    if (!s->count_selected_rows()) {
        return;
    }

    EditDialog d(M("DYNPROFILEEDITOR_EDIT_RULE"),
                 static_cast<Gtk::Window &>(*get_toplevel()));
    Gtk::TreeModel::Row row = *(s->get_selected());
    d.set_rule(to_rule(row));
    int status = d.run();

    if (status == 1) {
        update_rule(row, d.get_rule());
    }
}

void DynamicProfilePanel::on_button_reset()
{
    auto ps = ProfileStore::getInstance();
    auto rules = ps->getRules();
    ps->loadRules(true);
    reset();
    ps->setRules(rules);
}

void DynamicProfilePanel::save()
{
    std::vector<DynamicProfileRule> rules;
    int serial = 1;

    for (auto row : treemodel_->children()) {
        rules.emplace_back(to_rule(row, serial++));
    }

    ProfileStore::getInstance()->setRules(rules);

    if (!ProfileStore::getInstance()->storeRules()) {
        printf("Error in saving dynamic profile rules\n");
    } else if (options.rtSettings.verbose > 1) {
        printf("Saved %d dynamic profile rules\n", int(rules.size()));
    }
}
