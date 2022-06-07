// -*- mode:c++ -*-

#ifndef ICI_TYPE_H
#define ICI_TYPE_H

#include "fwd.h"

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

/*
 * Each type of object is defined by a, registered, instance of a
 * type derived from ici::type and overriding the various virtual
 * members to implement the type's behaviour.
 *
 * Type are registered in a global table and then represented as
 * a small integer index into that table. The index is embedded
 * in the header of each object and used to lookup the object's
 * corresponding type instance via the global types table.
 *
 * Implementations of new types define a class inheriting ici::type
 * and overriding the member functions required as per the new type.
 * (use the existing types as examples).  A single instance of the
 * type class is created using the instanceof() function (fwd.h) and
 * registered with the interpreter which allocates a type code for the
 * type. The user can then create objects corresponding with the type
 * using that type code.
 *
 * This --class-- forms part of the --ici-api--.
 */
class type
{
  public:
    /*
     * The 'name' member defines the name of this type and is used to
     * implement the 'typeof()' function and also in error messages.
     * Type names have no fundamental importance in the langauge and
     * need not even be unique.
     */
    const char *const name;

  private:
    const size_t _size;  // the size of this type's associated object structure
    const int    _flags; // type feature flags, see below.
    mutable str *_name;  // str version of name, created on demand (hence mutable).

  protected:
    /*
     * Flags are used to indicate that a type class overrides a
     * similarly named member function. Type flags are used to
     * detetermine behaviour in a number of places that do not want
     * the default implementation but do not override the
     * function. This mimics the original C code's use of NULL
     * function pointers as flags but without inheritence, it was
     * essentially implementing an optional<func *>.
     *
     * We do this explicitly using some flag bits and a few inline
     * functions to test them.  With everything const and constexpr
     * there should be ample opportunity for a smart-enough compiler
     * to optimize well.
     */
    static constexpr int has_fetch_method = 1 << 0;
    static constexpr int has_objname = 1 << 1;
    static constexpr int has_call = 1 << 2;

    /*
     * The constructor sets the base type information and has protected
     * access to force the use of the derived, actual, type classes.
     *
     * Type classes are constructed passing the type name, as a C
     * string, and the base size of the type's associated structure.
     */
    type(const char *name, size_t size, int flags = 0)
        : name(name)
        , _size(size)
        , _flags(flags)
        , _name(nullptr)
    {
    }

    /*
     * objectsize() returns the size of the type's associated object structure.
     */
    inline size_t objectsize() const
    {
        return _size;
    }

  public:
    virtual ~type();

    /*
     * Feature predicates.
     */
    inline bool can_fetch_method() const
    {
        return _flags & has_fetch_method;
    }
    inline bool can_objname() const
    {
        return _flags & has_objname;
    }
    inline bool can_call() const
    {
        return _flags & has_call;
    }

    /*
     * mark(o)              Must set the O_MARK flag in o->o_flags of this object
     *                      and all objects referenced by this one which don't
     *                      already have O_MARK set.  Returns the approximate
     *                      memory cost of this and all other objects it sets the
     *                      O_MARK of.  Typically recurses on all referenced
     *                      objects which don't already have O_MARK set (this
     *                      recursion is a potential problem due to the
     *                      uncontrolled stack depth it can create).  This is only
     *                      used in the marking phase of garbage collection.
     *
     *                      The function ici_mark() calls the mark function of the
     *                      object (based on object type) if the O_MARK flag of
     *                      the object is clear, else it returns 0.  This is the
     *                      usual interface to an object's mark function.
     *
     *                      The mark function implemantation of objects can assume
     *                      the O_MARK flag of the object they are being invoked
     *                      on is clear.
     */
    virtual size_t mark(object *);

    /*
     * free(o)              Must free the object o and all associated data, but not
     *                      other objects which are referenced from it.  This is
     *                      only called from garbage collection.  Care should be
     *                      taken to remember that errors can occur during object
     *                      creation and that the free function might be asked to
     *                      free a partially allocated object.
     */
    virtual void free(object *);

    /*
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
     */
    virtual int cmp(object *, object *);

    /* copy(o)              Must return a copy of the given object.  This is the
     *                      basis for the implementation of the copy() function.
     *                      On failure, nullptr is returned and error is set.  The
     *                      returned object has been ici_incref'ed.  The returned
     *                      object should cmp() as being equal, but be a distinct
     *                      object for objects that are not intrinsically atomic.
     *
     *                      Intrinsically atomic objects may use the default
     *                      implemenation of this function.
     *
     *                      Return nullptr on failure, usual conventions.
     */
    virtual object *copy(object *);
    /*
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
     */
    virtual unsigned long hash(object *);

    /*
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
     */
    virtual int assign(object *, object *, object *);

    /*
     * fetch(o, k)          Fetch the value of key 'k' of the object 'o'.  Return
     *                      nullptr on error.
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
     *                      Return nullptr on failure, usual conventions.
     *
     */
    virtual object *fetch(object *, object *);

