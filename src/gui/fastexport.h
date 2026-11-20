/* -*- C++ -*-
 *
 *  This file is part of ART.
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
#pragma once

#include "../engine/fastexport.h"
#include "options.h"

namespace art { namespace gui {

// The processing jobs of the GUI and ART-cli: the size of the fast export
// comes from the preferences, unless fast_size > 0 (the long edge, in pixels)

inline art::engine::ProcessingJob *
create_processing_job(const Glib::ustring &fname, bool is_raw,
                      art::engine::procparams::ProcParams pparams, bool fast,
                      int fast_size = 0)
{
    return art::engine::create_processing_job(
        fname, is_raw, std::move(pparams), fast,
        fast_size > 0 ? fast_size : options.fastexport_resize_width,
        fast_size > 0 ? fast_size : options.fastexport_resize_height);
}

inline art::engine::ProcessingJob *
create_processing_job(art::engine::InitialImage *initialImage,
                      art::engine::procparams::ProcParams params, bool fast,
                      int fast_size = 0)
{
    return art::engine::create_processing_job(
        initialImage, std::move(params), fast,
        fast_size > 0 ? fast_size : options.fastexport_resize_width,
        fast_size > 0 ? fast_size : options.fastexport_resize_height);
}

}} // namespace art::gui
