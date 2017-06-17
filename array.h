// -*- mode:c++ -*-

#ifndef ICI_ARRAY_H
#define ICI_ARRAY_H

/*
 * array.h - ICI array objects.
 */

#include "object.h"

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
/*
 * The ICI array object. Array objects are implemented in a manner to
 * make them efficient to use as stacks. And they are used that way in the
 * execution engine. A single contiguous array of object pointers is
 * associated with the array. a_base points to its start, a_limit one
 * past its end and a_bot and a_top somewhere in the middle.
 *
 * In the general case, arrays are growable circular buffers (queues). Thus
 * allowing efficient addition and removal from both ends. However, their
 * use in the internals of the execution engine is only as stacks. For
 * efficiency, we desire to be able to ensure that there is some amount of
 * contiguous space available on a stack by just making a single efficient
 * check.
 *
 * This is easy if they really are stacks, and the bottom is always anchored
 * to the start of allocation. (See Case 1 below.) This condition will
 * be true if *no* rpop() or rpush() operations have been done on the array.
 * Thus we conceptually consider arrays to have two forms. Ones that are
 * stacks (never had rpop() or rpush() done) and ones that are queues
 * (may, possibly, have had rpop() or rpush() done).
 *
 * Now, if an array is still a stack, you can use the functions:
 *
 *     ici_stk_push_chk(a, n)
 *     ici_stk_pop_chk(a, n)
 *
 * to ensure that there are n spaces or objects available, then just
 * increment/decrement a_top as you push and pop things on the stack.
 * Basically you can assume that object pointers and empty slots are
 * contiguous around a_top.
 *
 * But, if you can not guarantee (by context) that the array is a stack,
 * you can only push, pop, rpush or rpop single object pointers at a time.
 * Basically, the end you are dealing with may be near the wrap point of
 * the circular buffer.
 *
 * Case 1: Pure stack. Only ever been push()ed and pop()ed.
 *   ooooooooooooooooooooo.....................
 *   ^a_base              ^a_top               ^a_limit
 *   ^a_bot
 *
 * Case 2: Queue. rpush() and/or rpop()s have been done.
 *   ..........ooooooooooooooooooo.............
 *   ^a_base   ^a_bot             ^a_top       ^a_limit
 *
 * Case 3: Queue that has wrapped.
 *   oooooooooooooo.................ooooooooooo
 *   ^a_base       ^a_top           ^a_bot     ^a_limit
 *
 * A data structure such as this should really use an integer count
 * to indicate how many used elements there are. By using pure pointers
 * we have to keep one empty slot so we don't find ourselves unable
 * to distinguish full from empty (a_top == a_bot). But by using simple
 * pointers, only a_top needs to change as we push and pop elements.
 * If a_top == a_bot, the array is empty.
 *
 * Note that one must never take the atomic form of a stack, and
 * assume the result is still a stack.
 */
struct array : object
{
    ici_obj_t   **a_top;    /* The next free slot. */
    ici_obj_t   **a_bot;    /* The first used slot. */
    ici_obj_t   **a_base;   /* The base of allocation. */
    ici_obj_t   **a_limit;  /* Allocation limit, first one you can't use. */

    int grow_stack(ptrdiff_t n);
    int fault_stack(ptrdiff_t i);
    ptrdiff_t len();
    ici_obj_t **span(int i, ptrdiff_t *np);
    int grow();
    int push(ici_obj_t *o);
    int rpush(ici_obj_t *o);
    ici_obj_t *pop();
    ici_obj_t **find_slot(ptrdiff_t i);
    ici_obj_t *get(ptrdiff_t i);
    ici_obj_t *rpop();
};

inline ici_array_t *ici_arrayof(ici_obj_t *o)   { return static_cast<ici_array_t *>(o); }
inline bool ici_isarray(ici_obj_t *o)           { return o->isa(ICI_TC_ARRAY); }

/*
 * Check that there is room for 'n' new elements on the end of 'a'.  May
 * reallocate array memory to get more room. Return non-zero on failure,
 * usual conventions.
 *
 * This function can only be used where the array has never had
 * elements rpush()ed or rpop()ed. See the discussion on 'Accessing
 * ICI array object from C' before using.
 *
 * This --func-- forms part of the --ici-ap--.
 */
inline int ici_stk_push_chk(ici_array_t *a, ptrdiff_t n) {
    return a->a_limit - a->a_top < n ? a->grow_stack(n) : 0;
}

/*
 * Ensure that the stack a has i as a valid index.  Will grow and NULL fill
 * as necessary. Return non-zero on failure, usual conventions.
 */
inline int ici_stk_probe(ici_array_t *a, ptrdiff_t i) {
    return a->a_top - a->a_bot <= i ? a->fault_stack(i) : 0;
}

/*
 * A function to assist in doing for loops over the elements of an array.
 * Use as:
 *
 *  ici_array_t  *a;
 *  ici_obj_t    **e;
 *  for (e = ici_astart(a); e != ici_alimit(a); e = ici_anext(a, e))
 *      ...
 *
 * This --func-- forms part of the --ici-api--.
 */
inline ici_obj_t **ici_astart(ici_array_t *a) {
    return a->a_bot == a->a_limit && a->a_bot != a->a_top ? a->a_base : a->a_bot;
}

/*
 * A function to assist in doing for loops over the elements of an array.
 * Use as:
 *
 *  ici_array_t  *a;
 *  ici_obj_t    **e;
 *  for (e = ici_astart(a); e != ici_alimit(a); e = ici_anext(a, e))
 *      ...
 *
 * This --func-- forms part of the --ici-api--.
 */
inline ici_obj_t **ici_alimit(ici_array_t *a) {
    return a->a_top;
}

/*
 * A funcion to assist in doing for loops over the elements of an array.
 * Use as:
 *
 *  ici_array_t  *a;
 *  ici_obj_t    **e;
 *  for (e = ici_astart(a); e != ici_alimit(a); e = ici_anext(a, e))
 *      ...
 *
 * This --func-- forms part of the --ici-api--.
 */
inline ici_obj_t **ici_anext(ici_array_t *a, ici_obj_t **e) {
    return e + 1 == a->a_limit && a->a_limit != a->a_top ? a->a_base : e + 1;
}

 /*
 * End of ici.h export. --ici.h-end--
 */

class array_type : public type
{
 public:
    array_type() : type("array", sizeof (struct array), type::has_forall) {}

    unsigned long       mark(ici_obj_t *o) override;
    void                free(ici_obj_t *o) override;
    unsigned long       hash(ici_obj_t *o) override;
    int                 cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    ici_obj_t *         copy(ici_obj_t *o) override;
    int                 assign(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v) override;
    ici_obj_t *         fetch(ici_obj_t *o, ici_obj_t *k) override;
    int                 forall(ici_obj_t *o) override;
};

} // namespace ici

#endif
