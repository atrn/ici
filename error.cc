#define ICI_CORE
#include "fwd.h"
#include "error.h"

namespace ici
{

static char msg[max_error_msg]; /* FIXME: should be per-thread if ici_error also per-thread. */

int
ici_set_error(const char *fmt, ...)
{
    va_list     va;

    va_start(va, fmt);
    vsnprintf(msg, sizeof msg, fmt, va);
    va_end(va);
    ici_error = msg;
    return 1;
}

} // namespace ici
