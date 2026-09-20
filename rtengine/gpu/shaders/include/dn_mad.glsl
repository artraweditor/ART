/* Shared tail of the exact-MAD median interpolation, used by
 * dn_mad_finalize.comp and dn_mad_finalize_cf.comp.
 *
 * MadRgb (ipdenoise.cc:123-125) ends with
 *     return ((median - 1) + (datalen/2 - count_) / (float)(count - count_))
 *            / 0.6745;
 * where `0.6745` is a *double* literal.  The numerator is a float, so C
 * promotes it, divides in double, and narrows the result on return.  A plain
 * float division by float(0.6745) does not reproduce that: measured over 24
 * subbands of real wavelet coefficients, 3 came back wrong, by up to 8 ulp.
 *
 * So correct for it.  With c = HI + LO the exact float pair for
 * double(0.6745), one Newton step on q0 = x/HI removes the HI-vs-c error:
 * q = q0 + (r - q0*LO)/HI, where r = x - HI*q0.  That takes the wrong
 * subbands from 3 to 1 and the worst error from 8 ulp to exactly 1.
 *
 * It does not get to zero, and on this backend it cannot.  The step needs
 * `r` to be the *exact* residual, since r/HI is the larger of the two
 * corrections; fma() delivers that only if the compiler treats it as a
 * single fused operation, which GLSL guarantees only for results consumed by
 * a `precise` variable.  glslc emits **no NoContraction decorations at all**
 * for this shader even with every intermediate marked precise (checked in
 * the SPIR-V), so float contraction here is entirely at the driver's
 * discretion.  A Dekker two-product, which needs no fma guarantee but does
 * need each operation to round separately, was tried and measures
 * identically -- 1 subband, 1 ulp -- for three times the code, so this
 * simpler form is preferred.
 *
 * The one surviving case is a near-tie: the true quotient sits 0.0145 ulp
 * from the midpoint between two floats, so rounding it correctly needs the
 * correction to ~1% accuracy.  What the failure is *not* is a porting error:
 * the GPU's answer for that subband is exactly float(numerator/float(0.6745))
 * for the CPU's own bit-identical numerator, which is to say the bins, the
 * cumulative search and the interpolation all agree and only the last
 * rounding of the constant division differs.  `run wavmad` checks that
 * property for every subband rather than taking it on trust. */

#define ART_MAD_HI 0.67449998855590820  /* float(0.6745)           */
#define ART_MAD_LO 1.14440918963510e-08 /* float(0.6745 - HI)      */

/* value / 0.6745, rounded as if the division had been done in double --
 * exactly, except in near-ties (see above). */
float art_mad_scale(float x)
{
    float q0 = x / float(ART_MAD_HI);
    float r = fma(-float(ART_MAD_HI), q0, x);
    return q0 + fma(-q0, float(ART_MAD_LO), r) / float(ART_MAD_HI);
}
