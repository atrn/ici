#define ICI_CORE
/*
 * System calls and related
 *
 * Not all system calls are supported and some are incompletely
 * supported.  Most system call functions return integers, zero if the
 * call succeeded.  Errors are reported using ICI's usual error
 * handling ("system calls" will never return the -1 error return
 * value).  If an error is raised by a system call the value of
 * 'error' in the error handler will be the error message (as returned
 * by the ANSI C strerror() function for the errno set by the call).
 */

#include "fwd.h"
#include "int.h"
#include "null.h"
#include "str.h"
#include "array.h"
#include "struct.h"
#include "cfunc.h"
#include "file.h"
#include "mem.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <signal.h>

#ifndef _WIN32
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#endif

#ifdef __CYGWIN__
#define ICI_SYS_NOFLOCK
#define NO_ACCT
#endif

#if defined(__linux__)
#include <sys/signal.h>
#include <ulimit.h>
#endif

#ifdef _WIN32
#define        ICI_SYS_NOPASSWD
#define        ICI_SYS_NOFLOCK
#define        ICI_SYS_NORLIMITS
#endif

#ifndef ICI_SYS_NOPASSWD
#include <pwd.h>
#endif

#ifndef ICI_SYS_NORLIMITS
#include <sys/resource.h>
#endif

#if defined(__linux__) && !defined(MAXPATHLEN)
#include <sys/param.h>
#endif

#if defined(__linux__) || defined(__sun__) || defined(__CYGWIN__) || defined(__APPLE__)
#define SETPGRP_0_ARGS
#endif

#if defined BSD4_4 && !defined __APPLE__
#define SETPGRP_2_ARGS
#endif

#if defined(__linux__) && defined(BSD4_4)
#error __linux__ or BSD4_4 for setpgrp(), not both
#endif

#ifndef _WIN32
#ifndef ICI_SYS_NOFLOCK
#include <sys/file.h>
#endif
#endif

#ifdef _WIN32
#include <io.h>
#include <process.h>
#include <direct.h>

/*
 * Some macros for compatibility with Unix
 */

#define F_OK    0
#define W_OK    2
#define R_OK    4

#define MAXPATHLEN  _MAX_PATH
#endif

/*
 * The following list summarises the pre-defined values.  See the
 * documentation for the C macros for information as to their use.
 *
 * Values for open's flags parameter:
 *
 *  O_RDONLY
 *  O_WRONLY
 *  O_RDWR
 *  O_APPEND
 *  O_CREAT
 *  O_TRUNC
 *  O_EXCL
 *  O_SYNC
 *  O_NDELAY
 *  O_NONBLOCK
 *  O_BINARY        (Win32 only)
 *
 * Values for access's mode parameter:
 *
 *  R_OK
 *  W_OK
 *  X_OK
 *  F_OK
 *
 * Values for lseek's whence parameter:
 *
 *  SEEK_SET
 *  SEEK_CUR
 *  SEEK_END
 *
 * Values for flock's op parameter:
 *
 *  LOCK_SH
 *  LOCK_EX
 *  LOCK_NB
 *  LOCK_UN
 *
 * Values for spawn's mode parameter:
 *
 *  _P_WAIT
 *  _P_NOWAIT
 *
 * Values returned by stat:
 *
 *  S_IFMT
 *  S_IFCHR
 *  S_IFDIR
 *  S_IFREG
 *  S_IREAD
 *  S_IWRITE
 *  S_IEXEC
 *  S_IFIFO
 *  S_IFBLK
 *  S_IFLNK
 *  S_IFSOCK
 *  S_ISUID
 *  S_ISGID
 *  S_ISVTX
 *  S_IRUSR
 *  S_IWUSR
 *  S_IXUSR
 *  S_IRGRP
 *  S_IWGRP
 *  S_IXGRP
 *  S_IROTH
 *  S_IWOTH
 *  S_IXOTH
 *
 * This --intro-- forms part of the --ici-sys-- documentation.
 */

namespace ici
{

/*
 * Create pre-defined variables to replace C's #define's.
 */
int ici_sys_init(ici::objwsup *scp)
{
    size_t      i;

#define VALOF(x) { #x , x }
    static struct { const char *name; long val; } var[] =
    {
        VALOF(O_RDONLY),
        VALOF(O_WRONLY),
        VALOF(O_RDWR),
        VALOF(O_APPEND),
        VALOF(O_CREAT),
        VALOF(O_TRUNC),
        VALOF(O_EXCL),
#ifdef O_BINARY         /* WINDOWS has binary mode for open() */
        VALOF(O_BINARY),
#endif
#ifdef O_SYNC           /* WINDOWS doesn't have O_SYNC */
        VALOF(O_SYNC),
#endif
#ifdef O_NDELAY         /* WINDOWS doesn't have O_NDELAY */
        VALOF(O_NDELAY),
#endif
        VALOF(O_NONBLOCK),
        VALOF(R_OK),
        VALOF(W_OK),
#ifdef X_OK             /* WINDOWS doesn't have X_OK */
        VALOF(X_OK),
#endif
        VALOF(F_OK),
        VALOF(SEEK_SET),
        VALOF(SEEK_CUR),
        VALOF(SEEK_END),

#if !defined(ICI_SYS_NOFLOCK) && defined(LOCK_SH)
        VALOF(LOCK_SH),
        VALOF(LOCK_EX),
        VALOF(LOCK_NB),
        VALOF(LOCK_UN),
#endif

#ifdef _WIN32
        VALOF(_P_WAIT),
        VALOF(_P_NOWAIT),
#endif

        VALOF(S_IFMT),
        VALOF(S_IFCHR),
        VALOF(S_IFDIR),
        VALOF(S_IFREG),
        VALOF(S_IREAD),
        VALOF(S_IWRITE),
        VALOF(S_IEXEC),

#ifndef _WIN32
        VALOF(S_IFIFO),
        VALOF(S_IFBLK),
        VALOF(S_IFLNK),
        VALOF(S_IFSOCK),
        VALOF(S_ISUID),
        VALOF(S_ISGID),
        VALOF(S_ISVTX),
        VALOF(S_IRUSR),
        VALOF(S_IWUSR),
        VALOF(S_IXUSR),
        VALOF(S_IRGRP),
        VALOF(S_IWGRP),
        VALOF(S_IXGRP),
        VALOF(S_IROTH),
        VALOF(S_IWOTH),
        VALOF(S_IXOTH)
#endif
    };
#undef VALOF

    for (i = 0; i < sizeof var/sizeof var[0]; ++i)
        if (ici_cmkvar(scp, var[i].name, 'i', &var[i].val))
            return 1;
    return 0;
}

/*
 * Used as a common error return for system calls that fail. Sets the
 * global error string if the call fails otherwise returns the integer
 * result of the system call.
 */
static int
sys_ret(int ret)
{
    char        n1[ICI_OBJNAMEZ];

    if (ret < 0)
        return ici_get_last_errno(ici_objname(n1, ici_os.a_top[-1]), NULL);
    return ici_int_ret((long)ret);
}

/*
 * Used to call "simple" system calls. Simple calls are those that take
 * up to four integer parameters and return an integer. FUNCDEF() just
 * makes the name unique for this module. We have to use CFUNC3()
 * to use it, see below.
 */
static int ici_sys_simple()
{
    long        av[4];

    if (ici_typecheck((const char *)ICI_CF_ARG2(), &av[0], &av[1], &av[2], &av[3]))
        return 1;
    return sys_ret((*(int (*)(...))ICI_CF_ARG1())(av[0], av[1], av[2], av[3]));
}

/*
 * int = open(pathname, int [, int])
 *
 * Open the named file for reading or writing depending upon the value of the
 * second parameter, flags, and return a file descriptor.  The second
 * parameter is a bitwise combination of the various O_ values (see above) and
 * if this set includes the O_CREAT flag a third parameter, mode, must also be
 * supplied. Returns the file descriptor (an int).
 *
 * Supported on Win32 platforms.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_open()
{
    char        *fname;
    long        omode;
    long        perms = -1; /* -1 means not passed */

