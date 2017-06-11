#define ICI_CORE
#include "fwd.h"
#include "float.h"
#include "primes.h"
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

class float_type : public type
{
public:
    float_type() : type("float") {}

    unsigned long mark(ici_obj_t *o) override
    {
        o->o_flags |= ICI_O_MARK;
        return sizeof (ici_float_t);
    }

    void free(ici_obj_t *o) override
    {
        ici_tfree(o, ici_float_t);
    }

    int cmp(ici_obj_t *o1, ici_obj_t *o2) override
    {
        assert(sizeof(double) == 2 * sizeof(int32_t));
        return !DBL_BIT_CMP(&ici_floatof(o1)->f_value, &ici_floatof(o2)->f_value);
    }

    unsigned long hash(ici_obj_t *o) override
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
        if (sizeof ici_floatof(o)->f_value == 2 * sizeof(unsigned long))
        {
            /*
             * The little dance of getting the address of the double into
             * a pointer to ulong via void * is brought to you by the
             * aliasing rules of ISO C.
             */
            void            *vp;
            unsigned long   *p;

            vp = &ici_floatof(o)->f_value;
            p = (unsigned long *)vp;
            h += p[0] + p[1] * 31;
            h ^= (h >> 12) ^ (h >> 24);
        }
        else
        {
            unsigned char   *p;

            p = (unsigned char *)&ici_floatof(o)->f_value;
            i = sizeof(ici_floatof(o)->f_value);
            while (--i >= 0)
                h = *p++ + h * 31;
        }
        return h;
    }
};

} // namespace ici
