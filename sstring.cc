#define ICI_CORE
#include "fwd.h"
#include "str.h"

namespace ici
{

/*
 * Include sstring.h to define static string objects (with 1 ref count
 * and a pesudo size of 1 in o_leafz).
 */
#if ICI_KEEP_STRING_HASH
#define SSTRING(name, str) sstring_t ici_ss_##name(str);
//        = {{TC_STRING, 0, 1, 1}, nullptr, nullptr, 0, 0, (sizeof str) - 1, nullptr, str};
#else
#define SSTRING(name, str) sstring_t ici_ss_##name(str);
//        = {{TC_STRING, 0, 1, 1}, nullptr, nullptr, 0, (sizeof str) - 1, nullptr, str};
#endif
#include "sstring.h"
#undef SSTRING

/*
 * Make all our staticly initialised strings atoms. Note that they are
 * *not* registered with the garbage collector.
 */
int init_sstrings()
{
    if (
#define SSTRING(name, str) (SS(name)->s_chars = SS(name)->s_u.su_inline_chars, atom(SS(name), 1)) == SS(name) &&
#include "sstring.h"
#undef SSTRING
        1)
        return 0;
    return set_error("failed to setup static strings");
}

} // namespace ici
