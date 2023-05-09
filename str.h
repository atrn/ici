// -*- mode:c++ -*-

#ifndef ICI_STRING_H
#define ICI_STRING_H

#include "object.h"

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

/*
 * This define enables keeping of the string hash in the string structure to
 * save re-computation. It is potentially recomputed when the atom pool is
 * rebuilt or atoms are being removed from it. Changes to hashing and atom
 * pool usage have reduced the number of times hashes are recomputer and it
 * now seems that *not* keeping the hash value makes things faster overall
 * than keeping it (which must be due to memory bandwidth and cache misses).
 * So it is now ifdefed out. However it's a close thing and it could go back
 * in one day.
 */
#define ICI_KEEP_STRING_HASH 1

struct str : object
{
    str()
        : object{TC_STRING}
    {
    }

    map     *s_map;   /* Where we were last found on the vs. */
    slot    *s_slot;  /* And our slot. */
    uint32_t s_vsver; /* The vs version at that time. */
#if ICI_KEEP_STRING_HASH
    unsigned long s_hash; /* String hash code or 0 if not yet computed */
#endif
    size_t s_nchars;
    char  *s_chars;
    union {
        int  su_nalloc;
        char su_inline_chars[1]; /* And following bytes. */
    } s_u;
};
/*
 * s_nchars             The actual number of characters in the string. Note
 *                      that room is always allocated for a guard '\0' beyond
 *                      this amount.
 *
 * s_chars              This points to the characters of the string, which
 *                      *may* be in a seperate allocation, or may be following
 *                      directly on in the same allocation. The flag
 *                      S_SEP_ALLOC reveals which (see below).
 *
 * s_u.su_nalloc        The number of bytes allocaed at s_chars iff it
 *                      is a seperate allocation (ICI_S_SEP_ALLOC set in
 *                      o_flags).
 *
 * su.su_inline_chars   If ICI_S_SEP_ALLOC is *not* set, this is where s_chars will
 *                      be pointing. The actual string chars follow on from this.
 */
inline str *stringof(object *o)
{
    return o->as<str>();
}
inline bool isstring(object *o)
{
    return o->hastype(TC_STRING);
}

/*
 * This flag (in o_flags) indicates that the lookup-lookaside mechanism
 * is referencing an atomic struct.  It is stored in the allowed area of
 * o_flags.
 */
constexpr int ICI_S_LOOKASIDE_IS_ATOM = 0x20;

/*
 * This flag (in o_flags) indicates that s_chars points to seperately
 * allocated memory.  If this is the case, s_u.su_nalloc is significant and
 * the memory was allocated with ici_nalloc(s_u.su_nalloc).
 */
constexpr int ICI_S_SEP_ALLOC = 0x40;

/*
 * Macros to assist external modules in getting ICI strings. To use, make
 * an include file called "icistr.h" with your strings, and what you want to
 * call them by, formatted like this:
 *
 *  ICI_STR(fred, "fred")
 *  ICI_STR(amp, "&")
 *
 * etc. Include that file in any files that access ICI strings.
 * Access them with either ICIS(fred) or ICISO(fred) which return
 * str* and object* pointers respectively. For example:
 *
 *  o = ici_fetch(s, ICIS(fred));
 *
 * Next, in one of your source file, include the special include file
 * "icistr-setup.h". This will (a) declare pointers to the string objects,
 * and (b) define a function (init_ici_str()) that initialises those pointers.
 *
 * Finally, call init_ici_str() at startup. It returns 1 on error, usual
 * conventions.
 */
#ifdef ICI_MODULE_NAME
#define ICIS_SYM_EXP(MOD, NAM) ici_##MOD##_str_##NAM
#define ICIS_SYM(MOD, NAM) ICIS_SYM_EXP(MOD, NAM)
#define ICIS(NAM) (ICIS_SYM(ICI_MODULE_NAME, NAM))
#define ICI_STR_NORM(NAM, STR) extern ici::str *ICIS_SYM(ICI_MODULE_NAME, NAM);
#define ICI_STR_DECL(NAM, STR) ici::str *ICIS_SYM(ICI_MODULE_NAME, NAM);
#else
#define ICIS(NAM) (ici_str_##NAM)
#define ICI_STR_NORM(NAM, STR) extern ici::str *ici_str_##NAM;
#define ICI_STR_DECL(NAM, STR) ici::str *ici_str_##NAM;
#endif
#define ICISO(NAM) (ICIS(NAM))
#define ICI_STR_MAKE(NAM, STR) (ICIS(NAM) = ici::new_str_nul_term(STR)) == nullptr ||
#define ICI_STR_REL(NAM, STR) (ICIS(NAM))->decref();
#define ICI_STR ICI_STR_NORM

class string_type : public type
{
public:
    string_type() : type("string", sizeof(struct str))
    {
    }

    size_t        mark(object *) override;
    void          free(object *) override;
    int           cmp(object *, object *) override;
    object       *copy(object *) override;
    unsigned long hash(object *) override;
    object       *fetch(object *, object *) override;
    int           assign(object *, object *, object *) override;
    int           forall(object *) override;
    int           save(archiver *, object *) override;
    object       *restore(archiver *) override;
    int64_t       len(object *) override;
};

/*
 * End of ici.h export. --ici.h-end--
 */

#ifdef ICI_CORE
/*
 * A structure to hold static (ie, not allocated) strings. These can
 * only be used where the atom() operation is guaranteed to use the
 * string given, and never find an existing one already in the atom pool.
 * They are only used by the ICI core on first initialisation. They
 * are not registered with the garbage collector. They are inserted into
 * the atom pool of course. They only support strings up to 15 characters.
 * See sstring.c.
 *
 * This structure must be an exact overlay of the one above.
 */
typedef struct sstring sstring_t;

struct sstring : object
{
    sstring(const char *cs)
        : object(TC_STRING)
        , s_map(nullptr)
        , s_slot(nullptr)
#if ICI_KEEP_STRING_HASH
        , s_hash(0)
#endif
        , s_nchars(strlen(cs))
        , s_chars(s_inline_chars)
    {
        memcpy(s_inline_chars, cs, s_nchars);
    }

    map     *s_map;   /* Where we were last found on the vs. */
    slot    *s_slot;  /* And our slot. */
    uint32_t s_vsver; /* The vs version at that time. */
#if ICI_KEEP_STRING_HASH
    unsigned long s_hash; /* String hash code or 0 if not yet computed */
#endif
    size_t s_nchars;
    char  *s_chars;
    char   s_inline_chars[15]; /* Longest string in sstring.h */
};

#define SSTRING(name, str) extern sstring_t ici_ss_##name;
#include "sstring.h"
#undef SSTRING

#define SS(name) (reinterpret_cast<ici::str *>(&ici_ss_##name))

#define str_char_at(s, i) ((s)->s_chars[i])

#endif /* ICI_CORE */

} // namespace ici

#endif /* ICI_STRING_H */
