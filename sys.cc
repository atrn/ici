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
#include "map.h"
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

#if !defined(MAXPATHLEN)
#include <sys/param.h>
#endif

#if defined(__linux__) || defined(__sun__) || defined(__CYGWIN__) || defined(__APPLE__)
#define SETPGRP_0_ARGS
#endif

#if defined BSD && !defined __APPLE__
#define SETPGRP_2_ARGS
#endif

#if defined(__linux__) && defined(BSD)
#error __linux__ or BSD for setpgrp(), not both
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
 * Used as a common error return for system calls that fail. Sets the
 * global error string if the call fails otherwise returns the integer
 * result of the system call.
 */
static int
sys_ret(int ret)
{
    char        n1[objnamez];

    if (ret < 0)
        return get_last_errno(objname(n1, os.a_top[-1]), nullptr);
    return int_ret((long)ret);
}

/*
 * Used to call "simple" system calls. Simple calls are those that take
 * up to four integer parameters and return an integer. FUNCDEF() just
 * makes the name unique for this module. We have to use CFUNC3()
 * to use it, see below.
 */
static int sys_simple()
{
    long        av[4];

    if (typecheck((const char *)ICI_CF_ARG2(), &av[0], &av[1], &av[2], &av[3]))
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
static int sys_open()
{
    char        *fname;
    long        omode;
    long        perms = -1; /* -1 means not passed */

    switch (NARGS())
    {
    case 2:
        if (typecheck("si", &fname, &omode))
            return 1;
        break;

    case 3:
        if (typecheck("sii", &fname, &omode, &perms))
            return 1;
        break;

    default:
        return argcount(3);
    }
    if (omode & O_CREAT && perms == -1)
    {
        set_error("permission bits not specified in open() with O_CREAT");
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
    sprintf(buf, "%s is not implemented on Win32 platforms", s);
    set_error(buf);
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
static int sys_fdopen()
{
#ifdef _WIN32 /* WINDOWS can't do fdopen() without lots of work */
    return not_on_win32("fdopen");
#else
    long        fd;
    const char  *mode;
    FILE        *stream;
    file  *f;

    switch (NARGS())
    {
    case 1:
        if (typecheck("i", &fd))
            return 1;
        mode = "r";
        break;
    case 2:
        if (typecheck("is", &fd, &mode))
            return 1;
        break;
    default:
        return argcount(2);
    }
    if ((stream = fdopen(fd, mode)) == nullptr)
    {
        set_error("can't fdopen");
        return 1;
    }
    setvbuf(stream, nullptr, _IOLBF, 0);
    if ((f = new_file((char *)stream, stdio_ftype, nullptr, nullptr)) == nullptr)
    {
        fclose(stream);
        return 1;
    }
    return ret_with_decref(f);
#endif /* _WIN32 */
}

/*
 * _close(int)
 *
 * Close a file descriptor.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int sys_close()
{
    int                 rc;
    object            *fd0;
    object            *fd1;

    if (NARGS() != 1)
        return argcount(1);
    if (isint(ARG(0)))
        rc = close(intof(ARG(0))->i_value);
    else if (isarray(ARG(0)))
    {
        array *a = arrayof(ARG(0));

        if
        (
            a->len() != 2
            ||
            !isint(fd0 = a->get(0))
            ||
            !isint(fd1 = a->get(1))
        )
        {
            set_error("invalid fd array passed to _close");
            return 1;
        }
        rc = close(intof(fd0)->i_value);
        if (rc == 0)
            rc = close(intof(fd1)->i_value);
    }
    else
        return argerror(0);
    return sys_ret(rc);
}


#ifndef _WIN32

/* Convert a struct to a struct flock for fcntl's F_SETLK */

static int
struct_to_flock(map *d, struct flock *flock)
{
    object    *o;

    if ((o = ici_fetch(d, SS(start))) == null)
        flock->l_start = 0;
    else
        flock->l_start = intof(o)->i_value;
    if ((o = ici_fetch(d, SS(len))) == null)
        flock->l_len = 0;
    else
        flock->l_len = intof(o)->i_value;
    if ((o = ici_fetch(d, SS(type))) == null)
        flock->l_type = F_RDLCK;
    else if (isstring(o))
    {
        if (o == SS(rdlck))
            flock->l_type = F_RDLCK;
        else if (o == SS(wrlck))
            flock->l_type = F_WRLCK;
        else if (o == SS(unlck))
            flock->l_type = F_UNLCK;
        else
            goto bad_lock_type;
    }
    else if (isint(o))
        flock->l_type = intof(o)->i_value;
    else
    {
    bad_lock_type:
        set_error("invalid lock type");
        return 1;
    }
    if ((o = ici_fetch(d, SS(whence))) == null)
        flock->l_whence = SEEK_SET;
    else
        flock->l_whence = intof(o)->i_value;
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
 * struct (or nullptr) that will be used to populate a 'struct flock' with the
 * following items (a default is used for each if missing or nullptr):
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
static int sys_fcntl()
{
#ifdef _WIN32
    return not_on_win32("fcntl");
#else
    long        fd;
    str    *what;
    object    *arg;
    int         iarg;
    int         iwhat;
    int         r;

    switch (NARGS())
    {
    case 2:
        iarg = 1;
        arg = null;
        if (typecheck("io", &fd, &what))
            return 1;
        break;

    case 3:
        if (typecheck("ioo", &fd, &what, &arg))
            return 1;
        if (isint(arg))
            iarg = intof(arg)->i_value;
        else
            iarg = 0;
        break;

    default:
        return argcount(3);
    }
    if (!isstring(what))
        return argerror(1);
    if (what == SS(dupfd))
        iwhat = F_DUPFD;
    else if (what == SS(getfd))
        iwhat = F_GETFD;
    else if (what == SS(setfd))
        iwhat = F_SETFD;
    else if (what == SS(getfl))
        iwhat = F_GETFL;
    else if (what == SS(setfl))
        iwhat = F_SETFL;
    else if (what == SS(getown))
        iwhat = F_GETOWN;
    else if (what == SS(setown))
        iwhat = F_SETOWN;
    else if (what == SS(setlk))
    {
        struct flock myflock;

        memset(&myflock, 0, sizeof myflock);
        myflock.l_type = F_RDLCK;
        myflock.l_whence = SEEK_SET;
        iwhat = F_SETLK;
        if (ismap(arg) && struct_to_flock(mapof(arg), &myflock))
            return 1;
        r = fcntl(fd, iwhat, &myflock);
        goto ret;
    }
    else
        return argerror(1);
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
static int sys_fileno()
{
    file      *f;

    if (typecheck("u", &f))
        return 1;
    if
    (
        f->f_type != stdio_ftype
#ifndef NOPIPES
        &&
        f->f_type != popen_ftype
#endif
    )
    {
        set_error("attempt to obtain file descriptor of non-stdio file");
        return 1;
    }
    return int_ret(fileno((FILE *)f->f_file));
}

/*
 * mkdir(pathname, int)
 *
 * Create a directory with the specified mode.  Supported on Win32 platforms.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int sys_mkdir()
{
    char        *path;

#ifdef _WIN32
    if (typecheck("s", &path))
        return 1;
    if (mkdir(path) == -1)
        return get_last_errno("mkdir", path);
#else
    long        mode = 0777;

    if (NARGS() == 1)
    {
        if (typecheck("s", &path))
            return 1;
    }
    else if (typecheck("si", &path, &mode))
        return 1;
    if (mkdir(path, mode) == -1)
        return get_last_errno("mkdir", path);
#endif
    return ret_no_decref(null);
}

/*
 * mkfifo(path, mode)
 *
 * Make a "named pipe" with the given mode (an int).
 * Not supported on Win32.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int sys_mkfifo()
{
#ifdef _WIN32 /* WINDOWS can't do mkifo() */
    return not_on_win32("mkfifo");
#else
    char        *path;
    long        mode;

    if (typecheck("si", &path, &mode))
        return 1;
    if (mkfifo(path, mode) == -1)
        return get_last_errno("mkfifo", path);
    return ret_no_decref(null);
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
static int sys_read()
{
    long        fd;
    long        len;
    str    *s;
    int         r;
    char        *msg;

    if (typecheck("ii", &fd, &len))
        return 1;
    if ((msg = (char *)ici_alloc(len+1)) == nullptr)
        return 1;
    switch (r = read(fd, msg, len))
    {
    case -1:
        ici_free(msg);
        return sys_ret(-1);

    case 0:
        ici_free(msg);
        return null_ret();
    }
    if ((s = str_alloc(r)) == nullptr)
    {
        ici_free(msg);
        return 1;
    }
    memcpy(s->s_chars, msg, r);
    s = stringof(atom(s, 1));
    ici_free(msg);
    return ret_with_decref(s);
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
static int sys_write()
{
    long        fd;
    object      *o;
    char        *addr;
    int64_t     sz;
    int         havesz = 0;

    if (typecheck("io", &fd, &o))
    {
        if (typecheck("ioi", &fd, &o, &sz))
            return 1;
        havesz = 1;
    }
    if (isstring(o))
    {
        addr = (char *)stringof(o)->s_chars;
        if (!havesz || size_t(sz) > stringof(o)->s_nchars)
            sz = stringof(o)->s_nchars;
    }
    else if (ismem(o))
    {
        addr = (char *)memof(o)->m_base;
        if (!havesz || (size_t)sz > memof(o)->m_length * memof(o)->m_accessz)
            sz = memof(o)->m_length * memof(o)->m_accessz;
    }
    else
    {
        return argerror(1);
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
static int sys_symlink()
{
#ifdef _WIN32 /* WINDOWS can't do symlink() */
    return not_on_win32("symlink");
#else
    char        *a, *b;

    if (typecheck("ss", &a, &b))
        return 1;
    if (symlink(a, b) == -1)
        return get_last_errno("symlink", a);
    return ret_no_decref(null);
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
static int sys_readlink()
{
#ifdef _WIN32 /* WINDOWS can't do readlink(). */
    return not_on_win32("fdopen");
#else
    char        *path;
    char        pbuf[MAXPATHLEN+1];

    if (typecheck("s", &path))
        return 1;
    if (readlink(path, pbuf, sizeof pbuf) == -1)
        return get_last_errno("readlink", path);
    return ret_with_decref(new_str_nul_term(pbuf));
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
static int sys_stat()
{
    object    *o;
    struct stat statb;
    int         rc;
    map  *s;

    if (NARGS() != 1)
        return argcount(1);
    o = ARG(0);
    if (isint(o))
        rc = fstat(intof(o)->i_value, &statb);
    else if (isstring(o))
        rc = stat(stringof(o)->s_chars, &statb);
    else if (isfile(o) && fileof(o)->f_type == stdio_ftype)
        rc = fstat(fileof(o)->fileno(), &statb);
    else
        return argerror(0);
    if (rc == -1)
        return sys_ret(rc);
    if ((s = new_map()) == nullptr)
        return 1;
#define SETFIELD(x)                                     \
    if ((o = new_int(statb.st_ ##x)) == nullptr)       \
        goto fail;                                      \
    else if (ici_assign(s, SS(x), o))                  \
    {                                                   \
        decref(o);                                      \
        goto fail;                                      \
    }                                                   \
    else                                                \
        decref(o)

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

    return ret_with_decref(s);

 fail:
    decref(s);
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
static int sys_lstat()
{
    object    *o;
    struct stat statb;
    int         rc;
    map  *s;

    if (NARGS() != 1)
        return argcount(1);
    o = ARG(0);
    if (isstring(o))
        rc = lstat(stringof(o)->s_chars, &statb);
    else
        return argerror(0);
    if (rc == -1)
        return sys_ret(rc);
    if ((s = new_map()) == nullptr)
        return 1;
#define SETFIELD(x)                                     \
    if ((o = new_int(statb.st_ ##x)) == nullptr)       \
        goto fail;                                      \
    else if (ici_assign(s, SS(x), o))                  \
    {                                                   \
        decref(o);                                      \
        goto fail;                                      \
    }                                                   \
    else                                                \
        decref(o)

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

    return ret_with_decref(s);

 fail:
    decref(s);
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
static int sys_ctime()
{
    time_t  timev;
    str    *s;

    if (typecheck("i", &timev) || (s = new_str_nul_term(ctime(&timev))) == nullptr)
        return 1;
    return ret_with_decref(s);
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
static int sys_time()
{
    return int_ret(time(nullptr));
}

#ifndef _WIN32

static int
assign_timeval(map *s, str *k, struct timeval *tv)
{
    map    *ss;
    integer       *i;

    if (k == nullptr)
        ss = s;
    else if ((ss = new_map()) == nullptr)
        return 1;
    if ((i = new_int(tv->tv_usec)) == nullptr)
        goto fail;
    if (ici_assign(ss, SS(usec), i))
    {
        decref(i);
        goto fail;
    }
    decref(i);
    if ((i = new_int(tv->tv_sec)) == nullptr)
        goto fail;
    if (ici_assign(ss, SS(sec), i))
    {
        decref(i);
        goto fail;
    }
    decref(i);
    if (k != nullptr && ici_assign(s, k, ss))
        goto fail;
    return 0;

 fail:
    if (k != nullptr)
        decref(ss);
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
static int sys_getitimer()
{
    long                which = ITIMER_VIRTUAL;
    map          *s;
    struct itimerval    value;
    object              *o;

    if (NARGS() != 0)
    {
        if (typecheck("o", &o))
            return 1;
        if (!isstring(o))
            return argerror(0);
        if (o == SS(real))
            which = ITIMER_REAL;
        else if (o == SS(virtual))
            which = ITIMER_VIRTUAL;
        else if (o == SS(prof))
            which = ITIMER_PROF;
        else
            return argerror(0);
    }
    if (getitimer(which, &value) == -1)
        return sys_ret(-1);
    if ((s = new_map()) == nullptr)
        return 1;
    if
    (
        assign_timeval(s, SS(interval), &value.it_interval)
        ||
        assign_timeval(s, SS(value), &value.it_value)
    )
    {
        decref(s);
        return 1;
    }
    return ret_with_decref(s);
}

static int
fetch_timeval(object *s, struct timeval *tv)
{
    object    *o;

    if (!ismap(s))
        return 1;
    if ((o = ici_fetch(s, SS(usec))) == null)
        tv->tv_usec = 0;
    else if (isint(o))
        tv->tv_usec = intof(o)->i_value;
    else
        return 1;
    if ((o = ici_fetch(s, SS(sec))) == null)
        tv->tv_sec = 0;
    else if (isint(o))
        tv->tv_sec = intof(o)->i_value;
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
static int sys_setitimer()
{
    long                which = ITIMER_VIRTUAL;
    map          *s;
    struct itimerval    value;
    struct itimerval    ovalue;
    object              *o;

    if (NARGS() == 1)
    {
        if (typecheck("d", &s))
            return 1;
    }
    else
    {
        if (typecheck("od", &o, &s))
            return 1;
        if (o == SS(real))
            which = ITIMER_REAL;
        else if (o == SS(virtual))
            which = ITIMER_VIRTUAL;
        else if (o == SS(prof))
            which = ITIMER_PROF;
        else
            return argerror(0);
    }
    if ((o = ici_fetch(s, SS(interval))) == null)
        value.it_interval.tv_sec = value.it_interval.tv_usec = 0;
    else if (fetch_timeval(o, &value.it_interval))
        goto invalid_itimerval;
    if ((o = ici_fetch(s, SS(value))) == null)
        value.it_value.tv_sec = value.it_value.tv_usec = 0;
    else if (fetch_timeval(o, &value.it_value))
        goto invalid_itimerval;
    if (setitimer(which, &value, &ovalue) == -1)
        return sys_ret(-1);
    if ((s = new_map()) == nullptr)
        return 1;
    if
    (
        assign_timeval(s, SS(interval), &ovalue.it_interval)
        ||
        assign_timeval(s, SS(value), &ovalue.it_value)
    )
    {
        decref(s);
        return 1;
    }
    return ret_with_decref(s);

 invalid_itimerval:
    set_error("invalid itimer struct");
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
static int sys_gettimeofday()
{
    map          *s;
    struct timeval      tv;

    if (gettimeofday(&tv, nullptr) == -1)
        return sys_ret(-1);
    if ((s = new_map()) == nullptr)
        return 1;
    if (assign_timeval(s, nullptr, &tv))
    {
        decref(s);
        return 1;
    }
    return ret_with_decref(s);
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
static int sys_access()
{
    char        *fname;
    int         bits = F_OK;

    switch (NARGS())
    {
    case 1:
        if (typecheck("s", &fname))
            return 1;
        break;
    case 2:
        if (typecheck("si", &fname, &bits))
            return 1;
        break;
    default:
        return argcount(2);
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
static int sys_pipe()
{
#ifdef _WIN32
    return not_on_win32("pipe");
#else
    int         pfd[2];
    array     *a;
    integer   *fd;

    if ((a = new_array(2)) == nullptr)
        return 1;
    if (pipe(pfd) == -1)
    {
        decref(a);
        return sys_ret(-1);
    }
    if ((fd = new_int(pfd[0])) == nullptr)
        goto fail;
    a->push(fd, with_decref);
    if ((fd = new_int(pfd[1])) == nullptr)
        goto fail;
    a->push(fd, with_decref);
    return ret_with_decref(a);

 fail:
    decref(a);
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
static int sys_creat()
{
    char        *fname;
    long        perms;
    int         fd;

    if (typecheck("si", &fname, &perms))
        return 1;
    if ((fd = creat(fname, perms)) == -1)
        return get_last_errno("creat", fname);
    return int_ret(fd);
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
static int sys_dup()
{
    long        fd1;
    long        fd2;

    switch (NARGS())
    {
    case 1:
        if (typecheck("i", &fd1))
            return 1;
        fd2 = -1;
        break;
    case 2:
        if (typecheck("ii", &fd1, &fd2))
            return 1;
        break;
    default:
        return argcount(2);
    }
    if (fd2 == -1)
    {
        fd2 = dup(fd1);
        return fd2 < 0 ? sys_ret(fd2) : int_ret(fd2);
    }
    if (dup2(fd1, fd2) < 0)
        return sys_ret(-1);
    return int_ret(fd2);
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
static int sys_exec()
{
    char        *sargv[16];
    char        **argv;
    int         maxargv;
    int         n;
    object    **o;
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
                if ((newp = (char **)ici_alloc(maxargv * sizeof (char *))) == nullptr) \
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

    if ((n = NARGS()) < 2)
        return argcount(2);
    if (!isstring(*(o = ARGS())))
        return argerror(0);
    path = stringof(*o)->s_chars;
    --o;
    argc = 0;
    argv = sargv;
    maxargv = 16;
    if (n > 2 || isstring(*o))
    {
        while (--n > 0)
        {
            ADDARG(stringof(*o)->s_chars);
            --o;
        }
    }
    else if (!isarray(*o))
        return argerror(0);
    else
    {
        object **p;
        array *a;

        a = arrayof(*o);
        for (p = a->astart(); p < a->alimit(); p = a->anext(p))
            if (isstring(*p))
                ADDARG(stringof(*p)->s_chars);
    }
    ADDARG(nullptr);
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
static int sys_spawn()
{
    char        *sargv[16];
    char        **argv;
    int         maxargv;
    int         n;
    object    **o;
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
                if ((newp = ici_alloc(maxargv * sizeof (char *))) == nullptr) \
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

    if ((n = NARGS()) < 2)
        return argcount(2);
    o = ARGS();
    if (isint(*o))
    {
        mode = intof(*o)->i_value;
        --o;
        if (--n < 2)
            return argcount(2);
        if (!isstring(*o))
            return argerror(1);
    }
    else if (!isstring(*o))
        return argerror(0);
    path = stringof(*o)->s_chars;
    --o;
    argc = 0;
    argv = sargv;
    maxargv = 16;
    if (n > 2 || isstring(*o))
    {
        while (--n > 0)
        {
            ADDARG(stringof(*o)->s_chars);
            --o;
        }
    }
    else if (!isarray(*o))
        return argerror(0);
    else
    {
        object **p;
        array *a;

        a = arrayof(*o);
        for (p = a->astart(); p < a->alimit(); p = a->anext(p))
            if (isstring(*p))
                ADDARG(stringof(*p)->s_chars);
    }
    ADDARG(nullptr);
    n = (*(int (*)())ICI_CF_ARG1())(mode, path, argv);
    if (argv != sargv)
        ici_free(argv);
    if (n == -1)
        return sys_ret(n);
    return int_ret(n);
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
static int sys_lseek()
{
    long        fd;
    long        ofs;
    long        whence = SEEK_SET;

    switch (NARGS())
    {
    case 2:
        if (typecheck("ii", &fd, &ofs))
            return 1;
        break;
    case 3:
        if (typecheck("iii", &fd, &ofs, &whence))
            return 1;
        break;

    default:
        return argcount(3);
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
static int sys_wait()
{
#ifdef _WIN32
    return not_on_win32("wait");
#else
    int                 pid;
    map          *s;
    integer             *i;
    int                 status;

    if ((pid = wait(&status)) < 0)
        return sys_ret(-1);
    if ((s = new_map()) == nullptr)
        return 1;
    if ((i = new_int(pid)) == nullptr)
        goto fail;
    if (ici_assign(s, SS(pid), i))
    {
        decref(i);
        goto fail;
    }
    decref(i);
    if ((i = new_int(status)) == nullptr)
        goto fail;
    if (ici_assign(s, SS(status), i))
    {
        decref(i);
        goto fail;
    }
    decref(i);
    return ret_with_decref(s);

 fail:
    decref(s);
    return 1;
#endif /* _WIN32 */
}

#ifndef _WIN32

static map *password_map(struct passwd *);

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
static int sys_passwd()
{
    struct passwd     *pwent;
    array             *a;

    switch (NARGS())
    {
    case 0:
        break;

    case 1:
        if (isint(ARG(0)))
            pwent = getpwuid((uid_t)intof(ARG(0))->i_value);
        else if (isstring(ARG(0)))
            pwent = getpwnam(stringof(ARG(0))->s_chars);
        else
            return argerror(0);
        if (pwent == nullptr)
            set_error("no such user");
        return ret_with_decref(password_map(pwent));

    default:
        return argcount(1);
    }

    if ((a = new_array()) == nullptr)
        return 1;
    setpwent();
    while ((pwent = getpwent()) != nullptr) {
        map *m;
        if ((m = password_map(pwent)) == nullptr) {
            decref(a);
            return 1;
        }
        if (a->push_checked(m, with_decref)) {
            decref(a);
            return 1;
        }
    }
    endpwent();
    return ret_with_decref(a);
}

static map *
password_map(struct passwd *pwent)
{
    map *d;
    object     *o;

    if (pwent == nullptr)
        return nullptr;
    if ((d = new_map()) != nullptr)
    {

#define SET_INT_FIELD(x)                                \
        if ((o = new_int(pwent->pw_ ##x)) == nullptr)  \
            goto fail;                                  \
        else if (ici_assign(d, SS(x), o))              \
        {                                               \
            decref(o);                                  \
            goto fail;                                  \
        }                                               \
        else                                            \
            decref(o)

#define SET_STR_FIELD(x)                                        \
        if ((o = new_str_nul_term(pwent->pw_ ##x)) == nullptr) \
            goto fail;                                          \
        else if (ici_assign(d, SS(x), o))                       \
        {                                                       \
            decref(o);                                          \
            goto fail;                                          \
        }                                                       \
        else                                                    \
            decref(o)

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
    decref(d);
    return nullptr;
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
static int sys_getpass()
{
    const char *prompt = "Password: ";

    if (NARGS() > 0)
    {
        if (typecheck("s", &prompt))
            return 1;
    }
    return str_ret(getpass(prompt));
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
static int sys_setpgrp()
{
#ifdef SETPGRP_2_ARGS
    long        pid, pgrp;
#else
    long        pgrp;
#endif

#ifdef SETPGRP_2_ARGS
    switch (NARGS())
    {
    case 1:
        pid = 0;
#endif
        if (typecheck("i", &pgrp))
            return 1;
#ifdef SETPGRP_2_ARGS
        break;

    default:
        if (typecheck("ii", &pid, &pgrp))
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
static int sys_flock()
{
    long        fd, operation;

    if (typecheck("ii", &fd, &operation))
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
static int sys_truncate()
{
    long        fd;
    long        len;
    char        *s;

    if (typecheck("ii", &fd, &len) == 0)
    {
        if (ftruncate(fd, len) == -1)
            return sys_ret(-1L);
        return null_ret();
    }
    if (typecheck("si", &s, &len) == 0)
    {
        if (truncate(s, len) == -1)
            return get_last_errno("truncate", s);
        return null_ret();
    }
    return 1;
}

#ifndef _WIN32

static int
string_to_resource(object *what)
{
    if (what == SS(core))
        return RLIMIT_CORE;
    if (what == SS(cpu))
        return RLIMIT_CPU;
    if (what == SS(data))
        return RLIMIT_DATA;
    if (what == SS(fsize))
        return RLIMIT_FSIZE;
#if defined(RLIMIT_MEMLOCK)
    if (what == SS(memlock))
        return RLIMIT_MEMLOCK;
#endif
    if (what == SS(nofile))
        return RLIMIT_NOFILE;
#if defined(RLIMIT_NPROC)
    if (what == SS(nproc))
        return RLIMIT_NPROC;
#endif
#if defined(RLIMIT_RSS)
    if (what == SS(rss))
        return RLIMIT_RSS;
#endif
    if (what == SS(stack))
        return RLIMIT_STACK;

#if !(defined __MACH__ && defined __APPLE__)
#define NO_RLIMIT_DBSIZE
#endif
#if __FreeBSD__ < 4
#define NO_RLIMIT_SBSIZE
#endif

#ifndef NO_RLIMIT_SBSIZE
    if (what == SS(sbsize))
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
static int sys_getrlimit()
{
    object        *what;
    int            resource;
    struct rlimit  rlimit;
    map           *limit;
    integer       *iv;

    if (typecheck("o", &what))
        return 1;
    if (isint(what))
        resource = intof(what)->i_value;
    else if (isstring(what))
    {
        if ((resource = string_to_resource(what)) == -1)
            return argerror(0);
    }
    else
        return argerror(0);

    if (getrlimit(resource, &rlimit) < 0)
        return sys_ret(-1);

    if ((limit = new_map()) == nullptr)
        return 1;
    if ((iv = new_int(rlimit.rlim_cur)) == nullptr)
        goto fail;
    if (ici_assign(limit, SS(cur), iv))
    {
        decref(iv);
        goto fail;
    }
    decref(iv);
    if ((iv = new_int(rlimit.rlim_max)) == nullptr)
        goto fail;
    if (ici_assign(limit, SS(max), iv))
    {
        decref(iv);
        goto fail;
    }
    decref(iv);
    return ret_with_decref(limit);

 fail:
    decref(limit);
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
static int sys_setrlimit()
{
    object            *what;
    object            *value;
    struct rlimit       rlimit;
    int                 resource;
    object            *iv;

    if (typecheck("oo", &what, &value))
        return 1;
    if (isint(what))
        resource = intof(what)->i_value;
    else if (isstring(what))
    {
        if ((resource = string_to_resource(what)) == -1)
            return argerror(0);
    }
    else
        return argerror(0);

    if (isint(value))
    {
        if (getrlimit(resource, &rlimit) < 0)
            return sys_ret(-1);
        rlimit.rlim_cur = intof(value)->i_value;
    }
    else if (value == SS(infinity))
    {
        rlimit.rlim_cur = RLIM_INFINITY;
    }
    else if (ismap(value))
    {
        if ((iv = ici_fetch(value, SS(cur))) == null)
            goto fail;
        if (!isint(iv))
            goto fail;
        rlimit.rlim_cur = intof(iv)->i_value;
        if ((iv = ici_fetch(value, SS(max))) == null)
            goto fail;
        if (!isint(iv))
            goto fail;
        rlimit.rlim_max = intof(iv)->i_value;
    }
    else
        return argerror(1);

    return sys_ret(setrlimit(resource, &rlimit));

 fail:
    set_error("invalid rlimit struct");
    return 1;
}

/*
 * usleep(int)
 *
 * Suspend the process for the specified number of milliseconds.
 *
 * This --topic-- forms part of the --ici-sys-- documentation.
 */
static int sys_usleep()
{
    long t;
    exec *x;

    if (typecheck("i", &t))
        return 1;
    blocking_syscall(1);
    x = leave();
    usleep(t);
    enter(x);
    blocking_syscall(0);
    return null_ret();
}

#endif /* #ifndef _WIN32 */

ICI_DEFINE_CFUNCS(sys) {
    /* utime */
    ICI_DEFINE_CFUNC(access,  sys_access),
    ICI_DEFINE_CFUNC(_close,   sys_close),
    ICI_DEFINE_CFUNC(creat,   sys_creat),
    ICI_DEFINE_CFUNC(ctime,   sys_ctime),
    ICI_DEFINE_CFUNC(dup,     sys_dup),
    ICI_DEFINE_CFUNC1(exec,   sys_exec, execv),
    ICI_DEFINE_CFUNC1(execp,  sys_exec, execvp),
    /*
     * _exit(int)
     *
     * Exit the current process returning an integer exit status to the
     * parent.  Supported on Win32 platforms.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(_exit,   sys_simple, _exit,  "i"),
    ICI_DEFINE_CFUNC(fcntl,   sys_fcntl),
    ICI_DEFINE_CFUNC(fdopen,  sys_fdopen),
    ICI_DEFINE_CFUNC(fileno,  sys_fileno),
    ICI_DEFINE_CFUNC(lseek,   sys_lseek),
    ICI_DEFINE_CFUNC(mkdir,   sys_mkdir),
    ICI_DEFINE_CFUNC(mkfifo,  sys_mkfifo),
    ICI_DEFINE_CFUNC(open,    sys_open),
    ICI_DEFINE_CFUNC(pipe,    sys_pipe),
    ICI_DEFINE_CFUNC(read,    sys_read),
    ICI_DEFINE_CFUNC(readlink,sys_readlink),
    /*
     * rmdir(pathname)
     *
     * Remove a directory. Supported on Win32 platforms.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(rmdir,  sys_simple, rmdir,  "s"),
    ICI_DEFINE_CFUNC(stat,    sys_stat),
    ICI_DEFINE_CFUNC(symlink, sys_symlink),
    ICI_DEFINE_CFUNC(time,    sys_time),
    /*
     * unlink(pathname)
     *
     * Unlink a file. Supported on WIN32 platforms.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    /* should go as remove(}, is more portable */
    ICI_DEFINE_CFUNC2(unlink, sys_simple, unlink, "s"),
    ICI_DEFINE_CFUNC(wait,    sys_wait),
    ICI_DEFINE_CFUNC(write,   sys_write),
#ifdef _WIN32
    ICI_DEFINE_CFUNC1(spawn,  sys_spawn, spawnv),
    ICI_DEFINE_CFUNC1(spawnp, sys_spawn, spawnvp),
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
    ICI_DEFINE_CFUNC2(alarm,   sys_simple, alarm,  "i"),
    /*
     * chmod(pathname, int)
     *
     * Change the mode of a file system object. Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(chmod,   sys_simple, chmod,  "si"),
    /*
     * chown(pathname, uid, gid)
     *
     * Use 'chown(2)' to change the ownership of a file to new integer
     * user and group indentifies. Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(chown,   sys_simple, chown,  "sii"),
    /*
     * chroot(pathname)
     *
     * Change root directory for process. Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(chroot,  sys_simple, chroot, "s"),
    /*
     * int = clock()
     *
     * Return the value of 'clock(2)'.  Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(clock,   sys_simple, clock,  ""),
    /*
     * int = fork()
     *
     * Create a new process.  In the parent this returns the process
     * identifier for the newly created process.  In the newly created
     * process it returns zero. Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(fork,    sys_simple, fork,   ""),
    /*
     * int = getegid()
     *
     * Get the effective group identifier of the owner of the current process.
     * Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(getegid, sys_simple, getegid,""),
    /*
     * int = geteuid()
     *
     * Get the effective user identifier of the owner of the current process.
     * Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(geteuid, sys_simple, geteuid,""),
    /*
     * int = getgid()
     *
     * Get the real group identifier of the owner of the current process.
     * Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(getgid,  sys_simple, getgid, ""),
    ICI_DEFINE_CFUNC(getitimer,sys_getitimer),
    ICI_DEFINE_CFUNC(getpass, sys_getpass),
    /*
     * int = getpgrp()
     *
     * Get the current process group identifier.  Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(getpgrp, sys_simple, getpgrp,""),
    /*
     * int = getpid()
     *
     * Get the process identifier for the current process.  Not supported
     * on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(getpid,  sys_simple, getpid, ""),
    /*
     * int = getppid()
     *
     * Get the process identifier for the parent process.  Not supported
     * on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(getppid, sys_simple, getppid,""),
    ICI_DEFINE_CFUNC(getrlimit,sys_getrlimit),
    ICI_DEFINE_CFUNC(gettimeofday,sys_gettimeofday),
    /*
     * int = getuid()
     *
     * Get the real user identifier of the owner of the current process.
     * Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(getuid,  sys_simple, getuid, ""),
    /*
     * int = isatty(fd)
     *
     * Returns 1 if the int 'fd' is the open file descriptor of a "tty".
     * Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(isatty,  sys_simple, isatty, "i"),
    /*
     * kill(int, int)
     *
     * Post the signal specified by the second argument to the process
     * with process ID given by the first argument.  Not supported on
     * Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(kill,    sys_simple, kill,   "ii"),
    /*
     * link(oldpath, newpath)
     *
     * Create a link to an existing file.  Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(link,    sys_simple, link,   "ss"),
    ICI_DEFINE_CFUNC(lstat,   sys_lstat),
    /*
     * mknod(pathname, int, int)
     *
     * Create a special file with mode given by the second argument and
     * device type given by the third.  Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(mknod,   sys_simple, mknod,  "sii"),
    /*
     * nice(int)
     *
     * Change the nice value of a process.
     * Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(nice,    sys_simple, nice,   "i"),
    ICI_DEFINE_CFUNC(passwd,  sys_passwd),
    /*
     * pause()
     *
     * Wait until a signal is delivered to the process.
     * Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(pause,   sys_simple, pause,  ""),
    /*
     * setgid(int)
     *
     * Set the real and effective group identifier for the current process.
     * Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(setgid,  sys_simple, setgid, "i"),
    ICI_DEFINE_CFUNC(setitimer,sys_setitimer),
    ICI_DEFINE_CFUNC(setpgrp, sys_setpgrp),
    ICI_DEFINE_CFUNC(setrlimit,sys_setrlimit),
    /*
     * setuid(int)
     *
     * Set the real and effective user identifier for the current process.
     * Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(setuid,  sys_simple, setuid, "i"),
    /*
     * sync()
     *
     * Schedule in-memory file data to be written to disk.
     * Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(sync,    sys_simple, sync,   ""),
#endif
    ICI_DEFINE_CFUNC(truncate,sys_truncate),
#ifndef _WIN32
    /*
     * umask(int)
     *
     * Set file creation mask.
     * Not supported on Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(umask,   sys_simple, umask,  "i"),
    ICI_DEFINE_CFUNC(usleep,  sys_usleep),
#ifndef NO_ACCT
    /*
     * acct(pathname)
     *
     * Enable accounting on the specified file.  Not suppored on
     * cygwin or Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(acct,    sys_simple, acct,   "s"),
#endif
#ifndef ICI_SYS_NOFLOCK
    ICI_DEFINE_CFUNC(flock,   sys_flock),
#endif
#if !defined(__linux__) && !defined(BSD) && !defined(__CYGWIN__)
    /*
     * int = lockf(fd, cmd, len)
     *
     * Invoked 'lockf(3)' on a file.
     *
     * Not supported on Linux, Cygwin, Win32.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(lockf,   sys_simple, lockf,  "iii"),
#endif /* __linux__ */
#if !defined(__FreeBSD__) && !defined(__CYGWIN__) && !defined(__APPLE__)
    /*
     * ulimit(int, int)
     *
     * Get and set user limits.
     * Not supported on Win32, NeXT, some BSD, or Cygwin.
     *
     * This --topic-- forms part of the --ici-sys-- documentation.
     */
    ICI_DEFINE_CFUNC2(ulimit,  sys_simple, ulimit, "ii"),
#endif
#endif
    ICI_CFUNCS_END()
};

/*
 * Create pre-defined variables to replace C's #define's.
 */
int sys_init(ici::objwsup *scp) {
#define VALOF(x) { #x , x }
    static struct { const char *name; long val; } var[] = {
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

    ref<objwsup> mod = new_module(ici_sys_cfuncs);
    if (!mod) {
        return 1;
    }
    for (size_t i = 0u; i < sizeof var/sizeof var[0]; ++i) {
        if (cmkvar(mod, var[i].name, 'i', &var[i].val)) {
            return 1;
        }
    }
    return ici_assign(scp, SS(sys), mod);
}

} // namespace ici
