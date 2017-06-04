#define ICI_CORE
#include "buf.h"
#include "exec.h"
#include "func.h"
#include "int.h"
#include "float.h"
#include "str.h"
#include "null.h"
#include "op.h"
#include "catch.h"

/*
 * This function is a variation on 'ici_func()'. See that function for details
 * on the meaning of the 'types' argument.
 *
 * 'va' is a va_list (variable argument list) passed from an outer var-args
 * function.
 *
 * If 'subject' is NULL, then 'callable' is taken to be a callable object
 * (could be a function, a method, or something else) and is called directly.
 * If 'subject' is non-NULL, it is taken to be an instance object and 
 * 'callable' should be the name of one of its methods (i.e. an 'ici_str_t *').
 *
 * This --func-- forms part of the --ici-api--.
 */
int
ici_funcv(ici_obj_t *subject, ici_obj_t *callable, const char *types, va_list va)
{
    int                 nargs;
    int                 arg;
    ici_obj_t           *member_obj;
    ici_obj_t           *ret_obj;
    char                ret_type;
    char                *ret_ptr;
    ptrdiff_t           os_depth;
    ici_op_t            *call_op;

    if (types[0] != '\0' && types[1] == '@')
    {
        member_obj = va_arg(va, ici_obj_t *);
        types += 2;
    }
    else
    {
        member_obj = NULL;
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
        ret_ptr = NULL;
    }

    os_depth = ici_os.a_top - ici_os.a_base;
    /*
     * We include an extra 80 in our ici_stk_push_chk, see start of evaluate().
     */
    nargs = strlen(types);
    if (ici_stk_push_chk(&ici_os, nargs + 80))
    {
        return 1;
    }
    for (arg = 0; arg < nargs; ++arg)
    {
        *ici_os.a_top++ = ici_null;
    }
    for (arg = -1; arg >= -nargs; --arg)
    {
        switch (*types++)
        {
        case 'o':
            ici_os.a_top[arg] = va_arg(va, ici_obj_t *);
            break;

        case 'i':
            if ((ici_os.a_top[arg] = ici_objof(ici_int_new(va_arg(va, long)))) == NULL)
            {
                goto fail;
            }
            ici_decref(ici_os.a_top[arg]);
            break;

        case 'q':
            ici_os.a_top[arg] = ici_objof(&ici_o_quote);
            --nargs;
            break;

        case 's':
            if ((ici_os.a_top[arg] = ici_objof(ici_str_new_nul_term(va_arg(va, char *)))) == NULL)
            {
                goto fail;
            }
            ici_decref(ici_os.a_top[arg]);
            break;

        case 'f':
            if ((ici_os.a_top[arg] = ici_objof(ici_float_new(va_arg(va, double)))) == NULL)
            {
                goto fail;
            }
            ici_decref(ici_os.a_top[arg]);
            break;

        default:
            ici_set_error("error in function call");
            goto fail;
        }
    }
    if (member_obj != NULL)
    {
        *ici_os.a_top++ = member_obj;
        nargs++;
    }
    /*
     * Push the number of actual args, followed by the function
     * itself onto the operand stack.
     */
    if ((*ici_os.a_top = ici_objof(ici_int_new(nargs))) == NULL)
    {
        goto fail;
    }
    ici_decref(*ici_os.a_top);
    ++ici_os.a_top;
    if (subject != NULL)
    {
        *ici_os.a_top++ = subject;
    }
    *ici_os.a_top++ = callable;

    os_depth = (ici_os.a_top - ici_os.a_base) - os_depth;
    call_op = subject != NULL ? &ici_o_method_call : &ici_o_call;
    if ((ret_obj = ici_evaluate(ici_objof(call_op), os_depth)) == NULL)
    {
        goto fail;
    }

    switch (ret_type)
    {
    case '\0':
        ici_decref(ret_obj);
        break;

    case 'o':
        *(ici_obj_t **)ret_ptr = ret_obj;
        break;

    case 'i':
        if (!ici_isint(ret_obj))
        {
            goto typeclash;
        }
        *(long *)ret_ptr = ici_intof(ret_obj)->i_value;
        ici_decref(ret_obj);
        break;

    case 'f':
        if (!ici_isfloat(ret_obj))
        {
            goto typeclash;
        }
        *(double *)ret_ptr = ici_floatof(ret_obj)->f_value;
        ici_decref(ret_obj);
        break;

    case 's':
        if (!ici_isstring(ret_obj))
        {
            goto typeclash;
        }
        *(char **)ret_ptr = ici_stringof(ret_obj)->s_chars;
        ici_decref(ret_obj);
        break;

    default:
    typeclash:
        ici_decref(ret_obj);
        ici_set_error("incorrect return type");
        goto fail;
    }
    return 0;

fail:
    return 1;
}

/*
 * Varient of 'ici_call()' (see) taking a variable argument list.
 *
 * There is some historical support for '@' operators, but it is deprecated
 * and may be removed in future versions.
 *
 * This --func-- forms part of the --ici-api--.
 */
int
ici_callv(ici_str_t *func_name, const char *types, va_list va)
{
    ici_obj_t           *func_obj;
    ici_obj_t           *member_obj;

    func_obj = NULL;
    if (types[0] != '\0' && types[1] == '@')
    {
        va_list tmp;
        va_copy(tmp, va);
        member_obj = va_arg(tmp, ici_obj_t *);
        if ((func_obj = ici_fetch(member_obj, func_name)) == ici_null)
        {
            return ici_set_error("\"%s\" undefined in object", func_name->s_chars);
        }
        va_end(tmp);
    }
    else if ((func_obj = ici_fetch(ici_vs.a_top[-1], func_name)) == ici_null)
    {
        return ici_set_error("\"%s\" undefined", func_name->s_chars);
    }
    return ici_funcv(NULL, func_obj, types, va);
}

/*
 * Call a callable ICI object 'callable' from C with simple argument
 * marshalling and an optional return value.  The callable object is typically
 * a function (but not a function name, see 'ici_call' for that case).
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
 * Returns 0 on success, else 1, in which case ici_error has been set.
 *
 * See also: ici_callv(), ici_method(), ici_call(), ici_funcv().
 *
 * This --func-- forms part of the --ici-api--.
 */
int
ici_func(ici_obj_t *callable, const char *types, ...)
{
    va_list     va;
    int         result;

    va_start(va, types);
    result = ici_funcv(NULL, callable, types, va);
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
int
ici_method(ici_obj_t *inst, ici_str_t *mname, const char *types, ...)
{
    va_list     va;
    int         result;

    va_start(va, types);
    result = ici_funcv(inst, ici_objof(mname), types, va);
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
int
ici_call(ici_str_t *func_name, const char *types, ...)
{
    ici_obj_t           *func_obj;
    ici_obj_t           *member_obj;
    va_list             va;
    int                 result;

    func_obj = NULL;
    if (types[0] != '\0' && types[1] == '@')
    {
        va_start(va, types);
        member_obj = va_arg(va, ici_obj_t *);
        if ((func_obj = ici_fetch(member_obj, func_name)) == ici_null)
        {
            return ici_set_error("\"%s\" undefined in object", func_name->s_chars);
        }
    }
    else if ((func_obj = ici_fetch(ici_vs.a_top[-1], func_name)) == ici_null)
    {
        return ici_set_error("\"%s\" undefined", func_name->s_chars);
    }
    va_start(va, types);
    result = ici_funcv(NULL, func_obj, types, va);
    va_end(va);
    return result;
}
