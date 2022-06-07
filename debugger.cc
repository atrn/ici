#define ICI_CORE
#include "debugger.h"
#include "fwd.h"

namespace ici
{

debugger::~debugger()
{
}
void debugger::error_set(const char *, struct src *)
{
}
void debugger::error_uncaught(const char *, struct src *)
{
}
void debugger::function_call(object *, object **, int)
{
}
void debugger::function_result(object *)
{
}
void debugger::cfunc_call(object *)
{
}
void debugger::cfunc_result(object *)
{
}
void debugger::source_line(struct src *)
{
}
void debugger::watch(object *, object *, object *)
{
}
void debugger::finished()
{
}

class debugger *debugger = instanceof <class debugger>();

} // namespace ici
