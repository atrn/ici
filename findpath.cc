#define ICI_CORE
#include "fwd.h"
#include "array.h"
#include "str.h"
#ifndef _WIN32
#include <unistd.h> /* access(2) prototype */
#else
#include <io.h>
#endif

namespace ici
{

/*
 * Search for the given file called 'name', with the optional extension 'ext',
 * on our path (that is, the current value of 'path' in the current scope).
 * 'name' must point to a buffer of at least FILENAME_MAX chars which will be
 * overwritten with the full file name should it be found.  'ext' must be less
 * than 10 chars long and include any leading dot (or NULL if not required).
 * Returns 1 if the expansion was made, else 0, never errors.
 */
int find_on_path(char name[FILENAME_MAX], const char *ext)
{
    array   *a;
    char    *p;
    int      xlen;
    object **e;
    str     *s;
    char     realname[FILENAME_MAX];

    if ((a = need_path()) == NULL)
        return 0;
    xlen = 1 + strlen(name) + (ext != NULL ? strlen(ext) : 0) + 1;
    for (e = a->astart(); e != a->alimit(); e = a->anext(e))
    {
        if (!isstring(*e))
            continue;
        s = stringof(*e);
        if (s->s_nchars + xlen > FILENAME_MAX)
            continue;
        strcpy(realname, s->s_chars);
        p = realname + s->s_nchars;
        *p++ = ICI_DIR_SEP;
        strcpy(p, name);
        if (ext != NULL)
            strcat(p, ext);
        if (access(realname, 0) == 0)
        {
            strcpy(name, realname);
            return 1;
        }
    }
    return 0;
}

} // namespace ici
