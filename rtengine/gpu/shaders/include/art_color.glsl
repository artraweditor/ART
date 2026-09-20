/* Colour-space helpers shared by the GPU operators.
 *
 * Faithful translations of rtengine/color.{h,cc}.  Where ART uses a sleef
 * approximation (xatan2f, xsincosf, xexpf) the GLSL builtin is used instead:
 * both approximate the same function, and the "GPU may be the new reference"
 * decision permits the difference -- but it is a real difference, so anything
 * built on these must be diffed against the CPU.
 *
 * Matrix convention: ART's dot_product is row-major, res[i] = sum_k a[i][k]*b[k]
 * (linalgebra.h:216), so a matrix arrives as three row vectors.
 */
#ifndef ART_COLOR_GLSL
#define ART_COLOR_GLSL

vec3 art_mat3_row_mul(vec4 r0, vec4 r1, vec4 r2, vec3 v)
{
    return vec3(dot(r0.xyz, v), dot(r1.xyz, v), dot(r2.xyz, v));
}

/* LUT<float> lookup with a FLOAT index.
 *
 * Beware: LUT has both operator[](int) (truncating, LUT.h:308) and a templated
 * operator[](V) for floating-point V that LINEARLY INTERPOLATES (LUT.h:471).
 * A float argument selects the interpolating one, and every call site that
 * matters passes a float -- truncating instead quantises the result to the
 * sample below, which is a visible error wherever only a small part of the
 * table's index range is ever addressed.
 *
 * Semantics reproduced here, from LUT.h:471 with upperBound = size-1 and
 * maxs = size-2:
 *   index < 0 or NaN : CLIP_BELOW ? data[0]          : idx = 0
 *   index > size-2   : CLIP_ABOVE ? data[size-1]     : idx = size-2
 *   otherwise        : lerp(data[idx], data[idx+1], index - idx)
 *
 * GLSL cannot pass a buffer to a function, so each shader defines its own
 * wrapper over its own binding (dn_noisevar_prep.comp's `lut_lookup`, over
 * NoiseCurve's rawData(), is the live example); this comment is the single
 * specification they all follow.
 */

/* Color::XYZ2Lab / Color::rgb2lab, color.h:691 and color.cc:219-240.
 *
 * ART bakes the CIE piecewise companding function into a 65536-entry LUT
 * (Color::cachef/cachefy) indexed by the raw XYZ value (roughly [0,65535]) and
 * truncated to the nearest integer bucket. Reproduced here as the continuous
 * closed form instead -- algebraically identical at every integer sample
 * (cachef[i] = 327.68*f(i/65535), cachefy[i] = 116*cachef[i] - 16*327.68,
 * verified against color.cc:219-240), and strictly more precise off-lattice,
 * which the "GPU may be the new reference" policy permits. */
float art_xyz2lab_f(float v)
{
    const float eps = 216.0 / 24389.0;       // Color::eps
    const float kappa = 24389.0 / 27.0;      // Color::kappa
    float t = v / 65535.0;
    return t > eps ? 327.68 * pow(t, 1.0 / 3.0)
                   : 327.68 * ((kappa * t + 16.0) / 116.0);
}

void art_xyz2lab(vec3 xyz, out float L, out float a, out float b)
{
    float fx = art_xyz2lab_f(xyz.x);
    float fy = art_xyz2lab_f(xyz.y);
    float fz = art_xyz2lab_f(xyz.z);
    L = 116.0 * fy - 16.0 * 327.68;
    a = 500.0 * (fx - fy);
    b = 200.0 * (fy - fz);
}

/* Color::Lab2XYZ (color.cc:1266) followed by nothing else -- the inverse of
 * art_xyz2lab above, sharing its policy of evaluating the closed form rather
 * than reproducing ART's LUT.  Color::f2xyz's cube is exact, so unlike the
 * forward direction there is no approximation to argue about; only the
 * epsilon comparison has to match (epsilonExpInv3f = cbrt(216/24389)).
 *
 * Outputs are in ART's 0..65535 XYZ scale, and L/a/b are in ART's scaled
 * units (L in [0, 32768], a/b roughly +-42000), matching the CPU. */
float art_f2xyz(float f)
{
    const float epsilonExpInv3 = 6.0 / 29.0;   // Color::epsilonExpInv3f
    const float kappaInv = 27.0 / 24389.0;     // Color::kappaInvf
    return f > epsilonExpInv3 ? f * f * f : (116.0 * f - 16.0) * kappaInv;
}

vec3 art_lab2xyz(float L, float a, float b)
{
    const float D50x = 0.9642;
    const float D50z = 0.8249;
    const float epskap = 8.0;
    const float kappa = 24389.0 / 27.0;

    const float LL = L / 327.68;
    const float aa = a / 327.68;
    const float bb = b / 327.68;
    const float fy = LL / 116.0 + 16.0 / 116.0;
    const float fx = 0.002 * aa + fy;
    const float fz = fy - 0.005 * bb;

    return vec3(65535.0 * art_f2xyz(fx) * D50x,
                LL > epskap ? 65535.0 * fy * fy * fy : 65535.0 * LL / kappa,
                65535.0 * art_f2xyz(fz) * D50z);
}

/* sleef.h xlin2log: log(x*(base-1)+1) / log(base). */
float art_xlin2log(float x, float base)
{
    return log(x * (base - 1.0) + 1.0) / log(base);
}

/* sleef.h xlog2lin: (pow(base,x)-1) / (base-1) -- xlin2log's inverse. */
float art_xlog2lin(float x, float base)
{
    return (pow(base, x) - 1.0) / (base - 1.0);
}

#endif // ART_COLOR_GLSL
