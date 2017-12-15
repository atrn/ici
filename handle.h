// -*- mode:c++ -*-

#ifndef ICI_HANDLE_H
#define ICI_HANDLE_H

namespace ici
{


/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

/*
 * The C struct which is the ICI handle object.  A handle is a generic object
 * that can be used to refer to some C data object.  Handles support an
 * (optional) super pointer.  Handles are named with an ICI string to give
 * type checking, reporting, and diagnostic support.  The handle object
 * provides most of the generic machinery of ICI objects.  An optional
 * pre-free function pointer can be supplied to handle cleanup on final
 * collection of the handle.
 *
 * See also 'new_handle()'.
 *
 * h_ptr                The pointer to the primitive data object that his
 *                      handle is associated with.
 *
 * h_name               The type name this handle will appear to have from ICI
 *                      script code, and for type checking in interfacing with
 *                      C.
 *
 * h_pre_free           An optional function that will be called just before
 *                      this handle object is freed by the garbage collector.
 *                      nullptr if not needed.
 *
 * h_member_map         An optional map (nullptr if not needed) as made by
 *                      'make_handle_member_map()' and used internally
 *                      when the 'h_member_intf' function is used.
 *
 * h_member_intf        An optional function (nullptr if not needed) to implement
 *                      property access and method invocation on the object.
 *                      'ptr' is the 'h_ptr' field of the handle.  The
 *                      implementation must know which 'id' values apply to
 *                      methods, and which to properties.  When the 'id'
 *                      refers to a method, the usual environment for
 *                      intrinsic function invocations can be assumed (e.g.
 *                      'typecheck()' is available) except the return
 *                      value should be stored through '*retv' without any
 *                      extra reference count.
 *
 *                      When the 'id' refers to a property, if 'setv' is
 *                      non-nullptr, this is an assignment of 'setv' to the
 *                      property.  If the assignment is possible and proceeds
 *                      without error, 'setv' should be assigned to '*retv'
 *                      prior to return (else '*retv' should be unmodified).
 *
 *                      When the 'id' refers to a property and 'setv' is nullptr,
 *                      this is a fetch, and '*retv' should be set to the
 *                      value, without any extra reference count.
 *
 *                      In all cases, 0 indicates a successful return
 *                      (although if '*retv' has not been updated, it will be
 *                      assumed that the 'id' was not actually a member of
 *                      this object and an error may be raised by the calling
 *                      code).  Non-zero on error, usual conventions.
 *
 * h_general_intf       An optional function (nullptr if not needed) to implement
 *                      general fetch and assign processing on the handle,
 *                      even when the keys are not known in advance (as might
 *                      happen, for example, if the object could be indexed by
 *                      integers).  If 'h_member_intf' is non-nullptr, and
 *                      satisfied a fetch or assign first, this function is
 *                      not called.
 *
 *                      If 'setv' is non-nullptr, this is an assignment.  If the
 *                      assignment is to a key ('k') that is valid and the
 *                      assignment is successful, '*retv' should be updated
 *                      with 'setv'.
 *
 *                      If 'setv' is nullptr, this is a fetch, and '*retv' should
 *                      be set to the fetched value.
 *
 *                      In both cases, no extra reference should be given to
 *                      the returned object.
 *
 *                      In both cases, 0 indicates a successful return
 *                      (although if '*retv' has not been updated, it will be
 *                      assumed that the key was not actually a member of
 *                      this object and an error may be raised by the calling
 *                      code).  Non-zero on error, usual conventions.
 *
 * This --struct-- forms part of the --ici-api--.
 */
struct handle : objwsup
{
    handle() : objwsup{TC_HANDLE, 0, 1, 0}
             , h_ptr(nullptr)
             , h_name(nullptr)
             , h_pre_free(nullptr)
             , h_member_map(nullptr)
             , h_member_intf(nullptr)
             , h_general_intf(nullptr)
    {}

    void    *h_ptr;
    str     *h_name;
    void    (*h_pre_free)(handle *h);
    object   *h_member_map;
    int     (*h_member_intf)(void *ptr, int id, object *setv, object **retv);
    int     (*h_general_intf)(handle *h, object *k, object *setv, object **retv);
};

inline handle *handleof(object *o) { return o->as<handle>(); }
inline bool ishandle(object *o) { return o->isa(TC_HANDLE); }
inline bool ishandleof(object *o, str *n) { return ishandle(o) && handleof(o)->h_name == n; }

/*
 * Flags set in the upper nibble of o_flags, which is
 * allowed for type specific use.
 */
constexpr int ICI_H_CLOSED =  0x20;
constexpr int ICI_H_HAS_PRIV_MAP = 0x40;
/*
 * ICI_H_CLOSED             If set, the thing h_ptr points to is no longer
 *                      valid (it has probably been freed). This flag
 *                      exists for the convenience of users, as the
 *                      core handle code doesn't touch this much.
 *                      Use of this latent feature depends on needs.
 *
 * ICI_H_HAS_PRIV_MAP    This handle has had a private map allocated
 *                      to hold ICI values that have been assigned to
 *                      it. This does not happen until required, as
 *                      not all handles will ever need one. The super
 *                      is the private struct (and it's super is the
 *                      super the creator originally supplied).
 */

struct name_id
{
    const char  *ni_name;
    long        ni_id;
};
constexpr int ICI_H_METHOD = 0x8000000;

/*
 * End of ici.h export. --ici.h-end--
 */

class handle_type : public type
{
public:
    handle_type() : type("handle", sizeof (struct handle), type::has_objname) {}

    size_t mark(object *o) override;
    void free(object *o) override;
    unsigned long hash(object *o) override;
    int cmp(object *o1, object *o2) override;
    object *fetch(object *o, object *k) override;
    int fetch_super(object *o, object *k, object **v, map *b) override;
    object *fetch_base(object *o, object *k) override;
    int assign_base(object *o, object *k, object *v) override;
    int assign(object *o, object *k, object *v) override;
    int assign_super(object *o, object *k, object *v, map *b) override;
    void objname(object *o, char p[objnamez]) override;
};

} // namespace ici

#endif /* ICI_HANDLE_H */
