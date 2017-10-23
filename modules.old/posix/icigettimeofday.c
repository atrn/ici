#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <str.h>
#include <exec.h>
#include <op.h>
#include <func.h>

static string_t	*string_sec;
static string_t	*string_usec;

static int	need_strings = 1;

static int
get_strings(void)
{
    if (need_string(&string_sec, "sec"))
	return 1;
    if (need_string(&string_usec, "usec"))
	return 1;
    return need_strings = 0;
}

static int
assign_timeval(struct_t *s, struct timeval *tv)
{
    int_t	*i;

    if ((i = new_int(tv->tv_usec)) == NULL)
	goto fail;
    if (assign(s, string_usec, i))
    {
	decref(i);
	goto fail;
    }
    decref(i);
    if ((i = new_int(tv->tv_sec)) == NULL)
	goto fail;
    if (assign(s, string_sec, i))
    {
	decref(i);
	goto fail;
    }
    decref(i);
    return 0;

fail:
    return 1;
}

static int
f_gettimeofday(void)
{
    struct_t		*s;
    struct timeval	tv;

    if (need_strings && get_strings())
	return 1;
    if (gettimeofday(&tv, NULL) == -1)
    {
	error = strerror(errno);
	return 1;
    }
    if ((s = new_struct()) == NULL)
	return 1;
    if (assign_timeval(s, &tv))
    {
	decref(s);
	return 1;
    }
    return ici_ret_with_decref(objof(s));
}

object_t *
ici_gettimeofday_library_init(void)
{
    static cfunc_t cfunc = {CF_OBJ, "gettimeofday", f_gettimeofday};
#ifdef ici3
    cfunc.o_head.o_type = &ici_cfunc_type;
#else
    cfunc.o_head.o_type = &func_type;
#endif
    return objof(&cfunc);
}
