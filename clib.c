#define ICI_CORE
#include "fwd.h"
#include "file.h"
#include "exec.h"
#include "func.h"
#include "cfunc.h"
#include "op.h"
#include "int.h"
#include "float.h"
#include "str.h"
#include "buf.h"
#include "null.h"
#include "re.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#if defined(sun) || defined(BSD4_4) || defined(linux)
#include <sys/wait.h>
#endif

#ifndef _WIN32
#include <sys/param.h>
#include <dirent.h>
#endif   /* _WIN32 */

#if defined(linux) && !defined(MAXPATHLEN)
#include <sys/param.h>
#endif

#ifdef _WIN32
#include <io.h>
#include <process.h>
#include <direct.h>
#endif

#include "pcre/pcre.h"

/*
 * C library and others.  We don't want to go overboard here.  Would
 * like to keep the executable's size under control.
 */

#ifndef _WIN32
#endif

extern int      ici_f_sprintf();

int stdio_getc(void *file)
{
    return fgetc((FILE *)file);
}

static int stdio_ungetc(int c, void *file)
{
    return ungetc(c, (FILE *)file);
}

static int stdio_flush(void *file)
{
    return fflush((FILE *)file);
}

static int stdio_close(void *file)
{
    return fclose((FILE *)file);
}

static int stdio_pclose(void *file)
{
    return pclose((FILE *)file);
}

static long     stdio_seek(void *file, long offset, int whence)
{
    if (fseek((FILE *)file, offset, whence) == -1)
    {
        ici_set_error("seek failed");
        return -1;
    }
    return ftell((FILE *)file);
}

static int      stdio_eof(void *file)
{
    return feof((FILE *)file);
}

static int      stdio_write(const void *buf, long n, void *file)
{
    return fwrite(buf, 1, (size_t)n, (FILE *)file);
}

ici_ftype_t ici_stdio_ftype =
{
    FT_NOMUTEX,
    stdio_getc,
    stdio_ungetc,
    stdio_flush,
    stdio_close,
    stdio_seek,
    stdio_eof,
    stdio_write
};

ici_ftype_t  ici_popen_ftype =
{
    FT_NOMUTEX,
    stdio_getc,
    stdio_ungetc,
    stdio_flush,
    stdio_pclose,
    stdio_seek,
    stdio_eof,
    stdio_write
};

static int
f_getchar()
{
    ici_file_t          *f;
    int                 c;
    ici_exec_t          *x = NULL;

    if (ICI_NARGS() != 0)
    {
        if (ici_typecheck("u", &f))
	{
            return 1;
	}
    }
    else
    {
        if ((f = ici_need_stdin()) == NULL)
	{
            return 1;
	}
    }
    ici_signals_blocking_syscall(1);
    if (f->f_type->ft_flags & FT_NOMUTEX)
    {
        x = ici_leave();
    }
    c = (*f->f_type->ft_getch)(f->f_file);
    if (f->f_type->ft_flags & FT_NOMUTEX)
    {
        ici_enter(x);
    }
    ici_signals_blocking_syscall(0);
    if (c == EOF)
    {
        if ((FILE *)f->f_file == stdin)
	{
            clearerr(stdin);
	}
        return ici_null_ret();
    }
    buf[0] = c;
    return ici_ret_with_decref(ici_objof(ici_str_new(buf, 1)));
}

static int
f_ungetchar()
{
    ici_file_t  *f;
    char        *ch;

    if (ICI_NARGS() != 1)
    {
        if (ici_typecheck("su", &ch, &f))
            return 1;
    }
    else
    {
        if ((f = ici_need_stdin()) == NULL)
	{
            return 1;
	}
        if (ici_typecheck("s", &ch))
	{
            return 1;
	}
    }
    if ((*f->f_type->ft_ungetch)(*ch, f->f_file) == EOF)
    {
        return ici_set_error("unable to unget character");
    }
    return ici_str_ret(ch);
}

