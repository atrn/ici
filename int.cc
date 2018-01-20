#define ICI_CORE
#include "fwd.h"
#include "int.h"
#include "primes.h"
#include "archiver.h"

namespace ici
{

integer *small_ints[small_int_count];

/*
 * Return the int object with the value 'v'.  The returned object has had its
 * ref count incremented.  Returns NULL on error, usual convention.  Note that
 * ints are intrinsically atomic, so if the given integer already exists, it
 * will just incref it and return it.
 *
 * Note, 0 and 1 are available directly as 'o_zero' and 'o_one'.
 *
 * This --func-- forms part of the --ici-api--.
 */
integer *new_int(int64_t i)
{
    object *o;
    object **po;

    if ((i & ~small_int_mask) == 0 && (o = small_ints[i]) != NULL) {
        o->incref();
        return intof(o);
    }
    for
    (
        po = &atoms[atom_hash_index((unsigned long)i * INT_PRIME)];
        (o = *po) != NULL;
        --po < atoms ? po = atoms + atomsz - 1 : NULL
    )
    {
        if (isint(o) && intof(o)->i_value == i) {
            o->incref();
            return intof(o);
        }
    }
    ++supress_collect;
    if ((o = ici_talloc(integer)) == NULL) {
        --supress_collect;
        return NULL;
    }
    set_tfnz(o, TC_INT, object::O_ATOM, 1, sizeof (integer));
    rego(o);
    intof(o)->i_value = i;
    --supress_collect;
    store_atom_and_count(po, o);
    return intof(o);
}

/*
 * Returns 0 if these objects are equal, else non-zero.
 * See the comments on t_cmp() in object.h.
 */
int int_type::cmp(object *o1, object *o2)
{
    return intof(o1)->i_value != intof(o2)->i_value;
}

/*
 * Return a hash sensitive to the value of the object.
 * See the comment on t_hash() in object.h
 */
unsigned long int_type::hash(object *o)
{
    /*
     * There are in-line versions of this in object.c and binop.h.
     */
    return (unsigned long)intof(o)->i_value * INT_PRIME;
}

int int_type::save(archiver *ar, object *obj) {
    return ar->write(intof(obj)->i_value);
}

object *int_type::restore(archiver *ar) {
    int64_t value;
    if (ar->read(value)) {
        return nullptr;
    }
    return new_int(value);
}

} // namespace ici
