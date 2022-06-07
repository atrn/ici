#define ICI_CORE
#include "buf.h"
#include "catcher.h"
#include "exec.h"
#include "forall.h"
#include "int.h"
#include "map.h"
#include "null.h"
#include "op.h"
#include "pc.h"

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
int op_for()
{
    set_pc(arrayof(os.a_top[-1]), xs.a_top + 1);
    pcof(xs.a_top[1])->pc_next += opof(xs.a_top[-1])->op_code;
    xs.a_top[-1] = os.a_top[-1];
    xs.push(&o_looper);
    ++xs.a_top; /* pc */
    --os.a_top;
    return 0;
}

op o_exec{OP_EXEC};
op o_looper{OP_LOOPER};
op o_loop{OP_LOOP};
op o_break{OP_BREAK};
op o_continue{OP_CONTINUE};
op o_if{OP_IF};
op o_ifnotbreak{OP_IFNOTBREAK};
op o_ifbreak{OP_IFBREAK};
op o_ifelse{OP_IFELSE};
op o_pop{OP_POP};
op o_andand{OP_ANDAND, 1};
op o_barbar{OP_ANDAND, 0};
op o_switch{OP_SWITCH};
op o_switcher{OP_SWITCHER};
op o_critsect{OP_CRITSECT};
op o_waitfor{OP_WAITFOR};
op o_rewind{OP_REWIND};
op o_end{OP_ENDCODE};

} // namespace ici
