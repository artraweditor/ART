/* -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright (c) 2026 Alberto Griggio <alberto.griggio@gmail.com>
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

#include "iccfloatpipeline.h"

#include "LUT3D.h"
#include "iccstore.h"
#include "linalgebra.h"
#include "rt_math.h"
#include "settings.h"

#include <lcms2_plugin.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <utility>
#include <vector>

namespace art { namespace engine {

extern const Settings *settings;

namespace {

constexpr int BLOCK = 256; // pixels per pass over one stage
// See close_enough()
constexpr double TOLERANCE = 2e-6;
constexpr double PIXEL_TOLERANCE = 1e-5;
constexpr double PIXEL_FLOOR = 1e-12;
// lcms2's PLUS_INF and MINUS_INF are +-1e22: finite values, used as the
// ends of the first and last curve segment and as the result outside them
constexpr double LCMS_INF = 1e22;

// Says why the fast path is not used (with verbose output); always false.
bool decline(const char *why)
{
    if (settings->verbose) {
        std::cout << "ICCFloatPipeline: not used: " << why << std::endl;
    }
    return false;
}

// Move d representable float steps from x (down if d < 0).
float next(float x, int d)
{
    const float inf = std::numeric_limits<float>::infinity();
    for (; d < 0; ++d) {
        x = std::nextafter(x, -inf);
    }
    for (; d > 0; --d) {
        x = std::nextafter(x, inf);
    }
    return x;
}

// Whether the three channels of a pixel agree. For every channel
//   abs(mine - ref) <= TOLERANCE * max(1, abs(ref))
// and
//   abs(mine - ref) <= PIXEL_TOLERANCE * (largest abs(ref) of the pixel)
//                      + PIXEL_FLOOR
// The second condition keeps dark pixels to a small error in proportion to
// their size; it is relative to the pixel and not to the channel because a
// channel can be near zero by cancellation. Non-finite values never agree.
bool close_enough(const float ref[3], const float mine[3])
{
    double big = 0;
    for (int c = 0; c < 3; ++c) {
        if (!std::isfinite(ref[c]) || !std::isfinite(mine[c])) {
            return false;
        }
        big = std::max(big, std::fabs(double(ref[c])));
    }
    for (int c = 0; c < 3; ++c) {
        const double error = std::fabs(double(ref[c]) - double(mine[c]));
        if (error > TOLERANCE * std::max(1.0, std::fabs(double(ref[c]))) ||
            error > PIXEL_TOLERANCE * big + PIXEL_FLOOR) {
            return false;
        }
    }
    return true;
}

/*
 * Curve evaluator with a fast path for the formula of function type 0 of the
 * formulaCurveSegment of multiProcessElements:
 *     Y = (a * X + b)^gamma + c
 * lcms2 represents this formula as segment Type 6. Other supported curves use
 * cmsEvalToneCurveFloat().
 */
