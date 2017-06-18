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
 * Now, if an array is still a stack, you can use the member functions:
 *
 *     a->stk_push_chk(n)
 *     a->stk_pop_chk(n)
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
    object   **a_top;    /* The next free slot. */
    object   **a_bot;    /* The first used slot. */
    object   **a_base;   /* The base of allocation. */
    object   **a_limit;  /* Allocation limit, first one you can't use. */

    /*
     * Functions to assist in doing for loops over the elements of an array.
     * Use as:
     *
     *  array *a;
     *  object **e;
     *  for (e = a->astart(); e != a->alimit(); e = a->anext(e))
     *      ...
     *
     * This --func-- forms part of the --ici-api--.
     */
    inline object **astart() {
        return a_bot == a_limit && a_bot != a_top ? a_base : a_bot;
    }

    inline object **alimit() {
        return a_top;
    }

    inline object **anext(object **e) {
        return e + 1 == a_limit && a_limit != a_top ? a_base : e + 1;
    }

    int grow_stack(ptrdiff_t n);
    int fault_stack(ptrdiff_t i);
    ptrdiff_t len();
    object **span(int i, ptrdiff_t *np);
    int grow();
    int push(object *o);
    int rpush(object *o);
    object *pop();
    object **find_slot(ptrdiff_t i);
    object *get(ptrdiff_t i);
    object *rpop();
    void gather(object **, ptrdiff_t, ptrdiff_t);

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
    inline int stk_push_chk(ptrdiff_t n = 1) {
        return a_limit - a_top < n ? grow_stack(n) : 0;
    }

    /*
     * Ensure that the stack a has i as a valid index.  Will grow and NULL fill
     * as necessary. Return non-zero on failure, usual conventions.
     */
    inline int stk_probe(ptrdiff_t i) {
        return a_top - a_bot <= i ? fault_stack(i) : 0;
    }
};

inline array *arrayof(object *o) { return static_cast<array *>(o); }
inline bool isarray(object *o) { return o->isa(ICI_TC_ARRAY); }

 /*
 * End of ici.h export. --ici.h-end--
 */

class array_type : public type
{
 public:
    array_type() : type("array", sizeof (struct array), type::has_forall) {}

    size_t mark(object *o) override;
    void free(object *o) override;
    unsigned long hash(object *o) override;
    int cmp(object *o1, object *o2) override;
    object *copy(object *o) override;
    int assign(object *o, object *k, object *v) override;
    object *fetch(object *o, object *k) override;
    int forall(object *o) override;
};

} // namespace ici

#endif
