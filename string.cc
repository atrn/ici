#define ICI_CORE
#include "fwd.h"
#include "str.h"
#include "map.h"
#include "null.h"
#include "exec.h"
#include "int.h"
#include "primes.h"
#include "forall.h"
#include "archiver.h"

namespace ici
{

/*
 * How many bytes of memory we need for a string of n chars (single
 * allocation).
 */
#define STR_ALLOCZ(n)   ((n) + sizeof (str))

int (str_char_at)(str *s, size_t index)
{
    return index >= s->s_nchars ? 0 : s->s_chars[index];
}

/*
 * Return a hash sensitive to the value of the object.
 * See the comment on t_hash() in object.h
 */
unsigned long hash_string(object *o)
{
    unsigned long       h;

#   if ICI_KEEP_STRING_HASH
    if (stringof(o)->s_hash != 0) {
        return stringof(o)->s_hash;
    }
#   endif

#   if defined(ICI_USE_SF_HASH)
    {
        extern uint32_t ici_superfast_hash(const char *, int);
        h = STR_PRIME_0 * ici_superfast_hash(stringof(o)->s_chars, stringof(o)->s_nchars);
    }
#   elif defined(ICI_USE_MURMUR_HASH)
    {
        unsigned int ici_murmur_hash(const unsigned char * data, int len, unsigned int h);
        h = STR_PRIME_0 * ici_murmur_hash((const unsigned char *)stringof(o)->s_chars, stringof(o)->s_nchars, 0);
    }
#   else
    h = crc(STR_PRIME_0, (const unsigned char *)stringof(o)->s_chars, stringof(o)->s_nchars);
#   endif
#   if ICI_KEEP_STRING_HASH
    stringof(o)->s_hash = h;
#   endif
    return h;
}

/*
 * Allocate a new string object (single allocation) large enough to hold
 * nchars characters, and register it with the garbage collector.  Note: This
 * string is not yet an atom, but must become so as it is *not* mutable.
 *
 * WARINING: This is *not* the normal way to make a string object. See
 * new_str().
 *
 * This --func-- forms part of the --ici-api--.
 */
str *str_alloc(size_t nchars)
{
    str  *s;
    size_t              az;

    az = STR_ALLOCZ(nchars);
    if ((s = (str *)ici_nalloc(az)) == NULL) {
        return NULL;
    }
    set_tfnz(s, TC_STRING, 0, 1, az <= 127 ? az : 0);
    s->s_chars = s->s_u.su_inline_chars;
    s->s_nchars = nchars;
    s->s_chars[nchars] = '\0';
    s->s_map = NULL;
    s->s_slot = NULL;
#   if ICI_KEEP_STRING_HASH
    s->s_hash = 0;
#   endif
    s->s_vsver = 0;
    rego(s);
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
 * See also: 'new_str_nul_term()' and 'str_get_nul_term()'.
 *
 * Returns NULL on error, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
str *
new_str(const char *p, size_t nchars)
{
    str           *s;
    size_t        az;
    static struct
    {
        str       s;
        char      d[40];
    }
    proto;

    az = STR_ALLOCZ(nchars);
    if ((size_t)nchars < sizeof proto.d) {
        object       **po;

        proto.s.s_nchars = nchars;
        proto.s.s_chars = proto.s.s_u.su_inline_chars;
        memcpy(proto.s.s_chars, p, nchars);
        proto.s.s_chars[nchars] = '\0';
#       if ICI_KEEP_STRING_HASH
        proto.s.s_hash = 0;
#       endif
        if ((s = stringof(atom_probe2(&proto.s, &po))) != NULL) {
            s->incref();
            return s;
        }
        ++supress_collect;
        az = STR_ALLOCZ(nchars);
        if ((s = (str *)ici_nalloc(az)) == NULL) {
            --supress_collect;
            return NULL;
        }
        memcpy((char *)s, (char *)&proto.s, az);
        set_tfnz(s, TC_STRING, object::O_ATOM, 1, az);
        s->s_chars = s->s_u.su_inline_chars;
        rego(s);
        --supress_collect;
        store_atom_and_count(po, s);
        return s;
    }
    if ((s = (str *)ici_nalloc(az)) == NULL) {
        return NULL;
    }
    set_tfnz(s, TC_STRING, 0, 1, az <= 127 ? az : 0);
    s->s_chars = s->s_u.su_inline_chars;
    s->s_nchars = nchars;
    s->s_map = NULL;
    s->s_slot = NULL;
    s->s_vsver = 0;
    memcpy(s->s_chars, p, nchars);
    s->s_chars[nchars] = '\0';
#   if ICI_KEEP_STRING_HASH
    s->s_hash = 0;
#   endif
    rego(s);
    return stringof(atom(s, 1));
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
str *
new_str_nul_term(const char *p)
{
    str  *s;

    if ((s = new_str(p, strlen(p))) == NULL) {
        return NULL;
    }
    return s;
}

/*
 * Make a new atomic immutable string from the given nul terminated
 * string of characters.
 *
 * The returned string has a reference count of 0, unlike
 * new_str_nul_term() which is exactly the same in other respects.
 *
 * Returns NULL on error, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
str *str_get_nul_term(const char *p)
{
    str   *s;

    if ((s = new_str(p, strlen(p))) == NULL) {
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
str *new_str_buf(size_t n)
{
    str *s;

    if ((s = ici_talloc(str)) == NULL) {
        return NULL;
    }
    if ((s->s_chars = (char *)ici_nalloc(n)) == NULL) {
        ici_tfree(s, str);
        return NULL;
    }
    set_tfnz(s, TC_STRING, ICI_S_SEP_ALLOC, 1, 0);
    s->s_u.su_nalloc = n;
    s->s_vsver = 0;
    s->s_nchars = 0;
    s->s_hash = 0;
    s->s_map = NULL;
    s->s_slot = NULL;
    s->s_vsver = 0;
    rego(s);
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
int str_need_size(str *s, size_t n)
{
    char                *chars;
    char                n1[objnamez];

    if (s->flags(object::O_ATOM|ICI_S_SEP_ALLOC) != ICI_S_SEP_ALLOC) {
        return set_error("attempt to modify an atomic string %s", objname(n1, s));
    }
    if (size_t(s->s_u.su_nalloc) >= n + 1) {
        return 0;
    }
    n <<= 1;
    if ((chars = (char *)ici_nalloc(n)) == NULL) {
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
    if (o->flagged(ICI_S_SEP_ALLOC)) {
        return type::mark(o) + stringof(o)->s_u.su_nalloc;
    }
    else {
        o->setmark();
        return STR_ALLOCZ(stringof(o)->s_nchars);
    }
}

/*
 * Returns 0 if these objects are equal, else non-zero.
 * See the comments on t_cmp() in object.h.
 */
int string_type::cmp(object *o1, object *o2)
{
    if (stringof(o1)->s_nchars != stringof(o2)->s_nchars) {
        return 1;
    }
    if (stringof(o1)->s_nchars == 0) {
        return 0;
    }
    if (stringof(o1)->s_chars[0] != stringof(o2)->s_chars[0]) {
        return 1;
    }
    return memcmp
    (
        stringof(o1)->s_chars,
        stringof(o2)->s_chars,
        stringof(o1)->s_nchars
    );
}

/*
 * Return a copy of the given object, or NULL on error.
 * See the comment on t_copy() in object.h.
 */
object *string_type::copy(object *o)
{
    str *ns;

    if ((ns = new_str_buf(stringof(o)->s_nchars + 1)) == NULL) {
        return NULL;
    }
    ns->s_nchars = stringof(o)->s_nchars;
    memcpy(ns->s_chars, stringof(o)->s_chars, ns->s_nchars);
    ns->s_chars[ns->s_nchars] = '\0';
    return ns;
}

/*
 * Free this object and associated memory (but not other objects).
 * See the comments on t_free() in object.h.
 */
void string_type::free(object *o)
{
    if (o->flagged(ICI_S_SEP_ALLOC)) {
        ici_nfree(stringof(o)->s_chars, stringof(o)->s_u.su_nalloc);
        ici_tfree(o, str);
    }
    else {
        ici_nfree(o, STR_ALLOCZ(stringof(o)->s_nchars));
    }
}

/*
 * Return a hash sensitive to the value of the object.
 * See the comment on t_hash() in object.h
 */
unsigned long string_type::hash(object *o)
{
    return hash_string(o);
}

/*
 * Return the object at key k of the obejct o, or NULL on error.
 * See the comment on t_fetch in object.h.
 */
object *string_type::fetch(object *o, object *k)
{
    int64_t i;

    if (!isint(k)) {
        return fetch_fail(o, k);
    }
    if ((i = (int)intof(k)->i_value) < 0 || size_t(i) >= stringof(o)->s_nchars) {
        k = new_str("", 0);
    }
    else {
        k = new_str(&stringof(o)->s_chars[i], 1);
    }
    if (k != NULL) {
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

    if (o->isatom()) {
        return set_error("attempt to assign to an atomic string");
    }
    if (!isint(k) || !isint(v)) {
        return assign_fail(o, k, v);
    }
    i = intof(k)->i_value;
    if (i < 0) {
        return set_error("attempt to assign to negative string index");
    }
    s = stringof(o);
    if (str_need_size(s, i + 1)) {
        return 1;
    }
    for (n = s->s_nchars; n < i; ++n) {
        s->s_chars[n] = ' ';
    }
    s->s_chars[i] = (char)intof(v)->i_value;
    if (s->s_nchars < size_t(++i)) {
        s->s_nchars = i;
        s->s_chars[i] = '\0';
    }
    return 0;
}

int string_type::forall(object *o) {
    struct forall *fa = forallof(o);
    str *s;
    integer *i;

    s = stringof(fa->fa_aggr);
    if (++fa->fa_index >= s->s_nchars) {
        return -1;
    }
    if (fa->fa_vaggr != null) {
        if ((s = new_str(&s->s_chars[fa->fa_index], 1)) == NULL) {
            return 1;
	}
        if (ici_assign(fa->fa_vaggr, fa->fa_vkey, s)) {
            return 1;
	}
        s->decref();
    }
    if (fa->fa_kaggr != null) {
        if ((i = new_int((int64_t)fa->fa_index)) == NULL) {
            return 1;
	}
        if (ici_assign(fa->fa_kaggr, fa->fa_kkey, i)) {
            return 1;
	}
        i->decref();
    }
    return 0;
}

int string_type::save(archiver *ar, object *o) {
    auto s = stringof(o);
    return ar->save_name(o) || ar->write(int32_t(s->s_nchars)) || ar->write(s->s_chars, s->s_nchars);
}

object *string_type::restore(archiver *ar) {
    str *s;
    int32_t len;
    object *name;
    object *obj;

    if (ar->restore_name(&name)) {
        return nullptr;
    }
    if (ar->read(len)) {
        return nullptr;
    } if ((s = str_alloc(len)) == nullptr) {
        return nullptr;
    }
    if (ar->read(s->s_chars, len)) {
        goto fail;
    }
    hash_string(s);
    if ((obj = atom(s, 1)) == NULL) {
        goto fail;
    }
    if (ar->record(name, obj)) {
        obj->decref();
        goto fail;
    }
    return obj;

fail:
    s->decref();
    return NULL;
}

} // namespace ici
