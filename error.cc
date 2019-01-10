#define ICI_CORE
#include "fwd.h"
#include "error.h"

namespace ici {

/*
 * The global error message pointer. The ICI error return convention
 * dictacts that the originator of an error sets this to point to a
 * short human readable string, in addition to returning the functions
 * error condition. See 'The error return convention' for more details.
 *
 * This --variable-- forms part of the --ici-api--.
 */
char *error = nullptr;

static char msg[max_error_msg]; /* FIXME: should be per-thread if error also per-thread. */

int set_errorv(const char *fmt, va_list va) {
    vsnprintf(msg, sizeof msg, fmt, va);
    error = msg;
    return 1;
}

int set_error(const char *fmt, ...) {
    va_list     va;

    va_start(va, fmt);
    int result = set_errorv(fmt, va);
    va_end(va);
    return result;
}

void clear_error() {
    error = nullptr;
}

} // namespace ici
