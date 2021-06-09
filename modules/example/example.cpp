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

#include <cstring> /* strlen */

/*
 *  Modules only need to export a single name, their 'init' function
 *  (see below), so to isolate themselves from other modules and speed
 *  loading they should enclose their code in an anonymous namespace.
 *
 */
namespace
{

/*
 *  The icistr.h/icistr-setup.h "dance" can be done inside the
 *  anonymous namespace and stops our strings polluting the global
 *  namespace.  Larger modules may wish to define their own namespace
 *  and put their definitions inside that.
 */

#include "icistr.h"
#include <icistr-setup.h>

int f_function()
{
    /*
     *  The ici::typecheck() function is used to check the types of
     *  the actual parameters and obtain their values. It works in a
     *  similar manner to C's scanf() using a format-string to define
     *  the argument types and of pointers to locations where the
     *  corresponding values are stored.
     *
     *  As per ICI convention ici::typecheck() returns non-zero if it
     *  fails and has set the ICI error to indicate why.
     *
     *  We use the 's' format to get a string argument. The 's' format
     *  expects an ICI string object as the actual parameter and
     *  returns its value by setting a 'const char *' to point into
     *  the string's value held within the ICI string object.  This is
     *  safe for the duration of the call to the function as the
     *  argument is referenced via the stack frame and will not be
     *  collected if garbage collection occurs.
     */
    const char *s;
    if (ici::typecheck("s", &s))
        return 1;

    /*
     *  Call strlen() to get the length, our result.
     */
    int len = strlen(s);

    /*
     *  And finally call ici::int_ret to return an ICI integer value
     *  to our caller.  There are various other "ret" functions for
     *  different types, see ici.h for more information.
     *
     *  The "ret" functions push the ICI object onto the ICI stack
     *  used by the interpreter's function calling mechanism to obtain
     *  the function's result.  Functions in ICI always return
     *  something and those without a defined result, which would be
     *  void in C/C++ (aka "procedures" in Pascal) return an ICI
     *  "NULL" (via ici::null_ret()).
     *
     *  The int being returned by this, C++, function is NOT the value
     *  of the ici integer but is an error code - non-zero meaning the
     *  function failed.
     */
    return ici::int_ret(len);
}

int f_other_function()
{
    /*
     *  ICI string objects already know their length so we can use
     *  that knowledge to avoid calling strlen(). This is not only
     *  faster (O(1) vs O(n)) but also means our function now works
     *  with strings that contain embedded NULs.
     *
     *  This time, to get the actual string argument, we use
     *  typecheck's 'q' format. This expects the actual argument to be
     *  a string and returns the corresponding ici::str * which we
     *  then use to obtain the string length.
     */
    ici::str *s;
    if (ici::typecheck("q", &s))
        return 1;
    return ici::int_ret(s->s_nchars);
}

} // anon


/*
 *  Each module has a single entry point, its 'init' function.
 *
 *  A module's init function takes no arguments and returns a
 *  pointer to an ici object which represents the module.
 *
 *  Module init functions must be declared 'extern "C"' to
 *  disable name mangling so the function's symbol is easily
 *  located in the loaded code.
 *
 *  The name of the module's init function has a well defined format.
 *  It consists of the module name prefxied with "ici_" and suffixed
 *  with "_init". E.g., this "example" module has an init function
 *  called "ici_example_init".
 *
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
    {
        return nullptr;
    }

    /*
     *  The icistr-setup.h header defines a function 'init_ici_str' that
     *  is used to initialize all of the strings defined in the 'icistr.h'
     *  file.  It needs to be called and, as per ICI convention, returns
     *  non-zero if it fails.
     *
     */
    if (init_ici_str())
    {
        return nullptr;
    }

    /*
     *  Finally we can create our module using the 'new_module'
     *  function. It is passed a table that defines the module's
     *  functions.
     *
     */
    static ICI_DEFINE_CFUNCS(test)
    {
        ICI_DEFINE_CFUNC(function, f_function),
        ICI_DEFINE_CFUNC(other_function, f_other_function),
        ICI_CFUNCS_END()
    };

    return ici::new_module(ICI_CFUNCS(test));
}