    /* call(o, s)           Must call the object 'o'.  If the object does not
     *                      support being called, this should be nullptr.  If 's' is
     *                      non-null this is a method call and s is the subject
     *                      object of the call.  Return 1 on error, else 0.
     *                      The environment upon calling this function is
     *                      the same as that for intrinsic functions. Functions
     *                      and techniques that can be used in intrinsic function
     *                      implementations can be used in the implementation of
     *                      this function. The object being called can be assumed
     *                      to be on top of the operand stack
     *                      (i.e. os.a_top[-1])
     */
    virtual int call(object *, object *);

    /*
     * Assign into the super of an objwsup.
     */
    virtual int assign_super(object *, object *, object *, map *);

    /*
     * Fetch from  the super of an objwsup.
     */
    virtual int fetch_super(object *, object *, object **, map *);

    /*
     * Assign into the base of an objwsup.
     */
    virtual int assign_base(object *, object *, object *);

    /*
     * Fetch from the base of an objwsup.
     */
    virtual object *fetch_base(object *, object *);

    /* fetch_method         An optional alternative to the basic 't_fetch()' that
     *                      will be called (if supplied) when doing a fetch for
     *                      the purpose of forming a method.  This is really only
     *                      a hack to support COM under Windows.  COM allows
     *                      remote objects to have properties, like
     *                      object.property, and methods, like object:method().
     *                      But without this special hack, we can't tell if a
     *                      fetch operation is supposed to perform the COM get/set
     *                      property operation, or return a callable object for a
     *                      future method call.  Most objects will leave this
     *                      null.
     *
     *                      Return nullptr on failure, usual conventions.
     */
    virtual object *fetch_method(object *, object *);

    /* forall               An optional alternative to the predefined type
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
     */
    virtual int forall(object *);

    /*
     * nkeys(o)		    Return the number of key objects this object
     *			    contains, i.e. the number of objects the keys()
     *			    function will return when applied to the object.
     *			    The default implementation returns 0.
     */
    virtual int nkeys(object *);

    /* keys(o, a)	    Append the key values of this object contains
     *			    to the given array. Usual error conventions,
     *			    returns non-zero upon failure.
     *			    The default implementation returns the error
     *		    	    "attempt to obtains keys from a value of type xxx"
     *			    where "xxx" is replaced by the type's name.
     */
    virtual int keys(object *, array *);

    /* objname(o, p)        Must place a short (less than 30 chars) human readable
     *                      representation of the object in the given buffer.
     *                      This is not intended as a basis for re-parsing or
     *                      serialisation.  It is just for diagnostics and debug.
     *                      An implementation of 'objname()' must not allocate
     *                      memory or otherwise allow the garbage collector to
     *                      run.  It is often used to generate formatted failure
     *                      messages after an error has occured, but before
     *                      cleanup has completed.
     */
    virtual void objname(object *, char[objnamez]);

    /*
     * save(ar, o)          Writes an encoded representation of the object using
     *                      the supplied archiver object. Returns 0 on success,
     *                      non-zero on error, usual semantics.
     */
    virtual int save(archiver *, object *);

    /*
     * restore(ar)          Reads an encoded representation of an object of this
     *                      type using the supplied archiver object. Returns a
     *                      object on success, nullptr on error.
     */
    virtual object *restore(archiver *);

    /*
     * len(o)               Returns the "length" of an object of this type.
     *                      The default implementation returns 1.
     */
    virtual int64_t len(object *);

    /*
     * The ici_name function returns the type's name as an ICI string object,
     * created upon demand. As per convention this may return a nullptr if
     * string object creation fails (out of memory).
     */
    str *ici_name() const
    {
        if (_name == nullptr)
            _name = new_str_nul_term(name);
        return _name;
    }

    /*
     * Frees resources associated with the type at uninit() time.
     */
    void uninit();

    /*
     * This is a convenience function which can be used to implement
     * 'fetch' if the type doesn't support fetching.  It sets the
     * 'error' to a message of the form:
     *
     *  attempt to read %s keyed by %
     *
     * and returns 1.  Also, it can b called from within a custom assign function
     * in cases where the particular fetch is illegal.
     *
     * This --func-- forms part of the --ici-api--.
     */
    static object *fetch_fail(object *, object *);

    /*
     * This is a convenience function which can be used to implement
     * 'assign' if the type doesn't support assignment.  It sets the
     * 'error' to a message of the form:
     *
     *  attempt to set %s keyed by %s to %s
     *
     * and returns 1.  Also, it can be called from within a custom assign
     * functions where the particular assignment is illegal.
     *
     * This --func-- forms part of the --ici-api--.
     */
    static int assign_fail(object *, object *, object *);

    /*
     * This is a convenience function which can be used to implement
     * 'save' if the type doesn't support archiving.  It sets the
     * 'error' to a message of the form:
     *
     *  attempt to save a %s
     *
     * and returns 1.  Also, it can be called from within a custom save
     * functions where the particular operation is illegal.
     *
     * This --func-- forms part of the --ici-api--.
     */
    static int save_fail(archiver *, object *);

    /*
     * This is a convenience function which can be used to implement
     * 'restore' if the type doesn't support archiving.  It sets the
     * 'error' to a message of the form:
     *
     *  attempt to restore a %s
     *
     * and returns 1.  Also, it can be called from within a custom
     * restore functions where the particular operation is illegal.
     *
     * This --func-- forms part of the --ici-api--.
     */
    static object *restore_fail(const char *);
};

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif
