#define ICI_CORE
#include "fwd.h"
#include "str.h"
#include "struct.h"
#include "null.h"
#include "exec.h"
#include "int.h"
#include "primes.h"
#include "forall.h"

namespace ici
{

/*
 * How many bytes of memory we need for a string of n chars (single
 * allocation).
 */
#define STR_ALLOCZ(n)   ((n) + sizeof (ici_str_t) - sizeof (int))
// (offsetof(ici_str_t, s_u) + (n) + 1)

int (ici_str_char_at)(str *s, int index)
{
    return index < 0 || index >= s->s_nchars ? 0 : s->s_chars[index];
}

/*
 * Return a hash sensitive to the value of the object.
 * See the comment on t_hash() in object.h
 */
unsigned long
ici_hash_string(object *o)
{
    unsigned long       h;

#   if ICI_KEEP_STRING_HASH
    if (ici_stringof(o)->s_hash != 0)
    {
        return ici_stringof(o)->s_hash;
    }
#   endif

#   if defined(ICI_USE_SF_HASH)
    {
        extern uint32_t ici_superfast_hash(const char *, int);
        h = STR_PRIME_0 * ici_superfast_hash(ici_stringof(o)->s_chars, ici_stringof(o)->s_nchars);
    }
#   elif defined(ICI_USE_MURMUR_HASH)
    {
        unsigned int ici_murmur_hash(const unsigned char * data, int len, unsigned int h);
        h = STR_PRIME_0 * ici_murmur_hash((const unsigned char *)ici_stringof(o)->s_chars, ici_stringof(o)->s_nchars, 0);
    }
#   else
    h = ici_crc(STR_PRIME_0, (const unsigned char *)ici_stringof(o)->s_chars, ici_stringof(o)->s_nchars);
#   endif
#   if ICI_KEEP_STRING_HASH
    ici_stringof(o)->s_hash = h;
#   endif
    return h;
}

/*
 * Allocate a new string object (single allocation) large enough to hold
 * nchars characters, and register it with the garbage collector.  Note: This
 * string is not yet an atom, but must become so as it is *not* mutable.
 *
 * WARINING: This is *not* the normal way to make a string object. See
 * ici_str_new().
 *
 * This --func-- forms part of the --ici-api--.
 */
ici_str_t *
ici_str_alloc(int nchars)
{
    ici_str_t  *s;
    size_t              az;

    az = STR_ALLOCZ(nchars);
    if ((s = (ici_str_t *)ici_nalloc(az)) == NULL)
    {
        return NULL;
    }
    ICI_OBJ_SET_TFNZ(s, ICI_TC_STRING, 0, 1, az <= 127 ? az : 0);
    s->s_chars = s->s_u.su_inline_chars;
    s->s_nchars = nchars;
    s->s_chars[nchars] = '\0';
    s->s_struct = NULL;
    s->s_slot = NULL;
#   if ICI_KEEP_STRING_HASH
    s->s_hash = 0;
#   endif
    s->s_vsver = 0;
    ici_rego(s);
    return s;
}

/*
 * Make a new atomic immutable string from the given characters.
 *
 * Note that the memory allocated to a string is always at least one byte
 * larger than the listed size and the extra byte contains a '\0'.  For
 * when a C string is needed.
 *
 * The returned string has a reference count of 1 (which is caller is
 * expected to decrement, eventually).
 *
 * See also: 'ici_str_new_nul_term()' and 'ici_str_get_nul_term()'.
 *
 * Returns NULL on error, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
ici_str_t *
ici_str_new(const char *p, int nchars)
{
    ici_str_t           *s;
    size_t              az;
    static struct
    {
        ici_str_t       s;
        char            d[40];
    }
    proto; //    = {{ICI_OBJ(ICI_TC_STRING)}};

    assert(nchars >= 0);
    az = STR_ALLOCZ(nchars);
    if ((size_t)nchars < sizeof proto.d)
    {
        object       **po;

        proto.s.s_nchars = nchars;
        proto.s.s_chars = proto.s.s_u.su_inline_chars;
        memcpy(proto.s.s_chars, p, nchars);
        proto.s.s_chars[nchars] = '\0';
#       if ICI_KEEP_STRING_HASH
        proto.s.s_hash = 0;
#       endif
        if ((s = ici_stringof(ici_atom_probe2(&proto.s, &po))) != NULL)
        {
            s->incref();
            return s;
        }
        ++ici_supress_collect;
        az = STR_ALLOCZ(nchars);
        if ((s = (ici_str_t *)ici_nalloc(az)) == NULL)
        {
            --ici_supress_collect;
            return NULL;
        }
        memcpy((char *)s, (char *)&proto.s, az);
        ICI_OBJ_SET_TFNZ(s, ICI_TC_STRING, ICI_O_ATOM, 1, az);
        s->s_chars = s->s_u.su_inline_chars;
        ici_rego(s);
        --ici_supress_collect;
        ICI_STORE_ATOM_AND_COUNT(po, s);
        return s;
    }
    if ((s = (ici_str_t *)ici_nalloc(az)) == NULL)
    {
        return NULL;
    }
    ICI_OBJ_SET_TFNZ(s, ICI_TC_STRING, 0, 1, az <= 127 ? az : 0);
    s->s_chars = s->s_u.su_inline_chars;
    s->s_nchars = nchars;
    s->s_struct = NULL;
    s->s_slot = NULL;
    s->s_vsver = 0;
    memcpy(s->s_chars, p, nchars);
    s->s_chars[nchars] = '\0';
#   if ICI_KEEP_STRING_HASH
    s->s_hash = 0;
#   endif
    ici_rego(s);
    return ici_stringof(ici_atom(s, 1));
}

/*
 * Make a new atomic immutable string from the given nul terminated
 * string of characters.
 *
 * The returned string has a reference count of 1 (which is caller is
 * expected to decrement, eventually).
 *
 * Returns NULL on error, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
ici_str_t *
ici_str_new_nul_term(const char *p)
{
    ici_str_t  *s;

    if ((s = ici_str_new(p, strlen(p))) == NULL)
    {
        return NULL;
    }
    return s;
}

/*
 * Make a new atomic immutable string from the given nul terminated
 * string of characters.
 *
 * The returned string has a reference count of 0, unlike
 * ici_str_new_nul_term() which is exactly the same in other respects.
 *
 * Returns NULL on error, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
ici_str_t *
ici_str_get_nul_term(const char *p)
{
    ici_str_t   *s;

    if ((s = ici_str_new(p, strlen(p))) == NULL)
    {
        return NULL;
    }
    s->decref();
    return s;
}

/*
 * Return a new mutable string (i.e. one with a seperate growable allocation).
 * The initially allocated space is n, but the length is 0 until it has been
 * set by the caller.
 *
 * The returned string has a reference count of 1 (which is caller is
 * expected to decrement, eventually).
 *
 * Returns NULL on error, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
ici_str_t *
ici_str_buf_new(int n)
{
    ici_str_t           *s;

    if ((s = ici_talloc(ici_str_t)) == NULL)
    {
        return NULL;
    }
    if ((s->s_chars = (char *)ici_nalloc(n)) == NULL)
    {
        ici_tfree(s, ici_str_t);
        return NULL;
    }
    ICI_OBJ_SET_TFNZ(s, ICI_TC_STRING, ICI_S_SEP_ALLOC, 1, 0);
    s->s_u.su_nalloc = n;
    s->s_vsver = 0;
    s->s_nchars = 0;
    s->s_hash = 0;
    s->s_struct = NULL;
    s->s_slot = NULL;
    s->s_vsver = 0;
    ici_rego(s);
    return s;
}

/*
 * Ensure that the given string has enough allocated memory to hold a string
 * of n characters (and a guard '\0' which this routine stores).  Grows ths
 * string as necessary.  Returns 0 on success, 1 on error, usual conventions.
 * Checks that the string is mutable and not atomic.
 *
 * This --func-- forms part of the --ici-api--.
 */
int
ici_str_need_size(ici_str_t *s, int n)
{
    char                *chars;
    char                n1[30];

    if ((s->o_flags & (ICI_O_ATOM|ICI_S_SEP_ALLOC)) != ICI_S_SEP_ALLOC)
    {
        return ici_set_error("attempt to modify an atomic string %s", ici_objname(n1, s));
    }
    if (s->s_u.su_nalloc >= n + 1)
    {
        return 0;
    }
    n <<= 1;
    if ((chars = (char *)ici_nalloc(n)) == NULL)
    {
        return 1;
    }
    memcpy(chars, s->s_chars, s->s_nchars + 1);
    ici_nfree(s->s_chars, s->s_u.su_nalloc);
    s->s_chars = chars;
    s->s_u.su_nalloc = n;
    s->s_chars[n >> 1] = '\0';
    return 0;
}

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
size_t string_type::mark(object *o)
{
    o->setmark();
    if (o->flag(ICI_S_SEP_ALLOC))
    {
        return typesize() + ici_stringof(o)->s_u.su_nalloc;
    }
    else
    {
        return STR_ALLOCZ(ici_stringof(o)->s_nchars);
    }
}

/*
 * Returns 0 if these objects are equal, else non-zero.
 * See the comments on t_cmp() in object.h.
 */
int string_type::cmp(object *o1, object *o2)
{
    if (ici_stringof(o1)->s_nchars != ici_stringof(o2)->s_nchars)
    {
        return 1;
    }
    if (ici_stringof(o1)->s_nchars == 0)
    {
        return 0;
    }
    if (ici_stringof(o1)->s_chars[0] != ici_stringof(o2)->s_chars[0])
    {
        return 1;
    }
    return memcmp
    (
        ici_stringof(o1)->s_chars,
        ici_stringof(o2)->s_chars,
        ici_stringof(o1)->s_nchars
    );
}

/*
 * Return a copy of the given object, or NULL on error.
 * See the comment on t_copy() in object.h.
 */
object *string_type::copy(object *o)
{
    str *ns;

