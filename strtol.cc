#define ICI_CORE
#include "fwd.h"
#include <ctype.h>

namespace ici
{

/*
 * xstrtol
 *
 * ANSI strtol() handles underflow/overflow during conversion by clamping the
 * result to LONG_MIN/LONG_MAX.
 *
 * Ici's int's are 64bit _signed_ for the purposes of arithmetic, but may
 * be treated as unsigned for input/output. This subtle variation on strtol
 * uses strtoul to allow such numbers as 0xFFFFFFFF and even -0xFFFFFFFF.
 *
 * This is not the exact prototype as the ANSI function. How do they
 * return a pointer into a const string through a non-const pointer?
 * (Without a cast.)
 */
int64_t xstrtol(char const *s, char **ptr, int base)
{
    uint64_t       v;
    char const     *eptr;
    char const     *start;
    int            minus;

    start = s;
    minus = 0;
    while (isspace((int)*s))
        s++;
    if ((minus = (*s == '-')) || (*s == '+'))
        s++;
    v = strtoull(s, (char **)&eptr, base);
    if (ptr != NULL)
       *ptr = (char *)((eptr == s) ? start : eptr);
    return minus ? -(long)v : (long)v;
}

} // namespace ici
