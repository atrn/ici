#include <ici.h>

/*
 * string = join(array [, string])
 *
 * Implodes the array of strings or integer character codes  separating
 * each element with the given separator string or a single space by default.
 */

static int
f_join(void)
{
    array_t             *a;
    char		*sepstr;
    int			seplen;
    int                 i;
    object_t            **o;
    string_t            *s;
    char                *p;

    if (NARGS() == 1)
    {
	if (ici_typecheck("a", &a))
	    return 1;
	sepstr = " ";
	seplen = 1;
    }
    else if (ici_typecheck("as", &a, &sepstr))
	return 1;
    else
	seplen = strlen(sepstr);
    i = 0;
    for (o = a->a_base; o < a->a_top; ++o)
    {
        switch ((*o)->o_tcode)
        {
        case TC_INT:
            ++i;
            break;

        case TC_STRING:
            i += stringof(*o)->s_nchars;
            break;
        }
	i += seplen;
    }
    if (i > 0)
	i -= seplen; /* remove trailing separator */
    if ((s = new_string(i)) == NULL)
        return 1;
    p = s->s_chars;
    for (o = a->a_base; o < a->a_top; ++o)
    {
        switch ((*o)->o_tcode)
        {
        case TC_INT:
            *p++ = (char)intof(*o)->i_value;
            break;

        case TC_STRING:
            memcpy(p, stringof(*o)->s_chars, stringof(*o)->s_nchars);
            p += stringof(*o)->s_nchars;
            break;
        }
	if ((o + 1) < a->a_top)
	{
	    memcpy(p, sepstr, seplen);
	    p += seplen;
	}
    }
    if ((s = stringof(atom(objof(s), 1))) == NULL)
        return 1;
    return ici_ret_with_decref(objof(s));
}

object_t *
ici_join_library_init(void)
{
    static cfunc_t cfunc = {CF_OBJ, "join", f_join};
#if ici3
    cfunc.o_head.o_type = &ici_cfunc_type;
#else
    cfunc.o_head.o_type = &func_type;
#endif
    return objof(&cfunc);
}