    switch (ICI_NARGS())
    {
    case 2:
        if (ici_typecheck("si", &fname, &omode))
            return 1;
        break;

    case 3:
        if (ici_typecheck("sii", &fname, &omode, &perms))
            return 1;
        break;

    default:
        return ici_argcount(3);
    }
    if (omode & O_CREAT && perms == -1)
    {
        ici_set_error("permission bits not specified in open() with O_CREAT");
        return 1;
    }
    return sys_ret(open(fname, omode, perms));
}

#ifdef _WIN32
/*
 * General return for things not implemented on (that suckful) Win32.
 */
static int
not_on_win32(const char *s)
{
    sprintf(ici_buf, "%s is not implemented on Win32 platforms", s);
    ici_set_error(ici_buf);
    return 1;
}
#endif

/*
 * file = fdopen(int [, openmode])
 *
 * Open a file descriptor as an (ICI) file object. This is analogous
 * to fopen, sopen, mopen, popen, net.sktopen, etc... The openmode is a
 * string as accepted by fopen and defaults to "r" (read only).
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_fdopen()
{
#ifdef _WIN32 /* WINDOWS can't do fdopen() without lots of work */
    return not_on_win32("fdopen");
#else
    long        fd;
    const char  *mode;
    FILE        *stream;
    ici_file_t  *f;

    switch (ICI_NARGS())
    {
    case 1:
        if (ici_typecheck("i", &fd))
            return 1;
        mode = "r";
        break;
    case 2:
        if (ici_typecheck("is", &fd, &mode))
            return 1;
        break;
    default:
        return ici_argcount(2);
    }
    if ((stream = fdopen(fd, mode)) == NULL)
    {
        ici_set_error("can't fdopen");
        return 1;
    }
    setvbuf(stream, NULL, _IOLBF, 0);
    if ((f = ici_file_new((char *)stream, &ici_stdio_ftype, NULL, NULL)) == NULL)
    {
        fclose(stream);
        return 1;
    }
    return ici_ret_with_decref(f);
#endif /* _WIN32 */
}

/*
 * _close(int)
 *
 * Close a file descriptor.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_close()
{
    int                 rc;
    ici_obj_t            *fd0;
    ici_obj_t            *fd1;

    if (ICI_NARGS() != 1)
        return ici_argcount(1);
    if (ici_isint(ICI_ARG(0)))
        rc = close(ici_intof(ICI_ARG(0))->i_value);
    else if (ici_isarray(ICI_ARG(0)))
    {
        ici_array_t *a = ici_arrayof(ICI_ARG(0));

        if
        (
            ici_array_nels(a) != 2
            ||
            !ici_isint(fd0 = ici_array_get(a, 0))
            ||
            !ici_isint(fd1 = ici_array_get(a, 1))
        )
        {
            ici_set_error("invalid fd array passed to _close");
            return 1;
        }
        rc = close(ici_intof(fd0)->i_value);
        if (rc == 0)
            rc = close(ici_intof(fd1)->i_value);
    }
    else
        return ici_argerror(0);
    return sys_ret(rc);
}


#ifndef _WIN32

/* Convert a struct to a struct flock for fcntl's F_SETLK */

static int
struct_to_flock(ici_struct_t *d, struct flock *flock)
{
    ici_obj_t    *o;

    if ((o = ici_fetch(d, SSO(start))) == ici_null)
        flock->l_start = 0;
    else
        flock->l_start = ici_intof(o)->i_value;
    if ((o = ici_fetch(d, SSO(len))) == ici_null)
        flock->l_len = 0;
    else
        flock->l_len = ici_intof(o)->i_value;
    if ((o = ici_fetch(d, SSO(type))) == ici_null)
        flock->l_type = F_RDLCK;
    else if (ici_isstring(o))
    {
        if (o == SSO(rdlck))
            flock->l_type = F_RDLCK;
        else if (o == SSO(wrlck))
            flock->l_type = F_WRLCK;
        else if (o == SSO(unlck))
            flock->l_type = F_UNLCK;
        else
            goto bad_lock_type;
    }
    else if (ici_isint(o))
        flock->l_type = ici_intof(o)->i_value;
    else
    {
    bad_lock_type:
        ici_set_error("invalid lock type");
        return 1;
    }
    if ((o = ici_fetch(d, SSO(whence))) == ici_null)
        flock->l_whence = SEEK_SET;
    else
        flock->l_whence = ici_intof(o)->i_value;
    return 0;
}
#endif  /* _WIN32 */

/*
 * int = fcntl(fd, what [, arg])
 *
 * Invoke the 'fcntl(2)' (file control) function.  The action taken depends on
 * the value of 'what', which must be one of the following strings (which map
 * to the given C define):
 *
 * "dupfd"              F_DUPFD - Dup and return the file descriptor.
 *
 * "getfd"              F_GETFD - Read the close-on-exec flag.
 *
 * "getfl"              F_GETFL - Read the descriptor's flags (all flags (as
 *                      set by open(2)) are returned).
 *
 * "setfl"              F_SETFL - Set the descriptor's flags to the value
 *                      specified by arg.  Only O_APPEND, O_NONBLOCK and
 *                      O_ASYNC may be set; the other flags are unaffected.
 *
 * "getown"             F_GETOWN - Get the process ID or process group
 *                      currently receiving SIGIO and SIGURG signals for
 *                      events on file descriptor fd.  Process groups are
 *                      returned as negative values.
 *
 * "setown"             F_SETOWN - Set the process ID or process group that
 *                      will receive SIGIO and SIGURG signals for events on
 *                      file descriptor fd.
 *
 * "setlk"              F_SETLK - Set or clear a lock (see below).
 *
 * For 'setlk', the third parameter 'arg' must be supplied.  It may be a
 * struct (or NULL) that will be used to populate a 'struct flock' with the
 * following items (a default is used for each if missing or NULL):
 *
 * start                An int which will be used in the 'l_start' field.
 *                      Default 0.
 *
 * len                  An int which will be used in the 'l_len' field.
 *                      Default 0.
 *
 * type                 Either an int giving the lock type, or one of the
 *                      strings 'rdlck', 'wrlck', or 'unlck'.  Default
 *                      F_RDLCK;
 *
 * Not suppored on Win32.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_fcntl()
{
#ifdef _WIN32
    return not_on_win32("fcntl");
#else
    long        fd;
    ici_str_t    *what;
    ici_obj_t    *arg;
    int         iarg;
    int         iwhat;
    int         r;

    switch (ICI_NARGS())
    {
    case 2:
        iarg = 1;
        arg = ici_null;
        if (ici_typecheck("io", &fd, &what))
            return 1;
        break;

    case 3:
        if (ici_typecheck("ioo", &fd, &what, &arg))
            return 1;
        if (ici_isint(arg))
            iarg = ici_intof(arg)->i_value;
        else
            iarg = 0;
        break;

    default:
        return ici_argcount(3);
    }
    if (!ici_isstring(what))
        return ici_argerror(1);
    if (what == SS(dupfd))
        iwhat = F_DUPFD;
    else if (what == SSO(getfd))
        iwhat = F_GETFD;
    else if (what == SSO(setfd))
        iwhat = F_SETFD;
    else if (what == SSO(getfl))
        iwhat = F_GETFL;
    else if (what == SSO(setfl))
        iwhat = F_SETFL;
    else if (what == SSO(getown))
        iwhat = F_GETOWN;
    else if (what == SSO(setown))
        iwhat = F_SETOWN;
    else if (what == SSO(setlk))
    {
        struct flock myflock;

        memset(&myflock, 0, sizeof myflock);
        myflock.l_type = F_RDLCK;
        myflock.l_whence = SEEK_SET;
        iwhat = F_SETLK;
        if (ici_isstruct(arg) && struct_to_flock(ici_structof(arg), &myflock))
            return 1;
        r = fcntl(fd, iwhat, &myflock);
        goto ret;
    }
    else
        return ici_argerror(1);
    r = fcntl(fd, iwhat, iarg);
 ret:
    return sys_ret(r);
#endif /* _WIN32 */
}

