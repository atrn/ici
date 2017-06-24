#define ICI_CORE
#include "exec.h"
#include "op.h"
#include "int.h"
#include "buf.h"
#include "pc.h"
#include "struct.h"
#include "null.h"
#include "forall.h"
#include "catcher.h"

namespace ici
{

/*
 * self => array looper &array[2] (xs)
 * array => - (os)
 *
 * Ie, like a loop, but on the first time in it skips the first two elements
 * of the the loop, which are expected to be an array of code for the step
 * and an exec operator to run it.
 */
int
ici_op_for()
{
    ici_get_pc(arrayof(ici_os.a_top[-1]), ici_xs.a_top + 1);
    pcof(ici_xs.a_top[1])->pc_next += opof(ici_xs.a_top[-1])->op_code;
    ici_xs.a_top[-1] = ici_os.a_top[-1];
    *ici_xs.a_top++ = &ici_o_looper;
    ++ici_xs.a_top; /* pc */
    --ici_os.a_top;
    return 0;
}

ici_op_t    ici_o_exec{ICI_OP_EXEC};
ici_op_t    ici_o_looper{ICI_OP_LOOPER};
ici_op_t    ici_o_loop{ICI_OP_LOOP};
ici_op_t    ici_o_break{ICI_OP_BREAK};
ici_op_t    ici_o_continue{ICI_OP_CONTINUE};
ici_op_t    ici_o_if{ICI_OP_IF};
ici_op_t    ici_o_ifnotbreak{ICI_OP_IFNOTBREAK};
ici_op_t    ici_o_ifbreak{ICI_OP_IFBREAK};
ici_op_t    ici_o_ifelse{ICI_OP_IFELSE};
ici_op_t    ici_o_pop{ICI_OP_POP};
ici_op_t    ici_o_andand{ICI_OP_ANDAND, 1};
ici_op_t    ici_o_barbar{ICI_OP_ANDAND, 0};
ici_op_t    ici_o_switch{ICI_OP_SWITCH};
ici_op_t    ici_o_switcher{ICI_OP_SWITCHER};
ici_op_t    ici_o_critsect{ICI_OP_CRITSECT};
ici_op_t    ici_o_waitfor{ICI_OP_WAITFOR};
ici_op_t    ici_o_rewind{ICI_OP_REWIND};
ici_op_t    ici_o_end{ICI_OP_ENDCODE};

} // namespace ici
