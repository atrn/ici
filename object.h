// -*- mode:c++ -*-

#ifndef ICI_OBJECT_H
#define ICI_OBJECT_H

#include "fwd.h"
#include "type.h"
#include "types.h"

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

/*
 * The generic flags that may appear in the lower 4 bits of o_flags are:
 *
 * ICI_O_MARK           The garbage collection mark flag.
 *
 * ICI_O_ATOM           Indicates that this object is the read-only
 *                      atomic form of all objects of the same type with
 *                      the same value. Any attempt to change an object
 *                      in a way that would change its value with respect
 *                      to the 't_cmp()' function (see 'type_t') must
 *                      check for this flag and fail the attempt if it is
 *                      set.
 *
 * ICI_O_SUPER          This object can support a super.
 *
 * --ici-api-- continued.
 */
constexpr int ICI_O_MARK  =         0x01;    /* Garbage collection mark. */
constexpr int ICI_O_ATOM  =         0x02;    /* Is a member of the atom pool. */
constexpr int ICI_O_TEMP  =         0x04;    /* Is a re-usable temp (flag for asserts). */
constexpr int ICI_O_SUPER =         0x08;    /* Has super (is objwsup derived). */

/*
 * This is the universal 'header' of all objects.  Each object type
 * inherits from object giving it the common 'header' fields, then
 * adds any type specific stuff, if any.
 *
 * This --struct-- forms part of the --ici-api--.
 */
struct object
{
    uint8_t        o_tcode;     // type code, index into types[]
    uint8_t        o_flags;     // flags, see above
    uint8_t        o_nrefs;     // # non-ICI references
    uint8_t        o_leafz;     // size of small object, iff != 0

    object()
        : o_tcode(0)
        , o_flags(0)
        , o_nrefs(0)
        , o_leafz(0)
    {}

    explicit object(uint8_t tcode, uint8_t flags = 0, uint8_t nrefs = 1, uint8_t leafz = 0)
        : o_tcode(tcode)
        , o_flags(flags)
        , o_nrefs(nrefs)
        , o_leafz(leafz)
    {}

    inline bool isa(uint8_t tcode) const noexcept {
        return o_tcode == tcode;
    }

    inline type *otype() const noexcept {
        return types[o_tcode];
    }

    inline const char * type_name() const noexcept {
        return otype()->name;
    }

    inline uint8_t flags(uint8_t mask = 0xff) const noexcept {
        return o_flags & mask;
    }

    inline bool flagged(uint8_t mask) const noexcept {
        return flags(mask) != 0;
    }

    inline void setflag(uint8_t mask) noexcept {
        o_flags |= mask;
    }

    inline void clrflag(uint8_t mask) noexcept {
        o_flags &= ~mask;
    }

    inline bool isatom() const noexcept {
        return flagged(ICI_O_ATOM);
    }

    inline void setmark() noexcept {
        setflag(ICI_O_MARK);
    }

    inline void clrmark() noexcept {
        clrflag(ICI_O_MARK);
    }

    inline bool marked() const noexcept {
        return flagged(ICI_O_MARK);
    }

    inline size_t mark() noexcept {
        if (flagged(ICI_O_MARK)) {
            return 0;
        }
        if (o_leafz != 0) {
            setmark();
            return o_leafz;
        }
        return otype()->mark(this);
    }

    inline void free() {
        otype()->free(this);
    }

#ifndef BUGHUNT
    inline void incref() noexcept {
        ++o_nrefs;
    }

    inline void decref() noexcept {
        --o_nrefs;
    }
#else
    void incref();
    void decref();
#endif

    inline unsigned long hash() noexcept {
        return otype()->hash(this);
    }

    inline int cmp(object *that) noexcept {
        return otype()->cmp(this, that);
    }

    inline object *copy() {
        return otype()->copy(this);
    }

    inline int assign(object *k, object *v) {
        return otype()->assign(this, k, v);
    }

    inline object *fetch(object *k) {
        return otype()->fetch(this, k);
    }

