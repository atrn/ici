#include <ici.h>

#include <unistd.h>
#include <pwd.h>

/*
 * string = getpass([string])
 */
static int
f_getpass(void)
{
    char	*prompt = "Password: ";
    string_t	*pass;

    if (NARGS() > 0)
    {
	if (ici_typecheck("s", &prompt))
	    return 1;
    }
    return str_ret(getpass(prompt));
}

object_t *
ici_getpass_library_init(void)
{
    static cfunc_t cfunc = {CF_OBJ, "getpass", f_getpass};
#ifdef ici3
    cfunc.o_head.o_type = &ici_cfunc_type;
#else
    cfunc.o_head.o_type = &func_type;
#endif
    return objof(&cfunc);
}
