/* -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright (c) 2004-2010 Gabor Horvath <hgabor@rawtherapee.com>
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

#include "../utils/threadutils.h"
#include "image16.h"
#include "image8.h"
#include <chrono>

#include "imagesource.h"
#include "improccoordinator.h"
#include "improcfun.h"
#include "pipettebuffer.h"
#include "procevents.h"
#include "rtengine.h"

namespace art { namespace engine {

using namespace procparams;

class ImProcCoordinator;

class Crop: public DetailedCrop, public PipetteBuffer {

protected:
    // --- permanently allocated in RAM and only renewed on size changes
    Imagefloat *origCrop; // "one chunk" allocation
    Imagefloat *spotCrop; // "one chunk" allocation
    Imagefloat *denoiseCrop;
    Imagefloat *bufs_[3];
    std::array<bool, 4> pipeline_stop_;
    Image8
        *cropImg; // "one chunk" allocation ; displayed image in monitor color
                  // space, showing the output profile as well (soft-proofing
                  // enabled, which then correspond to workimg) or not

    // --- automatically allocated and deleted when necessary, and only renewed
    // on size changes
    Imagefloat *transCrop; // "one chunk" allocation, allocated if necessary
    // -----------------------------------------------------------------

    bool updating; /// Flag telling if an updater thread is currently processing
    bool newUpdatePending; /// Flag telling the updater thread that a new update
                           /// is pending
    int skip;
    int cropx, cropy, cropw, croph; /// size of the detail crop image ('skip'
                                    /// taken into account), with border
    int trafx, trafy, trafw,
        trafh; /// the size and position to get from the imagesource that is
               /// transformed to the requested crop area
    int rqcropx, rqcropy, rqcropw,
        rqcroph; /// size of the requested detail crop image (the image might be
                 /// smaller) (without border)
    const int
        borderRequested; /// requested extra border size for image processing
    int upperBorder,
        leftBorder; /// extra border size really allocated for image processing

    bool cropAllocated;
    bool srgbOutput_ = false;
    DetailedCropListener *cropImageListener;

    art::utils::MyMutex cropMutex;
    ImProcCoordinator *const parent;
    const bool isDetailWindow;
    art::utils::EditUniqueID getCurrEditID();
    bool setCropSizes(int cropX, int cropY, int cropW, int cropH, int skip,
                      bool internal);
    void freeAll();

    friend class ImProcCoordinator;
    void update(int todo);

private:
    /// The work of update(), which may throw Cancelled between tools.
    void updateImpl(int todo);

    /// What must be redone next time because an abandoned update did not
    /// finish it (consumed by the next update()).
    int pendingTodo_ = 0;
    /// What an update abandoned at its current phase would have to redo;
    /// kept up to date by updateImpl().
    int restoreOnAbort_ = 0;
    /// When this crop last made it to the end of an update.
    std::chrono::steady_clock::time_point lastShown_{};

protected:

public:
    Crop(ImProcCoordinator *parent, art::gui::EditDataProvider *editDataProvider,
         bool isDetailWindow);
    ~Crop() override;

    void setEditSubscriber(art::gui::EditSubscriber *newSubscriber);
    void setSRGBOutput(bool yes) override { srgbOutput_ = yes; }
    bool hasListener();
    void setWindow(int cropX, int cropY, int cropW, int cropH,
                   int skip) override
    {
        setCropSizes(cropX, cropY, cropW, cropH, skip, false);
    }

    /** @brief Synchronously look out if a full update is necessary
     * First try, only make fullUpdate if this returns false
     */
    bool tryUpdate() override;
    /** @brief Asynchronously reprocess the detailed crop */
    void fullUpdate() override; // called via thread

    void setListener(DetailedCropListener *il) override;
    void destroy() override {}
    int get_skip();
    int getLeftBorder();
    int getUpperBorder();
};
}} // namespace art::engine
