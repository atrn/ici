#define ICI_CORE
#include "op.h"
#include "archiver.h"
#include "exec.h"
#include "fwd.h"
#include "primes.h"

namespace ici
{

op *new_op(int (*func)(), int16_t ecode, int16_t code)
{
    op       *o;
    object  **po;
    static op proto(TC_OP);

    proto.op_func = func;
    proto.op_code = code;
    proto.op_ecode = ecode;
    if (auto x = atom_probe2(&proto, &po))
    {
        o = opof(x);
        incref(o);
        return o;
    }
    ++supress_collect;
    if ((o = ici_talloc(op)) == nullptr)
    {
        --supress_collect;
        return nullptr;
    }
    set_tfnz(o, TC_OP, object::O_ATOM, 1, sizeof(op));
    o->op_code = code;
    o->op_ecode = ecode;
    o->op_func = func;
    rego(o);
    --supress_collect;
    store_atom_and_count(po, o);
    return o;
}

/*
 * Returns 0 if these objects are equal, else non-zero.
 * See the comments on t_cmp() in object.h.
 */
int op_type::cmp(object *o1, object *o2)
{
    return opof(o1)->op_func != opof(o2)->op_func || opof(o1)->op_code != opof(o2)->op_code ||
           opof(o1)->op_ecode != opof(o2)->op_ecode;
}

/*
 * Return a hash sensitive to the value of the object.
 * See the comment on t_hash() in object.h
 */
unsigned long op_type::hash(object *o)
{
    return OP_PRIME * ((unsigned long)opof(o)->op_func + opof(o)->op_code + opof(o)->op_ecode);
}

int op_type::save(archiver *ar, object *o)
{
    auto    op = opof(o);
    int16_t f = ar->op_func_code(op->op_func);
    int16_t ecode = op->op_ecode;
    int16_t code = op->op_code;
    if (ar->write(f))
    {
        return 1;
    }
    if (ar->write(ecode))
    {
        return 1;
    }
    if (ar->write(code))
    {
        return 1;
    }
    return 0;
}

object *op_type::restore(archiver *ar)
{
    int16_t op_func_code, op_ecode, op_code;
    if (ar->read(&op_func_code))
    {
        return nullptr;
    }
    if (ar->read(&op_ecode))
    {
        return nullptr;
    }
    if (ar->read(&op_code))
    {
        return nullptr;
    }
    auto ofunc = archiver::op_func(op_func_code);
    if (op_func_code && !ofunc)
    {
        set_error("bad op func");
        return nullptr;
    }
    return new_op(ofunc, op_ecode, op_code);
}

} // namespace ici
