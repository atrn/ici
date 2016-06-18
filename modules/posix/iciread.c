#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <str.h>
#include <exec.h>
#include <op.h>
#include <func.h>

/*
 * string = read(int, int)
 */
static int
f_read(void)
{
    long	fd;
    long	len;
    string_t	*s;
    int		r;
    char	*msg;

    if (ici_typecheck("ii", &fd, &len))
	return 1;
    if ((msg = ici_alloc(len+1)) == NULL)
	return 1;
    switch (r = read(fd, msg, len))
    {
    case -1:
	ici_free(msg);
	error = strerror(errno);
	return 1;

    case 0:
	ici_free(msg);
	return null_ret();
    }
    if ((s = new_string(r)) == NULL)
    {
	ici_free(msg);
	return 1;
    }
    memcpy(s->s_chars, msg, r);
    ici_hash_string(objof(s));
    ici_free(msg);
    return ici_ret_with_decref(objof(s));
}

object_t *
ici_read_library_init(void)
{
    static cfunc_t cfunc = {CF_OBJ, "read", f_read};
#ifdef ici3
    cfunc.o_head.o_type = &ici_cfunc_type;
#else
    cfunc.o_head.o_type = &func_type;
#endif
    return objof(&cfunc);
}
