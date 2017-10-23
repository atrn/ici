// ici_viewdata.cpp : implementation file
//
#include <ici.h>
#include <windows.h>
#include "widb-priv.h"
/*
#include "object.h"
#include "struct.h"
#include "array.h"
#include "file.h"
#include "src.h"
#include "set.h"
#include "str.h"
#include "ptr.h"
#include "mem.h"
#include "func.h"
#include "float.h"
#include "int.h"
#include "null.h"
#ifndef NOPROFILE
#include "profile.h"
#endif
*/
#ifndef WIDB_H
    #include "widb.h"
#endif
#ifndef WIDB_WND_H
    #include "widb_wnd.h"
#endif
#include "resource.h"
#include <commctrl.h>


/*
 * The ICI object that this dialog displays.
 */
ici_obj_t *root_object;


/*
 * When a dialog opens up that allows you to edit strings (or other stuff
 * in the future), then this points to it.  Yucky global variables are kind
 * of necessary with C Win32 programming.
 */
ici_obj_t *to_edit;


/*
 * The window that will be used as the parent for dialog boxes that should
 * be shown over the application being debugged.  See WIDB_set_resources().
 */
HWND widb_dialog_parent;


/*
 * Prototypes for functions defined within this file.
 */
static void add_item_children(HWND hDlg, HTREEITEM parent, ici_obj_t *parent_o);
static void display_mem_use(ici_obj_t *o, HWND hDlg, HWND tree, HTREEITEM tv_item);
static void edit_object(ici_obj_t *o, HWND parent);
static LRESULT CALLBACK edit_string_wnd_proc
(
    HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam
);


/*
 * Called by profile_done_callback() to add up the time taken in each function
 * and its children.
 *
 * Parameters:
 *  funcs       A struct that maps ici_func_t's to profilecall_t's.
 *  pc          The call graph.  This function recursively calls itself
 *              and this is the tree being traversed.
 */
static void
func_totals(ici_struct_t *funcs, profilecall_t *pc)
{
    /* Iterate through the functions called from this function, looking
     * for primitives and recursing. */
    int i;
    for (i = 0; i < pc->pc_calls->s_nslots; ++ i)
    {
        ici_sslot_t*sl;
        ici_func_t *f;

        /* Does this slot have anything in it? */
        sl = &pc->pc_calls->s_slots[i];
        f = funcof(sl->sl_key);
        if (f != NULL)
        {
            profilecall_t *tot_pc;
            profilecall_t *parent;
            profilecall_t *called = profilecallof(sl->sl_value);

            /* Has this function been found before? */
            if (isnull(objof(tot_pc = profilecallof(ici_fetch(objof(funcs), f)))))
            {
                /* No, create a new record. */
                tot_pc = ici_profilecall_new(NULL);
                _ASSERT(tot_pc != NULL);

                /* Add it to the calling function. */
                ici_assign(objof(funcs), f, objof(tot_pc));
                ici_decref(tot_pc);
            }

            /* Add the total for this call to the overall total for this
             * function, but only if it's not part of a recursion (that
             * would make it be counted twice). */
            for
            (
                parent = called->pc_calledby, parent = parent->pc_calledby;
                parent != NULL;
                parent = parent->pc_calledby
            )
            {
                if (!isnull(ici_fetch(objof(parent->pc_calls), objof(f))))
                {
                    /* Found recursion, leave parent non-null. */
                    break;
                }
            }
            if (parent == NULL)
            {
                tot_pc->pc_total += called->pc_total;
                tot_pc->pc_call_count += called->pc_call_count;
            }

            /* Recurse. */
            func_totals(funcs, called);
        }
    }
}


/*
 * Called by profile_done_callback() to add up the time taken in each function
 * excluding that taken by its children.
 *
 * Parameters:
 *  funcs       A profilecall_t that maps ici_func_t's to profilecall_t's.
 *  pc          The call graph.  This function recursively calls itself
 *              and this is the tree being traversed.
 */
