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

#include "tiling.h"
#include "../imagefloat.h"
#include "gpu.h"

#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <memory>

#include <algorithm>
#include <cmath>
#include <cstdlib>

#ifdef ART_USE_VULKAN
#include "vk_context.h"
#endif

namespace art { namespace engine { namespace gpu {

namespace {

int roundDown(int v, int a)
{
    return (v / a) * a;
}

int roundUp(int v, int a)
{
    return ((v + a - 1) / a) * a;
}

} // namespace


void coarseRange(int o, int n, int F, int F4, int &j0, int &j1)
{
    const float R = float(F) / float(F4);
    if (o == 0) {
        j0 = 0;
    } else {
        j0 = std::max(0, int(std::ceil(double(o) / R)) - 1);
        while (int(float(j0) * R) < o) {
            ++j0;
        }
    }
    if (o + n >= F) {
        j1 = F4 - 1;
    } else {
        j1 = std::min(F4 - 1, int(std::floor(double(o + n - 2) / R)) + 1);
        while (j1 >= 0 && std::min(int(float(j1) * R), F - 1) + 1 > o + n - 1) {
            --j1;
        }
    }
}


std::vector<Tile> planTiles(int W, int H, int halo, int align,
                            size_t max_pixels)
{
    std::vector<Tile> ret;
    if (W <= 0 || H <= 0 || max_pixels == 0) {
        return ret;
    }
    align = std::max(align, 1);
    halo = roundUp(std::max(halo, 0), align);

    if ((size_t)W * (size_t)H <= max_pixels) {
        Tile t;
        t.interior = t.padded = TileRect(0, 0, W, H);
        ret.push_back(t);
        return ret;
    }

    /* Aim for square padded tiles (least halo overhead per interior pixel);
     * if that makes one dimension span the whole image, give the slack to the
     * other one, since a full-span dimension needs no halo on that axis. */
    const double P = double(max_pixels);
    int tw = int(std::sqrt(P)) - 2 * halo;
    int th = tw;
    if (tw >= W) {
        tw = W;
        th = int(P / double(W)) - 2 * halo;
    } else if (th >= H) {
        th = H;
        tw = int(P / double(H)) - 2 * halo;
    }
    tw = std::min(tw, W);
    th = std::min(th, H);
    if (tw < W) {
        tw = roundDown(tw, align);
    }
    if (th < H) {
        th = roundDown(th, align);
    }
    if (tw < align || th < align) {
        return ret;
    }

    /* Rounding and clipping can only shrink a padded tile, but a span that is
     * not clipped on one side (an interior tile) is the worst case, so check
     * it explicitly rather than trust the arithmetic above. */
    const int pw = std::min(W, tw + 2 * halo);
    const int ph = std::min(H, th + 2 * halo);
    if ((size_t)pw * (size_t)ph > max_pixels) {
        return ret;
    }

    for (int y = 0; y < H; y += th) {
        for (int x = 0; x < W; x += tw) {
            Tile t;
            t.interior = TileRect(x, y, std::min(tw, W - x), std::min(th, H - y));
            const int x0 = std::max(0, x - halo);
            const int y0 = std::max(0, y - halo);
            const int x1 = std::min(W, t.interior.right() + halo);
            const int y1 = std::min(H, t.interior.bottom() + halo);
            t.padded = TileRect(x0, y0, x1 - x0, y1 - y0);
            ret.push_back(t);
        }
    }
    return ret;
}


size_t tilePixelBudget(int planes, int scratch_factor)
{
#ifdef ART_USE_VULKAN
    Context *ctx = Context::get();
    if (!ctx || ctx->deviceLost() || planes <= 0) {
        return 0;
    }
    size_t bytes = ctx->caps().max_storage_buffer_range;
    if (const char *e = std::getenv("ART_GPU_MAX_TILE_BYTES")) {
        const unsigned long long v = std::strtoull(e, nullptr, 10);
        if (v > 0 && v < bytes) {
            bytes = size_t(v);
        }
    }
    const size_t per_pixel =
        sizeof(float) * size_t(planes) * size_t(std::max(scratch_factor, 1));
    return bytes / per_pixel;
#else
    (void)planes;
    (void)scratch_factor;
    return 0;
#endif
}


namespace {

void copyRect(Imagefloat &dst, int dx, int dy, Imagefloat &src, int sx, int sy,
              int w, int h)
{
    for (int y = 0; y < h; ++y) {
        std::memcpy(&dst.r(dy + y, dx), &src.r(sy + y, sx), w * sizeof(float));
        std::memcpy(&dst.g(dy + y, dx), &src.g(sy + y, sx), w * sizeof(float));
        std::memcpy(&dst.b(dy + y, dx), &src.b(sy + y, sx), w * sizeof(float));
    }
}

} // namespace

TiledResult processTiled(
    Imagefloat *img, int halo, int align, int planes, int scratch_factor,
    const std::function<bool(Imagefloat &, const Tile &, int phase)> &op,
    int phases, const std::function<bool(Imagefloat &)> *whole,
    size_t cap_pixels, double max_cost)
{
    const int W = img->getWidth();
    const int H = img->getHeight();
    size_t budget = tilePixelBudget(planes, scratch_factor);
    if (!budget) {
        return TiledResult::FAILED;
    }
    if (cap_pixels && cap_pixels < budget) {
        budget = cap_pixels;
    }
    const std::vector<Tile> tiles = planTiles(W, H, halo, align, budget);
    if (tiles.empty()) {
        logOnce("GPU: no tile fits the buffer limit; using the CPU");
        return TiledResult::FAILED;
    }
    if (tiles.size() == 1) {
        return TiledResult::NOT_NEEDED;
    }

    if (max_cost > 0.0 && !std::getenv("ART_GPU_TILE_FORCE") &&
        !std::getenv("ART_GPU_TILE_VERIFY")) {
        double area = 0.0;
        for (size_t i = 0; i < tiles.size(); ++i) {
            area += double(tiles[i].padded.w) * double(tiles[i].padded.h);
        }
        const double cost =
            double(std::max(phases, 1)) * area / (double(W) * double(H));
        if (cost > max_cost) {
            std::ostringstream os;
            os << "GPU: tiling would cost ~" << std::fixed
               << std::setprecision(1) << cost
               << " whole-image passes (limit " << max_cost
               << "), slower than the CPU; using the CPU";
            logOnce(os.str());
            return TiledResult::FAILED;
        }
    }

    img->syncCpu();
    std::unique_ptr<Imagefloat> ref, orig;
    if (whole && std::getenv("ART_GPU_TILE_VERIFY")) {
        ref.reset(new Imagefloat(W, H, img));
        copyRect(*ref, 0, 0, *img, 0, 0, W, H);
        orig.reset(new Imagefloat(W, H, img));
        copyRect(*orig, 0, 0, *img, 0, 0, W, H);
    }
    Imagefloat out(W, H, img);
    for (int phase = 0; phase < std::max(phases, 1); ++phase) {
        const bool last = phase == std::max(phases, 1) - 1;
        for (size_t i = 0; i < tiles.size(); ++i) {
            const Tile &t = tiles[i];
            Imagefloat tile(t.padded.w, t.padded.h, img);
            copyRect(tile, 0, 0, *img, t.padded.x, t.padded.y, t.padded.w,
                     t.padded.h);
            if (!op(tile, t, phase)) {
                return TiledResult::FAILED;
            }
            if (last) {
                tile.syncCpu();
                copyRect(out, t.interior.x, t.interior.y, tile,
                         t.interior.x - t.padded.x, t.interior.y - t.padded.y,
                         t.interior.w, t.interior.h);
            }
        }
    }
    img->syncCpuForWrite();
    copyRect(*img, 0, 0, out, 0, 0, W, H);

    if (ref) {
        if ((*whole)(*ref)) {
            ref->syncCpu();
            double mx = 0, sum = 0;
            long n = 0, big = 0, nan_mismatch = 0, nans = 0;
            int bx = 0, by = 0;
            for (int y = 0; y < H; ++y) {
                for (int x = 0; x < W; ++x) {
                    const float a[3] = {img->r(y, x), img->g(y, x), img->b(y, x)};
                    const float b[3] = {ref->r(y, x), ref->g(y, x), ref->b(y, x)};
                    for (int c = 0; c < 3; ++c) {
                        if (std::isnan(a[c]) != std::isnan(b[c])) {
                            ++nan_mismatch;
                        }
                        if (std::isnan(a[c]) || std::isnan(b[c])) {
                            ++nans;
                            continue;
                        }
                        const double d = a[c] == b[c] ? 0.0 : std::fabs(double(a[c]) - b[c]);
                        sum += d;
                        ++n;
                        big += d > 1e-4;
                        if (d > mx) {
                            mx = d;
                            bx = x;
                            by = y;
                        }
                    }
                }
            }
            double change = 0, vmax = 0;
            long changed = 0, nonfinite_in = 0;
            for (int y = 0; y < H; ++y) {
                for (int x = 0; x < W; ++x) {
                    const double d = std::fabs(double(img->g(y, x)) - orig->g(y, x));
                    change += std::isfinite(d) ? d : 0.0;
                    changed += !(d == 0.0);
                    nonfinite_in += !std::isfinite(orig->g(y, x));
                    if (std::isfinite(img->g(y, x))) {
                        vmax = std::max(vmax, std::fabs(double(img->g(y, x))));
                    }
                }
            }
            std::cerr << "GPU tile verify: " << changed << " G values changed, "
                      << nonfinite_in << " non-finite in input, max |G| out "
                      << vmax << ", G[H/2][W/2] in/out/ref "
                      << orig->g(H / 2, W / 2) << " " << img->g(H / 2, W / 2)
                      << " " << ref->g(H / 2, W / 2) << std::endl;
            std::cerr << "GPU tile verify (op changed G by a mean of "
                      << change / (double(W) * H) << "): " << tiles.size() << " tiles, max "
                      << mx << " at " << bx << "," << by << ", mean "
                      << (n ? sum / n : 0.0) << ", " << big << " values > 1e-4, "
                      << nans << " NaN values, " << nan_mismatch
                      << " NaN mismatches" << std::endl;
        } else {
            std::cerr << "GPU tile verify: whole-image run failed" << std::endl;
        }
    }
    return TiledResult::DONE;
}

}}} // namespace art::engine::gpu
