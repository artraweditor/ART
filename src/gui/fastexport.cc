/** -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright (c) 2019-2021 Alberto Griggio <alberto.griggio@gmail.com>
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
#include "fastexport.h"
#include "options.h"

namespace art { namespace gui {


namespace {

void adjust_fast_params(art::engine::procparams::ProcParams &params,
                        int size)
{
    // a size overrides the preferences: the long edge is at most `size`
    const int fw = size > 0 ? size : options.fastexport_resize_width;
    const int fh = size > 0 ? size : options.fastexport_resize_height;
    params.resize.unit = art::engine::procparams::ResizeParams::PX;
    if (params.resize.enabled) {
        params.resize.width = art::engine::min(params.resize.get_width(), fw);
        params.resize.height = art::engine::min(params.resize.get_height(), fh);
    } else {
        params.resize.width = fw;
        params.resize.height = fh;
    }

    params.resize.enabled = true;
    params.resize.scale = 1;
    params.resize.appliesTo = "Cropped area";
    params.resize.dataspec = 3;
    params.resize.allowUpscaling = false;
}

} // namespace

art::engine::ProcessingJob *
create_processing_job(const Glib::ustring &fname, bool is_raw,
                      art::engine::procparams::ProcParams params, bool fast, int fast_size)
{
    if (fast) {
        adjust_fast_params(params, fast_size);
    }

    auto ret = art::engine::ProcessingJob::create(fname, is_raw, params, fast);
    return ret;
}

art::engine::ProcessingJob *
create_processing_job(art::engine::InitialImage *initialImage,
                      art::engine::procparams::ProcParams params, bool fast, int fast_size)
{
    if (fast) {
        adjust_fast_params(params, fast_size);
    }

    auto ret = art::engine::ProcessingJob::create(initialImage, params, fast);
    return ret;
}


} } // namespace art::gui
