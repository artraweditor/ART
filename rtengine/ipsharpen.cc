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

#include "bilateral2.h"
#include "gauss.h"
#include "improcfun.h"
#include "jaggedarray.h"
#include "opthelper.h"
#include "rt_math.h"
#include "sleef.h"
// #define BENCHMARK
#include "../rtgui/multilangmgr.h"
#include "StopWatch.h"
#include "cJSON.h"
#include "cache.h"
#include "coord.h"
#include "rt_algo.h"
#include "stdimagesource.h"
#include <sstream>

namespace rtengine {

extern const Settings *settings;

namespace {

template <bool reverse>
void apply_gamma(float **Y, int W, int H, float pivot, float gamma,
                 bool multiThread)
{
    BENCHFUN

    if (!reverse) {
        gamma = 1.f / gamma;
    }

    LUTf glut(65536);
    glut[0] = 0;
    const float d = 65535.f * pivot;
    for (int i = 1; i < 65536; ++i) {
        glut[i] = pow_F(float(i) / d, gamma) * pivot;
        glut[i] *= 65535.f;
    }

#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            float l = Y[y][x];
            if (LIKELY(l >= 0.f && l < 65536.f)) {
                l = glut[l];
            } else {
                l = pow_F(std::max(l / d, 1e-18f), gamma) * pivot;
                l *= 65535.f;
            }
            Y[y][x] = l;
        }
    }
}

void sharpenHaloCtrl(float **luminance, float **blurmap, float **base,
                     float **blend, int W, int H,
                     const SharpeningParams &sharpenParam, bool multiThread)
{

    const float scale = (100.f - sharpenParam.halocontrol_amount) * 0.01f;
    const float sharpFac = sharpenParam.amount * 0.01f;
    float **nL = base;

#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif

    for (int i = 2; i < H - 2; i++) {
        float max1 = 0, max2 = 0, min1 = 0, min2 = 0;

        for (int j = 2; j < W - 2; j++) {
            // compute 3 iterations, only forward
            float np1 =
                2.f *
                    (nL[i - 2][j] + nL[i - 2][j + 1] + nL[i - 2][j + 2] +
                     nL[i - 1][j] + nL[i - 1][j + 1] + nL[i - 1][j + 2] +
                     nL[i][j] + nL[i][j + 1] + nL[i][j + 2]) /
                    27.f +
                nL[i - 1][j + 1] / 3.f;
            float np2 =
                2.f *
                    (nL[i - 1][j] + nL[i - 1][j + 1] + nL[i - 1][j + 2] +
                     nL[i][j] + nL[i][j + 1] + nL[i][j + 2] + nL[i + 1][j] +
                     nL[i + 1][j + 1] + nL[i + 1][j + 2]) /
                    27.f +
                nL[i][j + 1] / 3.f;
            float np3 =
                2.f *
                    (nL[i][j] + nL[i][j + 1] + nL[i][j + 2] + nL[i + 1][j] +
                     nL[i + 1][j + 1] + nL[i + 1][j + 2] + nL[i + 2][j] +
                     nL[i + 2][j + 1] + nL[i + 2][j + 2]) /
                    27.f +
                nL[i + 1][j + 1] / 3.f;

            // Max/Min of all these deltas and the last two max/min
            float maxn = rtengine::max(np1, np2, np3);
            float minn = rtengine::min(np1, np2, np3);
            float max_ = rtengine::max(max1, max2, maxn);
            float min_ = rtengine::min(min1, min2, minn);

            // Shift the queue
            max1 = max2;
            max2 = maxn;
            min1 = min2;
            min2 = minn;
            float labL = luminance[i][j];

            if (max_ < labL) {
                max_ = labL;
            }

            if (min_ > labL) {
                min_ = labL;
            }

            // deviation from the environment as measurement
            float diff = nL[i][j] - blurmap[i][j];

            constexpr float upperBound =
                2000.f; // WARNING: Duplicated value, it's baaaaaad !
            float delta = sharpenParam.threshold.multiply<float, float, float>(
                rtengine::min(fabsf(diff),
                              upperBound), // X axis value = absolute value of
                                           // the difference
                sharpFac * diff // Y axis max value = sharpening.amount * signed
                                // difference
            );
            float newL = labL + delta;

            // applying halo control
            if (newL > max_) {
                newL = max_ + (newL - max_) * scale;
            } else if (newL < min_) {
                newL = min_ - (min_ - newL) * scale;
            }

            luminance[i][j] = intp(blend[i][j], newL, luminance[i][j]);
        }
    }
}

