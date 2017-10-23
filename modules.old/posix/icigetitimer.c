#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <str.h>
#include <exec.h>
#include <op.h>
#include <func.h>

static string_t	*string_real;
static string_t	*string_virtual;
static string_t	*string_prof;
static string_t	*string_interval;
static string_t	*string_value;
static string_t	*string_sec;
static string_t	*string_usec;

static int	need_strings = 1;

static int
get_strings(void)
{
    if (need_string(&string_real, "real"))
	return 1;
    if (need_string(&string_virtual, "virtual"))
	return 1;
    if (need_string(&string_prof, "prof"))
	return 1;
    if (need_string(&string_interval, "interval"))
	return 1;
    if (need_string(&string_value, "value"))
	return 1;
    if (need_string(&string_sec, "sec"))
	return 1;
    if (need_string(&string_usec, "usec"))
	return 1;
    return need_strings = 0;
}

static int
assign_timeval(struct_t *s, string_t *k, struct timeval *tv)
{
    struct_t	*ss;
    int_t	*i;

    if ((ss = new_struct()) == NULL)
	return 1;
    if ((i = new_int(tv->tv_usec)) == NULL)
	goto fail;
    if (assign(ss, string_usec, i))
    {
	decref(i);
	goto fail;
    }
    decref(i);
    if ((i = new_int(tv->tv_sec)) == NULL)
	goto fail;
    if (assign(ss, string_sec, i))
    {
	decref(i);
	goto fail;
    }
    decref(i);
    if (assign(s, k, ss))
	goto fail;
    return 0;

fail:
    decref(ss);
    return 1;
}

/*
 * struct = getitimer(string)
 *
 */
static int
f_getitimer(void)
{
    long		which = ITIMER_VIRTUAL;
    struct_t		*s;
    struct itimerval	value;
    object_t		*o;

    if (need_strings && get_strings())
	return 1;
    if (NARGS() != 0)
    {
	if (ici_typecheck("o", &o))
	    return 1;
	if (!isstring(o))
	    return ici_argerror(0);
	if (o == objof(string_real))
	    which = ITIMER_REAL;
	else if (o == objof(string_virtual))
	    which = ITIMER_VIRTUAL;
	else if (o == objof(string_prof))
	    which = ITIMER_PROF;
	else
	    return ici_argerror(0);
    }
    if (getitimer(which, &value) == -1)
    {
	error = strerror(errno);
	return 1;
    }
    if ((s = new_struct()) == NULL)
	return 1;
    if
    (
	assign_timeval(s, string_interval, &value.it_interval)
	||
	assign_timeval(s, string_value, &value.it_value)
    )
    {
	decref(s);
	return 1;
    }
    return ici_ret_with_decref(objof(s));
}

object_t *
ici_getitimer_library_init(void)
{
    static cfunc_t cfunc = {CF_OBJ, "getitimer", f_getitimer};
#ifdef ici3
    cfunc.o_head.o_type = &ici_cfunc_type;
#else
    cfunc.o_head.o_type = &func_type;
#endif
    return objof(&cfunc);
}
