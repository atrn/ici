// -*- mode:c++ -*-

#ifndef ICI_FWD_H
#define ICI_FWD_H

/*
 * fwd.h - basic configuration defines, universally used macros and
 * forward type, data and function defintions. Almost every ICI source
 * file needs this.
 */

/*
 * In general CONFIG_FILE is defined by the external build environment,
 * but for some common cases we have standard settings that can be used
 * directly. Windows in particular, because it is such a pain to handle
 * setting like this in Visual C, Project Builder and similar "advanced"
 * development environments.
 */
#if !defined(CONFIG_FILE)
#if defined(_WINDOWS)
#define CONFIG_FILE "conf/windows.h"
#elif defined(__MACH__) && defined(__APPLE__)
#define CONFIG_FILE "conf/darwin.h"
#elif defined(__linux__)
#define CONFIG_FILE "conf/linux.h"
#elif defined(__FreeBSD__)
#define CONFIG_FILE "conf/freebsd.h"
#elif defined(__CYGWIN__)
#define CONFIG_FILE "conf/cygwin.h"
#endif
#endif

#ifndef CONFIG_FILE
/*
 * CONFIG_FILE is supposed to be set from some makefile with some compile
 * line option to something like "conf/sun.h" (including the quotes).
 */
#error "The preprocessor define CONFIG_FILE has not been set."
#endif

#ifndef ICI_CONF_H
#include CONFIG_FILE
#endif

/*
 * Configuration files MAY defines the UNLIKELY() and LIKELY() macros
 * used to inform the compiler of the likelihood of a condition.  If
 * either of these are not defined we use the expression as-is.
 */
#ifndef UNLIKELY
#define UNLIKELY(X) (X)
#endif
#ifndef LIKELY
#define LIKELY(X) (X)
#endif

#include <cassert>

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
#include <cerrno>
#include <cmath>
#include <csignal>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef isset
#undef isset
#endif

