#define ICI_CORE
#include "fwd.h"

#ifndef NOEVENTS
#include "exec.h"
#include "str.h"
#include "func.h"

#ifdef  _WIN32
#include <windows.h>
#endif

namespace ici
{

#ifdef  _WIN32
/*
 * Win32 specific event processing.
 */

static int
f_eventloop()
{
    MSG         msg;
    ici_exec_t  *x;

    x = ici_leave();
    for (;;)
    {
        switch (GetMessage(&msg, NULL, 0, 0))
        {
        case 0:
            ici_enter(x);
            return ici_null_ret();

        case -1:
            ici_enter(x);
            return ici_get_last_win32_error();
        }
        ici_enter(x);
        TranslateMessage(&msg);
        ici_error = NULL;
        if (DispatchMessage(&msg) == ICI_EVENT_ERROR && ici_error != NULL)
            return 1;
        x = ici_leave();
    }
}

#endif /* _WIN32 */

ici_cfunc_t ici_event_cfuncs[] =
{
    ICI_DEFINE_CFUNC(eventloop, f_eventloop),
    ICI_CFUNCS_END
};

} // namespace ici

#endif /* NOEVENTS */