static int
f_getline()
{
    int        i;
    int        c;
    void       *file;
    ici_file_t          *f;
    int                 (*get)(void *);
    ici_exec_t          *x = NULL;
    char                *b;
    int                 buf_size;
    ici_str_t           *str;

    x = NULL;
    if (ICI_NARGS() != 0)
    {
        if (ici_typecheck("u", &f))
            return 1;
    }
    else
    {
        if ((f = ici_need_stdin()) == NULL)
            return 1;
    }
    get = f->f_type->ft_getch;
    file = f->f_file;
    if ((b = (char *)malloc(buf_size = 128)) == NULL)
        goto nomem;
    if (f->f_type->ft_flags & FT_NOMUTEX)
    {
        ici_signals_blocking_syscall(1);
        x = ici_leave();
    }
    for (i = 0; (c = (*get)(file)) != '\n' && c != EOF; ++i)
    {
        if (i == buf_size && (b = (char *)realloc(b, buf_size *= 2)) == NULL)
            break;
        b[i] = c;
    }
    if (f->f_type->ft_flags & FT_NOMUTEX)
    {
        ici_enter(x);
        ici_signals_blocking_syscall(0);
    }
    if (b == NULL)
        goto nomem;
    if (i == 0 && c == EOF)
    {
        free(b);
        if ((FILE *)f->f_file == stdin)
            clearerr(stdin);
        return ici_null_ret();
    }
    str = ici_str_new(b, i);
    free(b);
    if (str == NULL)
        return 1;
    return ici_ret_with_decref(ici_objof(str));

nomem:
    return ici_set_error("ran out of memory");
}

static int
f_getfile()
{
    int                 i;
    int                 c;
    ici_file_t          *f;
    int                 (*get)(void *);
    void                *file;
    ici_exec_t          *x = NULL;
    char                *b;
    int                 buf_size;
    ici_str_t           *str;
    int                 must_close;
    
    must_close = 0;
    str = NULL; /* Pessimistic. */
    if (ICI_NARGS() != 0)
    {
        if (ici_isstring(ICI_ARG(0)))
        {
            if (ici_call(SS(fopen), "o=o", &f, ICI_ARG(0)))
                goto finish;
            must_close = 1;
        }
        else
            f = ici_fileof(ICI_ARG(0));
        if (!ici_isfile(f))
        {
            char    n1[ICI_OBJNAMEZ];
            ici_set_error("getfile() given %s instead of a file",
                ici_objname(n1, ici_objof(f)));
            goto finish;
        }
    }
    else
    {
        if ((f = ici_need_stdin()) == NULL)
            goto finish;
    }
    get = f->f_type->ft_getch;
    file = f->f_file;
    if ((b = (char *)malloc(buf_size = 128)) == NULL)
        goto nomem;
    if (f->f_type->ft_flags & FT_NOMUTEX)
    {
        ici_signals_blocking_syscall(1);
        x = ici_leave();
    }
    for (i = 0; (c = (*get)(file)) != EOF; ++i)
    {
        if (i == buf_size && (b = (char *)realloc(b, buf_size *= 2)) == NULL)
            break;
        b[i] = c;
    }
    if (f->f_type->ft_flags & FT_NOMUTEX)
    {
        ici_enter(x);
        ici_signals_blocking_syscall(0);
    }
    if (b == NULL)
        goto nomem;
    str = ici_str_new(b, i);
    free(b);
    goto finish;

nomem:
    ici_set_error("ran out of memory");

finish:
    if (must_close)
    {
        ici_call(SS(close), "o", f);
        ici_decref(f);
    }
    return ici_ret_with_decref(ici_objof(str));
}

static int
f_tmpname()
{
    char nametemplate[] = "/tmp/ici.XXXXXX";
    int fd = mkstemp(nametemplate);
    if (fd == -1)
    {
	return ici_get_last_errno("mkstemp", NULL);
    }
    close(fd);
    return ici_str_ret(nametemplate);
}