void deconvsharpening(float **luminance, float **blend, char **impulse, int W,
                      int H, double sigma, float amount, bool multiThread)
{
    if (amount <= 0) {
        return;
    }
    BENCHFUN

    const int maxiter = 20;
    const float delta_factor = 0.2f;

    if (sigma < 0.2f) {
        return;
    }

    JaggedArray<float> tmp(W, H);
    JaggedArray<float> tmpI(W, H);
    JaggedArray<float> out(W, H);

    constexpr float offset = 1000.f;

#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif
    for (int i = 0; i < H; i++) {
        for (int j = 0; j < W; j++) {
            luminance[i][j] += offset;
            tmpI[i][j] = std::max(luminance[i][j], 0.f);
            assert(std::isfinite(tmpI[i][j]));
            out[i][j] = RT_NAN;
        }
    }

    const auto get_output = [&](int i, int j) -> float {
        if (UNLIKELY(std::isnan(tmpI[i][j]))) {
            return luminance[i][j];
        }
        float b = impulse[i][j] ? 0.f : blend[i][j] * amount;
        return intp(b, std::max(tmpI[i][j], 0.0f), luminance[i][j]);
    };

    const auto check_stop = [&](int y, int x) -> void {
        if (LIKELY(std::isnan(out[y][x]))) {
            float l = luminance[y][x];
            float delta = l * delta_factor;
            if (UNLIKELY(std::abs(tmpI[y][x] - l) > delta)) {
                out[y][x] = get_output(y, x);
            }
        }
    };

#ifdef _OPENMP
#pragma omp parallel if (multiThread)
#endif
    {
        for (int k = 0; k < maxiter; k++) {
            gaussianBlur(tmpI, tmp, W, H, sigma, nullptr, GAUSS_DIV, luminance);
            gaussianBlur(tmp, tmpI, W, H, sigma, nullptr, GAUSS_MULT);
#ifdef _OPENMP
#pragma omp for
#endif
            for (int y = 0; y < H; ++y) {
                for (int x = 0; x < W; ++x) {
                    check_stop(y, x);
                }
            }
        }

#ifdef _OPENMP
#pragma omp for
#endif
        for (int i = 0; i < H; ++i) {
            for (int j = 0; j < W; ++j) {
                float l = out[i][j];
                if (std::isnan(l)) {
                    l = get_output(i, j);
                }
                assert(std::isfinite(l));
                luminance[i][j] = std::max(l - offset, 0.f);
            }
        }
    }
}

