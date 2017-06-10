// -*- mode:c++ -*-

#ifndef ICI_OBJECT_H
#define ICI_OBJECT_H

#ifndef ICI_FWD_H
#include "fwd.h"
#endif

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

/*
 * types[] is the "map" from the small integer type codes found in the
 * o_tcode field of every object header, to a type structure (below) that
 * characterises that type of object.  Standard core data types have standard
 * positions (see ICI_TC_* defines below).  Other types are registered at run-time
 * by calls to ici_register_type() and are given the next available slot.
 * ici_object_t's o_tcode is one byte so we're limited to 256 distinct types.
 */
constexpr size_t        max_types = 256;
extern DLI type_t *     types[max_types];

/*
 * Every object has a header. In the header the o_tcode (type code) field
 * can be used to index the types[] array to discover the obejct's
 * type structure. This is the type structure.
 *
 * Implementations of new types typically declare one of these strutures
 * statically and initialise its members with the functions that determine the
 * nature of the new type.  (Actually, most of the time it is only initialised
 * as far as the 't_name' field.  The remainder is mostly for intenal ICI use
 * and should be left zero.)
 *
 * This --struct-- forms part of the --ici-api--.
 */
struct type
{
    unsigned long       (*t_mark)               (ici_obj_t *);
    void                (*t_free)               (ici_obj_t *);
    unsigned long       (*t_hash)               (ici_obj_t *);
    int                 (*t_cmp)                (ici_obj_t *, ici_obj_t *);
    ici_obj_t	*       (*t_copy)               (ici_obj_t *);
    int                 (*t_assign)             (ici_obj_t *, ici_obj_t *, ici_obj_t *);
    ici_obj_t   *       (*t_fetch)              (ici_obj_t *, ici_obj_t *);
    const char  *       t_name;
    void                (*t_objname)            (ici_obj_t *, char [ICI_OBJNAMEZ]);
    int                 (*t_call)               (ici_obj_t *, ici_obj_t *);
    ici_str_t   *       t_ici_name;
    int                 (*t_assign_super)       (ici_obj_t *, ici_obj_t *, ici_obj_t *, ici_struct_t *);
    int                 (*t_fetch_super)        (ici_obj_t *, ici_obj_t *, ici_obj_t **, ici_struct_t *);
    int                 (*t_assign_base)        (ici_obj_t *, ici_obj_t *, ici_obj_t *);
    ici_obj_t   *       (*t_fetch_base)         (ici_obj_t *, ici_obj_t *);
    ici_obj_t   *       (*t_fetch_method)       (ici_obj_t *, ici_obj_t *);
    void        *       t_reserved2;   /* Must be zero. */
    void        *       t_reserved3;   /* Must be zero. */
    void        *       t_reserved4;   /* Must be zero. */
    int                 (*t_forall_step)        (ici_obj_t *);
};
/*
 * t_mark(o)            Must sets the ICI_O_MARK flag in o->o_flags of this object
 *                      and all objects referenced by this one which don't
 *                      already have ICI_O_MARK set.  Returns the approximate
 *                      memory cost of this and all other objects it sets the
 *                      ICI_O_MARK of.  Typically recurses on all referenced
 *                      objects which don't already have ICI_O_MARK set (this
 *                      recursion is a potential problem due to the
 *                      uncontrolled stack depth it can create).  This is only
 *                      used in the marking phase of garbage collection.
 *
 *                      The macro ici_mark() calls the t_mark function of the
 *                      object (based on object type) if the ICI_O_MARK flag of
 *                      the object is clear, else it returns 0.  This is the
 *                      usual interface to an object's mark function.
 *
 *                      The mark function implemantation of objects can assume
 *                      the ICI_O_MARK flag of the object they are being invoked
 *                      on is clear.
 *
 * t_free(o)            Must free the object o and all associated data, but not
 *                      other objects which are referenced from it.  This is
 *                      only called from garbage collection.  Care should be
 *                      taken to remember that errors can occur during object
 *                      creation and that the free function might be asked to
 *                      free a partially allocated object.
 *
 * t_cmp(o1, o2)        Must compare o1 and o2 and return 0 if they are the
 *                      same, else non zero.  This similarity is the basis for
 *                      merging objects into single atomic objects and the
 *                      implementation of the == operator.
 *
 *                      Currently only zero versus non-zero results are
 *                      significant.  However in future versions the t_cmp()
 *                      function may be generalised to return less than, equal
 *                      to, or greater than zero depending if 'o1' is less
 *                      than, equal to, or greater than 'o2'.  New
 *                      implementations would be wise to adopt this usage now.
 *
 *                      Some objects are by nature both unique and
 *                      intrinsically atomic (for example, objects which are
 *                      one-to-one with some other allocated data which they
 *                      alloc when the are created and free when they die).
 *                      For these objects the existing function
 *                      ici_cmp_unique() can be used as their implementation
 *                      of this function.
 *
 *                      It is very important in implementing this function not
 *                      to miss any fields which may otherwise distinguish two
 *                      obejcts.  The cmp, hash and copy operations of an
 *                      object are all related.  It is useful to check that
 *                      they all regard the same data fields as significant in
 *                      performing their operation.
 *
 * t_copy(o)            Must returns a copy of the given object.  This is the
 *                      basis for the implementation of the copy() function.
 *                      On failure, NULL is returned and error is set.  The
 *                      returned object has been ici_incref'ed.  The returned
 *                      object should cmp() as being equal, but be a distinct
 *                      object for objects that are not intrinsically atomic.
 *
 *                      Intrinsically atomic objects may use the existing
 *                      function ici_copy_simple() as their implemenation of
 *                      this function.
 *
 *                      Return NULL on failure, usual conventions.
 *
 * t_hash(o)            Must return an unsigned long hash which is sensitive
 *                      to the value of the object.  Two objects which cmp()
 *                      equal should return the same hash.
 *
 *                      The returned hash is used in a hash table shared by
 *                      objects of all types.  So, somewhat problematically,
 *                      it is desireable to generate hashes which have good
 *                      spread and seperation across all objects.
 *
 *                      Some objects are by nature both unique and
 *                      intrinsically atomic (for example, objects which are
 *                      one-to-one with some other allocated data which they
 *                      alloc when the are created and free when they die).
 *                      For these objects the existing function
 *                      'ici_hash_unique()' can be used as their
 *                      implementation of this function.
 *
 * t_assign(o, k, v)    Must assign to key 'k' of the object 'o' the value
 *                      'v'.  Return 1 on error, else 0.
 *
 *                      The existing function 'ici_assign_fail()' may be used
 *                      both as the implementation of this function for object
 *                      types which do not support any assignment, and as a
 *                      simple method of generating an error for particular
 *                      assignments which break some rule of the object.
 *
 *                      Not that it is not necessarilly wrong for an
 *                      intrinsically atomic object to support some form of
 *                      assignment.  Only for the modified field to be
 *                      significant in a 't_cmp()' operation.  Objects which
 *                      are intrinsically unique and atomic often support
 *                      assignments.
 *
 *                      Return non-zero on failure, usual conventions.
 *
 * t_fetch(o, k)        Fetch the value of key 'k' of the object 'o'.  Return
 *                      NULL on error.
 *              
 *                      Note that the returned object does not have any extra
 *                      reference count; however, in some circumstances it may
 *                      not have any garbage collector visible references to
 *                      it.  That is, it may be vunerable to a garbage
 *                      collection if it is not either incref()ed or hooked
 *                      into a referenced object immediately.  Callers are
 *                      responsible for taking care.
 *
 *                      The existing function 'ici_fetch_fail()' may be used
 *                      both as the implementation of this function for object
 *                      types which do not support any assignment, and as a
 *                      simple method of generating an error for particular
 *                      fetches which break some rule of the object.
 *
 *                      Return NULL on failure, usual conventions.
 *
 * t_name               The name of this type. Use for the implementation of
 *                      'typeof()' and in error messages.  But apart from that,
 *                      type names have no fundamental importance in the
 *                      langauge and need not even be unique.
 *
 * t_objname(o, p)      Must place a short (less than 30 chars) human readable
 *                      representation of the object in the given buffer.
 *                      This is not intended as a basis for re-parsing or
 *                      serialisation.  It is just for diagnostics and debug.
 *                      An implementation of 't_objname()' must not allocate
 *                      memory or otherwise allow the garbage collector to
 *                      run.  It is often used to generate formatted failure
 *                      messages after an error has occured, but before
 *                      cleanup has completed.
 *
 * t_call(o, s)         Must call the object 'o'.  If the object does not
 *                      support being called, this should be NULL.  If 's' is
 *                      non-NULL this is a method call and s is the subject
 *                      object of the call.  Return 1 on error, else 0.
 *                      The environment upon calling this function is
 *                      the same as that for intrinsic functions. Functions
 *                      and techniques that can be used in intrinsic function
 *                      implementations can be used in the implementation of
 *                      this function. The object being called can be assumed
 *                      to be on top of the operand stack
 *                      (i.e. ici_os.a_top[-1])
 *
 * t_ici_name           A 'ici_str_t' copy of 't_name'. This is just a cached
 *                      version so that typeof() doesn't keep re-computing the
 *                      string.
 *
 * t_fetch_method       An optional alternative to the basic 't_fetch()' that
 *                      will be called (if supplied) when doing a fetch for
 *                      the purpose of forming a method.  This is really only
 *                      a hack to support COM under Windows.  COM allows
 *                      remote objects to have properties, like
 *                      object.property, and methods, like object:method().
 *                      But without this special hack, we can't tell if a
 *                      fetch operation is supposed to perform the COM get/set
 *                      property operation, or return a callable object for a
 *                      future method call.  Most objects will leave this
 *                      NULL.
 *
 *                      Return NULL on failure, usual conventions.
 *
 * t_forall_step        An optional alternative to the predefined type
 *                      support in the 'forall' statement
 *                      implementation.  This functions performs a
 *                      single step of a forall loop, either assigning
 *                      a key/value for the next iteration or
 *                      indicating the end of the loop has been
 *                      reached.  The function is passed the
 *                      ici_forall_t representing the statement. The
 *                      fa_index field of ici_forall_t indicates the
 *                      calling sequence. An fa_index of -1 indicating
 *                      the first call.  The function returns 0 to
 *                      indicate it has assigned key/values for an
 *                      iteration, -1 to indicate that iteration
 *                      should end and any other value upon error.
 *
 * --ici-api--
 */

