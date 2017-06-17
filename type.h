// -*- mode:c++ -*-

#ifndef ICI_TYPES_H
#define ICI_TYPES_H

#include "fwd.h"

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

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
class type
{
public:
    static constexpr int has_fetch_method = 1<<0;
    static constexpr int has_forall       = 1<<1;
    static constexpr int has_objname      = 1<<2;
    static constexpr int has_call         = 1<<3;

public:
    const char * const  name;

private:
    const size_t        _size;
    const int           _flags;
    mutable ici_str_t * _name;

protected:
    explicit type(const char *name, size_t size, int flags = 0)
        : name(name)
        , _size(size)
        , _flags(flags)
        , _name(nullptr)
    {
    }

    inline size_t typesize() const noexcept { return _size; }

public:
    inline bool can_fetch_method() const { return _flags & has_fetch_method; }
    inline bool can_forall() const       { return _flags & has_forall; }
    inline bool can_objname() const      { return _flags & has_objname; }
    inline bool can_call() const         { return _flags & has_call; }

    virtual unsigned long       mark(ici_obj_t *o);
    virtual void                free(ici_obj_t *o);
    virtual unsigned long       hash(ici_obj_t *o);
    virtual int                 cmp(ici_obj_t *a, ici_obj_t *b);
    virtual ici_obj_t *         copy(ici_obj_t *o);
    virtual int                 assign(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v);
    virtual ici_obj_t *         fetch(ici_obj_t *o, ici_obj_t *k);
    virtual int                 assign_super(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v, ici_struct_t *b);
    virtual int                 fetch_super(ici_obj_t *o, ici_obj_t *k, ici_obj_t **pv, ici_struct_t *b);
    virtual int                 assign_base(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v);
    virtual ici_obj_t   *       fetch_base(ici_obj_t *o, ici_obj_t *k) ;
    virtual ici_obj_t   *       fetch_method(ici_obj_t *o, ici_obj_t *n);
    virtual int                 call(ici_obj_t *, ici_obj_t *);
    virtual int                 forall(ici_obj_t *o);
    virtual void                objname(ici_obj_t *, char [ICI_OBJNAMEZ]);

    ici_str_t *                 ici_name() const
    {
        if (_name == nullptr) _name = ici_str_new_nul_term(name);
        return _name;
    }

    /*
     * This is a convenience function which can be used directly as the 't_fetch'
     * entry in a type's 'type_t' struction if the type doesn't support
     * fetching.  It sets 'ici_error' to a message of the form:
     *
     *  attempt to read %s keyed by %
     *
     * and returns 1.  Also, it can b called from within a custom assign function
     * in cases where the particular fetch is illegal.
     *
     * This --func-- forms part of the --ici-api--.
     */
    static ici_obj_t *          fetch_fail(ici_obj_t *, ici_obj_t *);

    /*
     * This is a convenience function which can be used directly as the 't_assign'
     * entry in a type's 'type_t' struction if the type doesn't support
     * asignment.  It sets 'ici_error' to a message of the form:
     *
     *  attempt to set %s keyed by %s to %s
     *
     * and returns 1.  Also, it can b called from within a custom assign function
     * in cases where the particular assignment is illegal.
     *
     * This --func-- forms part of the --ici-api--.
     */
    static int                  assign_fail(ici_obj_t *, ici_obj_t *, ici_obj_t *);
};

