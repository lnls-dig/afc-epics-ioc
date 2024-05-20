#include <stdexcept>

#include <aSubRecord.h>
#include <menuFtype.h>

#include <registryFunction.h>
#include <epicsExport.h>

template <class T>
static inline void apply_fn(void *v, epicsInt16 ft, T fn)
{
    if (ft == menuFtypeSHORT) fn((epicsInt16 *)v);
    else if (ft == menuFtypeLONG) fn((epicsInt32 *)v);
    else if (ft == menuFtypeINT64) fn((epicsInt64 *)v);
    else if (ft == menuFtypeFLOAT) fn((epicsFloat32 *)v);
    else if (ft == menuFtypeDOUBLE) fn((epicsFloat64 *)v);
    else throw std::runtime_error("unimplemented type");
}

template <class T>
static inline void get_scalar (T &out, void *v, epicsInt16 ft)
{
    apply_fn(v, ft, [&out](auto rv){ out = *rv; });
}

/** This array subroutine is used to apply gain and offset to elements in an
 * array, given the following formula:
 *
 *   out = (in + pre_offset) * gain + post_offset
 *
 * This function expects as input:
 * - array in A
 * - pre_offset in B
 * - gain in C
 * - post_offset in D
 *
 * And will write its output to:
 * - array in OUTA
 */
static long asub_goff(aSubRecord *prec)
{
    /* sanity checking: these input elements were provided */
    if (prec->neb != 1 || prec->nec != 1 || prec->ned != 1 || prec->nee != 1)
        return 1;
    /* sanity checking: the output array is big enough */
    if (prec->nova < prec->nea)
        return 2;

    const epicsUInt32 elements = prec->nea;
    /* write NORD field in output */
    prec->neva = elements;

    double pre_offset, gain, post_offset;
    get_scalar(pre_offset, prec->b, prec->ftb);
    get_scalar(gain, prec->c, prec->ftc);
    get_scalar(post_offset, prec->d, prec->ftd);

    auto apply_goff = [pre_offset, gain, post_offset, elements](auto out, auto in) {
        for (epicsUInt32 i = 0; i < elements; i++)
            out[i] = (in[i] + pre_offset) * gain + post_offset;
    };

    /* run apply_goff regardless of the types of INPA and OUTA,
     * without having to convert anything */
    apply_fn(prec->a, prec->fta,
        [&prec, apply_goff](auto in) {
            apply_fn(prec->vala, prec->ftva, [in, apply_goff](auto out) { apply_goff(out, in); });
        });

    return 0;
}

/** This array subroutine is used to calculate BPM position using readings from
 * BPM ADCs.
 *
 * This function expects as input:
 * - antennas A,B,C,D in fields A,B,C,D, respectively
 * - offset and gain for X in fields E,F, respectively
 * - offset and gain for Y in fields G,H, respectively
 * - gain for Sum in field I
 * - offset and gain for Q in fields J,K, respectively
 *
 * And will write its output to:
 * - X,Y,Sum,Q in OUTA,OUTB,OUTC,OUTD, respectively
 */
static long asub_pos_calc(aSubRecord *prec)
{
    /* sanity checking: these input elements were provided */
    if (prec->nea < 1 || prec->neb < 1 || prec->nec < 1 || prec->ned < 1 ||
        prec->nee != 1 || prec->nef != 1 || prec->neg != 1 || prec->neh != 1 ||
        prec->nei != 1 || prec->nej != 1 || prec->nek != 1)
        return 1;
    /* sanity checking: these have the same length */
    if (prec->neb != prec->nea || prec->nec != prec->nea || prec->ned != prec->nea)
        return 2;
    /* sanity checking: data types */
    if (prec->fta != menuFtypeLONG || prec->ftb != menuFtypeLONG || prec->ftc != menuFtypeLONG || prec->ftd != menuFtypeLONG ||
        prec->ftva != menuFtypeDOUBLE || prec->ftvb != menuFtypeDOUBLE)
        return 3;
    /* sanity checking: the output arrays are big enough */
    if (prec->nova < prec->nea || prec->novb < prec->nea || prec->novc < prec->nea || prec->novd < prec->nea)
        return 4;

    const epicsUInt32 elements = prec->nea;

    /* write NORD field in output */
    prec->neva = elements;
    prec->nevb = elements;

    double x_off, x_gain, y_off, y_gain, s_gain, q_off, q_gain;
    get_scalar(x_off, prec->e, prec->fte);
    get_scalar(x_gain, prec->f, prec->ftf);
    get_scalar(y_off, prec->g, prec->ftg);
    get_scalar(y_gain, prec->h, prec->fth);
    get_scalar(s_gain, prec->i, prec->fti);
    get_scalar(q_off, prec->j, prec->ftj);
    get_scalar(q_gain, prec->k, prec->ftk);

    auto *a = (epicsInt32 *)prec->a;
    auto *b = (epicsInt32 *)prec->b;
    auto *c = (epicsInt32 *)prec->c;
    auto *d = (epicsInt32 *)prec->d;

    auto *x = (epicsFloat64 *)prec->vala;
    auto *y = (epicsFloat64 *)prec->valb;
    auto *s = (epicsFloat64 *)prec->valc;
    auto *q = (epicsFloat64 *)prec->vald;

    for (epicsUInt32 i = 0; i < elements; i++) {
        auto goff = [](auto raw, auto off, auto gain) {
            return raw * gain - off;
        };

        /* partial delta-sigma implementation */
        double s_ac = a[i] + c[i];
        double s_bd = b[i] + d[i];

        double s_ab = a[i] + b[i];
        double s_cd = c[i] + d[i];

        auto raw_s = s_ac + s_bd;
        s[i] = goff(raw_s, 0, s_gain);

        /* don't divide by 0 */
        if (s_ac != 0 && s_bd != 0) {
            double d_ac = a[i] - c[i];
            double d_bd = b[i] - d[i];

            auto partial_ac = d_ac / s_ac;
            auto partial_bd = d_bd / s_bd;
            auto raw_x = .5 * (partial_ac - partial_bd);
            auto raw_y = .5 * (partial_ac + partial_bd);

            x[i] = goff(raw_x, x_off, x_gain);
            y[i] = goff(raw_y, y_off, y_gain);
        }

        /* don't divide by 0 */
        if (s_ab != 0 && s_cd != 0) {
            double d_ab = a[i] - b[i];
            double d_cd = c[i] - d[i];

            auto partial_ab = d_ab / s_ab;
            auto partial_cd = d_cd / s_cd;
            auto raw_q = .5 * (partial_ab + partial_cd);
            q[i] = goff(raw_q, q_off, q_gain);
        }
    }

    return 0;
}

epicsRegisterFunction(asub_goff);
epicsRegisterFunction(asub_pos_calc);