/*
 * Macros to perform the operation on the object.
 */

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
 * This --macro-- forms part of the --ici-api--.
 */
#define ici_fetch(o,k)      ((*ici_typeof(o)->t_fetch)(ici_objof(o), ici_objof(k)))

/*
 * Assign the value 'v' to key 'k' of the object 'o'. This macro just calls
 * the particular object's 't_assign()' function.
 *
 * Note that the argument 'o' is subject to multiple expansions.
 *
 * Returns non-zero on error, usual conventions.
 *
 * This --macro-- forms part of the --ici-api--.
 */
#define ici_assign(o,k,v)   ((*ici_typeof(o)->t_assign)(ici_objof(o), ici_objof(k), ici_objof(v)))

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
#define ici_assign_base(o,k,v) ((*ici_typeof(o)->t_assign_base)(ici_objof(o), ici_objof(k), ici_objof(v)))
/*
 * This version retained for backwards compatibility.
 */
#define assign_base(o,k,v) ((*ici_typeof(o)->t_assign_base)(ici_objof(o), ici_objof(k), ici_objof(v)))

/*
 * Fetch the value of the key 'k' from the object 'o', but only consider
 * the base object, even if there is a super chain. See the notes on
 * 'ici_fetch()', which also apply here. The object 'o' *must* be one that
 * supports super types (such as a 'struct' or a 'handle').
 *
 * This --macro-- forms part of the --ici-api--.
 */
