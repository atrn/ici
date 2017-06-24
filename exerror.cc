#define ICI_CORE
#include "fwd.h"
#include "str.h"
#include "buf.h"

namespace ici
{

/*
 * Expand the current error string (assumed non-NULL) to include more
 * information.  But only if it seems not to contain it already.  Zero
 * argument values are ignored.
 */
void expand_error(int lineno, str *fname)
{
    char        *s;
    int         z;

    s = strchr(error + 2, ':');
    if (s != NULL && s > error && s[-1] >= '0' && s[-1] <= '9')
        return;

    /*
     * Expand the error to include the module and function,
     * but shuffle it through some new memory because it might
     * currently be in the standard buffer.
     */
    z = strlen(error) + (fname == NULL ? 0 : fname->s_nchars) + 20;
    if ((s = (char *)ici_nalloc(z)) == NULL)
        return;
    if (lineno != 0)
    {
        if (fname != NULL && fname->s_nchars > 0)
            sprintf(s, "%s, %d: %s", fname->s_chars, lineno, error);
        else
            sprintf(s, "%d: %s", lineno, error);
    }
    else
    {
        if (fname != NULL && fname->s_nchars > 0)
            sprintf(s, "%s: %s", fname->s_chars, error);
        else
            sprintf(s, "%s", error);
    }
    if (chkbuf(strlen(s)))
        return;
    strcpy(buf, s);
    ici_nfree(s, z);
    error = buf;
}

} // namespace ici
