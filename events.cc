#define ICI_CORE
#include "fwd.h"

#ifndef NOEVENTS
#include "exec.h"
#include "str.h"
#include "cfunc.h"

#ifdef  _WIN32
#include <windows.h>
#endif

namespace ici
{

#ifdef __APPLE__
static int f_eventloop() {
    return null_ret();
}
#endif

#ifdef  _WIN32
/*
 * Win32 specific event processing.
 */

static int
f_eventloop()
{
    MSG   msg;
    exec  *x;

    x = leave();
    for (;;)
    {
        switch (GetMessage(&msg, nullptr, 0, 0))
        {
        case 0:
            enter(x);
            return null_ret();

        case -1:
            enter(x);
            return ici_get_last_win32_error();
        }
        enter(x);
        TranslateMessage(&msg);
        clear_error();
        if (DispatchMessage(&msg) == ICI_EVENT_ERROR && error)
            return 1;
        x = leave();
    }
}

#endif /* _WIN32 */

ICI_DEFINE_CFUNCS(event)
{
    ICI_DEFINE_CFUNC(eventloop, f_eventloop),
    ICI_CFUNCS_END()
};

} // namespace ici

#endif /* NOEVENTS */