#define ici_fetch_base(o,k) ((*ici_typeof(o)->t_fetch_base)(ici_objof(o), ici_objof(k)))
/*
 * This version retained for backwards compatibility.
 */
#define fetch_base(o,k) ((*ici_typeof(o)->t_fetch_base)(ici_objof(o), ici_objof(k)))

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
#define ici_fetch_super(o,k,v,b) ((*ici_typeof(o)->t_fetch_super)(ici_objof(o), ici_objof(k), v, b))

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
#define ici_assign_super(o,k,v,b) ((*ici_typeof(o)->t_assign_super)(ici_objof(o), ici_objof(k), ici_objof(v), b))

/*
 * Increment the object 'o's reference count.  References from ordinary
 * machine data objects (ie.  variables and stuff, not other objects) are
 * invisible to the garbage collector.  These refs must be accounted for if
 * there is a possibility of garbage collection.  Note that most routines that
 * make objects (new_*(), copy() etc...) return objects with 1 ref.  The
 * caller is expected to ici_decref() it when they attach it into wherever it
 * is going.
 *
 * This --macro-- forms part of the --ici-api--.
 */
#define ici_incref(o)       (++ici_objof(o)->o_nrefs)

/*
 * Decrement the object 'o's reference count.  References from ordinary
 * machine data objects (ie.  variables and stuff, not other objects) are
 * invisible to the garbage collector.  These refs must be accounted for if
 * there is a possibility of garbage collection.  Note that most routines that
 * make objects (new_*(), copy() etc...) return objects with 1 ref.  The
 * caller is expected to ici_decref() it when they attach it into wherever it
 * is going.
 *
 * This --macro-- forms part of the --ici-api--.
 */
#define ici_decref(o)       (--ici_objof(o)->o_nrefs)