static void
func_intrinsic_totals(profilecall_t *funcs, profilecall_t *pc)
{
    /* Iterate through the functions called from this function, looking
     * for primitives and recursing. */
    int i;
    for (i = 0; i < pc->pc_calls->s_nslots; ++ i)
    {
        ici_sslot_t*sl;
        ici_func_t *f;

        /* Does this slot have anything in it? */
        sl = &pc->pc_calls->s_slots[i];
        f = funcof(sl->sl_key);
        if (f != NULL)
        {
            int j;
            profilecall_t *tot_pc;
            profilecall_t *called = profilecallof(sl->sl_value);

            /* Has this function been found before? */
            if (isnull(objof(tot_pc = profilecallof(ici_fetch(funcs->pc_calls, f)))))
            {
                /* No, create a new record. */
                tot_pc = ici_profilecall_new(funcs);
                _ASSERT(tot_pc != NULL);

                /* Add it to the calling function. */
                ici_assign(funcs->pc_calls, f, objof(tot_pc));
                ici_decref(tot_pc);
            }

            /* Add the total for this call to the overall total for this
             * function. */
            tot_pc->pc_total += called->pc_total;
            tot_pc->pc_call_count += called->pc_call_count;

            /* Remove any time taken by functions it called. */
            for (j = 0; j < called->pc_calls->s_nslots; ++ j)
            {
                ici_sslot_t*j_sl;

                /* Does this slot have anything in it? */
                j_sl = &called->pc_calls->s_slots[j];
                if (j_sl->sl_key != NULL)
                {
                    profilecall_t *sub_called = profilecallof(j_sl->sl_value);
                    tot_pc->pc_total -= sub_called->pc_total;
                }
            }

            /* Recurse. */
            func_intrinsic_totals(funcs, called);
        }
    }
}


/*
 * Called by profiling to display the results upon completion.
 */
static void
profile_done_callback(profilecall_t *pc)
{
    ici_obj_t *profile, *name;
    ici_struct_t *totals;
    profilecall_t *intrinsic_totals;

    /* Create a struct that will hold the different versions of the
     * profiling information.  Each member of this struct has a key naming
     * the type of profiling data and a value that's the information. */
    profile = objof(ici_struct_new());
    _ASSERT(profile != NULL);

    /* Construct a list of fundamental functions (ie. functions that don't
     * call other functions). */
    name = objof(ici_str_new_nul_term("function totals"));
    _ASSERT(name != NULL);
    /* Hold them in a profilecall_t so that they're sorted by pc_total */
    totals = ici_struct_new();
    _ASSERT(totals != NULL);
    VERIFY(!ici_assign(profile, name, objof(totals)));
    ici_decref(name);
    ici_decref(objof(totals));
    func_totals(totals, pc);

    /* Construct a list of fundamental functions (ie. functions that don't
     * call other functions). */
    name = objof(ici_str_new_nul_term("function intrinsic totals"));
    _ASSERT(name != NULL);
    /* Hold them in a profilecall_t so that they're sorted by pc_total */
    intrinsic_totals = ici_profilecall_new(NULL);
    _ASSERT(intrinsic_totals != NULL);
    VERIFY(!ici_assign(profile, name, objof(intrinsic_totals)));
    ici_decref(name);
    ici_decref(objof(intrinsic_totals));
    func_intrinsic_totals(intrinsic_totals, pc);
    /* We're paranoid that these don't add up right, so add them up to
     * check. */
    {
        int i;
        for (i = 0; i < intrinsic_totals->pc_calls->s_nslots; ++ i)
        {
            ici_sslot_t*sl;

            /* Does this slot have anything in it? */
            sl = &intrinsic_totals->pc_calls->s_slots[i];
            if (sl->sl_key != NULL)
                intrinsic_totals->pc_total +=
                    profilecallof(sl->sl_value)->pc_total;
        }
    }

    /* Add the call basic call graph output. */
    name = objof(ici_str_new_nul_term("call graph"));
    _ASSERT(name != NULL);
    VERIFY(!ici_assign(profile, name, pc));
    ici_decref(name);

    /* Display it. */
    WIDB_view_object(profile, NULL);
    ici_decref(profile);
}


/*
 * Call this to make the profiler display using WIDB.  This is much more
 * usable than the file output.
 *
 * The debugger will call this for you if you've got debugging on.
 */
