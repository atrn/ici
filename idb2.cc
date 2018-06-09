#define ICI_CORE
#include "fwd.h"
#include "debugger.h"

namespace ici
{

debugger::~debugger() {}
void debugger::error_set(char *, struct src *) {}
void debugger::error_uncaught(char *, struct src *) {}
void debugger::function_call(object *, object **, int) {}
void debugger::function_result(object *) {}
void debugger::source_line(struct src *) {}
void debugger::watch(object *, object *, object *) {}

debugger *o_debug = singleton<debugger>();

} // namespace ici
