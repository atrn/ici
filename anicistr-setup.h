/*
 * This somewhat special include file may be included to provide
 * defintions and initialisations (at run-time) of ICI string
 * objects that have been listed in the include file "icistr.h"
 * by the invoker. See ici.h or "The ICI Programming Language"
 * for further details.
 */
#undef  ICI_STR
#define ICI_STR ICI_STR_DECL
#include "icistr.h"

static int
init_ici_str(void)
{
#undef  ICI_STR
#define ICI_STR ICI_STR_MAKE
    return
#include "icistr.h"
    0;
}
#undef  ICI_STR
#define ICI_STR ICI_STR_NORM
