#define ICI_CORE
#include "fwd.h"
#include "float.h"
#include "primes.h"
#include "archiver.h"
#include <assert.h>

namespace ici
{

/*
 * Return an ICI float object corresponding to the given value 'v'.  Note that
 * floats are intrinsically atomic.  The returned object will have had its
 * reference count inceremented. Returns NULL on error, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
ici_float *new_float(double v)
{
    ici_float          *f;
    object           **po;
    static ici_float   proto;

    proto.f_value = v;
    if ((f = floatof(atom_probe2(&proto, &po))) != NULL) {
        f->incref();
        return f;
    }
    ++supress_collect;
    if ((f = ici_talloc(ici_float)) == NULL) {
        return NULL;
    }
    set_tfnz(f, TC_FLOAT, object::O_ATOM, 1, sizeof (ici_float));
    f->f_value = v;
    rego(f);
    --supress_collect;
    store_atom_and_count(po, f);
    return f;
}

int float_type::cmp(object *o1, object *o2)
{
    assert(sizeof (double) == 2 * sizeof (int32_t));
    return !DBL_BIT_CMP(&floatof(o1)->f_value, &floatof(o2)->f_value);
}

unsigned long float_type::hash(object *o)
{
    return hash_float(floatof(o)->f_value);
}

unsigned long hash_float(double v)
{
    unsigned long       h;
    int                 i;

    h = FLOAT_PRIME;
    /*
     * We assume that the compiler will decide this constant expression
     * at compile time and not actually make a run-time decision about
     * which bit of code to run.
     *
     * WARNING: there is an in-line expansion of this in binop.h.
     */
    if (sizeof v == 2 * sizeof (int32_t)) {
        /*
         * The little dance of getting the address of the double into
         * a pointer to ulong via void * is brought to you by the
         * aliasing rules of ISO C.
         */
        void            *vp;
        int32_t         *p;

        vp = &v;
        p = (int32_t *)vp;
        h += p[0] + p[1] * 31;
        h ^= (h >> 12) ^ (h >> 24);
    } else {
        unsigned char   *p;

        p = (unsigned char *)&v;
        i = sizeof v;
        while (--i >= 0)
            h = *p++ + h * 31;
    }
    return h;
}

int float_type::save(archiver *ar, object *obj) {
    return ar->write(floatof(obj)->f_value);
}

object *float_type::restore(archiver *ar) {
    double val;
    if (ar->read(val)) {
        return NULL;
    }
    return new_float(val);
}

} // namespace ici
