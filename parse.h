// -*- mode:c++ -*-

#ifndef ICI_PARSE_H
#define ICI_PARSE_H

#include "object.h"

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
struct token
{
    int t_what; /* See TM_* and T_* below. */
    union {
        int64_t t_int;
        double  t_float;
        object *t_obj;
    };
};

struct parse : object
{
    file *p_file;
    int   p_lineno; /* Diagnostic information. */
    short p_sol;    /* At first char in line. */
    short p_cr;     /* New-line caused by \r, not \n. */
    token p_got;
    token p_ungot;
    func *p_func;         /* nullptr when not within scope. */
    int   p_module_depth; /* Depth within module, 0 is file level. */
    int   p_break_depth;
    int   p_continue_depth;
};

inline parse *parseof(object *o)
{
    return o->as<parse>();
}
inline parse *parseof(void *f)
{
    return reinterpret_cast<parse *>(f);
}
inline bool isparse(object *o)
{
    return o->hastype(TC_PARSE);
}

/*
 * End of ici.h export. --ici.h-end--
 */

/*
 * Token numbers.  Note that the precedence and order of binary operators
 * is built into the token number; this drives the expression parser.
 */
constexpr int TM_SUBTYPE = 0x003F; /* 6 bits. */
constexpr int TM_TYPE = 0x07C0;    /* 5 bits. */
constexpr int TM_PREC = 0x7800;    /* 4 bits, 0 is high (tight bind).*/
constexpr int TM_HASOBJ = 0x8000;  /* Implies incref on t_obj. */

constexpr int t_subtype(int t)
{
    return t & TM_SUBTYPE;
}
constexpr int t_type(int t)
{
    return t & TM_TYPE;
}
constexpr int t_prec(int t)
{
    return (t & TM_PREC) >> 11;
}
constexpr int TYPE(int n)
{
    return n << 6;
}
constexpr int PREC(int n)
{
    return n << 11;
}

constexpr int T_NONE = TYPE(0);
constexpr int T_NAME = (TM_HASOBJ | TYPE(1));
constexpr int T_REGEXP = (TM_HASOBJ | TYPE(2));
constexpr int T_STRING = (TM_HASOBJ | TYPE(3));
constexpr int T_SEMICOLON = TYPE(4);
constexpr int T_EOF = TYPE(5);
constexpr int T_INT = TYPE(6);
constexpr int T_FLOAT = TYPE(7);
constexpr int T_BINOP = TYPE(8); /* Has sub-types, see below. */
constexpr int T_ERROR = TYPE(9);
constexpr int T_NULL = TYPE(10);
constexpr int T_ONROUND = TYPE(11);
constexpr int T_OFFROUND = TYPE(12);
constexpr int T_ONCURLY = TYPE(13);
constexpr int T_OFFCURLY = TYPE(14);
constexpr int T_ONSQUARE = TYPE(15);
constexpr int T_OFFSQUARE = TYPE(16);
constexpr int T_DOT = TYPE(17);
constexpr int T_PTR = TYPE(18);
constexpr int T_EXCLAM = TYPE(19);
constexpr int T_PLUSPLUS = TYPE(20);
constexpr int T_MINUSMINUS = TYPE(21);
constexpr int T_CONST = TYPE(22);
constexpr int T_PRIMARYCOLON = TYPE(23);
/* constexpr int T_2COLON =        TYPE(24); */
constexpr int T_DOLLAR = TYPE(25);
constexpr int T_COLONCARET = TYPE(26);
/* constexpr int       T_OFFCURLYOFFSQ = TYPE(27); */
constexpr int T_AT = TYPE(28);
constexpr int T_BINAT = TYPE(29);
constexpr int T_GO = TYPE(30);
/* Maximum value        TYPE(31) */

/*
 * T_BINOP sub types.
 */