    inline int assign_super(object *k, object *v, ici_struct *b) {
        return otype()->assign_super(this, k, v, b);
    }
    
    inline int fetch_super(object *k, object **pv, ici_struct *b) {
        return otype()->fetch_super(this, k, pv, b);
    }

    inline int assign_base(object *k, object *v) {
        return otype()->assign_base(this, k, v);
    }

    inline object *fetch_base(object *k) {
        return otype()->fetch_base(this, k);
    }

    inline object *fetch_method(object *n) {
        return otype()->fetch_method(this, n);
    }

    inline bool can_call() const {
        return otype()->can_call();
    }

    inline int call(object *o) {
        return otype()->call(this, o);
    }

    inline int forall() {
        return otype()->forall(this);
    }
    
    inline void objname(char n[ICI_OBJNAMEZ]) {
        otype()->objname(this, n);
    }
};
/*
 * o_tcode              The small integer type code that characterises
 *                      this object. Standard core types have well known
 *                      codes identified by the ICI_TC_* defines. Other
 *                      types are registered at run-time and are given
 *                      the next available code.
 *
 *                      This code can be used to index types[] to discover
 *                      a pointer to the type structure.
 *
 * o_flags              Some boolean flags. Well known flags that apply to
 *                      all objects occupy the lower five bits of this byte.
 *                      The upper three bits are available for object specific
 *                      use. See O_* below.
 *
 * o_nrefs              A small integer count of the number of references
 *                      to this object that are *not* otherwise visible
 *                      to the garbage collector.
 *
 * o_leafz              If (and only if) this object does not reference any
 *                      other objects (i.e. its t_mark() function just sets
 *                      the ICI_O_MARK flag), and its memory cost fits in this
 *                      signed byte (< 127), then its size can be set here
 *                      to accelerate the marking phase of the garbage
 *                      collector. Else it must be zero.
 *
 * --ici-api-- continued.
 */

/*
 * Return a pointer to the 'type' struct of the given object.
 *
 * This --func-- forms part of the --ici-api--.
 */
inline type_t *ici_typeof(object *o) { return o->otype(); }

/*
 * "Object with super." This is a specialised header for all objects that
 * support a super pointer.  All such objects must have the ICI_O_SUPER flag set
 * in o_flags and provide the 't_fetch_super()' and 't_assign_super()'
 * functions in their type structure.  The actual 'o_super' pointer will be
 * NULL if there is no actual super, which is different from ICI_O_SUPER being
 * clear (which would mean there could not be a super, ever).
 *
 * This --struct-- forms part of the --ici-api--.
 */
struct objwsup : object
{
    objwsup(uint8_t tcode, uint8_t flags, uint8_t nrefs, uint8_t leafz)
        : object(tcode, flags, nrefs, leafz)
        , o_super(nullptr)
    {}

    objwsup *o_super;
};

inline objwsup *objwsupof(object *o) { return static_cast<objwsup *>(o); }

/*
 * Test if this object supports a super type.  (It may or may not have a super
 * at any particular time).
 *
 * This --macro-- forms part of the --ici-api--.
 */
inline bool hassuper(const object *o) { return o->flagged(ICI_O_SUPER); }

/*
 * Set the basic fields of the object header of 'o'.  'o' can be any struct
 * declared with an object header (this macro casts it).  This macro is
 * prefered to doing it by hand in case there is any future change in the
 * structure.  See comments on each field of 'object'.  This is normally
 * the first thing done after allocating a new bit of memory to hold an ICI
 * object.
 *
 * This --func-- forms part of the --ici-api--.
 */
inline void ICI_OBJ_SET_TFNZ(object *o, uint8_t tcode, uint8_t flags, uint8_t nrefs, uint8_t leafz) {
    o->o_tcode = tcode;
    o->o_flags = flags;
    o->o_nrefs = nrefs;
    o->o_leafz = leafz;
}
/*
 * I was really hoping that most compilers would reduce the above to a
 * single word write. Especially as they are all constants most of the
 * time. Maybe in future we can have some endian specific code to to
 * it manually.
 (*(unsigned long *)(o) = (tcode) | ((flags) << 8) | ((nrefs) << 16) | ((leafz) << 24))
 */