/*
 * The generic flags that may appear in the lower 4 bits of o_flags are:
 *
 * ICI_O_MARK               The garbage collection mark flag.
 *
 * ICI_O_ATOM               Indicates that this object is the read-only
 *                      atomic form of all objects of the same type with
 *                      the same value. Any attempt to change an object
 *                      in a way that would change its value with respect
 *                      to the 't_cmp()' function (see 'type_t') must
 *                      check for this flag and fail the attempt if it is
 *                      set.
 *
 * ICI_O_SUPER              This object can support a super.
 *
 * --ici-api-- continued.
 */
constexpr int ICI_O_MARK  =         0x01;    /* Garbage collection mark. */
constexpr int ICI_O_ATOM  =         0x02;    /* Is a member of the atom pool. */
constexpr int ICI_O_TEMP  =         0x04;    /* Is a re-usable temp (flag for asserts). */
constexpr int ICI_O_SUPER =         0x08;    /* Has super (is ici_objwsup_t derived). */
constexpr int ICI_O_OLD   =	    0x10;    /* Has been through 1+ collects */

/*
 * This is the universal header of all objects.  Each object includes this as
 * its first element.  In the real structures associated with each object type the type
 * specific stuff follows
 *
 * This --struct-- forms part of the --ici-api--.
 */
struct ici_obj
{
    ici_obj()
        : o_tcode(0)
        , o_flags(0)
        , o_nrefs(0)
        , o_leafz(0)
    {}

    ici_obj(char tcode, char flags = 0, char nrefs = 1, char leafz = 0)
        : o_tcode(tcode)
        , o_flags(flags)
        , o_nrefs(nrefs)
        , o_leafz(leafz)
    {}

    inline type_t *type() const noexcept {
        return types[(size_t)o_tcode];
    }

    inline size_t mark()
    {
        if (o_flags & ICI_O_MARK) {
            return 0;
        }
        if (o_leafz != 0) {
            o_flags |= ICI_O_MARK;
            return o_leafz;
        }
        return type()->t_mark(this);
    }

    inline bool isa(char tcode) const noexcept {
        return o_tcode == tcode;
    }

    inline bool isa(type_t *type) const noexcept {
        return this->type() == type;
    }

    char        o_tcode;
    char        o_flags;
    char        o_nrefs;
    char        o_leafz;
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
 * ici_objof converts an arbitrary object pointer to its base ici_obj_t pointer.
 *
 * TODO: This will disappear. All objects are "is-a ici_obj_t" so normal C++
 * rules suffice for the down casts. Hence the current definition.
 */
#define ici_objof(x) (x)

/*
 * Return a pointer to the 'ici_type_t' struct of the given object.
 *
 * This --macro-- forms part of the --ici-api--.
 */
#define ici_typeof(o)      (ici_objof(o)->type())

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
struct ici_objwsup : ici_obj
{
    ici_objwsup(char tcode, char flags, char nrefs, char leafz)
        : ici_obj(tcode, flags, nrefs, leafz)
        , o_super(nullptr)
    {}

