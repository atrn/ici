/*
 *  This is an example of a dynamically loaded module for ICI.
 *
 *  ICI supports automatic loading of modules when resolving
 *  variable names. If a variable is not found an attempt is
 *  made to load a module with that name. This allows using
 *  modules located in a number of known locations without
 *  any prior declaration.
 *
 *  This module, called "example", defines two functions, "function"
 *  and "other_function". Both take a single string argument and
 *  returns its length as an integer.
 *
 *  Using ICI's automatic loading this module is used like so,
 *
 *      var len = example.function("213");
 *      if (len != example.other_function("213")) ...
 *
 */

#include <ici.h>
#include "icistr.h"
#include <icistr-setup.h>

#include <string.h> /* strlen */

/*
 *   Modules only need to export a single function, their 'init'
 *  function (see below) and to isolate themselves from other modules
 *  and speed their loading should enclose their code in an anonymous
 *  namespace.
 */
namespace
{

int f_function()
{
    /*
     *  The ici::typecheck() function is used to check the types of
     *  the actual parameters and obtain their values. It works in
     *  a similar manner to C's scanf() using a format-string to
     *  define the types and some number of pointers to locations
     *  where the corresponding values are stored.
     *
     *  As per ICI convention ici::typecheck() returns non-zero
     *  if it fails and has set the ICI error to indicate why.
     *
     *  We use the 's' format to get a string argument. The 's'
     *  format expects an ICI string object as the actual parameter
     *  and returns its value by setting a 'const char *' to point
     *  to the string's value (held within the ICI string object).
     */
    const char *s;
    if (ici::typecheck("s", &s))
        return 1;

    return ici::int_ret(strlen(s));
}

int f_other_function()
{
    /*
     *  ICI string objects already know their length so we can use
     *  that knowledge to avoid calling strlen() and also work with
     *  strings that contain embedded NULs.
     *
     *  To get the string object typecheck's 'q' format. This expects
     *  the actual argument be a string and returns the corresponding
     *  ici::str object which we use directly.
     *
     */
    ici::str *arg;
    if (ici::typecheck("q", &arg))
        return 1;
    return ici::int_ret(arg->s_nchars);
}

} // anon


/*
 *  Each module has a single entry point, its 'init' function.
 *
 *  Module init functions must be declared 'extern "C"' to
 *  disable name mangling.
 *
 *  The name of the module's init function is well defined and
 *  is formed from the module name by prefixing it with "ici_"
 *  and appending an "_init" suffix. E.g., this "example" module
 *  has an init function called "ici_example_init".
 */
extern "C" ici::object *ici_example_init()
{
    /*
     *  First thing a module should do is check if it is compatible
     *  with this version of the interpreter.  The check_interface
     *  function determines if the version numbers compiled into the
     *  module are compatible with those compiled into the
     *  interpreter's build.  If not it returns non-zero and has set
     *  the ICI error to describe the issue.
     *
     */
    if (ici::check_interface(ici::version_number, ici::back_compat_version, "example"))
        return nullptr;

    /*
     *  The icistr-setup.h header defines a function 'init_ici_str' that
     *  is used to initialize all of the strings defined in the 'icistr.h'
     *  file.  It needs to be called and, as per ICI convention, returns
     *  non-zero if it fails.
     *
     */
    if (init_ici_str())
        return nullptr;

    /*
     *  Finally we can create our module using the 'new_module'
     *  function passing a table with our entry points.
     *
     */

    ICI_DEFINE_CFUNCS(test)
    {
        ICI_DEFINE_CFUNC(function, f_function),
        ICI_DEFINE_CFUNC(other_function, f_other_function),
        ICI_CFUNCS_END()
    };

    return ici::new_module(ICI_CFUNCS(test));
}
