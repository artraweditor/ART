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

/*
 * Tile planner for images too large for one device buffer -- see
 * doc/gpu_pipeline.md (Tiling).
 *
 * C++11 and Vulkan-free.  A plan is a list of tiles; each has an `interior`
 * (the pixels it is responsible for; interiors partition the image exactly)
 * and a `padded` rectangle (interior grown by the op's halo and clipped to
 * the image).  Clipping means a tile touching the image border sees the same
 * clamp-to-edge behaviour as the whole-image run, while an interior edge sees
 * real neighbouring pixels.
 */
#pragma once

#include <cstddef>
#include <functional>
#include <vector>

namespace art { namespace engine {

class Imagefloat;

namespace gpu {

struct TileRect {
    TileRect(): x(0), y(0), w(0), h(0) {}
    TileRect(int x, int y, int w, int h): x(x), y(y), w(w), h(h) {}
    int x, y, w, h;
    int right() const { return x + w; }
    int bottom() const { return y + h; }
};

struct Tile {
    TileRect interior;
    TileRect padded;
};

/* Plans tiles for a W x H image.
 *
 *   halo        minimum neighbourhood radius the op needs, in pixels.  Rounded
 *               up to a multiple of `align` so padded origins stay aligned too.
 *   align       every interior origin and every padded origin is a multiple of
 *               this (decimation by 2 -> 2, the NL-means mask rescale -> 4, ...).
 *               Only the last tile in a row/column may have a size that is not.
 *   max_pixels  upper bound on padded.w * padded.h for every tile (the caller
 *               derives it from the buffer limit / memory budget).
 *
 * Returns a single tile covering the whole image when it fits, and an empty
 * vector when no tile of at least `align` x `align` interior fits in the
 * budget (the caller then falls back to the CPU).  Tiles are in row-major
 * order. */
/* Where a W x H buffer sits inside a larger fw x fh image.  Ops that sample on
 * a grid defined by the whole image's size (a 4x or Nx subsampling whose real
 * ratio is fw/(fw/N), not N) take one so a tile reads the very samples the
 * whole-image run would.  nullptr means the buffer is the whole image. */
struct TileFrame {
    int ox, oy, fw, fh;
};

/* The coarse samples of a fine axis [o, o+n) of an F-long image whose coarse
 * grid has F4 samples spaced F/F4 apart (sample j reads fine position
 * j*F/F4): the first, j0, and last, j1, whose bilinear footprint lies inside
 * the span -- all the way to the border where the span touches the image's. */
void coarseRange(int o, int n, int F, int F4, int &j0, int &j1);

std::vector<Tile> planTiles(int W, int H, int halo, int align,
                            size_t max_pixels);

/* Pixel budget for one padded tile holding `planes` float planes with
 * `scratch_factor` times that in scratch space, bounded by the device's
 * max_storage_buffer_range and by ART_GPU_MAX_TILE_BYTES when set (a debug
 * aid that forces tiling on a machine with plenty of memory).  0 when no
 * device is available. */
size_t tilePixelBudget(int planes, int scratch_factor = 1);


enum class TiledResult {
    NOT_NEEDED, // the whole image fits one tile: run the untiled path
    DONE,       // img now holds the result
    FAILED      // img is untouched: use the CPU path
};

/* Host-side tile loop for an op that maps an Imagefloat to an Imagefloat of
 * the same size and only looks `halo` pixels around each pixel.
 *
 * For every tile, `op` receives a padded copy of the tile (and the Tile, whose
 * padded.x/y say where it sits in the image) and must leave its result in
 * that image (CPU-valid).  Only the interior is kept.  Results go
 * to a separate output so later tiles still see unfiltered halos; img is
 * overwritten only when every tile succeeded.  `planes` and `scratch_factor`
 * feed tilePixelBudget().
 *
 * `phases` > 1 runs the whole tile loop that many times, for ops that need
 * whole-image statistics first: `op` gets the phase number, and only the last
 * phase's results are kept.
 *
 * `cap_pixels`, when nonzero, lowers the per-tile pixel budget (for ops whose
 * memory use is not just their planes).
 *
 * ART_GPU_TILE_VERIFY=1 additionally runs `whole` (when given) on a copy of the
 * input and reports the largest difference to the tiled result on stderr --
 * the check that a tiled op is exact, without the downstream noise of a full
 * export. */
TiledResult processTiled(
    Imagefloat *img, int halo, int align, int planes, int scratch_factor,
    const std::function<bool(Imagefloat &, const Tile &, int phase)> &op,
    int phases = 1,
    const std::function<bool(Imagefloat &)> *whole = nullptr,
    size_t cap_pixels = 0);

}}} // namespace art::engine::gpu