class Curve {
public:
    explicit Curve(const cmsToneCurve *tc = nullptr)
        : tc_(tc), usable_(tc != nullptr), power_(false), gamma1_(false),
          gamma_(1.0)
    {
#if LCMS_VERSION >= 2160
        // A curve given as a table has no segments: seg_ stays empty and
        // eval() hands it to cmsEvalToneCurveFloat() (16 bit, reentrant).
        bool formulas = true; // all segments are Type 6, and not too many
        for (int i = 0; tc; ++i) {
            const cmsCurveSegment *s = cmsGetToneCurveSegment(i, tc);
            if (!s) {
                break;
            }
            // lcms2 mutates interpolation state when it evaluates a sampled
            // segment (Type 0). Reject those: this evaluator is shared by
            // the worker threads. Types that lcms2 does not define, and
            // non-finite parameters, are rejected too.
            const int n = parameters(s->Type);
            usable_ = usable_ && n > 0 && std::isfinite(s->x0) &&
                      std::isfinite(s->x1);
            for (int k = 0; k < n; ++k) {
                usable_ = usable_ && std::isfinite(s->Params[k]);
            }
            // powf() takes a float exponent: only use it where that is exact
            // (ICC files store float32 parameters)
            formulas = formulas && s->Type == 6 && i < MAX_SEGMENTS &&
                       double(float(s->Params[0])) == s->Params[0];
            if (formulas && usable_) {
                Segment g = {s->x0,        s->x1,        s->Params[0],
                             s->Params[1], s->Params[2], s->Params[3]};
                seg_.push_back(g);
            }
        }
        if (!formulas) {
            seg_.clear();
        }
        // Recognize the common piecewise curve: X for X <= 0, X^gamma above.
        power_ = seg_.size() == 2 && seg_[0].x0 == -float(LCMS_INF) &&
                 seg_[0].x1 == 0.f && seg_[1].x0 == 0.f &&
                 seg_[1].x1 == float(LCMS_INF) && seg_[0].gamma == 1.0 &&
                 seg_[0].a == 1.0 && seg_[0].b == 0.0 && seg_[0].c == 0.0 &&
                 seg_[1].gamma > 0 && seg_[1].gamma != 1.0 &&
                 seg_[1].a == 1.0 && seg_[1].b == 0.0 && seg_[1].c == 0.0;
        gamma_ = power_ ? seg_[1].gamma : 1.0;
        // A gamma curve of the matrix/shaper profiles, one segment of Type 1
        // (Y = X^gamma), over the whole domain
        const cmsCurveSegment *s0 =
            tc ? cmsGetToneCurveSegment(0, tc) : nullptr;
        if (usable_ && s0 && !cmsGetToneCurveSegment(1, tc) && s0->Type == 1 &&
            s0->x0 == -float(LCMS_INF) && s0->x1 == float(LCMS_INF) &&
            s0->Params[0] > 0 &&
            double(float(s0->Params[0])) == s0->Params[0]) {
            gamma1_ = true;
            gamma_ = s0->Params[0];
        }
#else
        // Segments cannot be inspected. Only a single parametric segment of a
        // known type is known not to be sampled. Its parameters cannot be
        // read; a non-finite one gives non-finite results, which the
        // comparison with lcms2 rejects.
        usable_ = usable_ && parameters(cmsGetToneCurveParametricType(tc)) > 0;
#endif
    }

    // Whether this curve can be used by the fast path.
    bool usable() const { return usable_; }

    // Appends segment ends and the zero of a * X + b, each with its
    // neighbouring floats.
    void breakpoints(std::vector<float> &out) const
    {
        for (size_t i = 0; i < seg_.size(); ++i) {
            const Segment &s = seg_[i];
            add_around(out, s.x0); // open end: a gap would show here
            add_around(out, s.x1);
            if (s.a != 0 && s.gamma != 1.0) {
                add_around(out, float(-s.b / s.a));
            }
        }
    }

    float eval(float v) const
    {
        // The ends of the domain are open below and closed above, as in
        // EvalSegmentedFn(); anything outside goes the general way.
        if (gamma1_ && v > -float(LCMS_INF) && v <= float(LCMS_INF)) {
            if (v < 0.f) { // as lcms2 does for Type 1
                return std::fabs(gamma_ - 1.0) < MATRIX_DET_TOLERANCE ? v : 0.f;
            }
            const double out = power(v, gamma_);
            // powf() can overflow, and lose a denormal result, where lcms2's
            // pow() does not
            return std::isfinite(out) && (v == 0.f || out >= FLT_MIN)
                       ? float(out)
                       : cmsEvalToneCurveFloat(tc_, v);
        }
        if (power_) {
            if (v > 0.f && v <= float(LCMS_INF)) {
                const double out = power(v, gamma_);
                // powf() can overflow where lcms2's pow() does not
                return std::isfinite(out) ? float(out)
                                          : cmsEvalToneCurveFloat(tc_, v);
            }
            if (v <= 0.f && v > -float(LCMS_INF)) {
                return v;
            }
        }
        if (seg_.empty()) {
            return cmsEvalToneCurveFloat(tc_, v);
        }
        // as EvalSegmentedFn(): the last segment with x0 < X <= x1 decides
        const double R = v;
        for (size_t i = seg_.size(); i-- > 0;) {
            const Segment &s = seg_[i];
            if (R > s.x0 && R <= s.x1) {
                const double e = s.a * R + s.b;
                double out;
                if (s.gamma == 1.0) {
                    out = e + s.c;
                } else if (e < 0) {
                    out = s.c;
                } else {
                    out = power(e, s.gamma) + s.c;
                }
                return std::isfinite(out) ? float(out)
                                          : cmsEvalToneCurveFloat(tc_, v);
            }
        }
        // outside every segment, lcms2 returns MINUS_INF (-1e22f)
        return float(-LCMS_INF);
    }

private:
    static constexpr int MAX_SEGMENTS = 8;
    static constexpr double MATRIX_DET_TOLERANCE = 0.0001; // lcms2's

    struct Segment {
        float x0, x1;
        double gamma, a, b, c;
    };

