/** -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright (c) 2004-2010 Gabor Horvath <hgabor@rawtherapee.com>
 *  Copyright (c) 2017-2026 Alberto Griggio <alberto.griggio@gmail.com>
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

#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <vector>

#include "filmsimulation.h"

#include <chrono>

#include "../engine/clutstore.h"
#include "eventmapper.h"
#include "options.h"

namespace art { namespace gui {
using namespace utils;


using namespace art::engine;
using namespace art::engine::procparams;

namespace {

Glib::ustring stripPrefixDir(const Glib::ustring &filename,
                             const Glib::ustring &dir)
{
    const Glib::ustring full_dir = !Glib::str_has_suffix(dir, G_DIR_SEPARATOR_S)
                                       ? dir + G_DIR_SEPARATOR_S
                                       : dir;
    return Glib::str_has_prefix(filename, full_dir)
               ? filename.substr(full_dir.size())
               : filename;
}

bool notifySlowParseDir(const std::chrono::system_clock::time_point &startedAt)
{
    enum Decision { UNDECIDED, CANCEL, CONTINUE };

    static Decision decision = UNDECIDED;

    if (decision == CANCEL) {
        return false;
    } else if (decision == CONTINUE) {
        return true;
    }

    const auto now = std::chrono::system_clock::now();
    if (now - startedAt < std::chrono::seconds(10)) {
        return true;
    }

    Gtk::MessageDialog dialog(M("TP_FILMSIMULATION_SLOWPARSEDIR"), false,
                              Gtk::MESSAGE_WARNING, Gtk::BUTTONS_YES_NO, true);
    if (dialog.run() == Gtk::RESPONSE_YES) {
        decision = CANCEL;
        return false;
    } else {
        decision = CONTINUE;
        return true;
    }
}

} // namespace

FilmSimulation::FilmSimulation()
    : FoldableToolPanel(this, "filmsimulation", M("TP_FILMSIMULATION_LABEL"),
                        false, true, true)
{
    EvToolEnabled.set_action(art::engine::RGBCURVE);
    EvAfterToneCurve = ProcEventMapper::getInstance()->newEvent(
        art::engine::RGBCURVE, "HISTORY_MSG_FILMSIMULATION_AFTER_TONE_CURVE");
    EvClutParams = ProcEventMapper::getInstance()->newEvent(
        art::engine::RGBCURVE, "HISTORY_MSG_FILMSIMULATION_CLUT_PARAMS");

    m_clutComboBox = Gtk::manage(
        new ClutComboBox({Glib::build_filename(options.ART_base_dir, "luts"),
                          options.clutsDir}));
    int foundClutsCount = m_clutComboBox->foundClutsCount();

    if (foundClutsCount == 0) {
        pack_start(*Gtk::manage(
            new Gtk::Label(M("TP_FILMSIMULATION_ZEROCLUTSFOUND"))));
    }

    m_clutComboBoxConn = m_clutComboBox->signal_changed().connect(
        sigc::mem_fun(*this, &FilmSimulation::onClutSelected));
    pack_start(*m_clutComboBox);

    lut_params_ = Gtk::manage(new CLUTParamsPanel());
    lut_params_->setEditIDBase(art::utils::ctlEditBase(false));
    lut_params_->signal_changed().connect(
        sigc::mem_fun(this, &FilmSimulation::onClutParamsChanged));
    pack_start(*lut_params_);

    pack_start(*Gtk::manage(new Gtk::HSeparator()));
    m_strength = Gtk::manage(
        new Adjuster(M("TP_FILMSIMULATION_STRENGTH"), 0., 100, 1., 100));
    m_strength->setAdjusterListener(this);

    pack_start(*m_strength, Gtk::PACK_SHRINK, 0);

    after_tone_curve_ = Gtk::manage(
        new Gtk::CheckButton(M("TP_FILMSIMULATION_AFTER_TONE_CURVE")));
    Gtk::HBox *hb = Gtk::manage(new Gtk::HBox());
    after_tone_curve_box_ = hb;
    Gtk::Image *w = Gtk::manage(new RTImage("warning-small.svg"));
    w->set_tooltip_markup(M("GENERAL_DEPRECATED_TOOLTIP"));
    hb->pack_start(*after_tone_curve_, Gtk::PACK_EXPAND_WIDGET, 4);
    hb->pack_start(*w, Gtk::PACK_SHRINK);
    setExpandAlignProperties(after_tone_curve_, true, false, Gtk::ALIGN_START,
                             Gtk::ALIGN_BASELINE);
    pack_start(*hb, Gtk::PACK_SHRINK, 0);
    after_tone_curve_->signal_toggled().connect(
        sigc::mem_fun(this, &FilmSimulation::afterToneCurveToggled));
}

void FilmSimulation::setEditProvider(EditDataProvider *provider)
{
    lut_params_->setEditProvider(provider);
}

void FilmSimulation::onClutSelected()
{
    auto info = m_clutComboBox->getSelectedClut();
    lut_params_->setParams(
        art::engine::CLUTApplication::get_param_descriptors(info.first));
    lut_params_->setValue({});

    if (listener && getEnabled()) {
        listener->panelChanged(EvFilmSimulationFilename, info.second);
    }
}

void FilmSimulation::onClutParamsChanged()
{
    if (listener && getEnabled()) {
        listener->panelChanged(EvClutParams, M("GENERAL_CHANGED"));
    }
}

void FilmSimulation::enabledChanged()
{

    if (listener) {
        if (get_inconsistent()) {
            listener->panelChanged(EvFilmSimulationEnabled,
                                   M("GENERAL_UNCHANGED"));
        } else if (getEnabled()) {
            listener->panelChanged(EvFilmSimulationEnabled,
                                   M("GENERAL_ENABLED"));
        } else {
            listener->panelChanged(EvFilmSimulationEnabled,
                                   M("GENERAL_DISABLED"));
        }
    }
}

void FilmSimulation::adjusterChanged(Adjuster *a, double newval)
{
    if (listener && getEnabled()) {
        const Glib::ustring value = a->getTextValue();
        listener->panelChanged(EvFilmSimulationStrength, value);
    }
}

void FilmSimulation::adjusterAutoToggled(Adjuster *a, bool newval) {}

void FilmSimulation::read(const art::engine::procparams::ProcParams *pp)
{
    // copypasted from lensprofile.cc & sharpening.cc
    disableListener();
    updateDisable(true);

    setEnabled(pp->filmSimulation.enabled);

    Glib::ustring fname;
    if (!pp->filmSimulation.clutFilename.empty()) {
        fname = !Glib::path_is_absolute(pp->filmSimulation.clutFilename)
                    ? Glib::ustring(Glib::build_filename(
                          options.clutsDir, pp->filmSimulation.clutFilename))
                    : pp->filmSimulation.clutFilename;
        m_clutComboBox->setSelectedClut(fname);
    } else {
        m_clutComboBox->set_active(-1);
    }

    m_strength->setValue(pp->filmSimulation.strength);

    if (!get_inconsistent() && !pp->filmSimulation.enabled) {
        if (options.clutCacheSize == 1) {
            CLUTStore::getInstance().clearCache();
        }
    }

    after_tone_curve_->set_active(pp->filmSimulation.after_tone_curve);
    after_tone_curve_box_->set_visible(after_tone_curve_->get_active());

    lut_params_->setParams(
        art::engine::CLUTApplication::get_param_descriptors(fname));
    lut_params_->setValue(pp->filmSimulation.lut_params);

    updateDisable(false);
    enableListener();
}

void FilmSimulation::updateDisable(bool value)
{
    m_clutComboBoxConn.block(value);
}

void FilmSimulation::write(art::engine::procparams::ProcParams *pp)
{
    pp->filmSimulation.enabled = getEnabled();
    const Glib::ustring clutFName = m_clutComboBox->getSelectedClut().first;

    pp->filmSimulation.clutFilename =
        stripPrefixDir(clutFName, options.clutsDir);

    pp->filmSimulation.strength = m_strength->getValue();
    pp->filmSimulation.after_tone_curve = after_tone_curve_->get_active();
    pp->filmSimulation.lut_params = lut_params_->getValue();
}

void FilmSimulation::trimValues(art::engine::procparams::ProcParams *pp)
{
    m_strength->trimValue(pp->filmSimulation.strength);
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

std::unique_ptr<ClutComboBox::ClutModel> ClutComboBox::cm;

ClutComboBox::ClutComboBox(const std::vector<Glib::ustring> &paths)
    : SearchableTreeCombo()
{
    if (!cm) {
        cm.reset(new ClutModel(paths));
    }

    setModel(m_model(), m_columns().label);
}

inline Glib::RefPtr<Gtk::TreeStore> &ClutComboBox::m_model()
{
    return cm->m_model;
}

inline ClutComboBox::ClutColumns &ClutComboBox::m_columns()
{
    return cm->m_columns;
}

void ClutComboBox::cleanup()
{
    cm.reset();
}

ClutComboBox::ClutColumns::ClutColumns()
{
    add(label);
    add(clutFilename);
}

ClutComboBox::ClutModel::ClutModel(const std::vector<Glib::ustring> &paths)
{
    m_model = Gtk::TreeStore::create(m_columns);
    // set_model (m_model);
    count = parseDir(paths);
}

int ClutComboBox::ClutModel::parseDir(const std::vector<Glib::ustring> &paths)
{
    if (paths.empty()) { // || !Glib::file_test(path, Glib::FILE_TEST_IS_DIR)) {
        return 0;
    }

    const auto sorted_dir_dirs =
        [](const Glib::ustring &path) -> std::map<std::string, std::string> {
        std::map<std::string, std::string> res;

        for (const auto &dir : Glib::Dir(path)) {
            const std::string full_path = Glib::build_filename(path, dir);

            if (Glib::file_test(full_path, Glib::FILE_TEST_IS_DIR)) {
                res.emplace(dir, full_path);
            }
        }

        return res;
    };

    const auto startedAt = std::chrono::system_clock::now();
    unsigned long fileCount = 0;
    bool aborted = false;

    // append a CLUT file under the given row (empty = top level), if it is
    // of a supported kind
    const auto addFile = [&](const Glib::ustring &entry,
                             const Gtk::TreeModel::Row &row) {
        auto extension = getExtension(entry).casefold();
        if (extension.compare("tif") != 0 && extension.compare("png") != 0) {
#ifdef ART_USE_OCIO
            if (extension != "clf" && extension != "clfz" &&
                extension != "json")
#endif // ART_USE_OCIO
#ifdef ART_USE_CTL
                if (extension != "ctl")
#endif // ART_USE_CTL
                    return;
        }

        auto label = CLUTStore::getClutDisplayName(entry);
        if (!label.ok()) {
            return;
        }

        auto newRow =
            row ? *m_model->append(row.children()) : *m_model->append();
        newRow[m_columns.label] = label;
        newRow[m_columns.clutFilename] = entry;

        ++fileCount;

        if (!notifySlowParseDir(startedAt)) {
            aborted = true;
        }
    };

    const auto sorted_dir_files =
        [](const Glib::ustring &path) -> std::vector<Glib::ustring> {
        std::set<Glib::ustring> sorted;
        for (const auto &entry : Glib::Dir(path)) {
            const auto entryPath = Glib::build_filename(path, entry);
            if (Glib::file_test(entryPath, Glib::FILE_TEST_IS_REGULAR)) {
                sorted.insert(entryPath);
            }
        }
        return std::vector<Glib::ustring>(sorted.begin(), sorted.end());
    };

    // optional ARTlist.txt in a top-level dir: one entry per line (a file or a
    // subdirectory, relative to the top-level dir), '#' starts a comment
    const auto read_list =
        [](const Glib::ustring &path) -> std::vector<std::string> {
        std::vector<std::string> res;
        const std::string list_name = findARTListFile(path);
        if (list_name.empty()) {
            return res;
        }
        std::ifstream src(list_name);
        std::string line;
        while (std::getline(src, line)) {
            auto pos = line.find('#');
            if (pos != std::string::npos) {
                line.erase(pos);
            }
            const char *ws = " \t\r\n";
            auto b = line.find_first_not_of(ws);
            if (b == std::string::npos) {
                continue;
            }
            line = line.substr(b, line.find_last_not_of(ws) - b + 1);
            while (line.size() > 1 && line.back() == '/') {
                line.pop_back();
            }
            res.push_back(line);
        }
        return res;
    };

    std::vector<Glib::ustring> plain_paths;
    std::vector<Glib::ustring> listed_paths;
    for (auto &path : paths) {
        if (Glib::file_test(path, Glib::FILE_TEST_IS_DIR)) {
            if (!findARTListFile(path).empty()) {
                listed_paths.push_back(path);
            } else {
                plain_paths.push_back(path);
            }
        }
    }

    // Build menu of limited directory structure using breadth-first search
    using Dirs = std::vector<std::pair<Glib::ustring, Gtk::TreeModel::Row>>;
    Dirs dirs;

    {
        Dirs currDirs;
        Dirs nextDirs;

        for (auto &path : plain_paths) {
            currDirs.emplace_back(path, Gtk::TreeModel::Row());
        }

        while (!currDirs.empty()) {
            for (auto &dir : currDirs) {
                const auto &path = dir.first;
                const auto &row = dir.second;

                try {
                    for (const auto &entry : sorted_dir_dirs(path)) {
                        auto newRow = row ? *m_model->append(row.children())
                                          : *m_model->append();
                        newRow[m_columns.label] = entry.first;

                        nextDirs.emplace_back(entry.second, newRow);
                    }
                } catch (Glib::Exception &) {
                }

                dirs.push_back(std::move(dir));

                if (!notifySlowParseDir(startedAt)) {
                    m_model->clear();
                    return 0;
                }
            }

            currDirs.clear();
            currDirs.swap(nextDirs);
        }
    }

    // Fill menu structure with CLUT files
    for (const auto &dir : dirs) {
        try {
            for (const auto &entry : sorted_dir_files(dir.first)) {
                addFile(entry, dir.second);
                if (aborted) {
                    m_model->clear();
                    return 0;
                }
            }
        } catch (Glib::Exception &) {
        }
    }

    // Top-level dirs with an ARTlist.txt: only the listed files and
    // subdirectories are shown, in the listed order. A subdirectory is a
    // group placed where it first appears in the list; ARTlist.txt files in
    // subdirectories are ignored
    for (const auto &top : listed_paths) {
        std::map<std::string, Gtk::TreeModel::Row> groups;
        std::set<Glib::ustring> seen;

        const auto ensure_group =
            [&](const std::string &rel) -> Gtk::TreeModel::Row {
            Gtk::TreeModel::Row row;
            std::string cur;
            size_t start = 0;
            while (start <= rel.size() && !rel.empty()) {
                auto end = rel.find('/', start);
                if (end == std::string::npos) {
                    end = rel.size();
                }
                auto part = rel.substr(start, end - start);
                start = end + 1;
                if (part.empty() || part == ".") {
                    continue;
                }
                cur = cur.empty() ? part : cur + "/" + part;
                auto it = groups.find(cur);
                if (it == groups.end()) {
                    auto newRow = row ? *m_model->append(row.children())
                                      : *m_model->append();
                    newRow[m_columns.label] = part;
                    it = groups.emplace(cur, newRow).first;
                }
                row = it->second;
            }
            return row;
        };

        for (const auto &line : read_list(top)) {
            // entries outside the top-level dir go in at the top level
            const bool outside =
                Glib::path_is_absolute(line) || line == ".." ||
                line.compare(0, 3, "../") == 0 ||
                line.find("/../") != std::string::npos;
            const std::string full =
                outside && Glib::path_is_absolute(line)
                    ? line
                    : Glib::build_filename(top, line);

            try {
                if (Glib::file_test(full, Glib::FILE_TEST_IS_DIR)) {
                    if (outside) {
                        continue;
                    }
                    auto row = ensure_group(line);
                    for (const auto &entry : sorted_dir_files(full)) {
                        if (seen.insert(entry).second) {
                            addFile(entry, row);
                            if (aborted) {
                                m_model->clear();
                                return 0;
                            }
                        }
                    }
                } else if (Glib::file_test(full, Glib::FILE_TEST_IS_REGULAR) &&
                           seen.insert(full).second) {
                    Gtk::TreeModel::Row row;
                    if (!outside) {
                        auto pos = line.rfind('/');
                        if (pos != std::string::npos) {
                            row = ensure_group(line.substr(0, pos));
                        }
                    }
                    addFile(full, row);
                    if (aborted) {
                        m_model->clear();
                        return 0;
                    }
                } else if (options.rtSettings.verbose) {
                    std::cout << "Entry in " << top << "/ARTlist.txt not found: "
                              << line << std::endl;
                }
            } catch (Glib::Exception &) {
            }
        }
    }

    return fileCount;
}

int ClutComboBox::foundClutsCount() const { return cm->count; }

std::pair<Glib::ustring, Glib::ustring> ClutComboBox::getSelectedClut()
{
    std::pair<Glib::ustring, Glib::ustring> result;
    Gtk::TreeModel::iterator current = get_active();
    Gtk::TreeModel::Row row = *current;

    if (row) {
        result = std::make_pair(row[m_columns().clutFilename],
                                row[m_columns().label]);
    }

    return result;
}

void ClutComboBox::setSelectedClut(Glib::ustring filename)
{
    if (!filename.empty()) {
        Gtk::TreeIter found =
            findRowByClutFilename(m_model()->children(), filename);

        if (found) {
            set_active(found);
        } else {
            set_active(-1);
        }
    }
}

Gtk::TreeIter
ClutComboBox::findRowByClutFilename(Gtk::TreeModel::Children childs,
                                    Glib::ustring filename)
{
    Gtk::TreeIter result = childs.end();

    for (Gtk::TreeModel::Children::iterator it = childs.begin();
         !result && it != childs.end(); ++it) {
        Gtk::TreeModel::Row row = *it;

        if (row[m_columns().clutFilename] == filename) {
            result = it;
        } else {
            result = findRowByClutFilename(it->children(), filename);
        }
    }

    return result;
}

void FilmSimulation::setDefaults(const ProcParams *defParams)
{
    initial_params = defParams->filmSimulation;
}

void FilmSimulation::toolReset(bool to_initial)
{
    ProcParams pp;
    if (to_initial) {
        pp.filmSimulation = initial_params;
    }
    pp.filmSimulation.enabled = getEnabled();
    read(&pp);
}

void FilmSimulation::afterToneCurveToggled()
{
    if (listener && getEnabled()) {
        listener->panelChanged(EvAfterToneCurve, after_tone_curve_->get_active()
                                                     ? M("GENERAL_ENABLED")
                                                     : M("GENERAL_DISABLED"));
    }
}


void FilmSimulation::registerShortcuts(ToolShortcutsManager *mgr)
{
    mgr->addShortcut("tool.filmsimulation.strength", this, m_strength);
}


} } // namespace art::gui
