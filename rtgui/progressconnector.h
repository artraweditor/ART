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
#include "../rtengine/threadpool.h"
#include "guiutils.h"
#include <gtkmm.h>
#include <mutex>
#include <sigc++/sigc++.h>

#undef THREAD_PRIORITY_NORMAL

class PLDBridge: public rtengine::ProgressListener {
public:
    explicit PLDBridge(rtengine::ProgressListener *pb): pl(pb) {}

    // ProgressListener interface
    void setProgress(double p) override
    {
        GThreadLock lock;
        pl->setProgress(p);
    }
    void setProgressStr(const Glib::ustring &str) override
    {
        GThreadLock lock;
        Glib::ustring progrstr;
        progrstr = M(str);
        pl->setProgressStr(progrstr);
    }

    void setProgressState(bool inProcessing) override
    {
        GThreadLock lock;
        pl->setProgressState(inProcessing);
    }

    void error(const Glib::ustring &descr) override
    {
        GThreadLock lock;
        pl->error(descr);
    }

private:
    rtengine::ProgressListener *const pl;
};

template <class T> class ProgressConnector {
    sigc::signal0<T> opStart;
    sigc::signal0<bool> opEnd;
    T retval;
    bool working_;
    std::mutex mtx_;

    static int emitEndSignalUI(void *data)
    {
        sigc::signal0<bool> *opEnd = (sigc::signal0<bool> *)data;
        int r = opEnd->emit();
        delete opEnd;

        return r;
    }

    void workingThread()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        retval = opStart.emit();
        gdk_threads_add_idle(ProgressConnector<T>::emitEndSignalUI,
                             new sigc::signal0<bool>(opEnd));
        working_ = false;
    }

public:
    ProgressConnector(): retval(0), working_(false) {}

    void startFunc(const sigc::slot0<T> &startHandler,
                   const sigc::slot0<bool> &endHandler)
    {
        if (!working_) {
            opStart.connect(startHandler);
            opEnd.connect(endHandler);
            rtengine::ThreadPool::add_task(
                rtengine::ThreadPool::Priority::NORMAL,
                sigc::mem_fun(*this, &ProgressConnector<T>::workingThread));
        }
    }

    T returnValue() { return retval; }

    void destroy()
    {
        // std::unique_lock<std::mutex> lock(mtx_);
        delete this;
    }
};
