#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <exec.h>
#include <op.h>
#include <func.h>

static int
f_mkfifo(void)
{
    char	*path;
    long	mode;

    if (ici_typecheck("si", &path, &mode))
	return 1;
    if (mkfifo(path, mode) == -1)
    {
	error = strerror(errno);
	return 1;
    }
    return ici_ret_no_decref(objof(&o_null));
}

object_t *
ici_mkfifo_library_init(void)
{
    static cfunc_t cfunc = {CF_OBJ, "mkfifo", f_mkfifo};
#ifdef ici3
    cfunc.o_head.o_type = &ici_cfunc_type;
#else
    cfunc.o_head.o_type = &func_type;
#endif
    return objof(&cfunc);
}
