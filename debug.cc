#define ICI_CORE
#include "fwd.h"
#include "src.h"
#include "exec.h"
#include "int.h"
#include "map.h"
#include "str.h"
#include "func.h"
#include "cfunc.h"
#include "array.h"
#include "op.h"
#include "buf.h"
#include "file.h"

namespace ici
{

/*
 * Flag indicating if debugging is enabled.  If this is non-zero and
 * there is a current debugger instance it's functions are called.
 * This value is the result of the ICI debug() function.
 */
int debug_enabled = 0;

/*
 * Flag indicating if error catching should be ignored.
 */
int debug_ignore_err = 0;

/*
 * Ignore errors within exec loop. Used by internal calls to
 * exec that handle errors themselves, e.g., f_include().
 */
void debug_ignore_errors() {
    ++debug_ignore_err;
}

/*
 * Restore error processing.
 */
void debug_respect_errors() {
    --debug_ignore_err;
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
static int f_debug(...) {
    auto result = debug_enabled;
    if (NARGS() != 0) {
        int64_t v;
        if (typecheck("i", &v)) {
            return 1;
        }
        debug_enabled = v;
    }
    return int_ret(result);
}

ICI_DEFINE_CFUNCS(debug)
{
    ICI_DEFINE_CFUNC(debug, f_debug),
    ICI_CFUNCS_END()
};

} // namespace ici
