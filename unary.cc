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
    return set_error("attempt to perform \"%s %s\"", what, towhat);
}

int op_unary()
{
    integer *i;

    switch (opof(xs.a_top[-1])->op_code)
    {
    case t_subtype(T_EXCLAM):
        if (isfalse(os.a_top[-1]))
            os.a_top[-1] = o_one;
        else
            os.a_top[-1] = o_zero;
        --xs.a_top;
        return 0;

    case t_subtype(T_TILDE):
        if (!isint(os.a_top[-1]))
            goto fail;
        if ((i = new_int(~intof(os.a_top[-1])->i_value)) == NULL)
            return 1;
        os.a_top[-1] = i;
        i->decref();
        --xs.a_top;
        return 0;

    case t_subtype(T_MINUS):
        /*
         * Unary minus is implemented as a binary op because they are
         * more heavily optimised.
         */
        assert(0);
    fail:
    default:
        switch (opof(xs.a_top[-1])->op_code)
        {
        case t_subtype(T_EXCLAM): return attempted("!", os.a_top[-1]->type_name());
        case t_subtype(T_TILDE): return attempted("~", os.a_top[-1]->type_name());
        case t_subtype(T_MINUS): return attempted("-", os.a_top[-1]->type_name());
        }
        return attempted("<unknown unary operator>", os.a_top[-1]->type_name());
    }
}

} // namespace ici
