/*
 * A doubly linked list data type for ICI.
 *
 * This --intro-- forms part of the --ici-list-- documentation.
 */
#define ICI_NO_OLD_NAMES

#include <ici.h>
#include "icistr.h"
#include <icistr-setup.h>

#include <assert.h>

typedef struct listnode_s listnode_t;
struct listnode_s
{
    ici_obj_t	o_head;
    listnode_t	*ln_next;
    listnode_t	*ln_prev;
    ici_obj_t	*ln_obj;
};

#define	islistnode(o)	((o)->o_tcode == listnode_tcode)
#define	listnodeof(o)	((listnode_t *)(o))

/*
 * Lists store references to listnode along with some other information
 * to speed access to elements.
 */
typedef struct list_s
{
    ici_obj_t	 o_head;
    listnode_t	*l_head;
    listnode_t	*l_tail;
    listnode_t	*l_ptr;
    long	 l_idx;
    unsigned int l_nels;
}
list_t;

#define	islist(o)	((o)->o_tcode == list_tcode)
#define	listof(o)	((list_t *)(o))

enum
{
    ICI_LIST_INDEX_INVALID = -2,
};

static int listnode_tcode;
static int list_tcode;

static unsigned long
mark_listnode(ici_obj_t *o)
{
    o->o_flags |= ICI_O_MARK;
    return sizeof (listnode_t) + ici_mark(listnodeof(o)->ln_obj);
}

static void
free_listnode(ici_obj_t *o)
{
    ici_tfree(o, sizeof (listnode_t));
}

static ici_obj_t *
fetch_listnode(ici_obj_t *o, ici_obj_t *k)
{
    listnode_t	*ln = listnodeof(o);

    if (k == ici_objof(ICIS(obj)))
	return ln->ln_obj;
    if (k == ici_objof(ICIS(next)))
	return ln->ln_next ?  ici_objof(ln->ln_next) : ici_null;
    if (k == ici_objof(ICIS(prev)))
	return ln->ln_prev ? ici_objof(ln->ln_prev) : ici_null;
    return ici_fetch_fail(o, k);
}

ici_type_t listnode_type =
{
    mark_listnode,
    free_listnode,
    ici_hash_unique,
    ici_cmp_unique,
    ici_copy_simple,
    ici_assign_fail,
    fetch_listnode,
    "listnode"
};

listnode_t *
ici_new_listnode(ici_obj_t *o)
{
    listnode_t	*ls;

    if ((ls = ici_talloc(listnode_t)) != NULL)
    {
        ICI_OBJ_SET_TFNZ(ls, listnode_tcode, 0, 1, 0);
	ls->ln_next = NULL;
	ls->ln_prev = NULL;
	ls->ln_obj  = o;
	ici_rego(ici_objof(ls));
    }
    return ls;
}

static unsigned long
mark_list(ici_obj_t *o)
{
    listnode_t		*ls;
    unsigned long	sz;

    o->o_flags |= ICI_O_MARK;
    sz = sizeof (list_t);
    for (ls = listof(o)->l_head; ls != NULL; ls = ls->ln_next)
	sz += ici_mark(ls);
    return sz;
}

static void
free_list(ici_obj_t *o)
{
    ici_tfree(o, sizeof (list_t));
}

static ici_obj_t *
copy_list(ici_obj_t *o)
{
    list_t	*l;
    listnode_t	*ls;
    listnode_t	*nls;

    if ((l = ici_talloc(list_t)) == NULL)
	return NULL;
    ICI_OBJ_SET_TFNZ(l, list_tcode, 0, 1, 0);
    ici_rego(l);
    l->l_head = NULL;
    l->l_tail = NULL;
    l->l_nels = listof(o)->l_nels;
    l->l_ptr  = NULL;
    l->l_idx  = ICI_LIST_INDEX_INVALID;
    for (ls = listof(o)->l_head; ls != NULL; ls = ls->ln_next)
    {
	if ((nls = ici_new_listnode(ls->ln_obj)) == NULL)
	    goto fail;
	if (l->l_tail == NULL)
	{
	    l->l_head = nls;
	    l->l_tail = nls;
	}
	else
	{
	    nls->ln_prev = l->l_tail;
	    l->l_tail = nls;
	}
	ici_decref(nls);
    }
    return ici_objof(l);

fail:
    ici_decref(l);
    return NULL;
}