    // Parameter count of the parametric types lcms2 defines; 0 for sampled
    // segments and unknown types.
    static int parameters(int type)
    {
        switch (type < 0 ? -type : type) {
        case 1:
        case 108:
        case 109:
            return 1;
        case 2:
            return 3;
        case 3:
        case 6:
            return 4;
        case 4:
        case 7:
        case 8:
            return 5;
        case 5:
            return 7;
        default:
            return 0;
        }
    }

    static void add_around(std::vector<float> &out, float x)
    {
        if (std::isfinite(x) && std::fabs(x) < float(LCMS_INF)) {
            out.push_back(next(x, -1));
            out.push_back(x);
            out.push_back(next(x, 1));
        }
    }

    static double power(double x, double g)
    {
        if (g == 1.0) {
            return x;
        }
        if (g == 3.0) {
            return SQR(x) * x;
        }
        if (g == 2.0) {
            return SQR(x);
        }
        const float fx = float(x);
        if (double(fx) == x) { // the usual case: the input itself
            return std::pow(fx, float(g));
        }
        return std::pow(x, g);
    }

    const cmsToneCurve *tc_;
    bool usable_, power_, gamma1_;
    double gamma_;
    std::vector<Segment> seg_;
};

/* the nodes of the CLUT of a float tag, for LUT3D */
class TableInit: public LUT3D::initializer {
public:
    explicit TableInit(const float *table): p_(table), finite_(true) {}

    void operator()(float &r, float &g, float &b) override
    {
        r = p_[0];
        g = p_[1];
        b = p_[2];
        p_ += 3;
        finite_ =
            finite_ && std::isfinite(r) && std::isfinite(g) && std::isfinite(b);
    }

    bool finite() const { return finite_; }

private:
    const float *p_;
    bool finite_;
};

struct Stage {
    enum Type { CURVES, CLUT, MATRIX };

    Stage(): type(MATRIX), has_offset(false) { o[0] = o[1] = o[2] = 0; }

    void run(float *p, int n) const
    {
        switch (type) {
        case CURVES:
            for (int k = 0; k < n; ++k, p += 3) {
                p[0] = curves[0].eval(p[0]);
                p[1] = curves[1].eval(p[1]);
                p[2] = curves[2].eval(p[2]);
            }
            break;
        case CLUT:
            // lcms2 sets inputs below 1e-9, and NaN, to 0 before it
            // interpolates; LUT3D only clamps to 0..1.
            for (int k = 0; k < n; ++k, p += 3) {
                for (int i = 0; i < 3; ++i) {
                    if (!(p[i] >= 1.0e-9f)) {
                        p[i] = 0.f;
                    }
                }
                (*clut)(p[0], p[1], p[2]);
            }
            break;
        default:
            // Accumulate in double and round to float, as lcms2 does. It adds
            // the offset only if the matrix has one.
            if (has_offset) {
                for (int k = 0; k < n; ++k, p += 3) {
                    const double r = p[0], g = p[1], b = p[2];
                    for (int i = 0; i < 3; ++i) {
                        p[i] = float(m[i][0] * r + m[i][1] * g + m[i][2] * b +
                                     o[i]);
                    }
                }
            } else {
                for (int k = 0; k < n; ++k, p += 3) {
                    const double r = p[0], g = p[1], b = p[2];
                    for (int i = 0; i < 3; ++i) {
                        p[i] = float(m[i][0] * r + m[i][1] * g + m[i][2] * b);
                    }
                }
            }
            break;
        }
    }

    // Make this matrix stage apply `first`, then itself; neither has an
    // offset.
    void after(const Stage &first) { m = dot_product(m, first.m); }

    Type type;
    Curve curves[3];
    std::shared_ptr<LUT3D> clut;
    Mat33<double> m;
    double o[3];
    bool has_offset; // the original offset pointer was set, even if all zeros
};

bool read_curves(cmsHPROFILE prof, const cmsToneCurve *out[3])
{
    const cmsTagSignature tags[3] = {cmsSigRedTRCTag, cmsSigGreenTRCTag,
                                     cmsSigBlueTRCTag};
    for (int c = 0; c < 3; ++c) {
        out[c] = static_cast<const cmsToneCurve *>(cmsReadTag(prof, tags[c]));
        if (!out[c]) {
            return false;
        }
    }
    return true;
}