static int
f_put()
{
    ici_str_t  *s;
    ici_file_t *f;
    ici_exec_t *x = NULL;

    if (ICI_NARGS() > 1)
    {
        if (ici_typecheck("ou", &s, &f))
            return 1;
    }
    else
    {
        if (ici_typecheck("o", &s))
            return 1;
        if ((f = ici_need_stdout()) == NULL)
            return 1;
    }
    if (!ici_isstring(ici_objof(s)))
        return ici_argerror(0);
    if (f->f_type->ft_flags & FT_NOMUTEX)
        x = ici_leave();
    if
    (
        (*f->f_type->ft_write)(s->s_chars, s->s_nchars, f->f_file)
        !=
        s->s_nchars
    )
    {
        if (f->f_type->ft_flags & FT_NOMUTEX)
            ici_enter(x);
        return ici_set_error("write failed");
    }
    if (f->f_type->ft_flags & FT_NOMUTEX)
        ici_enter(x);
    return ici_null_ret();
}

static int
f_fflush()
{
    ici_file_t          *f;
    ici_exec_t          *x = NULL;

    if (ICI_NARGS() > 0)
    {
        if (ici_typecheck("u", &f))
            return 1;
    }
    else
    {
        if ((f = ici_need_stdout()) == NULL)
            return 1;
    }
    if (f->f_type->ft_flags & FT_NOMUTEX)
        x = ici_leave();
    if ((*f->f_type->ft_flush)(f->f_file) == -1)
    {
        if (f->f_type->ft_flags & FT_NOMUTEX)
            ici_enter(x);
        return ici_set_error("flush failed");
    }
    if (f->f_type->ft_flags & FT_NOMUTEX)
        ici_enter(x);
    return ici_null_ret();
}

static int
f_fopen()
{
    const char  *name;
    const char  *mode;
    ici_file_t  *f;
    FILE        *stream;
    ici_exec_t  *x = NULL;
    int         i;

    mode = "r";
    if (ici_typecheck(ICI_NARGS() > 1 ? "ss" : "s", &name, &mode))
        return 1;
    x = ici_leave();
    ici_signals_blocking_syscall(1);
    stream = fopen(name, mode);
    ici_signals_blocking_syscall(0);
    if (stream == NULL)
    {
        i = errno;
        ici_enter(x);
        errno = i;
        return ici_get_last_errno("open", name);
    }
    ici_enter(x);
    if ((f = ici_file_new((char *)stream, &ici_stdio_ftype, ici_stringof(ICI_ARG(0)), NULL)) == NULL)
    {
        fclose(stream);
        return 1;
    }
    return ici_ret_with_decref(ici_objof(f));
}

static int
f_fseek()
{
    ici_file_t  *f;
    long        offset;
    long        whence;

    if (ici_typecheck("uii", &f, &offset, &whence))
    {
	if (ici_typecheck("ui", &f, &offset))
        {
	    return 1;
        }
	whence = 0;
    }
    switch (whence)
    {
    case 0:
    case 1:
    case 2:
        break;
    default:
        return ici_set_error("invalid whence value in seek()");
    }
    if ((offset = (*f->f_type->ft_seek)(f->f_file, offset, (int)whence)) == -1)
        return 1;
    return ici_int_ret(offset);
}

static int
f_popen()
{
    const char  *name;
    const char  *mode;
    ici_file_t  *f;
    FILE        *stream;
    ici_exec_t  *x = NULL;
    int         i;

    mode = "r";
    if (ici_typecheck(ICI_NARGS() > 1 ? "ss" : "s", &name, &mode))
        return 1;
    x = ici_leave();
    if ((stream = popen(name, mode)) == NULL)
    {
        i = errno;
        ici_enter(x);
        errno = i;
        return ici_get_last_errno("popen", name);
    }
    ici_enter(x);
    if ((f = ici_file_new((char *)stream, &ici_popen_ftype, ici_stringof(ICI_ARG(0)), NULL)) == NULL)
    {
        pclose(stream);
        return 1;
    }
    return ici_ret_with_decref(ici_objof(f));
}

static int
f_system()
{
    char        *cmd;
    long        result;
    ici_exec_t  *x = NULL;

    if (ici_typecheck("s", &cmd))
        return 1;
    x = ici_leave();
    result = system(cmd);
    ici_enter(x);
    return ici_int_ret(result);
}

static int
f_fclose()
{
    ici_file_t  *f;

    if (ici_typecheck("u", &f))
        return 1;
    if (ici_file_close(f))
        return 1;
    return ici_null_ret();
}

