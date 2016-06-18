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
 * int = fileno(file)
 */
static int
f_fileno(void)
{
    file_t	*f;

    if (ici_typecheck("u", &f))
	return 1;
    if (f->f_type != &stdio_ftype && f->f_type != &ici_popen_ftype)
    {
	error = "attempt to obtain file descriptor of non-stdio file";
	return 1;
    }
    return int_ret(fileno((FILE *)f->f_file));
}

object_t *
ici_fileno_library_init(void)
{
    static cfunc_t cfunc = {CF_OBJ, "fileno", f_fileno};
#ifdef ici3
    cfunc.o_head.o_type = &ici_cfunc_type;
#else
    cfunc.o_head.o_type = &func_type;
#endif
    return objof(&cfunc);
}
