#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <str.h>
#include <exec.h>
#include <op.h>
#include <func.h>

static int
f_fcntl(void)
{
    long	fd;
    string_t	*what;
    long	arg;
    int		iwhat;
    int		r;
    
    switch (NARGS())
    {
    case 2:
	arg = 1;
	if (ici_typecheck("io", &fd, &what))
	    return 1;
	break;
    case 3:
	if (ici_typecheck("ioi", &fd, &what, &arg))
	    return 1;
	break;
    default:
	return ici_argcount(3);
    }
    if (!isstring(objof(what)))
	return ici_argerror(1);
    if (what == new_cname("dupfd"))
	iwhat = F_DUPFD;
    else if (what == new_cname("getfd"))
	iwhat = F_GETFD;
    else if (what == new_cname("setfd"))
	iwhat = F_SETFD;
    else if (what == new_cname("getfl"))
	iwhat = F_GETFL;
    else if (what == new_cname("setfl"))
	iwhat = F_SETFL;
    else if (what == new_cname("getown"))
	iwhat = F_GETOWN;
    else if (what == new_cname("setown"))
	iwhat = F_SETOWN;
    else
	return ici_argerror(1);
    if ((r = fcntl(fd, iwhat, arg)) == -1)
    {
	error = strerror(errno);
	return 1;
    }
    return int_ret(r);
}

object_t *
ici_fcntl_library_init(void)
{
    static cfunc_t cfunc = {CF_OBJ, "fcntl", f_fcntl};
#ifdef ici3
    cfunc.o_head.o_type = &ici_cfunc_type;
#else
    cfunc.o_head.o_type = &func_type;
#endif
    return objof(&cfunc);
}
