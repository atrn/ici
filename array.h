#ifndef ICI_ARRAY_H
#define ICI_ARRAY_H

/*
 * array.h - ICI array objects.
 */

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

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
 * Now, if an array is still a stack, you can use the functions (macros):
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
struct ici_array : ici_obj
{
    ici_obj_t   **a_top;    /* The next free slot. */
    ici_obj_t   **a_bot;    /* The first used slot. */
    ici_obj_t   **a_base;   /* The base of allocation. */
    ici_obj_t   **a_limit;  /* Allocation limit, first one you can't use. */
};
#define ici_arrayof(o)  ((ici_array_t *)(o))
#define ici_isarray(o)  ((o)->o_tcode == ICI_TC_ARRAY)

/*
 * Check that there is room for 'n' new elements on the end of 'a'.  May
 * reallocate array memory to get more room. Return non-zero on failure,
 * usual conventions.
 *
 * This macro can only be used where the array has never had elements
 * rpush()ed or rpop()ed. See the discussion on
 * 'Accessing ICI array object from C' before using.
 *
 * This --macro-- forms part of the --ici-ap--.
 */
#define ici_stk_push_chk(a, n) \
                ((a)->a_limit - (a)->a_top < (n) ? ici_grow_stack((a), (n)) : 0)

/*
 * Ensure that the stack a has i as a valid index.  Will grow and NULL fill
 * as necessary. Return non-zero on failure, usual conventions.
 */
#define ici_stk_probe(a, i) ((a)->a_top - (a)->a_bot <= (i) \
                ? ici_fault_stack((a), (i)) \
                : 0)

/*
 * A macro to assist in doing for loops over the elements of an array.
 * Use as:
 *
 *  ici_array_t  *a;
 *  ici_obj_t    **e;
 *  for (e = ici_astart(a); e != ici_alimit(a); e = ici_anext(a, e))
 *      ...
 *
 * This --macro-- forms part of the --ici-api--.
 */
#define ici_astart(a)   ((a)->a_bot == (a)->a_limit && (a)->a_bot != (a)->a_top \
                            ? (a)->a_base : (a)->a_bot)

/*
 * A macro to assist in doing for loops over the elements of an array.
 * Use as:
 *
 *  ici_array_t  *a;
 *  ici_obj_t    **e;
 *  for (e = ici_astart(a); e != ici_alimit(a); e = ici_anext(a, e))
 *      ...
 *
 * This --macro-- forms part of the --ici-api--.
 */
#define ici_alimit(a)   ((a)->a_top)

/*
 * A macro to assist in doing for loops over the elements of an array.
 * Use as:
 *
 *  ici_array_t  *a;
 *  ici_obj_t    **e;
 *  for (e = ici_astart(a); e != ici_alimit(a); e = ici_anext(a, e))
 *      ...
 *
 * This --macro-- forms part of the --ici-api--.
 */
#define ici_anext(a, e) ((e) + 1 == (a)->a_limit && (a)->a_limit != (a)->a_top \
                            ? (a)->a_base : (e) + 1)

 /*
 * End of ici.h export. --ici.h-end--
 */
#endif
