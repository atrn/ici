#define ICI_CORE
#include "fwd.h"
#include "exec.h"
#include "array.h"
#include "null.h"
#include "cfunc.h"
#include "op.h"
#include "catch.h"

#include <atomic>
#include <errno.h>

namespace ici
{

#ifdef ICI_USE_WIN32_THREADS
HANDLE                  ici_mutex;
#endif

#ifdef ICI_USE_POSIX_THREADS
#include <pthread.h>

pthread_mutex_t         ici_mutex;
#endif

std::atomic<int>        ici_n_active_threads;

/*
 * Leave code that uses ICI data. ICI data refers to *any* ICI objects
 * or static variables. You would want to call this because you are
 * about to do something that uses a lot of CPU time or blocks for
 * any real time. But you must not even sniff any of ICI's data until
 * after you call 'ici_enter()' again. 'ici_leave()' releases the global
 * ICI mutex that stops ICI threads from simultaneous access to common data.
 * All ICI objects are "common data" because they are shared between
 * threads.
 *
 * Returns the pointer to the ICI execution context of the current
 * thread. This must be preserved (in a local variable on the stack
 * or some other thread safe location) and passed back to the matching
 * call to ici_enter() you will make some time in the future.
 *
 * If the current thread is in an ICI level critical section (e.g.
 * the test or body of a watifor) this will have no effect (but should
 * still be matched with a call to 'ici_enter()'.
 *
 * This function never fails.
 *
 * Note that even ICI implementations without thread support provide this
 * function. In these implemnetations it has no effect.
 *
 * This --func-- forms part of the --ici-api--.
 */
ici_exec_t *
ici_leave()
{
    ici_exec_t          *x;

    x = ici_exec;
    // if (!x->x_critsect)
    if (!__sync_fetch_and_add(&x->x_critsect, 0))
    {
        /*
         * Restore the copies of our stack arrays that are cached
         * in static locations back to the execution context.
         */
        ici_decref(&ici_os);
        ici_decref(&ici_xs);
        ici_decref(&ici_vs);
        *x->x_os = ici_os;
        *x->x_xs = ici_xs;
        *x->x_vs = ici_vs;
        x->x_count = ici_exec_count;
        --ici_n_active_threads;
#ifdef ICI_USE_WIN32_THREADS
        ReleaseMutex(ici_mutex);
#else
# ifdef ICI_USE_POSIX_THREADS
        if (pthread_mutex_unlock(&ici_mutex) != 0)
            abort();
# else
        /*
         * It is ok to do ici_leave in implementations with
         * no thread support.
         */
# endif
#endif
    }
    return x;
}

/*
 * Enter code that uses ICI data. ICI data referes to *any* ICI objects
 * or static variables. You must do this after having left ICI's mutex
 * domain, by calling 'ici_leave()', before you again access any ICI data.
 * This call will re-acquire the global ICI mutex that gates access to
 * common ICI data. You must pass in the ICI execution context pointer
 * that you remembered from the previous matching call to 'ici_leave()'.
 *
 * If the thread was in an ICI level critical section when the 'ici_leave()'
 * call was made, then this will have no effect (mirroring the no effect
 * that happened when the ici_leave() was done).
 *
 * Note that even ICI implementations without thread support provide this
 * function. In these implemnetations it has no effect.
 *
 * This --func-- forms part of the --ici-api--.
 */
void
ici_enter(ici_exec_t *x)
{
    // if (!x->x_critsect)
    if (!__sync_fetch_and_add(&x->x_critsect, 0))
    {
        ++ici_n_active_threads;
#ifdef ICI_USE_WIN32_THREADS
        WaitForSingleObject(ici_mutex, INFINITE);
#else
# ifdef ICI_USE_POSIX_THREADS
        if (pthread_mutex_lock(&ici_mutex) != 0)
            abort();
# else
        /*
         * It is ok to do ici_enter in implementations with
         * no thread support.
         */
# endif
#endif
        if (x != ici_exec)
        {
            /*
             * This really is a change of contexts. Set the global
             * context pointer, and move the stack arrays to the
             * global cached copies (which we do to save a level of
             * indirection on all accesses).
             */
            ici_exec = x;
            ici_os = *x->x_os;
            ici_xs = *x->x_xs;
            ici_vs = *x->x_vs;
            ici_exec_count = x->x_count;
        }
        x->x_os->a_base = NULL;
        x->x_xs->a_base = NULL;
        x->x_vs->a_base = NULL;
        ici_incref(&ici_os);
        ici_incref(&ici_xs);
        ici_incref(&ici_vs);
    }
}

/*
 * Allow a switch away from, and back to, this ICI thread, otherwise
 * no effect. This allows other ICI threads to run, but by the time
 * this function returns, the ICI mutex has be re-acquired for the
 * current thread. This is the same as as 'ici_enter(ici_leave())',
 * except it is more efficient when no actual switching was required.
 *
 * Note that even ICI implementations without thread support provide this
 * function. In these implemnetations it has no effect.
 *
 * This --func-- forms part of the --ici-api--.
 */
void
ici_yield()
{
    ici_exec_t          *x;

    x = ici_exec;
    // if (ici_n_active_threads > 1 && x->x_critsect == 0)
    if (ici_n_active_threads > 1 && __sync_fetch_and_add(&x->x_critsect, 0) == 0)
    {
        ici_decref(&ici_os);
        ici_decref(&ici_xs);
        ici_decref(&ici_vs);
        *x->x_os = ici_os;
        *x->x_xs = ici_xs;
        *x->x_vs = ici_vs;
        x->x_count = ici_exec_count;
#ifdef ICI_USE_WIN32_THREADS
        ReleaseMutex(ici_mutex);
        WaitForSingleObject(ici_mutex, INFINITE);
#else
# ifdef ICI_USE_POSIX_THREADS
        if (pthread_mutex_unlock(&ici_mutex) != 0)
            abort();
        sched_yield();
        if (pthread_mutex_lock(&ici_mutex) != 0)
            abort();
# else
        /*
         * It is ok to do ici_yield in implementations with
         * no thread support.
         */
# endif
#endif
        if (x != ici_exec)
        {
            /*
             * This really is a change of contexts. Set the global
             * context pointer, and move the stack arrays to the
             * global cached copies (which we do to save a level of
             * indirection on all accesses.
             */
            ici_exec = x;
            ici_os = *x->x_os;
            ici_xs = *x->x_xs;
            ici_vs = *x->x_vs;
            ici_exec_count = x->x_count;
        }
        x->x_os->a_base = NULL;
        x->x_xs->a_base = NULL;
        x->x_vs->a_base = NULL;
        ici_incref(&ici_os);
        ici_incref(&ici_xs);
        ici_incref(&ici_vs);
    }
}

/*
 * Wait for the given object to be signaled. This is the core primitive of
 * the waitfor ICI language construct. However this function only does the
 * actual waiting part. When called, it will release the ICI mutex, and
 * wait for the object 'o' to be signaled by an 'ici_wakeup' call. It will
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
int
ici_waitfor(ici_obj_t *o)
{
    ici_exec_t          *x;
    const char          *e;

    e = NULL;
    ici_exec->x_waitfor = o;
    x = ici_leave();
#ifdef ICI_USE_WIN32_THREADS
    /*
     * At this point an ici_wakeup() may happen even before we get
     * into the wait. But it's OK because it doesn't matter if the
     * semaphore is incremented before or after we wait, it will
     * still fall out.
     */
    if (WaitForSingleObject(x->x_semaphore, INFINITE) == WAIT_FAILED)
        e = "wait failed";
#else
# ifdef ICI_USE_POSIX_THREADS
    if (sem_wait(&x->x_semaphore) == -1)
        e = "wait failed";
# else
    e = "attempt to wait in waitfor, but no thread support";
# endif
#endif
    ici_enter(x);
    if (e != NULL)
    {
        return ici_set_error("%s", e);
    }
    return 0;
}

