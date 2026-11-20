/** -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright (c) 2019 Alberto Griggio <alberto.griggio@gmail.com>
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

#include "rtengine.h"

namespace art { namespace engine {


// fast_width, fast_height: the size (the long edge, in pixels) of the fast
// export (used only if `fast`)
art::engine::ProcessingJob *
create_processing_job(const Glib::ustring &fname, bool is_raw,
                      art::engine::procparams::ProcParams pparams, bool fast, int fast_width, int fast_height);

art::engine::ProcessingJob *
create_processing_job(art::engine::InitialImage *initialImage,
                      art::engine::procparams::ProcParams params, bool fast, int fast_width, int fast_height);


} } // namespace art::engine