void
WIDB_enable_profiling_display()
{
    ici_profile_set_done_callback(profile_done_callback);
}


/*
 * Called to tell WIDB to use a different instance for loading resources.
 * This is for applications that separate their resources into a separate
 * DLL for easy localisation.
 *
 * Parameters:
 *  resources       The handle to the module containing resources.
 */
void
WIDB_set_resources(HINSTANCE resources)
{
    widb_resources = resources;
}


/* EXTERN
 * WIDB_set_dialog_parent - Sets the window that will be used as the parent for
 *                          view object and other dialog boxes.
 *
 * This stops the user of WIDB_view_obj() from having to propagate an HWND
 * through their code, and still has it pop up over the right window.
 *
 * Parameters:
 *  dialog_parent   The window over which dialog boxes should be placed.
 */
void
WIDB_set_dialog_parent(HWND dialog_parent)
{
    widb_dialog_parent = dialog_parent;
}


/*
 * Returns a text representation of a simple ICI data type.
 */
static char const *
get_simple_value(ici_obj_t *o)
{
    static char retval[256];

    /* Any type-specific output the ici_objname() is not good enough for. */
    if (isfile(o))
        sprintf(retval, "file(%s)", fileof(o)->f_name);
    else if (issrc(o))
        sprintf(retval, "src(%s, %d)", srcof(o)->s_filename, srcof(o)->s_lineno);
    else if (isstring(o))
    {
        strncpy(retval, stringof(o)->s_chars, sizeof(retval));
        retval[sizeof(retval) - 1] = '\0';
    }
    else if (ismem(o))
        sprintf(retval, "mem -> 0x%08X", memof(o)->m_base);
    else if (isfloat(o))
    {
        sprintf(retval, "%g", floatof(o)->f_value);
        if (strchr(retval, '.') == NULL && strchr(retval, 'e') == NULL)
            strcat(retval, ".0");
    }
#if NOPROFILE
    else if (isprofilecall(o))
    {
        profilecall_t *pc = profilecallof(o);
        sprintf(retval, "%ldms, %ld calls", pc->pc_total, pc->pc_call_count);
    }
#endif
    else
    {
        char n[30];
        /* ici_objname() handles common cases. */
        strcpy(retval, ici_objname(n, o));
    }

    // Add a flag if it's atomic.
    if
    (
        (isstruct(o) || isset(o) || isarray(o))
        &&
        o->o_flags & O_ATOM
    )
    {
        strcat(retval, " @");
    }

    // Add the object location.
    if (!isint(o) && !isstring(o) && !isfloat(o))
        sprintf(retval + strlen(retval), " (0x%08X)", o);

    return retval;
}


/*
 * Ensures that copied strings are NULL terminated.
 */
static char *
safe_strncpy(char *target, char const *source, size_t count)
{
    strncpy(target, source, count);
    target[count - 1] = '\0';
    return target;
}


/*
 * Recursively fills a tree view control with the contents of an ICI object.
 */
static void
add_item(HWND hDlg, HTREEITEM parent, ici_obj_t *o, int expand)
{
    char value[256];
    TV_INSERTSTRUCT insert;
    ici_obj_t *object_to_expand;
    HTREEITEM new_item;
    BOOL has_children;

    //
    // Work out the string for this node in the tree.
    //

    // If it's a member of a structure then we make it
    //  name = value (the name portion can't be expanded).
    if (isptr(o))
    {
        ici_ptr_t *p = ptrof(o);
        safe_strncpy(value, get_simple_value(p->p_key), sizeof(value));
        strncat(value, " = ", sizeof(value) - strlen(value) - 1);
        object_to_expand = ici_fetch(p->p_aggr, p->p_key);
    }
    else
    {
        // It's not a pointer so it just has one value.
        value[0] = '\0';
        object_to_expand = o;
    }

    // Put the value portion in.
    strncat(value, get_simple_value(object_to_expand), sizeof(value) - strlen(value) - 1);


    //
    // Create the new tree item.
    //
    insert.hParent = parent;
    insert.hInsertAfter = TVI_LAST;
    insert.item.mask = TVIF_STATE | TVIF_TEXT | TVIF_CHILDREN | TVIF_PARAM;
    insert.item.state = parent == TVI_ROOT ? TVIS_EXPANDED : 0;
    insert.item.stateMask = TVIS_EXPANDED;
    insert.item.pszText = value;
    has_children = isstruct(object_to_expand)
                   ||
                   isset(object_to_expand)
                   ||
                   isarray(object_to_expand)
                   ||
                   isfunc(object_to_expand)
                   ||
                   isprofilecall(object_to_expand)
                   ;
    insert.item.cChildren = has_children ? 1 : 0;
    insert.item.lParam = (LPARAM)o;
    new_item = (HTREEITEM)SendDlgItemMessage
    (
        hDlg,
        IDC_WIDB_VIEWOBJ_TREE,
        TVM_INSERTITEM,
        0,
        (LPARAM)&insert
    );
    _ASSERT(new_item != NULL);

    if (expand && has_children)
        add_item_children(hDlg, new_item, object_to_expand);
}


