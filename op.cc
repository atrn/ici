#define ICI_CORE
#include "fwd.h"
#include "op.h"
#include "exec.h"
#include "primes.h"

namespace ici
{

ici_op_t *
ici_new_op(int (*func)(), int ecode, int code)
{
    ici_op_t            *o;
    ici_obj_t           **po;
    static ici_op_t     proto = {ICI_TC_OP};

    proto.op_func = func;
    proto.op_code = code;
    proto.op_ecode = ecode;
    if ((o = ici_opof(ici_atom_probe2(&proto, &po))) != NULL)
    {
        ici_incref(o);
        return o;
    }
    ++ici_supress_collect;
    if ((o = ici_talloc(ici_op_t)) == NULL)
        return NULL;
    ICI_OBJ_SET_TFNZ(o, ICI_TC_OP, ICI_O_ATOM, 1, sizeof(ici_op_t));
    o->op_code = code;
    o->op_ecode = ecode;
    o->op_func = func;
    ici_rego(o);
    --ici_supress_collect;
    ICI_STORE_ATOM_AND_COUNT(po, o);
    return o;
}

class op_type : public type
{
public:
    op_type() : type("op") {}

    /*
     * Mark this and referenced unmarked objects, return memory costs.
     * See comments on t_mark() in object.h.
     */
    unsigned long mark(ici_obj_t *o) override
    {
        o->o_flags |= ICI_O_MARK;
        return sizeof(ici_op_t);
    }

    /*
     * Returns 0 if these objects are equal, else non-zero.
     * See the comments on t_cmp() in object.h.
     */
    int cmp(ici_obj_t *o1, ici_obj_t *o2) override
    {
        return ici_opof(o1)->op_func != ici_opof(o2)->op_func
        || ici_opof(o1)->op_code != ici_opof(o2)->op_code
        || ici_opof(o1)->op_ecode != ici_opof(o2)->op_ecode;
    }

    /*
     * Return a hash sensitive to the value of the object.
     * See the comment on t_hash() in object.h
     */
    unsigned long hash(ici_obj_t *o) override
    {
        return OP_PRIME * ((unsigned long)ici_opof(o)->op_func
                           + ici_opof(o)->op_code
                           + ici_opof(o)->op_ecode);
    }

    /*
     * Free this object and associated memory (but not other objects).
     * See the comments on t_free() in object.h.
     */
    void free(ici_obj_t *o) override
    {
        ici_tfree(o, ici_op_t);
    }

};

} // namespace ici
