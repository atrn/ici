#include <ici.h>

/*
 * string = ungetc(c [, file])
 */

static int
f_ungetc(void)
{
    char	*ch;
    file_t	*file;
    int		rc;

    if (NARGS() == 1)
    {
	if (ici_typecheck("s", &ch))
	    return 1;
	if ((file = need_stdin()) == NULL)
	    return 1;
    }
    else
    {
	if (ici_typecheck("su", &ch, &file))
	    return 1;
    }
    rc = file->f_type->ft_ungetch(*ch, file->f_file);
    return int_ret(rc);
}

object_t *
ici_ungetc_library_init(void)
{
    static cfunc_t cfunc = {CF_OBJ, "ungetc", f_ungetc};
#if ici3
    cfunc.o_head.o_type = &ici_cfunc_type;
#else
    cfunc.o_head.o_type = &func_type;
#endif
    return objof(&cfunc);
}
