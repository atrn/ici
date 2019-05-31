// -*- mode:c++ -*-

#ifndef ICI_OBJECT_H
#define ICI_OBJECT_H

#include "fwd.h"
#include "type.h"
#include "types.h"

namespace ici {

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

/*
 *  The tag_with_incref/decref types and corresponding values are used
 *  to select particular function overloads that increment, or decrement,
 *  an object's reference count.
 */
extern struct tag_with_incref {} with_incref;
extern struct tag_with_decref {} with_decref;

/*
 * This is the universal 'header' of all objects.  Each object is
 * represented as an _object header_ is followed by object type-
 * specific data.
 * 
 * In the code this is acheived by defining per-type struct types that
 * inherit object. This provides the object header and behaviour.
 *
 * The object header is 32-bits, split into four 8-bit fields:
 *
 *      - object type code
 *      - object flags
 *      - reference count
 *      - memory size
 *
 * The object's type defines various operations upon the value.
 * See ici::type in type.h
 *
 * This --struct-- forms part of the --ici-api--.
 */
struct object
{
    uint8_t        o_tcode;     // type code, index into types[]
    uint8_t        o_flags;     // flags, see below
    uint8_t        o_nrefs;     // # non-ICI references
    uint8_t        o_leafz;     // size of (small) object, iff != 0

    /*
     * o_tcode              The small integer type code that characterises
     *                      this object. Standard core types have well known
     *                      codes identified by the TC_* values. Other
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
     *                      the O_MARK flag), and its memory cost fits in this
     *                      signed byte (< 127), then its size can be set here
     *                      to accelerate the marking phase of the garbage
     *                      collector. Else it must be zero.
     *
     * --ici-api-- continued.
     */

    /*
     * The generic flags that may appear in the lower 4 bits of o_flags are:
     *
     * O_MARK           The garbage collection mark flag.
     *
     * O_ATOM           Indicates that this object is the read-only
     *                  atomic form of all objects of the same type with
     *                  the same value. Any attempt to change an object
     *                  in a way that would change its value with respect
     *                  to the 'cmp()' function (see 'type') must
     *                  check for this flag and fail the attempt if it is
     *                  set.
     *
     * O_SUPER          This object can support a super.
     *
     * --ici-api-- continued.
     */
    static constexpr int O_MARK     = (1<<0); /* 0x01 Garbage collection mark. */
    static constexpr int O_ATOM     = (1<<1); /* 0x02 Is a member of the atom pool. */
    static constexpr int O_TEMP     = (1<<2); /* 0x04 Is a re-usable temp (flag for asserts). */
    static constexpr int O_SUPER    = (1<<3); /* 0x08 Has super (is objwsup derived). */
    static constexpr int O_ICIBITS  = 0x1F; /* 0b 0001 1111 */
    static constexpr int O_USERBITS = 0xE0; /* 0b 1110 0000 */

    /*
     * Default constructed objects have all header fields as zero.
     */
    object()
        : o_tcode(0)
        , o_flags(0)
        , o_nrefs(0)
        , o_leafz(0)
    {}

    /*
     * Construct an object with the given type code and, optionally, initial flags,
     * (external) reference count and object size.  If not given the object is
     * created with a reference count of one.
     */
    explicit object(uint8_t tcode, uint8_t flags = 0, uint8_t nrefs = 1, uint8_t leafz = 0)
        : o_tcode(tcode)
        , o_flags(flags)
        , o_nrefs(nrefs)
        , o_leafz(leafz)
    {}

    void set_tfnz(uint8_t tcode, uint8_t flags, uint8_t nrefs, uint8_t leafz) {
        o_tcode = tcode;
        o_flags = flags;
        o_nrefs = nrefs;
        o_leafz = leafz;
    }

    /*
     * Return a pointer to this object cast to a pointer to some compatbile T.
     * This is used as follows,
     *
     *      auto x = y->as<fred_t>();
     *
     *  x is a fred_t * version of y.
     */
    template <typename T>
    T *as() {
        return static_cast<T *>(this);
    }

    /*
     * Return true if this object has the given type code.
     */
    inline bool hastype(uint8_t tcode) const {
        return o_tcode == tcode;
    }

    /*
     * Return, a pointer to, this object's type instance.
     */
    inline type *icitype() const {
        return types[o_tcode];
    }

    /*
     * Return a C string with the name of this object's type.
     */
    inline const char * type_name() const {
        return icitype()->name;
    }

