#include <ici.h>

/*
 * string = fread([file, ] n)
 *
 * Reads n bytes from a stream and returns in a string. If
 * no file is specified stdin is used. Returns a string of
 * UP TO n bytes or NULL upon end of file.
 */

static int
f_fread(void)
{
    char	buffer[BUFSIZ+1];
    char	*bp;
    long	count;
    file_t	*file;
    long	n;
    string_t	*str;

    if (NARGS() == 1)
    {
	if (ici_typecheck("i", &count))
	    return 1;
	if ((file = need_stdin()) == NULL)
	    return 1;
    }
    else
    {
	if (ici_typecheck("ui", &file, &count))
	    return 1;
    }

    if (count <= sizeof buffer)
	bp = buffer;
    else if ((bp = ici_alloc(count+1)) == NULL)
	return 1;

    if (file->f_type->ft_getch == fgetc)
    {
	n = fread(bp, 1, count, (FILE *)file->f_file);
	if (n == 0 && feof((FILE *)file->f_file))
	{
	    ici_free(bp);
	    return null_ret();
	}
    }
    else
    {
	register char	*dst;
	register int	ch;

	for
	(
	    dst = bp, n = 0;
	    n < count && (ch = (*file->f_type->ft_getch)(file->f_file)) != EOF;
	    ++n
	)
	{
	    *dst++ = ch;
	}
	if (n == 0 && ch == EOF)
	{
	    ici_free(bp);
	    return null_ret();
	}
    }
    bp[n] = '\0';
    str = new_name(bp, n);
    if (bp != buffer)
	ici_free(bp);
    return ici_ret_with_decref(objof(str));
}

object_t *
ici_fread_library_init(void)
{
    static cfunc_t cfunc = {CF_OBJ, "fread", f_fread};
#if ici3
    cfunc.o_head.o_type = &ici_cfunc_type;
#else
    cfunc.o_head.o_type = &func_type;
#endif
    return objof(&cfunc);
}