/*
 * Wake up all ICI threads that are waiting for the given object (and
 * thus allow them re-evaluate their wait expression).
 *
 * This --func-- forms part of the --ici-api--.
 */
int
ici_wakeup(ici_obj_t *o)
{
    ici_exec_t          *x;

    for (x = ici_execs; x != NULL; x = x->x_next)
    {
        if (x->x_waitfor == o)
        {
            x->x_waitfor = NULL;
#ifdef ICI_USE_WIN32_THREADS
            ReleaseSemaphore(x->x_semaphore, 1, NULL);
#else
# ifdef ICI_USE_POSIX_THREADS
            sem_post(&x->x_semaphore);
# else
            /*
             * It is ok to do wakeup calls in implementations
             * with no thread support.
             */
# endif
#endif
        }
    }
    return 0;
}


#if defined(ICI_USE_WIN32_THREADS) || defined(ICI_USE_POSIX_THREADS)
/*
 * Entry point for a new thread. The passed argument is the pointer
 * to the execution context (ici_exec_t *). It has one ref count that is
 * now considered to be owned by this function. The operand stack
 * of the new context has the ICI function to be called configured on
 * it.
 */
static

#ifdef ICI_USE_WIN32_THREADS
DWORD WINAPI /* Ensure correct Win32 calling convention. */
ici_thread_base(void *arg)
{
    ici_exec_t      *x = (ici_exec_t *)arg;
#endif

#ifdef ICI_USE_POSIX_THREADS
void *
ici_thread_base(void *arg)
{
    ici_exec_t      *x = (ici_exec_t *)arg;
#endif

    int                 n_ops;

    ici_enter(x);
    n_ops = ici_os.a_top - ici_os.a_base;
    if ((x->x_result = ici_evaluate(&ici_o_call, n_ops)) == NULL)
    {
        x->x_result = ici_str_get_nul_term(ici_error);
        x->x_state = ICI_XS_FAILED;
        fprintf(stderr, "Warning, uncaught error in sub-thread: %s\n", ici_error);
    }
    else
    {
        ici_decref(x->x_result);
        x->x_state = ICI_XS_RETURNED;
    }
    ici_wakeup(x);
    ici_decref(x);
    (void)ici_leave();
    return 0;
}
#endif /* #if defined(ICI_USE_WIN32_THREADS) || defined(ICI_USE_POSIX_THREADS) */

/*
 * From ICI: exec = go(callable, arg1, arg2, ...)
 */
static int
f_go(...)
{
    ici_exec_t          *x;
    int                 i;

    if (ICI_NARGS() < 1 || !ici_typeof(ICI_ARG(0))->can_call())
        return ici_argerror(0);

    if ((x = ici_new_exec()) == NULL)
        return 1;
    /*
     * Copy the most-recently-executed source marker to the new thread
     * to give a useful indication for any errors during startup.
     */
    x->x_src = ici_exec->x_src;
    /*
     * Copy all the arguments to the operand stack of the new thread.
     */
    if (ici_stk_push_chk(x->x_os, ICI_NARGS() + 80))
        goto fail;
    for (i = 1; i < ICI_NARGS(); ++i)
        x->x_os->a_top[ICI_NARGS() - i - 1] = ICI_ARG(i);
    x->x_os->a_top += ICI_NARGS() - 1;
    /*
     * Now push the number of actuals and the object to call on the
     * new operand stack.
     */
    if ((*x->x_os->a_top = ici_int_new(ICI_NARGS() - 1)) == NULL)
        goto fail;
    ici_decref(*x->x_os->a_top);
    ++x->x_os->a_top;
    *x->x_os->a_top++ = ICI_ARG(0);
    /*
     * Create the native machine thread. We ici_incref x to give the new thread
     * it's own reference.
     */
    ici_incref(x);
#ifdef ICI_USE_WIN32_THREADS
    {
        HANDLE          thread_h;
        unsigned long   thread_id;

        thread_h = CreateThread(NULL, 1024, ici_thread_base, x, 0, &thread_id);
        if (thread_h == NULL)
        {
            ici_get_last_win32_error();
            ici_decref(x); /* The ref the thread was going to own. */
            goto fail;
        }
        x->x_thread_handle = thread_h;
    }
#else
# ifdef ICI_USE_POSIX_THREADS
    {
        pthread_attr_t thread_attr;
        if
        (
            (errno = pthread_attr_init(&thread_attr)) != 0
            ||
            (errno = pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED)) != 0
        )
        {
            ici_get_last_errno("create thread", NULL);
            ici_decref(x);
            goto fail;
        }
        if ((errno = pthread_create(&x->x_thread_handle, &thread_attr, ici_thread_base, x)) != 0)
        {
            ici_get_last_errno("create thread", NULL);
            pthread_attr_destroy(&thread_attr);
            ici_decref(x);
            goto fail;
        }
        pthread_attr_destroy(&thread_attr);
    }
# else
    ici_set_error("this implementation does not support thread creation");
    goto fail;
# endif
#endif
    return ici_ret_with_decref(x);

fail:
    ici_decref(x);
    return 1;
}