// The colorant matrix of a matrix/shaper profile in double, as lcms2 reads it
// (ICCStore::getProfileMatrix() narrows it to float). That function decides
// whether the profile is a matrix/shaper one.
bool read_matrix(cmsHPROFILE prof, Mat33<double> &out)
{
    Mat33<float> m;
    if (!ICCStore::getProfileMatrix(prof, m)) {
        return false;
    }
    const cmsTagSignature tags[3] = {cmsSigRedMatrixColumnTag,
                                     cmsSigGreenMatrixColumnTag,
                                     cmsSigBlueMatrixColumnTag};
    for (int j = 0; j < 3; ++j) {
        const cmsCIEXYZ *c =
            static_cast<const cmsCIEXYZ *>(cmsReadTag(prof, tags[j]));
        if (!c) {
            return false;
        }
        out[0][j] = c->X;
        out[1][j] = c->Y;
        out[2][j] = c->Z;
    }
    return true;
}

// A single parametric segment X^1, which lcms2 evaluates as the identity for
// all X in its domain (above -1e22, up to 1e22), negative ones included.
bool is_identity_curve(const cmsToneCurve *tc)
{
#if LCMS_VERSION >= 2160
    const cmsCurveSegment *s = cmsGetToneCurveSegment(0, tc);
    return s && !cmsGetToneCurveSegment(1, tc) && s->Type == 1 &&
           s->Params[0] == 1.0;
#else
    // Without segment access, restrict this to a single gamma curve and
    // require exact identity results at representative positive and negative
    // inputs.
    if (cmsGetToneCurveParametricType(tc) != 1) {
        return false;
    }
    static const float x[] = {-1.f,    -0.37f, -1e-6f, 0.f,  1e-6f,
                              0.0123f, 0.3f,   0.5f,   0.9f, 1.f};
    for (size_t i = 0; i < sizeof(x) / sizeof(x[0]); ++i) {
        if (cmsEvalToneCurveFloat(tc, x[i]) != x[i]) {
            return false;
        }
    }
    return true;
#endif
}

// lcms2 reads the output tables B2Ax/B2Dx of the intent, or of the perceptual
// one, before it falls back to the matrix and curves. Whatever the intent,
// a profile that has any of them is not used.
bool has_output_tables(cmsHPROFILE prof)
{
    for (int i = 0; i < 3; ++i) {
        if (cmsIsTag(prof, cmsTagSignature(cmsSigBToA0Tag + i))) {
            return true;
        }
    }
    for (int i = 0; i < 4; ++i) {
        if (cmsIsTag(prof, cmsTagSignature(cmsSigBToD0Tag + i))) {
            return true;
        }
    }
    return false;
}

bool finite(const Mat33<double> &m)
{
    bool ok = true;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            ok = ok && std::isfinite(m[i][j]);
        }
    }
    return ok;
}

// Deterministic probe value generator.
class Rng {
public:
    Rng(unsigned seed): s_(seed) {}

    float operator()() // 0 <= x < 1
    {
        s_ = s_ * 1664525u + 1013904223u;
        return float(s_ >> 8) / float(1 << 24);
    }

    int below(int n) { return int((*this)() * float(n)); }

private:
    unsigned s_;
};

