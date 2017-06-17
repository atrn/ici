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
ici_mem_t *
ici_mem_new(void *base, size_t length, int accessz, void (*free_func)(void *))
{
    ici_mem_t  *m;

    if ((m = ici_talloc(ici_mem_t)) == NULL)
        return NULL;
    ICI_OBJ_SET_TFNZ(m, ICI_TC_MEM, 0, 1, sizeof(ici_mem_t));
    ici_rego(m);
    m->m_base = base;
    m->m_length = length;
    m->m_accessz = accessz;
    m->m_free = free_func;
    return ici_memof(ici_atom(m, 1));
}

/*
 * Returns 0 if these objects are equal, else non-zero.
 * See the comments on t_cmp() in object.h.
 */
int mem_type::cmp(ici_obj_t *o1, ici_obj_t *o2)
{
    return ici_memof(o1)->m_base != ici_memof(o2)->m_base
        || ici_memof(o1)->m_length != ici_memof(o2)->m_length
        || ici_memof(o1)->m_accessz != ici_memof(o2)->m_accessz
        || ici_memof(o1)->m_free != ici_memof(o2)->m_free;
}

/*
 * Return a hash sensitive to the value of the object.
 * See the comment on t_hash() in object.h
 */
unsigned long mem_type::hash(ici_obj_t *o)
{
    return (unsigned long)ici_memof(o)->m_base * MEM_PRIME_0
    + (unsigned long)ici_memof(o)->m_length * MEM_PRIME_1
    + (unsigned long)ici_memof(o)->m_accessz * MEM_PRIME_2;
}

void mem_type::free(ici_obj_t *o)
{
    if (ici_memof(o)->m_free != NULL)
    {
        (*ici_memof(o)->m_free)(ici_memof(o)->m_base);
    }
    ici_tfree(o, ici_mem_t);
}

int mem_type::assign(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v)
{
    long       i;

    if (!ici_isint(k) || !ici_isint(v))
        return assign_fail(o, k, v);
    i = ici_intof(k)->i_value;
    if (i < 0 || i >= (long)ici_memof(o)->m_length)
    {
        return ici_set_error("attempt to write at mem index %ld\n", i);
    }
    switch (ici_memof(o)->m_accessz)
    {
    case 1:
        ((unsigned char *)ici_memof(o)->m_base)[i] = (unsigned char)ici_intof(v)->i_value;
        break;

    case 2:
        ((unsigned short *)ici_memof(o)->m_base)[i] = (unsigned short)ici_intof(v)->i_value;
        break;

    case 4:
        ((int *)ici_memof(o)->m_base)[i] = (int)ici_intof(v)->i_value;
        break;
    }
    return 0;
}

ici_obj_t * mem_type::fetch(ici_obj_t *o, ici_obj_t *k)
{
    long        i;

    if (!ici_isint(k))
        return fetch_fail(o, k);
    i = ici_intof(k)->i_value;
    if (i < 0 || i >= (long)ici_memof(o)->m_length)
        return ici_null;
    switch (ici_memof(o)->m_accessz)
    {
    case 1:
        i = ((unsigned char *)ici_memof(o)->m_base)[i];
        break;

    case 2:
        i = ((unsigned short *)ici_memof(o)->m_base)[i];
        break;

    case 4:
        i = ((int *)ici_memof(o)->m_base)[i];
        break;
    }
    o = ici_int_new(i);
    o->decref();
    return o;
}

} // namespace ici
