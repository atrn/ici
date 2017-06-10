// -*- mode:c++ -*-

#ifndef ICI_ALLOCATOR_H
#define ICI_ALLOCATOR_H

#include <memory>

#include "alloc.h"

namespace ici
{

// A std::allocator that uses the ICI allocator.
//
template <typename T>
class allocator
{
public:
    typedef T value_type;

    allocator() = default;
    template <class U> allocator(const allocator<U>&) {}

    value_type* allocate(std::size_t n)
    {
        return static_cast<value_type*>(ici_nalloc(n*sizeof (value_type)));
    }

    void deallocate(value_type* p, std::size_t n)
    {
        ici_nfree(p, n);
    }
};

template <class T, class U>
bool operator==(const allocator<T>&, const allocator<U>&) { return true; }

template <class T, class U>
bool operator!=(const allocator<T>&, const allocator<U>&) { return false; }

} // namespace ici