/*
 * int = fileno(file)
 *
 * Return the underlying file descriptor of a file opened with 'fopen' or
 * 'popen'.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_fileno()
{
    ici_file_t      *f;

    if (ici_typecheck("u", &f))
        return 1;
    if
    (
        f->f_type != &ici_stdio_ftype
#ifndef NOPIPES
        &&
        f->f_type != &ici_popen_ftype
#endif
    )
    {
        ici_set_error("attempt to obtain file descriptor of non-stdio file");
        return 1;
    }
    return ici_int_ret(fileno((FILE *)f->f_file));
}

/*
 * mkdir(pathname, int)
 *
 * Create a directory with the specified mode.  Supported on Win32 platforms.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_mkdir()
{
    char        *path;

#ifdef _WIN32
    if (ici_typecheck("s", &path))
        return 1;
    if (mkdir(path) == -1)
        return ici_get_last_errno("mkdir", path);
#else
    long        mode = 0777;

    if (ICI_NARGS() == 1)
    {
        if (ici_typecheck("s", &path))
            return 1;
    }
    else if (ici_typecheck("si", &path, &mode))
        return 1;
    if (mkdir(path, mode) == -1)
        return ici_get_last_errno("mkdir", path);
#endif
    return ici_ret_no_decref(ici_null);
}

/*
 * mkfifo(path, mode)
 *
 * Make a "named pipe" with the given mode (an int).
 * Not supported on Win32.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_mkfifo()
{
#ifdef _WIN32 /* WINDOWS can't do mkifo() */
    return not_on_win32("mkfifo");
#else
    char        *path;
    long        mode;

    if (ici_typecheck("si", &path, &mode))
        return 1;
    if (mkfifo(path, mode) == -1)
        return ici_get_last_errno("mkfifo", path);
    return ici_ret_no_decref(ici_null);
#endif /* _WIN32 */
}

/*
 * string = read(fd, len)
 *
 * Read up to 'len' bytes from the given file descriptor using 'read(2)',
 * and return the result as a string.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_read()
{
    long        fd;
    long        len;
    ici_str_t    *s;
    int         r;
    char        *msg;

    if (ici_typecheck("ii", &fd, &len))
        return 1;
    if ((msg = (char *)ici_alloc(len+1)) == NULL)
        return 1;
    switch (r = read(fd, msg, len))
    {
    case -1:
        ici_free(msg);
        return sys_ret(-1);

    case 0:
        ici_free(msg);
        return ici_null_ret();
    }
    if ((s = ici_str_alloc(r)) == NULL)
    {
        ici_free(msg);
        return 1;
    }
    memcpy(s->s_chars, msg, r);
    s = ici_stringof(ici_atom(s, 1));
    ici_free(msg);
    return ici_ret_with_decref(s);
}

/*
 * int = write(fd, string|mem [, len)
 *
 * Write a string or mem object to the given file descriptor using
 * 'write(2)'. If 'len' is given, and it is less than the size in
 * bytes of the data, that 'len' will be used.
 * Returns the actual number of bytes written.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_write()
{
    long        fd;
    ici_obj_t    *o;
    char        *addr;
    long        sz;
    int         havesz = 0;

    if (ici_typecheck("io", &fd, &o))
    {
        if (ici_typecheck("ioi", &fd, &o, &sz))
            return 1;
        havesz = 1;
    }
    if (ici_isstring(o))
    {
        addr = (char *)ici_stringof(o)->s_chars;
        if (!havesz || sz > ici_stringof(o)->s_nchars)
            sz = ici_stringof(o)->s_nchars;
    }
    else if (ici_ismem(o))
    {
        addr = (char *)ici_memof(o)->m_base;
        if (!havesz || (size_t)sz > ici_memof(o)->m_length * ici_memof(o)->m_accessz)
            sz = ici_memof(o)->m_length * ici_memof(o)->m_accessz;
    }
    else
    {
        return ici_argerror(1);
    }
    return sys_ret(write((int)fd, addr, (size_t)sz));
}

/*
 * symlink(oldpath, newpath)
 *
 * Creates a symbolic link 'newpath' that referes to 'oldpath'.
 * Not supported on Win32.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_symlink()
{
#ifdef _WIN32 /* WINDOWS can't do symlink() */
    return not_on_win32("symlink");
#else
    char        *a, *b;

    if (ici_typecheck("ss", &a, &b))
        return 1;
    if (symlink(a, b) == -1)
        return ici_get_last_errno("symlink", a);
    return ici_ret_no_decref(ici_null);
#endif /* _WIN32 */
}

/*
 * string = readlink(path)
 *
 * Return and return the contents of a symbolic link 'path'.
 * Not supported on Win32.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_readlink()
{
#ifdef _WIN32 /* WINDOWS can't do readlink(). */
    return not_on_win32("fdopen");
#else
    char        *path;
    char        pbuf[MAXPATHLEN+1];

    if (ici_typecheck("s", &path))
        return 1;
    if (readlink(path, pbuf, sizeof pbuf) == -1)
        return ici_get_last_errno("readlink", path);
    return ici_ret_with_decref(ici_str_new_nul_term(pbuf));
#endif /* _WIN32 */
}