    /*
     * Return this object's flags.
     */
    inline uint8_t flags() const {
        return o_flags;
    }

    /*
     * Return this object's flags filtered by a mask.
     */
    inline uint8_t flags(uint8_t mask) const {
        return o_flags & mask;
    }

    /*
     * Return true if this object has the given set of flags.
     */
    inline bool hasflag(uint8_t mask) const {
        return flags(mask) == mask;
    }

    /*
     * Set a flag on this object.
     */
    inline void set(uint8_t mask) {
        o_flags |= mask;
    }

    /*
     * Clear one or more flags set on this object.
     */
    inline void clr(uint8_t mask) {
        o_flags &= ~mask;
    }

    /*
     * Return true if this object is an atom.
     */
    inline bool isatom() const {
        return hasflag(O_ATOM);
    }

    /*
     * Set the mark flag on this object.
     */
    inline void setmark() {
        set(O_MARK);
    }

    /*
     * Clear the mark flag on this object.
     */
    inline void clrmark() {
        clr(O_MARK);
    }

    /*
     * Return true if this object is marked.
     */
    inline bool marked() const {
        return hasflag(O_MARK);
    }

    /*
     * Mark this object and return its size, in bytes, or
     * zero if already marked.
     */
    inline size_t mark() {
        if (marked()) {
            return 0;
        }
        if (o_leafz != 0) {
            setmark();
            return o_leafz;
        }
        return icitype()->mark(this);
    }

    /*
     * Free the memory used by this object.
     */
    inline void free() {
        icitype()->free(this);
    }

#ifdef ICI_INLINE_INCREF_DECREF
    /*
     * Increment the object's reference count.
     */
    inline void incref() {
        ++o_nrefs;
    }

    /*
     * Decrement the object's reference count.
     */
    inline void decref() {
        --o_nrefs;
    }
#else
    void incref();
    void decref();
#endif

    /*
     * Return this object's hash value.
     */
    inline unsigned long hash() {
        return icitype()->hash(this);
    }

    /*
     * Compare this object against another object
     * of the same type.
     */
    inline int cmp(object *that) {
        return icitype()->cmp(this, that);
    }

    /*
     * Return a copy of this object.
     */
    inline object *copy() {
        return icitype()->copy(this);
    }

    inline int assign(object *k, object *v) {
        return icitype()->assign(this, k, v);
    }

    inline object *fetch(object *k) {
        return icitype()->fetch(this, k);
    }

    inline int assign_super(object *k, object *v, map *b) {
        return icitype()->assign_super(this, k, v, b);
    }

    inline int fetch_super(object *k, object **pv, map *b) {
        return icitype()->fetch_super(this, k, pv, b);
    }

    inline int assign_base(object *k, object *v) {
        return icitype()->assign_base(this, k, v);
    }

    inline object *fetch_base(object *k) {
        return icitype()->fetch_base(this, k);
    }

    inline object *fetch_method(object *n) {
        return icitype()->fetch_method(this, n);
    }

    inline bool can_call() const {
        return icitype()->can_call();
    }

    inline int call(object *o) {
        return icitype()->call(this, o);
    }

    inline int forall(object *o) {
        return icitype()->forall(o);
    }

    inline void objname(char n[objnamez]) {
        icitype()->objname(this, n);
    }

    inline int save(archiver *a) {
        return icitype()->save(a, this);
    }

    inline int64_t objlen() {
        return icitype()->len(this);
    }
};

/*
 * "Object with super." This is a specialised header for all objects that
 * support a super pointer.  All such objects must have the O_SUPER flag set
 * in o_flags and provide the 't_fetch_super()' and 't_assign_super()'
 * functions in their type structure.  The actual 'o_super' pointer will be
 * nullptr if there is no actual super, which is different from O_SUPER being
 * clear (which would mean there could not be a super, ever).
 *
 * This --struct-- forms part of the --ici-api--.
 */
struct objwsup : object
{
    objwsup *o_super;

