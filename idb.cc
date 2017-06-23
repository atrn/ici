#define ICI_CORE
#include "fwd.h"
#include "src.h"
#include "exec.h"
#include "int.h"
#include "struct.h"
#include "str.h"
#include "func.h"
#include "array.h"
#include "op.h"
#include "buf.h"
#include "file.h"

namespace ici
{

/*
 * Flag indicating if the user wants debugging enabled. Could be used
 * as a bit set to control which things get debugged.
 */
int     ici_debug_enabled = 0;

/*
 * Flag indicating if error trapping should be ignored.
 */
int ici_debug_ign_err = 0;

/*
 * Ignore errors within exec loop. Used by internal calls to
 * exec that handle errors themselves, e.g., f_include().
 */
void
ici_debug_ignore_errors()
{
    ici_debug_ign_err = 1;
}

/*
 * Restore error processing.
 */
void
ici_debug_respect_errors()
{
    ici_debug_ign_err = 0;
}

/*
 * int = debug([int])
 *
 * With no argument debug() returns the current debug status, an integer
 * which is zero if not debugging or non-zero if debugging is enabled. If
 * non-zero the value of the debug status variable may affect debugging
 * functionality.
 *
 * With an argument - only one, an integer - the debug status is set and
 * the old value returned. This may be used to get the current debug state
 * and set a new one in one operation which may be useful for functions that
 * don't want to be debugged or need to avoid the performance impact of
 * debugging.
 */
static int
f_debug(...)
{
    long        v, t;

    t = ici_debug_enabled;
    if (ICI_NARGS() != 0)
    {
        if (typecheck("i", &v))
            return 1;
        ici_debug_enabled = v;
    }
    return ici_int_ret(t);
}

ici_cfunc_t ici_debug_cfuncs[] =
{
    ICI_DEFINE_CFUNC(    debug,        f_debug),
    ICI_CFUNCS_END
};

} // namespace ici
