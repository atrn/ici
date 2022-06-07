#define ICI_CORE
#include "buf.h"
#include "catcher.h"
#include "exec.h"
#include "float.h"
#include "func.h"
#include "int.h"
#include "null.h"
#include "op.h"
#include "str.h"

namespace ici
{

/*
 * This function is a variation on 'ici_func()'. See that function for details
 * on the meaning of the 'types' argument.
 *
 * 'va' is a va_list (variable argument list) passed from an outer var-args
 * function.
 *
 * If 'subject' is nullptr, then 'callable' is taken to be a callable object
 * (could be a function, a method, or something else) and is called directly.
 * If 'subject' is non-nullptr, it is taken to be an instance object and
 * 'callable' should be the name of one of its methods (i.e. an 'str *').
 *
 * This --func-- forms part of the --ici-api--.
 */
int callv(object *subject, object *callable, const char *types, va_list va)
{
    ssize_t   nargs;
    ssize_t   arg;
    object   *member_obj;
    object   *ret_obj;
    char      ret_type;
    char     *ret_ptr;
    ptrdiff_t os_depth;
    op       *call_op;

    if (types[0] != '\0' && types[1] == '@')
    {
        member_obj = va_arg(va, object *);
        types += 2;
    }
    else
    {
        member_obj = nullptr;
    }

    if (types[0] != '\0' && types[1] == '=')
    {
        ret_type = types[0];
        ret_ptr = va_arg(va, char *);
        types += 2;
    }
    else
    {
        ret_type = '\0';
        ret_ptr = nullptr;
    }

    os_depth = os.a_top - os.a_base;
    /*
     * We include an extra 80 in our push_check, see start of evaluate().
     */
    nargs = strlen(types);
    if (os.push_check(nargs + 80))
    {
        return 1;
    }
    for (arg = 0; arg < nargs; ++arg)
    {
        os.push(null);
    }
    for (arg = -1; arg >= -nargs; --arg)
    {
        switch (*types++)
        {
        case 'o':
            os.a_top[arg] = va_arg(va, object *);
            break;

        case 'i':
            if ((os.a_top[arg] = new_int(va_arg(va, long))) == nullptr)
            {
                goto fail;
            }
            decref(os.a_top[arg]);
            break;

        case 'q':
            os.a_top[arg] = &o_quote;
            --nargs;
            break;

        case 's':
            if ((os.a_top[arg] = new_str_nul_term(va_arg(va, char *))) == nullptr)
            {
                goto fail;
            }
            decref(os.a_top[arg]);
            break;

        case 'f':
            if ((os.a_top[arg] = new_float(va_arg(va, double))) == nullptr)
            {
                goto fail;
            }
            decref(os.a_top[arg]);
            break;

        default:
            set_error("error in function call");
            goto fail;
        }
    }
    if (member_obj != nullptr)
    {
        os.push(member_obj);
        nargs++;
    }
    /*
     * Push the number of actual args, followed by the function
     * itself onto the operand stack.
     */
    {
        auto no = new_int(nargs);
        if (!no)
        {
            goto fail;
        }
        os.push(no, with_decref);
    }
    if (subject != nullptr)
    {
        os.push(subject);
    }
    os.push(callable);

    os_depth = (os.a_top - os.a_base) - os_depth;
    call_op = subject != nullptr ? &o_method_call : &o_call;
    if ((ret_obj = evaluate(call_op, os_depth)) == nullptr)
    {
        goto fail;
    }

    switch (ret_type)
    {
    case '\0':
        decref(ret_obj);
        break;

    case 'o':
        *(object **)ret_ptr = ret_obj;
        break;

    case 'i':
        if (!isint(ret_obj))
        {
            goto typeclash;
        }
        *(long *)ret_ptr = intof(ret_obj)->i_value;
        decref(ret_obj);
        break;

    case 'f':
        if (!isfloat(ret_obj))
        {
            goto typeclash;
        }
        *(double *)ret_ptr = floatof(ret_obj)->f_value;
        decref(ret_obj);
        break;

    case 's':
        if (!isstring(ret_obj))
        {
            goto typeclash;
        }
        *(char **)ret_ptr = stringof(ret_obj)->s_chars;
        decref(ret_obj);
        break;

    default:
typeclash:
        decref(ret_obj);
        set_error("incorrect return type");
        goto fail;
    }
    return 0;

fail:
    return 1;
}

/*
 * Varient of 'call()' (see) taking a variable argument list.
 *
 * There is some historical support for '@' operators, but it is deprecated
 * and may be removed in future versions.
 *
 * This --func-- forms part of the --ici-api--.
 */
int callv(str *func_name, const char *types, va_list va)
{
    object *func_obj;
    object *member_obj;

    func_obj = nullptr;
    if (types[0] != '\0' && types[1] == '@')
    {
        va_list tmp;
        va_copy(tmp, va);
        member_obj = va_arg(tmp, object *);
        if ((func_obj = ici_fetch(member_obj, func_name)) == null)
        {
            return set_error("\"%s\" undefined in object", func_name->s_chars);
        }
        va_end(tmp);
    }
    else if ((func_obj = ici_fetch(vs.a_top[-1], func_name)) == null)
    {
        return set_error("\"%s\" undefined", func_name->s_chars);
    }
    return callv((object *)nullptr, func_obj, types, va);
}

/*
 * Call a callable ICI object 'callable' from C with simple argument
 * marshalling and an optional return value.  The callable object is typically
 * a function (but not a function name, see 'call' for that case).
 *
 * 'types' is a string that indicates what C values are being supplied as
 * arguments.  It can be of the form ".=..." or "..." where the dots represent
 * type key letters as described below.  In the first case the 1st extra
 * argument is used as a pointer to store the return value through.  In the
 * second case, the return value of the ICI function is not provided.
 *
 * Type key letters are:
 *
 * i        The corresponding argument should be a C long (a pointer to a long
 *          in the case of a return value).  It will be converted to an ICI
 *          'int' and passed to the function.
 *
 * f        The corresponding argument should be a C double.  (a pointer to a
 *          double in the case of a return value).  It will be converted to an
 *          ICI 'float' and passed to the function.
 *
 * s        The corresponding argument should be a nul terminated string (a
 *          pointer to a char * in the case of a return value).  It will be
 *          converted to an ICI 'string' and passed to the function.
 *
 *          When a string is returned it is a pointer to the character data of
 *          an internal ICI string object.  It will only remain valid until
 *          the next call to any ICI function.
 *
 * o        The corresponding argument should be a pointer to an ICI object (a
 *          pointer to an object in the case of a return value).  It will be
 *          passed directly to the ICI function.
 *
 *          When an object is returned it has been ici_incref()ed (that is, it
 *          is held against garbage collection).
 *
 * Returns 0 on success, else 1, in which case error has been set.
 *
 * See also: call(), call_method(), call(), call().
 *
 * This --func-- forms part of the --ici-api--.
 */
int call(object *callable, const char *types, ...)
{
    va_list va;
    int     result;

    va_start(va, types);
    result = callv((object *)nullptr, callable, types, va);
    va_end(va);
    return result;
}

/*
 * Call the method 'mname' of the object 'inst' with simple argument
 * marshalling.
 *
 * See 'ici_func()' for an explanation of 'types'. Apart from calling
 * a method, this function behaves in the same manner as 'ici_func()'.
 *
 * This --func-- forms part of the --ici-api--.
 */
int call_method(object *inst, str *mname, const char *types, ...)
{
    va_list va;
    int     result;

    va_start(va, types);
    result = callv(inst, mname, types, va);
    va_end(va);
    return result;
}

/*
 * Call an ICI function by name from C with simple argument types and
 * return value.  The name ('func_name') is looked up in the current scope.
 *
 * See 'ici_func()' for an explanation of 'types'. Apart from taking a name,
 * rather than an ICI function object, this function behaves in the same
 * manner as 'ici_func()'.
 *
 * There is some historical support for '@' operators, but it is deprecated
 * and may be removed in future versions.
 *
 * This --func-- forms part of the --ici-api--.
 */
int call(str *func_name, const char *types, ...)
{
    object *func_obj;
    object *member_obj;
    va_list va;
    int     result;

    func_obj = nullptr;
    if (types[0] != '\0' && types[1] == '@')
    {
        va_start(va, types);
        member_obj = va_arg(va, object *);
        if ((func_obj = ici_fetch(member_obj, func_name)) == null)
        {
            return set_error("\"%s\" undefined in object", func_name->s_chars);
        }
    }
    else if ((func_obj = ici_fetch(vs.a_top[-1], func_name)) == null)
    {
        return set_error("\"%s\" undefined", func_name->s_chars);
    }
    va_start(va, types);
    result = callv((object *)nullptr, func_obj, types, va);
    va_end(va);
    return result;
}

} // namespace ici