static ici_obj_t *
fetch_list(ici_obj_t *o, ici_obj_t *k)
{
    list_t	*l = listof(o);
    listnode_t	*ls;
    ici_obj_t	*v;
    long	idx;
    long	i;

    /* integer index means return k'th element of list */
    if (ici_isint(k))
    {
	idx = ici_intof(k)->i_value;
	if (idx < 0 || idx >= l->l_nels)
	    return ici_fetch_fail(ici_objof(l), k);
	if (idx == l->l_idx)
	    return l->l_ptr->ln_obj;
	if (idx == l->l_idx + 1)
	{
	    l->l_ptr = l->l_ptr->ln_next;
	    l->l_idx = idx;
	    return l->l_ptr->ln_obj;
	}
	if (idx == l->l_idx - 1)
	{
	    l->l_ptr = l->l_ptr->ln_prev;
	    l->l_idx = idx;
	    return l->l_ptr->ln_obj;
	}
	if (idx < l->l_idx)
	{
	    if (l->l_ptr == NULL || l->l_idx - idx > idx)
		for (ls = l->l_head, i = 0; i < idx; ++i)
		    ls = ls->ln_next;
	    else
		for (ls = l->l_ptr, i = l->l_idx; i > 0; --i)
		    ls = ls->ln_prev;
	}
	else
	{
	    if (l->l_ptr && l->l_nels - idx > idx)
		for (ls = l->l_ptr, i = l->l_idx; i < idx; ++i)
		    ls = ls->ln_next;
	    else
		for (ls = l->l_tail, i = l->l_nels-1; i > idx; --i)
		    ls = ls->ln_prev;
	}
	assert(ls != NULL);
	if (ls == NULL)
	{
	    ici_error = "internal inconsistency in list code, nels is wrong!";
	    return NULL;
	}
	l->l_ptr = ls;
	l->l_idx = idx;
	return ls->ln_obj;
    }

    /* list.nels returns number of elements in list */
    if (k == ici_objof(ICIS(nels)))
    {
	if ((v = ici_objof(ici_int_new(l->l_nels))) != NULL)
	    ici_decref(v);
	return v;
    }
    /* list.head returns head element of list */
    if (k == ici_objof(ICIS(head)))
    {
	if (l->l_head == NULL)
	    return ici_null;
	return ici_objof(l->l_head);
    }
    /* list.tail returns tail element of list */
    if (k == ici_objof(ICIS(tail)))
    {
	if (l->l_tail == NULL)
	    return ici_null;
	return ici_objof(l->l_tail);
    }

    /* all others fail */
    return ici_fetch_fail(ici_objof(l), k);
}

ici_type_t list_type =
{
    mark_list,
    free_list,
    ici_hash_unique,
    ici_cmp_unique,
    copy_list,
    ici_assign_fail,
    fetch_list,
    "list"
};

list_t *
ici_new_list(void)
{
    list_t	*l;

    if ((l = ici_talloc(list_t)) != NULL)
    {
        ICI_OBJ_SET_TFNZ(l, list_tcode, 0, 1, 0);
	l->l_head = NULL;
	l->l_tail = NULL;
	l->l_nels = 0;
	l->l_ptr  = NULL;
	l->l_idx  = ICI_LIST_INDEX_INVALID;
    }
    return l;
}

/*
 * list = list.list([any...])
 */
static int
list(void)
{
    list_t	*l;
    listnode_t	*ln;
    int		i;

    if ((l = ici_new_list()) == NULL)
	return 1;
    for (i = 0; i < ICI_NARGS(); ++i)
    {
	if ((ln = ici_new_listnode(ICI_ARG(i))) == NULL)
	{
	    ici_decref(l);
	    return 1;
	}
	if (l->l_tail == NULL)
	{
	    l->l_head = ln;
	    l->l_tail = ln;
	}
	else
	{
	    if ((ln->ln_prev = l->l_tail) != NULL)
		ln->ln_prev->ln_next = ln;
	    l->l_tail = ln;
	}
	ici_decref(ln);
	++l->l_nels;
    }
    return ici_ret_with_decref(ici_objof(l));
}

/*
 * object = list.addhead(list, object)
 */