// Compare a stage with a duplicated lcms2 stage (`src`) using synthetic inputs
// in that stage's coordinate space. Include curve boundaries, CLUT grid and
// interpolation boundaries, clamp thresholds, and random values.
bool stage_matches(cmsContext ctx, const Stage &st, const cmsStage *src)
{
    // cmsPipelineInsertStage() links the stage before it checks the channels,
    // so once both exist the pipeline owns the copy, whatever the result;
    // freeing the stage as well would free it twice.
    cmsStage *copy = cmsStageDup(const_cast<cmsStage *>(src));
    cmsPipeline *pipe = cmsPipelineAlloc(ctx, 3, 3);
    if (!copy || !pipe || !cmsPipelineInsertStage(pipe, cmsAT_BEGIN, copy)) {
        if (pipe) {
            cmsPipelineFree(pipe);
        } else if (copy) {
            cmsStageFree(copy);
        }
        return false;
    }

    std::vector<float> special; // values to use for the channels
    special.push_back(0.f);
    special.push_back(1.f);
    special.push_back(next(1.f, 1));
    special.push_back(next(1.f, -1));
    for (int d = -2; d <= 2; ++d) {
        special.push_back(next(0.f, d));
        special.push_back(next(1e-9f, d)); // the clamp of the CLUT
    }
    if (st.type == Stage::CURVES) {
        for (int i = 0; i < 3; ++i) {
            st.curves[i].breakpoints(special);
        }
        const float t[] = {1e-30f, 1e-20f, 1e-10f, 1e-6f, 1e-3f, 0.5f};
        for (int i = 0; i < 6; ++i) {
            special.push_back(t[i]);
            special.push_back(-t[i]);
        }
    } else if (st.type == Stage::CLUT) {
        const int dim = st.clut->dimension();
        for (int k = 0; k < dim; ++k) {
            for (int d = -3; d <= 3; ++d) {
                special.push_back(next(float(k) / float(dim - 1), d));
            }
        }
    }

    Rng rnd(777u);
    const int swept = 3 * int(special.size());
    const int N = swept + 4099;
    std::vector<float> in(3 * N);
    for (int i = 0; i < N; ++i) {
        float *p = &in[3 * i];
        for (int c = 0; c < 3; ++c) {
            p[c] = rnd() < 0.5f ? special[rnd.below(int(special.size()))]
                                : rnd() * 1.14f - 0.1f;
        }
        if (i < swept) { // each special value through each channel
            p[i % 3] = special[i / 3];
        } else if (i % 4 ==
                   1) { // two or three channels equal, or a float apart
            p[1] = next(p[0], rnd.below(3) - 1);
            p[2] = i % 8 == 1 ? p[0] : p[2];
        } else if (i % 4 == 2) {
            p[2] = next(p[1], rnd.below(3) - 1);
        }
    }

    std::vector<float> mine = in;
    st.run(mine.data(), N);
    bool ok = true;
    for (int i = 0; i < N && ok; ++i) {
        float ref[3];
        cmsPipelineEvalFloat(&in[3 * i], ref, pipe);
        ok = close_enough(ref, &mine[3 * i]);
        if (!ok && settings->verbose) {
            std::cout << "ICCFloatPipeline: the "
                      << (st.type == Stage::CURVES ? "curves"
                          : st.type == Stage::CLUT ? "CLUT"
                                                   : "matrix")
                      << " stage differs from lcms2 at " << in[3 * i] << " "
                      << in[3 * i + 1] << " " << in[3 * i + 2] << ": " << ref[0]
                      << " " << ref[1] << " " << ref[2] << " against "
                      << mine[3 * i] << " " << mine[3 * i + 1] << " "
                      << mine[3 * i + 2] << std::endl;
        }
    }
    cmsPipelineFree(pipe);
    return ok;
}

} // namespace

struct ICCFloatPipeline::Impl {
    std::vector<Stage> stages;

    void apply(float *rgb, int W) const
    {
        for (int off = 0; off < W; off += BLOCK) {
            float *p = rgb + 3 * off;
            const int n = std::min(BLOCK, W - off);
            for (size_t i = 0; i < stages.size(); ++i) {
                stages[i].run(p, n);
            }
            // The inverse curves of the output profile, dropped as identities,
            // return MINUS_INF for NaN and anything outside the domain of
            // their segment.
            const float limit = float(LCMS_INF);
            for (int k = 0; k < 3 * n; ++k) {
                p[k] = (p[k] > -limit && p[k] <= limit) ? p[k] : -limit;
            }
        }
    }

