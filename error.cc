#define ICI_CORE
#include "fwd.h"
#include "error.h"

namespace ici
{

int
ici_set_error(const char *fmt, ...)
{
    static char msg[ICI_MAX_ERROR_MSG]; /* FIXME: SHOULD BE PER-THREAD IF ici_error FIXED */
    va_list     va;

    va_start(va, fmt);
    vsnprintf(msg, sizeof msg, fmt, va);
    va_end(va);
    ici_error = msg;
    return 1;
}

} // namespace ici