static int
f_eof()
{
    ici_file_t          *f;
    ici_exec_t          *x = NULL;
    int                 r;

    if (ICI_NARGS() != 0)
    {
        if (ici_typecheck("u", &f))
            return 1;
    }
    else
    {
        if ((f = ici_need_stdin()) == NULL)
            return 1;
    }
    if (f->f_type->ft_flags & FT_NOMUTEX)
        x = ici_leave();
    r = (*f->f_type->ft_eof)(f->f_file);
    if (f->f_type->ft_flags & FT_NOMUTEX)
        ici_enter(x);
    return ici_int_ret((long)r);
}

static int
f_remove(void)
{
    char        *s;

    if (ici_typecheck("s", &s))
        return 1;
    if (remove(s) != 0)
        return ici_get_last_errno("remove", s);
    return ici_null_ret();
}

#ifdef _WIN32
/*
 * Emulate opendir/readdir/et al under WIN32 environments via findfirst/
 * findnext. Only what f_dir() needs has been emulated (which is to say,
 * not much).
 */

#define MAXPATHLEN      _MAX_PATH

struct dirent
{
    char        *d_name;
};

typedef struct DIR
{
    long                handle;
    struct _finddata_t  finddata;
    int                 needfindnext;
    struct dirent       dirent;
}
DIR;

static DIR *
opendir(const char *path)
{
    DIR         *dir;
    char        fspec[_MAX_PATH+1];

    if (strlen(path) > (_MAX_PATH - 4))
        return NULL;
    sprintf(fspec, "%s/*.*", path);
    if ((dir = ici_talloc(DIR)) != NULL)
    {
        if ((dir->handle = _findfirst(fspec, &dir->finddata)) == -1)
        {
            ici_tfree(dir, DIR);
            return NULL;
        }
        dir->needfindnext = 0;
    }
    return dir;
}

static struct dirent *
readdir(DIR *dir)
{
    if (dir->needfindnext && _findnext(dir->handle, &dir->finddata) != 0)
            return NULL;
    dir->dirent.d_name = dir->finddata.name;
    dir->needfindnext = 1;
    return &dir->dirent;
}

static void
closedir(DIR *dir)
{
    _findclose(dir->handle);
    ici_tfree(dir, DIR);
}

#define S_ISREG(m)      (((m) & _S_IFMT) == _S_IFREG)
#define S_ISDIR(m)      (((m) & _S_IFMT) == _S_IFDIR)

#endif // WIN32

/*
 * array = dir([path] [, regexp] [, format])
 *
 * Read directory named in path (a string, defaulting to ".", the current
 * working directory) and return the entries that match the pattern (or
 * all names if no pattern passed). The format string identifies what
 * sort of entries should be returned. If the format string is passed
 * then a path MUST be passed (to avoid any ambiguity) but path may be
 * NULL meaning the current working directory (same as "."). The format
 * string uses the following characters,
 *
 *      f       return file names
 *      d       return directory names
 *      a       return all names (which includes things other than
 *              files and directories, e.g., hidden or special files
 *
 * The default format specifier is "f".
 */
