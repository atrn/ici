#define ICI_CORE
#include "fwd.h"
#include "buf.h"
#include <errno.h>

/*
 * Convert the current errno (that is, the standard C global error code) into
 * an ICI error message based on the standard C 'strerror' function.  Returns
 * 1 so it can be use directly in a return from an ICI instrinsic function or
 * similar.  If 'dothis' and/or 'tothis' are non-NULL, they are included in
 * the error message.  'dothis' should be a short name like "'open'".
 * 'tothis' is typically a file name.  The messages it sets are, depending on
 * which of 'dothis' and 'tothis' are NULL, the message will be one of:
 *
 *  strerror
 *  failed to dothis: strerror
 *  failed to dothis tothis: strerror
 *  tothis: strerror
 *
 * This --func-- forms part of the --ici-api--.
 */
int
ici_get_last_errno(const char *dothis, const char *tothis)
{
    const char          *e;

    if ((e = strerror(errno)) == NULL)
        e = "system call failure";
    if (dothis == NULL && tothis == NULL)
        return ici_set_error("%s", e);
    if (dothis != NULL && tothis == NULL)
        return ici_set_error("failed to %s: %s", dothis, e);
    if (dothis != NULL && tothis != NULL)
        return ici_set_error("failed to %s %s: %s", dothis, tothis, e);
    return ici_set_error("%s: %s", tothis, e);
}
