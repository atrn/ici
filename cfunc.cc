/*
 * cfunc.c
 *
 * Implementations of many (not all) of the core language intrinsic functions.
 * For historical reasons this is *NOT* the code associated with the cfunc
 * type. That's in cfunco.c
 *
 * This is public domain source. Please do not add any copyrighted material.
 */
#define ICI_CORE
#include "exec.h"
#include "func.h"
#include "str.h"
#include "int.h"
#include "float.h"
#include "struct.h"
#include "set.h"
#include "op.h"
#include "ptr.h"
#include "buf.h"
#include "file.h"
#include "ftype.h"
#include "re.h"
#include "null.h"
#include "parse.h"
#include "mem.h"
#include "handle.h"
#include "channel.h"
#include "pcre.h"

#include <stdio.h>
#include <limits.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>

#ifdef  _WIN32
#include <windows.h>
#endif

/*
 * For select() for waitfor().
 */
#include <sys/types.h>

#if defined(__linux__) || defined(BSD) || defined(__sun)
#include <sys/param.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <dirent.h>
#endif

#ifndef environ
    /*
     * environ is sometimes mapped to be a function, so only extern it
     * if it is not already defined.
     */
    extern char         **environ;
#endif

#undef isset

namespace ici
{

/*
 * Marshall function arguments in a call from ICI to C.  This function may
 * only be called from the implementation of an intrinsic function.
 *
 * 'types' is a character string.  Each character corresponds to an actual
 * argument in the ICI side of the call.  Each is checked according to the
 * particular letter, and possibly converted and/or assigned through a
 * corresponing pointer to a C-side data item provided in the vargars argument
 * list to this function.
 *
 * Any detected type mismatches result in a non-zero return.  If all types
 * match, all assignments will be made and zero will be returned.
 *
 * The key letters that may be used in 'types', and their meaning, are:
 *
 * o        Any ICI object is required in the ICI actuals, the corresponding
 *          vararg must be a pointer to an '(object *)'; which will be set
 *          to the actual argument.
 *
 * h        An ICI handle object.  The next available vararg must be an ICI
 *          string object.  The corresponding ICI argument must be a handle
 *          with that name.  The next (again) available vararg after that is a
 *          pointer to store the '(handle *)' through.
 *
 * p        An ICI ptr object is required in the actuals, then as for 'o'.
 *
 * d        An ICI struct object is required in the actuals, then as for 'o'.
 *
 * a        An ICI array object is required in the actuals, then as for 'o'.
 *
 * u        An ICI file object is required in the actuals, then as for 'o'.
 *
 * r        An ICI regexp object is required in the actuals, then as for 'o'.
 *
 * m        An ICI mem object is required in the actuals, then as for 'o'.
 *
 * i        An ICI int object is required in the actuals, the value of this
 *          int will be stored through the corresponding pointer which must be
 *          a '(long *)'.
 *
 * f        An ICI float object is required in the actuals, the value of this
 *          float will be stored through the corresponding pointer which must
 *          be a '(double *)'.
 *
 * n        An ICI float or int object is required in the actuals, the value
 *          of this float or int will be stored through the corresponding
 *          pointer which must be a '(double *)'.
 *
 * s        An ICI string object is required in the actuals, the corresponding
 *          pointer must be a (char **).  A pointer to the raw characters of
 *          the string will be stored through this (this will be '\0'
 *          terminated by virtue of all ICI strings having a gratuitous '\0'
 *          just past their real end).  These characters can be assumed to
 *          remain available until control is returned back to ICI because the
 *          string is still on the ICI operand stack and can't be collected.
 *          Once control has reurned to ICI, they could be collected at any
 *          time.
 *
 * -        The acutal parameter at this position is skipped, but it must be
 *          present.
 *
 * *        All remaining actual parametes are ignored (even if there aren't
 *          any).
 *
 * The capitalisation of any of the alphabetic key letters above changes
 * their meaning.  The acutal must be an ICI ptr type.  The value this
 * pointer points to is taken to be the value which the above descriptions
 * concern themselves with (i.e. in place of the raw actual parameter).
 *
 * There must be exactly as many actual arguments as key letters unless
 * the last key letter is a *.
 *
 * Error returns have the usual ICI error conventions.
 *
 * This --func-- forms part or the --ici-api--.
 */
int typecheck(const char *types, ...)
{
    va_list   va;
    object  **ap;               /* Argument pointer. */
    int       nargs;
    int       i;
    char     *aptr;              /* Subsequent things from va_alist. */
    int       tcode;
    object   *o;

    va_start(va, types);
    nargs = NARGS();
    ap = ARGS();
    for (i = 0; types[i] != '\0'; ++i, --ap)
    {
        if (types[i] == '*')
        {
            va_end(va);
            return 0;
        }

        if (i == nargs)
        {
            va_end(va);
            return argcount(strlen(types));
        }

        if ((tcode = types[i]) == '-')
            continue;

        aptr = va_arg(va, char *);
        if (tcode >= 'A' && tcode <= 'Z')
        {
            if (!isptr(*ap))
                goto fail;
            if ((o = ici_fetch(*ap, o_zero)) == NULL)
                goto fail;
            tcode += 'a' - 'A';
        }
        else
        {
            o = *ap;
        }

        switch (tcode)
        {
        case 'o': /* Any object. */
            *(object **)aptr = o;
            break;

        case 'h': /* A handle with a particular name. */
            if (!ishandleof(o, (str *)aptr))
                goto fail;
            aptr = va_arg(va, char *);
            *(handle **)aptr = handleof(o);
            break;

        case 'p': /* Any pointer. */
            if (!isptr(o))
                goto fail;
            *(ptr **)aptr = ptrof(o);
            break;

        case 'i': /* An int -> long. */
            if (!isint(o))
                goto fail;
            *(long *)aptr = intof(o)->i_value;
            break;

        case 's': /* A string -> (char *). */
            if (!isstring(o))
                goto fail;
            *(char **)aptr = stringof(o)->s_chars;
            break;

        case 'f': /* A float -> double. */
            if (!isfloat(o))
                goto fail;
            *(double *)aptr = floatof(o)->f_value;
            break;

        case 'n': /* A number, int or float -> double. */
            if (isint(o))
                *(double *)aptr = intof(o)->i_value;
            else if (isfloat(o))
                *(double *)aptr = floatof(o)->f_value;
            else
                goto fail;
            break;

        case 'd': /* A struct ("dict") -> (ici_struct *). */
            if (!isstruct(o))
                goto fail;
            *(ici_struct **)aptr = structof(o);
            break;

        case 'a': /* An array -> (array *). */
            if (!isarray(o))
                goto fail;
            *(array **)aptr = arrayof(o);
            break;

        case 'u': /* A file -> (file *). */
            if (!isfile(o))
                goto fail;
            *(file **)aptr = fileof(o);
            break;

        case 'r': /* A regular expression -> (regexpr_t *). */
            if (!isregexp(o))
                goto fail;
            *(regexp **)aptr = regexpof(o);
            break;

        case 'm': /* A mem -> (mem *). */
            if (!ismem(o))
                goto fail;
            *(mem **)aptr = memof(o);
            break;

        default:
            assert(0);
        }
    }
    va_end(va);
    if (i != nargs)
        return argcount(i);
    return 0;

fail:
    return argerror(i);
}

/*
 * Perform storage of values through pointers in the actual arguments to
 * an ICI to C function. Typically in preparation for returning values
 * back to the calling ICI code.
 *
 * 'types' is a character string consisting of key letters which
 * correspond to actual arguments of the current ICI/C function.
 * Each of the characters in the retspec has the following meaning.
 *
 * o        The actual ICI argument must be a ptr, the corresponding
 *                      pointer is assumed to be an (object **).  The
 *                      location indicated by the ptr object is updated with
 *                      the (object *).
 *
 * d
 * a
 * u    Likwise for types as per typecheck() above.
 * ...
 * -    The acutal argument is skipped.
 * *    ...
 *
 */
int retcheck(const char *types, ...)
{
    va_list   va;
    int       i;
    int       nargs;
    object  **ap;
    char     *aptr;
    int       tcode;
    object   *o;
    object   *s;

    va_start(va, types);
    nargs = NARGS();
    ap = ARGS();
    for (i = 0; types[i] != '\0'; ++i, --ap)
    {
        if ((tcode = types[i]) == '*')
        {
            va_end(va);
            return 0;
        }

        if (i == nargs)
        {
            va_end(va);
            return argcount(strlen(types));
        }

        if (tcode == '-')
            continue;

        o = *ap;
        if (!isptr(o))
            goto fail;

        aptr = va_arg(va, char *);

        switch (tcode)
        {
        case 'o': /* Any object. */
            *(object **)aptr = o;
            break;

        case 'p': /* Any pointer. */
            if (!isptr(o))
                goto fail;
            *(ptr **)aptr = ptrof(o);
            break;

        case 'i':
            if ((s = new_int(*(long *)aptr)) == NULL)
                goto ret1;
            if (ici_assign(o, o_zero, s))
                goto ret1;
            s->decref();
            break;

        case 's':
            if ((s = new_str_nul_term(*(char **)aptr)) == NULL)
                goto ret1;
            if (ici_assign(o, o_zero, s))
                goto ret1;
            s->decref();
            break;

        case 'f':
            if ((s = new_float(*(double *)aptr)) == NULL)
                goto ret1;
            if (ici_assign(o, o_zero, s))
                goto ret1;
            s->decref();
            break;

        case 'd':
            if (!isstruct(o))
                goto fail;
            *(ici_struct **)aptr = structof(o);
            break;

        case 'a':
            if (!isarray(o))
                goto fail;
            *(array **)aptr = arrayof(o);
            break;

        case 'u':
            if (!isfile(o))
                goto fail;
            *(file **)aptr = fileof(o);
            break;

        case '*':
            return 0;

        }
    }
    va_end(va);
    if (i != nargs)
        return argcount(i);
    return 0;

ret1:
    va_end(va);
    return 1;

fail:
    va_end(va);
    return argerror(i);
}

/*
 * Generate a generic error message to indicate that argument i of the current
 * intrinsic function is bad.  Despite being generic, this message is
 * generally pretty informative and useful.  It has the form:
 *
 *   argument %d of %s incorrectly supplied as %s
 *
 * The argument number is base 0.  I.e.  argerror(0) indicates the 1st
 * argument is bad.
 *
 * The function returns 1, for use in a direct return from an intrinsic
 * function.
 *
 * This function may only be called from the implementation of an intrinsic
 * function.  It takes the function name from the current operand stack, which
 * therefore should not have been distured (which is normal for intrincic
 * functions).  This function is typically used from C coded functions that
 * are not using typecheck() to process arguments.  For example, a
 * function that just takes a single mem object as an argument might start:
 *
 *  static int
 *  myfunc()
 *  {
 *      object  *o;
 *
 *      if (NARGS() != 1)
 *          return argcount(1);
 *      if (!ismem(ARG(0)))
 *          return argerror(0);
 *      . . .
 *
 * This --func-- forms part of ICI's exernal API --ici-api-- 
 */
int
argerror(int i)
{
    char        n1[30];
    char        n2[30];

    return set_error("argument %d of %s incorrectly supplied as %s",
        i + 1,
        objname(n1, os.a_top[-1]),
        objname(n2, ARG(i)));
}

/*
 * Generate a generic error message to indicate that the wrong number of
 * arguments have been supplied to an intrinsic function, and that it really
 * (or most commonly) takes 'n'.  This function sets the error descriptor
 * (error) to a message like:
 *
 *      %d arguments given to %s, but it takes %d
 *
 * and then returns 1.
 *
 * This function may only be called from the implementation of an intrinsic
 * function.  It takes the number of actual argument and the function name
 * from the current operand stack, which therefore should not have been
 * distured (which is normal for intrincic functions).  It takes the number of
 * arguments the function should have been supplied with (or typically is)
 * from 'n'.  This function is typically used from C coded functions that are
 * not using typecheck() to process arguments.  For example, a function
 * that just takes a single object as an argument might start:
 *
 *      static int
 *      myfunc()
 *      {
 *          object  *o;
 *
 *          if (NARGS() != 1)
 *              return argcount(1);
 *          o = ARG(0);
 *          . . .
 *
 * This function forms part of ICI's exernal API --ici-api-- --func--
 */
int
argcount(int n)
{
    char        n1[30];

    return set_error("%d arguments given to %s, but it takes %d",
        NARGS(), objname(n1, os.a_top[-1]), n);
}

/*
 * Similar to argcount() this is used to generate a generic error message
 * to indicate the wrong number of arguments have been supplied to an intrinsic
 * function.  This function is intended for use by functions that take a varying
 * number of arguments and permits the caller to specify the minimum and
 * maximum argument counts which are used to set the error description to a
 * message like:
 *
 *      %d arguments given to %s, but it takes from %d to %d arguments
 *
 * Other than the differing number of parameters, two rather than one, and
 * the message generated this function behaves in the same manner as argcount()
 * and has the same restrictions upon where it may be used.
 *
 * This function forms part of ICI's exernal API --ici-api-- --func--
 */
int
argcount2(int m, int n)
{
    char        n1[30];

    return set_error("%d arguments given to %s, but it takes from %d to %d arguments",
        NARGS(), objname(n1, os.a_top[-1]), m, n);
}

/*
 * General way out of an intrinsic function returning the object 'o', but
 * the given object has a reference count which must be decref'ed on the
 * way out. Return 0 unless the given 'o' is NULL, in which case it returns
 * 1 with no other action.
 *
 * This is suitable for using as a return from an intrinsic function
 * as say:
 *
 *      return ret_with_decref(new_int(2));
 *
 * (Although see int_ret().) If the object you wish to return does
 * not have an extra reference, use ret_no_decref().
 *
 * This function forms part of ICI's exernal API --ici-api-- --func--
 */
int
ret_with_decref(object *o)
{
    if (o == NULL)
        return 1;
    os.a_top -= NARGS() + 1;
    os.a_top[-1] = o;
    o->decref();
    --xs.a_top;
    return 0;
}

/*
 * General way out of an intrinsic function returning the object o where
 * the given object has no extra refernce count. Returns 0 indicating no
 * error.
 *
 * This is suitable for using as a return from an intrinsic function
 * as say:
 *
 *      return ret_no_decref(o);
 *
 * If the object you are returning has an extra reference which must be
 * decremented as part of the return, use ret_with_decref() (above).
 *
 * This function forms part of ICI's exernal API --ici-api-- --func--
 */
int
ret_no_decref(object *o)
{
    if (o == NULL)
        return 1;
    os.a_top -= NARGS() + 1;
    os.a_top[-1] = o;
    --xs.a_top;
    return 0;
}

/*
 * Use 'return int_ret(ret);' to return an integer (i.e. a C long) from
 * an intrinsic fuction.
 *
 * This function forms part of ICI's exernal API --ici-api-- --func--
 */
int
int_ret(int64_t ret)
{
    return ret_with_decref(new_int(ret));
}

/*
 * Use 'return float_ret(ret);' to return a float (i.e. a C double)
 * from an intrinsic fuction. The double will be converted to an ICI
 * float.
 * 
 *
 * This function forms part of ICI's exernal API --ici-api-- --func--
 */
int
float_ret(double ret)
{
    return ret_with_decref(new_float(ret));
}

/*
 * Use 'return str_ret(str);' to return a nul terminated string from
 * an intrinsic fuction. The string will be converted into an ICI string.
 *
 * This function forms part of ICI's exernal API --ici-api-- --func--
 */
int
str_ret(const char *str)
{
    return ret_with_decref(new_str_nul_term(str));
}

static object *
not_a(const char *what, const char *typ)
{
    set_error("%s is not a %s", what, typ);
    return NULL;
}

/*
 * Return the array object that is the current value of "path" in the
 * current scope. The array is not increfed - it is assumed to be still
 * referenced from the scope until the caller has finished with it.
 */
array *need_path()
{
    object           *o;

    o = ici_fetch(vs.a_top[-1], SS(path));
    if (!isarray(o))
    {
        return arrayof(not_a("path", "array"));
    }
    return arrayof(o);
}

/*
 * Return the ICI file object that is the current value of the 'stdin'
 * name in the current scope. Else NULL, usual conventions. The file
 * has not increfed (it is referenced from the current scope, until
 * that assumption is broken, it is known to be uncollectable).
 *
 * This --func-- forms part of the --ici-api--.
 */
file *need_stdin()
{
    file          *f;

    f = fileof(ici_fetch(vs.a_top[-1], SS(_stdin)));
    if (!isfile(f))
    {
        return fileof(not_a("stdin", "file"));
    }
    return f;
}

/*
 * Return the file object that is the current value of the 'stdout'
 * name in the current scope. Else NULL, usual conventions.  The file
 * has not increfed (it is referenced from the current scope, until
 * that assumption is broken, it is known to be uncollectable).
 *
 * This --func-- forms part of the --ici-api--.
 */
file *need_stdout()
{
    file          *f;

    f = fileof(ici_fetch(vs.a_top[-1], SS(_stdout)));
    if (!isfile(f))
    {
        return fileof(not_a("stdout", "file"));
    }
    return f;
}

/*
 * For any C functions that return a double and take 0, 1, or 2 doubles as
 * arguments.
 */
static int
f_math()
{
    double      av[2];
    double      r;
    char        n1[30];
    char        n2[80];

    av[0] = 0.0;
    av[1] = 0.0;
    if (typecheck((char *)ICI_CF_ARG2() + 2, &av[0], &av[1]))
        return 1;
    errno = 0;
    r = (*(double (*)(...))ICI_CF_ARG1())(av[0], av[1]);
    if (errno != 0)
    {
        sprintf(n2, "%g", av[0]);
        if (NARGS() == 2)
            sprintf(n2 + strlen(n2), ", %g", av[1]);
         return get_last_errno(objname(n1, os.a_top[-1]), n2);
    }
    return float_ret(r);
}

/*
 * Stand-in function for core functions that are coded in ICI.  The real
 * function is loaded on first call, and replaces this one, then we transfer
 * to it.  On subsequent calls, this code is out of the picture.
 *
 * cf_arg1              The name (an ICI string) of the function to be loaded
 *                      and transfered to.
 *
 * cf_arg2              The name (an ICI string) of the core ICI extension
 *                      module that defines the function.  (Eg "core1",
 *                      meaning the function is in "ici4core1.ici".  Only
 *                      "ici4core.ici" is always parsed.  Others are
 *                      on-demand.)
 */
static int
f_coreici(object *s)
{
    object           *c;
    object           *f;

    /*
     * Use the execution engine to evaluate the name of the core module
     * this function is in. It will auto-load if necessary.
     */
    if ((c = evaluate((object *)ICI_CF_ARG2(), 0)) == NULL)
        return 1;
    /*
     * Fetch the real function from that module and verify it is callable.
     */
    f = ici_fetch_base(c, (object *)ICI_CF_ARG1());
    c->decref();
    if (f == NULL)
        return 1;
    if (!f->can_call())
    {
        char    n1[30];
        return set_error("attempt to call %s", objname(n1, f));
    }
    /*
     * Over-write the definition of the function (which was us) with the
     * real function.
     */
    if (ici_assign(vs.a_top[-1], (object *)ICI_CF_ARG1(), f))
        return 1;
    /*
     * Replace us with the new callable object on the operand stack
     * and transfer to it.
     */
    os.a_top[-1] = f;
    return f->call(s);
}

/*--------------------------------------------------------------------------------*/

static int
f_array(...)
{
    int        nargs;
    array    *a;
    object  **o;

    nargs = NARGS();
    if ((a = new_array(nargs)) == NULL)
        return 1;
    for (o = ARGS(); nargs > 0; --nargs)
        *a->a_top++ = *o--;
    return ret_with_decref(a);
}

static int
f_struct()
{
    object     **o;
    int          nargs;
    ici_struct  *s;
    objwsup     *super;

    nargs = NARGS();
    o = ARGS();
    super = NULL;
    if (nargs & 1)
    {
        super = objwsupof(*o);
        if (!hassuper(super) && !isnull(super))
            return argerror(0);
        if (isnull(super))
            super = NULL;
        --nargs;
        --o;
    }
    if ((s = new_struct()) == NULL)
        return 1;
    for (; nargs >= 2; nargs -= 2, o -= 2)
    {
        if (ici_assign(s, o[0], o[-1]))
        {
            s->decref();
            return 1;
        }
    }
    s->o_super = super;
    return ret_with_decref(s);
}

static int
f_set()
{
    int      nargs;
    set     *s;
    object **o;

    if ((s = new_set()) == NULL)
        return 1;
    for (nargs = NARGS(), o = ARGS(); nargs > 0; --nargs, --o)
    {
        if (ici_assign(s, *o, o_one))
        {
            s->decref();
            return 1;
        }
    }
    return ret_with_decref(s);
}

static int
f_keys()
{
    array    *k;

    if (NARGS() != 1)
        return argcount(1);
    if (isstruct(ARG(0)))
    {
        ici_struct *s = structof(ARG(0));
        sslot *sl;

        if ((k = new_array(s->s_nels)) == NULL)
            return 1;
        for (sl = s->s_slots; sl < s->s_slots + s->s_nslots; ++sl)
        {
            if (sl->sl_key != NULL)
                *k->a_top++ = sl->sl_key;
        }
    }
    else if (isset(ARG(0)))
    {
        set *s = setof(ARG(0));
        size_t i;

        if ((k = new_array(s->s_nels)) == NULL)
            return 1;
        for (i = 0; i < s->s_nslots; ++i)
        {
            object *o;
            if ((o = s->s_slots[i]) != NULL)
                *k->a_top++ = o;
        }
    }
    else
    {
        return argerror(0);
    }

    return ret_with_decref(k);
}

static int
f_copy(object *o)
{
    if (o != NULL)
        return ret_with_decref(ici_copy(o));
    if (NARGS() != 1)
        return argcount(1);
    return ret_with_decref(ici_copy(ARG(0)));
}

static int
f_typeof()
{
    if (NARGS() != 1)
        return argcount(1);
    if (ishandle(ARG(0)))
        return ret_no_decref(handleof(ARG(0))->h_name);
    return ret_no_decref(ARG(0)->otype()->ici_name());
}

static int
f_nels()
{
    object  *o;
    size_t     size;

    if (NARGS() != 1)
        return argcount(1);
    o = ARG(0);
    if (isstring(o))
        size = stringof(o)->s_nchars;
    else if (isarray(o))
        size = arrayof(o)->len();
    else if (isstruct(o))
        size = structof(o)->s_nels;
    else if (isset(o))
        size = setof(o)->s_nels;
    else if (ismem(o))
        size = memof(o)->m_length;
    else if (ischannel(o))
        size = channelof(o)->c_capacity;
    else
        size = 1;
    return int_ret(size);
}

static int
f_int()
{
    object  *o;
    long       v;

    if (NARGS() < 1)
        return argcount(1);
    o = ARG(0);
    if (isint(o))
        return ret_no_decref(o);
    else if (isstring(o))
    {
        int base = 0;

        if (NARGS() > 1)
        {
            if (!isint(ARG(1)))
                return argerror(1);
            base = intof(ARG(1))->i_value;
            if (base != 0 && (base < 2 || base > 36))
                return argerror(1);
        }
        v = xstrtol(stringof(o)->s_chars, NULL, base);
    }
    else if (isfloat(o))
        v = (long)floatof(o)->f_value;
    else
        v = 0;
    return int_ret(v);
}

static int
f_float()
{
    object  *o;
    double     v;

    if (NARGS() != 1)
        return argcount(1);
    o = ARG(0);
    if (isfloat(o))
        return ret_no_decref(o);
    else if (isstring(o))
        v = strtod(stringof(o)->s_chars, NULL);
    else if (isint(o))
        v = (double)intof(o)->i_value;
    else
        v = 0;
    return float_ret(v);
}

static int
f_num()
{
    object  *o;
    double     f;
    long       i;
    char                *s;
    char                n[30];

    if (NARGS() != 1)
        return argcount(1);
    o = ARG(0);
    if (isfloat(o) || isint(o))
        return ret_no_decref(o);
    else if (isstring(o))
    {
        int             base = 0;

        if (NARGS() > 1)
        {
            if (!isint(ARG(1)))
                return argerror(1);
            base = intof(ARG(1))->i_value;
            if (base != 0 && (base < 2 || base > 36))
                return argerror(1);
        }
        i = xstrtol(stringof(o)->s_chars, &s, base);
        if (*s == '\0')
            return int_ret(i);
        f = strtod(stringof(o)->s_chars, &s);
        if (*s == '\0')
            return float_ret(f);
    }
    return set_error("%s is not a number", objname(n, o));
}

static int
f_string()
{
    object  *o;

    if (NARGS() != 1)
        return argcount(1);
    o = ARG(0);
    if (isstring(o))
        return ret_no_decref(o);
    if (isint(o))
        sprintf(buf, "%lld", static_cast<long long int>(intof(o)->i_value));
    else if (isfloat(o))
        sprintf(buf, "%g", floatof(o)->f_value);
    else if (isregexp(o))
        return ret_no_decref(regexpof(o)->r_pat);
    else
        sprintf(buf, "<%s>", o->type_name());
    return str_ret(buf);
}

static int
f_eq()
{
    object   *o1;
    object   *o2;

    if (typecheck("oo", &o1, &o2))
        return 1;
    if (o1 == o2)
        return ret_no_decref(o_one);
    return ret_no_decref(o_zero);
}

static int
f_push()
{
    array *a;
    object   *o;

    if (typecheck("ao", &a, &o))
        return 1;
    if (a->push(o))
        return 1;
    return ret_no_decref(o);
}

static int
f_rpush()
{
    array *a;
    object   *o;

    if (typecheck("ao", &a, &o))
        return 1;
    if (a->rpush(o))
        return 1;
    return ret_no_decref(o);
}

static int
f_pop()
{
    array *a;
    object   *o;

    if (typecheck("a", &a))
        return 1;
    if ((o = a->pop()) == NULL)
        return 1;
    return ret_no_decref(o);
}

static int
f_rpop()
{
    array *a;
    object   *o;

    if (typecheck("a", &a))
        return 1;
    if ((o = a->rpop()) == NULL)
        return 1;
    return ret_no_decref(o);
}

static int
f_top()
{
    array *a;
    long        n = 0;

    switch (NARGS())
    {
    case 1:
        if (typecheck("a", &a))
            return 1;
        break;

    default:
        if (typecheck("ai", &a, &n))
            return 1;
    }
    n += a->len() - 1;
    return ret_no_decref(a->get(n));
}

static int
f_parse()
{
    object     *o;
    file       *f;
    ici_struct *s;              /* Statics. */
    ici_struct *a;              /* Autos. */

    switch (NARGS())
    {
    case 1:
        if (typecheck("o", &o))
            return 1;
        if ((a = new_struct()) == NULL)
            return 1;
        if ((a->o_super = objwsupof(s = new_struct())) == NULL)
        {
            a->decref();
            return 1;
        }
        s->decref();
        s->o_super = objwsupof(vs.a_top[-1])->o_super;
        break;

    default:
        if (typecheck("od", &o, &a))
            return 1;
        a->incref();
        break;
    }

    if (isstring(o))
    {
        if ((f = sopen(stringof(o)->s_chars, stringof(o)->s_nchars, o)) == NULL)
        {
            a->decref();
            return 1;
        }
        f->f_name = SS(empty_string);
    }
    else if (isfile(o))
        f = fileof(o);
    else
    {
        a->decref();
        return argerror(0);
    }

    if (parse_file(f, objwsupof(a)) < 0)
        goto fail;

    if (isstring(o))
        f->decref();
    return ret_with_decref(a);

fail:
    if (isstring(o))
        f->decref();
    a->decref();
    return 1;
}

static int f_include()
{
    str        *filename;
    ici_struct *a;
    int         rc;
    file       *f;

    switch (NARGS())
    {
    case 1:
        if (typecheck("o", &filename))
            return 1;
        a = structof(vs.a_top[-1]);
        break;

    case 2:
        if (typecheck("od", &filename, &a))
            return 1;
        break;

    default:
        return argcount(2);
    }
    if (!isstring(filename))
        return argerror(0);
#ifndef NODEBUGGING
    debug_ignore_errors();
#endif
    if (call(SS(fopen), "o=o", &f, filename))
    {
        char    fname[1024];

        strncpy(fname, filename->s_chars, 1023);
        if (!find_on_path(fname, NULL))
        {
#ifndef NODEBUGGING
            debug_respect_errors();
#endif
            return set_error("could not find \"%s\" on path", fname);
        }
        if (call(SS(fopen), "o=s", &f, fname))
        {
#ifndef NODEBUGGING
            debug_respect_errors();
#endif
            return 1;
        }
    }
#ifndef NODEBUGGING
    debug_respect_errors();
#endif
    rc = parse_file(f, objwsupof(a));
    call(SS(close), "o", f);
    f->decref();
    return rc < 0 ? 1 : ret_no_decref(a);
}

static int
f_call()
{
    array    *aa;               /* The array with extra arguments, or NULL. */
    int       nargs;            /* Number of args to target function. */
    int       naargs;           /* Number of args comming from the array. */
    object  **base;
    object  **e;
    int       i;
    ici_int  *nargso;
    object   *func;

    if (NARGS() < 2)
        return argcount(2);
    nargso = NULL;
    base = &ARG(NARGS() - 1);
    if (isarray(*base))
        aa = arrayof(*base);
    else if (isnull(*base))
        aa = NULL;
    else
        return argerror(NARGS() - 1);
    if (aa == NULL)
        naargs = 0;
    else
        naargs = aa->len();
    nargs = naargs + NARGS() - 2;
    func = ARG(0);
    func->incref();
    /*
     * On the operand stack, we have...
     *    [aa] [argn]...[arg2] [arg1] [func] [nargs] [us] [    ]
     *      ^                                               ^
     *      +-base                                          +-os.a_top
     *
     * We want...
     *    [aa[n]]...[aa[1]] [aa[0]] [argn]...[arg2] [arg1] [nargs] [func] [    ]
     *      ^                                                               ^
     *      +-base                                                          +-os.a_top
     *
     * Do everything that can get an error first, before we start playing with
     * the stack.
     *
     * We include an extra 80 in our ici_stk_push_chk, see start of
     * evaluate().
     */
    if (os.stk_push_chk(naargs + 80))
        goto fail;
    base = &ARG(NARGS() - 1);
    if (aa != NULL)
        aa = arrayof(*base);
    if ((nargso = new_int(nargs)) == NULL)
        goto fail;
    /*
     * First move the arguments that we want to keep up to the stack
     * to their new position (all except the func and the array).
     */
    memmove(base + naargs, base + 1, (NARGS() - 2) * sizeof (object *));
    os.a_top += naargs - 2;
    if (naargs > 0)
    {
        i = naargs;
        for (e = aa->astart(); i > 0; e = aa->anext(e))
            base[--i] = *e;
    }
    /*
     * Push the count of actual args and the target function.
     */
    os.a_top[-2] = nargso;
    nargso->decref();
    os.a_top[-1] = func;
    func->decref();
    xs.a_top[-1] = &o_call;
    /*
     * Very special return. Drops back into the execution loop with
     * the call on the execution stack.
     */
    return 0;

fail:
    func->decref();
    if (nargso != NULL)
        nargso->decref();
    return 1;
}

static int
f_fail()
{
    char        *s;

    if (typecheck("s", &s))
        return 1;
    return set_error("%s", s);
}

static int
f_exit()
{
    object   *rc;
    long        status;

    switch (NARGS())
    {
    case 0:
        rc = ici_null;
        break;

    case 1:
        if (typecheck("o", &rc))
            return 1;
        break;

    default:
        return argcount(1);
    }
    if (isint(rc))
        status = (int)intof(rc)->i_value;
    else if (rc == ici_null)
        status = 0;
    else if (isstring(rc))
    {
        if (stringof(rc)->s_nchars == 0)
            status = 0;
        else
        {
            if (strchr(stringof(rc)->s_chars, ':') != NULL)
                fprintf(stderr, "%s\n", stringof(rc)->s_chars);
            else
                fprintf(stderr, "exit: %s\n", stringof(rc)->s_chars);
            status = 1;
        }
    }
    else
    {
        return argerror(0);
    }
    uninit();
    exit((int)status);
    /*NOTREACHED*/
}

static int
f_vstack()
{
    int                 depth;

    if (NARGS() == 0)
        return ret_with_decref(ici_copy(&vs));

    if (!isint(ARG(0)))
        return argerror(0);
    depth = intof(ARG(0))->i_value;
    if (depth < 0)
        return argerror(0);
    if (depth >= vs.a_top - vs.a_bot)
        return null_ret();
    return ret_no_decref(vs.a_top[-depth - 1]);
}

static int
f_tochar()
{
    long        i;

    if (typecheck("i", &i))
        return 1;
    buf[0] = (unsigned char)i;
    return ret_with_decref(new_str(buf, 1));
}

static int
f_toint()
{
    char        *s;

    if (typecheck("s", &s))
        return 1;
    return int_ret((long)(s[0] & 0xFF));
}

static int
f_rand()
{
    static long seed    = 1;

    if (NARGS() >= 1)
    {
        if (typecheck("i", &seed))
            return 1;
        srand(seed);
    }
#ifdef ICI_RAND_IS_C_RAND
    return int_ret(rand());
#else
    seed = seed * 1103515245 + 12345;
    return int_ret((seed >> 16) & 0x7FFF);
#endif
}

static int
f_interval()
{
    object        *o;
    int64_t       start;
    int64_t       length;
    size_t        nel;
    str           *s = 0; /* init to shut up compiler */
    array         *a = 0; /* init to shut up compiler */
    array         *a1;


    if (typecheck("oi*", &o, &start))
        return 1;
    if (isstring(o))
    {
        s = stringof(o);
        nel = s->s_nchars;
    }
    else if (isarray(o))
    {
        a = arrayof(o);
        nel = a->len();
    }
    else
    {
        return argerror(0);
    }

    length = nel;
    if (NARGS() > 2)
    {
        if (!isint(ARG(2)))
            return argerror(2);
        if ((length = intof(ARG(2))->i_value) < 0)
            argerror(2);
    }

    if (length < 0)
    {
        if ((length += nel) < 0)
            length = 0;
    }
    if (start < 0)
    {
        if ((start += nel) < 0)
        {
            if ((length += start) < 0)
                length = 0;
            start = 0;
        }
    }
    else if (start > int64_t(nel))
        start = nel;
    if (start + length > int64_t(nel))
        length = nel - start;

    if (isstring(o))
    {
        return ret_with_decref(new_str(s->s_chars + start, (int)length));
    }
    else
    {
        if ((a1 = new_array(length)) == NULL)
            return 1;
        a->gather(a1->a_base, start, length);
        a1->a_top += length;
        return ret_with_decref(a1);
    }
}

static int
f_explode()
{
    int        i;
    char                *s;
    array         *x;

    if (typecheck("s", &s))
        return 1;
    i = stringof(ARG(0))->s_nchars;
    if ((x = new_array(i)) == NULL)
        return 1;
    while (--i >= 0)
    {
        if ((*x->a_top = new_int(*s++ & 0xFFL)) == NULL)
        {
            x->decref();
            return 1;
        }
        (*x->a_top)->decref();
        ++x->a_top;
    }
    return ret_with_decref(x);
}

static int
f_implode()
{
    array   *a;
    int      i;
    object **o;
    str     *s;
    char    *p;

    if (typecheck("a", &a))
        return 1;
    i = 0;
    for (o = a->astart(); o != a->alimit(); o = a->anext(o))
    {
        if (isint(*o))
            ++i;
        else if (isstring(*o))
            i += stringof(*o)->s_nchars;
    }
    if ((s = str_alloc(i)) == NULL)
        return 1;
    p = s->s_chars;
    for (o = a->astart(); o != a->alimit(); o = a->anext(o))
    {
        if (isint(*o))
            *p++ = (char)intof(*o)->i_value;
        else if (isstring(*o))
        {
            memcpy(p, stringof(*o)->s_chars, stringof(*o)->s_nchars);
            p += stringof(*o)->s_nchars;
        }
    }
    if ((s = stringof(atom(s, 1))) == NULL)
        return 1;
    return ret_with_decref(s);
}

static int
f_sopen()
{
    file  *f;
    char        *str;
    const char  *mode;
    int         readonly;

    mode = "r";
    if (typecheck(NARGS() > 1 ? "ss" : "s", &str, &mode))
        return 1;
    readonly = 1;
    if (strcmp(mode, "r") != 0 && strcmp(mode, "rb") != 0)
    {
        if (strcmp(mode, "r+") != 0 && strcmp(mode, "r+b") != 0)
        {
            return set_error("attempt to use mode \"%s\" in sopen()", mode);
        }
        readonly = 0;
    }
    if ((f = open_charbuf(str, stringof(ARG(0))->s_nchars, ARG(0), readonly)) == NULL)
        return 1;
    f->f_name = SS(empty_string);
    return ret_with_decref(f);
}

static int
f_mopen()
{
    mem        *mem;
    file       *f;
    const char *mode;
    int         readonly;

    mode = "r";
    if (typecheck(NARGS() > 1 ? "ms" : "m", &mem, &mode))
        return 1;
    readonly = 1;
    if (strcmp(mode, "r") && strcmp(mode, "rb"))
    {
        if (strcmp(mode, "r+") && strcmp(mode, "r+b"))
        {
            return set_error("attempt to use mode \"%s\" in mopen()", mode);
        }
        readonly = 0;
    }
    if (mem->m_accessz != 1)
    {
        return set_error("memory object must have access size of 1 to be opened");
    }
    if ((f = open_charbuf((char *)mem->m_base, (int)mem->m_length, mem, readonly)) == NULL)
        return 1;
    f->f_name = SS(empty_string);
    return ret_with_decref(f);
}

int
ici_f_sprintf()
{
    char     *fmt;
    char     *p;
    int       i;                /* Where we are up to in buf. */
    int       j;
    long      which;
    int       nargs;
    char      subfmt[40];       /* %...? portion of string. */
    int       stars[2];         /* Precision and field widths. */
    int       nstars;
    int       gotl;             /* Have a long int flag. */
    int       gotdot;           /* Have a . in % format. */
    int64_t   ivalue;
    double    fvalue;
    char     *svalue;
    object  **o;                /* Argument pointer. */
    file     *file;
    char      oname[objnamez];
#ifdef  BAD_PRINTF_RETVAL
#define IPLUSEQ
#else
#define IPLUSEQ         i +=
#endif

    which = (long)ICI_CF_ARG1(); /* sprintf, printf, fprintf */
    if (which != 0 && NARGS() > 0 && isfile(ARG(0)))
    {
        which = 2;
        if (typecheck("us*", &file, &fmt))
            return 1;
        o = ARGS() - 2;
        nargs = NARGS() - 2;
    }
    else
    {
        if (typecheck("s*", &fmt))
            return 1;
        o = ARGS() - 1;
        nargs = NARGS() - 1;
    }

    p = fmt;
    i = 0;
    while (*p != '\0')
    {
        if (*p != '%')
        {
            if (chkbuf(i))
                return 1;
            buf[i++] = *p++;
            continue;
        }

        nstars = 0;
        stars[0] = 0;
        stars[1] = 0;
        gotl = 0;
        gotdot = 0;
        subfmt[0] = *p++;
        j = 1;
        while (*p != '\0' && strchr("adiouxXfeEgGcs%", *p) == NULL)
        {
            if (*p == '*')
                ++nstars;
            else if (*p == 'l')
                gotl++;
            else if (*p == '.')
                gotdot = 1;
            else if (*p >= '0' && *p <= '9')
            {
                do
                {
                    stars[gotdot] = stars[gotdot] * 10 + *p - '0';
                    subfmt[j++] = *p++;

                } while (*p >= '0' && *p <= '9');
                continue;
            }
            subfmt[j++] = *p++;
        }
        if (strchr("diouxX", *p) != NULL) {
            subfmt[j++] = 'l';
            subfmt[j++] = 'l';
        }
        subfmt[j++] = *p;
        subfmt[j++] = '\0';
        if (nstars > 2)
            nstars = 2;
        for (j = 0; j < nstars; ++j)
        {
            if (nargs <= 0)
                goto lacking;
            if (!isint(*o))
                goto type;
            stars[j] = (int)intof(*o)->i_value;
            --o;
            --nargs;
        }
        switch (*p++)
        {
        case 'd':
        case 'i':
        case 'o':
        case 'u':
        case 'x':
        case 'X':
            if (nargs <= 0)
                goto lacking;
            if (isint(*o))
                ivalue = intof(*o)->i_value;
            else if (isfloat(*o))
                ivalue = (long)floatof(*o)->f_value;
            else
                goto type;
            if (chkbuf(i + 30 + stars[0] + stars[1])) /* Pessimistic. */
                return 1;
            switch (nstars)
            {
            case 0:
                IPLUSEQ sprintf(&buf[i], subfmt, ivalue);
                break;

            case 1:
                IPLUSEQ sprintf(&buf[i], subfmt, stars[0], ivalue);
                break;

            case 2:
                IPLUSEQ sprintf(&buf[i], subfmt, stars[0], stars[1], ivalue);
                break;
            }
            --o;
            --nargs;
            break;

        case 'c':
            if (nargs <= 0)
                goto lacking;
            if (isint(*o))
                ivalue = intof(*o)->i_value;
            else if (isfloat(*o))
                ivalue = (long)floatof(*o)->f_value;
            else
                goto type;
            if (chkbuf(i + 30 + stars[0] + stars[1])) /* Pessimistic. */
                return 1;
            switch (nstars)
            {
            case 0:
                IPLUSEQ sprintf(&buf[i], subfmt, (int)ivalue);
                break;

            case 1:
                IPLUSEQ sprintf(&buf[i], subfmt, stars[0], (int)ivalue);
                break;

            case 2:
                IPLUSEQ sprintf(&buf[i], subfmt, stars[0], stars[1], (int)ivalue);
                break;
            }
            --o;
            --nargs;
            break;

        case 's':
            if (nargs <= 0)
                goto lacking;
            if (!isstring(*o))
                goto type;
            svalue = stringof(*o)->s_chars;
            if (chkbuf(i + stringof(*o)->s_nchars + stars[0] + stars[1]))
                return 1;
            switch (nstars)
            {
            case 0:
                IPLUSEQ sprintf(&buf[i], subfmt, svalue);
                break;

            case 1:
                IPLUSEQ sprintf(&buf[i], subfmt, stars[0], svalue);
                break;

            case 2:
                IPLUSEQ sprintf(&buf[i], subfmt, stars[0], stars[1], svalue);
                break;
            }
            --o;
            --nargs;
            break;

        case 'a':
            if (nargs <= 0)
                goto lacking;
            objname(oname, *o);
            svalue = oname;
            if (chkbuf(i + objnamez + stars[0] + stars[1]))
                return 1;
            subfmt[strlen(subfmt) - 1] = 's';
            switch (nstars)
            {
            case 0:
                IPLUSEQ sprintf(&buf[i], subfmt, svalue);
                break;

            case 1:
                IPLUSEQ sprintf(&buf[i], subfmt, stars[0], svalue);
                break;

            case 2:
                IPLUSEQ sprintf(&buf[i], subfmt, stars[0], stars[1], svalue);
                break;
            }
            --o;
            --nargs;
            break;

        case 'f':
        case 'e':
        case 'E':
        case 'g':
        case 'G':
            if (nargs <= 0)
                goto lacking;
            if (isint(*o))
                fvalue = intof(*o)->i_value;
            else if (isfloat(*o))
                fvalue = floatof(*o)->f_value;
            else
                goto type;
            if (chkbuf(i + 40 + stars[0] + stars[1])) /* Pessimistic. */
                return 1;
            switch (nstars)
            {
            case 0:
                IPLUSEQ sprintf(&buf[i], subfmt, fvalue);
                break;

            case 1:
                IPLUSEQ sprintf(&buf[i], subfmt, stars[0], fvalue);
                break;

            case 2:
                IPLUSEQ sprintf(&buf[i], subfmt, stars[0], stars[1], fvalue);
                break;
            }
            --o;
            --nargs;
            break;

        case '%':
            if (chkbuf(i))
                return 1;
            buf[i++] = '%';
            continue;
        }
#ifdef  BAD_PRINTF_RETVAL
        i = strlen(buf); /* old BSD sprintf doesn't return usual value. */
#endif
    }
    buf[i] = '\0';
    switch (which)
    {
    case 1: /* printf */
        if ((file = need_stdout()) == NULL)
            return 1;
    case 2: /* fprintf */
        if (file->flagged(ICI_F_CLOSED))
        {
            return set_error("write to closed file");
        }
        {
            char        small_buf[128];
            char        *out_buf;
            exec  *x = NULL;

            if (i <= (int)(sizeof small_buf))
            {
                out_buf = small_buf;
            }
            else
            {
                if ((out_buf = (char *)ici_nalloc(i)) == NULL)
                    return 1;
            }
            memcpy(out_buf, buf, i);
            if (file->flagged(FT_NOMUTEX))
                x = leave();
            file->write(out_buf, i);
            if (file->flagged(FT_NOMUTEX))
                enter(x);
            if (out_buf != small_buf)
                ici_nfree(out_buf, i);
        }
        return int_ret((long)i);

    default: /* sprintf */
        return ret_with_decref(new_str(buf, i));
    }

type:
    return set_error("attempt to use a %s with a \"%s\" format in sprintf", (*o)->type_name(), subfmt);

lacking:
    return set_error("not enoughs args to sprintf");
}

static int
f_currentfile()
{
    object   **o;
    int         raw;
    file  *f;

    raw = NARGS() > 0 && ARG(0) == SS(raw);
    for (o = xs.a_top - 1; o >= xs.a_base; --o)
    {
        if (isparse(*o))
        {
            if (raw)
                return ret_no_decref(parseof(*o)->p_file);
            f = new_file(*o, parse_ftype, parseof(*o)->p_file->f_name, *o);
            if (f == NULL)
                return 1;
            return ret_with_decref(f);
        }
    }
    return null_ret();
}

static int
f_del()
{
    object   *s;
    object   *o;

    if (typecheck("oo", &s, &o))
        return 1;
    if (isstruct(s))
    {
        unassign(structof(s), o);
    }
    else if (isset(s))
    {
        unassign(setof(s), o);
    }
    else if (isarray(s))
    {
        array      *a;
        object    **e;
        long        i;
        ptrdiff_t   n;

        
        if (!isint(o))
            return null_ret();
        a = arrayof(s);
        i = intof(o)->i_value;
        n = a->len();
        if (i < 0 || i >= n)
            return null_ret();
        if (s->isatom())
        {
            return set_error("attempt to modify to an atomic array");
        }
        if (i >= n / 2)
        {
            object       **prev_e;

            e = a->find_slot(i);
            prev_e = e;
            for (e = a->anext(e); e != a->alimit(); e = a->anext(e))
            {
                *prev_e = *e;
                prev_e = e;
            }
            a->pop();
        }
        else
        {
            object       *prev_o;

            prev_o = *(e = a->astart());
            for (e = a->anext(e); --i >= 0; e = a->anext(e))
            {
                o = *e;
                *e = prev_o;
                prev_o = o;
            }
            a->rpop();
        }
    }
    else
    {
        return argerror(0);
    }
    return null_ret();
}

/*
 * super_loop()
 *
 * Return 1 and set error if the super chain of the given struct has
 * a loop. Else return 0.
 */
static int
super_loop(objwsup *base)
{
    objwsup       *s;

    /*
     * Scan up the super chain setting the mark flag as we go. If we hit
     * a marked struct, we must have looped. Note that the mark flag
     * is a strictly transitory flag that can only be used in local
     * non-allocating areas such as this. It must be left cleared at all
     * times. The garbage collector assumes it is cleared on all objects
     * when it runs.
     */
    for (s = base; s != NULL; s = s->o_super)
    {
        if (s->marked())
        {
            /*
             * A loop. Clear all the mark flags we set and set error.
             */
            for (s = base; s->marked(); s = s->o_super)
                s->clrmark();
            return set_error("cycle in struct super chain");
        }
        s->setmark();
    }
    /*
     * No loop. Clear all the mark flags we set.
     */
    for (s = base; s != NULL; s = s->o_super)
        s->clrmark();
    return 0;
}

static int
f_super()
{
    objwsup       *o;
    objwsup       *newsuper;
    objwsup       *oldsuper;

    if (typecheck("o*", &o))
        return 1;
    if (!hassuper(o))
        return argerror(0);
    newsuper = oldsuper = o->o_super;
    if (NARGS() >= 2)
    {
        if (o->isatom())
        {
            return set_error("attempt to set super of an atomic struct");
        }
        if (isnull(ARG(1)))
            newsuper = NULL;
        else if (hassuper(ARG(1)))
            newsuper = objwsupof(ARG(1));
        else
            return argerror(1);
        ++vsver;
    }
    o->o_super = newsuper;
    if (super_loop(o))
    {
        o->o_super = oldsuper;
        return 1;
    }
    if (oldsuper == NULL)
        return null_ret();
    return ret_no_decref(oldsuper);
}

static int
f_scope()
{
    ici_struct *s;

    s = structof(vs.a_top[-1]);
    if (NARGS() > 0)
    {
        if (typecheck("d", &vs.a_top[-1]))
            return 1;
    }
    return ret_no_decref(s);
}

static int
f_isatom()
{
    object   *o;

    if (typecheck("o", &o))
        return 1;
    return ret_no_decref(o->isatom() ? o_one : o_zero);
}

static int
f_alloc()
{
    long        length;
    int         accessz;
    char        *p;

    if (typecheck("i*", &length))
        return 1;
    if (length < 0)
    {
        return set_error("attempt to allocate negative amount");
    }
    if (NARGS() >= 2)
    {
        if
        (
            !isint(ARG(1))
            ||
            (
                (accessz = (int)intof(ARG(1))->i_value) != 1
                &&
                accessz != 2
                &&
                accessz != 4
            )
        )
            return argerror(1);
    }
    else
        accessz = 1;
    if ((p = (char *)ici_alloc((size_t)length * accessz)) == NULL)
        return 1;
    memset(p, 0, (size_t)length * accessz);
    return ret_with_decref(new_mem(p, (unsigned long)length, accessz, ici_free));
}

#ifndef NOMEM
static int
f_mem()
{
    long        base;
    long        length;
    int         accessz;

    if (typecheck("ii*", &base, &length))
        return 1;
    if (NARGS() >= 3)
    {
        if
        (
            !isint(ARG(2))
            ||
            (
                (accessz = (int)intof(ARG(2))->i_value) != 1
                &&
                accessz != 2
                &&
                accessz != 4
            )
        )
            return argerror(2);
    }
    else
        accessz = 1;
    return ret_with_decref(new_mem((char *)base, (unsigned long)length, accessz, NULL));
}
#endif

static int
f_assign()
{
    object   *s;
    object   *k;
    object   *v;

    switch (NARGS())
    {
    case 2:
        if (typecheck("oo", &s, &k))
            return 1;
        if (isset(s))
            v = o_one;
        else
            v = ici_null;
        break;

    case 3:
        if (typecheck("ooo", &s, &k, &v))
            return 1;
        break;

    default:
        return argcount(2);
    }
    if (hassuper(s))
    {
        if (ici_assign_base(s, k, v))
            return 1;
    }
    else
    {
        if (ici_assign(s, k, v))
            return 1;
    }
    return ret_no_decref(v);
}

static int
f_fetch()
{
    ici_struct *s;
    object   *k;

    if (typecheck("oo", &s, &k))
        return 1;
    if (hassuper(s))
        return ret_no_decref(ici_fetch_base(s, k));
    return ret_no_decref(ici_fetch(s, k));
}

static int
f_waitfor()
{
    object  **e;
    int                 nargs;
    fd_set              readfds;
    struct timeval      timeval;
    struct timeval      *tv;
    double              to;
    int                 nfds;

    if (NARGS() == 0)
        return ret_no_decref(o_zero);
    tv = NULL;
    nfds = 0;
    FD_ZERO(&readfds);
    to = 0.0; /* Stops warnings, not required. */
    for (nargs = NARGS(), e = ARGS(); nargs > 0; --nargs, --e)
    {
        if (isfile(*e))
        {
            int fd = fileof(*e)->fileno();
            if (fd != -1)
            {
                fileof(*e)->setvbuf(NULL, _IONBF, 0);
                FD_SET(fd, &readfds);
                if (fd >= nfds)
                    nfds = fd + 1;
            }
            else
                return ret_no_decref(*e);
        }
        else if (isint(*e))
        {
            if (tv == NULL || to > intof(*e)->i_value / 1000.0)
            {
                to = intof(*e)->i_value / 1000.0;
                tv = &timeval;
            }
        }
        else if (isfloat(*e))
        {
            if (tv == NULL || to > floatof(*e)->f_value)
            {
                to = floatof(*e)->f_value;
                tv = &timeval;
            }
        }
        else
            return argerror(ARGS() - e);
    }
    if (tv != NULL)
    {
        tv->tv_sec = to;
        tv->tv_usec = (to - tv->tv_sec) * 1000000.0;
    }
    blocking_syscall(1);
    switch (select(nfds, &readfds, NULL, NULL, tv))
    {
    case -1:
        blocking_syscall(0);
        return set_error("could not select");

    case 0:
        blocking_syscall(0);
        return ret_no_decref(o_zero);
    }
    blocking_syscall(0);
    for (nargs = NARGS(), e = ARGS(); nargs > 0; --nargs, --e)
    {
        if (!isfile(*e))
            continue;
        auto fn = fileof(*e)->fileno();
        if (fn != -1)
        {
            if (FD_ISSET(fn, &readfds))
                return ret_no_decref(*e);
        }
    }
    return set_error("no file selected");
}

static int
f_gettoken()
{
    object           *fo;
    file          *f;
    str           *s;
    unsigned char       *seps;
    int                 nseps;
    int                 c;
    int                 i;
    int                 j;

    seps = (unsigned char *)" \t\n";
    nseps = 3;
    switch (NARGS())
    {
    case 0:
        if ((f = need_stdin()) == NULL)
            return 1;
        break;

    case 1:
        if (typecheck("o", &fo))
            return 1;
        if (isstring(fo))
        {
            if ((f = sopen(stringof(fo)->s_chars, stringof(fo)->s_nchars, fo)) == NULL)
                return 1;
            f->decref();
        }
        else if (!isfile(fo))
            return argerror(0);
        else
            f = fileof(fo);
        break;

    default:
        if (typecheck("oo", &fo, &s))
            return 1;
        if (isstring(fo))
        {
            if ((f = sopen(stringof(fo)->s_chars, stringof(fo)->s_nchars, fo)) == NULL)
                return 1;
            f->decref();
        }
        else if (!isfile(fo))
            return argerror(0);
        else
            f = fileof(fo);
        if (!isstring(s))
            return argerror(1);
        seps = (unsigned char *)s->s_chars;
        nseps = s->s_nchars;
        break;
    }
    do
    {
        c = f->getch();
        if (c == EOF)
            return null_ret();
        for (i = 0; i < nseps; ++i)
        {
            if (c == seps[i])
                break;
        }

    } while (i != nseps);

    j = 0;
    do
    {
        chkbuf(j);
        buf[j++] = c;
        c = f->getch();
        if (c == EOF)
            break;
        for (i = 0; i < nseps; ++i)
        {
            if (c == seps[i])
            {
                f->ungetch(c);
                break;
            }
        }

    } while (i == nseps);

    if ((s = new_str(buf, j)) == NULL)
        return 1;
    return ret_with_decref(s);
}

/*
 * Fast (relatively) version for gettokens() if argument is not file.
 */
static int
fast_gettokens(const char *str, const char *delims)
{
    array *a;
    int         k       = 0;
    const char *cp     = str;

    if ((a = new_array(0)) == NULL)
        return 1;
    while (*cp)
    {
        while (*cp && strchr(delims, *cp))
            cp++;
        if ((k = strcspn(cp, delims)))
        {
            if
            (
                a->stk_push_chk()
                ||
                (*a->a_top = new_str(cp, k)) == NULL
            )
            {
                a->decref();
                return 1;
            }
            (*a->a_top)->decref();
            ++a->a_top;
            if (*(cp += k))
                cp++;
            continue;
        }
    }
    if (a->a_top == a->a_base)
    {
        a->decref();
        return null_ret();
    }
    return ret_with_decref(a);
}

static int
f_gettokens()
{
    object           *fo;
    file          *f;
    str           *s;
    unsigned char       *terms;
    int                 nterms;
    unsigned char       *seps;
    int                 nseps;
    unsigned char       *delims = NULL; /* init to shut up compiler */
    int                 ndelims;
    int                 hardsep;
    unsigned char       sep;
    array         *a;
    int                 c;
    int                 i;
    int                 j = 0; /* init to shut up compiler */
    int                 state;
    int                 what;
    int                 loose_it = 0;

    seps = (unsigned char *)" \t";
    nseps = 2;
    hardsep = 0;
    terms = (unsigned char *)"\n";
    nterms = 1;
    ndelims = 0;
    switch (NARGS())
    {
    case 0:
        if ((f = need_stdin()) == NULL)
            return 1;
        break;

    case 1:
        if (typecheck("o", &fo))
            return 1;
        if (isstring(fo))
        {
            return fast_gettokens(stringof(fo)->s_chars, " \t");
        }
        else if (!isfile(fo))
            return argerror(0);
        else
            f = fileof(fo);
        break;

    case 2:
    case 3:
    case 4:
        if (typecheck("oo*", &fo, &s))
            return 1;
        if (NARGS() == 2 && isstring(fo) && isstring(s))
        {
            return fast_gettokens(stringof(fo)->s_chars, stringof(s)->s_chars);
        }
        if (isstring(fo))
        {
            if ((f = sopen(stringof(fo)->s_chars, stringof(fo)->s_nchars, fo)) == NULL)
                return 1;
            loose_it = 1;
        }
        else if (!isfile(fo))
            return argerror(0);
        else
            f = fileof(fo);
        if (isint(s))
        {
            object *so = s;
            sep = (unsigned char)intof(so)->i_value;
            hardsep = 1;
            seps = (unsigned char *)&sep;
            nseps = 1;
        }
        else if (isstring(s))
        {
            seps = (unsigned char *)s->s_chars;
            nseps = s->s_nchars;
        }
        else
        {
            if (loose_it)
                f->decref();
            return argerror(1);
        }
        if (NARGS() > 2)
        {
            if (!isstring(ARG(2)))
            {
                if (loose_it)
                    f->decref();
                return argerror(2);
            }
            terms = (unsigned char *)stringof(ARG(2))->s_chars;
            nterms = stringof(ARG(2))->s_nchars;
            if (NARGS() > 3)
            {
                if (!isstring(ARG(3)))
                {
                    if (loose_it)
                        f->decref();
                    return argerror(3);
                }
                delims = (unsigned char *)stringof(ARG(3))->s_chars;
                ndelims = stringof(ARG(3))->s_nchars;
            }
        }
        break;

    default:
        return argcount(4);
    }

#define S_IDLE  0
#define S_INTOK 1

#define W_EOF   0
#define W_SEP   1
#define W_TERM  2
#define W_TOK   3
#define W_DELIM 4

    state = S_IDLE;
    if ((a = new_array(0)) == NULL)
        goto fail;
    for (;;)
    {
        /*
         * Get the next character and classify it.
         */
        if ((c = f->getch()) == EOF)
        {
            what = W_EOF;
            goto got_what;
        }
        for (i = 0; i < nseps; ++i)
        {
            if (c == seps[i])
            {
                what = W_SEP;
                goto got_what;
            }
        }
        for (i = 0; i < nterms; ++i)
        {
            if (c == terms[i])
            {
                what = W_TERM;
                goto got_what;
            }
        }
        for (i = 0; i < ndelims; ++i)
        {
            if (c == delims[i])
            {
                what = W_DELIM;
                goto got_what;
            }
        }
        what = W_TOK;
    got_what:

        /*
         * Act on state and current character classification.
         */
        switch ((state << 8) + what)
        {
        case (S_IDLE << 8) + W_EOF:
            if (loose_it)
                f->decref();
            if (a->a_top == a->a_base)
            {
                a->decref();
                return null_ret();
            }
            return ret_with_decref(a);

        case (S_IDLE << 8) + W_TERM:
            if (!hardsep)
            {
                if (loose_it)
                    f->decref();
                return ret_with_decref(a);
            }
            j = 0;
        case (S_INTOK << 8) + W_EOF:
        case (S_INTOK << 8) + W_TERM:
            if (a->stk_push_chk())
                goto fail;
            if ((s = new_str(buf, j)) == NULL)
                goto fail;
            *a->a_top++ = s;
            if (loose_it)
                f->decref();
            s->decref();
            return ret_with_decref(a);

        case (S_IDLE << 8) + W_SEP:
            if (!hardsep)
                break;
            j = 0;
        case (S_INTOK << 8) + W_SEP:
            if (a->stk_push_chk())
                goto fail;
            if ((s = new_str(buf, j)) == NULL)
                goto fail;
            *a->a_top++ = s;
            s->decref();
            if (hardsep)
            {
                j = 0;
                state = S_INTOK;
            }
            else
                state = S_IDLE;
            break;

        case (S_INTOK << 8) + W_DELIM:
            if (a->stk_push_chk())
                goto fail;
            if ((s = new_str(buf, j)) == NULL)
                goto fail;
            *a->a_top++ = s;
            s->decref();
        case (S_IDLE << 8) + W_DELIM:
            if (a->stk_push_chk())
                goto fail;
            buf[0] = c;
            if ((s = new_str(buf, 1)) == NULL)
                goto fail;
            *a->a_top++ = s;
            s->decref();
            j = 0;
            state = S_IDLE;
            break;

        case (S_IDLE << 8) + W_TOK:
            j = 0;
            state = S_INTOK;
        case (S_INTOK << 8) + W_TOK:
            if (chkbuf(j))
                goto fail;
            buf[j++] = c;
        }
    }

fail:
    if (loose_it)
        f->decref();
    if (a != NULL)
        a->decref();
    return 1;
}


/*
 * sort(array, cmp)
 */
static int
f_sort()
{
    array *a;
    object   **base;
    long        n;
    object   *f;
    long        cmp;
    long        k;                              /* element added or removed */
    long        p;                              /* place in heap */
    long        q;                              /* place in heap */
    long        l;                              /* left child */
    long        r;                              /* right child */
    object   *o;                             /* object used for swapping */
    object   *uarg;                          /* user argument to cmp func */

/*
 * Relations within heap.
 */
#define PARENT(i)       (((i) - 1) >> 1)
#define LEFT(i)         ((i) + (i) + 1)
#define RIGHT(i)        ((i) + (i) + 2)
/*
 * Macro for swapping elements.
 */
#define SWAP(a, b)      {o = base[a]; base[a] = base[b]; base[b] = o;}
#define CMP(rp, a, b)   call(f, "i=ooo", rp, base[a], base[b], uarg)

    uarg = ici_null;
    switch (NARGS())
    {
    case 3:
        if (typecheck("aoo", &a, &f, &uarg))
            return 1;
        if (!f->can_call())
            return argerror(1);
        break;

    case 2:
        if (typecheck("ao", &a, &f))
            return 1;
        if (!f->can_call())
            return argerror(1);
        break;

    case 1:
        if (typecheck("a", &a))
            return 1;
        f = ici_fetch(vs.a_top[-1], SS(cmp));
        if (!f->can_call())
        {
            return set_error("no suitable cmp function in scope");
        }
        break;

    default:
        return argcount(2);
    }
    if (a->isatom())
    {
        return set_error("attempt to sort an atomic array");
    }

    n = a->len();
    if (a->a_bot > a->a_top)
    {
        ptrdiff_t   m;
        object    **e;

        /*
         * Can't sort in-place because the array has wrapped. Force the
         * array to be contiguous. ### Maybe this should be a function
         * in array.c.
         */
        m = a->a_limit - a->a_base;
        if ((e = (object **)ici_nalloc(m * sizeof (object *))) == NULL)
            goto fail;
        a->gather(e, 0, n);
        ici_nfree(a->a_base, m * sizeof (object *));
        a->a_base = e;
        a->a_bot = e;
        a->a_top = e + n;
        a->a_limit = e + m;
    }
    base = a->a_bot;

    /*
     * Shuffle heap.
     */
    for (k = 1; k < n; ++k)
    {
        p = k;
        while (p != 0)
        {
            q = PARENT(p);
            if (CMP(&cmp, p, q))
                goto fail;
            if (cmp <= 0)
                break;
            SWAP(p, q);
            p = q;
        }
    }

    /*
     * Keep taking elements off heap and re-shuffling.
     */
    for (k = n - 1; k > 0; --k)
    {
        SWAP(0, k);
        p = 0;
        while (1)
        {
            l = LEFT(p);
            if (l >= k)
                break;
            r = RIGHT(p);
            if (r >= k)
            {
                if (CMP(&cmp, l, p))
                    goto fail;
                if (cmp <= 0)
                    break;
                SWAP(l, p);
                p = l;
            }
            else
            {
                if (CMP(&cmp, l, p))
                    goto fail;
                if (cmp <= 0)
                {
                    if (CMP(&cmp, r, p))
                        goto fail;
                    if (cmp <= 0)
                        break;
                    SWAP(r, p);
                    p = r;
                }
                else
                {
                    if (CMP(&cmp, r, l))
                        goto fail;
                    if (cmp <= 0)
                    {
                        SWAP(l, p);
                        p = l;
                    }
                    else
                    {
                        SWAP(r, p);
                        p = r;
                    }
                }
            }
        }
    }
    return ret_no_decref(a);

fail:
    return 1;

#undef  PARENT
#undef  LEFT
#undef  RIGHT
#undef  SWAP
}

static int
f_reclaim()
{
    reclaim();
    return null_ret();
}

static int
f_abs()
{
    if (isint(ARG(0)))
    {
        if (intof(ARG(0))->i_value >= 0)
            return ret_no_decref(ARG(0));
        return int_ret(-intof(ARG(0))->i_value);
    }
    else if (isfloat(ARG(0)))
    {
        if (floatof(ARG(0))->f_value >= 0)
            return ret_no_decref(ARG(0));
        return float_ret(-floatof(ARG(0))->f_value);
    }
    return argerror(0);
}

static int              got_epoch_time;
static time_t           epoch_time;

static void
get_epoch_time()
{
    struct tm   tm;

    memset(&tm, 0, sizeof tm);
    tm.tm_year = 100; /* 2000 is 100 years since 1900. */
    tm.tm_mday = 1;    /* First day of month is 1 */
    epoch_time = mktime(&tm);
    got_epoch_time = 1;
}

static int
f_now()
{
    if (!got_epoch_time)
        get_epoch_time();
    return float_ret(difftime(time(NULL), epoch_time));
}

static int
f_calendar()
{
    objwsup       *s;
    double              d;
    long                l;

    s = NULL;
    if (!got_epoch_time)
        get_epoch_time();
    if (NARGS() != 1)
        return argcount(1);
    if (isfloat(ARG(0)))
    {
        time_t          t;
        struct tm       *tm;

        /*
         * This is really bizarre. ANSI C doesn't define what the units
         * of a time_t are, just that it is an arithmetic type. difftime()
         * can be used to get seconds from time_t values, but their is no
         * inverse operation. So without discovering a conversion factor
         * from calls to mktime(), one can't usefully combine a time in,
         * say seconds, with a time_t. But we all know that time_t is
         * really in seconds. So I'll just assume that.
         */
        t = epoch_time + (time_t)floatof(ARG(0))->f_value;
        tm = localtime(&t);
        if ((s = objwsupof(new_struct())) == NULL)
            return 1;
        if
        (
               set_val(s, SS(second), 'f', (d = tm->tm_sec, &d))
            || set_val(s, SS(minute), 'i', (l = tm->tm_min, &l))
            || set_val(s, SS(hour), 'i', (l = tm->tm_hour, &l))
            || set_val(s, SS(day), 'i', (l = tm->tm_mday, &l))
            || set_val(s, SS(month), 'i', (l = tm->tm_mon, &l))
            || set_val(s, SS(year), 'i', (l = tm->tm_year + 1900, &l))
            || set_val(s, SS(wday), 'i', (l = tm->tm_wday, &l))
            || set_val(s, SS(yday), 'i', (l = tm->tm_yday, &l))
            || set_val(s, SS(isdst), 'i', (l = tm->tm_isdst, &l))
#ifdef ICI_HAS_BSD_STRUCT_TM
            || set_val(s, SS(zone), 's', (char *)tm->tm_zone)
	    || set_val(s, SS(gmtoff), 'i', &tm->tm_gmtoff)
#else
            || set_timezone_vals(structof(s))
#endif
        )
        {
            s->decref();
            return 1;
        }
        return ret_with_decref(s);
    }
    else if (isstruct(ARG(0)))
    {
        time_t          t;
        struct tm       tm;

        memset(&tm, 0, sizeof tm);
        s = objwsupof(ARG(0));
        if (fetch_num(s, SS(second), &d))
            return 1;
        tm.tm_sec = (int)d;
        if (fetch_int(s, SS(minute), &l))
            return 1;
        tm.tm_min = l;
        if (fetch_int(s, SS(hour), &l))
            return 1;
        tm.tm_hour = l;
        if (fetch_int(s, SS(day), &l))
            return 1;
        tm.tm_mday = l;
        if (fetch_int(s, SS(month), &l))
            return 1;
        tm.tm_mon = l;
        if (fetch_int(s, SS(year), &l))
            return 1;
        tm.tm_year = l - 1900;
        if (fetch_int(s, SS(isdst), &l))
            tm.tm_isdst = -1;
        else
            tm.tm_isdst = l;
        t = mktime(&tm);
        if (t == (time_t)-1)
        {
            return set_error("unsuitable calendar time");
        }
        return float_ret(difftime(t, epoch_time));
    }
    return argerror(0);
}

/*
 * ICI: sleep(num)
 */
static int
f_sleep()
{
    double              how_long;
    exec          *x;

    if (typecheck("n", &how_long))
        return 1;

#ifdef _WIN32
    {
        long            t;

        how_long *= 1000; /* Convert to milliseconds. */
        if (how_long > LONG_MAX)
            t = LONG_MAX;
        else if ((t = (long)how_long) < 1)
            t = 1;
        x = leave();
        Sleep(t);
        enter(x);
    }
#else
    {
        long            t;

        if (how_long > LONG_MAX)
            t = LONG_MAX;
        else if ((t = how_long) < 1)
            t = 1;
        x = leave();
        sleep(t);
        enter(x);
    }
#endif
    return null_ret();
}

/*
 * Return the accumulated cpu time in seconds as a float. The precision
 * is system dependent. If a float argument is provided, this forms a new
 * base cpu time from which future cputime values are relative to. Thus
 * if 0.0 is passed, future (but not this one) returns measure new CPU
 * time accumulated since this call.
 */
static int
f_cputime()
{
    static double       base;
    double              t;

#ifdef  _WIN32
    FILETIME            c;
    FILETIME            e;
    FILETIME            k;
    FILETIME            user;

    if (!GetProcessTimes(GetCurrentProcess(), &c, &e, &k, &user))
        return ici_get_last_win32_error();
    t = (user.dwLowDateTime + user.dwHighDateTime * 4294967296.0) / 1e7;

#else
# if defined(__linux__) || defined(BSD) || defined(__sun)
    struct rusage rusage;

    getrusage(RUSAGE_SELF, &rusage);
    t = rusage.ru_utime.tv_sec + (rusage.ru_utime.tv_usec / 1.0e6);

#else /* BSD */
    return set_error("cputime function not available on this platform");
# endif
#endif
    t -= base;
    if (NARGS() > 0 && isfloat(ARG(0)))
        base = floatof(ARG(0))->f_value + t;
    return float_ret(t);
}

str *ver_cache;

static int
f_version()
{

    if
    (
        ver_cache == NULL
        &&
        (ver_cache = new_str_nul_term(version_string)) == NULL
    )
        return 1;
    return ret_no_decref(ver_cache);
}

static int
f_strbuf()
{
    str *s;
    str *is;
    int  n;

    is = NULL;
    n = 10;
    if (NARGS() > 0)
    {
        if (!isstring(ARG(0)))
            return argerror(0);
        is = stringof(ARG(0));
        n = is->s_nchars;
    }
    if ((s = new_str_buf(n)) == NULL)
    {
        return 1;
    }
    if (is != NULL)
    {
        memcpy(s->s_chars, is->s_chars, n);
        s->s_nchars = n;
    }
    return ret_with_decref(s);
}

static int
f_strcat()
{
    str           *s1;
    str           *s2;
    int                 n;
    int                 i;
    int                 si;
    int                 z;
    int64_t             sz;


    if (NARGS() < 2)
        return argcount(2);
    if (!isstring(ARG(0)))
        return argerror(0);
    s1 = stringof(ARG(0));
    if (isint(ARG(1)))
    {
        si = 2;
        sz = intof(ARG(1))->i_value;
        if (sz < 0 || sz > int64_t(s1->s_nchars))
            return argerror(1);
    }
    else
    {
        si = 1;
        sz = s1->s_nchars;
    }
    n = NARGS();
    for (i = si, z = sz; i < n; ++i)
    {
        s2 = stringof(ARG(i));
        if (!isstring(s2))
            return argerror(i);
        z += s2->s_nchars;
    }
    if (str_need_size(s1, z))
        return 1;
    for (i = si, z = sz; i < n; ++i)
    {
        s2 = stringof(ARG(i));
        memcpy(&s1->s_chars[z], s2->s_chars, s2->s_nchars);
        z += s2->s_nchars;
    }
    if (s1->s_nchars < size_t(z))
        s1->s_nchars = z;
    s1->s_chars[s1->s_nchars] = '\0';
    return ret_no_decref(s1);
}

static int
f_which()
{
    objwsup       *s;
    object           *k;

    s = NULL;
    if (typecheck(NARGS() < 2 ? "o" : "oo", &k, &s))
        return 1;
    if (s == NULL)
        s = objwsupof(vs.a_top[-1]);
    else if (!hassuper(s))
        return argerror(0);
    while (s != NULL)
    {
        if (isstruct(s))
        {
            if (find_raw_slot(structof(s), k)->sl_key == k)
	    {
                return ret_no_decref(s);
	    }
        }
        else
        {
            objwsup *t;
            object  *v;
            int      r;

            t = s->o_super;
            s->o_super = NULL;
            r = ici_fetch_super(s, k, &v, NULL);
            s->o_super = t;
            switch (r)
            {
            case -1: return 1;
            case  1: return ret_no_decref(s);
            }
        }
        s = s->o_super;
    }
    return null_ret();
}

static int
f_ncollects()
{
    return int_ret(ncollects);
}

/*
 * Cleans up data structures allocated/referenced in this module.
 * Required for a clean shutdown.
 */
void
uninit_cfunc()
{
}

static int
f_getchar()
{
    file          *f;
    int                 c;
    exec          *x = NULL;

    if (NARGS() != 0)
    {
        if (typecheck("u", &f))
	{
            return 1;
	}
    }
    else
    {
        if ((f = need_stdin()) == NULL)
	{
            return 1;
	}
    }
    blocking_syscall(1);
    if (f->flagged(FT_NOMUTEX))
        x = leave();
    c = f->getch();
    if (f->flagged(FT_NOMUTEX))
        enter(x);
    blocking_syscall(0);
    if (c == EOF)
    {
        if ((FILE *)f->f_file == stdin)
	{
            clearerr(stdin);
	}
        return null_ret();
    }
    buf[0] = c;
    return ret_with_decref(new_str(buf, 1));
}

static int
f_ungetchar()
{
    file  *f;
    char        *ch;

    if (NARGS() != 1)
    {
        if (typecheck("su", &ch, &f))
            return 1;
    }
    else
    {
        if ((f = need_stdin()) == NULL)
	{
            return 1;
	}
        if (typecheck("s", &ch))
	{
            return 1;
	}
    }
    if (f->ungetch(*ch) == EOF)
    {
        return set_error("unable to unget character");
    }
    return str_ret(ch);
}

static int
f_getline()
{
    int        i;
    int        c;
    file          *f;
    exec          *x = NULL;
    char                *b;
    int                 buf_size;
    str           *str;

    x = NULL;
    if (NARGS() != 0)
    {
        if (typecheck("u", &f))
            return 1;
    }
    else
    {
        if ((f = need_stdin()) == NULL)
            return 1;
    }
    if ((b = (char *)malloc(buf_size = 128)) == NULL)
        goto nomem;
    if (f->flagged(FT_NOMUTEX))
    {
        blocking_syscall(1);
        x = leave();
    }
    for (i = 0; (c = f->getch()) != '\n' && c != EOF; ++i)
    {
        if (i == buf_size && (b = (char *)realloc(b, buf_size *= 2)) == NULL)
            break;
        b[i] = c;
    }
    if (f->flagged(FT_NOMUTEX))
    {
        enter(x);
        blocking_syscall(0);
    }
    if (b == NULL)
        goto nomem;
    if (i == 0 && c == EOF)
    {
        free(b);
        if ((FILE *)f->f_file == stdin)
            clearerr(stdin);
        return null_ret();
    }
    str = new_str(b, i);
    free(b);
    if (str == NULL)
        return 1;
    return ret_with_decref(str);

nomem:
    return set_error("ran out of memory");
}

static int
f_getfile()
{
    int                 i;
    int                 c;
    file          *f;
    exec          *x = NULL;
    char                *b;
    int                 buf_size;
    str           *str;
    int                 must_close;
    
    must_close = 0;
    str = NULL; /* Pessimistic. */
    if (NARGS() != 0)
    {
        if (isstring(ARG(0)))
        {
            if (call(SS(fopen), "o=o", &f, ARG(0)))
                goto finish;
            must_close = 1;
        }
        else
            f = fileof(ARG(0));
        if (!isfile(f))
        {
            char    n1[objnamez];
            set_error("getfile() given %s instead of a file", objname(n1, f));
            goto finish;
        }
    }
    else
    {
        if ((f = need_stdin()) == NULL)
            goto finish;
    }
    if ((b = (char *)malloc(buf_size = 128)) == NULL)
        goto nomem;
    if (f->flagged(FT_NOMUTEX))
    {
        blocking_syscall(1);
        x = leave();
    }
    for (i = 0; (c = f->getch()) != EOF; ++i)
    {
        if (i == buf_size && (b = (char *)realloc(b, buf_size *= 2)) == NULL)
            break;
        b[i] = c;
    }
    if (f->flagged(FT_NOMUTEX))
    {
        enter(x);
        blocking_syscall(0);
    }
    if (b == NULL)
        goto nomem;
    str = new_str(b, i);
    free(b);
    goto finish;

nomem:
    set_error("ran out of memory");

finish:
    if (must_close)
    {
        call(SS(close), "o", f);
        f->decref();
    }
    return ret_with_decref(str);
}

static int
f_tmpname()
{
    char nametemplate[] = "/tmp/ici.XXXXXX";
    int fd = mkstemp(nametemplate);
    if (fd == -1)
    {
	return get_last_errno("mkstemp", NULL);
    }
    close(fd);
    return str_ret(nametemplate);
}

static int
f_puts()
{
    str  *s;
    file *f;
    exec *x = NULL;

    if (NARGS() > 1)
    {
        if (typecheck("ou", &s, &f))
            return 1;
    }
    else
    {
        if (typecheck("o", &s))
            return 1;
        if ((f = need_stdout()) == NULL)
            return 1;
    }
    if (!isstring(s))
        return argerror(0);
    if (f->flagged(FT_NOMUTEX))
        x = leave();
    if (f->write(s->s_chars, s->s_nchars) != int(s->s_nchars))
    {
        if (f->flagged(FT_NOMUTEX))
            enter(x);
        return set_error("write failed");
    }
    if (f->flagged(FT_NOMUTEX))
        enter(x);
    return null_ret();
}

static int
f_fflush()
{
    file          *f;
    exec          *x = NULL;

    if (NARGS() > 0)
    {
        if (typecheck("u", &f))
            return 1;
    }
    else
    {
        if ((f = need_stdout()) == NULL)
            return 1;
    }
    if (f->flagged(FT_NOMUTEX))
        x = leave();
    if (f->flush() == -1)
    {
        if (f->flagged(FT_NOMUTEX))
            enter(x);
        return set_error("flush failed");
    }
    if (f->flagged(FT_NOMUTEX))
        enter(x);
    return null_ret();
}

static int
f_fopen()
{
    const char  *name;
    const char  *mode;
    file  *f;
    FILE        *stream;
    exec  *x = NULL;
    int         i;

    mode = "r";
    if (typecheck(NARGS() > 1 ? "ss" : "s", &name, &mode))
        return 1;
    x = leave();
    blocking_syscall(1);
    stream = fopen(name, mode);
    blocking_syscall(0);
    if (stream == NULL)
    {
        i = errno;
        enter(x);
        errno = i;
        return get_last_errno("open", name);
    }
    enter(x);
    if ((f = new_file((char *)stream, stdio_ftype, stringof(ARG(0)), NULL)) == NULL)
    {
        fclose(stream);
        return 1;
    }
    return ret_with_decref(f);
}

static int
f_fseek()
{
    file  *f;
    long        offset;
    long        whence;

    if (typecheck("uii", &f, &offset, &whence))
    {
	if (typecheck("ui", &f, &offset))
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
        return set_error("invalid whence value in seek()");
    }
    if ((offset = f->seek(offset, (int)whence)) == -1)
        return 1;
    return int_ret(offset);
}

static int
f_popen()
{
    const char  *name;
    const char  *mode;
    file  *f;
    FILE        *stream;
    exec  *x = NULL;
    int         i;

    mode = "r";
    if (typecheck(NARGS() > 1 ? "ss" : "s", &name, &mode))
        return 1;
    x = leave();
    if ((stream = popen(name, mode)) == NULL)
    {
        i = errno;
        enter(x);
        errno = i;
        return get_last_errno("popen", name);
    }
    enter(x);
    if ((f = new_file((char *)stream, popen_ftype, stringof(ARG(0)), NULL)) == NULL)
    {
        pclose(stream);
        return 1;
    }
    return ret_with_decref(f);
}

static int
f_system()
{
    char        *cmd;
    long        result;
    exec  *x = NULL;

    if (typecheck("s", &cmd))
        return 1;
    x = leave();
    result = system(cmd);
    enter(x);
    return int_ret(result);
}

static int
f_fclose()
{
    file  *f;

    if (typecheck("u", &f))
        return 1;
    if (close_file(f))
        return 1;
    return null_ret();
}

static int
f_eof()
{
    file          *f;
    exec          *x = NULL;
    int                 r;

    if (NARGS() != 0)
    {
        if (typecheck("u", &f))
            return 1;
    }
    else
    {
        if ((f = need_stdin()) == NULL)
            return 1;
    }
    if (f->flagged(FT_NOMUTEX))
        x = leave();
    r = f->eof();
    if (f->flagged(FT_NOMUTEX))
        enter(x);
    return int_ret((long)r);
}

static int
f_remove()
{
    char        *s;

    if (typecheck("s", &s))
        return 1;
    if (remove(s) != 0)
        return get_last_errno("remove", s);
    return null_ret();
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
f_dir()
{
    const char          *path   = ".";
    const char          *format = "f";
    regexp        *pattern = NULL;
    object           *o;
    array         *a;
    DIR                 *dir;
    struct dirent       *dirent;
    int                 fmt;
    str           *s;

    switch (NARGS())
    {
    case 0:
        break;

    case 1:
        o = ARG(0);
        if (isstring(o))
            path = stringof(o)->s_chars;
        else if (isnull(o))
            ;   /* leave path as is */
        else if (isregexp(o))
            pattern = regexpof(o);
        else
            return argerror(0);
        break;

    case 2:
        o = ARG(0);
        if (isstring(o))
            path = stringof(o)->s_chars;
        else if (isnull(o))
            ;   /* leave path as is */
        else if (isregexp(o))
            pattern = regexpof(o);
        else
            return argerror(0);
        o = ARG(1);
        if (isregexp(o))
        {
            if (pattern != NULL)
                return argerror(1);
            pattern = regexpof(o);
        }
        else if (isstring(o))
            format = stringof(o)->s_chars;
        else
            return argerror(1);
        break;

    case 3:
        o = ARG(0);
        if (isstring(o))
            path = stringof(o)->s_chars;
        else if (isnull(o))
            ;   /* leave path as is */
        else
            return argerror(0);
        o = ARG(1);
        if (!isregexp(o))
            return argerror(1);
        pattern = regexpof(o);
        o = ARG(2);
        if (!isstring(o))
            return argerror(2);
        format = stringof(o)->s_chars;
        break;

    default:
        return argcount(3);
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
            return set_error("bad directory format specifier");
        }
    }
    if ((a = new_array(0)) == NULL)
        return 1;
    if ((dir = opendir(path)) == NULL)
    {
        get_last_errno("open directory", path);
        goto fail;
    }
    while ((dirent = readdir(dir)) != NULL)
    {
        struct stat     statbuf;
        char            abspath[MAXPATHLEN+1];

        if
        (
            pattern != NULL
            &&
            pcre_exec
            (
                pattern->r_re,
                pattern->r_rex,
                dirent->d_name,
                strlen(dirent->d_name),
                0,
                0,
                re_bra,
                nels(re_bra)
            )
            < 0
        )
            continue;
        sprintf(abspath, "%s/%s", path, dirent->d_name);
#ifndef _WIN32
        if (lstat(abspath, &statbuf) == -1)
        {
            get_last_errno("get stats on", abspath);
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
                (s = new_str_nul_term(dirent->d_name)) == NULL
                ||
                a->stk_push_chk()
            )
            {
                if (s != NULL)
                    s->decref();
                closedir(dir);
                goto fail;
            }
            *a->a_top++ = s;
            s->decref();
        }
    }
    closedir(dir);
    return ret_with_decref(a);

#undef  FILES
#undef  DIRS
#undef  OTHERS

fail:
    a->decref();
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
        return get_last_errno(NULL, NULL);
    return int_ret((long)ret);
}