void unsharp_mask(float **Y, float **blend, int W, int H,
                  const SharpeningParams &sharpenParam, double scale,
                  bool multiThread)
{
    BENCHFUN
    apply_gamma<false>(Y, W, H, 1.f, 3.f, multiThread);

    float **b3 = nullptr;

    if (sharpenParam.edgesonly) {
        b3 = new float *[H];

        for (int i = 0; i < H; i++) {
            b3[i] = new float[W];
        }
    }

    JaggedArray<float> b2(W, H);

#ifdef _OPENMP
#pragma omp parallel if (multiThread)
#endif
    {

        if (!sharpenParam.edgesonly) {
            gaussianBlur(Y, b2, W, H, sharpenParam.radius / scale);
        } else {
            bilateral<float, float>(Y, (float **)b3, b2, W, H,
                                    sharpenParam.edges_radius / scale,
                                    sharpenParam.edges_tolerance, multiThread);
            gaussianBlur(b3, b2, W, H, sharpenParam.radius / scale);
        }
    }
    float **base = Y;

    if (sharpenParam.edgesonly) {
        base = b3;
    }

    if (!sharpenParam.halocontrol) {
#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif

        for (int i = 0; i < H; i++)
            for (int j = 0; j < W; j++) {
                constexpr float upperBound =
                    2000.f; // WARNING: Duplicated value, it's baaaaaad !
                float diff = base[i][j] - b2[i][j];
                float delta =
                    sharpenParam.threshold.multiply<float, float, float>(
                        std::min(fabsf(diff),
                                 upperBound), // X axis value = absolute value
                                              // of the difference, truncated to
                                              // the max value of this field
                        sharpenParam.amount * diff * 0.01f // Y axis max value
                    );
                Y[i][j] = intp(blend[i][j], Y[i][j] + delta, Y[i][j]);
            }
    } else {
        if (!sharpenParam.edgesonly) {
            // make a deep copy of lab->L
            JaggedArray<float> labCopy(W, H);

#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif

            for (int i = 0; i < H; i++)
                for (int j = 0; j < W; j++) {
                    labCopy[i][j] = Y[i][j];
                }

            sharpenHaloCtrl(Y, b2, labCopy, blend, W, H, sharpenParam,
                            multiThread);
        } else {
            sharpenHaloCtrl(Y, b2, base, blend, W, H, sharpenParam,
                            multiThread);
        }
    }

    if (sharpenParam.edgesonly) {
        for (int i = 0; i < H; i++) {
            delete[] b3[i];
        }

        delete[] b3;
    }

    apply_gamma<true>(Y, W, H, 1.f, 3.f, multiThread);
}

class CornerBoostMask {
public:
    CornerBoostMask(int ox, int oy, int width, int height, int latitude)
        : ox_(ox), oy_(oy), w2_(width / 2), h2_(height / 2)
    {
        float radius = std::max(w2_, h2_);
        r2_ = (radius - radius * LIM01(float(latitude) / 150.f)) / 2.f;
        sigma_ = 2.f * SQR(radius * 0.3f);
    }

    float operator()(int x, int y) const
    {
        int xx = x + ox_ - w2_;
        int yy = y + oy_ - h2_;
        float distance = std::sqrt(float(SQR(xx) + SQR(yy)));
        return 1.f -
               LIM01(xexpf((-SQR(std::max(distance - r2_, 0.f)) / sigma_)));
    }

private:
    int ox_;
    int oy_;
    int w2_;
    int h2_;
    float r2_;
    float sigma_;
};

Cache<Glib::ustring, std::shared_ptr<array2D<float>>> rl_kernel_cache(10);

template <class Img> bool import_kernel(Img *img, array2D<float> &out)
{
    int w = img->getWidth();
    if (w != img->getHeight()) {
        return false;
    }
    if (!(w & 1)) {
        return false;
    }
    out(w, w);
    for (int y = 0; y < w; ++y) {
        for (int x = 0; x < w; ++x) {
            auto v = img->g(y, x);
            out[y][x] = v;
        }
    }
    return true;
}

bool import_kernel(cJSON *obj, array2D<float> &out)
{
    if (!cJSON_IsArray(obj)) {
        return false;
    }
    int n2 = cJSON_GetArraySize(obj);
    if (n2 <= 1) {
        return false;
    }
    cJSON *item = cJSON_GetArrayItem(obj, 0);
    if (cJSON_IsArray(item)) {
        // matrix form
        int kn = n2;
        out(kn, kn);
        cJSON *row;
        int i = 0;
        cJSON_ArrayForEach(row, obj)
        {
            if (!cJSON_IsArray(row) || cJSON_GetArraySize(row) != kn) {
                return false;
            }
            cJSON *elem;
            int j = 0;
            cJSON_ArrayForEach(elem, row)
            {
                if (!cJSON_IsNumber(elem)) {
                    return false;
                }
                out[i][j] = elem->valuedouble;
                ++j;
            }
            ++i;
        }
        return true;
    } else if (cJSON_IsNumber(item)) {
        float n = std::sqrt(float(n2));
        if (n != float(int(n))) {
            return false;
        }
        int kn = int(n);
        out(kn, kn);
        int i = 0, j = 0;
        cJSON *elem;
        cJSON_ArrayForEach(elem, obj)
        {
            if (!cJSON_IsNumber(elem)) {
                return false;
            }
            out[i][j] = elem->valuedouble;
            if (++j >= kn) {
                ++i;
                j = 0;
            }
        }
        return true;
    } else {
        return false;
    }
}