/*
 * struct = stat(pathname|int|file)
 *
 * Obtain information on the named file system object, file descriptor or file
 * underlying an ici file object and return a struct containing that
 * information.  If the parameter is a file object that file object must refer
 * to a file opened with ici's fopen function.  The returned struct contains
 * the following keys (which have the same names as the fields of the UNIX
 * statbuf structure with the leading "st_" prefix removed):
 *
 *  dev
 *  ino
 *  mode
 *  nlink
 *  uid
 *  gid
 *  rdev
 *  size
 *  atime
 *  mtime
 *  ctime
 *  blksize
 *  blocks
 *
 * All values are integers. Supported on Win32 platforms.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_stat()
{
    ici_obj_t    *o;
    struct stat statb;
    int         rc;
    ici_struct_t    *s;

    if (ICI_NARGS() != 1)
        return ici_argcount(1);
    o = ICI_ARG(0);
    if (ici_isint(o))
        rc = fstat(ici_intof(o)->i_value, &statb);
    else if (ici_isstring(o))
        rc = stat(ici_stringof(o)->s_chars, &statb);
    else if (ici_isfile(o) && ici_fileof(o)->f_type == &ici_stdio_ftype)
        rc = fstat(fileno((FILE *)ici_fileof(o)->f_file), &statb);
    else
        return ici_argerror(0);
    if (rc == -1)
        return sys_ret(rc);
    if ((s = ici_struct_new()) == NULL)
        return 1;
#define SETFIELD(x)                                     \
    if ((o = ici_int_new(statb.st_ ##x)) == NULL)       \
        goto fail;                                      \
    else if (ici_assign(s, SSO(x), o))                  \
    {                                                   \
        o->decref();                                    \
        goto fail;                                      \
    }                                                   \
    else                                                \
        o->decref()

    SETFIELD(dev);
    SETFIELD(ino);
    SETFIELD(mode);
    SETFIELD(nlink);
    SETFIELD(uid);
    SETFIELD(gid);
    SETFIELD(rdev);
    SETFIELD(size);
    SETFIELD(atime);
    SETFIELD(mtime);
    SETFIELD(ctime);
#ifndef _WIN32
    SETFIELD(blksize);
    SETFIELD(blocks);
#endif

#undef SETFIELD

    return ici_ret_with_decref(s);

 fail:
    s->decref();
    return 1;
}

#ifndef _WIN32
/*
 * struct = lstat(pathname)
 *
 * Same as 'stat' except the argument must be a string, and if it refers
 * to a symbolic link, information on the link is returned rather than the
 * file it refers to. Not supported in Win32 platforms.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_lstat()
{
    ici_obj_t    *o;
    struct stat statb;
    int         rc;
    ici_struct_t    *s;

    if (ICI_NARGS() != 1)
        return ici_argcount(1);
    o = ICI_ARG(0);
    if (ici_isstring(o))
        rc = lstat(ici_stringof(o)->s_chars, &statb);
    else
        return ici_argerror(0);
    if (rc == -1)
        return sys_ret(rc);
    if ((s = ici_struct_new()) == NULL)
        return 1;
#define SETFIELD(x)                                     \
    if ((o = ici_int_new(statb.st_ ##x)) == NULL)       \
        goto fail;                                      \
    else if (ici_assign(s, SSO(x), o))                  \
    {                                                   \
        o->decref();                                    \
        goto fail;                                      \
    }                                                   \
    else                                                \
        o->decref()

    SETFIELD(dev);
    SETFIELD(ino);
    SETFIELD(mode);
    SETFIELD(nlink);
    SETFIELD(uid);
    SETFIELD(gid);
    SETFIELD(rdev);
    SETFIELD(size);
    SETFIELD(atime);
    SETFIELD(mtime);
    SETFIELD(ctime);
    SETFIELD(blksize);
    SETFIELD(blocks);

#undef SETFIELD

    return ici_ret_with_decref(s);

 fail:
    s->decref();
    return 1;
}
#endif

/*
 * string = ctime(int)
 *
 * Convert a time value (see time, below) to a string of the form
 *
 *  "Sun Sep 16 01:03:52 1973 "
 *
 * and return that string.  This is primarily of use when converting the time
 * values returned by stat.  Supported on Win32 platforms.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_ctime()
{
    time_t      timev;
    ici_str_t    *s;

    if (ici_typecheck("i", &timev) || (s = ici_str_new_nul_term(ctime(&timev))) == NULL)
        return 1;
    return ici_ret_with_decref(s);
}

/*
 * int = time()
 *
 * Return the time since 00:00:00 GMT, Jan.  1, 1970, measured in seconds.
 * Note that ICI ints are signed 32 bit quantities, but routines in the 'sys'
 * module that work with times interpret the 32 bit value as an unsigned
 * value.  Supported on Win32 platforms.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_time()
{
    return ici_int_ret(time(NULL));
}

#ifndef _WIN32

static int
assign_timeval(ici_struct_t *s, ici_str_t *k, struct timeval *tv)
{
    ici_struct_t    *ss;
    ici_int_t       *i;

    if (k == NULL)
        ss = s;
    else if ((ss = ici_struct_new()) == NULL)
        return 1;
    if ((i = ici_int_new(tv->tv_usec)) == NULL)
        goto fail;
    if (ici_assign(ss, SSO(usec), i))
    {
        i->decref();
        goto fail;
    }
    i->decref();
    if ((i = ici_int_new(tv->tv_sec)) == NULL)
        goto fail;
    if (ici_assign(ss, SSO(sec), i))
    {
        i->decref();
        goto fail;
    }
    i->decref();
    if (k != NULL && ici_assign(s, k, ss))
        goto fail;
    return 0;

 fail:
    if (k != NULL)
        ss->decref();
    return 1;
}

/*
 * struct = getitimer(which)
 *
 * Invokes 'getitimer(2)' where 'which' is a string with one of the
 * values:
 *
 * "real"               ITIMER_REAL - The real-time timer.
 *
 * "virtual"            ITIMER_VIRTUAL - The user CPU time timer.
 *
 * "prof"               ITIMER_PROF - The system and use CPU time timer.
 *
 * Returns a struct with the keys 'interval' and 'value', each of which
 * are struct with the keys 'sec' and 'usec' being the second and
 * microsecond values of the requested timer.
 *
 * Not supported on Win32 platforms.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_getitimer()
{
    long                which = ITIMER_VIRTUAL;
    ici_struct_t            *s;
    struct itimerval    value;
    ici_obj_t            *o;

    if (ICI_NARGS() != 0)
    {
        if (ici_typecheck("o", &o))
            return 1;
        if (!ici_isstring(o))
            return ici_argerror(0);
        if (o == SSO(real))
            which = ITIMER_REAL;
        else if (o == SSO(virtual))
            which = ITIMER_VIRTUAL;
        else if (o == SSO(prof))
            which = ITIMER_PROF;
        else
            return ici_argerror(0);
    }
    if (getitimer(which, &value) == -1)
        return sys_ret(-1);
    if ((s = ici_struct_new()) == NULL)
        return 1;
    if
    (
        assign_timeval(s, SS(interval), &value.it_interval)
        ||
        assign_timeval(s, SS(value), &value.it_value)
    )
    {
        s->decref();
        return 1;
    }
    return ici_ret_with_decref(s);
}

static int
fetch_timeval(ici_obj_t *s, struct timeval *tv)
{
    ici_obj_t    *o;

    if (!ici_isstruct(s))
        return 1;
    if ((o = ici_fetch(s, SSO(usec))) == ici_null)
        tv->tv_usec = 0;
    else if (ici_isint(o))
        tv->tv_usec = ici_intof(o)->i_value;
    else
        return 1;
    if ((o = ici_fetch(s, SSO(sec))) == ici_null)
        tv->tv_sec = 0;
    else if (ici_isint(o))
        tv->tv_sec = ici_intof(o)->i_value;
    else
        return 1;
    return 0;
}

/*
 * struct = setitimer([which ,] struct)
 *
 * Sets the interval timer named by 'which' (see 'getitimer'), or
 * 'ITIMER_VIRTUAL' by default to the values given in the struct parameter.
 * These are a time value as described in 'getitimer'.  Returns the old
 * value of the timer.
 *
 * Not supported on Win32 platforms.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_setitimer()
{
    long                which = ITIMER_VIRTUAL;
    ici_struct_t            *s;
    struct itimerval    value;
    struct itimerval    ovalue;
    ici_obj_t            *o;

    if (ICI_NARGS() == 1)
    {
        if (ici_typecheck("d", &s))
            return 1;
    }
    else
    {
        if (ici_typecheck("od", &o, &s))
            return 1;
        if (o == SSO(real))
            which = ITIMER_REAL;
        else if (o == SSO(virtual))
            which = ITIMER_VIRTUAL;
        else if (o == SSO(prof))
            which = ITIMER_PROF;
        else
            return ici_argerror(0);
    }
    if ((o = ici_fetch(s, SSO(interval))) == ici_null)
        value.it_interval.tv_sec = value.it_interval.tv_usec = 0;
    else if (fetch_timeval(o, &value.it_interval))
        goto invalid_itimerval;
    if ((o = ici_fetch(s, SSO(value))) == ici_null)
        value.it_value.tv_sec = value.it_value.tv_usec = 0;
    else if (fetch_timeval(o, &value.it_value))
        goto invalid_itimerval;
    if (setitimer(which, &value, &ovalue) == -1)
        return sys_ret(-1);
    if ((s = ici_struct_new()) == NULL)
        return 1;
    if
    (
        assign_timeval(s, SS(interval), &ovalue.it_interval)
        ||
        assign_timeval(s, SS(value), &ovalue.it_value)
    )
    {
        s->decref();
        return 1;
    }
    return ici_ret_with_decref(s);

 invalid_itimerval:
    ici_set_error("invalid itimer struct");
    return 1;
}

/*
 * struct = gettimeofday()
 *
 * Returns the result of 'gettimeofday(2)' as a struct as described
 * in 'getitimer'.
 *
 * Not supported on Win32 platforms.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_gettimeofday()
{
    ici_struct_t            *s;
    struct timeval      tv;

    if (gettimeofday(&tv, NULL) == -1)
        return sys_ret(-1);
    if ((s = ici_struct_new()) == NULL)
        return 1;
    if (assign_timeval(s, NULL, &tv))
    {
        s->decref();
        return 1;
    }
    return ici_ret_with_decref(s);
}
#endif /* _WIN32 */


