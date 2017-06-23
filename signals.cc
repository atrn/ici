/*
 * Signal handling code for ici.
 *
 * The code here provides ici programs the ability to trap and handle
 * process signals using ici code. The basic idea is to trap all signals
 * in the C and simply note that a signal has occurred. At a point around
 * the ici execution loop we check if any signals need processing and
 * go and do them. A signal may be set to its default operation, to be
 * ignored or to have an ici function associated with the signal. In the
 * latter case when the signal is delivered and is recognised the function
 * associated with the signal is called. Signal handler functions are passed
 * a single argument, the signal number (integer) of the signal being
 * handled.
 *
 * There is a variation on the above signal handling for situations where
 * the ici execution loop may not be called quickly enough. Blocking system
 * calls are a prime example of this, if the program blocks waiting for,
 * say, user input them other signals do not get processed. To overcome
 * this a new function is introduced that is supposed to be called to
 * define such points. The function allows ici signal handling code to
 * be invoked directly from the C signal handler. This function should
 * only be used when the program, i.e., an ici c-func implementation,
 * knows that the next thing they are doing will take some arbitrarially
 * long amount of time. This is fortunately more rare than first thought.
 *
 * Andy Newman <atrn@zeta.org.au>
 */
#define ICI_CORE
#include "fwd.h"

#include "exec.h"
#include "op.h"
#include "func.h"
#include "str.h"
#include "method.h"
#include "str.h"
#include "int.h"

#include <errno.h>
#include <signal.h>

#ifdef __linux__
#include <string.h>
#endif

