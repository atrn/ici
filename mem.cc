#define ICI_CORE
#include "fwd.h"
#include "mem.h"
#include "int.h"
#include "buf.h"
#include "null.h"
#include "primes.h"
#include "archiver.h"

namespace ici
{

/*
 * Return a new ICI mem object refering to the memory at address 'base'
 * with length 'length', which is measured in units of 'accessz' bytes.
 * 'accessz' must be either 1, 2 or 4. If 'free_func' is provided it
 * will be called when the mem object is about to be freed with 'base'
 * as an argument.
 *
 * Returns nullptr on error, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
mem *new_mem(void *base, size_t length, int accessz, void (*free_func)(void *))
{
    mem  *m;

    if ((m = ici_talloc(mem)) == nullptr)
        return nullptr;
    set_tfnz(m, TC_MEM, 0, 1, sizeof (mem));
    rego(m);
    m->m_base = base;
    m->m_length = length;
    m->m_accessz = accessz;
    m->m_free = free_func;
    return memof(atom(m, 1));
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
    auto m = memof(o);
    return (unsigned long)m->m_base * MEM_PRIME_0
        + (unsigned long)m->m_length * MEM_PRIME_1
        + (unsigned long)m->m_accessz * MEM_PRIME_2;
}

void mem_type::free(object *o)
{
    auto m = memof(o);
    if (m->m_free != nullptr)
    {
        (*m->m_free)(m->m_base);
    }
    ici_tfree(o, mem);
}

int mem_type::assign(object *o, object *k, object *v)
{
    auto m = memof(o);
    int64_t i;

    if (!isint(k) || !isint(v))
        return assign_fail(o, k, v);
    i = intof(k)->i_value;
    if (i < 0 || i >= (int64_t)m->m_length)
    {
        return set_error("attempt to write at mem index %ld\n", i);
    }
    switch (m->m_accessz)
    {
    case 1:
        ((uint8_t *)m->m_base)[i] = intof(v)->i_value;
        break;

    case 2:
        ((uint16_t *)m->m_base)[i] = intof(v)->i_value;
        break;

    case 4:
        ((uint32_t *)m->m_base)[i] = intof(v)->i_value;
        break;

    case 8:
        ((int64_t *)memof(o)->m_base)[i] = intof(v)->i_value;
        break;
    }
    return 0;
}

object * mem_type::fetch(object *o, object *k)
{
    auto m = memof(o);
    int64_t i;

    if (!isint(k))
        return fetch_fail(o, k);
    i = intof(k)->i_value;
    if (i < 0 || i >= (int64_t)m->m_length)
        return null;
    switch (m->m_accessz)
    {
    case 1:
        i = ((uint8_t *)memof(o)->m_base)[i];
        break;

    case 2:
        i = ((uint16_t *)memof(o)->m_base)[i];
        break;

    case 4:
        i = ((uint32_t *)memof(o)->m_base)[i];
        break;

    case 8:
        i = ((int64_t *)memof(o)->m_base)[i];
        break;
    }
    o = new_int(i);
    o->decref();
    return o;
}

int mem_type::save(archiver *ar, object *o) {
    auto m = memof(o);
    if (ar->save_name(o)) {
        return 1;
    }
    const int64_t len = m->m_length;
    if (ar->write(len)) {
        return 1;
    }
    const int16_t accessz = m->m_accessz;
    if (ar->write(accessz)) {
        return 1;
    }
    if (ar->write(m->m_base, m->m_length * m->m_accessz)) {
        return 1;
    }
    return 0;
}

object *mem_type::restore(archiver *ar) {
    int64_t len;
    int16_t accessz;
    size_t sz;
    void *p;
    mem *m = 0;
    object *name;

    if (ar->restore_name(&name) || ar->read(&len) || ar->read(&accessz)) {
        return nullptr;
    }
    sz = size_t(len) * size_t(accessz);
    if ((p = ici_alloc(sz)) != nullptr) {
        if ((m = new_mem(p, len, accessz, ici_free)) == nullptr) {
            ici_free(p);
        }
        else if (ar->read(p, sz) || ar->record(name, m)) {
            m->decref();
            m = nullptr;
        }
    }
    return m;
}

} // namespace ici
