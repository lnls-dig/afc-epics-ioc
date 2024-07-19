#include <stdexcept>
#include <limits>

#include <aSubRecord.h>
#include <menuFtype.h>

#include <registryFunction.h>
#include <epicsExport.h>

static_assert(std::numeric_limits<double>::has_quiet_NaN);

const auto mynan = std::numeric_limits<double>::quiet_NaN();

template <class T>
static inline void apply_fn(void *v, epicsInt16 ft, T fn)
{
    if (ft == menuFtypeSHORT) fn((epicsInt16 *)v);
    else if (ft == menuFtypeLONG) fn((epicsInt32 *)v);
    else if (ft == menuFtypeINT64) fn((epicsInt64 *)v);
    else if (ft == menuFtypeFLOAT) fn((epicsFloat32 *)v);
    else if (ft == menuFtypeDOUBLE) fn((epicsFloat64 *)v);
    else if (ft == menuFtypeENUM) fn((epicsEnum16 *)v);
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

static inline void pos_apply_poly(double x1, double y1, const double *coeff_x, const double *coeff_y, double &out_x, double &out_y)
{
    double coeffx1y0_x = coeff_x[0];
    double coeffx1y2_x = coeff_x[1];
    double coeffx1y4_x = coeff_x[2];
    double coeffx1y6_x = coeff_x[3];
    double coeffx1y8_x = coeff_x[4];
    double coeffx3y0_x = coeff_x[5];
    double coeffx3y2_x = coeff_x[6];
    double coeffx3y4_x = coeff_x[7];
    double coeffx3y6_x = coeff_x[8];
    double coeffx5y0_x = coeff_x[9];
    double coeffx5y2_x = coeff_x[10];
    double coeffx5y4_x = coeff_x[11];
    double coeffx7y0_x = coeff_x[12];
    double coeffx7y2_x = coeff_x[13];
    double coeffx9y0_x = coeff_x[14];

    double coeffx0y1_y = coeff_y[0];
    double coeffx2y1_y = coeff_y[1];
    double coeffx4y1_y = coeff_y[2];
    double coeffx6y1_y = coeff_y[3];
    double coeffx8y1_y = coeff_y[4];
    double coeffx0y3_y = coeff_y[5];
    double coeffx2y3_y = coeff_y[6];
    double coeffx4y3_y = coeff_y[7];
    double coeffx6y3_y = coeff_y[8];
    double coeffx0y5_y = coeff_y[9];
    double coeffx2y5_y = coeff_y[10];
    double coeffx4y5_y = coeff_y[11];
    double coeffx0y7_y = coeff_y[12];
    double coeffx2y7_y = coeff_y[13];
    double coeffx0y9_y = coeff_y[14];

    double y2 = y1*y1;
    double y3 = y2*y1;
    double y4 = y2*y2;
    double y5 = y3*y2;
    double y6 = y4*y2;
    double y7 = y4*y3;
    double y8 = y6*y2;
    double y9 = y6*y3;
    double x2 = x1*x1;
    double x3 = x2*x1;
    double x4 = x2*x2;
    double x5 = x3*x2;
    double x6 = x4*x2;
    double x7 = x5*x2;
    double x8 = x6*x2;
    double x9 = x6*x3;

    out_x =
        x1*(coeffx1y0_x + coeffx1y2_x*y2  + coeffx1y4_x*y4  + coeffx1y6_x*y6  + coeffx1y8_x*y8) +
        x3*(coeffx3y0_x + coeffx3y2_x*y2  + coeffx3y4_x*y4  + coeffx3y6_x*y6) +
        x5*(coeffx5y0_x + coeffx5y2_x*y2  + coeffx5y4_x*y4) +
        x7*(coeffx7y0_x + coeffx7y2_x*y2) +
        x9*(coeffx9y0_x);
    out_y =
        y1*(coeffx0y1_y + coeffx2y1_y*x2  + coeffx4y1_y*x4  + coeffx6y1_y*x6  + coeffx8y1_y*x8) +
        y3*(coeffx0y3_y + coeffx2y3_y*x2  + coeffx4y3_y*x4  + coeffx6y3_y*x6) +
        y5*(coeffx0y5_y + coeffx2y5_y*x2  + coeffx4y5_y*x4) +
        y7*(coeffx0y7_y + coeffx2y7_y*x2) +
        y9*(coeffx0y9_y);
}

static inline void sum_apply_poly(double x1, double y1, double s1, const double *coeff_sum, double &out_sum)
{
    double coeffx0y0_sum = coeff_sum[0];
    double coeffx0y2_sum = coeff_sum[1];
    double coeffx0y4_sum = coeff_sum[2];
    double coeffx0y6_sum = coeff_sum[3];
    double coeffx2y0_sum = coeff_sum[4];
    double coeffx2y2_sum = coeff_sum[5];
    double coeffx2y4_sum = coeff_sum[6];
    double coeffx2y6_sum = coeff_sum[7];
    double coeffx4y0_sum = coeff_sum[8];
    double coeffx4y2_sum = coeff_sum[9];
    double coeffx4y4_sum = coeff_sum[10];
    double coeffx4y6_sum = coeff_sum[11];
    double coeffx6y0_sum = coeff_sum[12];
    double coeffx6y2_sum = coeff_sum[13];
    double coeffx6y4_sum = coeff_sum[14];
    double coeffx6y6_sum = coeff_sum[15];

    double y2 = y1*y1;
    double y4 = y2*y2;
    double y6 = y4*y2;
    double x2 = x1*x1;
    double x4 = x2*x2;
    double x6 = x4*x2;

    double divisor =
        1 *(coeffx0y0_sum + coeffx0y2_sum*y2 + coeffx0y4_sum*y4 + coeffx0y6_sum*y6) +
        x2*(coeffx2y0_sum + coeffx2y2_sum*y2 + coeffx2y4_sum*y4 + coeffx2y6_sum*y6) +
        x4*(coeffx4y0_sum + coeffx4y2_sum*y2 + coeffx4y4_sum*y4 + coeffx4y6_sum*y6) +
        x6*(coeffx6y0_sum + coeffx6y2_sum*y2 + coeffx6y4_sum*y4 + coeffx6y6_sum*y6);

    if(divisor == 0.)
        out_sum = mynan;
    else
        out_sum = s1 / divisor;
}

static inline void q_apply_poly(double x1, double y1, double q1, const double *coeff_q, double &out_q)
{
    double coeffx1y1_q = coeff_q[0];
    double coeffx1y3_q = coeff_q[1];
    double coeffx1y5_q = coeff_q[2];
    double coeffx3y1_q = coeff_q[3];
    double coeffx3y3_q = coeff_q[4];
    double coeffx3y5_q = coeff_q[5];
    double coeffx5y1_q = coeff_q[6];
    double coeffx5y3_q = coeff_q[7];
    double coeffx5y5_q = coeff_q[8];

    double y2 = y1*y1;
    double y4 = y2*y2;
    double x2 = x1*x1;
    double x3 = x2*x1;
    double x5 = x3*x2;

    out_q =
        q1 -
        x1*y1*(coeffx1y1_q + coeffx1y3_q*y2 + coeffx1y5_q*y4) +
        x3*y1*(coeffx3y1_q + coeffx3y3_q*y2 + coeffx3y5_q*y4) +
        x5*y1*(coeffx5y1_q + coeffx5y3_q*y2 + coeffx5y5_q*y4);
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
 * - enable XY,Sum,Q polynomial correction in fields L,M,N, respectively (optional)
 * - X,Y,Sum,Q polynomial coefficients in fields O,P,Q,R respectively (optional)
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

    epicsUInt8 xy_poly = 0, sum_poly = 0, q_poly = 0;
    if (prec->l)
        get_scalar(xy_poly, prec->l, prec->ftl);
    if (prec->m)
        get_scalar(sum_poly, prec->m, prec->ftm);
    if (prec->n)
        get_scalar(q_poly, prec->n, prec->ftn);

    /* sanity checking: polynomial coefficients */
    if (xy_poly && (prec->fto != menuFtypeDOUBLE || prec->ftp != menuFtypeDOUBLE || prec->neo != 15 || prec->nep != 15))
        return 5;
    if (sum_poly && (prec->ftq != menuFtypeDOUBLE || prec->neq != 16))
        return 6;
    if (q_poly && (prec->ftr != menuFtypeDOUBLE || prec->ner != 9))
        return 7;

    const epicsUInt32 elements = prec->nea;

    /* write NORD field in output */
    prec->neva = elements;
    prec->nevb = elements;
    prec->nevc = elements;
    prec->nevd = elements;

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

    auto *coeff_x = (epicsFloat64 *)prec->o;
    auto *coeff_y = (epicsFloat64 *)prec->p;
    auto *coeff_sum = (epicsFloat64 *)prec->q;
    auto *coeff_q = (epicsFloat64 *)prec->r;

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

        /* don't divide by 0 */
        if (s_ac != 0 && s_bd != 0) {
            double d_ac = a[i] - c[i];
            double d_bd = b[i] - d[i];

            auto partial_ac = d_ac / s_ac;
            auto partial_bd = d_bd / s_bd;
            const auto raw_x = .5 * (partial_ac - partial_bd);
            const auto raw_y = .5 * (partial_ac + partial_bd);

            /* sum and q polynomial correction should use the raw value for x and y */
            auto raw_x_corr = raw_x, raw_y_corr = raw_y;

            if (xy_poly)
                pos_apply_poly(raw_x, raw_y, coeff_x, coeff_y, raw_x_corr, raw_y_corr);

            x[i] = goff(raw_x_corr, x_off, x_gain);
            y[i] = goff(raw_y_corr, y_off, y_gain);

            if (sum_poly)
                sum_apply_poly(raw_x, raw_y, raw_s, coeff_sum, raw_s);

            s[i] = goff(raw_s, 0, s_gain);

            /* don't divide by 0 */
            if (s_ab != 0 && s_cd != 0) {
                double d_ab = a[i] - b[i];
                double d_cd = c[i] - d[i];

                auto partial_ab = d_ab / s_ab;
                auto partial_cd = d_cd / s_cd;
                auto raw_q = .5 * (partial_ab + partial_cd);

                if (q_poly)
                    q_apply_poly(raw_x, raw_y, raw_q, coeff_q, raw_q);

                q[i] = goff(raw_q, q_off, q_gain);
            } else {
                q[i] = mynan;
            }
        } else {
            x[i] = y[i] = s[i] = q[i] = mynan;
        }
    }

    return 0;
}

epicsRegisterFunction(asub_goff);
epicsRegisterFunction(asub_pos_calc);
