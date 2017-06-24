// -*- mode:c++ -*-

#ifndef ICI_EXEC_H
#define ICI_EXEC_H

#include "fwd.h"
#include "array.h"
#include "int.h"
#include "null.h"
#include "float.h"

#include <condition_variable>

namespace ici
{

union ostemp
{
    ici_int   i;
    ici_float f;
};

struct exec : object
{
    array                   *x_xs;
    array                   *x_os;
    array                   *x_vs;
    src                     *x_src;
    int                      x_count;
    int                      x_yield_count;
    array                   *x_pc_closet; /* See below. */
    array                   *x_os_temp_cache; /* See below. */
    exec                    *x_next;
    int                      x_n_engine_recurse;
    int                      x_critsect;
    object                  *x_waitfor;
    int                      x_state;
    object                  *x_result;
    std::condition_variable *x_semaphore;
    char                    *x_error;
};

inline exec *execof(object *o) { return static_cast<exec *>(o); }
inline bool isexec(object *o) { return o->isa(ICI_TC_EXEC); }

/*
 * x_xs                 The ICI interpreter execution stack. This contains
 *                      objects being executed, which includes 'pc' objects
 *                      (never seen at the language level) that are special
 *                      pointers into code arrays. Entering blocks and
 *                      functions calls cause this stack to grow deeper.
 *                      NB: This stack is swapped into the global varable
 *                      ici_xs for the active thread. Accessing this field
 *                      directly is very rare.
 *
 * x_os                 The ICI interpreter operand stack. This is where
 *                      operands in expressions are stacked during expression
 *                      evaluation (which includes function call argument
 *                      preparation).
 *                      NB: This stack is swapped into the global varable
 *                      ici_os for the active thread. Accessing this field
 *                      directly is very rare.
 *
 * x_vs                 The ICI interpreter 'scope' or 'variable' stack. The
 *                      top element of this stack is always a struct that
 *                      defines the current context for the lookup of variable
 *                      names. Function calls cause this to grow deeper as
 *                      the new scope of the function being entered is pushed
 *                      on the stack.
 *                      NB: This stack is swapped into the global varable
 *                      ici_vs for the active thread. Accessing this field
 *                      directly is very rare.
 *
 * x_count              A count-down until we should check such things as
 *                      whether the above stacks need growing. Various expensive
 *                      tests are delayed and done occasionally to save time.
 *                      NB: This is cached in ici_exec_count for the current
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
 *                      the NULLs) are unions of int and float objects that
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
 *                      it is waiting to be signaled.  NULL if it is not
 *                      sleeping.
 */

/*
 * Possible values of for x_state.
 */
enum
{
    ICI_XS_ACTIVE,          /* Thread has not exited yet. */
    ICI_XS_RETURNED,        /* Function returned and thread exited normally. */
    ICI_XS_FAILED,          /* Function failed and thread exitied. */
};

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

/*
 * ICI debug interface.  The interpreter has a global debug interface enable
 * flag, 'ici_debug_enabled', and a global pointer, 'ici_debug', to one of
 * these structs.  If the flag is set, the interpreter calls these functions.
 * See 'ici_debug' and 'ici_debug_enabled'.
 *
 * idbg_error()         Called with the current value of ici_error (redundant,
 *                      for historical reasons) and a source line marker
 *                      object (see 'ici_src_t') on an uncaught error.
 *                      Actually, this is not so useful, because it is
 *                      currently called after the stack has been unwound.  So
 *                      a user would not be able to see their stack traceback
 *                      and local context.  This behaviour may change in
 *                      future.
 *
 * idbg_fncall()        Called with the object being called, the pointer to
 *                      the first actual argument (see 'ARGS()' and the number
 *                      of actual arguments just before control is transfered
 *                      to a callable object (function, method or anything
 *                      else).
 *
 * idbg_fnresult()      Called with the object being returned from any call.
 *
 * idbg_src()           Called each time execution passes into the region of a
 *                      new source line marker.  These typically occur before
 *                      any of the code generated by a particular line of
 *                      source.
 *
 * idbg_watch()         In theory, called when assignments are made.  However
 *                      optimisations in the interpreter have made this
 *                      difficult to support without performance penalties
 *                      even when debugging is not enabled.  So it is
 *                      currently disabled.  The function remains here pending
 *                      discovery of a method of achieving it efficiently.
 *
 * This --struct-- forms part of the --ici-api--.
 */
struct debug
{
    void    (*idbg_error)(char *, src *);
    void    (*idbg_fncall)(object *, object **, int);
    void    (*idbg_fnresult)(object *);
    void    (*idbg_src)(src *);
    void    (*idbg_watch)(object *, object *, object *);
};

/*
 * Test if an object represents a false value NULL or integer 0.
 */
inline bool isfalse(object *o) { return isnull(o) || o == ici_zero; }

/*
 * End of ici.h export. --ici.h-end--
 */

#define ici_get_pc(code, xs) \
    (*(xs) = ici_exec->x_pc_closet->a_base[(xs) - ici_xs.a_base], \
    pcof(*(xs))->pc_code = code, \
    pcof(*(xs))->pc_next = pcof(*(xs))->pc_code->a_base)

class exec_type : public type
{
public:
    exec_type() : type("exec", sizeof (struct exec)) {}

    size_t mark(object *o) override;
    void free(object *o) override;
    object *fetch(object *o, object *k) override;
};

} // namespace ici

#endif /* ICI_EXEC_H */