    objwsup(uint8_t tcode, uint8_t flags, uint8_t nrefs, uint8_t leafz)
        : object(tcode, flags, nrefs, leafz)
        , o_super(nullptr)
    {}
};

inline objwsup *objwsupof(object *o) { return o->as<objwsup>(); }

/*
 * Test if this object supports a super type.  (It may or may not have a super
 * at any particular time).
 *
 * This --func-- forms part of the --ici-api--.
 */
inline bool hassuper(const object *o) { return o->hasflag(object::O_SUPER); }

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
inline void set_tfnz(object *o, uint8_t tcode, uint8_t flags, uint8_t nrefs, uint8_t leafz) {
    o->set_tfnz(tcode, flags, nrefs, leafz);
}

inline void incref(object *o) {
    o->incref();
}

inline void decref(object *o) {
    o->decref();
}

/*
 * The recursive traversal of all objects performed by marking is particularly
 * expensive. So we take pains to cut short function calls wherever possible.
 * The o_leafz field of an object tells us it doesn't reference any other objects
 * and is of small (ie o_leafz) size.
 */
inline size_t ici_mark(object *o) {
    return o->mark();
}

/*
 * Mark the given object iff it is not NULL. Otherwise return 0.
 */
inline size_t mark_optional(object *o) {
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
 * Returns nullptr on failure, usual conventions.
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
 * This --func-- forms part of the --ici-api--.
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
 * This --func-- forms part of the --ici-api--.
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
 * not used in this manner, 'b' should be supplied as nullptr.
 *
 * Return -1 on error, 0 if it was not found, and 1 if it was found.  If
 * found, the value is stored in *v.
 *
 * This --func-- forms part of the --ici-api--.
 */
inline int ici_fetch_super(object *o, object *k, object **v, map *b) {
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
 * mechanism.  If not used in this manner, 'b' should be supplied as nullptr.
 *
 * Return -1 on error, 0 if it was not found, and 1 if the assignment was
 * completed.
 *
 * This --func-- forms part of the --ici-api--.
 */
inline int ici_assign_super(object *o, object *k, object *v, map *b) {
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
 * This --func-- forms part of the --ici-api--.
 */
#ifdef NDEBUG
/*
 * Inline function for rego.
 */
inline void rego(object *o) {
    if (objs_top < objs_limit) {
        *objs_top++ = o;
    } else {
        grow_objs(o);
    }
}
#else
extern void rego(object *);
#endif


/*
 * The o_tcode field is a small int. These are the "well known" core
 * language types. See comments on o_tcode above and types above.
 */
constexpr uint8_t TC_OTHER =        0;
constexpr uint8_t TC_PC =           1;
constexpr uint8_t TC_SRC =          2;
constexpr uint8_t TC_PARSE =        3;
constexpr uint8_t TC_OP =           4;
constexpr uint8_t TC_STRING =       5;
constexpr uint8_t TC_CATCHER =      6;
constexpr uint8_t TC_FORALL =       7;
constexpr uint8_t TC_INT =          8;
constexpr uint8_t TC_FLOAT =        9;
constexpr uint8_t TC_REGEXP =       10;
constexpr uint8_t TC_PTR =          11;
constexpr uint8_t TC_ARRAY =        12;
constexpr uint8_t TC_MAP =          13;
constexpr uint8_t TC_SET =          14;
constexpr uint8_t TC_MAX_BINOP =    14; /* Max of 15 for binary op args. */
constexpr uint8_t TC_EXEC =         15;
constexpr uint8_t TC_FILE =         16;
constexpr uint8_t TC_FUNC =         17;
constexpr uint8_t TC_CFUNC =        18;
constexpr uint8_t TC_METHOD =       19;
constexpr uint8_t TC_MARK =         20;
constexpr uint8_t TC_NULL =         21;
constexpr uint8_t TC_HANDLE =       22;
constexpr uint8_t TC_MEM =          23;
constexpr uint8_t TC_PROFILECALL =  24;
constexpr uint8_t TC_REF =          25; // serialized ref to previously saved object
constexpr uint8_t TC_CHANNEL =      26;
constexpr uint8_t TC_FRAMES32 =     27;
constexpr uint8_t TC_FRAMES64 =     28;
constexpr uint8_t TC_MAX_CORE =     28;

/*
 * End of ici.h export. --ici.h-end--
 */

inline unsigned long hashof(object *o) {
    return o->hash();
}

inline int compare(object *o1, object *o2) {
    return o1->cmp(o2);
}

inline object *copyof(object *o) {
    return o->copy();
}

inline void store_atom_and_count(object **po, object *s) {
    *po = s;
    if (++natoms > atomsz / 2) {
        grow_atoms(atomsz * 2);
    }
}

inline long atom_hash_index(long h)  {
    return h & (atomsz - 1);
}

inline int64_t objlen(object *o) {
    return o->objlen();
}

} // namespace ici

#include "ref.h"

#endif /* ICI_OBJECT_H */