/*
 * Comparison function used by qsort().  Compares two ici_obj_t's.
 *
 * Non-strings sort first by ici_obj_t*, strings sort alphabetically.
 */
static int
objcmp(void const *elem1, void const *elem2)
{
    int type1, type2;
    ici_obj_t *o1 = *(ici_obj_t **)elem1;
    ici_obj_t *o2 = *(ici_obj_t **)elem2;
    char        n1[30];
    char        n2[30];

    /* The first, roughest, grade of sorting is by types.  Strings are
     * at the bottom, then profilecall_t's, then functions and then anything
     * else. */
    type1 = isstring(o1) ? 3
            : isprofilecall(o1) ? 2
            : isfunc(o1) ? 1
            : 0;
    type2 = isstring(o2) ? 3
            : isprofilecall(o2) ? 2
            : isfunc(o2) ? 1
            : 0;
    if (type1 != type2)
        return type1 - type2;

    /* They're the same. */
    switch (type1)
    {
    default:
    case 0:
        /* Something else, sort by simple object pointer. */
        if (o1 < o2)
            return -1;
        else if (o1 == o2)
            return 0;
        else
            return 1;

    case 1:
        /* Functions */
        return strcmp(ici_objname(n1, o1), ici_objname(n2, o2));

    case 2:
        /* profilecall_t */
        return profilecallof(o2)->pc_total - profilecallof(o1)->pc_total;

    case 3:
        /* Strings. */
        return strcmp(stringof(o1)->s_chars, stringof(o2)->s_chars);
    }
}


/*
 * Adds the children of an ICI object to a tree view item.
 *
 * See add_item();
 */
