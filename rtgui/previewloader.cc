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

#include "previewloader.h"
#include "../rtengine/threadpool.h"
#include "guiutils.h"
#include "options.h"
#include "threadutils.h"
#include <atomic>
#include <chrono>
#include <deque>
#include <set>
#include <thread>

#ifdef _OPENMP
#include <omp.h>
#endif

#define DEBUG(format, args...)
// #define DEBUG(format,args...) printf("PreviewLoader::%s: " format "\n",
// __FUNCTION__, ## args)

class PreviewLoader::Impl: public rtengine::NonCopyable {
public:
    struct Job {
        Job(int dir_id, const Glib::ustring &dir_entry,
            PreviewLoaderListener *listener)
            : dir_id_(dir_id), dir_entry_(dir_entry), listener_(listener)
        {
        }

        Job(): dir_id_(0), listener_(nullptr) {}

        int dir_id_;
        Glib::ustring dir_entry_;
        PreviewLoaderListener *listener_;
    };
    /* Issue 2406
        struct OutputJob
        {
            bool complete;
            int dir_id;
            PreviewLoaderListener* listener;
            FileBrowserEntry* fdn;
        };
    */
    struct JobCompare {
        bool operator()(const Job &lhs, const Job &rhs) const
        {
            if (lhs.dir_id_ == rhs.dir_id_) {
                return lhs.dir_entry_ < rhs.dir_entry_;
            }

            return lhs.dir_id_ < rhs.dir_id_;
        }
    };

    typedef std::set<Job, JobCompare> JobSet;

    Impl(): num_concurrent_threads_(0), job_count_(0) {}

    MyMutex mutex_;
    std::deque<Job> jobs_;
    std::atomic<int> num_concurrent_threads_;
    size_t job_count_;

    void processNextJob()
    {
        Job j;
        {
            MyMutex::MyLock lock(mutex_);

            // nothing to do; could be jobs have been removed
            if (jobs_.empty()) {
                DEBUG("processing: nothing to do");
                return;
            }

            // copy and remove front job
            j = jobs_.front();
            jobs_.pop_front();
            DEBUG("processing %s", j.dir_entry_.c_str());
            DEBUG("%d job(s) remaining", jobs_.size());
        }

        ++num_concurrent_threads_; // to detect when last thread in pool has run
                                   // out
        try {
            Thumbnail *tmb = nullptr;
            if (Glib::file_test(j.dir_entry_, Glib::FILE_TEST_EXISTS)) {
                tmb = cacheMgr->getEntry(j.dir_entry_);
            }

            if (tmb) {
                DEBUG("Preview Ready\n");
                j.listener_->previewReady(
                    j.dir_id_, new FileBrowserEntry(tmb, j.dir_entry_));
            }

            if (++job_count_ % 20 == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

        } catch (Glib::Error &e) {
        } catch (...) {
        }

        bool last = num_concurrent_threads_--;

        // signal at end
        if (last && jobs_.empty()) {
            j.listener_->previewsFinished(j.dir_id_);
        }
    }
};

PreviewLoader::PreviewLoader(): impl_(new Impl()) {}

PreviewLoader::~PreviewLoader() { delete impl_; }

PreviewLoader *PreviewLoader::getInstance()
{
    static PreviewLoader instance_;
    return &instance_;
}

void PreviewLoader::add(int dir_id, const Glib::ustring &dir_entry,
                        PreviewLoaderListener *l)
{
    // somebody listening?
    if (l != nullptr) {
        {
            MyMutex::MyLock lock(impl_->mutex_);

            // create a new job and append to queue
            DEBUG("saving job %s", dir_entry.c_str());
            impl_->jobs_.push_back(Impl::Job(dir_id, dir_entry, l));
        }

        // queue a run request
        DEBUG("adding run request %s", dir_entry.c_str());
        rtengine::ThreadPool::add_task(
            rtengine::ThreadPool::Priority::LOWEST,
            sigc::mem_fun(*impl_, &PreviewLoader::Impl::processNextJob));
    }
}

void PreviewLoader::removeAllJobs()
{
    MyMutex::MyLock lock(impl_->mutex_);
    impl_->jobs_.clear();
}
