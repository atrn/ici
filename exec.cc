#define ICI_CORE
#include "exec.h"
#include "op.h"
#include "debugger.h"
#include "catcher.h"
#include "ptr.h"
#include "func.h"
#include "cfunc.h"
#include "str.h"
#include "buf.h"
#include "pc.h"
#include "int.h"
#include "map.h"
#include "set.h"
#include "parse.h"
#include "float.h"
#include "re.h"
#include "method.h"
#include "mark.h"
#include "src.h"
#include "null.h"
#include "forall.h"
#include "primes.h"
#include <signal.h>

namespace ici
{

/*
 * List of all active execution structures.
 */
exec      *execs;

/*
 * The global pointer to the current execution context and cached pointers
 * to the three stacks (execution, operand and variable (scope)). These are
 * set every time we switch ICI threads.
 */
exec      *ex;

/*
 * A cached copy of ex->x_count for the current thread.
 */
int        exec_count;

/*
 * The arrays that form the current execution, operand and variable (scope)
 * stacks. These are actually swapped-in copies of the stacks that are
 * referenced from the current execution context. They get copied into
 * these fixed-address locations when we switch into a particular context
 * to save a level of indirection on top-of-stack references (it can make
 * up to 20% difference in CPU time). While we are executing in a particular
 * context, the "real" arrays have their a_base pointer zapped to nullptr so
 * those copies of the base, limit and top pointers. When we switch away
 * from a particular context, we copies these stacks back to real array
 * structs (see thread.c).
 */
array     xs;
array     os;
array     vs;

integer   *o_zero;
integer   *o_one;

/*
 * Set this to non-zero to cause an "aborted" failure even when the ICI
 * program is stuck in an infinite loop.  For use in embeded systems etc
 * to provide protection against badly behaved user programs.  How it gets
 * set is not addressed here (by an interupt perhaps).  Remember to clear
 * it before re-running any ICI code.
 */
volatile int    aborted;

/*
 * The limit to the number of recursive invocations of evaluate().
 */
int             evaluate_recursion_limit = 50;

static sigset_t empty_sigset;

inline bool isempty(const sigset_t *s) {
    return memcmp(s, &empty_sigset, sizeof empty_sigset) == 0;
}

void init_exec() {
    sigemptyset(&empty_sigset);
}

/*
 * Create and return a pointer to a new ICI execution context.
 * On return, all stacks (execution, operand and variable) are empty.
 * Returns nullptr on failure, in which case error has been set.
 * The new exec struct is linked onto the global list of all exec
 * structs (execs).
 */
exec *new_exec() {
    exec          *x;
    static src    default_src;

    if ((x = ici_talloc(exec)) ==  nullptr)
    {
        return nullptr;
    }
    memset(x, 0, sizeof *x);
    set_tfnz(x, TC_EXEC, 0, 1, 0);
    rego(x);
    x->x_src = &default_src;
    if ((x->x_xs = new_array(80)) == nullptr)
    {
        goto fail;
    }
    decref(x->x_xs);
    if ((x->x_os = new_array(80)) == nullptr)
    {
        goto fail;
    }
    decref(x->x_os);
    if ((x->x_vs = new_array(80)) == nullptr)
    {
        goto fail;
    }
    decref(x->x_vs);
    if ((x->x_pc_closet = new_array(80)) == nullptr)
    {
        goto fail;
    }
    decref(x->x_pc_closet);
    if ((x->x_os_temp_cache = new_array(80)) == nullptr)
    {
        goto fail;
    }
    decref(x->x_os_temp_cache);
    x->x_semaphore = new std::condition_variable;
    x->x_state = XS_ACTIVE;
    x->x_count = 100;
    x->x_n_engine_recurse = 0;
    x->x_next = execs;
    x->x_error = nullptr;
    execs = x;
    return x;

fail:
    return nullptr;
}

/*
 * The main execution loop avoids continuous checks for available space
 * on its three stacks (operand, execution, and scope) by knowing that
 * no ordinary operation increase the stack depth by more than 3 levels
 * at a time. Knowing this, every N times round the loop it checks that
 * there is room for N * 3 objects.
 */
int engine_stack_check()
{
    array *pcs;
    int   depth;

    if (xs.push_check(60))
    {
        return 1;
    }
    if (os.push_check(60))
    {
        return 1;
    }
    if (vs.push_check(60))
    {
        return 1;
    }
    pcs = ex->x_pc_closet;
    depth = (xs.a_top - xs.a_base) + 60;
    if ((depth -= (pcs->a_top - pcs->a_base)) > 0)
    {
        if (pcs->push_check(depth))
        {
            return 1;
        }
        while (pcs->a_top < pcs->a_limit)
        {
            if ((*pcs->a_top = new_pc()) == nullptr)
            {
                return 1;
            }
            ++pcs->a_top;
        }
    }
    return 0;
}

/*
 * Faster lookup for string keys.
 */
inline object *fetch(object *s, object *k) {
    if (isstring(k) && stringof(k)->s_map == mapof(s) && stringof(k)->s_vsver == vsver)
        return stringof(k)->s_slot->sl_value;
    else
        return ici_fetch(s, k);
}

/*
 * Unwind the execution stack until a catcher is found.  Then unwind
 * the scope and operand stacks to the matching depth (but only if it is).
 * Returns the catcher, or nullptr if there wasn't one.
 */
static catcher *unwind() {
    for (auto p = xs.a_top - 1; p >= xs.a_base; --p) {
        if (iscatcher(*p)) {
            auto c = catcherof(*p);
            xs.a_top = p;
            os.a_top = &os.a_base[c->c_odepth];
            vs.a_top = &vs.a_base[c->c_vdepth];
            return c;
        }
    }
    assert(0);
    return nullptr;
}

/*
 * Execute 'code' (any object, normally an array of code or a
 * parse).  The execution procedes on top of the current stacks
 * (execution, operand and variable).  This call to evaluate will return when
 * the execution stack again returns to the level it was when entered.  It
 * then returns the object left on the operand stack, or null if there
 * wasn't one.  The returned object is ici_incref()ed.  Returns nullptr on error,
 * usual conventions (i.e.  'error' points to the error message).
 *
 * n_operands is the number of objects on the operand stack that are arguments
 * to this call.  They will all be poped off before evaluate returns.
 *
 * The execution loop knows when the execution stack returns to its origional
 * level because it puts a catcher object on it.  This object also records
 * the levels of the other two stacks that match.
 *
 * This is the main execution loop.  All of the nasty optimisations are
 * concentrated here.  It used to be clean, elegant and 20 lines long.  Now it
 * goes faster.
 *
 * Originally each type had an execution method, which was a function pointer
 * like all the other type specific methods.  This was unrolled into a switch
 * within the main loop for speed and variable optimisation.  Then many of the
 * op_type operations got unrolled in the loop in the same way.  Gotos got
 * added to short-circuit some of the steps where possible.  Then they got
 * removed, apart from the fail exits.  Then one got added again.  And again.
 *
 * Note that binop.h is included half way down this function.
 */
object *evaluate(object *code, int n_operands)
{
    object      *o;
    object      *pc;
    int         flags;
    catcher     frame;