static void
add_item_children(HWND hDlg, HTREEITEM parent, ici_obj_t *parent_o)
{
    // If its children have already been added then stop here,
    // otherwise there will be extra copies.
    if
    (
        NULL != (HTREEITEM) SendDlgItemMessage
                            (
                                hDlg,
                                IDC_WIDB_VIEWOBJ_TREE,
                                TVM_GETNEXTITEM,
                                TVGN_CHILD,
                                (LPARAM)parent
                            )
    )
        return;

    if (isstruct(parent_o))
    {
        ici_struct_t *s;
        int i, nels_sorted = 0, sorted_i;
        ici_obj_t **sorted;

        // This is a structure, we must create pointer objects for every
        // member and recursively add them.
        s = structof(parent_o);

        // Add any super the struct has.
        if (s->o_head.o_super != NULL)
        {
            add_item(hDlg, parent, objof(s->o_head.o_super), FALSE);
        }

        // Make an array containing everything in the struct.
        sorted = (ici_obj_t **)malloc(sizeof(ici_obj_t *) * s->s_nslots);
        for (i = 0; i < s->s_nslots; i ++)
        {
            // Only add hash elements that actually exist.
            ici_obj_t *element = s->s_slots[i].sl_key;
            if (element != NULL)
                sorted[nels_sorted ++] = element;
        }

        // Sort the array.
        qsort(sorted, nels_sorted, sizeof(ici_obj_t *), objcmp);

        // Add the items to the control.
        for (sorted_i = 0; sorted_i < nels_sorted; ++ sorted_i)
        {
            ici_obj_t *element = objof(ici_ptr_new(parent_o, sorted[sorted_i]));
            _ASSERT(element != NULL);
            add_item(hDlg, parent, element, FALSE);
            ici_decref(element);
        }
        free(sorted);
    }
    else if (isarray(parent_o))
    {
        ici_array_t *a;
        ici_obj_t **e;

        // This is an array, we can simply add every element in it.
        a = arrayof(parent_o);
        for (e = ici_astart(a); e < ici_alimit(a); e = ici_anext(a, e))
        {
            add_item(hDlg, parent, *e, FALSE);
        }
    }
    else if (isset(parent_o))
    {
        ici_set_t *s;
        int i, nels_sorted = 0, sorted_i;
        ici_obj_t **sorted;

        // Make an array containing everything in the set.
        s = setof(parent_o);
        sorted = (ici_obj_t **)malloc(sizeof(ici_obj_t *) * s->s_nslots);
        for (i = 0; i < s->s_nslots; i ++)
        {
            ici_obj_t *element = s->s_slots[i];
            if (element != NULL)
                sorted[nels_sorted ++] = element;
        }

        // Sort the array.
        qsort(sorted, nels_sorted, sizeof(ici_obj_t *), objcmp);

        // Add the items to the control.
        for (sorted_i = 0; sorted_i < nels_sorted; ++ sorted_i)
            add_item(hDlg, parent, sorted[sorted_i], FALSE);
        free(sorted);
    }
    else if (isfunc(parent_o))
    {
        ici_obj_t *name;
        ici_obj_t *element;

        name = objof(ici_str_new_nul_term("autos"));
        _ASSERT(name != NULL);
        element = objof(ici_ptr_new(parent_o, name));
        ici_decref(name);
        _ASSERT(element != NULL);
        add_item(hDlg, parent, element, FALSE);
        ici_decref(element);

        name = objof(ici_str_new_nul_term("args"));
        _ASSERT(name != NULL);
        element = objof(ici_ptr_new(parent_o, name));
        ici_decref(name);
        _ASSERT(element != NULL);
        add_item(hDlg, parent, element, FALSE);
        ici_decref(element);
    }
    else if (isprofilecall(parent_o))
    {
        // Profile data is display like a structure, but sorted according to
        // totals.
        ici_struct_t *s;
        int i, nels_sorted = 0, sorted_i;
        ici_obj_t **sorted;

        // This is a structure, we must create pointer objects for every
        // member and recursively add them.
        s = structof(profilecallof(parent_o)->pc_calls);

        // Make an containing pairs.  The first item is the profilecall_t,
        // this is sorted on.  The second item is the ici_func_t, this is added
        // to this branch of the list for expansion after sorting.
        sorted = (ici_obj_t **)malloc(sizeof(ici_obj_t *) * 2 * s->s_nslots);
        for (i = 0; i < s->s_nslots; i ++)
        {
            // Only add hash elements that actually exist.
            ici_obj_t *element = s->s_slots[i].sl_key;
            if (element != NULL)
            {
                sorted[nels_sorted * 2] = s->s_slots[i].sl_value;
                sorted[nels_sorted * 2 + 1] = element;
                nels_sorted ++;
            }
        }

        // Sort the array.
        qsort(sorted, nels_sorted, sizeof(ici_obj_t *) * 2, objcmp);

        // Add the items to the control.
        for (sorted_i = 0; sorted_i < nels_sorted; ++ sorted_i)
        {
            ici_obj_t *element = objof
                                (
                                    ici_ptr_new(objof(s),
                                    sorted[sorted_i * 2 + 1])
                                );
            _ASSERT(element != NULL);
            add_item(hDlg, parent, element, FALSE);
            ici_decref(element);
        }
        free(sorted);
    }
}


/*
 * Returns:
 *  The ICI object associated with a particular node in the tree view control.
 */
static ici_obj_t *
get_obj_from_tv_item(HWND tree, HTREEITEM tv_item)
{
    TVITEM item;
    ici_obj_t *o;

    _ASSERT(tree != NULL && tv_item != NULL);
    item.hItem = tv_item;
    item.mask = TVIF_PARAM;
    VERIFY(TreeView_GetItem(tree, &item));
    o = (ici_obj_t *)item.lParam;
    _ASSERT(o != NULL);
    return o;
}


