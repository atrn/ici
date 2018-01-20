#define ICI_CORE
#include "fwd.h"
#include "debugger.h"

namespace ici
{

debugger::~debugger() {}
void debugger::error(char *, struct src *) {}
void debugger::fncall(object *, object **, int) {}
void debugger::fnresult(object *) {}
void debugger::src(struct src *) {}
void debugger::watch(object *, object *, object *) {}

debugger *o_debug = single_instance_of<debugger>();

} // namespace ici
