// -*- mode:c++ -*-

#ifndef ICI_EXEC_H
#define ICI_EXEC_H

#include "array.h"
#include "float.h"
#include "fwd.h"
#include "int.h"
#include "null.h"
#include "pc.h"
#include "src.h"

#include <condition_variable>

namespace ici
{

union ostemp {
    integer   i;
    ici_float f;
};

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

struct exec : object
{
    array  *x_xs;
    array  *x_os;
    array  *x_vs;
    src    *x_src;
    int     x_count;
    int     x_yield_count;
    array  *x_pc_closet;     /* See below. */
    array  *x_os_temp_cache; /* See below. */
    exec   *x_next;
    int     x_n_engine_recurse;
    int     x_critsect;
    object *x_waitfor;
    int     x_state;
    object *x_result;
    char   *x_error;
    /*
     * End of ici.h export. --ici.h-end--
     */
    std::condition_variable *x_semaphore;
    char                     x_buf[1024 - 16 * sizeof(int)]; // space for x_error
    /*
     * The following portion of this file exports to ici.h. --ici.h-start--
     */
};

inline exec *execof(object *o)
{
    return o->as<exec>();
}
inline bool isexec(object *o)
{
    return o->hastype(TC_EXEC);
}

/*
 * x_xs                 The ICI interpreter execution stack. This contains
 *                      objects being executed, which includes 'pc' objects
 *                      (never seen at the language level) that are special
 *                      pointers into code arrays. Entering blocks and
 *                      functions calls cause this stack to grow deeper.
 *                      NB: This stack is swapped into the global varable
 *                      xs for the active thread. Accessing this field
 *                      directly is very rare.
 *
 * x_os                 The ICI interpreter operand stack. This is where
 *                      operands in expressions are stacked during expression
 *                      evaluation (which includes function call argument
 *                      preparation).
 *                      NB: This stack is swapped into the global varable
 *                      os for the active thread. Accessing this field
 *                      directly is very rare.
 *
 * x_vs                 The ICI interpreter 'scope' or 'variable' stack. The
 *                      top element of this stack is always a struct that
 *                      defines the current context for the lookup of variable
 *                      names. Function calls cause this to grow deeper as
 *                      the new scope of the function being entered is pushed
 *                      on the stack.
 *                      NB: This stack is swapped into the global varable
 *                      vs for the active thread. Accessing this field
 *                      directly is very rare.
 *
 * x_count              A count-down until we should check such things as
 *                      whether the above stacks need growing. Various expensive
 *                      tests are delayed and done occasionally to save time.
 *                      NB: This is cached in exec_count for the current
 *                      thread.
 *
 * x_src                The most recently executed source line tag. These tags
 *                      are placed in code arrays during compilation.  They
 *                      are no-ops with respect to execution, but allow us to
 *                      know where we are executing with respect to the
 *                      original source.
 *
 * x_pc_closet          An array that shadows the execution stack. pc objects
 *                      exist only in a one-to-one relationship with a fixed
 *                      (for their life) position on the execution stack. This
 *                      cache holds pc objects that are used whenever we need
 *                      a pc at that slot in the execution stack.
 *
 * x_os_temp_cache      An array of pseudo int/float objects that shadows the
 *                      operand stack.  The objects in this array (apart from
 *                      the nullptrs) are unions of int and float objects that
 *                      can be used as intermediate results in specific
 *                      circumstances as flaged by the compiler.
 *                      Specifically, they are known to be immediately
 *                      consumed by some operator that is only sensitive to
 *                      the value, not the address, of the object.  See
 *                      binop.h.
 *
 * x_next               Link to the next execution context on the list of all
 *                      existing execution contexts.
 *
 * x_n_engine_recurse   A count of the number of times the main interpreter
 *                      has been recursively entered in this thread (which is
 *                      *not* caused by recursion in the user's ICI code).
 *                      Only certain user constructs can cause this recusion
 *                      (recursive parsing for example).  Native machine stack
 *                      overflow is a nasty catastrophic error that we can't
 *                      otherwise detect, so we don't allow too much of this
 *                      sort of thing. See top of evaluate().
 *
 * x_waitfor            If this thread is sleeping, an aggragate object that
 *                      it is waiting to be signaled.  nullptr if it is not
 *                      sleeping.
 */

/*
 * Possible values of for x_state.
 */
enum
{
    XS_ACTIVE,   /* Thread has not exited yet. */
    XS_RETURNED, /* Function returned and thread exited normally. */
    XS_FAILED,   /* Function failed and thread exitied. */
};

/*
 * Test if an object represents a false value nullptr or integer 0.
 */
inline bool isfalse(object *o)
{
    return isnull(o) || o == o_zero;
}

/*
 * End of ici.h export. --ici.h-end--
 */

class exec_type : public type
{
public:
    exec_type() : type("exec", sizeof(struct exec))
    {
    }

    size_t  mark(object *o) override;
    void    free(object *o) override;
    object *fetch(object *o, object *k) override;
};

inline void set_pc(array *code, object **x)
{
    *x = ex->x_pc_closet->a_base[x - xs.a_base];
    auto p = pcof(*x);
    p->pc_code = code;
    p->pc_next = code->a_base;
}

} // namespace ici

#endif /* ICI_EXEC_H */