    ici_objwsup_t   *o_super;
};
#define ici_objwsupof(o)    (static_cast<ici_objwsup_t *>(o))

/*
 * Test if this object supports a super type.  (It may or may not have a super
 * at any particular time).
 *
 * This --macro-- forms part of the --ici-api--.
 */
inline bool ici_hassuper(const ici_obj_t *o) { return (o->o_flags & ICI_O_SUPER) != 0; }

// #define ici_hassuper(o)     (ici_objof(o)->o_flags & ICI_O_SUPER)

/*
 * For static object initialisations...
 */
#define ICI_OBJ(tc)    {(tc), 0, 1, 0}

/*
 * Set the basic fields of the object header of 'o'.  'o' can be any struct
 * declared with an object header (this macro casts it).  This macro is
 * prefered to doing it by hand in case there is any future change in the
 * structure.  See comments on each field of 'ici_obj_t'.  This is normally
 * the first thing done after allocating a new bit of memory to hold an ICI
 * object.
 *
 * This --macro-- forms part of the --ici-api--.
 */
#define ICI_OBJ_SET_TFNZ(o, tcode, flags, nrefs, leafz) \
    (ici_objof(o)->o_tcode = (tcode), \
     ici_objof(o)->o_flags = (flags), \
     ici_objof(o)->o_nrefs = (nrefs), \
     ici_objof(o)->o_leafz = (leafz))
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
inline size_t ici_mark(ici_obj_t *o)
{
    if (o->o_flags & ICI_O_MARK) {
        return 0;
    }
    if (o->o_leafz != 0) {
        o->o_flags |= ICI_O_MARK;
        return o->o_leafz;
    }
    return ici_typeof(o)->t_mark(o);
}

/*
 * Register the object 'o' with the garbage collector.  Object that are
 * registered with the garbage collector can get collected.  This is typically
 * done after allocaton and initialisation of basic fields when making a new
 * object.  Once an object has been registered with the garbage collector, it
 * can *only* be freed by the garbage collector.
 *
 * (Not all objects are registered with the garabage collector. The main
 * exception is staticly defined objects. For example, the 'ici_cfunt_t'
 * objects that are the ICI objects representing functions coded in C
 * are typically staticly defined and never registered. However there
 * are problems with unregistered objects that reference other objects,
 * so this should be used with caution.)
 *
 * This --macro-- forms part of the --ici-api--.
 */
#define ici_rego(o)     ici_rego_work(ici_objof(o))

/*
 * The o_tcode field is a small int. These are the "well known" core
 * language types. See comments on o_tcode above and types above.
 */
#define ICI_TC_OTHER        0
#define ICI_TC_PC           1
#define ICI_TC_SRC          2
#define ICI_TC_PARSE        3
#define ICI_TC_OP           4
#define ICI_TC_STRING       5
#define ICI_TC_CATCH        6
#define ICI_TC_FORALL       7
#define ICI_TC_INT          8
#define ICI_TC_FLOAT        9
#define ICI_TC_REGEXP       10
#define ICI_TC_PTR          11
#define ICI_TC_ARRAY        12
#define ICI_TC_STRUCT       13
#define ICI_TC_SET          14

#define ICI_TC_MAX_BINOP    14 /* Max of 15 for binary op args. */

#define ICI_TC_EXEC         15
#define ICI_TC_FILE         16
#define ICI_TC_FUNC         17
#define ICI_TC_CFUNC        18
#define ICI_TC_METHOD       19
#define ICI_TC_MARK         20
#define ICI_TC_NULL         21
#define ICI_TC_HANDLE       22
#define ICI_TC_MEM          23
#define ICI_TC_PROFILECALL  24
#define ICI_TC_ARCHIVE      25
/* TC_REF is a special type code reserved for use in the
   serialization protocol to indicate a reference to previously
   transmitted object. */
#define ICI_TC_REF          26
#define ICI_TC_RESTORER     27
#define ICI_TC_SAVER        28
#define ICI_TC_CHANNEL      29
#define ICI_TC_MAX_CORE     29

#define ICI_TRI(a,b,t)      (((((a) << 4) + b) << 6) + t_subtype(t))

#define ici_isfalse(o)      ((o) == ici_objof(ici_zero) || (o) == ici_null)

/*
 * End of ici.h export. --ici.h-end--
 */

/*
 * Forced cast of some pointer (e.g. ostemp union type)
 */
#define ici_object_cast(x) ((ici_obj_t *)(x))

#define freeo(o)        ((*ici_typeof(o)->t_free)(ici_objof(o)))
#define hash(o)         ((*ici_typeof(o)->t_hash)(ici_objof(o)))
#define cmp(o1,o2)      ((*ici_typeof(o1)->t_cmp)(ici_objof(o1), ici_objof(o2)))
#define copy(o)         ((*ici_typeof(o)->t_copy)(ici_objof(o)))

#ifndef BUGHUNT

/*
 * In the core we use a macro for ici_rego.
 */
#undef  ici_rego
#define ici_rego(o)     (ici_objs_top < ici_objs_limit \
                            ? (void)(*ici_objs_top++ = ici_objof(o)) \
                            : ici_grow_objs(ici_objof(o)))
#else
#undef  ici_rego
#define ici_rego(o)     bughunt_rego(ici_objof(o))
extern void             bughunt_rego(ici_obj_t *);
#endif

#define ICI_STORE_ATOM_AND_COUNT(po, s) \
        ((*(po) = ici_objof(s)), \
        ((++ici_natoms > ici_atomsz / 2) ? \
            ici_grow_atoms(ici_atomsz * 2), 0 : 0))

#define ici_atom_hash_index(h)  ((h) & (ici_atomsz - 1))

#ifdef BUGHUNT
#   undef ici_incref
#   undef ici_decref
    void bughunt_incref(ici_obj_t *o);
    void bughunt_decref(ici_obj_t *o);
#   define ici_incref(o) bughunt_incref(ici_objof(o))
#   define ici_decref(o) bughunt_decref(ici_objof(o))
#endif

} // namespace ici

#endif /* ICI_OBJECT_H */
