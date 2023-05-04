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

epicsRegisterFunction(asub_goff);