/*
 * int = access(string [, int])
 *
 * Call the 'access(2)' function to determine the accessibility of a file.
 * The first parameter is the pathname of the file system object to be tested.
 * The second, optional, parameter is the mode (a bitwise combination of
 * 'R_OK', 'W_OK' and 'X_OK' or the special value, 'F_OK').
 * If mode is not passed 'F_OK' is assumed.  Access returns 0 if the file
 * system object is accessible, else it fails.
 *
 * Supported on Win32 platforms.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_access()
{
    char        *fname;
    int         bits = F_OK;

    switch (ICI_NARGS())
    {
    case 1:
        if (ici_typecheck("s", &fname))
            return 1;
        break;
    case 2:
        if (ici_typecheck("si", &fname, &bits))
            return 1;
        break;
    default:
        return ici_argcount(2);
    }
    return sys_ret(access(fname, bits));
}

/*
 * array = pipe()
 *
 * Create a pipe and return an array containing two, integer, file descriptors
 * used to refer to the input and output endpoints of the pipe.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_pipe()
{
#ifdef _WIN32
    return not_on_win32("pipe");
#else
    int         pfd[2];
    ici_array_t     *a;
    ici_int_t       *fd;

    if ((a = ici_array_new(2)) == NULL)
        return 1;
    if (pipe(pfd) == -1)
    {
        a->decref();
        return sys_ret(-1);
    }
    if ((fd = ici_int_new(pfd[0])) == NULL)
        goto fail;
    *a->a_top++ = fd;
    fd->decref();
    if ((fd = ici_int_new(pfd[1])) == NULL)
        goto fail;
    *a->a_top++ = fd;
    fd->decref();
    return ici_ret_with_decref(a);

 fail:
    a->decref();
    close(pfd[0]);
    close(pfd[1]);
    return 1;
#endif  /* #ifndef _WIN32 */
}

/*
 * int = creat(pathname, mode)
 *
 * Create a new ordinary file with the given pathname and mode (permissions
 * etc.) and return the file descriptor, open for writing, for the file.
 *
 * Supported on Win32 platforms.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_creat()
{
    char        *fname;
    long        perms;
    int         fd;

    if (ici_typecheck("si", &fname, &perms))
        return 1;
    if ((fd = creat(fname, perms)) == -1)
        return ici_get_last_errno("creat", fname);
    return ici_int_ret(fd);
}

/*
 * int = dup(int [, int])
 *
 * Duplicate a file descriptor by calling 'dup(2)' or 'dup2(2)' and return a new
 * descriptor.  If only a single parameter is passed 'dup(2)' is called
 * otherwise 'dup2(2)' is called. 
 *
 * Supported on Win32 platforms.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_dup()
{
    long        fd1;
    long        fd2;

    switch (ICI_NARGS())
    {
    case 1:
        if (ici_typecheck("i", &fd1))
            return 1;
        fd2 = -1;
        break;
    case 2:
        if (ici_typecheck("ii", &fd1, &fd2))
            return 1;
        break;
    default:
        return ici_argcount(2);
    }
    if (fd2 == -1)
    {
        fd2 = dup(fd1);
        return fd2 < 0 ? sys_ret(fd2) : ici_int_ret(fd2);
    }
    if (dup2(fd1, fd2) < 0)
        return sys_ret(-1);
    return ici_int_ret(fd2);
}

/*
 * exec(pathname, array|string...)
 *
 * Execute a new program in the current process.  The first parameter to
 * 'exec' is the pathname of an executable file (the program).  The remaining
 * parameters are either; an array of strings defining the parameters to be
 * passed to the program, or, a variable number of strings that are passed, in
 * order, to the program as its parameters.  The first form is similar to C's
 * 'execv' function and the second form to C's 'execl' functions.  Note that
 * no searching of the user's path is performed and the environment passed to
 * the program is that of the current process (i.e., both are implemented by
 * calls to 'execv(2)').  This function is available on Win32 platforms.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_exec()
{
    char        *sargv[16];
    char        **argv;
    int         maxargv;
    int         n;
    ici_obj_t    **o;
    char        *path;
    int         argc;

#define ADDARG(X)                                                       \
    {                                                                   \
        if (argc >= 16)                                                 \
        {                                                               \
            if (argc >= maxargv)                                        \
            {                                                           \
                char    **newp;                                         \
                int      i;                                             \
                maxargv += argc;                                        \
                if ((newp = (char **)ici_alloc(maxargv * sizeof (char *))) == NULL) \
                {                                                       \
                    if (argv != sargv)                                  \
                        ici_free(argv);                                 \
                    return 1;                                           \
                }                                                       \
                for (i = 0; i < argc; ++i)                              \
                    newp[i] = argv[i];                                  \
                if (argv != sargv)                                      \
                    ici_free(argv);                                     \
                argv = newp;                                            \
            }                                                           \
        }                                                               \
        argv[argc++] = (X);                                             \
    }

    if ((n = ICI_NARGS()) < 2)
        return ici_argcount(2);
    if (!ici_isstring(*(o = ICI_ARGS())))
        return ici_argerror(0);
    path = ici_stringof(*o)->s_chars;
    --o;
    argc = 0;
    argv = sargv;
    maxargv = 16;
    if (n > 2 || ici_isstring(*o))
    {
        while (--n > 0)
        {
            ADDARG(ici_stringof(*o)->s_chars);
            --o;
        }
    }
    else if (!ici_isarray(*o))
        return ici_argerror(0);
    else
    {
        ici_obj_t **p;
        ici_array_t *a;

        a = ici_arrayof(*o);
        for (p = ici_astart(a); p < ici_alimit(a); p = ici_anext(a, p))
            if (ici_isstring(*p))
                ADDARG(ici_stringof(*p)->s_chars);
    }
    ADDARG(NULL);
    n = (*(int (*)(...))ICI_CF_ARG1())(path, argv);
    if (argv != sargv)
        ici_free(argv);
    return sys_ret(n);
}

#ifdef _WIN32
/*
 * int = spawn([mode, ] string, array|string...)
 *
 * Spawn a sub-process.  The parameters, other than mode, are as for exec -
 * the string is the name of the executable and the remaining parameters form
 * the command line arguments passed to the executable.  The mode parameter
 * controls whether or not the parent process waits for the spawned process to
 * termiante.  If mode is _P_WAIT the call to spawn returns when the process
 * terminates and the result of spawn is the process exit status.  If mode is
 * not passed or is _P_NOWAIT the call to spawn returns prior to the process
 * terminating and the result is the Win32 process handle for the new process.
 *
 * This function is only available on Win32 platforms.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
/*
 * int = spawnp([mode, ] string, array|string...)
 *
 * As for 'spawn' except it will search the directories listed in the PATH
 * environment variable for the executable program.
 *
 * This function is only available on Win32 platforms.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_spawn()
{
    char        *sargv[16];
    char        **argv;
    int         maxargv;
    int         n;
    ici_obj_t    **o;
    char        *path;
    int         argc;
    int         mode = _P_NOWAIT;

#define ADDARG(X)                                                       \
    {                                                                   \
        if (argc >= 16)                                                 \
        {                                                               \
            if (argc >= maxargv)                                        \
            {                                                           \
                char    **newp;                                         \
                int      i;                                             \
                maxargv += argc;                                        \
                if ((newp = ici_alloc(maxargv * sizeof (char *))) == NULL) \
                {                                                       \
                    if (argv != sargv)                                  \
                        ici_free(argv);                                 \
                    return 1;                                           \
                }                                                       \
                for (i = 0; i < argc; ++i)                              \
                    newp[i] = argv[i];                                  \
                if (argv != sargv)                                      \
                    ici_free(argv);                                     \
                argv = newp;                                            \
            }                                                           \
        }                                                               \
        argv[argc++] = (X);                                             \
    }

    if ((n = ICI_NARGS()) < 2)
        return ici_argcount(2);
    o = ICI_ARGS();
    if (ici_isint(*o))
    {
        mode = ici_intof(*o)->i_value;
        --o;
        if (--n < 2)
            return ici_argcount(2);
        if (!ici_isstring(*o))
            return ici_argerror(1);
    }
    else if (!ici_isstring(*o))
        return ici_argerror(0);
    path = stringof(*o)->s_chars;
    --o;
    argc = 0;
    argv = sargv;
    maxargv = 16;
    if (n > 2 || ici_isstring(*o))
    {
        while (--n > 0)
        {
            ADDARG(stringof(*o)->s_chars);
            --o;
        }
    }
    else if (!isarray(*o))
        return ici_argerror(0);
    else
    {
        ici_obj_t **p;
        ici_array_t *a;

        a = arrayof(*o);
        for (p = ici_astart(a); p < ici_alimit(a); p = ici_anext(a, p))
            if (ici_isstring(*p))
                ADDARG(stringof(*p)->s_chars);
    }
    ADDARG(NULL);
    n = (*(int (*)())ICI_CF_ARG1())(mode, path, argv);
    if (argv != sargv)
        ici_free(argv);
    if (n == -1)
        return sys_ret(n);
    return ici_int_ret(n);
}
#endif /* WIN32 */

