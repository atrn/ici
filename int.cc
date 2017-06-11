#define ICI_CORE
#include "fwd.h"
#include "int.h"
#include "primes.h"

namespace ici
{

ici_int_t                   *ici_small_ints[ICI_SMALL_INT_COUNT];

/*
 * Return the int object with the value 'v'.  The returned object has had its
 * ref count incremented.  Returns NULL on error, usual convention.  Note that
 * ints are intrinsically atomic, so if the given integer already exists, it
 * will just incref it and return it.
 *
 * Note, 0 and 1 are available directly as 'ici_zero' and 'ici_one'.
 *
 * This --func-- forms part of the --ici-api--.
 */
ici_int_t *
ici_int_new(long i)
{
    ici_obj_t           *o;
    ici_obj_t           **po;

    if ((i & ~ICI_SMALL_INT_MASK) == 0 && (o = ici_small_ints[i]) != NULL)
    {
        ici_incref(o);
        return ici_intof(o);
    }
    for
    (
        po = &ici_atoms[ici_atom_hash_index((unsigned long)i * INT_PRIME)];
        (o = *po) != NULL;
        --po < ici_atoms ? po = ici_atoms + ici_atomsz - 1 : NULL
    )
    {
        if (ici_isint(o) && ici_intof(o)->i_value == i)
        {
            ici_incref(o);
            return ici_intof(o);
        }
    }
    ++ici_supress_collect;
    if ((o = ici_talloc(ici_int_t)) == NULL)
    {
        --ici_supress_collect;
        return NULL;
    }
    ICI_OBJ_SET_TFNZ(o, ICI_TC_INT, ICI_O_ATOM, 1, sizeof(ici_int_t));
    ici_rego(o);
    ici_intof(o)->i_value = i;
    --ici_supress_collect;
    ICI_STORE_ATOM_AND_COUNT(po, o);
    return ici_intof(o);
}

class int_type : public type
{
public:
    int_type() : type("int") {}

    /*
     * Mark this and referenced unmarked objects, return memory costs.
     * See comments on t_mark() in object.h.
     */
    unsigned long
    mark(ici_obj_t *o) override
    {
        o->o_flags |= ICI_O_MARK;
        return sizeof(ici_int_t);
    }

    /*
     * Returns 0 if these objects are equal, else non-zero.
     * See the comments on t_cmp() in object.h.
     */
    int
    cmp(ici_obj_t *o1, ici_obj_t *o2) override
    {
        return ici_intof(o1)->i_value != ici_intof(o2)->i_value;
    }

    /*
     * Return a hash sensitive to the value of the object.
     * See the comment on t_hash() in object.h
     */
    unsigned long
    hash(ici_obj_t *o) override
    {
        /*
         * There are in-line versions of this in object.c and binop.h.
         */
        return (unsigned long)ici_intof(o)->i_value * INT_PRIME;
    }

    /*
     * Free this object and associated memory (but not other objects).
     * See the comments on t_free() in object.h.
     */
    void free(ici_obj_t *o) override
    {
        ici_tfree(o, ici_int_t);
    }


};

} // namespace ici