/*
 * Callback that servies the WIDB_view_object dialog box.
 */
static WNDPROC tree_wnd_proc_parent;
static LRESULT CALLBACK tree_wnd_proc
(
    HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam
)
{
    switch (message)
    {
        case WM_CHAR:
            switch (wParam)
            {
#if 0
                case 'M':
                case 'm':
                {
                    HTREEITEM sel;
                    ici_obj_t *sel_obj;

                    // Which object is currently selected?
                    sel = TreeView_GetSelection(hWnd);
                    _ASSERT(sel != NULL);
                    sel_obj = get_obj_from_tv_item(hWnd, sel);

                    display_mem_use(sel_obj, hWnd, hWnd, sel);

                    return 0;
                }
                case 'T':
                case 't':
                {
                    unsigned long used_mem, uncollected_mem;
                    char usage[100];
                    ici_obj_t **a;

                    // What's the total memory used by ICI structures?
                    // Find out how much memory is in this object.
                    //
                    // Count objects marked as in use.
                    used_mem = 0;
                    for (a = objs; a < objs_top; ++a)
                        if ((*a)->o_nrefs > 0)
                            used_mem += ici_mark(*a);

                    // Count the remaining objects, these could do with
                    // garbage collection.
                    uncollected_mem = 0;
                    for (a = objs; a < objs_top; ++a)
                        if (!((*a)->o_flags & O_MARK))
                            uncollected_mem += ici_mark(*a);

                    // Clear the flags we've set.
                    for (a = objs; a < objs_top; ++a)
                        (*a)->o_flags &= ~O_MARK;

                    sprintf
                    (
                        usage,
                        "used: %lu bytes\n"
                        "uncollected: %lu bytes\n",
                        used_mem,
                        uncollected_mem
                    );
                    MessageBox(hWnd, usage, "Total Memory Usage", MB_OK);
                    return 0;
                }
#endif
            }
    }

    return CallWindowProc
           (
               tree_wnd_proc_parent,
               hWnd,
               message,
               wParam,
               lParam
           );
}


/*
 * Callback that servies the WIDB_view_object dialog box.
 */
static LRESULT CALLBACK view_object_wnd_proc
(
    HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam
)
{
    switch (message)
    {
        case WM_INITDIALOG:
            // Put the ICI object in the tree.
            add_item(hDlg, TVI_ROOT, root_object, TRUE);

            // Subclass the tree view control.
            {
                HWND tree;
                tree = GetDlgItem(hDlg, IDC_WIDB_VIEWOBJ_TREE);
                _ASSERT(tree != NULL);
                tree_wnd_proc_parent = (WNDPROC) SetWindowLong
                                                 (
                                                     tree,
                                                     GWL_WNDPROC,
                                                     (LONG)tree_wnd_proc
                                                 );
                _ASSERT(tree_wnd_proc_parent != NULL);
            }
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, TRUE);
                return TRUE;
            }
            break;

        case WM_NOTIFY:
        {
            NMHDR *hdr = (NMHDR *)lParam;
            if ((int)wParam == IDC_WIDB_VIEWOBJ_TREE)
            {
                HWND tree = hdr->hwndFrom;
                switch (hdr->code)
                {
                    case TVN_ITEMEXPANDING:
                    {
                        NMTREEVIEW *pnmtv;
                        ici_obj_t *to_expand;

                        pnmtv = (NMTREEVIEW *)lParam;
                        to_expand = (ici_obj_t *)pnmtv->itemNew.lParam;
                        if (isptr(to_expand))
                        {
                            // We choose to expand the thing being pointed
                            // to, this is the commonly useful case.
                            ici_ptr_t *p = ptrof(to_expand);
                            to_expand = ici_fetch(p->p_aggr, p->p_key);
                            _ASSERT(to_expand != NULL && !isnull(to_expand));
                        }
                        add_item_children
                        (
                            hDlg,
                            pnmtv->itemNew.hItem,
                            to_expand
                        );
                        return FALSE;
                    }

                    case NM_DBLCLK:
                    {
                        // Edit the string.
                        edit_object
                        (
                            get_obj_from_tv_item
                            (
                                tree,
                                TreeView_GetSelection(tree)
                            ),
                            hDlg
                        );
                        return FALSE;
                    }
                }
            }
            return TRUE;
        }
    }

    return FALSE;
}