/*
 * int = lseek(int, int [, int])
 *
 * Set the read/write position for an open file.  The first parameter is the
 * file descriptor associated with the file system object, the second
 * parameter the offset.  The third is the whence value which determines how
 * the new file position is calculated.  The whence value may be one of
 * SEEK_SET, SEEK_CUR or SEEK_END and defaults to SEEK_SET if not specified.
 * supported on WIN32 platforms.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_lseek()
{
    long        fd;
    long        ofs;
    long        whence = SEEK_SET;

    switch (ICI_NARGS())
    {
    case 2:
        if (ici_typecheck("ii", &fd, &ofs))
            return 1;
        break;
    case 3:
        if (ici_typecheck("iii", &fd, &ofs, &whence))
            return 1;
        break;

    default:
        return ici_argcount(3);
    }
    return sys_ret(lseek((int)fd, ofs, whence));
}

/*
 * struct = wait()
 *
 * Wait for a child process to exit and return a struct containing the process
 * id (key of "pid") and the exit status (key of "status").
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_wait()
{
#ifdef _WIN32
    return not_on_win32("wait");
#else
    int                 pid;
    ici_struct_t        *s;
    ici_int_t           *i;
    int                 status;

    if ((pid = wait(&status)) < 0)
        return sys_ret(-1);
    if ((s = ici_struct_new()) == NULL)
        return 1;
    if ((i = ici_int_new(pid)) == NULL)
        goto fail;
    if (ici_assign(s, SSO(pid), i))
    {
        i->decref();
        goto fail;
    }
    i->decref();
    if ((i = ici_int_new(status)) == NULL)
        goto fail;
    if (ici_assign(s, SSO(status), i))
    {
        i->decref();
        goto fail;
    }
    i->decref();
    return ici_ret_with_decref(s);

 fail:
    s->decref();
    return 1;
#endif /* _WIN32 */
}

#ifndef _WIN32

static ici_struct_t *password_struct(struct passwd *);

/*
 * struct|array = passwd([int | string])
 *
 * The 'passwd()' function accesses the Unix password file (which may or
 * may not be an actual file according to the local system configuration).
 * With no parameters 'passwd()' returns an array of all password file
 * entries, each entry is a struct.  With a parameter 'passwd()' returns
 * the entry for the specific user id., int parameter, or user name, string
 * parameter.  A password file entry is a struct with the following keys and
 * values:
 *
 * name                 The user's login name, a string.
 *
 * passwd               The user's encrypted password, a string.  Note that
 *                      some systems protect this (shadow password files) and
 *                      this field may not be an actual encrypted password.
 *
 * uid                  The user id., an int.
 *
 * gid                  The user's default group, an int.
 *
 * gecos                The so-called gecos field, a string.
 *
 * dir                  The user's home directory, a string.
 *
 * shell                The user's shell (initial program), a string.
 *
 * Not supported on Win32.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_passwd()
{
    struct passwd       *pwent;
    ici_array_t             *a;

    switch (ICI_NARGS())
    {
    case 0:
        break;

    case 1:
        if (ici_isint(ICI_ARG(0)))
            pwent = getpwuid((uid_t)ici_intof(ICI_ARG(0))->i_value);
        else if (ici_isstring(ICI_ARG(0)))
            pwent = getpwnam(ici_stringof(ICI_ARG(0))->s_chars);
        else
            return ici_argerror(0);
        if (pwent == NULL)
            ici_set_error("no such user");
        return ici_ret_with_decref(password_struct(pwent));

    default:
        return ici_argcount(1);
    }

    if ((a = ici_array_new(0)) == NULL)
        return 1;
    setpwent();
    while ((pwent = getpwent()) != NULL)
    {
        ici_struct_t        *s;

        if (ici_stk_push_chk(a, 1) || (s = password_struct(pwent)) == NULL)
        {
            a->decref();
            return 1;
        }
        *a->a_top++ = s;
    }
    endpwent();
    return ici_ret_with_decref(a);
}

static ici_struct_t *
password_struct(struct passwd *pwent)
{
    ici_struct_t    *d;
    ici_obj_t    *o;

    if (pwent == NULL)
        return NULL;
    if ((d = ici_struct_new()) != NULL)
    {

#define SET_INT_FIELD(x)                                \
        if ((o = ici_int_new(pwent->pw_ ##x)) == NULL)  \
            goto fail;                                  \
        else if (ici_assign(d, SSO(x), o))              \
        {                                               \
            o->decref();                                \
            goto fail;                                  \
        }                                               \
        else                                            \
            o->decref()

#define SET_STR_FIELD(x)                                                \
        if ((o = ici_str_new_nul_term(pwent->pw_ ##x)) == NULL)        \
            goto fail;                                                  \
        else if (ici_assign(d, SSO(x), o))                              \
        {                                                               \
            o->decref();                                                \
            goto fail;                                                  \
        }                                                               \
        else                                                            \
            o->decref()

        SET_STR_FIELD(name);
        SET_STR_FIELD(passwd);
        SET_INT_FIELD(uid);
        SET_INT_FIELD(gid);
        SET_STR_FIELD(gecos);
        SET_STR_FIELD(dir);
        SET_STR_FIELD(shell);

#undef SET_STR_FIELD
#undef SET_INT_FIELD

    }
    return d;

 fail:
    d->decref();
    return NULL;
}

/*
 * string = getpass([prompt])
 *
 * Use the 'getpass(3)' function to read a password (without echo).
 * 'prompt' is a string (if present).
 *
 * Not supported on Win32.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_getpass()
{
    const char *prompt = "Password: ";

    if (ICI_NARGS() > 0)
    {
        if (ici_typecheck("s", &prompt))
            return 1;
    }
    return ici_str_ret(getpass(prompt));
}

/*
 * setpgrp()
 *
 * Set the process group.
 *
 * Not supported on Win32.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_setpgrp()
{
#ifdef SETPGRP_2_ARGS
    long        pid, pgrp;
#else
    long        pgrp;
#endif

#ifdef SETPGRP_2_ARGS
    switch (ICI_NARGS())
    {
    case 1:
        pid = 0;
#endif
        if (ici_typecheck("i", &pgrp))
            return 1;
#ifdef SETPGRP_2_ARGS
        break;

    default:
        if (ici_typecheck("ii", &pid, &pgrp))
            return 1;
        break;
    }
    return sys_ret(setpgrp((pid_t)pid, (pid_t)pgrp));
#elif defined SETPGRP_0_ARGS
    return sys_ret(setpgrp());
#else
    return sys_ret(setpgrp((pid_t)pgrp));
#endif
}

#ifndef ICI_SYS_NOFLOCK

/*
 * flock(fd, op)
 *
 * Invoke 'flock(2)' on the given file descriptor with the given operation.
 * See the introduction for values for 'op'.  Not available on some systems.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_flock()
{
    long        fd, operation;

    if (ici_typecheck("ii", &fd, &operation))
        return 1;
    return sys_ret(flock(fd, operation));
}
#endif /* ICI_SYS_NOFLOCK */

