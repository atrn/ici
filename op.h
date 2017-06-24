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
    int16_t op_ecode;       /* See OP_* below. */
    int16_t op_code;
};

inline op *opof(object *o) { return static_cast<op *>(o); }
inline bool isop(object *o) { return o->isa(ICI_TC_OP); }

op *ici_new_op(int (*func)(), int16_t ecode, int16_t code);

/*
 * Operator codes. These are stored in the op_ecode field and
 * allow direct switching to the appropriate code in the main
 * execution loop. If op_ecode is OP_OTHER, then the op_func field
 * is significant instead.
 */
enum
{
    OP_OTHER,
    OP_CALL,
    OP_NAMELVALUE,
    OP_DOT,
    OP_DOTKEEP,
    OP_DOTRKEEP,
    OP_ASSIGN,
    OP_ASSIGN_TO_NAME,
    OP_ASSIGNLOCAL,
    OP_EXEC,
    OP_LOOP,
    OP_REWIND,
    OP_ENDCODE,
    OP_IF,
    OP_IFELSE,
    OP_IFNOTBREAK,
    OP_IFBREAK,
    OP_BREAK,
    OP_QUOTE,
    OP_BINOP,
    OP_AT,
    OP_SWAP,
    OP_BINOP_FOR_TEMP,
    OP_AGGR_KEY_CALL,
    OP_COLON,
    OP_COLONCARET,
    OP_METHOD_CALL,
    OP_SUPER_CALL,
    OP_ASSIGNLOCALVAR,
    OP_CRITSECT,
    OP_WAITFOR,
    OP_POP,
    OP_CONTINUE,
    OP_LOOPER,
    OP_ANDAND,
    OP_SWITCH,
    OP_SWITCHER,
    OP_GO
};

/*
 * Extern definitions for various statically defined op objects. They
 * Are defined in various source files. Generally where they are
 * implemented.
 */
extern op         o_quote;
extern op         o_looper;
extern op         o_loop;
extern op         o_rewind;
extern op         o_end;
extern op         o_break;
extern op         o_continue;
extern op         o_exec;
extern op         o_return;
extern op         o_call;
extern op         o_method_call;
extern op         o_super_call;
extern op         o_if;
extern op         o_ifnotbreak;
extern op         o_ifbreak;
extern op         o_ifelse;
extern op         o_pop;
extern op         o_colon;
extern op         o_coloncaret;
extern op         o_dot;
extern op         o_dotkeep;
extern op         o_dotrkeep;
extern op         o_mkptr;
extern op         o_openptr;
extern op         o_fetch;
extern op         o_for;
extern op         o_mklvalue;
extern op         o_onerror;
extern op         o_andand;
extern op         o_barbar;
extern op         o_namelvalue;
extern op         o_switch;
extern op         o_switcher;
extern op         o_critsect;
extern op         o_waitfor;

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
