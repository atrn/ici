#ifndef ICI_DLLMANAGER_H
#define ICI_DLLMANAGER_H

typedef struct dllmanager
{
    ici_obj_t           o_head;
    ici_struct_t        *dllm_struct;
}
    dllmanager_t;

#define dllmanagerof(o)     ((dllmanager_t *)(o))
#define isdllmanager(o)     ((o)->o_tcode == dllmanager_tcode)

extern ici_type_t   dllmanager_type;
extern int          dllmanager_tcode;

#endif /* #ifndef ICI_DLLMANAGER_H */
