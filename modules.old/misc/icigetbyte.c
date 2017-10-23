#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <str.h>
#include <exec.h>
#include <op.h>
#include <func.h>
#include <file.h>

/*
 * NULL|int = getbyte(file)
 */
static int
f_getbyte(void)
{
    file_t	*f;
    int		c;

    if (ici_typecheck("u", &f))
	return 1;
    c = (*f->f_type->ft_getch)(f->f_file);
    if (c == EOF)
	return null_ret();
    return int_ret(c);
}

object_t *
ici_getbyte_library_init(void)
{
    static cfunc_t cfunc = {CF_OBJ, "getbyte", f_getbyte};
#if ici3
    cfunc.o_head.o_type = &ici_cfunc_type;
#else
    cfunc.o_head.o_type = &func_type;
#endif
    return objof(&cfunc);
}
