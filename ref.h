// -*- mode:c++ -*-

#ifndef ICI_REF_H
#define ICI_REF_H

#include "object.h"

namespace ici {

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

/**
 *  A ref<T> is an RAII class to manage an object's reference count.
 *  A ref<T> owns an object and acts as that object but will decref
 *  the object's reference count when destroyed unless the object is
 *  released from the ref's control (like other smart-pointers).  The
 *  ref<T> implements other operations - copy, move, assign - to allow
 *  it to maintain a correct reference count.
 * 
 *  A ref<T> is typically used to hold object references in code that
 *  can fail and return errors. Using ref's to hold object references
 *  removes the most common form of resource leak.
 */
template <typename T = object>
class ref {
public:
    ref(T *obj = nullptr)
        : _obj(obj)
    {
    }

    ref(T *obj, struct tag_with_incref)
        : _obj(obj)
    {
        if (_obj) {
            _obj->incref();
        }
    }

    ref(const ref &that)
        : _obj(that._obj)
    {
        if (_obj) {
            _obj->incref();
        }
    }

    void reset(T *obj) {
        release();
        _obj = obj;
    }

    void reset(T *obj, struct tag_with_incref &) {
        release();
        if ((_obj = obj)) {
            _obj->incref();
        }
    }

    T * release() {
        auto obj = _obj;
        _obj = nullptr;
        return obj;
    }

    T * release(struct tag_with_decref &) {
        auto obj = _obj;
        _obj = nullptr;
        if (obj) {
            obj->decref();
        }
        return obj;
    }

    ref(ref &&that)
        : _obj(that.release())
    {
    }

    ref &operator=(const ref &rhs) {
        if ((_obj = rhs._obj)) {
            _obj->incref();
        }
        return *this;
    }

    ref &operator=(ref &&rhs) {
        _obj = rhs.release();
        return *this;
    }

    ~ref() {
        if (_obj) {
            _obj->decref();
        }
    }

    T *get() {
        return _obj;
    }

    const T *get() const {
        return _obj;
    }

    operator T *() {
        return get();
    }

    T *operator->() {
        return get();
    }

    T &operator*() {
        assert(_obj);
        return *get();
    }

    operator const T *() const {
        return get();
    }

    const T *operator->() const {
        return get();
    }

    const T &operator*() const {
        assert(_obj);
        return *get();
    }

private:
    T *_obj;
};

/**
 *  make_ref<> uses argument type deduction to return the ref<T> for
 *  the given T *.
 */
template <typename T>
ref<T> make_ref(T *obj) {
    return ref<T>(obj);
}

/**
 *  make_ref<> with an incref.
 */
template <typename T>
ref<T> make_ref(T *obj, struct tag_with_incref &) {
    return ref<T>(obj, with_incref);
}

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif // ICI_REF_H