void rescale_kernel(const array2D<float> &src, array2D<float> &k)
{
    const int sw = src.width();
    const int w = k.width();

    const int sh = sw / 2;
    const int h = w / 2;

    const auto get = [&](float y, float x) -> float {
        bool nx = x < 0;
        bool ny = y < 0;
        x = std::abs(x);
        y = std::abs(y);

        int xi = x;
        int yi = y;
        float xf = x - xi;
        float yf = y - yi;
        int xi1 = std::min(xi + 1, sh);
        int yi1 = std::min(yi + 1, sh);

        if (nx) {
            xi1 = -xi1;
            xi = -xi;
        }
        if (ny) {
            yi1 = -yi1;
            yi = -yi;
        }

        float bl = src[yi + sh][xi + sh];
        float br = src[yi + sh][xi1 + sh];
        float tl = src[yi1 + sh][xi + sh];
        float tr = src[yi1 + sh][xi1 + sh];

        // interpolate
        float b = xf * br + (1.f - xf) * bl;
        float t = xf * tr + (1.f - xf) * tl;
        float pxf = yf * t + (1.f - yf) * b;
        return pxf;
    };

    float s = float(sw) / float(w);

    double sum = 0;
    for (int y = -h; y <= h; ++y) {
        for (int x = -h; x <= h; ++x) {
            float v = get(y * s, x * s);
            k[y + h][x + h] = v;
            sum += v;
        }
    }

    if (sum >= 1e-5f) {
        for (int y = 0; y < w; ++y) {
            for (int x = 0; x < w; ++x) {
                k[y][x] /= sum;
            }
        }
    } else {
        for (int y = 0; y < w; ++y) {
            for (int x = 0; x < w; ++x) {
                k[y][x] = 0;
            }
        }
        k[w / 2][w / 2] = 1;
    }
}

bool flip_kernel(array2D<float> &kernel)
{
    const int k = kernel.width();
    bool res = false;

    float magnitude = std::abs(kernel[k / 2][k / 2]);
    for (int y = 0; y < k; ++y) {
        for (int x = 0; x < k; ++x) {
            magnitude = std::max(magnitude, std::abs(kernel[y][x]));
        }
    }

    for (int y = 0; y < k; ++y) {
        for (int x = 0; x < k; ++x) {
            float d = std::abs(kernel[y][x] - kernel[k - 1 - y][k - 1 - x]) /
                      magnitude;
            if (d > 1e-5f) {
                res = true;
                kernel[y][x] = kernel[k - 1 - y][k - 1 - x];
            }
        }
    }

    return res;
}