/*
 * The recursive traversal of all objects performed by marking is particularly
 * expensive. So we take pains to cut short function calls wherever possible.
 * The o_leafz field of an object tells us it doesn't reference any other objects
 * and is of small (ie o_leafz) size.
 */
inline size_t ici_mark(object *o) {
    return o->mark();
}

inline size_t maybe_mark(object *o) {
    return o == nullptr ? 0 : ici_mark(o);
}

/*
 * Fetch the value of the key 'k' from the object 'o'.  This macro just calls
 * the particular object's 't_fetch()' function.
 *
 * Note that the returned object does not have any extra reference count;
 * however, in some circumstances it may not have any garbage collector
 * visible references to it.  That is, it may be vunerable to a garbage
 * collection if it is not either incref()ed or hooked into a referenced
 * object immediately.  Callers are responsible for taking care.
 *
 * Note that the argument 'o' is subject to multiple expansions.
 *
 * Returns NULL on failure, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
inline object *ici_fetch(object *o, object *k) {
    return o->fetch(k);
}

/*
 * Assign the value 'v' to key 'k' of the object 'o'. This macro just calls
 * the particular object's 't_assign()' function.
 *
 * Note that the argument 'o' is subject to multiple expansions.
 *
 * Returns non-zero on error, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
inline int ici_assign(object *o, object *k, object *v) {
    return o->assign(k, v);
}

/*
 * Assign the value 'v' to key 'k' of the object 'o', but only assign into
 * the base object, even if there is a super chain. This may only be called
 * on objects that support supers.
 *
 * Note that the argument 'o' is subject to multiple expansions.
 *
 * Returns non-zero on error, usual conventions.
 *
 * This --macro-- forms part of the --ici-api--.
 */
inline int ici_assign_base(object *o, object *k, object *v) {
    return o->assign_base(k, v);
}

/*
 * Fetch the value of the key 'k' from the object 'o', but only consider
 * the base object, even if there is a super chain. See the notes on
 * 'ici_fetch()', which also apply here. The object 'o' *must* be one that
 * supports super types (such as a 'struct' or a 'handle').
 *
 * This --macro-- forms part of the --ici-api--.
 */
inline object *ici_fetch_base(object *o, object *k) {
    return o->fetch_base(k);
}


/*
 * Fetch the value of the key 'k' from 'o' and store it through 'v', but only
 * if the item 'k' is already an element of 'o' or one of its supers.  See the
 * notes on 'ici_fetch()', which also apply here.  The object 'o' *must* be
 * one that supports supers (such as a 'struct' or a 'handle').
 *
 * This function is used internally in fetches up the super chain (thus the
 * name).  In this context the argument 'b' indicates the base struct of the
 * fetch and is used to maintain the internal lookup look-aside mechanism.  If
 * not used in this manner, 'b' should be supplied as NULL.
 *
 * Return -1 on error, 0 if it was not found, and 1 if it was found.  If
 * found, the value is stored in *v.
 *
 * This --macro-- forms part of the --ici-api--.
 */
inline int ici_fetch_super(object *o, object *k, object **v, ici_struct *b) {
    return o->fetch_super(k, v, b);
}

/*
 * Assign the value 'v' at the key 'k' of the object 'o', but only if the key
 * 'k' is already an element of 'o' or one of its supers.  The object 'o'
 * *must* be one that supports supers (such as a 'struct' or a 'handle').
 *
 * This function is used internally in assignments up the super chain (thus
 * the name).  In this context the argument 'b' indicates the base struct of
 * the assign and is used to maintain the internal lookup look-aside
 * mechanism.  If not used in this manner, 'b' should be supplied as NULL.
 *
 * Return -1 on error, 0 if it was not found, and 1 if the assignment was
 * completed.
 *
 * This --macro-- forms part of the --ici-api--.
 */
inline int ici_assign_super(object *o, object *k, object *v, ici_struct *b) {
    return o->assign_super(k, v, b);
}

