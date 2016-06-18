#define ICI_CORE
#include "fwd.h"
#include "buf.h"

char    *ici_buf;       /* #define'd to buf in buf.h. */
int     ici_bufz;       /* 1 less than actual allocation. */

/*
 * Ensure that the global buf has room for n chars. Return 1 on erorr,
 * else 0. This is not normally called directly, but rather via the
 * macro ici_chkbuf().
 */
int
ici_growbuf(int n)
{
    char       *p;

    if (ici_bufz > n)
        return 0;
    n = (n + 2) * 2;
    if ((p = (char *)ici_nalloc(n)) == NULL)
        return 1;
    if (ici_buf != NULL)
    {
        memcpy(p, ici_buf, ici_bufz);
        ici_nfree(ici_buf, ici_bufz + 1);
    }
    ici_buf = p;
    ici_bufz = n - 1;
    return 0;
}