static int
addhead(void)
{
    list_t	*l;
    ici_obj_t	*o;
    listnode_t	*ln;

    if (ici_typecheck("oo", &l, &o))
	return 1;
    if (!islist(ici_objof(l)))
	return ici_argerror(0);
    if ((ln = ici_new_listnode(o)) == NULL)
	return 1;
    if (l->l_head == NULL)
    {
	l->l_head = l->l_tail = l->l_ptr = ln;
	l->l_idx  = 0;
	l->l_nels = 1;
    }
    else
    {
	ln->ln_next = l->l_head;
	l->l_head->ln_prev = ln;
	l->l_head = ln;
	++l->l_nels;
	if (l->l_idx >= 0)
	    ++l->l_idx;
    }
    ici_decref(ln);
    return ici_ret_no_decref(o);
}

/*
 * object = list.addtail(list, object)
 */
static int
addtail(void)
{
    list_t	*l;
    listnode_t	*ln;
    ici_obj_t	*o;

    if (ici_typecheck("oo", &l, &o))
	return 1;
    if (!islist(ici_objof(l)))
	return ici_argerror(0);
    if ((ln = ici_new_listnode(o)) == NULL)
	return 1;
    if (l->l_head == NULL)
    {
	l->l_head = l->l_tail = l->l_ptr = ln;
	l->l_idx  = 0;
	l->l_nels = 1;
    }
    else
    {
	l->l_tail->ln_next = ln;
	ln->ln_prev = l->l_tail;
	l->l_tail = ln;
	++l->l_nels;
    }
    ici_decref(ln);
    return ici_ret_no_decref(o);
}

/*
 * listnode = list.head(list)
 */
static int
head(void)
{
    list_t	*l;

    if (ici_typecheck("o", &l))
	return 1;
    if (!islist(ici_objof(l)))
	return ici_argerror(0);
    if (l->l_head == NULL)
	return ici_null_ret();
    return ici_ret_no_decref(ici_objof(l->l_head));
}

/*
 * listnode = list.tail(list)
 */
static int
tail(void)
{
    list_t	*l;

    if (ici_typecheck("o", &l))
	return 1;
    if (!islist(ici_objof(l)))
	return ici_argerror(0);
    if (l->l_tail == NULL)
	return ici_null_ret();
    return ici_ret_no_decref(ici_objof(l->l_tail));
}

/*
 * object = list.remhead(list)
 */
static int
remhead(void)
{
    list_t	*l;
    listnode_t	*ln;

    if (ici_typecheck("o", &l))
	return 1;
    if (!islist(ici_objof(l)))
	return ici_argerror(0);
    if (l->l_nels == 0)
	return ici_null_ret();
    ln = l->l_head;
    if ((l->l_head = ln->ln_next) != NULL)
	l->l_head->ln_prev = NULL;
    else
	l->l_tail = NULL;
    --l->l_nels;
    if (l->l_idx > 0)
	--l->l_idx;
    else
	l->l_idx = ICI_LIST_INDEX_INVALID;
    return ici_ret_no_decref(ln->ln_obj);
}

/*
 * object = list.remtail(list)
 */
static int
remtail(void)
{
    list_t	*l;
    listnode_t	*ln;

    if (ici_typecheck("o", &l))
	return 1;
    if (!islist(ici_objof(l)))
	return ici_argerror(0);
    if (l->l_nels == 0)
	return ici_null_ret();
    ln = l->l_tail;
    if ((l->l_tail = ln->ln_prev) != NULL)
	l->l_tail->ln_next = NULL;
    else
	l->l_head = NULL;
    if (l->l_idx == --l->l_nels)
	l->l_idx = ICI_LIST_INDEX_INVALID;
    return ici_ret_no_decref(ln->ln_obj);
}

static ici_cfunc_t cfuncs[] =
{
    {ICI_CF_OBJ, "list",        list},
    {ICI_CF_OBJ, "addhead",     addhead},
    {ICI_CF_OBJ, "addtail",     addtail},
    {ICI_CF_OBJ, "head",        head},
    {ICI_CF_OBJ, "tail",        tail},
    {ICI_CF_OBJ, "remhead",     remhead},
    {ICI_CF_OBJ, "remtail",     remtail},
    {ICI_CF_OBJ}
};

ici_obj_t *
ici_list_library_init(void)
{
    if (ici_interface_check(ICI_VER, ICI_BACK_COMPAT_VER, "list"))
        return NULL;
    if (init_ici_str())
        return NULL;
    if ((list_tcode = ici_register_type(&list_type)) == 0)
        return NULL;
    if ((listnode_tcode = ici_register_type(&listnode_type)) == 0)
        return NULL;
    return ici_objof(ici_module_new(cfuncs));
}
