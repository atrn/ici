#define ICI_CORE
#include "fwd.h"
#include "exec.h"

namespace ici
{

/*
 * Stub functions for the debugging interface.
 */

/*
 * debug_error - called when the program raises an error.
 *
 * Parameters:
 *
 *      err     the error being set.
 *      src     the last source marker encountered.
 */
static void
debug_error(char *, src *)
{
}

/*
 * debug_fncall - called prior to a function call.
 *
 * Parameters:
 *
 *      o       The function being called.
 *      ap      The parameters to function, a (C) array of objects.
 *      nargs   The number of parameters in that array.
 */
static void
debug_fncall(object *, object **, int)
{
}

/*
 * debug_fnresult - called upon function return.
 *
 * Parameters:
 *
 *      o       The result of the function.
 */
static void
debug_fnresult(object *)
{
}

/*
 * debug_src - called when a source line marker is encountered.
 *
 * Parameters:
 *
 *      src     The source marker encountered.
 */
static void
debug_src(src *)
{
}

/*
 * debug_watch - called upon each assignment.
 *
 * Parameters:
 *
 *      o       The object being assigned into. For normal variable
 *              assignments this will be a struct, part of the scope.
 *      k       The key being used, typically a string, the name of a
 *              variable.
 *      v       The value being assigned to the object.
 */
static void
debug_watch(object *, object *, object *)
{
}

/*
 * The default debugging interface is the stub functions.  Debuggers
 * will assign ici_debug to point to a more useful set of functions.
 */
debug debug_stubs =
{
    debug_error,
    debug_fncall,
    debug_fnresult,
    debug_src,
    debug_watch
};

debug *o_debug = &debug_stubs;

} // namespace ici
