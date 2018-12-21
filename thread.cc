#define ICI_CORE
#include "fwd.h"
#include "exec.h"
#include "array.h"
#include "null.h"
#include "cfunc.h"
#include "op.h"
#include "catcher.h"

#include <atomic>
#include <mutex>
#include <thread>

namespace ici
{

std::mutex              ici_mutex;
std::atomic<int>        ici_n_active_threads;

/*
 * Leave code that uses ICI data. ICI data refers to *any* ICI objects
 * or static variables. You would want to call this because you are
 * about to do something that uses a lot of CPU time or blocks for
 * any real time. But you must not even sniff any of ICI's data until
 * after you call 'enter()' again. 'leave()' releases the global
 * ICI mutex that stops ICI threads from simultaneous access to common data.
 * All ICI objects are "common data" because they are shared between
 * threads.
 *
 * Returns the pointer to the ICI execution context of the current
 * thread. This must be preserved (in a local variable on the stack
 * or some other thread safe location) and passed back to the matching
 * call to enter() you will make some time in the future.
 *
 * If the current thread is in an ICI level critical section (e.g.
 * the test or body of a watifor) this will have no effect (but should
 * still be matched with a call to 'enter()'.
 *
 * This function never fails.
 *
 * Note that even ICI implementations without thread support provide this
 * function. In these implemnetations it has no effect.
 *
 * This --func-- forms part of the --ici-api--.
 */

static exec *leave_locked()
{
    exec *x = ex;
    if (!x->x_critsect)
    {
        /*
         * Restore the copies of our stack arrays that are cached
         * in static locations back to the execution context.
         */
        os.decref();
        xs.decref();
        vs.decref();
        *x->x_os = os;
        *x->x_xs = xs;
        *x->x_vs = vs;
        x->x_count = exec_count;
        --ici_n_active_threads;
    }
    return x;
}

exec *leave()
{
    auto x = leave_locked();
    ici_mutex.unlock();
    return x;
}

/*
 * Enter code that uses ICI data. ICI data referes to *any* ICI objects
 * or static variables. You must do this after having left ICI's mutex
 * domain, by calling 'leave()', before you again access any ICI data.
 * This call will re-acquire the global ICI mutex that gates access to
 * common ICI data. You must pass in the ICI execution context pointer
 * that you remembered from the previous matching call to 'leave()'.
 *
 * If the thread was in an ICI level critical section when the 'leave()'
 * call was made, then this will have no effect (mirroring the no effect
 * that happened when the leave() was done).
 *
 * Note that even ICI implementations without thread support provide this
 * function. In these implemnetations it has no effect.
 *
 * This --func-- forms part of the --ici-api--.
 */
void enter(exec *x)
{
    if (!x->x_critsect)
    {
        ++ici_n_active_threads;
        ici_mutex.lock();
        if (x != ex)
        {
            /*
             * This really is a change of contexts. Set the global
             * context pointer, and move the stack arrays to the
             * global cached copies (which we do to save a level of
             * indirection on all accesses).
             */
            ex = x;
            os = *x->x_os;
            xs = *x->x_xs;
            vs = *x->x_vs;
            exec_count = x->x_count;
        }
        x->x_os->a_base = nullptr;
        x->x_xs->a_base = nullptr;
        x->x_vs->a_base = nullptr;
        os.incref();
        xs.incref();
        vs.incref();
    }
}

/*
 * Allow a switch away from, and back to, this ICI thread, otherwise
 * no effect. This allows other ICI threads to run, but by the time
 * this function returns, the ICI mutex has be re-acquired for the
 * current thread. This is the same as as 'enter(leave())',
 * except it is more efficient when no actual switching was required.
 *
 * Note that even ICI implementations without thread support provide this
 * function. In these implemnetations it has no effect.
 *
 * This --func-- forms part of the --ici-api--.
 */
void yield()
{
    exec  *x;

    x = ex;
    if (ici_n_active_threads > 1 && x->x_critsect == 0)
    {
        os.decref();
        xs.decref();
        vs.decref();
        *x->x_os = os;
        *x->x_xs = xs;
        *x->x_vs = vs;
        x->x_count = exec_count;
        ici_mutex.unlock();
        std::this_thread::yield();
        ici_mutex.lock();
        if (x != ex)
        {
            /*
             * This really is a change of contexts. Set the global
             * context pointer, and move the stack arrays to the
             * global cached copies (which we do to save a level of
             * indirection on all accesses.
             */
            ex = x;
            os = *x->x_os;
            xs = *x->x_xs;
            vs = *x->x_vs;
            exec_count = x->x_count;
        }
        x->x_os->a_base = nullptr;
        x->x_xs->a_base = nullptr;
        x->x_vs->a_base = nullptr;
        os.incref();
        xs.incref();
        vs.incref();
    }
}

/*
 * Wait for the given object to be signaled. This is the core primitive of
 * the waitfor ICI language construct. However this function only does the
 * actual waiting part. When called, it will release the ICI mutex, and
 * wait for the object 'o' to be signaled by an 'wakeup' call. It will
 * the re-aquire the mutex and return. It should always be assumed that
 * any particular object could be "woken up" for reasons that are not
 * aparent to the waiter. In other words, always check that the condition
 * that necessitates you waiting has really finished.
 *
 * The caller of this function would use a loop such as:
 *
 *  while (condition-not-met)
 *      waitfor(object);
 *
 * Returns non-zero on error. Usual conventions. Note that this function
 * will always fail in implementations without thread support.
 *
 * This --func-- forms part of the --ici-api--.
 */
int waitfor(object *o)
{
    exec       *x;
    const char *e;

    e = nullptr;
    ex->x_waitfor = o;
    x = leave_locked();
    {
        // we need a lock<> to invoke cv's wait(), adopt ici_mutex
        std::unique_lock<std::mutex> lock(ici_mutex, std::adopt_lock);
        assert(lock.owns_lock());
        x->x_semaphore->wait(lock);
    }
    enter(x);
    if ((e = x->x_error) != nullptr)
    {
        return set_error("%s", e);
    }
    return 0;
}

/*
 * Wake up all ICI threads that are waiting for the given object (and
 * thus allow them re-evaluate their wait expression).
 *
 * This --func-- forms part of the --ici-api--.
 */
int wakeup(object *o)
{
    exec *x;

    for (x = execs; x != nullptr; x = x->x_next)
    {
        if (x->x_waitfor == o)
        {
            x->x_waitfor = nullptr;
            x->x_semaphore->notify_all();
        }
    }
    return 0;
}


/*
 * Entry point for a new thread. The passed argument is the pointer
 * to the execution context (ex_t *). It has one ref count that is
 * now considered to be owned by this function. The operand stack
 * of the new context has the ICI function to be called configured on
 * it.
 */
static void ici_thread_base(void *arg)
{
    exec *x = (exec *)arg;
    int  n_ops;

    enter(x);
    n_ops = os.a_top - os.a_base;
    if ((x->x_result = evaluate(&o_call, n_ops)) == nullptr)
    {
        x->x_result = str_get_nul_term(error);
        x->x_state = XS_FAILED;
        fprintf(stderr, "Warning, uncaught error in sub-thread: %s\n", error);
    }
    else
    {
        decref(x->x_result);
        x->x_state = XS_RETURNED;
    }
    wakeup(x);
    decref(x);
    (void)leave();
}

/*
 * From ICI: exec = go(callable, arg1, arg2, ...)
 */
static int
f_go(...)
{
    exec *x;
    int  i;

    if (NARGS() < 1 || !ARG(0)->can_call())
        return argerror(0);

    if ((x = new_exec()) == nullptr)
        return 1;
    /*
     * Copy the most-recently-executed source marker to the new thread
     * to give a useful indication for any errors during startup.
     */
    x->x_src = ex->x_src;
    /*
     * Copy all the arguments to the operand stack of the new thread.
     */
    if (x->x_os->push_check(NARGS() + 80))
        goto fail;
    for (i = 1; i < NARGS(); ++i)
        x->x_os->a_top[NARGS() - i - 1] = ARG(i);
    x->x_os->a_top += NARGS() - 1;
    /*
     * Now push the number of actuals and the object to call on the
     * new operand stack.
     */
    if ((*x->x_os->a_top = new_int(NARGS() - 1)) == nullptr)
        goto fail;
    decref((*x->x_os->a_top));
    ++x->x_os->a_top;
    x->x_os->push(ARG(0));
    /*
     * Create the native machine thread. We ici_incref x to give the new thread
     * it's own reference.
     */
    incref(x);
    {
        // TODO: try/catch
        std::thread t([x]() { ici_thread_base(x); });
        t.detach();
    }
    return ret_with_decref(x);

fail:
    decref(x);
    return 1;
}

static int
f_wakeup(...)
{
    if (NARGS() != 1)
        return argcount(1);
    if (wakeup(ARG(0)))
        return 1;
    return null_ret();
}

ICI_DEFINE_CFUNCS(thread)
{
    ICI_DEFINE_CFUNC(go,            f_go),
    ICI_DEFINE_CFUNC(wakeup,        f_wakeup),
    ICI_CFUNCS_END()
};

} // namespace ici
