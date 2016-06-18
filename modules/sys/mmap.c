#define ICI_CORE

#include "fwd.h"
#include "str.h"
#include "mem.h"
#include "cfunc.h"

/*
 * mem = mmap(string [, string [, int [, int]]])
 *
 *      Map a file or portion of a file into memory and return a mem
 *      object referencing the file's contents.  By default the
 *      mapping is shared and writable, modifications to the data
 *      result in changes to the underlying file.
 *
 * mem = mmap(int, int, int, string)
 *      Map memory from a file descriptor.
 *
 *
 * Mapping Options
 *      Options are specified as a comma-separated list
 *      of known words.
 *
 *      read            Read access.
 *      write           Write access.
 *      exec            Code execution.
 *
 *      anon
 *
 *      private
 *      shared
 *      nocache
 */
static int
f_mmap(void)
{
    return ici_null_ret();
}

ICI_DEFINE_CFUNCS(mmap)
{
    ICI_DEFINE_CFUNC(mmap, f_mmap),
    {ICI_CF_OBJ}
};
