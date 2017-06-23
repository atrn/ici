#define ICI_CORE
#include "fwd.h"
#include "object.h"
#include "string.h"
#include "handle.h"
#include "buf.h"
#include "null.h"
#include "array.h"
#include "str.h"
#include "cfunc.h"
#include "int.h"
#include "struct.h"

namespace ici
{

static ici_handle_t ici_handle_proto;

/*
 * Return a handle object corresponding to the given C data 'ptr', with the
 * ICI type 'name' (which may be NULL), and with the given 'super' (which
 * may be NULL).
 *
 * The returned handle will have had its reference count inceremented.
 *
 * ICI handle objects are generic wrapper/interface objects around some C data
 * structure. They act, on the ICI side, as objects with the type 'name'.
 * When you are passed a handle back from ICI code, you can check this name
 * to prevent the ICI program from giving you some other data type's handle.
 * (You can't make handles at the script level, so you are safe from all
 * except other native code mimicing your type name.)
 *
 * Handles are intrinsicly atomic with respect to the 'ptr' and 'name'. So
 * this function actually just finds the existing handle of the given data
 * object if that handle already exists.
 *
 * Handle's will, of course, be garbage collected as usual.  If your C data is
 * dependent on the handle, you should store a pointer to a free function
 * for your data in the 'h_pre_free' field of the handle.  It will be called
 * just before the gardbage collector frees the memory of the handle.
 *
 * If, on the other hand, your C data structure is the master structure and it
 * might be freed by some other aspect of your code, you must consider that
 * its handle object may still be referenced from ICI code.  You don't want to
 * have it passed back to you and inadvertently try to access your freed data.
 * To prevent this you can set the ICI_H_CLOSED flag in the handle's object header
 * when you free the C data (see 'ici_handle_probe()').  Note that in
 * callbacks where you are passed the handle object directly, you are
 * reponsible to checking ICI_H_CLOSED.  Also, once you use this mechanism, you
 * must *clear* the ICI_H_CLOSED field after a real new handle allocation (because
 * you might be reusing the old memory, and this function might be returning
 * to you a zombie handle).
 *
 * Handles can support assignment to fields "just like a struct" by
 * the automatic creation of a private struct to store such values in upon
 * first assignment. This mechanism is, by default, only enabled if you
 * supply a non-NULL super. But you can enable it even with a NULL super
 * by setting ICI_O_SUPER in the handle's object header at any time. (Actually,
 * it is an historical accident that 'super' was ever an argument to this
 * function.)
 *
 * Handles can support an interface function that allows C code to implement
 * fetch and assign operations, as well as method invocation on fields of the
 * handle.  See the 'h_member_intf' in the 'ici_handle_t' type description
 * (and the 'Common tasks' section of this chapter.)
 *
 * Handles can also be used as instances of an ICI class.  Typically the class
 * will have the methods that operate on the handle.  In this case you will
 * pass the class in 'super'.  Instance variables will be supported by the
 * automatic creation of the private struct to hold them (which allows the
 * class to be extended in ICI with additional instance data that is not part
 * of your C code).  However, note that these instance variables are not
 * "magic".  Your C code does not notice them getting fetched or assigned to.
 *
 * This --func-- forms part of the --ici-api--.
 */
ici_handle_t *
ici_handle_new(void *ptr, ici_str_t *name, ici_objwsup_t *super)
{
    ici_handle_t        *h;
    ici_obj_t           **po;

    ici_handle_proto.h_ptr = ptr;
    ici_handle_proto.h_name = name;
    ici_handle_proto.o_super = super;
    if ((h = ici_handleof(ici_atom_probe2(&ici_handle_proto, &po))) != NULL)
    {
        h->incref();
        return h;
    }
    ++ici_supress_collect;
    if ((h = ici_talloc(ici_handle_t)) == NULL)
        return NULL;
    ICI_OBJ_SET_TFNZ(h, ICI_TC_HANDLE, (super != NULL ? ICI_O_SUPER : 0) | ICI_O_ATOM, 1, 0);
    h->h_ptr = ptr;
    h->h_name = name;
    h->o_super = super;
    h->h_pre_free = NULL;
    h->h_member_map = NULL;
    h->h_member_intf = NULL;
    h->h_general_intf = NULL;
    ici_rego(h);
    --ici_supress_collect;
    ICI_STORE_ATOM_AND_COUNT(po, h);
    return h;
}

/*
 * If it exists, return a pointer to the handle corresponding to the C data
 * structure 'ptr' with the ICI type 'name'.  If it doesn't exist, return
 * NULL.  The handle (if returned) will have been increfed.
 *
 * This function can be used to probe to see if there is an ICI handle
 * associated with your C data structure in existence, but avoids allocating
 * it if does not exist already (as 'ici_handle_new()' would do).  This can be
 * useful if you want to free your C data structure, and need to mark any ICI
 * reference to the data by setting ICI_H_CLOSED in the handle's object header.
 *
 * This --func-- forms part of the --ici-api--.
 */
ici_handle_t *
ici_handle_probe(void *ptr, ici_str_t *name)
{
    ici_handle_t        *h;

    ici_handle_proto.h_ptr = ptr;
    ici_handle_proto.h_name = name;
    if ((h = ici_handleof(ici_atom_probe(&ici_handle_proto))) != NULL)
        h->incref();
    return h;
}

/*
 * Verify that a method on a handle has been invoked correctly.  In
 * particular, that 'inst' is not NULL and is a handle with the given 'name'.
 * If OK and 'h' is non-NULL, the handle is stored through it.  If 'p' is
 * non-NULL, the associted pointer ('h_ptr') is stored through it.  Return 1
 * on error and sets ici_error, else 0.
 *
 * For example, a typical method where the instance should be a handle
 * of type 'XML_Parse' might look like this:
 *
 *  static int
 *  ici_xml_SetBase(ici_obj_t *inst)
 *  {
 *      char                *s;
 *      XML_Parser          p;
 *
 *      if (ici_handle_method_check(inst, ICIS(XML_Parser), NULL, &p))
 *          return 1;
 *      if (typecheck("s", &s))
 *          return 1;
 *      if (!XML_SetBase(p, s))
 *          return ici_xml_error(p);
 *      return ici_null_ret();
 *  }
 *
 *
 * This --func-- forms part of the --ici-api--.
 */
int
ici_handle_method_check(ici_obj_t *inst, ici_str_t *name, ici_handle_t **h, void **p)
{
    char                n1[30];
    char                n2[30];

    if (ici_method_check(inst, ICI_TC_HANDLE))
        return 1;
    if (ici_handleof(inst)->h_name != name)
    {
        return ici_set_error("attempt to apply method %s to %s",
                             ici_objname(n1, ici_os.a_top[-1]),
                             ici_objname(n2, inst));
    }
    if (h != NULL)
        *h = ici_handleof(inst);
    if (p != NULL)
        *p = ici_handleof(inst)->h_ptr;
    return 0;
}

/*
 * The function used to field method calls that go through the memeber map
 * mechanism.  Checks conditions and transfers to the h_memeber_intf function
 * of the handle.
 */
static int
ici_handle_method(ici_obj_t *inst)
{
    ici_obj_t           *r;
    char                n1[30];
    char                n2[30];
    long                id;

    if (ici_method_check(inst, ICI_TC_HANDLE))
        return 1;
    if (inst->flag(ICI_H_CLOSED))
    {
        return ici_set_error("attempt to apply method %s to %s which is dead",
                             ici_objname(n1, ici_os.a_top[-1]),
                             ici_objname(n2, inst));
    }
    r = NULL;
    id = (long)ici_cfuncof(ici_os.a_top[-1])->cf_arg1;
    if ((*ici_handleof(inst)->h_member_intf)(ici_handleof(inst)->h_ptr, id, NULL, &r))
        return 1;
    if (r == NULL)
    {
        return ici_set_error("attempt to apply method %s to %s",
                             ici_objname(n1, ici_os.a_top[-1]),
                             ici_objname(n2, inst));
    }
    return ici_ret_no_decref(r);
}

/*
 * Build the map that 'ici_handle_t' objects use to map a member name (used in
 * ICI code) to an integer ID (used in the C code). The returned map is actually
 * an ICI struct. It is returned with 1 refernce count.
 *
 * The argument 'ni' should be a pointer to the first element of an arrary
 * of 'ici_name_id_t' structs that contain the names of members and the integer
 * IDs that your code would like to refere to them by. All members that are
 * to be invoked as methods calls must include the flag ICI_H_METHOD in the ID.
 * (This flag is removed from the ID when it is passed back to your code. ICI_H_METHOD
 * is the most significant bit in the 32 bit ID.) The list is terminated by an
 * entry with a name of NULL.
 *
 * For example:
 *
 *  enum {P_Property1, P_Property2, M_Method1, M_Method2, ...};
 *
 *  static ici_name_id_t member_name_ids[] =
 *  {
 *      {"Property1",        P_Property1},
 *      {"Property2",        P_Property1},
 *      {"Method1",          M_Method1},
 *      {"Method2",          M_Method2},
 *      {NULL},
 *  }
 *
 *  ici_obj_t   *ici_member_map;
 *
 *  ...
 *      ici_member_map = ici_make_handle_member_map(member_name_ids)
 *      if (ici_member_map == NULL)
 *          ...
 *
 * This --func-- forms part of the --ici-api--.
 */
ici_obj_t *
ici_make_handle_member_map(ici_name_id_t *ni)
{
    ici_obj_t           *m;
    ici_str_t           *n;
    ici_obj_t           *id;

    if ((m = ici_struct_new()) == NULL)
        return NULL;
    for (; ni->ni_name != NULL; ++ni)
    {
        id = NULL;
        if ((n = ici_str_new_nul_term(ni->ni_name)) == NULL)
            goto fail;
        if (ni->ni_id & ICI_H_METHOD)
        {
            id = (ici_obj_t *)ici_cfunc_new
            (
                n,
                (int (*)(...))(ici_handle_method),
                (void *)(ni->ni_id & ~ICI_H_METHOD),
                NULL
            );
            if (id == NULL)
                goto fail;
        }
        else
        {
            if ((id = ici_int_new(ni->ni_id)) == NULL)
                goto fail;
        }
        if (ici_assign(m, n, id))
            goto fail;
        n->decref();
        id->decref();
    }
    return m;

 fail:
    if (n != NULL)
        n->decref();
    if (id != NULL)
        id->decref();
    m->decref();
    return NULL;
}


void handle_type::objname(ici_obj_t *o, char p[ICI_OBJNAMEZ])
{
    if (ici_handleof(o)->h_name == NULL)
        strcpy(p, "handle");
    else
    {
        if (ici_handleof(o)->h_name->s_nchars > ICI_OBJNAMEZ - 1)
            sprintf(p, "%.*s...", ICI_OBJNAMEZ - 4, ici_handleof(o)->h_name->s_chars);
        else
            sprintf(p, "%s", ici_handleof(o)->h_name->s_chars);
    }
}

size_t handle_type::mark(ici_obj_t *o)
{
    o->setmark();
    auto mem = typesize();
    if (ici_objwsupof(o)->o_super != NULL)
        mem += ici_mark(ici_objwsupof(o)->o_super);
    if (ici_handleof(o)->h_name != NULL)
        mem += ici_mark(ici_handleof(o)->h_name);
    return mem;
}

int handle_type::cmp(ici_obj_t *o1, ici_obj_t *o2)
{
    return ici_handleof(o1)->h_ptr != ici_handleof(o2)->h_ptr
    || ici_handleof(o1)->h_name != ici_handleof(o2)->h_name;
}

unsigned long handle_type::hash(ici_obj_t *o)
{
    return ICI_PTR_HASH(ici_handleof(o)->h_ptr)
    ^ ICI_PTR_HASH(ici_handleof(o)->h_name);
}

ici_obj_t * handle_type::fetch(ici_obj_t *o, ici_obj_t *k)
{
    ici_handle_t        *h;
    ici_obj_t           *r;

    h = ici_handleof(o);
    if (h->h_member_map != NULL && !o->flag(ICI_H_CLOSED))
    {
        ici_obj_t       *id;

        if ((id = ici_fetch(h->h_member_map, k)) == NULL)
            return NULL;
        if (ici_iscfunc(id))
            return id;
        if (ici_isint(id))
        {
            r = NULL;
            if ((*h->h_member_intf)(h->h_ptr, ici_intof(id)->i_value, NULL, &r))
                return NULL;
            if (r != NULL)
                return r;
        }
    }
    if (h->h_general_intf != NULL)
    {
        r = NULL;
        if ((*h->h_general_intf)(h, k, NULL, &r))
            return NULL;
        if (r != NULL)
            return r;
    }
    if (!ici_hassuper(o) || ici_handleof(o)->o_super == NULL)
        return fetch_fail(o, k);
    return ici_fetch(ici_handleof(o)->o_super, k);
}

/*
 * Do a fetch where we are the super of some other object that is
 * trying to satisfy a fetch. Don't regard the item k as being present
 * unless it really is. Return -1 on error, 0 if it was not found,
 * and 1 if was found. If found, the value is stored in *v.
 *
 * If not NULL, b is a struct that was the base element of this
 * assignment. This is used to mantain the lookup lookaside mechanism.
 */
int handle_type::fetch_super(ici_obj_t *o, ici_obj_t *k, ici_obj_t **v, ici_struct_t *b)
{
    if (!ici_hassuper(o))
    {
        fetch_fail(o, k);
        return 1;
    }
    if (ici_handleof(o)->o_super == NULL)
        return 0;
    return ici_fetch_super(ici_handleof(o)->o_super, k, v, b);
}

ici_obj_t * handle_type::fetch_base(ici_obj_t *o, ici_obj_t *k)
{
    ici_handle_t        *h;
    ici_obj_t           *r;

    h = ici_handleof(o);
    if (h->h_member_map != NULL && !o->flag(ICI_H_CLOSED))
    {
        ici_obj_t       *id;

        if ((id = ici_fetch(h->h_member_map, k)) == NULL)
            return NULL;
        if (ici_iscfunc(id))
            return id;
        if (ici_isint(id))
        {
            r = NULL;
            if ((*h->h_member_intf)(h->h_ptr, ici_intof(id)->i_value, NULL, &r))
                return NULL;
            if (r != NULL)
                return r;
        }
    }
    if (h->h_general_intf != NULL)
    {
        r = NULL;
        if ((*h->h_general_intf)(h, k, NULL, &r))
            return NULL;
        if (r != NULL)
            return r;
    }
    if (!ici_hassuper(o))
        return fetch_fail(o, k);
    if (!o->flag(ICI_H_HAS_PRIV_STRUCT))
        return ici_null;
    return ici_fetch_base(h->o_super, k);
}

/*
 * Assign a value into a key of object o, but ignore the super chain.
 * That is, always assign into the lowest level. Usual error coventions.
 */
int handle_type::assign_base(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v)
{
    ici_handle_t        *h;
    ici_obj_t           *r;

    h = ici_handleof(o);
    if (h->h_member_map != NULL && !o->flag(ICI_H_CLOSED))
    {
        ici_obj_t       *id;

        if ((id = ici_fetch(h->h_member_map, k)) == NULL)
            return 1;
        if (ici_isint(id))
        {
            r = NULL;
            if ((*h->h_member_intf)(h->h_ptr, ici_intof(id)->i_value, v, &r))
                return 1;
            if (r != NULL)
                return 0;
        }
    }
    if (h->h_general_intf != NULL)
    {
        r = NULL;
        if ((*h->h_general_intf)(h, k, v, &r))
            return 1;
        if (r != NULL)
            return 0;
    }
    if (!ici_hassuper(o))
        return assign_fail(o, k, v);
    if (!o->flag(ICI_H_HAS_PRIV_STRUCT))
    {
        ici_objwsup_t   *s;

        /*
         * We don't yet have a private struct to hold our values.
         * Give ourselves one.
         *
         * This operation disturbs the struct-lookup lookaside mechanism.
         * We invalidate all existing entries by incrementing ici_vsver.
         */
        if ((s = ici_objwsupof(ici_struct_new())) == NULL)
            return 1;
        s->o_super = ici_objwsupof(o)->o_super;
        ici_objwsupof(o)->o_super = s;
        ++ici_vsver;
        o->setflag(ICI_H_HAS_PRIV_STRUCT);
    }
    return ici_assign_base(ici_objwsupof(o)->o_super, k, v);
}

/*
 * Assign to key k of the object o the value v. Return 1 on error, else 0.
 * See the comment on t_assign() in object.h.
 */
int handle_type::assign(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v)
{
    ici_handle_t        *h;
    ici_obj_t           *r;

    h = ici_handleof(o);
    r = NULL;
    if (h->h_member_map != NULL && !o->flag(ICI_H_CLOSED))
    {
        ici_obj_t       *id;

        if ((id = ici_fetch(h->h_member_map, k)) == NULL)
            return 1;
        if (ici_isint(id))
        {
            if ((*h->h_member_intf)(h->h_ptr, ici_intof(id)->i_value, v, &r))
                return 1;
            if (r != NULL)
                return 0;
        }
    }
    if (h->h_general_intf != NULL)
    {
        r = NULL;
        if ((*h->h_general_intf)(h, k, v, &r))
            return 1;
        if (r != NULL)
            return 0;
    }
    if (!ici_hassuper(o))
        return assign_fail(o, k, v);
    if (o->flag(ICI_H_HAS_PRIV_STRUCT))
        return ici_assign(h->o_super, k, v);
    /*
     * We don't have a base struct of our own yet. Try the super.
     */
    if (ici_handleof(o)->o_super != NULL)
    {
        switch (ici_assign_super(h->o_super, k, v, NULL))
        {
        case -1: return 1;
        case 1:  return 0;
        }
    }
    /*
     * We failed to assign the value to a super, and we haven't yet got
     * a private struct. Assign it to the base. This will create our
     * private struct.
     */
    return assign_base(o, k, v);
}

/*
 * Do an assignment where we are the super of some other object that
 * is trying to satisfy an assign. Don't regard the item k as being
 * present unless it really is. Return -1 on error, 0 if not found
 * and 1 if the assignment was completed.
 *
 * If 0 is returned, nothing has been modified during the
 * operation of this function.
 *
 * If not NULL, b is a struct that was the base element of this
 * assignment. This is used to mantain the lookup lookaside mechanism.
 */
int handle_type::assign_super(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v, ici_struct_t *b)
{
    if (!ici_hassuper(o))
        return assign_fail(o, k, v);
    if (ici_handleof(o)->o_super == NULL)
        return 0;
    return ici_assign_super(ici_handleof(o)->o_super, k, v, b);
}

/*
 * Free this object and associated memory (but not other objects).
 * See the comments on t_free() in object.h.
 */
void handle_type::free(ici_obj_t *o)
{
    if (ici_handleof(o)->h_pre_free != NULL)
        (*ici_handleof(o)->h_pre_free)(ici_handleof(o));
    ici_tfree(o, ici_handle_t);
}

} // namespace ici
