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
 * The flags that may appear in ft_flags.  NOTE: If any flag greater than 0x80
 * is specified, file creation with ici_file_new() will fail.  See that
 * function for details.
 *
 * FT_NOMUTEX           ICI will surround file I/O on this file with
 *                      ici_leave()/ici_enter().  Using this flag can increase
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
 * See also: 'ici_stdio_ftype', 'ici_popen_ftype'.
 *
 * This --struct-- forms part of the --ici-api--.
 */
class ftype
{
protected:
    ftype(int flags = 0) : ft_flags(flags) {}
public:
    virtual ~ftype() {}
    virtual int ft_getch(void *);
    virtual int ft_ungetch(int, void *);
    virtual int ft_flush(void *);
    virtual int ft_close(void *);
    virtual long ft_seek(void *, long, int);
    virtual int ft_eof(void *);
    virtual int ft_write(const void *, long, void *);
    virtual int ft_fileno(void *);
    virtual int ft_setvbuf(void *, char *, int, size_t);

    int ft_flags;
};
/*
 * ft_flags             A combination of FT_* flags, defined below.
 */

class stdio_ftype : public ftype
{
public:
    stdio_ftype();
    virtual int ft_getch(void *) override;
    virtual int ft_ungetch(int, void *) override;
    virtual int ft_flush(void *) override;
    virtual int ft_close(void *) override;
    virtual long ft_seek(void *, long, int) override;
    virtual int ft_eof(void *) override;
    virtual int ft_write(const void *, long, void *) override;
    virtual int ft_fileno(void *) override;
    virtual int ft_setvbuf(void *, char *, int, size_t) override;
};

/**
 */
class popen_ftype : public stdio_ftype
{
public:
    virtual int ft_close(void *) override;
};

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif
