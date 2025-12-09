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

#include "../rtengine/rtengine.h"
#include "threadutils.h"
#include "thumbnail.h"
#include <glibmm.h>

#include <atomic>
#include <future>
#include <mutex>

class BQEntryUpdateListener {
public:
    virtual ~BQEntryUpdateListener() = default;
    virtual void updateImage(guint8 *img, int w, int h, int origw, int origh,
                             guint8 *newOPreview) = 0;
};

class BatchQueueEntryUpdater {
    struct Job {
        guint8 *oimg;
        int ow, oh, newh;
        BQEntryUpdateListener *listener;
        rtengine::ProcParams *pparams;
        Thumbnail *thumbnail;

        Job() = default;
    };

public:
    BatchQueueEntryUpdater();

    void process(guint8 *oimg, int ow, int oh, int newh,
                 BQEntryUpdateListener *listener,
                 rtengine::ProcParams *pparams = nullptr,
                 Thumbnail *thumbnail = nullptr);
    void removeJobs(BQEntryUpdateListener *listener);
    void terminate();

private:
    bool process_thread();

    std::atomic<bool> tostop_;
    bool stopped_;
    std::future<bool> stopped_future_;
    std::list<Job> jqueue_;
    std::mutex job_queue_mutex_;
};

extern BatchQueueEntryUpdater batchQueueEntryUpdater;
