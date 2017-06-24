// -*- mode:c++ -*-

#ifndef ICI_FTYPE_H
#define ICI_FTYPE_H

#include "fwd.h"

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */


/*
 * The flags that may appear in flags.  NOTE: If any flag greater than 0x80
 * is specified, file creation with ici_file_new() will fail.  See that
 * function for details.
 *
 * FT_NOMUTEX           ICI will surround file I/O on this file with
 *                      leave()/enter().  Using this flag can increase
 *                      efficiency by allowing other threads to run during a
 *                      blocking I/O operation; however it must NOT be used if
 *                      the file's I/O functions can access ICI data.
 *
 * --ici-api-- continued.
 */
constexpr int FT_NOMUTEX = 0x01;

/*
 * A set of functions for file access.  ICI file objects are implemented
 * on top of this simple file abstraction in order to allow several
 * different types of file-like entities.  Each different type of file
 * uses one of these structures with specific functions.  Each function is
 * assumed to be compatible with the stdio function of the same name.  In the
 * case were the file is a stdio stream, these *are* the stdio functions.
 *
 * See also: 'stdio_ftype', 'popen_ftype', 'parse_ftype' et al.
 *
 * This --struct-- forms part of the --ici-api--.
 */
class ftype
{
protected:
    ftype(int flags = 0) : flags(flags) {}
public:
    virtual ~ftype() {}
    virtual int getch(void *);
    virtual int ungetch(int, void *);
    virtual int flush(void *);
    virtual int close(void *);
    virtual long seek(void *, long, int);
    virtual int eof(void *);
    virtual int write(const void *, long, void *);
    virtual int fileno(void *);
    virtual int setvbuf(void *, char *, int, size_t);

    int flags;
};
/*
 * flags             A combination of * flags, defined below.
 */

class stdio_ftype : public ftype
{
public:
    stdio_ftype();
    virtual int getch(void *) override;
    virtual int ungetch(int, void *) override;
    virtual int flush(void *) override;
    virtual int close(void *) override;
    virtual long seek(void *, long, int) override;
    virtual int eof(void *) override;
    virtual int write(const void *, long, void *) override;
    virtual int fileno(void *) override;
    virtual int setvbuf(void *, char *, int, size_t) override;
};

/*
 * The ftype for pipes is just a stdio ftype that uses pclose()
 * for its close implementation and checks the returned exit status.
 */
class popen_ftype : public stdio_ftype
{
public:
    virtual int close(void *) override;
};

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif
