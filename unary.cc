#define ICI_CORE
#include "exec.h"
#include "float.h"
#include "int.h"
#include "op.h"
#include "parse.h"
#include "buf.h"
#include "null.h"

namespace ici
{

static int
attempted(const char *what, const char *towhat)
{
    return ici_set_error("attempt to perform \"%s %s\"", what, towhat);
}

int
ici_op_unary()
{
    ici_int_t   *i;

    switch (ici_opof(ici_xs.a_top[-1])->op_code)
    {
    case t_subtype(T_EXCLAM):
        if (ici_isfalse(ici_os.a_top[-1]))
            ici_os.a_top[-1] = ici_one;
        else
            ici_os.a_top[-1] = ici_zero;
        --ici_xs.a_top;
        return 0;

    case t_subtype(T_TILDE):
        if (!ici_isint(ici_os.a_top[-1]))
            goto fail;
        if ((i = ici_int_new(~ici_intof(ici_os.a_top[-1])->i_value)) == NULL)
            return 1;
        ici_os.a_top[-1] = i;
        i->decref();
        --ici_xs.a_top;
        return 0;

    case t_subtype(T_MINUS):
        /*
         * Unary minus is implemented as a binary op because they are
         * more heavily optimised.
         */
        assert(0);
    fail:
    default:
        switch (ici_opof(ici_xs.a_top[-1])->op_code)
        {
        case t_subtype(T_EXCLAM): return attempted("!", ici_typeof(ici_os.a_top[-1])->name);
        case t_subtype(T_TILDE): return attempted("~", ici_typeof(ici_os.a_top[-1])->name);
        case t_subtype(T_MINUS): return attempted("-", ici_typeof(ici_os.a_top[-1])->name);
        }
        return attempted("<unknown unary operator>", ici_typeof(ici_os.a_top[-1])->name);
    }
}

} // namespace ici
