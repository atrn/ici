#include <sys/types.h>
#include <sys/param.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <exec.h>
#include <op.h>
#include <func.h>

static int
f_readlink(void)
{
    char	*path;
    char	pbuf[MAXPATHLEN+1];

    if (ici_typecheck("s", &path))
	return 1;
    if (readlink(path, pbuf, sizeof pbuf) == -1)
    {
	error = strerror(errno);
	return 1;
    }
    return ici_ret_with_decref(objof(new_cname(pbuf)));
}

object_t *
ici_readlink_library_init(void)
{
    static cfunc_t cfunc = {CF_OBJ, "readlink", f_readlink};
#ifdef ici3
    cfunc.o_head.o_type = &ici_cfunc_type;
#else
    cfunc.o_head.o_type = &func_type;
#endif
    return objof(&cfunc);
}
