#define ICI_CORE
#include "fwd.h"
#include "array.h"
#include "str.h"
#ifndef _WIN32
#include <unistd.h> /* access(2) prototype */
#else
#include <io.h>
#endif

/*
 * Search for the given file called 'name', with the optional extension 'ext',
 * on our path (that is, the current value of 'path' in the current scope).
 * 'name' must point to a buffer of at least FILENAME_MAX chars which will be
 * overwritten with the full file name should it be found.  'ext' must be less
 * than 10 chars long and include any leading dot (or NULL if not required).
 * Returns 1 if the expansion was made, else 0, never errors.
 */
int
ici_find_on_path(char name[FILENAME_MAX], const char *ext)
{
    ici_array_t         *a;
    char                *p;
    char                realname[FILENAME_MAX];
    int                 xlen;
    ici_obj_t           **e;
    ici_str_t           *s;

    if ((a = ici_need_path()) == NULL)
        return 0;
    xlen = 1 + strlen(name) + (ext != NULL ? strlen(ext) : 0) + 1;
    for (e = ici_astart(a); e != ici_alimit(a); e = ici_anext(a, e))
    {
        if (!ici_isstring(*e))
            continue;
        s = ici_stringof(*e);
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