constexpr int T_ASTERIX = (PREC(0) | T_BINOP | 1);
constexpr int T_SLASH = (PREC(0) | T_BINOP | 2);
constexpr int T_PERCENT = (PREC(0) | T_BINOP | 3);
constexpr int T_PLUS = (PREC(1) | T_BINOP | 4);
constexpr int T_MINUS = (PREC(1) | T_BINOP | 5);
constexpr int T_GRTGRT = (PREC(2) | T_BINOP | 6);
constexpr int T_LESSLESS = (PREC(2) | T_BINOP | 7);
constexpr int T_LESS = (PREC(3) | T_BINOP | 8);
constexpr int T_GRT = (PREC(3) | T_BINOP | 9);
constexpr int T_LESSEQ = (PREC(3) | T_BINOP | 10);
constexpr int T_GRTEQ = (PREC(3) | T_BINOP | 11);
constexpr int T_EQEQ = (PREC(4) | T_BINOP | 12);
constexpr int T_EXCLAMEQ = (PREC(4) | T_BINOP | 13);
constexpr int T_TILDE = (PREC(4) | T_BINOP | 14);
constexpr int T_EXCLAMTILDE = (PREC(4) | T_BINOP | 15);
constexpr int T_2TILDE = (PREC(4) | T_BINOP | 16);
constexpr int T_3TILDE = (PREC(4) | T_BINOP | 17);
constexpr int T_AND = (PREC(5) | T_BINOP | 18);
constexpr int T_CARET = (PREC(6) | T_BINOP | 19);
constexpr int T_BAR = (PREC(7) | T_BINOP | 20);
constexpr int T_ANDAND = (PREC(8) | T_BINOP | 21);
constexpr int T_BARBAR = (PREC(9) | T_BINOP | 22);
constexpr int T_COLON = (PREC(10) | T_BINOP | 23);
constexpr int T_QUESTION = (PREC(11) | T_BINOP | 24);
constexpr int T_EQ = (PREC(12) | T_BINOP | 25);
constexpr int T_COLONEQ = (PREC(12) | T_BINOP | 26);
constexpr int T_PLUSEQ = (PREC(12) | T_BINOP | 27);
constexpr int T_MINUSEQ = (PREC(12) | T_BINOP | 28);
constexpr int T_ASTERIXEQ = (PREC(12) | T_BINOP | 29);
constexpr int T_SLASHEQ = (PREC(12) | T_BINOP | 30);
constexpr int T_PERCENTEQ = (PREC(12) | T_BINOP | 31);
constexpr int T_GRTGRTEQ = (PREC(12) | T_BINOP | 32);
constexpr int T_LESSLESSEQ = (PREC(12) | T_BINOP | 33);
constexpr int T_ANDEQ = (PREC(12) | T_BINOP | 34);
constexpr int T_CARETEQ = (PREC(12) | T_BINOP | 35);
constexpr int T_BAREQ = (PREC(12) | T_BINOP | 36);
constexpr int T_2TILDEEQ = (PREC(12) | T_BINOP | 37);
constexpr int T_LESSEQGRT = (PREC(12) | T_BINOP | 38);
constexpr int T_COMMA = (PREC(13) | T_BINOP | 39);
constexpr int BINOP_MAX = 39;
/* Maximum values       (PREC(15)|T_BINOP|63) */

/*
 * Reasons for doing things (compiling expressions generally).
 */
constexpr int FOR_VALUE = 0;
constexpr int FOR_LVALUE = 1;
constexpr int FOR_EFFECT = 2;
constexpr int FOR_TEMP = 3;

/*
 * Flags modifying the behaviour of the colon operator. These are stored
 * in the op_code field of a OP_COLON operators by the compiler and noticed
 * by the execution loop.
 */
constexpr int OPC_COLON_CARET = 0x0001; /* It's a :^ not a : */
constexpr int OPC_COLON_CALL = 0x0002;  /* Don't form a method, just call it. */

/*
 * Expression tree.  This is what the parseing functions build and
 * pass to compile_expr().
 */
struct expr
{
    int     e_what; /* A token identifier, ie. T_*. */
    expr   *e_arg[2];
    object *e_obj;
};

class parse_type : public type
{
public:
    parse_type() : type("parse", sizeof(struct parse))
    {
    }

    size_t mark(object *o) override;
    void   free(object *o) override;
};

} // namespace ici

#endif /* ICI_PARSE_H */
