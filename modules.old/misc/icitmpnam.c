#include <ici.h>

/*
 * string = tmpnam()
 *
 * Return a name for a temporary file.
 */
static int
f_tmpnam(void)
{
	return str_ret(tmpnam(NULL));
}

object_t *
ici_tmpnam_library_init(void)
{
    static cfunc_t cfunc = {CF_OBJ, "tmpnam", f_tmpnam};
#if ici3
    cfunc.o_head.o_type = &ici_cfunc_type;
#else
    cfunc.o_head.o_type = &func_type;
#endif
    return objof(&cfunc);
}
