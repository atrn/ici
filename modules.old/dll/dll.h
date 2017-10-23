#ifndef ICI_DLL_H
#define ICI_DLL_H

/*
 * We'd like to call this dll_t. But that name has been taken.
 */
typedef struct dllib
{
    ici_obj_t           o_head;
    dll_t               *dll_lib;
    ici_struct_t        *dll_struct;
}
    dllib_t;

#define dllof(o)        ((dllib_t *)(o))
#define isdll(o)        ((o)->o_tcode == dll_tcode)

dllib_t                 *new_dll(dll_t lib);
extern ici_type_t       dll_type;
extern int              dll_tcode;

#endif /* ICI_DLL_H */
