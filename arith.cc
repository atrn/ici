#define ICI_CORE
#include "exec.h"
#include "float.h"
#include "int.h"
#include "op.h"
#include "parse.h"
#include "ptr.h"
#include "str.h"
#include "struct.h"
#include "buf.h"
#include "primes.h"
#include "re.h"
#include "set.h"

namespace ici
{

/*
 * binop.c
 *
 * In most configurations this file only defines binop_name() which
 * merely returns the textual form of a binary operator for error messages.
 * But in configuration where the function ici_evaluate() (in exec.c) cannot
 * be compiled with the whole binary operator switch statement in-line,
 * it can be done as a seperate function here. This is only necessary
 * if the compiler can't handle such a large function. Fortunately this
 * sort of thing is becomming rare.
 */

/*
 * Return the textual form of a binary operator. This is only to assist
 * in error messages.
 */
const char *binop_name(int op)
{
    switch (op)
    {
    case t_subtype(T_ASTERIX):  return "*";
    case t_subtype(T_SLASH):    return "/";
    case t_subtype(T_PERCENT):  return "%";
    case t_subtype(T_PLUS):     return "+";
    case t_subtype(T_MINUS):    return "-";
    case t_subtype(T_GRTGRT):   return ">>";
    case t_subtype(T_LESSLESS): return "<<";
    case t_subtype(T_LESS):     return "<";
    case t_subtype(T_GRT):      return ">";
    case t_subtype(T_LESSEQ):   return "<=";
    case t_subtype(T_GRTEQ):    return ">=";
    case t_subtype(T_EQEQ):     return "==";
    case t_subtype(T_EXCLAMEQ): return "!=";
    case t_subtype(T_TILDE):    return "~";
    case t_subtype(T_EXCLAMTILDE):return "!~";
    case t_subtype(T_2TILDE)    :return "~~";
    case t_subtype(T_3TILDE)    :return "~~~";
    case t_subtype(T_AND):      return "&";
    case t_subtype(T_CARET):    return "^";
    case t_subtype(T_BAR):      return "|";
    case t_subtype(T_ANDAND):   return "&&";
    case t_subtype(T_BARBAR):   return "||";
    case t_subtype(T_COLON):    return ":";
    case t_subtype(T_QUESTION): return "?";
    case t_subtype(T_EQ):       return "=";
    case t_subtype(T_COLONEQ):  return ":=";
    case t_subtype(T_PLUSEQ):   return "+=";
    case t_subtype(T_MINUSEQ):  return "-=";
    case t_subtype(T_ASTERIXEQ):return "*=";
    case t_subtype(T_SLASHEQ):  return "/=";
    case t_subtype(T_PERCENTEQ):return "%=";
    case t_subtype(T_GRTGRTEQ): return ">>=";
    case t_subtype(T_LESSLESSEQ):return "<<=";
    case t_subtype(T_ANDEQ):    return "&=";
    case t_subtype(T_CARETEQ):  return "^=";
    case t_subtype(T_BAREQ):    return "|=";
    case t_subtype(T_2TILDEEQ)  :return "~~=";
    case t_subtype(T_LESSEQGRT):return "<=>";
    case t_subtype(T_COMMA):    return ",";
    default:                    return "binop";
    }
}

#ifdef  BINOPFUNC

#include "pcre.h"

/*
 * See the comment in binop.h.
 */
int
op_binop(object *o)
{
#include "binop.h"
    return 0;

fail:
    return 1;
}
#endif

} // namespace ici
