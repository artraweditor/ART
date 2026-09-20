/* Shared geometry for denoise phase 7 (detail_recovery).
 *
 * The block grid is the one denoise::detailRecoveryCPU lays down
 * (ipdenoise.cc:3288): block (vblk, hblk) has its top-left corner at
 * ((vblk - blkrad) * offset, (hblk - blkrad) * offset) and spans TS x TS, so
 * consecutive blocks overlap by TS - offset = 39 px and every image pixel is
 * covered by at most ceil(TS/offset) = 3 blocks in each direction.
 *
 * Both consumers of that grid -- the totwt accumulation and the Ldetail
 * gather -- are pixel-centric on the device where the CPU is block-centric,
 * so they need the inverse mapping: given a pixel, which blocks cover it.
 */

#define TS 64
#define OFFSET 25
#define BLKRAD 1

/* The largest block index whose window still reaches pixel p.  The covering
 * set is exactly {last, last-1, last-2}: block b puts pixel p at index
 * i = p - (b - blkrad)*offset, so b = last gives i in [0, offset),
 * b = last-1 gives i in [offset, 2*offset) and b = last-2 gives i in
 * [2*offset, 3*offset) -- of which only the part below TS lands inside the
 * block.  There is never a fourth, so a loop of three with a bounds test is
 * both minimal and complete. */
#define DN_DCT_LAST_BLK(p) ((p) / OFFSET + BLKRAD)

/* The CPU accumulates both totwt and Ldetail inside a loop coloured by
 * vblk % kPhases (kPhases == 3, ipdenoise.cc:3277) and separated by the
 * implicit barrier at the end of each `omp for`, so for any given pixel the
 * three covering block rows are summed in order of vblk % 3, not in order of
 * vblk.  Only one covering row falls in each phase class -- the three are
 * consecutive -- so the order is deterministic despite the parallel loop, and
 * reproducing it is the difference between a diff that is explainable and one
 * that is not.  It costs one modulo.
 *
 * Within a block row the CPU's hblk loop is plain ascending, so the inner
 * gather needs no such treatment. */
#define DN_DCT_PHASES 3

/* The one member of {last, last-1, last-2} congruent to `ph` modulo kPhases.
 * The three candidates are consecutive, so exactly one matches each residue
 * and iterating ph over 0..2 walks the covering set in the CPU's order
 * without a scan. */
#define DN_DCT_BLK_IN_PHASE(last, ph)                                          \
    ((last) - ((((last) - (ph)) % DN_DCT_PHASES + DN_DCT_PHASES) %             \
               DN_DCT_PHASES))

/* ---------------------------------------------------------------------------
 * Driver hazard, found the hard way while bringing dn_dct_totwt.comp up, and
 * recorded here because it is a property of this backend rather than of this
 * shader.
 *
 * The tilemask profiles were originally one 128-float buffer holding fv in
 * [0,64) and gv in [64,128), read as
 *
 *     const float tin  = prof.v[i]      * prof.v[j]      + eps;
 *     const float tout = prof.v[TS + i] * prof.v[TS + j] + eps;
 *
 * On this device (Apple M4, MoltenVK 1.1.357) the second pair returns the
 * wrong value for some lanes: with i == 11 the load at index 75 read 0.0 while
 * the buffer holds 1.0 there.  It is not a coherency or addressing problem,
 * and the evidence is unusually clean -- in one statement group the shader
 * reported `float(TS + i)` as 75, `prof.v[75]` (constant index) as 1.0, and
 * `prof.v[TS + i]` as 0.0.  Narrowed further:
 *
 *   - reading only prof.v[TS + i] * prof.v[TS + j], with the fv pair removed,
 *     is correct;
 *   - keeping both pairs but putting gv in its own buffer indexed by the
 *     plain `i` is bit-exact over every pixel;
 *   - putting gv in its own buffer but still indexing it [TS + i] fails
 *     identically, as does binding the same buffer twice.
 *
 * So the trigger is having both `[i]` and `[K + i]` in flight for the same
 * dynamic `i`, not which buffer they come from -- consistent with a bad load
 * merge or index strength reduction in the shader compiler.  It went
 * unnoticed because it needs full occupancy: with every lane but one masked
 * off, the same shader computes the right answer, which is also why an
 * accuracy check on a single-pixel probe would have passed.
 *
 * Rule for this backend: do not index one array at two dynamically-computed
 * offsets that differ by a constant.  Use one buffer per table, or stage into
 * shared memory once and index that (which is what dn_dct_block.comp does).
 * ------------------------------------------------------------------------- */
