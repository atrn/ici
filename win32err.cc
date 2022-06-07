#define ICI_CORE
#include "buf.h"
#include "fwd.h"

#ifdef _WIN32
#include <windows.h>

namespace ici
{

/*
 * Windows only.  Convert the current Win32 error (that is, the value of
 * 'GetLastError()') into an ICI error message and sets error to point to
 * it.  Returns 1 so it can be use directly in a return from an ICI instrinsic
 * function.
 *
 * This --func-- forms part of the --ici-api--.
 */
int ici_get_last_win32_error()
{
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, GetLastError(),
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                  buf, bufz, nullptr);
    return set_error("%s", buf);
}

} // namespace ici

#endif /* _WIN32 */
