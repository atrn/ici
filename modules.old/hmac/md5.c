#include <ici.h>
#include <sys/types.h>
#include <md5.h>
#include <errno.h>
#include <string.h>

static int f_md5_file(void)
{
    char *filename;
    char buf[33];

    if (ici_typecheck("s", &filename))
	return 1;
    if (MD5File(filename, buf) == NULL)
    {
	ici_error = strerror(errno);
	return 1;
    }
    return ici_str_ret(buf);
}

static ici_cfunc_t cfuncs[] =
{
    {ICI_CF_OBJ, "file", f_md5_file},
    {ICI_CF_OBJ}
};

ici_obj_t *
ici_md5_library_init(void)
{
    if (ici_interface_check(ICI_VER, ICI_BACK_COMPAT_VER, "md5"))
	return NULL;
    return ici_objof(ici_module_new(cfuncs));
}
