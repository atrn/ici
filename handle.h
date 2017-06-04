#ifndef ICI_HANDLE_H
#define ICI_HANDLE_H

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
 * See also 'ici_handle_new()'.
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
 *                      NULL if not needed.
 *
 * h_member_map         An optional map (NULL if not needed) as made by
 *                      'ici_make_handle_member_map()' and used internally
 *                      when the 'h_member_intf' function is used.
 *
 * h_member_intf        An optional function (NULL if not needed) to implement
 *                      property access and method invocation on the object.
 *                      'ptr' is the 'h_ptr' field of the handle.  The
 *                      implementation must know which 'id' values apply to
 *                      methods, and which to properties.  When the 'id'
 *                      refers to a method, the usual environment for
 *                      intrinsic function invocations can be assumed (e.g.
 *                      'ici_typecheck()' is available) except the return
 *                      value should be stored through '*retv' without any
 *                      extra reference count.
 *
 *                      When the 'id' refers to a property, if 'setv' is
 *                      non-NULL, this is an assignment of 'setv' to the
 *                      property.  If the assignment is possible and proceeds
 *                      without error, 'setv' should be assigned to '*retv'
 *                      prior to return (else '*retv' should be unmodified).
 *
 *                      When the 'id' refers to a property and 'setv' is NULL,
 *                      this is a fetch, and '*retv' should be set to the
 *                      value, without any extra reference count.
 *
 *                      In all cases, 0 indicates a successful return
 *                      (although if '*retv' has not been updated, it will be
 *                      assumed that the 'id' was not actually a member of
 *                      this object and an error may be raised by the calling
 *                      code).  Non-zero on error, usual conventions.
 *
 * h_general_intf       An optional function (NULL if not needed) to implement
 *                      general fetch and assign processing on the handle,
 *                      even when the keys are not known in advance (as might
 *                      happen, for example, if the object could be indexed by
 *                      integers).  If 'h_member_intf' is non-NULL, and
 *                      satisfied a fetch or assign first, this function is
 *                      not called.
 *
 *                      If 'setv' is non-NULL, this is an assignment.  If the
 *                      assignment is to a key ('k') that is valid and the
 *                      assignment is successful, '*retv' should be updated
 *                      with 'setv'.
 *
 *                      If 'setv' is NULL, this is a fetch, and '*retv' should
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
struct ici_handle : ici_objwsup
{
    void            *h_ptr;
    ici_str_t       *h_name;
    void            (*h_pre_free)(ici_handle_t *h);
    ici_obj_t       *h_member_map;
    int             (*h_member_intf)(void *ptr, int id, ici_obj_t *setv, ici_obj_t **retv);
    int             (*h_general_intf)(ici_handle_t *h, ici_obj_t *k, ici_obj_t *setv, ici_obj_t **retv);
};

#define ici_handleof(o)        ((ici_handle_t *)(o))
#define ici_ishandle(o)        (ici_objof(o)->o_tcode == ICI_TC_HANDLE)
#define ici_ishandleof(o, n)   (ici_ishandle(o) && ici_handleof(o)->h_name == (n))
/*
 * Flags set in the upper nibble of o_flags, which is
 * allowed for type specific use.
 */
#define ICI_H_CLOSED                0x20
#define ICI_H_HAS_PRIV_STRUCT       0x40
/*
 * ICI_H_CLOSED             If set, the thing h_ptr points to is no longer
 *                      valid (it has probably been freed). This flag
 *                      exists for the convenience of users, as the
 *                      core handle code doesn't touch this much.
 *                      Use of this latent feature depends on needs.
 *
 * ICI_H_HAS_PRIV_STRUCT    This handle has had a private struct allocated
 *                      to hold ICI values that have been assigned to
 *                      it. This does not happen until required, as
 *                      not all handles will ever need one. The super
 *                      is the private struct (and it's super is the
 *                      super the creator originally supplied).
 */

struct ici_name_id
{
    char    *ni_name;
    long    ni_id;
};
#define ICI_H_METHOD    0x8000000

/*
 * End of ici.h export. --ici.h-end--
 */

#endif /* ICI_HANDLE_H */
