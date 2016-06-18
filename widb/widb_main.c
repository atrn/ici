#include <stdlib.h>
#include "exec.h"

extern int	ici_maind();

/*
 * Make up for missing ANSI defines.
 */
#ifndef	EXIT_FAILURE
#define	EXIT_FAILURE	1
#endif
#ifndef	EXIT_SUCCESS
#define	EXIT_SUCCESS	0
#endif

int
main(int argc, char *argv[])
{
    extern ici_debug_t ici_debug_funcs;
    ici_debug = &ici_debug_funcs;

    if (ici_maind(argc, argv, 1))
	return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

#ifdef NSFIP
/*
 * memcmp (as per ANSI definition) was shipped broken on
 * NeXTStep for Intel Processors. This should be removed
 * sometime after it is fixed.
 */
int
memcmp(a, b, n)
char *a;
char *b;
int n;
{
    while (n > 0 && *a == *b)
    {
	++a;
	++b;
	--n;
    }
    return n == 0 ? 0 : *a < *b ? -1 : 1;
}
#endif

#if defined(sun) && !defined(__SVR4)
void *
memmove(void *s1, void const *s2, size_t n)
{
    char                *sc1;
    char const          *sc2;

    sc1 = s1;
    sc2 = s2;
    if (sc2 < sc1 && sc1 < sc2 + n)
	for (sc1 += n, sc2 += n; 0 < n; --n)
	    *--sc1 = *--sc2;            /* copy backwards */
    else
	for (; 0 < n; --n)
	    *sc1++ = *sc2++;            /* copy forwards */
    return s1;
}
#endif