    if (++ex->x_n_engine_recurse > evaluate_recursion_limit)
    {
        set_error("excessive recursive invocations of main interpreter");
        goto badfail;
    }

    if (engine_stack_check())
    {
        goto badfail;
    }

    /*
     * This is pretty scary.  An object on the C stack.  But it should be OK
     * because it's only on the execution stack and there should be no way for
     * it to escape into the world at large, so no-one should be able to have
     * a reference to it after we return.  It will get poped off before we do.
     * It's not registered with the garbage collector, so after that, it's
     * just gone.  We do this to save allocation/collection of an object on
     * every call from C to ICI.  This object *will* be seen by the mark phase
     * of the garbage collection, which may occur in a thread other than this
     * one.  This is likely to cause a good memory integrity checking system
     * to complain.
     */
    set_tfnz(&frame, TC_CATCHER, CF_EVAL_BASE, 0, 0);
    frame.c_catcher = nullptr;
    frame.c_odepth = (os.a_top - os.a_base) - n_operands;
    frame.c_vdepth = vs.a_top - vs.a_base;
    xs.push(&frame);

    if (isarray(code)) {
        set_pc(arrayof(code), xs.a_top);
    } else {
        *xs.a_top = code;
    }
    ++xs.a_top;

    /*
     * The execution loop.
     */
    for (;;)
    {
        if (UNLIKELY(--exec_count == 0)) {
            if (UNLIKELY(aborted)) {
                set_error("aborted");
                goto fail;
            }
            /*
             * Ensure that there is enough room on all stacks for at
             * least 20 more worst case operations.  See also f_call().
             */
            if (UNLIKELY(engine_stack_check())) {
                goto fail;
            }
            exec_count = 100;
            if (UNLIKELY(++ex->x_yield_count > 10)) {
                yield();
                ex->x_yield_count = 0;
            }

            /*
             * Check for, and handle, pending signals.
             */
            sigset_t *p = (sigset_t *)(void *)&signals_pending;
            if (UNLIKELY(!isempty(p))) {
                invoke_signal_handlers();
	    }
        }

        /*
         * Places which would be inclined to continue in this loop, that
         * know that they have not increased any stack depths, can just
         * goto this label to avoid the check above.
         */
    stable_stacks_continue:
        /*
         * In principle our execution model is pretty simple. We execute
         * the thing on the top of the execution stack. When that is a pc
         * we push the thing the pc points to (with post increment) onto
         * the execution stack and continue.
         *
         * So technically the following test for a pc on top of the
         * execution stack should just be part of the main switch (as it
         * once was). But 90% of the time we execute: pc then other;
         * pc then other etc... Doing this test here means that normally
         * we run these two operations into one another without going
         * round the main loop. And then, enough of the things we do are
         * operators to make a short-circuit of the switch worth while.
         *
         * Code arrays never contain pcs, so the thing picked up from
         * indirecting the pc is not a pc, so there is no general case
         * in the switch.
         */
        assert(os.a_top >= os.a_base);
        if (ispc(pc = xs.a_top[-1]))
        {
    continue_with_same_pc:
            o = *pcof(pc)->pc_next++;
            if (isop(o)) {
                goto an_op;
            }
        } else {
            o = pc;
            --xs.a_top;
        }

        /*
         * Formally, the thing being executed should be on top of the
         * execution stack when we do this switch. But the value is
         * known to be pointed to by o, and most things pop it off.
         * We get a net gain if we assume it is pre-popped, because we
         * can avoid pushing things that are comming out of code arrays
         * on at all. Some of the cases below must push it on to restore
         * the formal model. The code just above here assumes this, but
         * has to explicitly pop the stack in the non-pc case.
         */
        switch (o->o_tcode)
        {
        case TC_SRC:
            ex->x_src = srcof(o);
            if (UNLIKELY(debug_active))
            {
                xs.push(o); /* Restore formal state. */
                o_debug->src(srcof(o));
                --xs.a_top;
                continue;
            }
            goto stable_stacks_continue;

        case TC_PARSE:
            xs.push(o); /* Restore formal state. */
            if (parse_exec())
	    {
                goto fail;
	    }
            continue;

        case TC_STRING:
            /*
             * Executing a string is the operation of variable lookup.
             * Look up the value of the string on the execution stack
             * in the current scope and push the value onto the operand
             * stack.
             *
             * First check for lookup lookaside.
             */
            if
            (
                stringof(o)->s_map == mapof(vs.a_top[-1])
                &&
                stringof(o)->s_vsver == vsver
            )
            {
                /*
                 * We know directly where the value is because we have
                 * looked up this name since the last change to the scope
                 * or structures etc.
                 */
                assert(ici_fetch_super(vs.a_top[-1], o, os.a_top, nullptr) == 1);
                assert(*os.a_top == stringof(o)->s_slot->sl_value);
                os.push(stringof(o)->s_slot->sl_value);
            }
            else
            {
                object   *f;

                /*
                 * This is an in-line version of fetch_map because
                 * (a) we know that the top of the variable stack is
                 * always a struct, and (b) we want to detect when the
                 * value is not found so we can do auto-loading.
                 */
                switch
                (
                    ici_fetch_super
                    (
                        vs.a_top[-1],
                        o,
                        os.a_top,
                        mapof(vs.a_top[-1])
                    )
                )
                {
                case -1:
                    goto fail;

                case 0:
                    /*
                     * We failed to find that name on first lookup.
                     * Try to load a library of that name and repeat
                     * the lookup before deciding it is undefined.
                     */
                    if ((f = ici_fetch(vs.a_top[-1], SS(load))) == null)
                    {
                        set_error("\"%s\" undefined", stringof(o)->s_chars);
                        goto fail;
                    }
                    xs.push(o); /* Temp restore formal state. */
                    {
                        src *srco = ex->x_src;
                        incref(srco);
                        if (call(f, "o", o))
                        {
                            decref(srco);
                            goto fail;
                        }
                        ex->x_src = srco;
                        decref(srco);
                    }
                    --xs.a_top;
                    switch
                    (
                        ici_fetch_super
                        (
                            vs.a_top[-1],
                            o,
                            os.a_top,
                            mapof(vs.a_top[-1])
                        )
                    )
                    {
                    case -1:
                        goto fail;

                    case 0:
                        set_error("load() failed to define \"%s\"", stringof(o)->s_chars);
                        goto fail;
                    }
                }
                ++os.a_top;
            }
            continue;

        case TC_CATCHER:
            /*
             * This can either be an error catcher which is being poped
             * off (having done its job, but it never got used) or it
             * can be the guard catch (frame marker) indicating it is time
             * to return from evaluate().
             *
             * First note the top of the operand stack, if there is anything
             * on it, it becomes the return value, else we return null.
             * The caller knows if there is really a value to return.
             */
            xs.push(o);  /* Restore formal state. */
            if (o->flagged(CF_EVAL_BASE))
            {
                /*
                 * This is the base of a call to evaluate().  It is now
                 * time to return.
                 */
                if (catcherof(o)->c_odepth < uint32_t(os.a_top - os.a_base))
                {
                    o = os.a_top[-1];
                }
                else
                {
                    o = null;
                }
                incref(o);
                unwind();
                --ex->x_n_engine_recurse;
                return o;
            }
            if (o->flagged(CF_CRIT_SECT))
            {
                --ex->x_critsect;
                /*
                 * Force a check for a yield (see top of loop). If we
                 * don't do this, there is a chance a loop that spends
                 * much of its time in critsects could sync with the
                 * stack check countdown and never yield.
                 */
                exec_count = 1;
            }
            unwind();
            goto stable_stacks_continue;

        case TC_FORALL:
            xs.push(o);  /* Restore formal state. */
            if (exec_forall())
            {
                goto fail;
            }
            continue;

        default:
            os.push(o);
            continue;

        case TC_OP:
        an_op:
            switch (opof(o)->op_ecode) {
            case OP_OTHER:
                xs.push(o); /* Restore to formal state. */
                if ((*opof(o)->op_func)()) {
                    goto fail;
                }
                continue;

            case OP_SUPER_CALL:
                flags = OPC_COLON_CALL | OPC_COLON_CARET;
                goto do_colon;

            case OP_METHOD_CALL:
                flags = OPC_COLON_CALL;
                goto do_colon;

            case OP_COLON:
                /*
                 * aggr key => method (os) (normal case)
                 */
                {
                    object           *o1;
                    object           *t;

                    flags = opof(o)->op_code;
                do_colon:
                    o1 = o;
                    t = os.a_top[-2];
                    if (flags & OPC_COLON_CARET) {
                        if ((o = fetch(vs.a_top[-1], SS(class))) == nullptr) {
                            goto fail;
                        }
                        if (!hassuper(o)) {
                            char        n1[objnamez];
                            set_error("\"class\" evaluated to %s in :^ operation", objname(n1, o));
                            goto fail;
                        }
                        if ((t = objwsupof(o)->o_super) == nullptr) {
                            set_error("class has no super class in :^ operation");
                            goto fail;
                        }
                    }
                    if (t->icitype()->can_fetch_method()) {
                        if ((o = t->icitype()->fetch_method(t, os.a_top[-1])) == nullptr) {
                            goto fail;
                        }
                    } else {
                        if ((o = fetch(t, os.a_top[-1])) == nullptr) {
                            goto fail;
                        }
                        if (isnull(o)) {
                            if ((o = fetch(t, SS(unknown_method))) == nullptr) {
                                goto fail;
                            }
                            if (!isnull(o)) {
                                object *nam = os.a_top[-1];
                                long nargs = intof(os.a_top[-3])->i_value;
                                
                                ++os.a_top;
                                os.a_top[-1] = SS(unknown_method);
                                os.a_top[-2] = os.a_top[-3];
                                if ((os.a_top[-3] = new_int(nargs + 1)) == nullptr) {
                                    goto fail;
                                }
                                decref(os.a_top[-3]);
                                os.a_top[-4] = nam;
                            }
                        }
                    }
                    if ((flags & OPC_COLON_CALL) == 0) {
                        method *m;

                        if ((m = new_method(os.a_top[-2], o)) == nullptr) {
                            goto fail;
                        }
                        --os.a_top;
                        os.a_top[-1] = m;
                        decref(m);
                        goto stable_stacks_continue;
                    }
                    /*
                     * This is a direct call, don't form the method object.
                     */
                    xs.push(o1); /* Restore xs to formal state. */
                    o1 = os.a_top[-2]; /* The subject object. */
                    --os.a_top;
                    os.a_top[-1] = o;  /* The callable object. */
                    o = o1;
                    incref(o);
                    goto do_call;
                }

            case OP_CALL:
                xs.push(o);  /* Restore to formal state. */
                o = nullptr;                   /* No subject object. */
            do_call:
                if (UNLIKELY(!os.a_top[-1]->can_call())) {
                    char    n1[objnamez];

                    set_error("attempt to call %s", objname(n1, os.a_top[-1]));
                    if (o != nullptr) {
                        decref(o);
                    }
                    goto fail;
                }
                if (UNLIKELY(debug_active)) {
                    o_debug->fncall(os.a_top[-1], ARGS(), NARGS());
		}
                if (os.a_top[-1]->call(o)) {
                    if (o != nullptr) {
                        decref(o);
                    }
                    goto fail;
                }
                if (o != nullptr) {
                    decref(o);
                }
                continue;

            case OP_QUOTE:
                /*
                 * pc           => pc+1 (xs)
                 *              => *pc (os)
                 */
                o = xs.a_top[-1];
                os.push(*pcof(o)->pc_next++);
                continue;

            case OP_AT:
                /*
                 * obj => obj (os)
                 */
                os.a_top[-1] = atom(os.a_top[-1], 0);
                goto stable_stacks_continue;

            case OP_NAMELVALUE:
                /*
                 * pc (xs)      => pc+1 (xs)
                 *              => struct *pc (os)
                 * (Ie. the next thing in the code array is a name, put its
                 * lvalue on the operand stack.)
                 */
                os.push(vs.a_top[-1]);
                os.push(*pcof(xs.a_top[-1])->pc_next++);
                continue;

            case OP_DOT:
                /*
                 * aggr key => value (os)
                 */
                if ((o = fetch(os.a_top[-2], os.a_top[-1])) == nullptr) {
                    goto fail;
                }
                --os.a_top;
                os.a_top[-1] = o;
                goto stable_stacks_continue;

            case OP_DOTKEEP:
                /*
                 * aggr key => aggr key value (os)
                 */
                if ((o = fetch(os.a_top[-2], os.a_top[-1])) == nullptr) {
                    goto fail;
                }
                os.push(o);
                continue;

            case OP_DOTRKEEP:
                /*
                 * aggr key => value aggr key value (os)
                 *
                 * Used in postfix ++/-- for value.
                 */
                if ((o = fetch(os.a_top[-2], os.a_top[-1])) == nullptr) {
                    goto fail;
                }
                os.a_top += 2;
                os.a_top[-1] = o;
                os.a_top[-2] = os.a_top[-3];
                os.a_top[-3] = os.a_top[-4];
                os.a_top[-4] = o;
                continue;

            case OP_ASSIGNLOCALVAR:
                /*
                 * name value => - (os, for effect)
                 *                => value (os, for value)
                 *                => aggr key (os, for lvalue)
                 */
                if (ici_assign_base(vs.a_top[-1], os.a_top[-2],os.a_top[-1])) {
                    goto fail;
                }
                switch (opof(o)->op_code) {
                case FOR_EFFECT:
                    os.a_top -= 2;
                    break;

                case FOR_VALUE:
                    os.a_top[-2] = os.a_top[-1];
                    --os.a_top;
                    break;

                case FOR_LVALUE:
                    os.a_top[-1] = os.a_top[-2];
                    os.a_top[-2] = vs.a_top[-1];
                    break;
                }
                continue;

            case OP_ASSIGN_TO_NAME:
                /*
                 * value on os, next item in code is name.
                 */
                os.a_top += 2;
                os.a_top[-1] = os.a_top[-3];
                os.a_top[-2] = *pcof(xs.a_top[-1])->pc_next++;
                os.a_top[-3] = vs.a_top[-1];
                /* Fall through. */
            case OP_ASSIGN:
                /*
                 * aggr key value => - (os, for effect)
                 *                => value (os, for value)
                 *                => aggr key (os, for lvalue)
                 */
                if
                (
                    stringof(os.a_top[-2])->s_map == mapof(os.a_top[-3])
                    &&
                    stringof(os.a_top[-2])->s_vsver == vsver
                    &&
                    isstring(os.a_top[-2])
                    &&
                    !os.a_top[-2]->flagged(ICI_S_LOOKASIDE_IS_ATOM)
                ) {
                    stringof(os.a_top[-2])->s_slot->sl_value = os.a_top[-1];
                    goto assign_finish;
                }
                if (ici_assign(os.a_top[-3], os.a_top[-2], os.a_top[-1])) {
                    goto fail;
		}
                goto assign_finish;

            case OP_ASSIGNLOCAL:
                /*
                 * aggr key value => - (os, for effect)
                 *                => value (os, for value)
                 *                => aggr key (os, for lvalue)
                 */
                if (hassuper(os.a_top[-3])) {
                    if (ici_assign_base(os.a_top[-3], os.a_top[-2], os.a_top[-1])) {
                        goto fail;
                    }
                } else {
                    if (ici_assign(os.a_top[-3], os.a_top[-2], os.a_top[-1])) {
                        goto fail;
                    }
                }
            assign_finish:
                switch (opof(o)->op_code) {
                case FOR_EFFECT:
                    os.a_top -= 3;
                    break;

                case FOR_VALUE:
                    os.a_top[-3] = os.a_top[-1];
                    os.a_top -= 2;
                    break;

                case FOR_LVALUE:
                    --os.a_top;
                    break;
                }
                continue;

            case OP_SWAP:
                /*
                 * aggr1 key1 aggr2 key2        =>
                 *                              => value1
                 *                              => aggr1 key1
                 */
                {
                    object  *v1;
                    object  *v2;

                    if ((v1 = ici_fetch(os.a_top[-4], os.a_top[-3])) == nullptr) {
                        goto fail;
                    }
                    incref(v1);
                    if ((v2 = ici_fetch(os.a_top[-2], os.a_top[-1])) == nullptr) {
                        decref(v1);
                        goto fail;
                    }
                    incref(v2);
                    if (ici_assign(os.a_top[-2], os.a_top[-1], v1)) {
                        decref(v1);
                        decref(v2);
                        goto fail;
                    }
                    if (ici_assign(os.a_top[-4], os.a_top[-3], v2)) {
                        decref(v1);
                        decref(v2);
                        goto fail;
                    }
                    switch (opof(o)->op_code) {
                    case FOR_EFFECT:
                        os.a_top -= 4;
                        break;

                    case FOR_VALUE:
                        os.a_top[-4] = v2;
                        os.a_top -= 3;
                        break;

                    case FOR_LVALUE:
                        os.a_top -= 2;
                        break;
                    }
                    decref(v1);
                    decref(v2);
                }
                continue;

            case OP_IF:
                /*
                 * bool => - (os)
                 *
                 */
                if (isfalse(os.a_top[-1])) {
                    --os.a_top;
                    ++pcof(xs.a_top[-1])->pc_next;
                    goto stable_stacks_continue;
                }
                o = *pcof(xs.a_top[-1])->pc_next++;
                set_pc(arrayof(o), xs.a_top);
                --os.a_top;
                ++xs.a_top;
                continue;

            case OP_IFELSE:
                /*
                 * bool => -
                 */
                if (isfalse(os.a_top[-1])) {
                    ++pcof(xs.a_top[-1])->pc_next;
                    o = *pcof(xs.a_top[-1])->pc_next++;
                } else {
                    o = *pcof(xs.a_top[-1])->pc_next++;
                    ++pcof(xs.a_top[-1])->pc_next;
                }
                set_pc(arrayof(o), xs.a_top);
                --os.a_top;
                ++xs.a_top;
                goto stable_stacks_continue;

            case OP_IFBREAK:
                /*
                 * bool => - (os)
                 *      => [o_break] (xs)
                 */
                if (isfalse(os.a_top[-1]))
                {
                    --os.a_top;
                    continue;
                }
                --os.a_top;
                goto do_break;

             case OP_IFNOTBREAK:
                /*
                 * bool => - (os)
                 *      => [o_break] (xs)
                 */
                if (!isfalse(os.a_top[-1]))
                {
                    --os.a_top;
                    continue;
                }
                --os.a_top;
                /* Falling through. */
            case OP_BREAK:
            do_break:
                /*
                 * Pop the execution stack until a looper or switcher
                 * is found and disgard it (and the thing under it,
                 * which is the code array that is the body of the loop).
                 * Oh, and forall as well.
                 */
                {
                    object  **s;

                    for (s = xs.a_top; s > xs.a_base + 1; --s)
                    {
                        if (iscatcher(s[-1]))
                        {
                            if (s[-1]->flagged(CF_CRIT_SECT))
                            {
                                --ex->x_critsect;
                                exec_count = 1;
                            }
                            else if (s[-1]->flagged(CF_EVAL_BASE))
                            {
                                break;
                            }
                        }
                        else if
                        (
                            s[-1] == &o_looper
                            ||
                            s[-1] == &o_switcher
                        )
                        {
                            xs.a_top = s - 2;
                            goto stable_stacks_continue;
                        }
                        else if (isforall(s[-1]))
                        {
                            xs.a_top = s - 1;
                            goto stable_stacks_continue;
                        }
                    }
                }
                set_error("break not within loop or switch");
                goto fail;

            case OP_ANDAND:
                /*
                 * bool obj => bool (os) OR pc (xs)
                 */
                {
                    int         c;

                    if ((c = !isfalse(os.a_top[-2])) == opof(o)->op_code)
                    {
                        /*
                         * Have to test next part of the condition.
                         */
                        set_pc(arrayof(os.a_top[-1]), xs.a_top);
                        ++xs.a_top;
                        os.a_top -= 2;
                        goto stable_stacks_continue;
                    }
                    /*
                     * This is the old behaviour of ICI 4.0.3 and before
                     * where the value was reduced to 0 or 1 exactly.
                     *
                     * os.a_top[-2] = c ? o_one : o_zero;
                     */
                    --os.a_top;
                }
                goto stable_stacks_continue;

            case OP_CONTINUE:
                /*
                 * Pop the execution stack until a looper is found.
                 */
                {
                    object   **s;

                    for (s = xs.a_top; s > xs.a_base + 1; --s)
                    {
                        if (iscatcher(s[-1]))
                        {
                            if (s[-1]->flagged(CF_EVAL_BASE))
                            {
                                break;
                            }
                            if (s[-1]->flagged(CF_CRIT_SECT))
                            {
                                --ex->x_critsect;
                                exec_count = 1;
                            }
                        }
                        if (s[-1] == &o_looper || isforall(s[-1]))
                        {
                            xs.a_top = s;
                            goto stable_stacks_continue;
                        }
                    }
                }
                set_error("continue not within loop");
                goto fail;

            case OP_REWIND:
                /*
                 * This is the end of a code array that is the subject
                 * of a loop. Rewind the pc back to its start.
                 */
                o = xs.a_top[-1];
                pcof(o)->pc_next = pcof(o)->pc_code->a_base;
                goto continue_with_same_pc;

            case OP_LOOPER:
                /*
                 * obj self     => obj self pc (xs)
                 *              => (os)
                 *
                 * We have fallen out of a code array that is the subject of
                 * a loop. Push a new pc pointing to the start of the code array
                 * back on the stack. This doesn't happen very often now, because
                 * the OP_REWIND (above) does the common case of comming to the
                 * end of a code array that should loop.
                 */
                xs.push(o);
                set_pc(arrayof(xs.a_top[-2]), xs.a_top);
                ++xs.a_top;
                goto stable_stacks_continue;

            case OP_ENDCODE:
                /*
                 * pc => - (xs)
                 */
                --xs.a_top;
                goto stable_stacks_continue;

            case OP_LOOP:
                o = *pcof(xs.a_top[-1])->pc_next++;
                xs.push(o);
                xs.push(&o_looper);
                set_pc(arrayof(o), xs.a_top);
                ++xs.a_top;
                break;

            case OP_EXEC:
                /*
                 * array => - (os)
                 *       => pc (xs)
                 */
                set_pc(arrayof(os.a_top[-1]), xs.a_top);
                ++xs.a_top;
                --os.a_top;
                continue;

            case OP_SWITCHER:
                /*
                 * nullptr self (xs) =>
                 *
                 * This only happens when we fall of the bottom of a switch
                 * without a break.
                 */
                --xs.a_top;
                goto stable_stacks_continue;

            case OP_SWITCH:
                /*
                 * value array struct => (os)
                 *           => nullptr switcher (pc(array) + struct.value) (xs)
                 */
                {
                    slot *sl;

                    if ((sl = find_raw_slot(mapof(os.a_top[-1]), os.a_top[-3]))->sl_key == nullptr)
                    {
                        if ((sl = find_raw_slot(mapof(os.a_top[-1]), &o_mark))->sl_key == nullptr)
                        {
                            /*
                             * No matching case, no default. Pop everything off and
                             * continue;
                             */
                            os.a_top -= 3;
                            goto stable_stacks_continue;
                        }
                    }
                    xs.push(null);
                    xs.push(&o_switcher);
                    set_pc(arrayof(os.a_top[-2]), xs.a_top);
                    pcof(*xs.a_top)->pc_next += intof(sl->sl_value)->i_value;
                    ++xs.a_top;
                    os.a_top -= 3;
                }
                goto stable_stacks_continue;

            case OP_CRITSECT:
                {
                    *xs.a_top = new_catcher
                    (
                        nullptr,
                        (os.a_top - os.a_base) - 1,
                        vs.a_top - vs.a_base,
                        CF_CRIT_SECT
                    );
                    if (*xs.a_top == nullptr)
                    {
                        goto fail;
                    }
                    ++xs.a_top;
                    set_pc(arrayof(os.a_top[-1]), xs.a_top);
                    ++xs.a_top;
                    --os.a_top;
                    ++ex->x_critsect;
                }
                continue;


            case OP_WAITFOR:
                /*
                 * obj => - (os)
                 */
                --ex->x_critsect;
                waitfor(os.a_top[-1]);
                ++ex->x_critsect;
                --os.a_top;
                goto stable_stacks_continue;

            case OP_POP:
                --os.a_top;
                goto stable_stacks_continue;

            case OP_BINOP:
            case OP_BINOP_FOR_TEMP:
#ifndef BINOPFUNC
#include        "binop.h"
#else
                if (op_binop(o))
                {
                    goto fail;
                }
#endif
                goto continue_with_same_pc;

            default:
                assert(0);
            }
            continue;
        }

    fail:
        {
            catcher *c;

            if (error == nullptr)
            {
                set_error("error");
            }
            if (UNLIKELY(debug_active && !debug_ignore_err))
	    {
                o_debug->errorset(error, ex->x_src);
	    }
            for (;;)
            {
                if ((c = unwind()) == nullptr || c->flagged(CF_EVAL_BASE))
                {
                    goto badfail;
                }
                if (c->flagged(CF_CRIT_SECT))
                {
                    --ex->x_critsect;
                    exec_count = 1;
                    continue;
                }
                break;
            }
            incref(c);
            if
            (
                set_val(objwsupof(vs.a_top[-1]), SS(error), 's', error)
                ||
                set_val(objwsupof(vs.a_top[-1]), SS(errorline), 'i', &ex->x_src->s_lineno)
                ||
                set_val(objwsupof(vs.a_top[-1]), SS(errorfile), 'o', ex->x_src->s_filename)
            )
            {
                decref(c);
                goto badfail;
            }
            set_pc(arrayof(c->c_catcher), xs.a_top);
            ++xs.a_top;
            decref(c);
            continue;

        badfail:
            /*
             * This is not such a useful place to hop into the debugger on
             * error, because we have already unwound the stack. So the user's
             * scope for debugging is very limited. But if it was earlier, we
             * would be breaking on every type of error, even caught ones.
             */
            if (UNLIKELY(debug_active && !debug_ignore_err))
	    {
                o_debug->error(error, ex->x_src);
	    }
            expand_error(ex->x_src->s_lineno, ex->x_src->s_filename);
            --ex->x_n_engine_recurse;
            return nullptr;
        }
    }
}

/*
 * Evaluate 'name' as if it was a variable in a script in the currently
 * prevailing scope, and return its value. If the name is undefined, this
 * will attempt to load extension modules in an attemot to get it defined.
 *
 * This is slightly different from fetching the name from the top element
 * of the scope stack (i.e. 'vs.a_top[-1]') because it will attempt to
 * auto-load, and fail if the name is not defined.
 *
 * The returned object has had it's reference count incremented.
 *
 * Returns nullptr on error, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
object *eval(str *name)
{
    assert(isstring(name));
    return evaluate(name, 0);
}

size_t exec_type::mark(object *o)
{
    auto x = execof(o);
    return type::mark(o)
        + mark_optional(x->x_xs)
        + mark_optional(x->x_os)
        + mark_optional(x->x_vs)
        + mark_optional(x->x_src)
        + mark_optional(x->x_pc_closet)
        + mark_optional(x->x_os_temp_cache)
        + mark_optional(x->x_waitfor)
        + mark_optional(x->x_result)
        + (x->x_error != nullptr ? strlen(x->x_error) + 1: 0);
}

void exec_type::free(object *o)
{
    exec *x;
    exec **xp;

    for (xp = &execs; (x = *xp) != nullptr; xp = &x->x_next)
    {
        if (x == execof(o))
        {
            *xp = x->x_next;
            break;
        }
    }
    assert(x != nullptr);
    delete x->x_semaphore;
    if (x->x_error != nullptr)
    {
        ::free(x->x_error); /* It came from strdup() so use free directly */
    }
    ici_tfree(o, exec);
}

object *exec_type::fetch(object *o, object *k)
{
    exec *x;

    x = execof(o);
    if (k == SS(error))
    {
        if (x->x_error == nullptr)
        {
            return null;
        }
        str *s = new_str_nul_term(x->x_error);
        if (s != nullptr)
        {
            decref(s);
        }
        return s;
    }
    if (k == SS(result))
    {
        switch (x->x_state)
        {
        case XS_ACTIVE:
            return null;

        case XS_RETURNED:
            return x->x_result;

        case XS_FAILED:
            set_error("%s", x->x_result == nullptr  ? "failed" : stringof(x->x_result)->s_chars);
            return nullptr;

        default:
            assert(0);
        }
    }
    else if (k == SS(status))
    {
        switch (x->x_state)
        {
        case XS_ACTIVE:     return SS(active);
        case XS_RETURNED:   return SS(finished);
        case XS_FAILED:     return SS(failed);
        default:                assert(0);
        }
    }
    return null;
}

op    o_quote{OP_QUOTE};

} // namespace ici