    // Fuse adjacent matrices only when neither had an offset pointer, and
    // drop a result that is the identity to 1e-5. lcms2 does the same in
    // PreOptimize() (_MultiplyMatrix()), which runs even with
    // cmsFLAGS_NOOPTIMIZE.
    void add(Stage st)
    {
        if (st.type == Stage::MATRIX && !st.has_offset && !stages.empty() &&
            stages.back().type == Stage::MATRIX && !stages.back().has_offset) {
            st.after(stages.back());
            stages.pop_back();
            bool identity = true;
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    identity =
                        identity &&
                        std::fabs(st.m[i][j] - (i == j ? 1.0 : 0.0)) < 0.00001;
                }
            }
            if (!identity) {
                stages.push_back(std::move(st));
            }
        } else {
            stages.push_back(std::move(st));
        }
    }

    // The D2Bx tag that lcms2 reads for this intent.
    bool add_float_tag(cmsHPROFILE in, cmsUInt32Number intent)
    {
        const cmsPipeline *lut = static_cast<const cmsPipeline *>(
            cmsReadTag(in, cmsTagSignature(cmsSigDToB0Tag + intent)));
        if (!lut || cmsPipelineInputChannels(lut) != 3 ||
            cmsPipelineOutputChannels(lut) != 3) {
            return decline("the float tag is not 3 to 3 channels");
        }
        for (const cmsStage *s = cmsPipelineGetPtrToFirstStage(lut); s;
             s = cmsStageNext(s)) {
            if (cmsStageInputChannels(s) != 3 ||
                cmsStageOutputChannels(s) != 3) {
                return decline("a stage is not 3 to 3 channels");
            }
            Stage st;
            switch (cmsStageType(s)) {
            case cmsSigCurveSetElemType: {
                const _cmsStageToneCurvesData *d =
                    static_cast<const _cmsStageToneCurvesData *>(
                        cmsStageData(s));
                st.type = Stage::CURVES;
                for (int i = 0; i < 3; ++i) {
                    st.curves[i] = Curve(d->TheCurves[i]);
                    if (!st.curves[i].usable()) {
                        return decline("a curve has sampled segments, an "
                                       "unsupported type, or non-finite "
                                       "parameters");
                    }
                }
            } break;
            case cmsSigCLutElemType: {
                const _cmsStageCLutData *d =
                    static_cast<const _cmsStageCLutData *>(cmsStageData(s));
                const cmsInterpParams *ip = d->Params;
                if (!d->HasFloatValues || !d->Tab.TFloat || !ip ||
                    ip->nInputs != 3 || ip->nOutputs != 3 ||
                    ip->nSamples[0] < 2 || ip->nSamples[1] != ip->nSamples[0] ||
                    ip->nSamples[2] != ip->nSamples[0] ||
                    (ip->dwFlags & CMS_LERP_FLAGS_TRILINEAR)) {
                    return decline("the CLUT is not a cubic tetrahedral "
                                   "float one");
                }
                st.type = Stage::CLUT;
                st.clut.reset(new LUT3D());
                TableInit init(d->Tab.TFloat);
                st.clut->init(int(ip->nSamples[0]), init);
                if (!init.finite()) {
                    return decline("the CLUT has values that are not "
                                   "numbers");
                }
            } break;
            case cmsSigMatrixElemType: {
                const _cmsStageMatrixData *d =
                    static_cast<const _cmsStageMatrixData *>(cmsStageData(s));
                for (int i = 0; i < 3; ++i) {
                    for (int j = 0; j < 3; ++j) {
                        st.m[i][j] = d->Double[3 * i + j];
                    }
                    st.o[i] = d->Offset ? d->Offset[i] : 0.0;
                }
                st.has_offset = d->Offset != nullptr;
                if (!finite(st.m) || !std::isfinite(st.o[0]) ||
                    !std::isfinite(st.o[1]) || !std::isfinite(st.o[2])) {
                    return decline("a matrix has values that are not "
                                   "numbers");
                }
            } break;
            default:
                return decline("a stage of an unknown kind");
            }
            if (!stage_matches(cmsGetProfileContextID(in), st, s)) {
                return decline("a stage differs from lcms2");
            }
            add(st);
        }
        return true;
    }

    // The curves and matrix of a profile without a LUT.
    bool add_matrix_shaper(cmsHPROFILE in, cmsUInt32Number intent)
    {
        Mat33<double> m;
        const cmsToneCurve *tc[3];
        if (cmsIsCLUT(in, intent, LCMS_USED_AS_INPUT) ||
            cmsIsCLUT(in, INTENT_PERCEPTUAL, LCMS_USED_AS_INPUT) ||
            !read_matrix(in, m) || !read_curves(in, tc)) {
            return decline("the input profile is not of a known kind");
        }
        Stage curves;
        curves.type = Stage::CURVES;
        for (int i = 0; i < 3; ++i) {
            curves.curves[i] = Curve(tc[i]);
            if (!curves.curves[i].usable()) {
                return decline("a curve has sampled segments, an "
                               "unsupported type, or non-finite parameters");
            }
        }
        Stage matrix;
        matrix.m = m;
        if (!finite(matrix.m)) {
            return decline("the matrix has non-finite values");
        }
        // lcms2 stages built from the same tags, to check these stages on
        // their own
        const cmsContext ctx = cmsGetProfileContextID(in);
        cmsStage *lcurves =
            cmsStageAllocToneCurves(ctx, 3, const_cast<cmsToneCurve **>(tc));
        double md[9];
        for (int i = 0; i < 9; ++i) {
            md[i] = matrix.m[i / 3][i % 3];
        }
        cmsStage *lmatrix = cmsStageAllocMatrix(ctx, 3, 3, md, nullptr);
        const bool ok = lcurves && lmatrix &&
                        stage_matches(ctx, curves, lcurves) &&
                        stage_matches(ctx, matrix, lmatrix);
        if (lcurves) {
            cmsStageFree(lcurves);
        }
        if (lmatrix) {
            cmsStageFree(lmatrix);
        }
        if (!ok) {
            return decline("a stage differs from lcms2");
        }
        add(curves);
        add(matrix);
        return true;
    }

    // PCS XYZ to the output profile's linear RGB. The XYZ encoding factor of
    // lcms2, MAX_ENCODEABLE_XYZ = 65535/32768, cancels: its output matrix has
    // it as OutpAdj (cmsio1.c), the input side as InpAdj = 1/MAX_ENCODEABLE_XYZ
    // in the matrix of a matrix/shaper profile, or as the normalization stage
    // _cmsStageNormalizeFromXyzFloat() after a float tag (cmslut.c).
    bool add_output(cmsHPROFILE out)
    {
        Mat33<double> md;
        const cmsToneCurve *tc[3];
        if (cmsGetColorSpace(out) != cmsSigRgbData ||
            cmsGetPCS(out) != cmsSigXYZData || has_output_tables(out) ||
            !read_matrix(out, md) || !read_curves(out, tc)) {
            return decline("the output profile is not a matrix/shaper one");
        }
        for (int i = 0; i < 3; ++i) {
            if (!is_identity_curve(tc[i])) {
                return decline("the output profile is not linear");
            }
        }
        Stage st;
        if (!finite(md)) {
            return decline("the output profile has non-finite values");
        }
        if (!inverse(md, st.m) || !finite(st.m)) {
            return decline("the output profile matrix is singular");
        }
        add(st);
        return true;
    }

    bool read(cmsHPROFILE in, cmsHPROFILE out, cmsUInt32Number intent)
    {
        // Absolute colorimetric adapts the white point, and perceptual and
        // saturation set black point compensation for v4 profiles: not done
        if (intent != INTENT_RELATIVE_COLORIMETRIC) {
            return decline("only the relative colorimetric intent is handled");
        }
        if (cmsGetColorSpace(in) != cmsSigRgbData ||
            cmsGetPCS(in) != cmsSigXYZData) {
            return decline("the input profile is not RGB to XYZ");
        }
        // lcms2 prefers the float tag, then the 16 bit ones, then the matrix
        if (cmsIsTag(in, cmsTagSignature(cmsSigDToB0Tag + intent))) {
            if (!add_float_tag(in, intent)) {
                return false;
            }
        } else if (!add_matrix_shaper(in, intent)) {
            return false;
        }
        return add_output(out);
    }

    // End-to-end probes: random RGB, dark and negative values, grays, corners,
    // and breakpoints of the initial curve stage.
    static void probes(std::vector<float> &v, const std::vector<float> &breaks)
    {
        Rng rnd(12345u);
        const int n = int(v.size() / 3);
        for (int i = 0; i < n; ++i) {
            float *p = &v[3 * i];
            const int k = i / 16; // number of this kind of probe
            switch (i % 16) {
            case 6: { // dark
                const float q = rnd() * rnd() * rnd() * 0.01f;
                p[0] = q * rnd();
                p[1] = q * rnd();
                p[2] = q * rnd();
            } break;
            case 7: { // nearly gray
                const float g = rnd();
                p[0] = g;
                p[1] = g * (0.9f + 0.2f * rnd());
                p[2] = g * (0.9f + 0.2f * rnd());
            } break;
            case 8: // exactly gray
                p[0] = p[1] = p[2] = float(k % 257) / 256.f;
                break;
            case 9: // the eight corners in turn
                p[0] = float(k & 1);
                p[1] = float((k >> 1) & 1);
                p[2] = float((k >> 2) & 1);
                break;
            case 10: { // two equal channels, or all three
                const float a = rnd(), c = rnd();
                p[0] = p[1] = p[2] = a;
                switch (k % 4) {
                case 0:
                    p[2] = c;
                    break;
                case 1:
                    p[1] = c;
                    break;
                case 2:
                    p[0] = c;
                    break;
                }
            } break;
            case 11: // where the formulas of the first curves change
                for (int c = 0; c < 3; ++c) {
                    p[c] = breaks.empty()
                               ? rnd()
                               : breaks[rnd.below(int(breaks.size()))];
                }
                break;
            case 12: { // around 0
                static const float t[] = {0.f,    1e-30f, 1e-20f,
                                          1e-10f, 1e-6f,  1e-3f};
                for (int c = 0; c < 3; ++c) {
                    const float x = t[rnd.below(6)];
                    p[c] = next(rnd() < 0.5f ? x : -x, rnd.below(3) - 1);
                }
            } break;
            case 13: // negative, as noise in the dark parts of a raw can be
                for (int c = 0; c < 3; ++c) {
                    p[c] = rnd() < 0.7f ? -rnd() * rnd() * 0.25f : rnd();
                }
                break;
            case 14: // a little outside 0..1
                p[0] = rnd() * 1.04f - 0.02f;
                p[1] = rnd() * 1.04f - 0.02f;
                p[2] = rnd() * 1.04f - 0.02f;
                break;
            default: // a random cube
                p[0] = rnd();
                p[1] = rnd();
                p[2] = rnd();
                break;
            }
        }
    }

    bool matches(cmsHTRANSFORM reference) const
    {
        std::vector<float> breaks;
        if (!stages.empty() && stages[0].type == Stage::CURVES) {
            for (int i = 0; i < 3; ++i) {
                stages[0].curves[i].breakpoints(breaks);
            }
        }
        std::vector<float> in(3 * 8189), ref, mine;
        probes(in, breaks);
        ref = in;
        mine = in;
        cmsDoTransform(reference, ref.data(), ref.data(), int(in.size() / 3));
        apply(mine.data(), int(mine.size() / 3));
        double worst = 0; // largest finite absolute difference
        bool ok = true, has_nonfinite = false;
        const size_t pixels = in.size() / 3;
        size_t first = pixels;
        for (size_t i = 0; i < pixels; ++i) {
            const bool good = close_enough(&ref[3 * i], &mine[3 * i]);
            if (!good && first == pixels) {
                first = i;
            }
            ok = ok && good;
            if (settings->verbose) { // for the messages below only
                for (int c = 0; c < 3; ++c) {
                    const double d = std::fabs(double(ref[3 * i + c]) -
                                               double(mine[3 * i + c]));
                    has_nonfinite = has_nonfinite || !std::isfinite(d);
                    if (std::isfinite(d)) {
                        worst = std::max(worst, d);
                    }
                }
            }
        }
        if (settings->verbose > 1 || (!ok && settings->verbose)) {
            std::cout << "ICCFloatPipeline: largest finite difference to lcms2 "
                      << worst
                      << (has_nonfinite ? ", with non-finite values" : "")
                      << std::endl;
        }
        if (!ok && settings->verbose) {
            std::cout << "ICCFloatPipeline: first validation failure at "
                      << in[3 * first] << " " << in[3 * first + 1] << " "
                      << in[3 * first + 2] << ": lcms2 " << ref[3 * first]
                      << " " << ref[3 * first + 1] << " " << ref[3 * first + 2]
                      << ", here " << mine[3 * first] << " "
                      << mine[3 * first + 1] << " " << mine[3 * first + 2]
                      << std::endl;
        }
        return ok;
    }
};