static int
f_dir(void)
{
    const char          *path   = ".";
    const char          *format = "f";
    ici_regexp_t        *regexp = NULL;
    ici_obj_t           *o;
    ici_array_t         *a;
    DIR                 *dir;
    struct dirent       *dirent;
    int                 fmt;
    ici_str_t           *s;

    switch (ICI_NARGS())
    {
    case 0:
        break;

    case 1:
        o = ICI_ARG(0);
        if (ici_isstring(o))
            path = ici_stringof(o)->s_chars;
        else if (ici_isnull(o))
            ;   /* leave path as is */
        else if (ici_isregexp(o))
            regexp = ici_regexpof(o);
        else
            return ici_argerror(0);
        break;

    case 2:
        o = ICI_ARG(0);
        if (ici_isstring(o))
            path = ici_stringof(o)->s_chars;
        else if (ici_isnull(o))
            ;   /* leave path as is */
        else if (ici_isregexp(o))
            regexp = ici_regexpof(o);
        else
            return ici_argerror(0);
        o = ICI_ARG(1);
        if (ici_isregexp(o))
        {
            if (regexp != NULL)
                return ici_argerror(1);
            regexp = ici_regexpof(o);
        }
        else if (ici_isstring(o))
            format = ici_stringof(o)->s_chars;
        else
            return ici_argerror(1);
        break;

    case 3:
        o = ICI_ARG(0);
        if (ici_isstring(o))
            path = ici_stringof(o)->s_chars;
        else if (ici_isnull(o))
            ;   /* leave path as is */
        else
            return ici_argerror(0);
        o = ICI_ARG(1);
        if (!ici_isregexp(o))
            return ici_argerror(1);
        regexp = ici_regexpof(o);
        o = ICI_ARG(2);
        if (!ici_isstring(o))
            return ici_argerror(2);
        format = ici_stringof(o)->s_chars;
        break;

    default:
        return ici_argcount(3);
    }

    if (*path == '\0')
        path = ".";

#define FILES   1
#define DIRS    2
#define OTHERS  4

    for (fmt = 0; *format != '\0'; ++format)
    {
        switch (*format)
        {
        case 'f':
            fmt |= FILES;
            break;

        case 'd':
            fmt |= DIRS;
            break;

        case 'a':
            fmt |= OTHERS | DIRS | FILES;
            break;

        default:
            return ici_set_error("bad directory format specifier");
        }
    }
    if ((a = ici_array_new(0)) == NULL)
        return 1;
    if ((dir = opendir(path)) == NULL)
    {
        ici_get_last_errno("open directory", path);
        goto fail;
    }
    while ((dirent = readdir(dir)) != NULL)
    {
        struct stat     statbuf;
        char            abspath[MAXPATHLEN+1];

        if
        (
            regexp != NULL
            &&
            pcre_exec
            (
                regexp->r_re,
                regexp->r_rex,
                dirent->d_name,
                strlen(dirent->d_name),
                0,
                0,
                ici_re_bra,
                nels(ici_re_bra)
            )
            < 0
        )
            continue;
        sprintf(abspath, "%s/%s", path, dirent->d_name);
#ifndef _WIN32
        if (lstat(abspath, &statbuf) == -1)
        {
            ici_get_last_errno("get stats on", abspath);
            closedir(dir);
            goto fail;
        }
        if (S_ISLNK(statbuf.st_mode) && stat(abspath, &statbuf) == -1)
            continue;
#else
        if (stat(abspath, &statbuf) == -1)
            continue;
#endif
        if
        (
            (S_ISREG(statbuf.st_mode) && fmt & FILES)
            ||
            (S_ISDIR(statbuf.st_mode) && fmt & DIRS)
            ||
            fmt & OTHERS
        )
        {
            if
            (
                (s = ici_str_new_nul_term(dirent->d_name)) == NULL
                ||
                ici_stk_push_chk(a, 1)
            )
            {
                if (s != NULL)
                    ici_decref(s);
                closedir(dir);
                goto fail;
            }
            *a->a_top++ = ici_objof(s);
            ici_decref(s);
        }
    }
    closedir(dir);
    return ici_ret_with_decref(ici_objof(a));

#undef  FILES
#undef  DIRS
#undef  OTHERS

fail:
    ici_decref(a);
    return 1;
}

/*
 * Used as a common error return for system calls that fail. Sets the
 * global error string if the call fails otherwise returns the integer
 * result of the system call.
 */
static int
sys_ret(int ret)
{
    if (ret < 0)
        return ici_get_last_errno(NULL, NULL);
    return ici_int_ret((long)ret);
}

/*
 * rename(oldpath, newpath)
 */
static int
f_rename()
{
    char                *o;
    char                *n;

    if (ici_typecheck("ss", &o, &n))
        return 1;
    return sys_ret(rename(o, n));
}

/*
 * chdir(newdir)
 */
static int
f_chdir()
{
    char                *n;

    if (ici_typecheck("s", &n))
        return 1;
    return sys_ret(chdir(n));
}

/*
 * string = getcwd()
 */