    if ((ns = ici_str_buf_new(ici_stringof(o)->s_nchars + 1)) == NULL)
    {
        return NULL;
    }
    ns->s_nchars = ici_stringof(o)->s_nchars;
    memcpy(ns->s_chars, ici_stringof(o)->s_chars, ns->s_nchars);
    ns->s_chars[ns->s_nchars] = '\0';
    return ns;
}

/*
 * Free this object and associated memory (but not other objects).
 * See the comments on t_free() in object.h.
 */
void string_type::free(object *o)
{
    if (o->flag(ICI_S_SEP_ALLOC))
    {
        ici_nfree(ici_stringof(o)->s_chars, ici_stringof(o)->s_u.su_nalloc);
        ici_tfree(o, ici_str_t);
    }
    else
    {
        ici_nfree(o, STR_ALLOCZ(ici_stringof(o)->s_nchars));
    }
}

/*
 * Return a hash sensitive to the value of the object.
 * See the comment on t_hash() in object.h
 */
unsigned long string_type::hash(object *o)
{
    return ici_hash_string(o);
}

/*
 * Return the object at key k of the obejct o, or NULL on error.
 * See the comment on t_fetch in object.h.
 */
object *string_type::fetch(object *o, object *k)
{
    int64_t i;

    if (!ici_isint(k))
    {
        return fetch_fail(o, k);
    }
    if ((i = (int)ici_intof(k)->i_value) < 0 || i >= ici_stringof(o)->s_nchars)
    {
        k = ici_str_new("", 0);
    }
    else
    {
        k = ici_str_new(&ici_stringof(o)->s_chars[i], 1);
    }
    if (k != NULL)
    {
        k->decref();
    }
    return k;
}

/*
 * Assign to key k of the object o the value v. Return 1 on error, else 0.
 * See the comment on t_assign() in object.h.
 *
 * The key k must be a positive integer. The string will attempt to grow
 * to accomodate the new index as necessary.
 */
int string_type::assign(object *o, object *k, object *v)
{
    int64_t     i;
    int64_t     n;
    str         *s;

    if (o->isatom())
    {
        return ici_set_error("attempt to assign to an atomic string");
    }
    if (!ici_isint(k) || !ici_isint(v))
        return assign_fail(o, k, v);
    i = ici_intof(k)->i_value;
    if (i < 0)
    {
        return ici_set_error("attempt to assign to negative string index");
    }
    s = ici_stringof(o);
    if (ici_str_need_size(s, i + 1))
        return 1;
    for (n = s->s_nchars; n < i; ++n)
        s->s_chars[n] = ' ';
    s->s_chars[i] = (char)ici_intof(v)->i_value;
    if (s->s_nchars < ++i)
    {
        s->s_nchars = i;
        s->s_chars[i] = '\0';
    }
    return 0;
}

int string_type::forall(object *o)
{
    struct forall *fa = forallof(o);
    str *s;
    ici_int *i;

    s = ici_stringof(fa->fa_aggr);
    if (++fa->fa_index >= s->s_nchars)
        return -1;
    if (fa->fa_vaggr != ici_null)
    {
        if ((s = ici_str_new(&s->s_chars[fa->fa_index], 1)) == NULL)
            return 1;
        if (ici_assign(fa->fa_vaggr, fa->fa_vkey, s))
            return 1;
        s->decref();
    }
    if (fa->fa_kaggr != ici_null)
    {
        if ((i = ici_int_new((long)fa->fa_index)) == NULL)
            return 1;
        if (ici_assign(fa->fa_kaggr, fa->fa_kkey, i))
            return 1;
        i->decref();
    }
    return 0;
}

} // namespace ici
