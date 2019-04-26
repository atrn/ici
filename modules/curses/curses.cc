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

int f_getch()
{
    return ici::int_ret(getch());
}

int f_cbreak()
{
    cbreak();
    return ici::null_ret();
}

int f_noecho()
{
    noecho();
    return ici::null_ret();
}

int f_nonl()
{
    nonl();
    return ici::null_ret();
}

int f_endwin()
{
    return ici::int_ret(endwin());
}

int f_initscr()
{
    return ici::ret_with_decref(new_window(initscr()));
}

int f_addstr()
{
    char *s;
    if (ici::typecheck("s", &s))
        return 1;
    addstr(s);
    return ici::null_ret();
}

} // anon

extern "C" ici::object *ici_curses_init()
{
    if (ici::check_interface(ici::version_number, ici::back_compat_version, "curses"))
        return nullptr;
    if (init_ici_str())
        return nullptr;
    static ICI_DEFINE_CFUNCS(curses)
    {
        ICI_DEFINE_CFUNC(getch, f_getch),
        ICI_DEFINE_CFUNC(addstr, f_addstr),
        ICI_DEFINE_CFUNC(cbreak, f_cbreak),
        ICI_DEFINE_CFUNC(noecho, f_noecho),
        ICI_DEFINE_CFUNC(nonl, f_nonl),
        ICI_DEFINE_CFUNC(endwin, f_endwin),
        ICI_DEFINE_CFUNC(initscr, f_initscr),
        ICI_CFUNCS_END()
    };
    return ici::new_module(ICI_CFUNCS(curses));
}
