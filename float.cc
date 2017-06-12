#define ICI_CORE
#include "fwd.h"
#include "float.h"
#include "primes.h"
#include "types.h"
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
ici_float_t *
ici_float_new(double v)
{
    ici_float_t          *f;
    ici_obj_t           **po;
    static ici_float_t   proto;

    proto.f_value = v;
    if ((f = ici_floatof(ici_atom_probe2(&proto, &po))) != NULL)
    {
        ici_incref(f);
        return f;
    }
    ++ici_supress_collect;
    if ((f = ici_talloc(ici_float_t)) == NULL)
        return NULL;
    ICI_OBJ_SET_TFNZ(f, ICI_TC_FLOAT, ICI_O_ATOM, 1, sizeof(ici_float_t));
    f->f_value = v;
    ici_rego(f);
    --ici_supress_collect;
    ICI_STORE_ATOM_AND_COUNT(po, f);
    return f;
}

unsigned long float_type::mark(ici_obj_t *o)
{
    o->o_flags |= ICI_O_MARK;
    return sizeof (ici_float_t);
}

void float_type::free(ici_obj_t *o)
{
    ici_tfree(o, ici_float_t);
}

int float_type::cmp(ici_obj_t *o1, ici_obj_t *o2)
{
    assert(sizeof(double) == 2 * sizeof(int32_t));
    return !DBL_BIT_CMP(&ici_floatof(o1)->f_value, &ici_floatof(o2)->f_value);
}

unsigned long float_type::hash(ici_obj_t *o)
{
    return ici_hash_float(ici_floatof(o)->f_value);
}

unsigned long ici_hash_float(double v)
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
    if (sizeof v == 2 * sizeof(int32_t))
    {
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
    }
    else
    {
        unsigned char   *p;

        p = (unsigned char *)&v;
        i = sizeof v;
        while (--i >= 0)
            h = *p++ + h * 31;
    }
    return h;
}

} // namespace ici
