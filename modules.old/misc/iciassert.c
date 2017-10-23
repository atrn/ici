#include <ici.h>

/*
 * assert(int [, string])
 */
static int
f_assert(void)
{
    char	*message;
    long	expr;

    if (NARGS() == 1)
    {
	if (ici_typecheck("i", &expr))
	    return 1;
	message = "assertion failed";
    }
    else
    {
	if (ici_typecheck("is", &expr, &message))
	    return 1;
    }
    if (expr == 0)
    {
	error = message;
	return 1;
    }
    return null_ret();
}

object_t *
ici_assert_library_init(void)
{
    static cfunc_t cfunc = {CF_OBJ, "assert", f_assert};
#if ici3
    cfunc.o_head.o_type = &ici_cfunc_type;
#else
    cfunc.o_head.o_type = &func_type;
#endif
    return objof(&cfunc);
}