static int
f_getcwd(void)
{
    char        buf[MAXPATHLEN+1];

    if (getcwd(buf, sizeof buf) == NULL)
        return sys_ret(-1);
    return ici_str_ret(buf);
}

#ifndef environ
    /*
     * environ is sometimes mapped to be a function, so only extern it
     * if it is not already defined.
     */
    extern char         **environ;
#endif

/*
 * Return the value of an environment variable.
 */
static int
f_getenv(void)
{
    ici_str_t           *n;
    char                **p;

    if (ICI_NARGS() != 1)
        return ici_argcount(1);
    if (!ici_isstring(ICI_ARG(0)))
        return ici_argerror(0);
    n = ici_stringof(ICI_ARG(0));

    for (p = environ; *p != NULL; ++p)
    {
        if
        (
#           if _WIN32
                /*
                 * Some versions of Windows (NT and 2000 at least)
                 * gratuitously change to case of some environment variables
                 * on boot.  So on Windows we do a case-insensitive
                 * compations. strnicmp is non-ANSI, but exists on Windows.
                 */
                strnicmp(*p, n->s_chars, n->s_nchars) == 0
#           else
                strncmp(*p, n->s_chars, n->s_nchars) == 0
#           endif
            &&
            (*p)[n->s_nchars] == '='
        )
        {
            return ici_str_ret(&(*p)[n->s_nchars + 1]);
        }
    }
    return ici_null_ret();
}

/*
 * Set an environment variable.
 */
static int
f_putenv(void)
{
    char        *s;
    char        *t;
    char        *e;
    char        *f;
    int         i;

    if (ici_typecheck("s", &s))
        return 1;
    if ((e = strchr(s, '=')) == NULL)
    {
        return ici_set_error("putenv argument not in form \"name=value\"");
    }
    i = strlen(s) + 1;
    /*
     * Some implementations of putenv retain a pointer to the supplied string.
     * To avoid the environment becoming corrupted when ICI collects the
     * string passed, we allocate a bit of memory to copy it into.  We then
     * forget about this memory.  It leaks.  To try to mitigate this a bit, we
     * check to see if the value is already in the environment, and free the
     * memory if it is.
     */
    if ((t = (char *)malloc(i)) == NULL)
    {
        return ici_set_error("ran out of memmory");
    }
    strcpy(t, s);
    t[e - s] = '\0';
    f = getenv(t);
    if (f != NULL && strcmp(f, e + 1) == 0)
    {
        free(t);
    }
    else
    {
        strcpy(t, s);
        putenv(t);
    }
    return ici_null_ret();
}

ICI_DEFINE_CFUNCS(clib)
{
    ICI_DEFINE_CFUNC1(printf,   ici_f_sprintf, 1),
    ICI_DEFINE_CFUNC(getchar,   f_getchar),
    ICI_DEFINE_CFUNC(ungetchar, f_ungetchar),
    ICI_DEFINE_CFUNC(getfile,   f_getfile),
    ICI_DEFINE_CFUNC(getline,   f_getline),
    ICI_DEFINE_CFUNC(fopen,     f_fopen),
    ICI_DEFINE_CFUNC(_popen,    f_popen),
    ICI_DEFINE_CFUNC(tmpname,   f_tmpname),
    ICI_DEFINE_CFUNC(put,       f_put),
    ICI_DEFINE_CFUNC(flush,     f_fflush),
    ICI_DEFINE_CFUNC(close,     f_fclose),
    ICI_DEFINE_CFUNC(seek,      f_fseek),
    ICI_DEFINE_CFUNC(system,    f_system),
    ICI_DEFINE_CFUNC(eof,       f_eof),
    ICI_DEFINE_CFUNC(remove,    f_remove),
    ICI_DEFINE_CFUNC(dir,       f_dir),
    ICI_DEFINE_CFUNC(getcwd,    f_getcwd),
    ICI_DEFINE_CFUNC(chdir,     f_chdir),
    ICI_DEFINE_CFUNC(rename,    f_rename),
    ICI_DEFINE_CFUNC(getenv,    f_getenv),
    ICI_DEFINE_CFUNC(putenv,    f_putenv),
    {ICI_CF_OBJ}
};
