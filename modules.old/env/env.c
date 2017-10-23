/*
 * The environment as a data object
 *
 * The env ici module provides access the process environment
 * vector as an ici struct. Variables can be retrieved from the environment
 * by fetching them from the global variable "env" and the environment may
 * be altered by assigning to fields of this variable. E.g.,
 *
 * To `get' an environment variable normal indexing is performed,
 *
 *      printf("%s\n", env.PATH);
 *
 * And to `set' a variable,
 *
 *      env.LD_LIBRARY_PATH += ":/usr/local/lib/ici";
 *
 * The env global is implemented as a struct object with modified
 * assignment semantics. Keys and values are restricted to being
 * strings and assignment to the struct is propogated to the process
 * environment as if putenv() had been called.
 *
 * LIMITATIONS
 *
 * The typeof `env' is `env', not `struct'. Because of this it
 * is not possible to iterate over the members of the environment
 * using a forall statement nor is it possible to retrieve the
 * names of all environment variables using the keys() function.
 *
 * This --intro-- and --synopsis-- are part of --ici-env-- documentation.
 */

#include <ici.h>

static int env_tcode = 0;

static int
assign_env(object_t *o, object_t *k, object_t *v)
{
    object_t	*prev;

    if (!ici_isstring(k))
    {
	ici_error = "attempt to use non-string key with environment";
	return 1;
    }
    if (!ici_isstring(v))
    {
	ici_error = "attempt to ici_assign non-string value to environment";
	return 1;
    }
    prev = ici_fetch(o, k);
    ici_incref(prev);
    if (ici_types[ICI_TC_STRUCT]->t_assign(o, k, v))
	goto fail;
    if (ici_chkbuf(ici_stringof(k)->s_nchars + ici_stringof(v)->s_nchars + 1))
    {
	if (prev == ici_null)
	{
	    ici_struct_unassign(ici_structof(o), k);
	    ++ici_vsver;
	}
	else if ((*ici_types[ICI_TC_STRUCT]->t_assign)(o, k, prev))
	    ici_error = "environment incoherency, out of memory";
	goto fail;
    }
    sprintf(ici_buf, "%s=%s", ici_stringof(k)->s_chars, ici_stringof(v)->s_chars);
    if (putenv(ici_buf))
    {
	if (prev == ici_null)
	{
	    ici_struct_unassign(ici_structof(o), k);
	    ++ici_vsver;
	}
	else if ((*ici_types[ICI_TC_STRUCT]->t_assign)(o, k, prev))
	    ici_error = "environment incoherency, out of memory";
	goto fail;
    }
    return 0;

fail:
    ici_decref(prev);
    return 1;
}

object_t *
ici_env_library_init(void)
{
    static ici_type_t 	env_type;
    extern char 	**environ;
    ici_struct_t	*e;
    char		**p;
    ici_str_t		*s;
    char		*t;
    ici_str_t		*k;

    if (ici_interface_check(ICI_VER, ICI_BACK_COMPAT_VER, "env"))
        return NULL;
    if ((e = ici_struct_new()) == NULL)
	return NULL;
    for (p = environ; *p != NULL; ++p)
    {
	t = strchr(*p, '=');
	if (t == NULL)
	{
	    ici_error = "mal-formed environ";
	    return NULL;
	}
	if ((k = ici_str_new(*p, t - *p)) == NULL)
	    goto fail;
	if ((s = ici_str_new_nul_term(t+1)) == NULL)
	{
	    ici_decref(k);
	    goto fail;
	}
	if (ici_assign(e, k, s))
	{
	    ici_decref(k);
	    ici_decref(s);
	    goto fail;
	}
	ici_decref(s);
	ici_decref(k);
    }
    /*
     * Now we've set up the struct trap assignments to it and
     * vector them through our code that sets the real environment.
     */
    env_type = *ici_types[ICI_TC_STRUCT];
    env_type.t_assign = assign_env;
    env_type.t_name = "env";
    if (!(env_tcode = ici_register_type(&env_type)))
    {
	return NULL;
    }
    ici_objof(e)->o_tcode = env_tcode;
    return ici_objof(e);

fail:
    ici_decref(e);
    return NULL;
}
