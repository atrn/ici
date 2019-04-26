#include <ici.h>
#include <curses.h>

namespace
{

#include "icistr.h"
#include <icistr-setup.h>

ici::handle *new_window(WINDOW *w)
{
    return ici::new_handle(w, ICIS(WINDOW), nullptr);
}

#define INT_FUNC(NAME, FUNC)            \
    int f_ ## NAME()                    \
    {                                   \
        return ici::int_ret(FUNC());    \
    }

#define NULL_FUNC(NAME, FUNC)           \
    int f_ ## NAME()                    \
    {                                   \
        FUNC();                         \
        return ici::null_ret();         \
    }

INT_FUNC(       getch,          getch           )
INT_FUNC(       endwin,         endwin          )
NULL_FUNC(      cbreak,         cbreak          )
NULL_FUNC(      clear,          clear           )
NULL_FUNC(      echo,           echo            )
NULL_FUNC(      nl,             nl              )
NULL_FUNC(      noecho,         noecho          )
NULL_FUNC(      nonl,           nonl            )
INT_FUNC(       erase,          erase           )
INT_FUNC(       clrtobot,       clrtobot        )
INT_FUNC(       clrtoeol,       clrtoeol        )
INT_FUNC(       delch,          delch           )
INT_FUNC(       refresh,        refresh         )

int f_initscr()
{
    auto w = initscr();
    if (!w)
    {
        return ici::set_error("initscr() failed");
    }
    return ici::ret_with_decref(new_window(w));
}

int f_addch()
{
    int64_t code;
    if (ici::typecheck("i", &code))
    {
        return 1;
    }
    addch(static_cast<chtype>(code));
    return ici::null_ret();
}

int f_addstr()
{
    char *s;
    if (ici::typecheck("s", &s))
    {
        return 1;
    }
    addstr(s);
    return ici::null_ret();
}

} // anon

extern "C" ici::object *ici_curses_init()
{
    if (ici::check_interface(ici::version_number, ici::back_compat_version, "curses"))
    {
        return nullptr;
    }
    if (init_ici_str())
    {
        return nullptr;
    }
    static ICI_DEFINE_CFUNCS(curses)
    {
        ICI_DEFINE_CFUNC(addch, f_addch),
        ICI_DEFINE_CFUNC(addstr, f_addstr),
        ICI_DEFINE_CFUNC(cbreak, f_cbreak),
        ICI_DEFINE_CFUNC(clear, f_clear),
        ICI_DEFINE_CFUNC(clrtobot, f_clrtobot),
        ICI_DEFINE_CFUNC(clrtoeol, f_clrtoeol),
        ICI_DEFINE_CFUNC(delch, f_delch),
        ICI_DEFINE_CFUNC(echo, f_echo),
        ICI_DEFINE_CFUNC(endwin, f_endwin),
        ICI_DEFINE_CFUNC(erase, f_erase),
        ICI_DEFINE_CFUNC(getch, f_getch),
        ICI_DEFINE_CFUNC(initscr, f_initscr),
        ICI_DEFINE_CFUNC(nl, f_nl),
        ICI_DEFINE_CFUNC(noecho, f_noecho),
        ICI_DEFINE_CFUNC(nonl, f_nonl),
        ICI_DEFINE_CFUNC(refresh, f_refresh),
        ICI_CFUNCS_END()
    };
    return ici::new_module(ICI_CFUNCS(curses));
}
