/* -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright 2019 Alberto Griggio <alberto.griggio@gmail.com>
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

#include "../rtgui/multilangmgr.h"
#include "clutstore.h"
#include "color.h"
#include "curves.h"
#include "improcfun.h"

#ifdef _OPENMP
#include <omp.h>
#endif

namespace rtengine {

void ImProcFunctions::filmSimulation(Imagefloat *img)
{
    if (!params->filmSimulation.enabled) {
        return;
    }

    img->setMode(Imagefloat::Mode::RGB, multiThread);

#ifdef _OPENMP
    int num_threads = multiThread ? omp_get_max_threads() : 1;
#else
    int num_threads = 1;
#endif
    CLUTApplication clut(
        params->filmSimulation.clutFilename, params->icm.workingProfile,
        float(params->filmSimulation.strength) / 100.f, num_threads);

    if (clut) {
        CLUTApplication::Quality q = CLUTApplication::Quality::HIGHEST;
        switch (cur_pipeline) {
        case Pipeline::THUMBNAIL:
            q = CLUTApplication::Quality::LOW;
            break;
        case Pipeline::NAVIGATOR:
            q = CLUTApplication::Quality::MEDIUM;
            break;
        case Pipeline::PREVIEW:
            if (scale > 1) {
                q = CLUTApplication::Quality::HIGH;
            }
            break;
        default:
            break;
        }
        if (clut.set_param_values(params->filmSimulation.lut_params, q)) {
            clut(img);
        } else if (plistener) {
            plistener->error(
                Glib::ustring::compose(M("TP_FILMSIMULATION_LABEL") + " - " +
                                           M("ERROR_MSG_INVALID_LUT_PARAMS"),
                                       params->filmSimulation.clutFilename));
        }
    } else if (plistener) {
        plistener->error(Glib::ustring::compose(
            M("TP_FILMSIMULATION_LABEL") + " - " + M("ERROR_MSG_FILE_READ"),
            params->filmSimulation.clutFilename.empty()
                ? "(" + M("GENERAL_NONE") + ")"
                : params->filmSimulation.clutFilename));
    }
}

} // namespace rtengine
