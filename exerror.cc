#define ICI_CORE
#include "fwd.h"
#include "str.h"
#include "buf.h"
#include "exec.h"

namespace ici
{

/*
 * Expand the current error string (assumed non-nullptr) to include more
 * information.  But only if it seems not to contain it already.  Zero
 * argument values are ignored.
 */
void expand_error(int lineno, str *fname)
{
    char        *s;
    int         z;

    s = strchr(ici_error + 1, ':');
    if (s != nullptr && s > ici_error && s[-1] >= '0' && s[-1] <= '9')
        return;

    /*
     * Expand the error to include the module and function,
     * but shuffle it through some new memory because it might
     * currently be in the standard buffer.
     */
    z = strlen(ici_error) + (fname == nullptr ? 0 : fname->s_nchars) + 20;
    if ((s = (char *)ici_nalloc(z)) == nullptr)
        return;
    if (lineno != 0)
    {
        if (fname != nullptr && fname->s_nchars > 0)
            sprintf(s, "%s, %d: %s", fname->s_chars, lineno, ici_error);
        else
            sprintf(s, "%d: %s", lineno, ici_error);
    }
    else
    {
        if (fname != nullptr && fname->s_nchars > 0)
            sprintf(s, "%s: %s", fname->s_chars, ici_error);
        else
            sprintf(s, "%s", ici_error);
    }
    if (chkbuf(strlen(s)))
        return;
    strcpy(buf, s);
    ici_nfree(s, z);
    set_error(buf);
}

} // namespace ici