/* EXTERN
 * WIDB_view_object - Shows an ICI variable in a modal dialog box.
 *
 * The dialog box has a single treeview control and starts out with all
 * structures and arrays expanded.
 *
 * Parameters:
 * o            The object to view, this may be of any ICI type, but only
 *              string, array, struct, float, pointer and int may be decoded.
 * parent       The window that this dialog should appear over.  This may be
 *              NULL if WIDB_set_dialog_parent() has previously been called.
 */
void
WIDB_view_object(ici_obj_t *o, HWND parent)
{
    int result;

    root_object = o;
    result = DialogBox
    (
        widb_resources,
        MAKEINTRESOURCE(IDD_WIDB_VIEWOBJ),
        parent == NULL ? widb_dialog_parent : parent,
        (DLGPROC)view_object_wnd_proc
    );
    if (result == -1)
    {
        char buf[256];
        FormatMessage
        (
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            buf,
            sizeof(buf),
            NULL
        );
        fprintf(stderr, "Couldn't show dialog box: %s\n", buf);
    }
}

#if 0
/*
 * Pops up a message box showing the memory used by a particular item in the
 * tree and everything below it.
 *
 * To avoid counting memory used by objects referred to with back pointers
 * (like _parent), we don't follow pointers to objects between this object
 * and the root of the tree view.
 *
 * Parameters:
 *  o           The item having its memory use shown.
 *  hDlg        The dialog box that the tree view is in.
 *  tree        The tree view control.
 *  tv_item     The selected item in the tree view.
 */
static void
display_mem_use(ici_obj_t *o, HWND hDlg, HWND tree, HTREEITEM tv_item)
{
    unsigned long obj_mem, atomic_mem;
    char usage[100];
    ici_obj_t **a;

    //
    // Show memory usage.
    //

    // Mark the structures down to this
    // one to avoid them being included
    // in the total.
    {
        HTREEITEM parent;

        parent = tv_item;
        while ((parent = TreeView_GetParent(tree, parent)) != NULL)
        {
            ici_obj_t *parent_o = get_obj_from_tv_item(tree, parent);
            if (isptr(parent_o))
            {
                ici_ptr_t *p = ptrof(parent_o);
                parent_o = ici_fetch(p->p_aggr, p->p_key);
            }
            parent_o->o_flags |= O_MARK;
        }
    }


    // If this is a pointer then fetch the actual object
    // being pointed at.
    if (isptr(o))
    {
        ici_ptr_t *p = ptrof(o);
        o = ici_fetch(p->p_aggr, p->p_key);
    }

    // Find out how much memory is in this object.
    obj_mem = ici_mark(o);

    // And how much is in atomic stuff.
    atomic_mem = 0;
    for (a = objs; a < objs_top; ++a)
    {
        if
        (
            ((*a)->o_flags & O_MARK)
            &&
            ((*a)->o_flags & O_ATOM)
        )
        {
            (*a)->o_flags &= ~O_MARK;
            atomic_mem += ici_mark(*a);
        }
    }

    // Clear the marks so as to leave the object
    // table the way we found it.
    for (a = objs; a < objs_top; ++a)
        (*a)->o_flags &= ~O_MARK;

    // Tell the user what we found out.
    sprintf
    (
        usage,
        "%lu bytes (%lu is atomic)",
        obj_mem,
        atomic_mem
    );
    MessageBox(hDlg, usage, "Memory Usage", MB_OK);
}
#endif

/*
 * edit_object
 *
 * Parameters:
 *  o           An ICI object to edit.  This should be a pointer, we'll edit
 *              what it points to.  If it isn't a pointer we'll ignore it.
 *              If it doesn't point to a string we'll ignore it (we can
 *              support numbers later).
 *  parent      The parent for any dialogs we raise.
 */
