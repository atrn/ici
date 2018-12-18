#ifndef ICI_INTF_H
#define ICI_INTF_H

#ifndef ICI_FWD_H
# include "fwd.h"
#endif

/*
 * C interface to ICI. For use in loadable modules.
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EXPORT
# define EXPORT
#endif

EXPORT	int                     ici_init(void);
EXPORT  void                    ici_uninit(void);

typedef struct ici_object       ici_obj_t;
typedef struct ici_array        ici_array_t;

/* object.h */
EXPORT  void                    ici_incref(ici_obj_t *);
EXPORT  void                    ici_decref(ici_obj_t *);

/* array.h */
EXPORT  ici_array_t *           ici_array_new(size_t);
EXPORT  ici_obj_t **            ici_array_astart(ici_array_t *);
EXPORT  ici_obj_t **            ici_array_alimit(ici_array_t *);
EXPORT  ici_obj_t **            ici_array_anext(ici_array_t *, ici_obj_t **);
EXPORT  int                     ici_array_grow_stack(ici_array_t *, ptrdiff_t);
EXPORT  int                     ici_array_fault_stack(ici_array_t *, ptrdiff_t);
EXPORT  size_t                  ici_array_len(ici_array_t *);
EXPORT  ici_obj_t **            ici_array_span(ici_array_t *, size_t i, ptrdiff_t *);
EXPORT  int                     ici_array_grow(ici_array_t *);
EXPORT  int                     ici_array_push_back(ici_array_t *, ici_obj_t *);
EXPORT  int                     ici_array_push_front(ici_array_t *, ici_obj_t *);
EXPORT  ici_obj_t *             ici_array_pop_back(ici_array_t *);
EXPORT  ici_obj_t **            ici_array_find_slot(ici_array_t *, ptrdiff_t);
EXPORT  ici_obj_t *             ici_array_get(ici_array_t *, ptrdiff_t i);
EXPORT  ici_obj_t *             ici_array_pop_front(ici_array_t *);
EXPORT  void                    ici_array_gather(ici_array_t *, ici_obj_t **, ptrdiff_t, ptrdiff_t);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // ICI_INTF_H
