// -*- mode:c++ -*-

#define ICI_CORE
#include "fwd.h"
#include "object.h"
#include "array.h"
#include "alloc.h"

namespace ici
{

class ici_interpreter_t
{
private:
        long                    ici_mem;
        long                    ici_mem_limit;
        int                     ici_n_allocs;
        long                    ici_alloc_mem;
        char                    *ici_flists[4];
        char                    *mem_next[4];
        char                    *mem_limit[4];
        achunk_t                *ici_achunks;
        char                    *ici_buf;       /* #define'd to buf in buf.h. */
        int                     ici_bufz;       /* 1 less than actual allocation. */
        char                    *ici_error;
        ici_type_t              *ici_types[ICI_MAX_TYPES];
        ici_obj_t               **ici_objs;         /* List of all objects. */
        ici_obj_t               **ici_objs_limit;   /* First element we can't use in list. */
        ici_obj_t               **ici_objs_top;     /* Next unused element in list. */
        ici_obj_t               **ici_atoms;    /* Hash table of atomic objects. */
        int                     ici_atomsz;     /* Number of slots in hash table. */
        int                     ici_natoms;     /* Number of atomic objects. */
        int                     ici_supress_collect;
        int		        ici_ncollects;	/* Number of ici_collect() calls */
        ici_exec_t              *ici_execs;
        ici_array_t             ici_xs;
        ici_array_t             ici_os;
        ici_array_t             ici_vs;
        volatile int            ici_aborted;
        int                     ici_evaluate_recursion_limit;

        int                     ici_exec_count;
};

} // namespace ici
