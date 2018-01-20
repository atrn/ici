#define ICI_CORE
#include "fwd.h"
#include "buf.h"

namespace ici
{

char    *buf;       /* #define'd to buf in buf.h. */
size_t  bufz;       /* 1 less than actual allocation. */

/*
 * Ensure that the global buf has room for n chars. Return 1 on erorr,
 * else 0. This is not normally called directly, but rather via the
 * function chkbuf().
 */
int growbuf(size_t n)
{
    char       *p;

    if (bufz > n)
        return 0;
    n = (n + 2) * 2;
    if ((p = (char *)ici_nalloc(n)) == nullptr)
        return 1;
    if (buf != nullptr)
    {
        memcpy(p, buf, bufz);
        ici_nfree(buf, bufz + 1);
    }
    buf = p;
    bufz = n - 1;
    return 0;
}

} // namespace ici