/*
 * rename(oldpath, newpath)
 */
static int
f_rename()
{
    char                *o;
    char                *n;

    if (typecheck("ss", &o, &n))
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

    if (typecheck("s", &n))
        return 1;
    return sys_ret(chdir(n));
}

/*
 * string = getcwd()
 */
static int
f_getcwd()
{
    char        buf[MAXPATHLEN+1];

    if (getcwd(buf, sizeof buf) == NULL)
        return sys_ret(-1);
    return str_ret(buf);
}

/*
 * Return the value of an environment variable.
 */
static int
f_getenv()
{
    str           *n;
    char          **p;

    if (NARGS() != 1)
        return argcount(1);
    if (!isstring(ARG(0)))
        return argerror(0);
    n = stringof(ARG(0));

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
            return str_ret(&(*p)[n->s_nchars + 1]);
        }
    }
    return null_ret();
}

/*
 * Set an environment variable.
 */
static int
f_putenv()
{
    char        *s;
    char        *t;
    char        *e;
    char        *f;
    int         i;

    if (typecheck("s", &s))
        return 1;
    if ((e = strchr(s, '=')) == NULL)
    {
        return set_error("putenv argument not in form \"name=value\"");
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
        return set_error("ran out of memmory");
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
    return null_ret();
}

namespace {
    // non-overloaded trampolines to force correct function type
    double xsin(double a) { return sin(a); }
    double xcos(double a) { return cos(a); }
    double xtan(double a) { return tan(a); }
    double xasin(double a) { return asin(a); }
    double xacos(double a) { return acos(a); }
    double xatan(double a) { return atan(a); }
    double xatan2(double a, double b) { return atan2(a, b); }
    double xexp(double a) { return exp(a); }
    double xlog(double a) { return log(a); }
    double xlog10(double a) { return log10(a); }
    double xpow(double a, double b) { return pow(a, b); }
    double xsqrt(double a) { return sqrt(a); }
    double xfloor(double a) { return floor(a); }
    double xceil(double a) { return ceil(a); }
    double xfmod(double a, double b) { return fmod(a, b); }
}

ICI_DEFINE_CFUNCS(std)
{
    ICI_DEFINE_CFUNC(array,        f_array),
    ICI_DEFINE_CFUNC(copy,         f_copy),
    ICI_DEFINE_CFUNC(exit,         f_exit),
    ICI_DEFINE_CFUNC(fail,         f_fail),
    ICI_DEFINE_CFUNC(float,        f_float),
    ICI_DEFINE_CFUNC(int,          f_int),
    ICI_DEFINE_CFUNC(eq,           f_eq),
    ICI_DEFINE_CFUNC(parse,        f_parse),
    ICI_DEFINE_CFUNC(string,       f_string),
    ICI_DEFINE_CFUNC(struct,       f_struct),
    ICI_DEFINE_CFUNC(set,          f_set),
    ICI_DEFINE_CFUNC(typeof,       f_typeof),
    ICI_DEFINE_CFUNC(push,         f_push),
    ICI_DEFINE_CFUNC(pop,          f_pop),
    ICI_DEFINE_CFUNC(rpush,        f_rpush),
    ICI_DEFINE_CFUNC(rpop,         f_rpop),
    ICI_DEFINE_CFUNC(call,         f_call),
    ICI_DEFINE_CFUNC(keys,         f_keys),
    ICI_DEFINE_CFUNC(vstack,       f_vstack),
    ICI_DEFINE_CFUNC(tochar,       f_tochar),
    ICI_DEFINE_CFUNC(toint,        f_toint),
    ICI_DEFINE_CFUNC(rand,         f_rand),
    ICI_DEFINE_CFUNC(interval,     f_interval),
    ICI_DEFINE_CFUNC(explode,      f_explode),
    ICI_DEFINE_CFUNC(implode,      f_implode),
    ICI_DEFINE_CFUNC(sopen,        f_sopen),
    ICI_DEFINE_CFUNC(mopen,        f_mopen),
    ICI_DEFINE_CFUNC(sprintf,      ici_f_sprintf),
    ICI_DEFINE_CFUNC(currentfile,  f_currentfile),
    ICI_DEFINE_CFUNC(del,          f_del),
    ICI_DEFINE_CFUNC(alloc,        f_alloc),
    ICI_DEFINE_CFUNC(mem,          f_mem),
    ICI_DEFINE_CFUNC(len,          f_nels),
    ICI_DEFINE_CFUNC(super,        f_super),
    ICI_DEFINE_CFUNC(scope,        f_scope),
    ICI_DEFINE_CFUNC(isatom,       f_isatom),
    ICI_DEFINE_CFUNC(gettoken,     f_gettoken),
    ICI_DEFINE_CFUNC(gettokens,    f_gettokens),
    ICI_DEFINE_CFUNC(num,          f_num),
    ICI_DEFINE_CFUNC(assign,       f_assign),
    ICI_DEFINE_CFUNC(fetch,        f_fetch),
    ICI_DEFINE_CFUNC(abs,          f_abs),
    ICI_DEFINE_CFUNC2(sin,         f_math, xsin,    "f=n"),
    ICI_DEFINE_CFUNC2(cos,         f_math, xcos,    "f=n"),
    ICI_DEFINE_CFUNC2(tan,         f_math, xtan,    "f=n"),
    ICI_DEFINE_CFUNC2(asin,        f_math, xasin,   "f=n"),
    ICI_DEFINE_CFUNC2(acos,        f_math, xacos,   "f=n"),
    ICI_DEFINE_CFUNC2(atan,        f_math, xatan,   "f=n"),
    ICI_DEFINE_CFUNC2(atan2,       f_math, xatan2,  "f=nn"),
    ICI_DEFINE_CFUNC2(exp,         f_math, xexp,    "f=n"),
    ICI_DEFINE_CFUNC2(log,         f_math, xlog,    "f=n"),
    ICI_DEFINE_CFUNC2(log10,       f_math, xlog10,  "f=n"),
    ICI_DEFINE_CFUNC2(pow,         f_math, xpow,    "f=nn"),
    ICI_DEFINE_CFUNC2(sqrt,        f_math, xsqrt,   "f=n"),
    ICI_DEFINE_CFUNC2(floor,       f_math, xfloor,  "f=n"),
    ICI_DEFINE_CFUNC2(ceil,        f_math, xceil,   "f=n"),
    ICI_DEFINE_CFUNC2(fmod,        f_math, xfmod,   "f=nn"),
    ICI_DEFINE_CFUNC(waitfor,      f_waitfor),
    ICI_DEFINE_CFUNC(top,          f_top),
    ICI_DEFINE_CFUNC(include,      f_include),
    ICI_DEFINE_CFUNC(sort,         f_sort),
    ICI_DEFINE_CFUNC(reclaim,      f_reclaim),
    ICI_DEFINE_CFUNC(now,          f_now),
    ICI_DEFINE_CFUNC(calendar,     f_calendar),
    ICI_DEFINE_CFUNC(cputime,      f_cputime),
    ICI_DEFINE_CFUNC(version,      f_version),
    ICI_DEFINE_CFUNC(sleep,        f_sleep),
    ICI_DEFINE_CFUNC(strbuf,       f_strbuf),
    ICI_DEFINE_CFUNC(strcat,       f_strcat),
    ICI_DEFINE_CFUNC(which,        f_which),
    ICI_DEFINE_CFUNC(ncollects,    f_ncollects),
    ICI_DEFINE_CFUNC2(cmp,         f_coreici, SS(cmp),       SS(core1)),
    ICI_DEFINE_CFUNC2(pathjoin,    f_coreici, SS(pathjoin),  SS(core2)),
    ICI_DEFINE_CFUNC2(basename,    f_coreici, SS(basename),  SS(core2)),
    ICI_DEFINE_CFUNC2(dirname,     f_coreici, SS(dirname),   SS(core2)),
    ICI_DEFINE_CFUNC2(pfopen,      f_coreici, SS(pfopen),    SS(core2)),
    ICI_DEFINE_CFUNC2(use,         f_coreici, SS(use),       SS(core2)),
    ICI_DEFINE_CFUNC2(walk,        f_coreici, SS(walk),      SS(core2)),
    ICI_DEFINE_CFUNC2(min,         f_coreici, SS(min),       SS(core3)),
    ICI_DEFINE_CFUNC2(max,         f_coreici, SS(max),       SS(core3)),
    ICI_DEFINE_CFUNC2(argerror,    f_coreici, SS(argerror),  SS(core3)),
    ICI_DEFINE_CFUNC2(argcount,    f_coreici, SS(argcount),  SS(core3)),
    ICI_DEFINE_CFUNC2(typecheck,   f_coreici, SS(typecheck), SS(core3)),
    ICI_DEFINE_CFUNC2(apply,       f_coreici, SS(apply),     SS(core4)),
    ICI_DEFINE_CFUNC2(map,         f_coreici, SS(map),       SS(core4)),
    ICI_DEFINE_CFUNC2(deepatom,    f_coreici, SS(deepatom),  SS(core5)),
    ICI_DEFINE_CFUNC2(deepcopy,    f_coreici, SS(deepcopy),  SS(core5)),
    ICI_DEFINE_CFUNC2(memoize,     f_coreici, SS(memoize),   SS(core6)),
    ICI_DEFINE_CFUNC2(memoized,    f_coreici, SS(memoized),  SS(core6)),
    ICI_DEFINE_CFUNC2(print,       f_coreici, SS(print),     SS(core7)),
    ICI_DEFINE_CFUNC1(printf,      ici_f_sprintf, 1),
    ICI_DEFINE_CFUNC(getchar,   f_getchar),
    ICI_DEFINE_CFUNC(ungetchar, f_ungetchar),
    ICI_DEFINE_CFUNC(getfile,   f_getfile),
    ICI_DEFINE_CFUNC(getline,   f_getline),
    ICI_DEFINE_CFUNC(fopen,     f_fopen),
    ICI_DEFINE_CFUNC(_popen,    f_popen),
    ICI_DEFINE_CFUNC(tmpname,   f_tmpname),
    ICI_DEFINE_CFUNC(puts,      f_puts),
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
    ICI_CFUNCS_END()
};

} // namespace ici
