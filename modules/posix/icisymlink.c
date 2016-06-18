#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <exec.h>
#include <op.h>
#include <func.h>

static int
f_symlink(void)
{
    char	*a, *b;

    if (ici_typecheck("ss", &a, &b))
	return 1;
    if (symlink(a, b) == -1)
    {
	error = strerror(errno);
	return 1;
    }
    return ici_ret_no_decref(objof(&o_null));
}

object_t *
ici_symlink_library_init(void)
{
    static cfunc_t cfunc = {CF_OBJ, "symlink", f_symlink};
#ifdef ici3
    cfunc.o_head.o_type = &ici_cfunc_type;
#else
    cfunc.o_head.o_type = &func_type;
#endif
    return objof(&cfunc);
}