void rl_deconvolution_psf(float **luminance, float **blend, int W, int H,
                          const SharpeningParams &shparam,
                          const ImProcData &data, ProgressListener *plistener)
{
    const Glib::ustring &psf_file = shparam.psf_kernel;
    int iterations = shparam.psf_iterations;

    std::shared_ptr<array2D<float>> kernel_ptr;

    auto &key = psf_file;
    if (!rl_kernel_cache.get(key, kernel_ptr)) {
        StdImageSource src;
        std::unique_ptr<cJSON, decltype(&cJSON_Delete)> jobj(nullptr,
                                                             cJSON_Delete);
        int err = src.load(psf_file);

        if (err) {
            FILE *src = g_fopen(psf_file.c_str(), "rb");
            if (src) {
                std::ostringstream data;
                int c;
                while (true) {
                    c = fgetc(src);
                    if (c != EOF) {
                        data << static_cast<unsigned char>(c);
                    } else {
                        break;
                    }
                }
                fclose(src);
                std::string s = data.str();
                jobj.reset(cJSON_Parse(s.c_str()));
            }
            err = !jobj;
        }

        if (err) {
            if (plistener) {
                plistener->error(Glib::ustring::compose(
                    M("TP_SHARPENING_LABEL") + " - " + M("ERROR_MSG_FILE_READ"),
                    psf_file.empty() ? "(" + M("GENERAL_NONE") + ")"
                                     : psf_file));
            }
            return;
        }

        kernel_ptr = std::make_shared<array2D<float>>();

        bool ok = false;
        if (jobj) {
            ok = import_kernel(jobj.get(), *kernel_ptr);
        } else {
            ImageIO *img = src.getImageIO();
            if (Image8 *im8 = dynamic_cast<Image8 *>(img)) {
                ok = import_kernel(im8, *kernel_ptr);
            } else if (Image16 *im16 = dynamic_cast<Image16 *>(img)) {
                ok = import_kernel(im16, *kernel_ptr);
            } else if (Imagefloat *imf = dynamic_cast<Imagefloat *>(img)) {
                ok = import_kernel(imf, *kernel_ptr);
            } else {
                ok = false;
            }
        }

        if (!ok) {
            if (plistener) {
                plistener->error(
                    Glib::ustring::compose(M("TP_SHARPENING_LABEL") + " - " +
                                               M("ERROR_MSG_INVALID_PSF"),
                                           psf_file));
            }
            return;
        }

        rl_kernel_cache.set(key, kernel_ptr);
    }

    float scale = data.scale;
    int kw = int(kernel_ptr->width() / scale) | 1;
    if (kw < 3) {
        return;
    }

    array2D<float> kernel(kw, kw);
    rescale_kernel(*kernel_ptr, kernel);

    array2D<float> lum(W, H);
    array2D<float> tmp(W, H);
    array2D<float> out(W, H);

#ifdef _OPENMP
#pragma omp parallel for if (data.multiThread)
#endif
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            luminance[y][x] = std::max(luminance[y][x], 0.f);
            lum[y][x] = luminance[y][x];
            out[y][x] = RT_NAN;
        }
    }

    Convolution conv(kernel, W, H, data.multiThread);
    Convolution *flipconv = &conv;
    std::unique_ptr<Convolution> flipconv_ptr;
    if (flip_kernel(kernel)) {
        flipconv_ptr.reset(new Convolution(kernel, W, H, data.multiThread));
        flipconv = flipconv_ptr.get();
    }

    LUTf loglut(65536);
    for (int i = 1; i < 65536; ++i) {
        float x = float(i) / 65535.f;
        loglut[i] = xlogf(x);
    }
    const auto get_log = [&](float x) -> float {
        if (x > 1.f && x <= 65535.f) {
            return loglut[x];
        } else if (x > 1e-5f) {
            return xlogf(x / 65535.f);
        } else {
            return -RT_INFINITY_F;
        }
    };

    const auto get_output = [&](int i, int j) -> float {
        if (UNLIKELY(std::isnan(lum[i][j]))) {
            return luminance[i][j];
        }
        return intp(blend[i][j], std::max(lum[i][j], 0.0f), luminance[i][j]);
    };

    constexpr float delta_factor = 0.3f;

    const auto check_stop = [&](int y, int x) -> void {
        if (LIKELY(std::isnan(out[y][x]))) {
            float l = get_log(luminance[y][x]);
            float l2 = get_log(lum[y][x]);
            if (UNLIKELY(std::abs(l2 - l) > delta_factor)) {
                out[y][x] = get_output(y, x);
            }
        }
    };

    for (int i = 0; i < iterations; ++i) {
        conv(lum, tmp);

#ifdef _OPENMP
#pragma omp parallel for if (data.multiThread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                if (tmp[y][x] > 1e-5f) {
                    tmp[y][x] = luminance[y][x] / tmp[y][x];
                    assert(std::isfinite(tmp[y][x]));
                }
            }
        }

        (*flipconv)(tmp, tmp);

