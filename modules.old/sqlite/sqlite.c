/*
 * Embedded SQL database
 *
 * The sqlite module provides access to the SQLite embedded
 * SQL database library.
 *
 * This --intro-- and --synopsis-- are part of --ici-sqlite-- documentation.
 */

#include <sqlite.h>

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#define ICI_NO_OLD_NAMES
#include <ici.h>
#include "icistr.h"
#include <icistr-setup.h>

static ici_objwsup_t *ici_db_class = NULL;

static int
set_error(char *err)
{
    if (ici_chkbuf(strlen(err)))
    {
	ici_error = "sqlite error";
    }
    else
    {
	strcpy(ici_buf, err);
	free(err);
	ici_error = ici_buf;
    }
    return 1;
}

static void
ici_db_pre_free(ici_handle_t *h)
{
    if (!(objof(h)->o_flags & H_CLOSED))
	sqlite_close((sqlite *)h->h_ptr);
}

ici_handle_t *
new_db(sqlite *s)
{
    ici_handle_t *h;

    if ((h = ici_handle_new(s, ICIS(db), ici_db_class)) != NULL)
    {
	h->h_pre_free = ici_db_pre_free;
    }
    return h;
}

/*
 * database = sqlite.db:new(filename [, options])
 *
 * Open the database stored in the file given by filename.
 *
 * This --topic-- forms part of the --ici-sqlite-- documentation.
 */
static int
ici_db_new(ici_objwsup_t *klass)
{
    char *filename;
    char *err;
    sqlite *s;
    ici_handle_t *h;
    int mkdb = 0;

    if (ici_method_check(objof(klass), 0))
	return 1;
    if (ici_typecheck("s", &filename))
    {
	char *opts;
	if (ici_typecheck("ss", &filename, &opts))
	    return 1;
	for (; *opts != '\0'; ++opts)
	{
	    switch (*opts)
	    {
	    case 'c':
		mkdb = 1;
		break;
	    default:
		ici_error = "unknown database option in db:new()";
		return 1;
	    }
	}
    }
    if (!mkdb)
    {
	if (access(filename, R_OK|W_OK) == -1)
	{
	     ici_error = "attempt to open non-existing database";
	     return 1;
	}
    }
    if ((s = sqlite_open(filename, O_RDWR, &err)) == NULL)
	return set_error(err);
    if ((h = new_db(s)) == NULL)
	sqlite_close(s);
    return ici_ret_with_decref(objof(h));
}

static sqlite *
get_sqlite(ici_obj_t *inst, ici_handle_t **h)
{
    ici_handle_t *handle;
    void *ptr;

    if (h == NULL)
	h = &handle;
    if (ici_handle_method_check(inst, ICIS(db), h, &ptr))
	return NULL;
    if (objof(*h)->o_flags & H_CLOSED)
    {
	ici_error = "attempt to use closed db intsance";
	return NULL;
    }
    return ptr;
}

static int
callback(void *u, int ncols, char **cols, char **names)
{
    ici_array_t *recs = u;
    ici_struct_t *s;
    ici_str_t *k;
    ici_obj_t *v;
    int i;

    if ((s = ici_struct_new()) == NULL)
	return 1;
    for (i = 0; i < ncols; ++i)
    {
	if ((k = ici_str_new_nul_term(names[i])) == NULL)
	    goto fail;
	if (cols[i] == NULL)
	{
	    if (ici_assign(s, k, &o_null))
		goto fail1;
	}
	else 
	{
	    if ((v = objof(ici_str_new_nul_term(cols[i]))) == NULL)
		goto fail1;
	    if (ici_assign(s, k, v))
		goto fail2;
	    ici_decref(v);
	}
	ici_decref(k);
    }
    if (ici_array_push(recs, objof(s)))
	goto fail;
    ici_decref(s);
    return 0;

fail2:
    ici_decref(v);
fail1:
    ici_decref(k);
fail:
    ici_decref(s);
    return 1;
}

/*
 * array = database:exec(string)
 *
 * Execute SQL statements against the database and return an array
 * storing the affected rows.
 *
 * This --topic-- forms part of the --ici-sqlite-- documentation.
 */
static int
ici_db_exec(ici_obj_t *inst)
{
    sqlite *s = get_sqlite(inst, NULL);
    char *err;
    char *sql;
    ici_array_t *recs;

    if (s == NULL)
	return 1;
    if (ici_typecheck("s", &sql))
	return 1;
    if ((recs = ici_array_new(0)) == NULL)
	return 1;
    if (sqlite_exec(s, sql, callback, recs, &err) != SQLITE_OK)
    {
	ici_decref(recs);
	return set_error(err);
    }
    return ici_ret_with_decref(objof(recs));
}

/*
 * int = database:changes()
 *
 * Return the number of rows changed in the last database operation.
 *
 * This --topic-- forms part of the --ici-sqlite-- documentation.
 */
static int
ici_db_changes(ici_obj_t *inst)
{
    sqlite *s = get_sqlite(inst, NULL);
    if (s == NULL)
	return 1;
    return ici_int_ret(sqlite_changes(s));
}

static ici_cfunc_t ici_db_cfuncs[] =
{
    {CF_OBJ, "new", ici_db_new},
    {CF_OBJ, "exec", ici_db_exec},
    {CF_OBJ, "changes", ici_db_changes},
    {CF_OBJ}
};

static int
make_class(ici_objwsup_t *module, ici_objwsup_t **pklass, ici_cfunc_t *cfuncs, ici_str_t *klass_name)
{
    if ((*pklass = ici_class_new(cfuncs, module)) == NULL)
    {
        return 1;
    }
    if (ici_assign_base(module, klass_name, objof(*pklass)))
    {
        ici_decref(*pklass);
        return 1;
    }
    ici_decref(*pklass);
    return 0;
}

ici_obj_t *
ici_sqlite_library_init(void)
{
    ici_objwsup_t *module;

    if (ici_interface_check(ICI_VER, ICI_BACK_COMPAT_VER, "sqlite"))
        return NULL;
    if (init_ici_str())
        return NULL;
    if ((module = ici_module_new(ici_db_cfuncs)) == NULL)
        return NULL;

#define MAKE_CLASS(N) do if (make_class(module, &ici_ ## N ## _class, ici_ ## N ## _cfuncs, ICIS(N))) goto fail;  while (0)
    MAKE_CLASS(db);

#undef MAKE_CLASS

    return objof(module);

fail:

#define DECREF_IF_NON_NULL(x) do if ((x) != NULL) ici_decref(x); while (0)

    DECREF_IF_NON_NULL(ici_db_class);

#undef DECREF_IF_NON_NULL

    return NULL;
}
