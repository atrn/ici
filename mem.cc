#define ICI_CORE
#include "fwd.h"
#include "mem.h"
#include "int.h"
#include "buf.h"
#include "null.h"
#include "primes.h"

namespace ici
{

/*
 * Return a new ICI mem object refering to the memory at address 'base'
 * with length 'length', which is measured in units of 'accessz' bytes.
 * 'accessz' must be either 1, 2 or 4. If 'free_func' is provided it
 * will be called when the mem object is about to be freed with 'base'
 * as an argument.
 *
 * Returns NULL on error, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
mem *ici_mem_new(void *base, size_t length, int accessz, void (*free_func)(void *))
{
    mem  *m;

    if ((m = ici_talloc(mem)) == NULL)
        return NULL;
    ICI_OBJ_SET_TFNZ(m, ICI_TC_MEM, 0, 1, sizeof(mem));
    ici_rego(m);
    m->m_base = base;
    m->m_length = length;
    m->m_accessz = accessz;
    m->m_free = free_func;
    return memof(ici_atom(m, 1));
}

/*
 * Returns 0 if these objects are equal, else non-zero.
 * See the comments on t_cmp() in object.h.
 */
int mem_type::cmp(object *o1, object *o2)
{
    auto m1 = memof(o1);
    auto m2 = memof(o2);
    return m1->m_base != m2->m_base
        || m1->m_length != m2->m_length
        || m1->m_accessz != m2->m_accessz
        || m1->m_free != m2->m_free;
}

/*
 * Return a hash sensitive to the value of the object.
 * See the comment on t_hash() in object.h
 */
unsigned long mem_type::hash(object *o)
{
    return (unsigned long)memof(o)->m_base * MEM_PRIME_0
    + (unsigned long)memof(o)->m_length * MEM_PRIME_1
    + (unsigned long)memof(o)->m_accessz * MEM_PRIME_2;
}

void mem_type::free(object *o)
{
    auto m = memof(o);
    if (m->m_free != NULL)
    {
        (*m->m_free)(m->m_base);
    }
    ici_tfree(o, mem);
}

int mem_type::assign(object *o, object *k, object *v)
{
    int64_t i;

    if (!ici_isint(k) || !ici_isint(v))
        return assign_fail(o, k, v);
    i = ici_intof(k)->i_value;
    if (i < 0 || i >= (int64_t)memof(o)->m_length)
    {
        return ici_set_error("attempt to write at mem index %ld\n", i);
    }
    switch (memof(o)->m_accessz)
    {
    case 1:
        ((unsigned char *)memof(o)->m_base)[i] = (unsigned char)ici_intof(v)->i_value;
        break;

    case 2:
        ((unsigned short *)memof(o)->m_base)[i] = (unsigned short)ici_intof(v)->i_value;
        break;

    case 4:
        ((int *)memof(o)->m_base)[i] = (int32_t)ici_intof(v)->i_value;
        break;

    case 8:
        ((int *)memof(o)->m_base)[i] = (int64_t)ici_intof(v)->i_value;
        break;
    }
    return 0;
}

object * mem_type::fetch(object *o, object *k)
{
    int64_t i;

    if (!ici_isint(k))
        return fetch_fail(o, k);
    i = ici_intof(k)->i_value;
    if (i < 0 || i >= (int64_t)memof(o)->m_length)
        return ici_null;
    switch (memof(o)->m_accessz)
    {
    case 1:
        i = ((unsigned char *)memof(o)->m_base)[i];
        break;

    case 2:
        i = ((unsigned short *)memof(o)->m_base)[i];
        break;

    case 4:
        i = ((int32_t *)memof(o)->m_base)[i];
        break;

    case 8:
        i = ((int64_t *)memof(o)->m_base)[i];
        break;
    }
    o = ici_int_new(i);
    o->decref();
    return o;
}

} // namespace ici