static int
f_wakeup(...)
{
    if (ICI_NARGS() != 1)
        return ici_argcount(1);
    if (ici_wakeup(ICI_ARG(0)))
        return 1;
    return ici_null_ret();
}

/*
 * Perform any OS specific initialisations concerning thread support.
 * Called once from ici_init() before the first execution context is
 * made.
 */
int
ici_init_thread_stuff()
{
#ifdef ICI_USE_WIN32_THREADS
    if ((ici_mutex = CreateMutex(NULL, 0, NULL)) == NULL)
        return ici_get_last_win32_error();
#endif
#ifdef ICI_USE_POSIX_THREADS
    pthread_mutexattr_t mutex_attr;

    if ((errno = pthread_mutexattr_init(&mutex_attr)) != 0)
    {
        ici_get_last_errno("mutex", NULL);
        return 1;
    }
    if (pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE) != 0)
        abort();
    if ((errno = pthread_mutex_init(&ici_mutex, &mutex_attr)) != 0)
    {
        ici_get_last_errno("mutex", NULL);
        pthread_mutexattr_destroy(&mutex_attr);
        return 1;
    }
    pthread_mutexattr_destroy(&mutex_attr);
#endif
    return 0;
}

ICI_DEFINE_CFUNCS(thread)
{
    ICI_DEFINE_CFUNC(go,            f_go),
    ICI_DEFINE_CFUNC(wakeup,        f_wakeup),
    ICI_CFUNCS_END
};

} // namespace ici