#endif /* _WIN32 */

#ifdef _WIN32

/*
 * Win32 does not provide ftruncate()/truncate(), so roll our own.
 * It provides _chsize() which does the same as ftruncate().
 */
static int ftruncate(int fd, long length)
{
    return _chsize(fd, length);
}
static int truncate(const char *path, long length)
{
    long        fd;
    int         ret;

    fd = open(path, _O_WRONLY);
    if (fd == -1)
        return -1;
    ret = _chsize(fd, length);
    close(fd);
    return ret;
}

#endif

/*
 * truncate(int|string, len)
 *
 * Use 'truncate(2)' or 'ftrunctate(2)' to truncate a file to 'len'
 * bytes.
 *
 * Supported on Win32 platforms.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_truncate()
{
    long        fd;
    long        len;
    char        *s;

    if (ici_typecheck("ii", &fd, &len) == 0)
    {
        if (ftruncate(fd, len) == -1)
            return sys_ret(-1L);
        return ici_null_ret();
    }
    if (ici_typecheck("si", &s, &len) == 0)
    {
        if (truncate(s, len) == -1)
            return ici_get_last_errno("truncate", s);
        return ici_null_ret();
    }
    return 1;
}

#ifndef _WIN32

static int
string_to_resource(ici_obj_t *what)
{
    if (what == SSO(core))
        return RLIMIT_CORE;
    if (what == SSO(cpu))
        return RLIMIT_CPU;
    if (what == SSO(data))
        return RLIMIT_DATA;
    if (what == SSO(fsize))
        return RLIMIT_FSIZE;
#if defined(RLIMIT_MEMLOCK)
    if (what == SSO(memlock))
        return RLIMIT_MEMLOCK;
#endif
    if (what == SSO(nofile))
        return RLIMIT_NOFILE;
#if defined(RLIMIT_NPROC)
    if (what == SSO(nproc))
        return RLIMIT_NPROC;
#endif
#if defined(RLIMIT_RSS)
    if (what == SSO(rss))
        return RLIMIT_RSS;
#endif
    if (what == SSO(stack))
        return RLIMIT_STACK;

#if !(defined __MACH__ && defined __APPLE__)
#define NO_RLIMIT_DBSIZE
#endif
#if __FreeBSD__ < 4
#define NO_RLIMIT_SBSIZE
#endif

#ifndef NO_RLIMIT_SBSIZE
    if (what == SSO(sbsize))
        return RLIMIT_SBSIZE;
#endif
    return -1;
}

/*
 * struct = getrlimit(resource)
 *
 * Use 'getrlimit(2)' to return th current and maximum values of
 * a set of system parameters identified by 'resource'. 'resource'
 * may be an int, or one of the strings:
 *
 * "core"               RLIMIT_CORE - max core file size.
 *
 * "cpu"                RLIMIT_CPU - CPU time in seconds.
 *
 * "data"               RLIMIT_DATA - max data size.
 *
 * "fsize"              RLIMIT_FSIZE - maximum filesize.
 *
 * "memlock"            RLIMIT_MEMLOCK - max locked-in-memory address space.
 *
 * "nofile"             RLIMIT_NOFILE - max number of open files.
 *
 * "nproc"              RLIMIT_NPROC - max number of processes.
 *
 * "rss"                RLIMIT_RSS - max resident set size.
 *
 * "stack"              RLIMIT_STACK - max stack size.
 *
 * "sbsize"             RLIMIT_SBSIZE - max socket buffer size.
 *
 * The returned struct has two integer fields, 'cur' and 'max'.
 *
 * Not supported on Win32.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_getrlimit()
{
    ici_obj_t            *what;
    int                 resource;
    struct rlimit       rlimit;
    ici_struct_t            *limit;
    ici_int_t               *iv;

    if (ici_typecheck("o", &what))
        return 1;
    if (ici_isint(what))
        resource = ici_intof(what)->i_value;
    else if (ici_isstring(what))
    {
        if ((resource = string_to_resource(what)) == -1)
            return ici_argerror(0);
    }
    else
        return ici_argerror(0);

    if (getrlimit(resource, &rlimit) < 0)
        return sys_ret(-1);

    if ((limit = ici_struct_new()) == NULL)
        return 1;
    if ((iv = ici_int_new(rlimit.rlim_cur)) == NULL)
        goto fail;
    if (ici_assign(limit, SSO(cur), iv))
    {
        iv->decref();
        goto fail;
    }
    iv->decref();
    if ((iv = ici_int_new(rlimit.rlim_max)) == NULL)
        goto fail;
    if (ici_assign(limit, SSO(max), iv))
    {
        iv->decref();
        goto fail;
    }
    iv->decref();
    return ici_ret_with_decref(limit);

 fail:
    limit->decref();
    return 1;
}

/*
 * setrlimit(resource, value)
 *
 * Use 'setrlimit(2)' to set a resouce limit.  'resource' identifies which (as
 * per 'getrlimit'.  'value' may be an int, the string "infinity", or a
 * struct ints in fields 'cur' and 'max'.
 *
 * Not supported on Win32.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_setrlimit()
{
    ici_obj_t            *what;
    ici_obj_t            *value;
    struct rlimit       rlimit;
    int                 resource;
    ici_obj_t            *iv;

    if (ici_typecheck("oo", &what, &value))
        return 1;
    if (ici_isint(what))
        resource = ici_intof(what)->i_value;
    else if (ici_isstring(what))
    {
        if ((resource = string_to_resource(what)) == -1)
            return ici_argerror(0);
    }
    else
        return ici_argerror(0);

    if (ici_isint(value))
    {
        if (getrlimit(resource, &rlimit) < 0)
            return sys_ret(-1);
        rlimit.rlim_cur = ici_intof(value)->i_value;
    }
    else if (value == SSO(infinity))
    {
        rlimit.rlim_cur = RLIM_INFINITY;
    }
    else if (ici_isstruct(value))
    {
        if ((iv = ici_fetch(value, SSO(cur))) == ici_null)
            goto fail;
        if (!ici_isint(iv))
            goto fail;
        rlimit.rlim_cur = ici_intof(iv)->i_value;
        if ((iv = ici_fetch(value, SSO(max))) == ici_null)
            goto fail;
        if (!ici_isint(iv))
            goto fail;
        rlimit.rlim_max = ici_intof(iv)->i_value;
    }
    else
        return ici_argerror(1);

    return sys_ret(setrlimit(resource, &rlimit));

 fail:
    ici_set_error("invalid rlimit struct");
    return 1;
}

/*
 * usleep(int)
 *
 * Suspend the process for the specified number of milliseconds.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int ici_sys_usleep()
{
    long    t;
    ici_exec_t *x;

    if (ici_typecheck("i", &t))
        return 1;
    ici_signals_blocking_syscall(1);
    x = ici_leave();
    usleep(t);
    ici_enter(x);
    ici_signals_blocking_syscall(0);
    return ici_null_ret();
}

#endif /* #ifndef _WIN32 */