ICCFloatPipeline::ICCFloatPipeline(): impl_(new Impl()) {}

ICCFloatPipeline::~ICCFloatPipeline() {}

std::unique_ptr<ICCFloatPipeline>
ICCFloatPipeline::create(cmsHPROFILE in, cmsHPROFILE out,
                         cmsUInt32Number intent, cmsHTRANSFORM reference)
{
    const char *env = std::getenv("ART_ICC_FAST");
    if (!in || !out || !reference || (env && std::strcmp(env, "0") == 0)) {
        return nullptr;
    }
    // This reads the profiles and runs the reference transform, all under the
    // lock the caller holds: do not move it out of that lock.
    std::unique_ptr<ICCFloatPipeline> res(new ICCFloatPipeline());
    bool ok = res->impl_->read(in, out, intent);
    if (ok && !res->impl_->matches(reference)) {
        decline("it differs from lcms2");
        ok = false;
    }
    if (!ok) {
        if (settings->verbose) {
            char name[256] = "";
            cmsGetProfileInfoASCII(in, cmsInfoDescription, "en", "US", name,
                                   sizeof(name));
            std::cout << "ICCFloatPipeline: not used for the input profile \""
                      << name << "\"" << std::endl;
        }
        return nullptr;
    }
    if (settings->verbose) {
        std::cout << "ICCFloatPipeline: used for the input profile"
                  << std::endl;
    }
    return res;
}

void ICCFloatPipeline::apply(float *rgb, int W) const { impl_->apply(rgb, W); }

}} // namespace art::engine
