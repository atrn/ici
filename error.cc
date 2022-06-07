#define ICI_CORE
#include "error.h"
#include "exec.h"
#include "fwd.h"

namespace ici
{

const char *get_error()
{
    return ex->x_error;
}

int set_errorc(const char *p)
{
    ex->x_error = const_cast<char *>(p);
    return 1;
}

void clear_error()
{
    set_errorc(nullptr);
}

int set_errorv(const char *fmt, va_list va)
{
    vsnprintf(ex->x_buf, sizeof ex->x_buf, fmt, va);
    return set_errorc(ex->x_buf);
}

int set_error(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    int result = set_errorv(fmt, va);
    va_end(va);
    return result;
}

} // namespace ici