#ifdef _OPENMP
#pragma omp parallel for if (data.multiThread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                lum[y][x] *= tmp[y][x];
                assert(std::isfinite(tmp[y][x]));
                assert(std::isfinite(lum[y][x]));

                check_stop(y, x);
            }
        }
    }

#ifdef _OPENMP
#pragma omp parallel for if (data.multiThread)
#endif
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            float l = out[y][x];
            if (std::isnan(l)) {
                l = get_output(y, x);
            }
            assert(std::isfinite(l));
            luminance[y][x] = std::max(l, 0.f);
        }
    }
}

} // namespace

bool ImProcFunctions::doSharpening(Imagefloat *rgb,
                                   const SharpeningParams &sharpenParam,
                                   bool showMask)
{
    const int W = rgb->getWidth();
    const int H = rgb->getHeight();

    if ((!sharpenParam.enabled) || sharpenParam.amount < 1 || W < 8 || H < 8) {
        return false;
    }

    rgb->setMode(Imagefloat::Mode::RGB, multiThread);
    array2D<float> Y(ARRAY2D_ALIGNED);
    TMatrix ws = ICCStore::getInstance()->workingSpaceMatrix(rgb->colorSpace());
    get_luminance(rgb, Y, ws, multiThread);

    float s_scale = std::sqrt(scale);
    float contrast = pow_F(sharpenParam.contrast / 100.f, 1.2f) * s_scale;
    JaggedArray<float> blend(W, H);
    buildBlendMask(Y, blend, W, H, contrast, 1.f, false, 2.f / s_scale);

    if (showMask) {
        float **r = rgb->r.ptrs;
        float **g = rgb->g.ptrs;
        float **b = rgb->b.ptrs;
#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif
        for (int i = 0; i < H; ++i) {
            for (int j = 0; j < W; ++j) {
                r[i][j] = g[i][j] = b[i][j] = blend[i][j] * 65536.f;
            }
        }

        return true;
    }

    std::unique_ptr<JaggedArray<char>> impulse;
    if (sharpenParam.method == "rld") {
        impulse.reset(new JaggedArray<char>(W, H));
        markImpulse(W, H, Y, *impulse, 2.f);
    }

    array2D<float> YY(W, H, Y, ARRAY2D_ALIGNED);

    if (sharpenParam.method == "rld") {
        double sigma = sharpenParam.deconvradius / scale;
        float amount = sharpenParam.deconvamount / 100.f;
        float delta = sharpenParam.deconvCornerBoost / scale;
        if (delta > 0.01f) {
            array2D<float> YY2(W, H, Y, ARRAY2D_ALIGNED);
            deconvsharpening(YY, blend, *impulse, W, H, sigma, amount,
                             multiThread);
            deconvsharpening(YY2, blend, *impulse, W, H, sigma + delta, amount,
                             multiThread);
            int fw = full_width > 0 ? full_width : W;
            int fh = full_height > 0 ? full_height : H;
            CornerBoostMask mask(offset_x, offset_y, fw, fh,
                                 sharpenParam.deconvCornerLatitude);
#ifdef _OPENMP
#pragma omp parallel for if (multiThread)
#endif
            for (int y = 0; y < H; ++y) {
                for (int x = 0; x < W; ++x) {
                    float blend = mask(x, y);
                    YY[y][x] = intp(blend, YY2[y][x], YY[y][x]);
                }
            }
        } else {
            deconvsharpening(YY, blend, *impulse, W, H, sigma, amount,
                             multiThread);
        }
    } else if (sharpenParam.method == "psf") {
        ImProcData data(params, scale, multiThread);
        rl_deconvolution_psf(YY, blend, W, H, sharpenParam, data, plistener);
    } else {
        unsharp_mask(YY, blend, W, H, sharpenParam, scale, multiThread);
    }
    multiply(rgb, YY, Y, multiThread);

    return false;
}

bool ImProcFunctions::sharpening(Imagefloat *img)
{
    return doSharpening(img, params->sharpening, show_sharpening_mask);
}

bool ImProcFunctions::prsharpening(Imagefloat *img)
{
    return doSharpening(img, params->prsharpening, false);
}

} // namespace rtengine
