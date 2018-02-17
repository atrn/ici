// -*- mode:c++ -*-

#ifndef ICI_REF_H
#define ICI_REF_H

#include "object.h"

namespace ici {

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

// A ref<T> is an RAII class that manages an object's reference count.
// To summarise: a ref<T> owns an object, via its address, and decrefs
// its object's reference count when the ref is destroyed. The ref<T>
// implements other operations - copy, move, assign - to maintain a
// correct reference count.
//
// A ref<T> is typically used to hold object references in code that
// can fail and return errors. Using ref's to hold object references
// removes the most common form of resource leak.
//
// The ref<T> type provides a release() function, much like other
// _smart pointer_ types, to transfer ownership of the underlying
// object.
//
template <typename T = object>
class ref {
public:
    ref(T *obj = nullptr) : _obj(obj) {}

    ref(T *obj, struct tag_with_incref) : _obj(obj) {
        if (_obj) {
            _obj->incref();
        }
    }

    ref(const ref &that) : _obj(that._obj) {
        if (_obj) {
            _obj->incref();
        }
    }

    ref(ref &&that) : _obj(that.release()) {}

    ref &operator=(const ref &that) {
        _obj = that._obj;
        if (_obj) {
            _obj->incref();
        }
        return *this;
    }

    ref &operator=(ref &&that) {
        _obj = that.release();
        return *this;
    }

    ~ref() {
        if (_obj) {
            _obj->decref();
        }
    }

    operator T *() {
        return _obj;
    }

    T *operator->() {
        return _obj;
    }

    T &operator*() {
        assert(_obj != nullptr);
        return *_obj;
    }

    operator const T *() const {
        return _obj;
    }

    const T *operator->() const {
        return _obj;
    }

    const T &operator*() const {
        assert(_obj != nullptr);
        return *_obj;
    }

    T * release() {
        auto obj = _obj;
        _obj = nullptr;
        return obj;
    }

private:
    T *_obj;
};

// make_ref applies argument type deduction to turn a T * into an
// ref<T>.
//
template <typename T>
ref<T> make_ref(T *obj) {
    return ref<T>(obj);
}

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif // ICI_REF_H