/*
 * mark(o)              Must set the ICI_O_MARK flag in o->o_flags of this object
 *                      and all objects referenced by this one which don't
 *                      already have ICI_O_MARK set.  Returns the approximate
 *                      memory cost of this and all other objects it sets the
 *                      ICI_O_MARK of.  Typically recurses on all referenced
 *                      objects which don't already have ICI_O_MARK set (this
 *                      recursion is a potential problem due to the
 *                      uncontrolled stack depth it can create).  This is only
 *                      used in the marking phase of garbage collection.
 *
 *                      The function ici_mark() calls the mark function of the
 *                      object (based on object type) if the ICI_O_MARK flag of
 *                      the object is clear, else it returns 0.  This is the
 *                      usual interface to an object's mark function.
 *
 *                      The mark function implemantation of objects can assume
 *                      the ICI_O_MARK flag of the object they are being invoked
 *                      on is clear.
 *
 * free(o)              Must free the object o and all associated data, but not
 *                      other objects which are referenced from it.  This is
 *                      only called from garbage collection.  Care should be
 *                      taken to remember that errors can occur during object
 *                      creation and that the free function might be asked to
 *                      free a partially allocated object.
 *
 * cmp(o1, o2)          Must compare o1 and o2 and return 0 if they are the
 *                      same, else non zero.  This similarity is the basis for
 *                      merging objects into single atomic objects and the
 *                      implementation of the == operator.
 *
 *                      Currently only zero versus non-zero results are
 *                      significant.  However in future versions the cmp()
 *                      function may be generalised to return less than, equal
 *                      to, or greater than zero depending if 'o1' is less
 *                      than, equal to, or greater than 'o2'.  New
 *                      implementations would be wise to adopt this usage now.
 *
 *                      Some objects are by nature both unique and
 *                      intrinsically atomic (for example, objects which are
 *                      one-to-one with some other allocated data which they
 *                      alloc when the are created and free when they die).
 *                      For these objects the defaultimplementation
 *                      of this function.
 *
 *                      It is very important in implementing this function not
 *                      to miss any fields which may otherwise distinguish two
 *                      obejcts.  The cmp, hash and copy operations of an
 *                      object are all related.  It is useful to check that
 *                      they all regard the same data fields as significant in
 *                      performing their operation.
 *
 * copy(o)              Must return a copy of the given object.  This is the
 *                      basis for the implementation of the copy() function.
 *                      On failure, NULL is returned and error is set.  The
 *                      returned object has been ici_incref'ed.  The returned
 *                      object should cmp() as being equal, but be a distinct
 *                      object for objects that are not intrinsically atomic.
 *
 *                      Intrinsically atomic objects may use the default
 *                      implemenation of this function.
 *
 *                      Return NULL on failure, usual conventions.
 *
 * hash(o)              Must return an unsigned long hash which is sensitive
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
 *                      For these objects the default implementation of this
 *                      function should be used.
 *
 * assign(o, k, v)      Must assign to key 'k' of the object 'o' the value
 *                      'v'.  Return 1 on error, else 0.
 *
 *                      The static function 'assign_fail()' may be used
 *                      both as the implementation of this function for object
 *                      types which do not support any assignment, and as a
 *                      simple method of generating an error for particular
 *                      assignments which break some rule of the object.
 *
 *                      Not that it is not necessarilly wrong for an
 *                      intrinsically atomic object to support some form of
 *                      assignment.  Only for the modified field to be
 *                      significant in a 'cmp()' operation.  Objects which
 *                      are intrinsically unique and atomic often support
 *                      assignments.
 *
 *                      Return non-zero on failure, usual conventions.
 *
 * fetch(o, k)          Fetch the value of key 'k' of the object 'o'.  Return
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
 *                      The static function 'fetch_fail()' may be used
 *                      both as the implementation of this function for object
 *                      types which do not support any assignment, and as a
 *                      simple method of generating an error for particular
 *                      fetches which break some rule of the object.
 *
 *                      Return NULL on failure, usual conventions.
 *
 * name                 The name of this type. Use for the implementation of
 *                      'typeof()' and in error messages.  But apart from that,
 *                      type names have no fundamental importance in the
 *                      langauge and need not even be unique.
 *
 * objname(o, p)        Must place a short (less than 30 chars) human readable
 *                      representation of the object in the given buffer.
 *                      This is not intended as a basis for re-parsing or
 *                      serialisation.  It is just for diagnostics and debug.
 *                      An implementation of 'objname()' must not allocate
 *                      memory or otherwise allow the garbage collector to
 *                      run.  It is often used to generate formatted failure
 *                      messages after an error has occured, but before
 *                      cleanup has completed.
 *
 * call(o, s)           Must call the object 'o'.  If the object does not
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
 * ici_name             Returns an 'ici_str_t' copy of 'name'. This is just a
 *                      cached version so that typeof() doesn't keep re-computing
 *                      the string.
 *
 * fetch_method         An optional alternative to the basic 't_fetch()' that
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
 * forall_step          An optional alternative to the predefined type
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
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif
