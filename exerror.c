#define ICI_CORE
#include "fwd.h"
#include "str.h"
#include "buf.h"

/*
 * Expand the current error string (assumed non-NULL) to include more
 * information.  But only if it seems not to contain it already.  Zero
 * argument values are ignored.
 */
void
ici_expand_error(int lineno, ici_str_t *fname)
{
    char        *s;
    int         z;

    s = strchr(ici_error + 2, ':');
    if (s != NULL && s > ici_error && s[-1] >= '0' && s[-1] <= '9')
        return;

    /*
     * Expand the error to include the module and function,
     * but shuffle it through some new memory because it might
     * currently be in the standard buffer.
     */
    z = strlen(ici_error) + (fname == NULL ? 0 : fname->s_nchars) + 20;
    if ((s = (char *)ici_nalloc(z)) == NULL)
        return;
    if (lineno != 0)
    {
        if (fname != NULL && fname->s_nchars > 0)
            sprintf(s, "%s, %d: %s", fname->s_chars, lineno, ici_error);
        else
            sprintf(s, "%d: %s", lineno, ici_error);
    }
    else
    {
        if (fname != NULL && fname->s_nchars > 0)
            sprintf(s, "%s: %s", fname->s_chars, ici_error);
        else
            sprintf(s, "%s", ici_error);
    }
    if (ici_chkbuf(strlen(s)))
        return;
    strcpy(buf, s);
    ici_nfree(s, z);
    ici_error = buf;
}
