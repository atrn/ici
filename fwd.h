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
#    if defined(_WINDOWS)
#        define CONFIG_FILE "conf/windows.h"
#    elif defined(__MACH__) && defined(__APPLE__)
#        define CONFIG_FILE "conf/macos.h"
#    elif defined(__linux__)
#        define CONFIG_FILE "conf/linux.h"
#    elif defined(__bsd__)
#        define CONFIG_FILE "conf/bsd.h"
#    elif defined(__CYGWIN__)
#        define CONFIG_FILE "conf/cygwin.h"
#    endif
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

/*
 * Turn on extra bug hunting in non-release builds.
 */
#ifndef NDEBUG
#define BUGHUNT
#endif

#include <assert.h>

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <math.h>

namespace ici
{

/*
 * ICI version number. Note that this occurs in a string in conf.c too.
 *
 * ANICI uses a major version of 5 and the word 'ANICI' in the string.
 */
constexpr int ICI_VER_MAJOR   = 5;
constexpr int ICI_VER_MINOR   = 0;
constexpr int ICI_VER_RELEASE = 0;

/*
 * The ICI version number composed into an 8.8.16 unsigned long for simple
 * comparisons. The components of this are also available as 'ICI_VER_MAJOR',
 * 'ICI_VER_MINOR', and 'ICI_VER_RELEASE'.
 *
 * This --constant-- forms part of the --ici-api--.
 */
constexpr unsigned long ICI_VER = (((unsigned long)ICI_VER_MAJOR << 24) | ((unsigned long)ICI_VER_MINOR << 16) | ICI_VER_RELEASE);

/*
 * The oldet version number for which the binary interface for seperately
 * compiled modules is backwards compatible. This is updated whenever
 * the exernal interface changes in a way that could break already compiled
 * modules. We aim to never to do that again. See 'ici_interface_check()'.
 *
 * This --constant-- forms part of the --ici-api--.
 */
constexpr unsigned long ICI_BACK_COMPAT_VER = (5UL << 24) | (0UL << 16) | 0;

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
#define ICI_PATH_SEP    ':' /* Default, may have been set in config file */
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
#define ICI_DIR_SEP    '/' /* Default, may have been set in config file */
#endif

#ifndef ICI_DLL_EXT
/*
 * The string which is the extension of a dynamicly loaded library on this
 * architecture.
 *
 * This --macro-- forms part of the --ici-api--.
 */
#define ICI_DLL_EXT     ".so" /* Default, may have been set in config file */
#endif

/*
 * A hash function for pointers.  This is used in a few places.  Notably in
 * the hash of object addresses for struct lookup.  It is a balance between
 * effectiveness, speed, and machine knowledge.  It may or may not be right
 * for a given machine, so we allow it to be defined in the config file.  But
 * if it wasn't, this is what we use.
 */
#ifndef ICI_PTR_HASH
#define ICI_PTR_HASH(p) (ici_crc_table[((size_t)(p) >>  4) & 0xFF] ^ ici_crc_table[((size_t)(p) >> 12) & 0xFF])

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
#define nels(a)         (sizeof (a) / sizeof (a)[0])
#endif

/*
 * Size of a char arrays used to hold formatted object names.
 */
constexpr int ICI_OBJNAMEZ = 31;

/*
 * Standard types.
 */
typedef struct array            ici_array_t;
typedef struct catcher          ici_catch_t;
typedef struct sslot            ici_sslot_t;
typedef struct set              ici_set_t;
typedef struct ici_struct       ici_struct_t;
typedef struct exec             ici_exec_t;
typedef struct ici_float        ici_float_t;
typedef struct file             ici_file_t;
typedef struct func             ici_func_t;
typedef struct cfunc            ici_cfunc_t;
typedef struct method           ici_method_t;
typedef struct ici_int          ici_int_t;
typedef struct mark             ici_mark_t;
typedef struct null             ici_null_t;
typedef struct object           ici_obj_t;
typedef struct objwsup          ici_objwsup_t;
typedef struct op               ici_op_t;
typedef struct pc               ici_pc_t;
typedef struct ptr              ici_ptr_t;
typedef struct regexp           ici_regexp_t;
typedef struct src              ici_src_t;
typedef struct str              ici_str_t;
typedef class  type             type_t;
typedef struct wrap             ici_wrap_t;
typedef struct ftype            ici_ftype_t;
typedef struct forall           ici_forall_t;
typedef struct parse            ici_parse_t;
typedef struct mem              ici_mem_t;
typedef struct handle           ici_handle_t;
typedef struct debug            ici_debug_t;
typedef struct name_id          ici_name_id_t;

extern DLI ici_int_t            *ici_zero;
extern DLI ici_int_t            *ici_one;
extern DLI char                 *ici_error;
extern DLI ici_exec_t           *ici_execs;
extern DLI ici_exec_t           *ici_exec;
extern DLI ici_array_t          ici_xs;
extern DLI ici_array_t          ici_os;
extern DLI ici_array_t          ici_vs;
extern DLI long                 ici_vsver;
constexpr int NSUBEXP =         10;
extern DLI int                  ici_re_bra[(NSUBEXP + 1) * 3];
extern DLI int                  ici_re_nbra;
extern DLI volatile int         ici_aborted;            /* See exec.c */
extern DLI int                  ici_dont_record_line_nums;      /* See lex.c */
extern DLI char                 *ici_buf;                       /* See buf.h */
extern DLI int                  ici_bufz;                       /* See buf.h */
extern DLI ici_ftype_t          ici_stdio_ftype;
extern DLI ici_ftype_t          ici_popen_ftype;
extern DLI ici_mark_t           ici_o_mark;
extern DLI ici_null_t           ici_o_null;
extern DLI ici_debug_t          *debugfunc;
extern char                     ici_version_string[];
extern unsigned long const      ici_crc_table[256];
extern int                      ici_exec_count;

/*
 * This ICI NULL object. It is of type '(ici_obj_t *)'.
 *
 * This --macro-- forms part of the --ici-api--.
 */
#define ici_null                (&ici_o_null)

/*
 * Use 'return ici_null_ret();' to return a ICI NULL from an intrinsic
 * fuction.
 *
 * This --macro-- forms part of the --ici-api--.
 */
#define ici_null_ret()          ici_ret_no_decref(ici_null)


extern ici_obj_t                *ici_atom_probe(ici_obj_t *o);
extern ici_obj_t                *ici_fetch_fail(ici_obj_t *, ici_obj_t *);
extern ici_obj_t                *ici_atom(ici_obj_t *, int);
extern int                      ici_parse_file(const char *, char *, ici_ftype_t *);
extern ici_array_t              *ici_array_new(ptrdiff_t);
extern ici_mem_t                *ici_mem_new(void *, size_t, int, void (*)(void *));
extern ici_src_t                *ici_src_new(int, ici_str_t *);
extern ici_str_t                *ici_str_alloc(int);
extern ici_str_t                *ici_str_new_nul_term(const char *);
extern ici_str_t                *ici_str_get_nul_term(const char *);
extern ici_set_t                *ici_set_new();
extern ici_struct_t             *ici_struct_new();
extern ici_float_t              *ici_float_new(double);
extern ici_file_t               *ici_file_new(void *, ici_ftype_t *, ici_str_t *, ici_obj_t *);
extern ici_int_t                *ici_int_new(long);
extern int                      ici_interface_check(unsigned long, unsigned long, char const *);
extern ici_str_t                *ici_str_new(const char *, int);
extern ici_ptr_t                *ici_ptr_new(ici_obj_t *, ici_obj_t *);
extern ici_regexp_t             *ici_regexp_new(ici_str_t *, int);
extern int                      ici_assign_fail(ici_obj_t *, ici_obj_t *, ici_obj_t *);
extern ici_file_t               *ici_open_charbuf(char *, int, ici_obj_t *, int);
extern int                      ici_get_last_errno(const char *, const char *);
extern int                      ici_argcount(int);
extern int                      ici_argcount2(int, int);
extern int                      ici_argerror(int);
extern void                     ici_struct_unassign(ici_struct_t *, ici_obj_t *);
extern int                      ici_set_unassign(ici_set_t *, ici_obj_t *);
extern char                     *ici_objname(char [ICI_OBJNAMEZ], ici_obj_t *);
extern int                      ici_file_close(ici_file_t *f);
extern int                      ici_ret_with_decref(ici_obj_t *);
extern int                      ici_int_ret(long);
extern int                      ici_ret_no_decref(ici_obj_t *);
extern int                      ici_typecheck(const char *, ...);
extern int                      ici_retcheck(const char *, ...);
extern int                      ici_init();
extern void                     ici_uninit();
extern ici_file_t               *ici_need_stdin();
extern ici_file_t               *ici_need_stdout();
extern ici_array_t              *ici_need_path();
extern void                     ici_reclaim();
extern int                      ici_str_ret(const char *);
extern int                      ici_float_ret(double);
extern int                      ici_func(ici_obj_t *, const char *, ...);
extern int                      ici_method(ici_obj_t *, ici_str_t *, const char *, ...);
extern int                      ici_funcv(ici_obj_t *, ici_obj_t *, const char *, va_list);
extern int                      ici_call(ici_str_t *, const char *, ...);
extern int                      ici_callv(ici_str_t *, const char *, va_list);
extern int                      ici_cmkvar(ici_objwsup_t *, const char *, int, void *);
extern int                      ici_set_val(ici_objwsup_t *, ici_str_t *, int, void *);
extern int                      ici_fetch_num(ici_obj_t *, ici_obj_t *, double *);
extern int                      ici_fetch_int(ici_obj_t *, ici_obj_t *, long *);
extern int                      ici_assign_cfuncs(ici_objwsup_t *, ici_cfunc_t *);
extern int                      ici_def_cfuncs(ici_cfunc_t *);
extern int                      main(int, char **);
extern ici_method_t             *ici_method_new(ici_obj_t *, ici_obj_t *);
extern ici_handle_t             *ici_handle_new(void *, ici_str_t *, ici_objwsup_t *);
extern ici_handle_t             *ici_handle_probe(void *, ici_str_t *);
extern int                      ici_register_type(type_t *t);
extern void                     ici_rego_work(ici_obj_t *o);

extern void                     ici_array_gather(ici_obj_t **, ici_array_t *, ptrdiff_t, ptrdiff_t);

extern ptrdiff_t                ici_array_nels(ici_array_t *);
extern int                      ici_grow_stack(ici_array_t *, ptrdiff_t);
extern int                      ici_fault_stack(ici_array_t *, ptrdiff_t);
extern int                      ici_array_push(ici_array_t *, ici_obj_t *);
extern int                      ici_array_rpush(ici_array_t *, ici_obj_t *);
extern ici_obj_t                *ici_array_pop(ici_array_t *);
extern ici_obj_t                *ici_array_rpop(ici_array_t *);
extern ici_obj_t                *ici_array_get(ici_array_t *, ptrdiff_t);
extern ici_obj_t                **ici_array_find_slot(ici_array_t *, ptrdiff_t);

extern void                     ici_invalidate_struct_lookaside(ici_struct_t *);
extern int                      ici_engine_stack_check();
extern void                     ici_atexit(void (*)(), ici_wrap_t *);
extern ici_objwsup_t            *ici_class_new(ici_cfunc_t *cf, ici_objwsup_t *super);
extern ici_objwsup_t            *ici_module_new(ici_cfunc_t *cf);
extern int                      ici_handle_method_check(ici_obj_t *, ici_str_t *, ici_handle_t **, void **);
extern int                      ici_method_check(ici_obj_t *o, int tcode);
extern unsigned long            ici_crc(unsigned long, unsigned char const *, ptrdiff_t);
extern int                      ici_str_need_size(ici_str_t *, int);
extern ici_str_t                *ici_str_buf_new(int);
extern int                      ici_parse(ici_file_t *, ici_objwsup_t *);
extern ici_obj_t                *ici_eval(ici_str_t *);
extern ici_obj_t                *ici_make_handle_member_map(ici_name_id_t *);
extern int                      ici_parse_fname(const char *);
extern ici_exec_t               *ici_leave();
extern void                     ici_enter(ici_exec_t *);
extern void                     ici_yield();
extern int                      ici_waitfor(ici_obj_t *);
extern int                      ici_wakeup(ici_obj_t *);
extern int                      ici_init_thread_stuff();
extern DLI int                  ici_debug_enabled;
extern int                      ici_debug_ign_err;
extern DLI void                 ici_debug_ignore_errors();
extern DLI void                 ici_debug_respect_errors();

/*
 * ici_sopen() is now a macro that calls ici_open_charbuf() for read-only access.
 * Included only for backward compatibility; use ici_open_charbuf() instead.
 */
#define ici_sopen(data, sz, ref) ici_open_charbuf((data), (sz), (ref), 1)

#ifdef NODEBUGGING
    /*
     * If debug is not compiled in, we let the compiler use it's sense to
     * remove a lot of the debug code in performance critical areas.
     * Just to save on lots of ifdefs.
     */
#   define ici_debug_active     0
#else
    /*
     * Debugging is compiled-in. It is active if it is enabled at
     * run-time.
     */
#   define ici_debug_active     ici_debug_enabled
#endif

extern volatile sigset_t        ici_signals_pending;
extern volatile long            ici_signals_count[];
extern void                     ici_signals_init();
extern int                      ici_signals_invoke_handlers();
extern int                      ici_signals_blocking_syscall(int);

/*
 * End of ici.h export. --ici.h-end--
 */

typedef struct expr             expr_t;
typedef union ici_ostemp        ici_ostemp_t;

extern ici_obj_t        *ici_evaluate(ici_obj_t *, int);
extern char             **ici_smash(char *, int);
extern char             **ici_ssmash(char *, char *);
extern int              ici_natoms;
extern void             ici_grow_atoms(ptrdiff_t newz);
extern int              ici_supress_collect;
extern int              ici_ncollects;
extern const char       *ici_binop_name(int);
extern ici_sslot_t      *find_slot(ici_struct_t **, ici_obj_t *);
extern ici_sslot_t      *ici_find_raw_slot(ici_struct_t *, ici_obj_t *);
extern ici_obj_t        *ici_atom_probe2(ici_obj_t *, ici_obj_t ***);
extern int              ici_parse_exec();
extern ici_parse_t      *ici_new_parse(ici_file_t *);
extern ici_catch_t      *ici_new_catch(ici_obj_t *, int, int, int);
extern ici_func_t       *ici_new_func();
extern ici_op_t         *ici_new_op(int (*)(), int, int);
extern ici_pc_t         *ici_new_pc();
extern ici_catch_t      *ici_unwind();
extern void             ici_collect();
extern unsigned long    ici_hash_float(double);
extern unsigned long    ici_hash_string(ici_obj_t *);
extern int              ici_op_binop();
extern int              ici_op_onerror();
extern int              ici_op_for();
extern int              ici_op_andand();
extern int              ici_op_switcher();
extern int              ici_op_switch();
extern int              ici_op_forall();
extern int              ici_op_return();
extern int              ici_op_call();
extern int              ici_op_mkptr();
extern int              ici_op_openptr();
extern int              ici_op_fetch();
extern int              ici_op_unary();
extern int              ici_op_call();
extern void             ici_grow_objs(ici_obj_t *);
extern int              ici_set_error(const char *, ...);
extern void             ici_expand_error(int, ici_str_t *);
extern int              ici_lex(ici_parse_t *, ici_array_t *);
extern void             ici_uninit_cfunc();
extern int              ici_exec_forall();
extern int              ici_compile_expr(ici_array_t *, expr_t *, int);
extern void             ici_uninit_compile();
extern int              ici_set_issubset(ici_set_t *, ici_set_t *);
extern int              ici_set_ispropersubset(ici_set_t *, ici_set_t *);
extern ici_exec_t       *ici_new_exec();
extern long             ici_strtol(char const *, char **, int);
extern int              ici_init_path(ici_objwsup_t *externs);
extern int              ici_find_on_path(char [FILENAME_MAX], const char *);
extern int              ici_init_sstrings();
extern void             ici_drop_all_small_allocations();
extern ici_objwsup_t    *ici_outermost_writeable_struct();
extern ici_cfunc_t      *ici_cfunc_new(const char *, int (*)(...), void *, void *);
extern int              ici_str_char_at(ici_str_t *, int);

extern ici_obj_t        **ici_objs;
extern ici_obj_t        **ici_objs_top;
extern ici_obj_t        **ici_objs_limit;
extern ici_obj_t        **ici_atoms;
extern int              ici_atomsz;

extern DLI ici_ftype_t  ici_parse_ftype;

#if !defined(ICI_HAS_BSD_STRUCT_TM)
extern int              ici_set_timezone_vals(ici_struct_t *);
#endif

} // namespace ici

#include "alloc.h"

#if defined(_WIN32) && !defined(NDEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#endif /* ICI_FWD_H */
