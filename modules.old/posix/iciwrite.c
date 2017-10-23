#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <str.h>
#include <exec.h>
#include <op.h>
#include <func.h>
#include <mem.h>

/*
 * int = write(int, string|mem [, len])
 */
static int
f_write(void)
{
    long	fd;
    object_t	*o;
    int		r;
    char	*addr;
    long	sz;
    int		havesz = 0;

    if (ici_typecheck("io", &fd, &o))
    {
	if (ici_typecheck("ioi", &fd, &o, &sz))
	    return 1;
	havesz = 1;
    }
    if (isstring(o))
    {
	addr = (char *)stringof(o)->s_chars;
	if (!havesz)
	    sz = stringof(o)->s_nchars;
    }
    else if (ismem(o))
    {
	addr = (char *)memof(o)->m_base;
	if (!havesz)
	    sz = memof(o)->m_length * memof(o)->m_accessz;
    }
    else
    {
	return ici_argerror(1);
    }
    if ((r = write((int)fd, addr, sz)) == -1)
    {
	error = strerror(errno);
	return 1;
    }
    return int_ret(r);
}

object_t *
ici_write_library_init(void)
{
    static cfunc_t cfunc = {CF_OBJ, "write", f_write};
#ifdef ici3
    cfunc.o_head.o_type = &ici_cfunc_type;
#else
    cfunc.o_head.o_type = &func_type;
#endif
    return objof(&cfunc);
}
