// -*- mode:c++ -*-

#ifndef ICI_OP_H
#define ICI_OP_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
struct ici_op : ici_obj
{
    ici_op() : ici_obj(ICI_TC_OP) {}

    ici_op(int (*func)())
        : ici_obj(ICI_TC_OP)
        , op_func(func)
        , op_ecode(0)
        , op_code(0)
    {}

    ici_op(int ecode, int code = 0)
        : ici_obj(ICI_TC_OP)
        , op_func(nullptr)
        , op_ecode(ecode)
        , op_code(code)
    {}

    int         (*op_func)();
    int         op_ecode;       /* See ICI_OP_* below. */
    int         op_code;
};

#define ici_opof(o) ((ici_op_t *)o)
#define ici_isop(o) ((o)->o_tcode == ICI_TC_OP)

ici_op_t *ici_new_op(int (*func)(), int ecode, int code);

/*
 * Operator codes. These are stored in the op_ecode field and
 * allow direct switching to the appropriate code in the main
 * execution loop. If op_ecode is ICI_OP_OTHER, then the op_func field
 * is significant instead.
 */
enum
{
    ICI_OP_OTHER,
    ICI_OP_CALL,
    ICI_OP_NAMELVALUE,
    ICI_OP_DOT,
    ICI_OP_DOTKEEP,
    ICI_OP_DOTRKEEP,
    ICI_OP_ASSIGN,
    ICI_OP_ASSIGN_TO_NAME,
    ICI_OP_ASSIGNLOCAL,
    ICI_OP_EXEC,
    ICI_OP_LOOP,
    ICI_OP_REWIND,
    ICI_OP_ENDCODE,
    ICI_OP_IF,
    ICI_OP_IFELSE,
    ICI_OP_IFNOTBREAK,
    ICI_OP_IFBREAK,
    ICI_OP_BREAK,
    ICI_OP_QUOTE,
    ICI_OP_BINOP,
    ICI_OP_AT,
    ICI_OP_SWAP,
    ICI_OP_BINOP_FOR_TEMP,
    ICI_OP_AGGR_KEY_CALL,
    ICI_OP_COLON,
    ICI_OP_COLONCARET,
    ICI_OP_METHOD_CALL,
    ICI_OP_SUPER_CALL,
    ICI_OP_ASSIGNLOCALVAR,
    ICI_OP_CRITSECT,
    ICI_OP_WAITFOR,
    ICI_OP_POP,
    ICI_OP_CONTINUE,
    ICI_OP_LOOPER,
    ICI_OP_ANDAND,
    ICI_OP_SWITCH,
    ICI_OP_SWITCHER,
    ICI_OP_GO
};

/*
 * Extern definitions for various statically defined op objects. They
 * Are defined in various source files. Generally where they are
 * implemented.
 */
extern ici_op_t         ici_o_quote;
extern ici_op_t         ici_o_looper;
extern ici_op_t         ici_o_loop;
extern ici_op_t         ici_o_rewind;
extern ici_op_t         ici_o_end;
extern ici_op_t         ici_o_break;
extern ici_op_t         ici_o_continue;
extern ici_op_t         ici_o_exec;
extern ici_op_t         ici_o_return;
extern ici_op_t         ici_o_call;
extern ici_op_t         ici_o_method_call;
extern ici_op_t         ici_o_super_call;
extern ici_op_t         ici_o_if;
extern ici_op_t         ici_o_ifnotbreak;
extern ici_op_t         ici_o_ifbreak;
extern ici_op_t         ici_o_ifelse;
extern ici_op_t         ici_o_pop;
extern ici_op_t         ici_o_colon;
extern ici_op_t         ici_o_coloncaret;
extern ici_op_t         ici_o_dot;
extern ici_op_t         ici_o_dotkeep;
extern ici_op_t         ici_o_dotrkeep;
extern ici_op_t         ici_o_mkptr;
extern ici_op_t         ici_o_openptr;
extern ici_op_t         ici_o_fetch;
extern ici_op_t         ici_o_for;
extern ici_op_t         ici_o_mklvalue;
extern ici_op_t         ici_o_onerror;
extern ici_op_t         ici_o_andand;
extern ici_op_t         ici_o_barbar;
extern ici_op_t         ici_o_namelvalue;
extern ici_op_t         ici_o_switch;
extern ici_op_t         ici_o_switcher;
extern ici_op_t         ici_o_critsect;
extern ici_op_t         ici_o_waitfor;

/*
 * End of ici.h export. --ici.h-end--
 */
#endif /* ICI_OP_H */
