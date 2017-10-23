#include <file.h>
#include <exec.h>
#include <op.h>
#include <func.h>

static int
f_setlinebuf(void)
{
    file_t	*file;
    
    if (ici_typecheck("u", &file))
	    return 1;
    if (file->f_type->ft_getch == fgetc)
	setlinebuf((FILE *)file->f_file);
    return null_ret();
}

object_t *
ici_setlinebuf_library_init(void)
{
    static cfunc_t cfunc = {CF_OBJ, "setlinebuf", f_setlinebuf};
#ifdef ici3
    cfunc.o_head.o_type = &ici_cfunc_type;
#else
    cfunc.o_head.o_type = &func_type;
#endif
    return objof(&cfunc);
}
