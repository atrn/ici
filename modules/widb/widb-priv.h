/*
 * Dealing with errors is generally done via _ASSERT() and VERIFY().  This
 * is just a debugger after all.
 */
#include <crtdbg.h>
#ifdef _DEBUG
    #define VERIFY(a) _ASSERT(a)
#else
    #define VERIFY(a) (a)
#endif

/*
 * Handle to the module containing resources for the debugger and associated
 * code.
 */
HINSTANCE widb_resources;