namespace ici
{

/*
 * ici_signals_pending
 *
 *  A bit-set indicating if any signals are pending.  Used
 *  externally to determine if there is a need to call the
 *  function to process signals.
 *
 * ici_signals_count
 *
 *  A per-signal counter that is presently unused. This is
 *  intended to count the number of signals that occur between
 *  handlers being invoked.
 */

volatile sigset_t ici_signals_pending;
volatile long   ici_signal_count[NSIG];


/*
 * Internally we keep,
 *
 *  signal_handler
 *
 *      An array of pointers to functions, one per signal. If non-NULL
 *      the signal is being handled by an ici function object to by
 *      the pointer.
 *
 *  currently_blocked
 *
 *      True if ici is currently blocked waiting for some external
 *      event and the execution loop is not being processed. This
 *      is really used to call signal handlers immediately the signal
 *      occurs rather than postponing the call to within the execution
 *      loop.
 */
static ici_obj_t *signal_handler[NSIG];
static int  currently_blocked;


/*
 * Call an ici signal handler function for the specified signal.
 * This sets up an ici call operator to invoke the function passing
 * the signal number as a parameter.
 *
 * Returns non-zero on error, zero if okay.
 */
static int
call_signal_handler(ici_obj_t *func, int signo)
{
    ici_int_t           *isigno;
    ici_obj_t           *ret_obj;

    if (ici_os.stk_push_chk(3 + 80)) /* see comment in ici/call.c */
        return 1;
    if ((isigno = ici_int_new(signo)) == NULL)
        return 1;
    *ici_os.a_top++ = isigno;
    isigno->decref();
    *ici_os.a_top++ = ici_one; /* One argument. */
    *ici_os.a_top++ = func;
    if ((ret_obj = ici_evaluate(&ici_o_call, 3)) == NULL)
        goto fail;
    ret_obj->decref();
    return 0;

fail:
    return 1;
}


/*
 * Macros/functions to convert between signal numbers and
 * various other forms. Signals are numbered between 1 and
 * NSIG. Arrays want an index from 0 to NSIG-1 and rather
 * than waste the first element the following macros are
 * used to convert between uses.
 */

#define     index_to_signo(N)   ((N)+1)
#define     signo_to_index(N)   ((N)-1)


/*
 * Names for signals. Taken from the macros in sys/signal.h,
 * converted to lower case and the "sig" removed. This, of
 * course, needs to be in signal order.
 */
#ifdef __sun
static const char * const signal_names[NSIG] =
{
    "hup",  "int",      "quit",     "ill",      "trap",
    "abrt", "emt",      "fpe",      "kill",     "bus",
    "segv", "sys",      "pipe",     "alrm",     "term",
    "usr1", "usr2",     "chld",     "pwr",      "winch",
    "urg",  "poll",     "stop",     "tstp",     "cont",
    "ttin", "ttou",     "vtalrm",   "prof",     "xcpu",
    "xfsz", "waiting",  "lwp",      "freeze",   "thaw",
    "cancel",   "",     "",     "",     "",
    "",     "",     "",     "",     "",
    ""
};
#else
# if ! defined(BSD) && !defined(__linux__)
static char *signal_names[NSIG] =
{
    "hup",  "int",      "quit",     "ill",
    "trap", "abrt",     "iot",      "emt",
    "fpe",  "kill",     "bus",      "segv",
    "sys",  "pipe",     "alrm",     "term",
    "urg",  "stop",     "tstp",     "cont",
    "chld", "ttin",     "ttou",     "io",
    "xcpu", "xfsz",     "vtalrm",   "prof",
    "winch",    "info",     "usr1",     "usr2"
};
# endif
#endif

/*
 * Modern BSD systems define the signal names in the sys_signame
 * table so we use that if possible. 
 */
#ifdef __linux__
# define signal_names sys_siglist
#elif defined BSD
# define signal_names sys_signame
#endif

/*
 * Given a signal name return the signal number. Understands
 * upper- and lower-case names and names with a "sig" prefix
 * or not.
 *
 * Returns a signal number or zero on error (no such signal).
 */
static int
signam_to_signo(char *nam)
{
    int signo;

    if (strncasecmp(nam, "sig", 3) == 0)
        nam += 3;
    for (signo = 0; signo < NSIG; ++signo)
    {
        if (strcasecmp(nam, signal_names[signo]) == 0)
            return index_to_signo(signo);
    }
    return 0;
}


/*
 * Map a signal number to a name.
 *
 * Returns a name for the given signal or NULL if the signal number
 * is invalid.
 */
static const char *
signo_to_signam(int signo)
{
#ifdef __linux__
    return strsignal(signo);
#else
    if (signo < 1 || signo >= NSIG)
        return NULL;
    return signal_names[signo_to_index(signo)];
#endif
}


/*
 * This is the signal handler function invoked when a signal
 * we are handling is delivered to the process.
 *
 * If the process is currently blocked the ici signal handler
 * is invoked immediately otherwise note is taken of the signal
 * and it is handled via other means.
 */
static void
ici_signal_handler(int signo)
{
    ici_obj_t   *func;

    if (currently_blocked)
    {
        func = signal_handler[signo_to_index(signo)];
        if (func != NULL)
        {
            if (call_signal_handler(func, signo))
            {
                /* NOTHING */
            }
        }
    }
    else
    {
#if defined(__sun) || defined(__FreeBSD__) || defined(__linux__)
        sigaddset((sigset_t *)&ici_signals_pending, signo);
#else
        ici_signals_pending |= sigmask(signo);
#endif
        ++ici_signal_count[signo_to_index(signo)];
    }
}


/*
 * Initialize ici's signal handling.
 */
void
ici_signals_init()
{
    int     signo;

#if defined(__sun) || defined(__FreeBSD__) || defined(__linux__)
    memset((void *)&ici_signals_pending, 0, sizeof(sigset_t));
#else
    ici_signals_pending = 0;
#endif
    currently_blocked = 0;
    for (signo = 0; signo < NSIG; ++signo)
    {
        signal_handler[signo] = NULL;
        ici_signal_count[signo] = 0;
    }
}


/*
 * Tell signals code to invoke handlers directly as
 * we're probably blocked in a system call and the
 * execution loop isn't being processed.
 */
int
ici_signals_blocking_syscall(int state)
{
    int previous;

    previous = currently_blocked;
    currently_blocked = state;
    return previous;
}


/*
 * Call handlers for any pending signals.
 */
int
ici_signals_invoke_handlers()
{
    int     signo;
    ici_obj_t   *fn;

    for (signo = 1; signo <= NSIG; ++signo)
    {
#if defined(__sun) || defined(__FreeBSD__) || defined(__linux__)
        if (sigismember((sigset_t *)&ici_signals_pending, signo))
        {
	    sigdelset((sigset_t *)&ici_signals_pending, signo);
            if ((fn = signal_handler[signo_to_index(signo)]) != NULL)
                if (call_signal_handler(fn, signo))
                    return 1;
            ici_signal_count[signo_to_index(signo)] = 0;
        }
#else
        long mask = sigmask(signo);
        if (ici_signals_pending & mask)
        {
            ici_signals_pending &= ~mask;
            if ((fn = signal_handler[signo_to_index(signo)]) != NULL)
                if (call_signal_handler(fn, signo))
                    return 1;
            ici_signal_count[signo_to_index(signo)] = 0;
        }
#endif
    }
    return 0;
}

/*
 * string|func = signal(int|string [, func|string])
 *
 * Set or get signal handling state.  With a single argument, an int
 * or string giving the id. of a signal, the function returns the
 * current status of the signal - either a string ("default" or "ignore")
 * or a function that is handling the signal.  With two arguments
 * the state of the signal handling is set. The second argument is
 * either one of the strings "default" or "ignore" or is a function
 * that is to handle the single.
 */
static int
f_signal(...)
{
    ici_obj_t   *sigo;
    ici_obj_t   *handlero;
    int         signo;
    void        (*handler)(int);
    void        (*rc)(int);
    ici_obj_t   *prev_handler;
    ici_obj_t   *result;

    handlero = NULL;
    switch (ICI_NARGS())
    {
    case 2:
        handlero = ICI_ARG(1);
        /*FALLTHROUGH*/
    case 1:
        sigo = ICI_ARG(0);
        break;
    default:
        return ici_argcount(2);
    }

    if (ici_isstring(sigo))
    {
        if ((signo = signam_to_signo(ici_stringof(sigo)->s_chars)) == 0)
        {
            return ici_set_error("invalid signal name");
        }
    }
    else if (ici_isint(sigo))
    {
        signo = ici_intof(sigo)->i_value;
        if (signo < 1 || signo > NSIG)
        {
            return ici_set_error("invalid signal number");
        }
    }
    else
    {
        return ici_argerror(0);
    }

    prev_handler = signal_handler[signo_to_index(signo)];

    if (handlero == NULL)
    {
        if (prev_handler)
            return ici_ret_no_decref(handlero);
        signal(signo, handler = signal(signo, SIG_IGN));
        if (handler == ici_signal_handler)
        {
            return ici_set_error("signals messed up, unrecorded handler present");
        }
        if (handler == SIG_DFL)
            return ici_ret_no_decref(SS(default));
        if (handler == SIG_IGN)
            return ici_ret_no_decref(SS(ignore));
        return ici_set_error("signal in indeterminate state");
    }

    if (ici_stringof(handlero) == SS(default))
    {
        signal_handler[signo_to_index(signo)] = NULL;
        handler = SIG_DFL;
    }
    else if (ici_stringof(handlero) == SS(ignore))
    {
        signal_handler[signo_to_index(signo)] = NULL;
        handler = SIG_IGN;
    }
    else if (ici_isfunc(handlero) || ici_ismethod(handlero))
    {
        signal_handler[signo_to_index(signo)] = handlero;
        handlero->incref();
        handler = ici_signal_handler;
    }
    else
    {
        return ici_argerror(1);
    }

    rc = signal(signo, handler);

    if (rc == SIG_ERR)
    {
        signal_handler[signo_to_index(signo)] = prev_handler;
        return ici_set_error("%s", strerror(errno));
    }

    if (rc == ici_signal_handler)
        result = prev_handler;
    else if (rc == SIG_DFL)
        result = SS(default);
    else if (rc == SIG_IGN)
        result = SS(ignore);
    else
    {
        signal(signo, rc);
        return ici_set_error("unexpected result from signal");
    }

    return ici_ret_no_decref(result);
}


/*
 * string = signam(int)
 *
 * Map a signal number to a name. Raises an error if the signal
 * number is out of bounds.  This is a relatively low-cost method
 * of checking signal numbers.
 */
static int
f_signam(...)
{
    long  signo;
    char  *nam;

    if (typecheck("i", &signo))
        return 1;
    if ((nam = (char *)signo_to_signam(signo)) == NULL)
    {
        return ici_set_error("invalid signal number");
    }
    return ici_str_ret(nam);
}


/*
 * Our C-funcs
 */
ICI_DEFINE_CFUNCS(signals)
{
    ICI_DEFINE_CFUNC(signal,   f_signal),
    ICI_DEFINE_CFUNC(signam,   f_signam),
    ICI_CFUNCS_END
};

} // namespace ici
