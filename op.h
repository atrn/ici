// -*- mode:c++ -*-

#ifndef ICI_OP_H
#define ICI_OP_H

#include "object.h"

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
struct op : object
{
    op() : object(ICI_TC_OP) {}

    op(int (*func)())
        : object(ICI_TC_OP)
        , op_func(func)
        , op_ecode(0)
        , op_code(0)
    {}

    op(int ecode, int code = 0)
        : object(ICI_TC_OP)
        , op_func(nullptr)
        , op_ecode(ecode)
        , op_code(code)
    {}

    int     (*op_func)();
    int16_t op_ecode;       /* See ICI_OP_* below. */
    int16_t op_code;
};

inline op *opof(object *o) { return static_cast<op *>(o); }
inline bool isop(object *o) { return o->isa(ICI_TC_OP); }

op *ici_new_op(int (*func)(), int16_t ecode, int16_t code);

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
extern op         ici_o_quote;
extern op         ici_o_looper;
extern op         ici_o_loop;
extern op         ici_o_rewind;
extern op         ici_o_end;
extern op         ici_o_break;
extern op         ici_o_continue;
extern op         ici_o_exec;
extern op         ici_o_return;
extern op         ici_o_call;
extern op         ici_o_method_call;
extern op         ici_o_super_call;
extern op         ici_o_if;
extern op         ici_o_ifnotbreak;
extern op         ici_o_ifbreak;
extern op         ici_o_ifelse;
extern op         ici_o_pop;
extern op         ici_o_colon;
extern op         ici_o_coloncaret;
extern op         ici_o_dot;
extern op         ici_o_dotkeep;
extern op         ici_o_dotrkeep;
extern op         ici_o_mkptr;
extern op         ici_o_openptr;
extern op         ici_o_fetch;
extern op         ici_o_for;
extern op         ici_o_mklvalue;
extern op         ici_o_onerror;
extern op         ici_o_andand;
extern op         ici_o_barbar;
extern op         ici_o_namelvalue;
extern op         ici_o_switch;
extern op         ici_o_switcher;
extern op         ici_o_critsect;
extern op         ici_o_waitfor;

/*
 * End of ici.h export. --ici.h-end--
 */

class op_type : public type
{
public:
    op_type() : type("op", sizeof (struct op)) {}
    int cmp(object *o1, object *o2) override;
    unsigned long hash(object *o) override;
};

} // namespace ici

#endif /* ICI_OP_H */
