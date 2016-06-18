#include "exec.h"
#include "func.h"
#include "struct.h"
#include "str.h"

int
f_rpc_main()
{
        char *command;
        ici_struct_t *autos;
        ici_struct_t *statics;
        ici_struct_t *externs;

        autos = new_struct();
        statics = new_struct();
        externs = copy(structof(v_top[-1])->s_super);
        autos->s_super = statics;
        loose(statics);
        statics->s_super = externs;
        loose(externs);
        *v_top++ = autos;
        loose(autos);

        command = "auto x=1;";
        assign(structof(v_top[-1])->s_super, get_cname("#test#"),
                get_cname(command));
        if (ici_func("parse", "o=os", &autos, "#test#", autos))
        {
                printf("ERROR\n");
        }
        else
        {
                printf("SUCCESS\n");
                command = "x=2;printf(\"x = %d\\n\", x);";
                assign(structof(v_top[-1])->s_super, get_cname("#test#"),
                        get_cname(command));
                if (ici_func("parse", "o=os", &autos, "#test#", autos))
                {
                        printf("ERROR2\n");
                }
                else printf("SUCCESS2\n");

        }
        --v_top;
        return null_ret();
}

static int
test_ici_func_obj()
{
    ici_str_t *name_obj;
    ici_func_t *func_obj;
    
    name_obj = new_cname("experiment");
    func_obj = fetch(v_top[-1], name_obj);
    loose(name_obj);
    ici_func_obj(func_obj, "");
    loose(func_obj);
    return null_ret();
}

ici_cfunc_t experiment[] =
{
    {CF_OBJ, "experiment", f_rpc_main},
    {CF_OBJ, "test_ici_func_obj", test_ici_func_obj},
    {CF_OBJ}
};