/*
 * Register the object 'o' with the garbage collector.  Object that are
 * registered with the garbage collector can get collected.  This is typically
 * done after allocaton and initialisation of basic fields when making a new
 * object.  Once an object has been registered with the garbage collector, it
 * can *only* be freed by the garbage collector.
 *
 * (Not all objects are registered with the garabage collector. The main
 * exception is staticly defined objects. For example, the 'cfunt'
 * objects that are the ICI objects representing functions coded in C
 * are typically staticly defined and never registered. However there
 * are problems with unregistered objects that reference other objects,
 * so this should be used with caution.)
 *
 * This --macro-- forms part of the --ici-api--.
 */
#ifndef BUGHUNT
/*
 * Inline function for ici_rego.
 */
inline void ici_rego(object *o) {
    if (ici_objs_top < ici_objs_limit) {
        *ici_objs_top++ = o;
    } else {
        ici_grow_objs(o);
    }
}
#else
extern void ici_rego(object *);
#endif


/*
 * The o_tcode field is a small int. These are the "well known" core
 * language types. See comments on o_tcode above and types above.
 */
constexpr int ICI_TC_OTHER =        0;
constexpr int ICI_TC_PC =           1;
constexpr int ICI_TC_SRC =          2;
constexpr int ICI_TC_PARSE =        3;
constexpr int ICI_TC_OP =           4;
constexpr int ICI_TC_STRING =       5;
constexpr int ICI_TC_CATCHER =      6;
constexpr int ICI_TC_FORALL =       7;
constexpr int ICI_TC_INT =          8;
constexpr int ICI_TC_FLOAT =        9;
constexpr int ICI_TC_REGEXP =       10;
constexpr int ICI_TC_PTR =          11;
constexpr int ICI_TC_ARRAY =        12;
constexpr int ICI_TC_STRUCT =       13;
constexpr int ICI_TC_SET =          14;
constexpr int ICI_TC_MAX_BINOP =    14; /* Max of 15 for binary op args. */

constexpr int ICI_TC_EXEC =         15;
constexpr int ICI_TC_FILE =         16;
constexpr int ICI_TC_FUNC =         17;
constexpr int ICI_TC_CFUNC =        18;
constexpr int ICI_TC_METHOD =       19;
constexpr int ICI_TC_MARK =         20;
constexpr int ICI_TC_NULL =         21;
constexpr int ICI_TC_HANDLE =       22;
constexpr int ICI_TC_MEM =          23;
constexpr int ICI_TC_PROFILECALL =  24;
constexpr int ICI_TC_ARCHIVE =      25;
/* TC_REF is a special type code reserved for use in the
   serialization protocol to indicate a reference to previously
   transmitted object. */
constexpr int ICI_TC_REF =          26;
constexpr int ICI_TC_RESTORER =     27;
constexpr int ICI_TC_SAVER =        28;
constexpr int ICI_TC_CHANNEL =      29;
constexpr int ICI_TC_MAX_CORE =     29;

/*
 * End of ici.h export. --ici.h-end--
 */

#define ICI_TRI(a,b,t)      (((((a) << 4) + b) << 6) + t_subtype(t))

/*
 * Forced cast of some pointer (e.g. ostemp union type)
 */
inline object *ici_object_cast(void *x) {
    return reinterpret_cast<object *>(x);
}

inline void ici_freeo(object *o) {
    return o->free();
}

inline unsigned long ici_hash(object *o) {
    return o->hash();
}

inline int ici_cmp(object *o1, object *o2) {
    return o1->cmp(o2);
}

inline object *ici_copy(object *o) {
    return o->copy();
}

inline void ICI_STORE_ATOM_AND_COUNT(object **po, object *s) {
    *po = s;
    if (++ici_natoms > ici_atomsz / 2) {
        ici_grow_atoms(ici_atomsz * 2);
    }
}

inline long ici_atom_hash_index(long h)  { return h & (ici_atomsz - 1); }

} // namespace ici

#endif /* ICI_OBJECT_H */
