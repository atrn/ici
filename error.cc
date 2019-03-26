#define ICI_CORE
#include "fwd.h"
#include "error.h"
#include "exec.h"

namespace ici {

int set_errorv(const char *fmt, va_list va) {
    vsnprintf(ex->x_buf, sizeof ex->x_buf, fmt, va);
    error = ex->x_buf;
    return 1;
}

int set_error(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    int result = set_errorv(fmt, va);
    va_end(va);
    return result;
}

void clear_error() {
    error = nullptr;
}

} // namespace ici
