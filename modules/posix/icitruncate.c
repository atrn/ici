#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <str.h>
#include <exec.h>
#include <op.h>
#include <func.h>

/*
 * truncate(int|string, int)
 */
static int
f_truncate(void)
{
    long	fd;
    long	len;
    char	*s;
    int		r;

    errno = 0;

    if (ici_typecheck("ii", &fd, &len) == 0)
    {
	if (ftruncate(fd, len) == -1)
	    goto fail;
	return null_ret();
    }

    if (ici_typecheck("si", &s, &len) == 0)
    {
	if (truncate(s, len) == -1)
	    goto fail;
	return null_ret();
    }

    return 1;

fail:
    error = strerror(errno);
    return 1;
}

object_t *
ici_truncate_library_init(void)
{
    static cfunc_t cfunc = {CF_OBJ, "truncate", f_truncate};
#ifdef ici3
    cfunc.o_head.o_type = &ici_cfunc_type;
#else
    cfunc.o_head.o_type = &func_type;
#endif
    return objof(&cfunc);
}