static void
edit_object(ici_obj_t *o, HWND parent)
{
    // We can only edit strings, and they're always
    // embedded in some struct as a pointer.
    if (isptr(o))
    {
        ici_ptr_t *p;

        p = ptrof(o);
        to_edit = ici_fetch(p->p_aggr, p->p_key);
        _ASSERT(to_edit != NULL);

        // So far we only have capability to edit strings.
        if (isstring(to_edit))
        {
            int result;

            result = DialogBox
            (
                widb_resources,
                MAKEINTRESOURCE(IDD_WIDB_EDITSTRING),
                parent,
                (DLGPROC)edit_string_wnd_proc
            );
            switch (result)
            {
                // Error.
                case -1:
                {
                    char buf[256];
                    FormatMessage
                    (
                        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL,
                        GetLastError(),
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        buf,
                        sizeof(buf),
                        NULL
                    );
                    fprintf(stderr,"Couldn't show edit dialog box: %s\n", buf);
                    break;
                }

                // It's been changed.
                case IDOK:
                {
                    // Remember the change.
                    VERIFY(0 == ici_assign(p->p_aggr, p->p_key, to_edit));
                    break;
                }

                // Nothing changed.
                case IDCANCEL:
                    break;
            }
        }
    }
}


/*
 * Callback that servies the string edit dialog box.
 *
 * This dialog simply allows the editing of an ICI string, there's nothing
 * sophisticated about it.
 *
 * When the dialog is started it assumes that to_edit points to a
 * gotted string, on exit it will ensure that this is still the case,
 * although the string pointed to may be different.
 */
static LRESULT CALLBACK edit_string_wnd_proc
(
    HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam
)
{
    switch (message)
    {
        case WM_INITDIALOG:
        {
            HWND edit_control;
            char *text;
            int i;

            // Get the string we're editing.
            _ASSERT(to_edit != NULL && isstring(to_edit));
            edit_control = GetDlgItem(hDlg, IDC_WIDB_EDIT);
            _ASSERT(edit_control != NULL);

            // Convert LF to CR/LF pairs for Windows.
            text = (char *)malloc(stringof(to_edit)->s_nchars * 2 + 1);
            _ASSERT(text != NULL);
            (void) memcpy
                   (
                       text,
                       stringof(to_edit)->s_chars,
                       stringof(to_edit)->s_nchars + 1
                   );
            for (i = 0; text[i] != '\0'; ++ i)
            {
                // Found a newline?
                if (text[i] == '\n')
                {
                    // Translate it and push everything after
                    // it back by one character.
                    char *to_move = text + i + 1;
                    (void) memmove(to_move + 1, to_move, strlen(to_move) + 1);
                    text[i] = '\r';
                    text[++ i] = '\n';
                }
            }

            // Put the text into the edit box.
            SetWindowText(edit_control, text);
            free(text);
            return TRUE;
        }
        case WM_COMMAND:
            // OK Button.
            if (LOWORD(wParam) == IDOK)
            {
                // SET!
                HWND edit_control;
                int text_len;
                char *text;
                int i;

                // Get the text out of the edit control.
                edit_control = GetDlgItem(hDlg, IDC_WIDB_EDIT);
                text_len = GetWindowTextLength(edit_control);
                text = malloc(text_len + 1);
                _ASSERT(text != NULL);
                GetWindowText(edit_control, text, text_len + 1);

                // Convert CR/LF pairs back to plain LF.
                for (i = 0; text[i] != '\0'; ++ i)
                {
                    // Found a pair?
                    if (text[i] == '\r' && text[i + 1] == '\n')
                    {
                        // Yep, get rid of the CR and pull everything after
                        // it over the left over byte.
                        char *to_move = text + i + 2;
                        (void) memmove(to_move - 1, to_move, strlen(to_move) + 1);
                        text[i] = '\n';
                        -- text_len;
                    }
                }

                // Save the text back into the object we're editing.
                ici_decref(to_edit);
                to_edit = objof(ici_str_new(text, text_len));
                _ASSERT(to_edit != NULL);
                free(text);
                EndDialog(hDlg, IDOK);
                return TRUE;
            }

            // Cancel Button.
            else if (LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, IDCANCEL);
                return TRUE;
            }
            break;
    }

    return FALSE;
}
