/* -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright 2023 Alberto Griggio <alberto.griggio@gmail.com>
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

#include "../rtengine/compress.h"
#include "../rtengine/iccstore.h"
#include "../rtengine/procparams.h"
#include "../rtengine/rtengine.h"
#include "cachemanager.h"
#include <glibmm.h>

namespace art {
namespace thumbimgcache {

/******************************************************************************
 * file format:
 *
 * "ART\n" header
 * monitor hash
 * size of the procparams
 * compressed procparams
 * width
 * height
 * image data
 ******************************************************************************/
rtengine::IImage8 *load(const Glib::ustring &cache_fname,
                        const rtengine::procparams::ProcParams &pparams, int h);

bool store(const Glib::ustring &cache_fname,
           const rtengine::procparams::ProcParams &pparams,
           rtengine::IImage8 *img);

} // namespace thumbimgcache
} // namespace art
