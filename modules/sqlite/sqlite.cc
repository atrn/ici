/*
 *  ICI sqlite module.
 *
 *  Copyright (C) 2019 A.Newman.
 */

#include <sqlite3.h>

#include <ici.h>
#include "icistr.h"
#include <icistr-setup.h>

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

namespace
{

/* ================================================================
 *
 *  Basic 'db' object type - a handle with an 'sqlite3 *' in its pointer.
 *
 */

ici::objwsup *db_class = nullptr;

inline bool isclosed(ici::handle *h)
{
    return h->hasflag(ici::handle::CLOSED);
}

void db_pre_free(ici::handle *h)
{
    if (!isclosed(h))
    {
	sqlite3_close(static_cast<sqlite3 *>(h->h_ptr));
    }
}

ici::handle *new_db(sqlite3 *s)
{
    if (auto h = ici::new_handle(s, ICIS(db), db_class))
    {
	h->h_pre_free = db_pre_free;
        return h;
    }
    return nullptr;
}

/* ================================================================
 *
 * Module functions.
 *
 */

/*
 * string = sqlite.version()
 */
int db_version()
{
    return ici::str_ret(SQLITE_VERSION);
}

/*
 * int = sqlite.version_number()
 */
int db_version_number()
{
    return ici::int_ret(SQLITE_VERSION_NUMBER);
}

/*
 * sqlite.db = sqlite.open(filename [, options])
 *
 * Opens the database file given by filename and returns a new
 * instance of the "sqlite.db" class representing the database.
 *
 * This --topic-- forms part of the --ici-sqlite-- documentation.
 */
int db_open()
{
    char *              filename;
    sqlite3 *           s;
    ici::handle *       h;
    bool                mkdb = false;
    bool                rmdb = false;

    if (ici::typecheck("s", &filename))
    {
	char *opts;
	if (ici::typecheck("ss", &filename, &opts))
        {
	    return 1;
        }
	for (; *opts != '\0'; ++opts)
	{
	    switch (*opts)
	    {
	    case 'c':
		mkdb = true;
		break;
            case 'x':
                rmdb = true;
                mkdb = true;
                break;
	    default:
                ici::set_error("'%c': unknown database option in db:new()", *opts);
		return 1;
	    }
	}
    }
    if (rmdb)
    {
        remove(filename);
    }
    if (!mkdb)
    {
	if (access(filename, R_OK|W_OK) == -1)
	{
            ici::set_error("%s: attempt to open non-existing database", filename);
            return 1;
	}
    }
    int error;
    if ((error = sqlite3_open(filename, &s)) != SQLITE_OK)
    {
	return ici::set_error("%s: %d", filename, sqlite3_errstr(error));
    }
    if ((h = new_db(s)) == nullptr)
    {
	sqlite3_close(s);
        return 1;
    }
    return ici::ret_with_decref(h);
}

/* ================================================================
 *
 * sqlite.db implementation
 *
 */

sqlite3 * get_sqlite(ici::object *inst, ici::handle **h)
{
    ici::handle *handle;
    if (h == nullptr)
    {
	h = &handle;
    }

    void *ptr;
    if (ici::handle_method_check(inst, ICIS(db), h, &ptr))
    {
	return nullptr;
    }

    if (isclosed(*h))
    {
        ici::set_error("attempt to use closed db instance");
	return nullptr;
    }

    return static_cast<sqlite3 *>(ptr);
}

int callback(void *u, int ncols, char **cols, char **names)
{
    ici::array * rows = ici::arrayof(static_cast<ici::object *>(u));

    auto row = ici::make_ref<ici::map>(ici::new_map());
    if (!row)
    {
	return 1;
    }
    for (int i = 0; i < ncols; ++i)
    {
	if (cols[i] != nullptr)
	{
            auto col = ici::make_ref<ici::str>(ici::new_str_nul_term(names[i]));
            if (!col)
            {
                return 1;
            }
            auto val = ici::make_ref<ici::str>(ici::new_str_nul_term(cols[i]));
            if (!val || row->assign(col, val))
            {
		return 1;
            }
	}
    }
    return rows->push_checked(row, ici::with_decref) ? 1 : 0;
}

/*
 * array = db:exec(string)
 *
 * Execute SQL statements against the database and return an array
 * with the resultant rows.
 *
 * This --topic-- forms part of the --ici-sqlite-- documentation.
 */
int db_exec(ici::object *inst)
{
    char *statement;
    char *err;

    auto s = get_sqlite(inst, nullptr);
    if (!s || ici::typecheck("s", &statement))
    {
	return 1;
    }
    auto rows = ici::make_ref<ici::array>(ici::new_array());
    if (!rows)
    {
	return 1;
    }
    if (sqlite3_exec(s, statement, callback, rows.get(), &err) != SQLITE_OK)
    {
	return ici::set_error("%s: %s", statement, err);
    }
    return ici::ret_no_decref(rows);
}

/*
 * int = db:changes()
 *
 * Return the number of rows changed in the last database operation.
 *
 * This --topic-- forms part of the --ici-sqlite-- documentation.
 */
int db_changes(ici::object *inst)
{
    if (auto s = get_sqlite(inst, nullptr))
    {
        return ici::int_ret(sqlite3_changes(s));
    }
    return 1;
}

} // anon namespace

/* ================================================================
 *
 * Module init
 *
 */

extern "C" ici::object *ici_sqlite_init()
{
    if (ici::check_interface(ici::version_number, ici::back_compat_version, "sqlite"))
    {
        return nullptr;
    }
    if (init_ici_str())
    {
        return nullptr;
    }
    static ICI_DEFINE_CFUNCS(sqlite)
    {
        ICI_DEFINE_CFUNC(open, db_open),
        ICI_DEFINE_CFUNC(version, db_version),
        ICI_DEFINE_CFUNC(version_number, db_version_number),
        ICI_CFUNCS_END()
    };
    auto module = ici::new_module(ICI_CFUNCS(sqlite));
    if (!module)
    {
        return nullptr;
    }

    static ICI_DEFINE_CFUNCS(db_methods)
    {
        ICI_DEFINE_CFUNC(changes, db_changes),
        ICI_DEFINE_CFUNC(exec, db_exec),
        ICI_CFUNCS_END()
    };
    db_class = ici::new_class(ICI_CFUNCS(db_methods), module);
    if (!db_class)
    {
        module->decref();
        return nullptr;
    }

    return module;
}