ICI_DEFINE_CFUNCS(sys)
{
    /* utime */
    ICI_DEFINE_CFUNC(access,  ici_sys_access),
    ICI_DEFINE_CFUNC(_close,   ici_sys_close),
    ICI_DEFINE_CFUNC(creat,   ici_sys_creat),
    ICI_DEFINE_CFUNC(ctime,   ici_sys_ctime),
    ICI_DEFINE_CFUNC(dup,     ici_sys_dup),
    ICI_DEFINE_CFUNC1(exec,   ici_sys_exec, execv),
    ICI_DEFINE_CFUNC1(execp,  ici_sys_exec, execvp),
    /*
     * _exit(int)
     *
     * Exit the current process returning an integer exit status to the
     * parent.  Supported on Win32 platforms.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(_exit,   ici_sys_simple, _exit,  "i"),
    ICI_DEFINE_CFUNC(fcntl,   ici_sys_fcntl),
    ICI_DEFINE_CFUNC(fdopen,  ici_sys_fdopen),
    ICI_DEFINE_CFUNC(fileno,  ici_sys_fileno),
    ICI_DEFINE_CFUNC(lseek,   ici_sys_lseek),
    ICI_DEFINE_CFUNC(mkdir,   ici_sys_mkdir),
    ICI_DEFINE_CFUNC(mkfifo,  ici_sys_mkfifo),
    ICI_DEFINE_CFUNC(open,    ici_sys_open),
    ICI_DEFINE_CFUNC(pipe,    ici_sys_pipe),
    ICI_DEFINE_CFUNC(read,    ici_sys_read),
    ICI_DEFINE_CFUNC(readlink,ici_sys_readlink),
    /*
     * rmdir(pathname)
     *
     * Remove a directory. Supported on Win32 platforms.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(rmdir,  ici_sys_simple, rmdir,  "s"),
    ICI_DEFINE_CFUNC(stat,    ici_sys_stat),
    ICI_DEFINE_CFUNC(symlink, ici_sys_symlink),
    ICI_DEFINE_CFUNC(time,    ici_sys_time),
    /*
     * unlink(pathname)
     *
     * Unlink a file. Supported on WIN32 platforms.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    /* should go as remove(}, is more portable */
    ICI_DEFINE_CFUNC2(unlink, ici_sys_simple, unlink, "s"),
    ICI_DEFINE_CFUNC(wait,    ici_sys_wait),
    ICI_DEFINE_CFUNC(write,   ici_sys_write),
#ifdef _WIN32
    ICI_DEFINE_CFUNC1(spawn,  ici_sys_spawn, spawnv),
    ICI_DEFINE_CFUNC1(spawnp, ici_sys_spawn, spawnvp),
#endif
#ifndef _WIN32
    /* poll */
    /* times */
    /* uname */
    /*
     * alarm(int)
     * 
     * Schedule a SIGALRM signal to be posted to the current process in
     * the specified number of seconds.  If the parameter is zero any
     * alarm is cancelled. Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(alarm,   ici_sys_simple, alarm,  "i"),
    /*
     * chmod(pathname, int)
     *
     * Change the mode of a file system object. Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(chmod,   ici_sys_simple, chmod,  "si"),
    /*
     * chown(pathname, uid, gid)
     *
     * Use 'chown(2)' to change the ownership of a file to new integer
     * user and group indentifies. Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(chown,   ici_sys_simple, chown,  "sii"),
    /*
     * chroot(pathname)
     *
     * Change root directory for process. Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(chroot,  ici_sys_simple, chroot, "s"),
    /*
     * int = clock()
     *
     * Return the value of 'clock(2)'.  Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(clock,   ici_sys_simple, clock,  ""),
    /*
     * int = fork()
     *
     * Create a new process.  In the parent this returns the process
     * identifier for the newly created process.  In the newly created
     * process it returns zero. Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(fork,    ici_sys_simple, fork,   ""),
    /*
     * int = getegid()
     *
     * Get the effective group identifier of the owner of the current process.
     * Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(getegid, ici_sys_simple, getegid,""),
    /*
     * int = geteuid()
     *
     * Get the effective user identifier of the owner of the current process.
     * Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(geteuid, ici_sys_simple, geteuid,""),
    /*
     * int = getgid()
     *
     * Get the real group identifier of the owner of the current process.
     * Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(getgid,  ici_sys_simple, getgid, ""),
    ICI_DEFINE_CFUNC(getitimer,ici_sys_getitimer),
    ICI_DEFINE_CFUNC(getpass, ici_sys_getpass),
    /*
     * int = getpgrp()
     *
     * Get the current process group identifier.  Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(getpgrp, ici_sys_simple, getpgrp,""),
    /*
     * int = getpid()
     *
     * Get the process identifier for the current process.  Not supported
     * on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(getpid,  ici_sys_simple, getpid, ""),
    /*
     * int = getppid()
     *
     * Get the process identifier for the parent process.  Not supported
     * on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(getppid, ici_sys_simple, getppid,""),
    ICI_DEFINE_CFUNC(getrlimit,ici_sys_getrlimit),
    ICI_DEFINE_CFUNC(gettimeofday,ici_sys_gettimeofday),
    /*
     * int = getuid()
     *
     * Get the real user identifier of the owner of the current process.
     * Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(getuid,  ici_sys_simple, getuid, ""),
    /*
     * int = isatty(fd)
     *
     * Returns 1 if the int 'fd' is the open file descriptor of a "tty".
     * Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(isatty,  ici_sys_simple, isatty, "i"),
    /*
     * kill(int, int)
     *
     * Post the signal specified by the second argument to the process
     * with process ID given by the first argument.  Not supported on
     * Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(kill,    ici_sys_simple, kill,   "ii"),
    /*
     * link(oldpath, newpath)
     *
     * Create a link to an existing file.  Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(link,    ici_sys_simple, link,   "ss"),
    ICI_DEFINE_CFUNC(lstat,   ici_sys_lstat),
    /*
     * mknod(pathname, int, int)
     *
     * Create a special file with mode given by the second argument and
     * device type given by the third.  Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(mknod,   ici_sys_simple, mknod,  "sii"),
    /*
     * nice(int)
     *
     * Change the nice value of a process.
     * Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(nice,    ici_sys_simple, nice,   "i"),
    ICI_DEFINE_CFUNC(passwd,  ici_sys_passwd),
    /*
     * pause()
     *
     * Wait until a signal is delivered to the process.
     * Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(pause,   ici_sys_simple, pause,  ""),
    /*
     * setgid(int)
     *
     * Set the real and effective group identifier for the current process.
     * Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(setgid,  ici_sys_simple, setgid, "i"),
    ICI_DEFINE_CFUNC(setitimer,ici_sys_setitimer),
    ICI_DEFINE_CFUNC(setpgrp, ici_sys_setpgrp),
    ICI_DEFINE_CFUNC(setrlimit,ici_sys_setrlimit),
    /*
     * setuid(int)
     *
     * Set the real and effective user identifier for the current process.
     * Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(setuid,  ici_sys_simple, setuid, "i"),
    /*
     * sync()
     *
     * Schedule in-memory file data to be written to disk.
     * Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(sync,    ici_sys_simple, sync,   ""),
#endif
    ICI_DEFINE_CFUNC(truncate,ici_sys_truncate),
#ifndef _WIN32
    /*
     * umask(int)
     *
     * Set file creation mask.
     * Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(umask,   ici_sys_simple, umask,  "i"),
    ICI_DEFINE_CFUNC(usleep,  ici_sys_usleep),
#ifndef NO_ACCT
    /*
     * acct(pathname)
     *
     * Enable accounting on the specified file.  Not suppored on
     * cygwin or Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(acct,    ici_sys_simple, acct,   "s"),
#endif
#ifndef ICI_SYS_NOFLOCK
    ICI_DEFINE_CFUNC(flock,   ici_sys_flock),
#endif
#if !defined(__linux__) && !defined(BSD4_4) && !defined(__CYGWIN__)
    /*
     * int = lockf(fd, cmd, len)
     *
     * Invoked 'lockf(3)' on a file.
     *
     * Not supported on Linux, Cygwin, Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(lockf,   ici_sys_simple, lockf,  "iii"),
#endif /* __linux__ */
#if !defined(BSD4_4) && !defined(__CYGWIN__)
    /*
     * ulimit(int, int)
     *
     * Get and set user limits.
     * Not supported on Win32, NeXT, some BSD, or Cygwin.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(ulimit,  ici_sys_simple, ulimit, "ii"),
#endif
#endif
    ICI_CFUNCS_END
};

} // namespace ici