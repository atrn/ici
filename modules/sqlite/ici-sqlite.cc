/*
 *  sqlite module.
 *  Copyright (C) 2019 A.Newman.
 *
 *  This is a module that provides access to SQLite (v3).  The module
 *  defines a handful of native code functions to access the core
 *  SQLite features and uses ICI code to extend that access.
 *
 *  The module's primary interface is the sqlite.open() function.
 *  open() opens a SQLite database file and returns an instance
 *  of the sqlite.db class which is used to access and manipulate
 *  the database.
 *
 *  The principle interface provided by the sqlite.db class is the
 *  exec() method, used to execute SQL statements.
 */

#include <sqlite3.h>

#include <ici.h>

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

namespace
{

#include "icistr.h"
#include <icistr-setup.h>

/* ----------------------------------------------------------------
 *
 *  'db' object type - an ICI handle instance with the 'sqlite3 *'
 *  database 'handle' as its pointer.
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

/* ----------------------------------------------------------------
 *
 * Module functions.
 *
 */

/*
 * string = sqlite.version()
 *
 * Return a string containing the SQLite version.
 */
int f_version()
{
    return ici::str_ret(SQLITE_VERSION);
}

/*
 * int = sqlite.version_number()
 *
 * Return an integer containing the SQLite version.
 */
int f_version_number()
{
    return ici::int_ret(SQLITE_VERSION_NUMBER);
}

/*
 * sqlite.db = sqlite.open(filename [, options])
 *
 * Opens the database file given by filename and returns a new
 * instance of the "sqlite.db" class representing the database.
 *
 * Options, if supplied, are single characters and may include
 * any of the following,
 *
 * 'c'  - create the database file if it does not exist
 * 'x'  - remove any database file and re-create, implies 'c'
 *
 * This --topic-- forms part of the --ici-sqlite-- documentation.
 */
int f_open()
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
	if (access(filename, F_OK) == -1)
	{
            ici::set_error("%s: no such database", filename);
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

/* ----------------------------------------------------------------
 *
 * sqlite.db implementation
 *
 * The 'db' class represents an open database and is the class of
 * the object returned by sqlite.open().
 *
 * A 'db' instance provides the following methods:
 *
 * - array = db:exec(string)
 *      Execute the SQL statement against the database and return an
 *      array of the "rows" returned by the statement.
 *
 * - int = db:changes()
 *      Return the count of changes made by the last database operation.
 *
 */

sqlite3 *get_sqlite(ici::object *inst, ici::handle **h)
{
    void *ptr;

    if (ici::handle_method_check(inst, ICIS(db), h, &ptr))
    {
	return nullptr;
    }
    if (isclosed(*h))
    {
        ici::set_error("attempt to use closed db");
	ptr = nullptr;
    }
    return static_cast<sqlite3 *>(ptr);
}

sqlite3 *get_sqlite(ici::object *inst)
{
    ici::handle *_;
    return get_sqlite(inst, &_);
}

int callback(void *u, int ncols, char **cols, char **names)
{
    ici::array * rows = ici::arrayof(static_cast<ici::object *>(u));

    auto row = ici::make_ref(ici::new_map());
    if (!row)
    {
	return 1;
    }
    for (int i = 0; i < ncols; ++i)
    {
	if (cols[i] != nullptr)
	{
            auto col = ici::make_ref(ici::new_str_nul_term(names[i]));
            if (!col)
            {
                return 1;
            }
            // TOOD: use column type to create appropriate ICI type
            auto val = ici::make_ref(ici::new_str_nul_term(cols[i]));
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
 * with the rows resulting from those statements.
 *
 * This --topic-- forms part of the --ici-sqlite-- documentation.
 */
int f_db_exec(ici::object *inst)
{
    char *statement;
    char *err;
    sqlite3 *s;

    if (!(s = get_sqlite(inst)) || ici::typecheck("s", &statement))
    {
	return 1;
    }
    auto rows = ici::make_ref(ici::new_array());
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
int f_db_changes(ici::object *inst)
{
    if (auto s = get_sqlite(inst))
    {
        return ici::int_ret(sqlite3_changes(s));
    }
    return 1;
}

} // anon namespace

/* ----------------------------------------------------------------
 *
 * Module entry point and initialization.
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
        ICI_DEFINE_CFUNC(open, f_open),
        ICI_DEFINE_CFUNC(version, f_version),
        ICI_DEFINE_CFUNC(version_number, f_version_number),
        ICI_CFUNCS_END()
    };

    auto module = ici::new_module(ICI_CFUNCS(sqlite));
    if (!module)
    {
        return nullptr;
    }

    static ICI_DEFINE_CFUNCS(db_methods)
    {
        ICI_DEFINE_CFUNC(changes, f_db_changes),
        ICI_DEFINE_CFUNC(exec, f_db_exec),
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