namespace ici
{

/*
 * ICI version number. Note that this occurs in a string in conf.c too.
 */
constexpr int major_version = 5;
constexpr int minor_version = 0;
constexpr int release_number = 0;

/*
 * The ICI version number composed into an 8.8.16 unsigned long for simple
 * comparisons. The components of this are also available as 'major_version',
 * 'minor_version', and 'release_number'.
 *
 * This --constant-- forms part of the --ici-api--.
 */
constexpr unsigned long version_number =
    (((unsigned long)major_version << 24) | ((unsigned long)minor_version << 16) | release_number);

/*
 * The oldet version number for which the binary interface for seperately
 * compiled modules is backwards compatible. This is updated whenever
 * the exernal interface changes in a way that could break already compiled
 * modules. We aim to never to do that again. See 'check_interface()'.
 *
 * This --constant-- forms part of the --ici-api--.
 */
constexpr unsigned long back_compat_version = (5UL << 24) | (0UL << 16) | 0;

/*
 * DLI is defined in some configurations (Windows, in the conf include file)
 * to be a declaration modifier which must be applied to data objects being
 * referenced from a dynamically loaded DLL.
 *
 * If it hasn't been defined yet, define it to be null. Most system don't
 * need it.
 */
#ifndef DLI
#define DLI
#endif

#ifndef ICI_PATH_SEP
/*
 * The character which seperates directories in a path list on this
 * architecture.
 *
 * This --macro-- forms part of the --ici-api--.
 */
#define ICI_PATH_SEP ':' /* Default, may have been set in config file */
#endif

/*
 * The character which seperates segments in a path on this
 * architecture. This is the default value, it may have been set in
 * the config file.
 */
#ifndef ICI_DIR_SEP
/*
 * The character which seperates segments in a path on this
 * architecture.
 *
 * This --macro-- forms part of the --ici-api--.
 */
#define ICI_DIR_SEP '/' /* Default, may have been set in config file */
#endif

#ifndef ICI_DLL_EXT
/*
 * The string which is the extension of a dynamicly loaded library on this
 * architecture.
 *
 * This --macro-- forms part of the --ici-api--.
 */
#define ICI_DLL_EXT ".so" /* Default, may have been set in config file */
#endif

/*
 * A hash function for pointers.  This is used in a few places.  Notably in
 * the hash of object addresses for map lookup.  It is a balance between
 * effectiveness, speed, and machine knowledge.  It may or may not be right
 * for a given machine, so we allow it to be defined in the config file.  But
 * if it wasn't, this is what we use.
 */
#ifndef ICI_PTR_HASH
#define ICI_PTR_HASH(p) (crc_table[((size_t)(p) >> 4) & 0xFF] ^ crc_table[((size_t)(p) >> 12) & 0xFF])

/*
 * This is an alternative that avoids looking up the crc table.
#define ICI_PTR_HASH(p) (((unsigned long)(p) >> 12) * 31 ^ ((unsigned long)(p) >> 4) * 17)
*/
#endif

/*
 * A 'random' value for Windows event handling functions to return
 * to give a better indication that an ICI style error has occured which
 * should be propagated back. See events.c
 */
constexpr int ICI_EVENT_ERROR = 0x7A41B291;

#ifndef nels
#define nels(a) (sizeof(a) / sizeof(a)[0])
#endif

/*
 * Size of a char arrays used to hold formatted object names.
 */
constexpr int objnamez = 32;

/*
 * Standard types.
 */

class archiver;
class debugger;
class ftype;
class type;

struct array;
struct catcher;
struct cfunc;
struct channel;
struct exec;
struct expr;
struct file;
struct forall;
struct func;
struct handle;
struct integer;
struct ici_float;
struct map;
struct mark;
struct mem;
struct method;
struct name_id;
struct null;
struct object;
struct objwsup;
struct op;
struct parse;
struct pc;
struct ptr;
struct regexp;
struct set;
struct src;
struct slot;
struct str;
struct wrap;

// Maximum number of subexpressions supported with regular expressions.
//
constexpr int nsubexp = 10;

// Globals
//
extern DLI null     o_null;
extern DLI integer *o_zero;
extern DLI integer *o_one;
extern DLI exec    *execs;
extern DLI exec    *ex;
/*
 * The global error message pointer. The ICI error return convention
 * dictacts that the originator of an error sets this to point to a
 * short human readable string, in addition to returning the functions
 * error condition. See 'The error return convention' for more details.
 *
 * This --macro-- forms part of the --ici-api--.
 */
#ifdef ICI_CORE
#define ici_error (ici::ex->x_error) // per-thread, requires exec.h
#endif
extern DLI array           xs;
extern DLI array           os;
extern DLI array           vs;
extern DLI uint32_t        vsver;
extern DLI int             re_bra[(nsubexp + 1) * 3];
extern DLI int             re_nbra;
extern DLI volatile int    aborted;          /* See exec.c */
extern DLI int             record_line_nums; /* See lex.c */
extern DLI char           *buf;              /* See buf.h */
extern DLI size_t          bufz;             /* See buf.h */
extern DLI mark            o_mark;
extern DLI class debugger *debugger;
extern char                version_string[];
extern unsigned long const crc_table[256];
extern int                 exec_count;
extern DLI ftype          *stdio_ftype;
extern DLI ftype          *popen_ftype;
extern DLI ftype          *parse_ftype;

/*
 * This ICI NULL object.
 *
 * This --macro-- forms part of the --ici-api--.
 */
extern DLI null *null;

/*
 * Use 'return null_ret();' to return a ICI nullptr from an intrinsic
 * fuction.
 *
 * This --macro-- forms part of the --ici-api--.
 */

extern void       enter(exec *);
extern exec      *leave();
extern int        wakeup(object *);
extern int        waitfor(object *);
extern void       yield();
extern int        main(int, char **, bool = true);
extern int        init();
extern void       uninit();
extern void       atexit(void (*)(), wrap *);
extern void       uninit_compile();
extern void       uninit_cfunc();
extern object    *atom_probe(object *o);
extern object    *atom(object *, int);
extern void       reclaim();
extern int        unassign(set *, object *);
extern int        unassign(map *, object *);
extern void       invalidate_lookaside(map *);
extern int        parse_file(file *, objwsup *);
extern int        parse_file(const char *, char *, ftype *);
extern int        parse_file(const char *);
extern object    *eval(str *);
extern uint32_t   crc(unsigned long, unsigned char const *, ptrdiff_t);
extern str       *str_get_nul_term(const char *);
extern int        str_need_size(str *, size_t);
extern str       *str_alloc(size_t);
extern str       *str_intern(str *);
extern array     *new_array(ptrdiff_t = 0);
extern exec      *new_exec();
extern str       *new_str_nul_term(const char *);
extern str       *new_str_buf(size_t);
extern str       *new_str(const char *, size_t);
extern src       *new_src(int, str *);
extern set       *new_set();
extern regexp    *new_regexp(str *, int);
extern ptr       *new_ptr(object *, object *);
extern objwsup   *new_module(cfunc *cf);
extern objwsup   *new_class(cfunc *cf, objwsup *super);
extern map       *new_map();
extern map       *new_map(objwsup *);
extern integer   *new_int(int64_t);
extern ici_float *new_float(double);
extern handle    *new_handle(void *, str *, objwsup *, void (*)(handle *) = nullptr);
extern method    *new_method(object *, object *);
extern mem       *new_mem(void *, size_t, int, void (*)(void *));
extern object    *make_handle_member_map(name_id *);
extern int        argerror(int);
extern int        argcount2(int, int);
extern int        argcount(int);
extern int        typecheck(const char *, ...);
extern int        str_ret(const char *);
extern int        set_val(objwsup *, str *, int, void *);
extern int        retcheck(const char *, ...);
extern int        ret_with_decref(object *);
extern int        ret_no_decref(object *);
extern int        null_ret();
extern int        int_ret(int64_t);
extern int        float_ret(double);
extern int        register_type(type *);
extern void       init_types();
extern void       uninit_types();
extern file      *open_charbuf(char *, int, object *, bool);
extern file      *new_file(void *, ftype *, str *, object *);
extern int        close_file(file *);
extern int        close_channel(channel *);
extern int        method_check(object *o, int tcode);
extern int        handle_method_check(object *, str *, handle **, void **);
extern int        get_last_errno(const char *, const char *);
extern int        fetch_num(object *, object *, double *);
extern int        fetch_int(object *, object *, long *);
extern int        engine_stack_check();
extern int        define_cfuncs(cfunc *);
extern int        cmkvar(objwsup *, const char *, int, void *);
extern int        check_interface(unsigned long, unsigned long, char const *);
extern int        call_method(object *, str *, const char *, ...);
extern int        callv(str *, const char *, va_list);
extern int        callv(object *, object *, const char *, va_list);
extern int        call(str *, const char *, ...);
extern int        call(object *, const char *, ...);
extern int        assign_cfuncs(objwsup *, cfunc *);
extern handle    *handle_probe(void *, str *);
extern file      *need_stderr();
extern file      *need_stdout();
extern file      *need_stdin();
extern array     *need_path();
extern char      *objname(char[objnamez], object *);
extern int        find_on_path(char[FILENAME_MAX], const char *);
extern DLI int    debug_enabled;
extern int        debug_ignore_err;

#ifndef NODEBUGGING
extern DLI void debug_ignore_errors();
extern DLI void debug_respect_errors();
#endif

/*
 * ici_sopen() calls ici_open_charbuf() to obtain a read-only file.
 * open_charbuf() is preferred.
 */
inline file *sopen(char *data, int size, object *ref = nullptr)
{
    return open_charbuf(data, size, ref, true);
}

#ifdef NODEBUGGING
/*
 * If debug is not compiled in, we let the compiler use it's sense to
 * remove a lot of the debug code in performance critical areas.
 * Just to save on lots of ifdefs.
 */
#define debug_active 0
#define debug_ignore_errors()
#define debug_respect_errors()
#else
/*
 * Debugging is compiled-in. It is active if it is enabled at
 * run-time.
 */
#define debug_active debug_enabled
#endif

extern object **objs;
extern object **objs_top;
extern object **objs_limit;

extern void              init_signals();
extern volatile sigset_t signals_pending;
extern volatile long     signal_count[];
extern int               invoke_signal_handlers();
extern int               signals_invoke_immediately(int);
extern void              grow_objs(object *);
extern void              collect();

/*
 * End of ici.h export. --ici.h-end--
 */

extern object       *evaluate(object *, int);
extern char        **smash(char *, int);
extern char        **ssmash(char *, char *);
extern void          grow_atoms(ptrdiff_t newz);
extern const char   *binop_name(int);
extern slot         *find_raw_slot(map *, object *);
extern object       *atom_probe2(object *, object ***);
extern int           parse_exec();
extern int           exec_forall();
extern catcher      *new_catcher(object *, int, int, int);
extern cfunc        *new_cfunc(str *, int (*)(...), void *, void *);
extern func         *new_func();
extern op           *new_op(int (*)(), int16_t, int16_t);
extern parse        *new_parse(file *);
extern pc           *new_pc();
extern unsigned long hash_float(double);
extern unsigned long hash_string(object *);
int                  f_coreici(object *);
extern int           op_binop();
extern int           op_onerror();
extern int           op_for();
extern int           op_forall();
extern int           op_return();
extern int           op_mkptr();
extern int           op_openptr();
extern int           op_fetch();
extern int           op_unary();
extern int           set_error(const char *, ...);
extern const char   *get_error();
extern void          clear_error();
extern void          expand_error(int, str *);
extern int           lex(parse *, array *);
extern int           compile_expr(array *, expr *, int);
extern int           set_issubset(set *, set *);
extern int           set_ispropersubset(set *, set *);
extern int64_t       xstrtol(char const *, char **, int);
extern void          init_exec();
extern int           init_sstrings();
extern void          drop_all_small_allocations();
extern objwsup      *outermost_writeable();
extern int           str_char_at(str *, size_t);
extern void          repl();
extern int           supress_collect;
extern int           ncollects;

extern object **atoms;
extern size_t   natoms;
extern size_t   atomsz;

#if !defined(ICI_HAS_BSD_STRUCT_TM)
extern int set_timezone_vals(map *);
#endif

template <typename T> inline T * instanceof ()
{
    static T value;
    return &value;
}

} // namespace ici

#include "alloc.h"

#if defined(_WIN32) && !defined(NDEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#endif /* ICI_FWD_H */
