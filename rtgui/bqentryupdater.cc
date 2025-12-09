/*
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
#include "bqentryupdater.h"
#include "../rtengine/threadpool.h"
#include "guiutils.h"
#include <gtkmm.h>

BatchQueueEntryUpdater batchQueueEntryUpdater;

BatchQueueEntryUpdater::BatchQueueEntryUpdater(): tostop_(false), stopped_(true)
{
}

void BatchQueueEntryUpdater::process(guint8 *oimg, int ow, int oh, int newh,
                                     BQEntryUpdateListener *listener,
                                     rtengine::ProcParams *pparams,
                                     Thumbnail *thumbnail)
{
    if (!oimg && (!pparams || !thumbnail)) {
        return;
    }

    {
        std::unique_lock<std::mutex> lock(job_queue_mutex_);
        // look up if an older version is in the queue
        std::list<Job>::iterator i;

        for (i = jqueue_.begin(); i != jqueue_.end(); ++i)
            if (i->oimg == oimg && i->listener == listener) {
                i->ow = ow;
                i->oh = oh;
                i->newh = newh;
                i->listener = listener;
                i->pparams = pparams;
                i->thumbnail = thumbnail;
                break;
            }

        // not found, create and append new job
        if (i == jqueue_.end()) {
            Job j;
            j.oimg = oimg;
            j.ow = ow;
            j.oh = oh;
            j.newh = newh;
            j.listener = listener;
            j.pparams = pparams;
            j.thumbnail = thumbnail;
            jqueue_.push_back(j);
        }
    }

    // Start thread if not running yet
    if (stopped_) {
        stopped_ = false;
        tostop_ = false;

        stopped_future_ = rtengine::ThreadPool::add_task(
            rtengine::ThreadPool::Priority::NORMAL,
            sigc::mem_fun(*this, &BatchQueueEntryUpdater::process_thread));
    }
}

bool BatchQueueEntryUpdater::process_thread()
{
    bool is_empty = false;

    while (!tostop_ && !is_empty) {
        Job current;
        {
            std::unique_lock<std::mutex> lock(job_queue_mutex_);
            is_empty = jqueue_.empty();

            if (!is_empty) {
                current = jqueue_.front();
                jqueue_.pop_front();
            }
        }

        if (is_empty) {
            break;
        }

        rtengine::IImage8 *img = nullptr;
        bool newBuffer = false;

        if (current.thumbnail && current.pparams) {
            // the thumbnail and the pparams are provided, it means that we have
            // to build the original preview image
            double tmpscale;
            img = current.thumbnail->processThumbImage(*current.pparams,
                                                       current.oh, tmpscale);

            if (img) {
                int prevw = img->getWidth();
                int prevh = img->getHeight();
                current.ow = prevw;
                current.oh = prevh;

                if (!current.oimg) {
                    current.oimg = new guint8[prevw * prevh * 3];
                    newBuffer = true;
                }

                memcpy(current.oimg, img->getData(), prevw * prevh * 3);
                img->free();
            }
        }

        if (current.oimg && !is_empty && current.listener) {
            int neww = current.newh * current.ow / current.oh;
            guint8 *img = new guint8[current.newh * neww * 3];
            thumbInterp(current.oimg, current.ow, current.oh, img, neww,
                        current.newh);
            current.listener->updateImage(img, neww, current.newh, current.ow,
                                          current.oh,
                                          newBuffer ? current.oimg : nullptr);
        }

        if (current.oimg) {
            delete[] current.oimg;
            current.oimg = nullptr;
        }
    }

    stopped_ = true;

    return true;
}

void BatchQueueEntryUpdater::removeJobs(BQEntryUpdateListener *listener)
{
    std::unique_lock<std::mutex> lock(job_queue_mutex_);
    bool ready = false;

    while (!ready) {
        ready = true;
        std::list<Job>::iterator i;

        for (i = jqueue_.begin(); i != jqueue_.end(); ++i)
            if (i->listener == listener) {
                jqueue_.erase(i);
                ready = false;
                break;
            }
    }
}

void BatchQueueEntryUpdater::terminate()
{
    if (stopped_) {
        return;
    }

    tostop_ = true;
    stopped_ = stopped_future_.get();

    // Remove remaining jobs
    {
        std::unique_lock<std::mutex> lock(job_queue_mutex_);

        while (!jqueue_.empty()) {
            jqueue_.pop_front();
        }
    }
}
