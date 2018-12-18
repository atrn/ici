#include "intf.h"

#include "fwd.h"
#include "array.h"

extern "C" {

    struct ici_object : ici::object {};
    struct ici_array : ici::array {};

    EXPORT int ici_init(void) {
        return ici::init();
    }

    EXPORT void ici_uninit(void) {
        ici::uninit();
    }

    EXPORT void ici_incref(ici_obj_t *o) {
        o->incref();
    }

    EXPORT void ici_decref(ici_obj_t *o) {
        o->decref();
    }

    EXPORT ici_array_t *ici_array_new(size_t z) {
        return static_cast<ici_array_t *>(ici::new_array(z));
    }

    EXPORT ici_obj_t **ici_array_astart(ici_array_t *a) {
        return reinterpret_cast<ici_obj_t **>(a->astart());
    }

    EXPORT  int ici_array_push(ici_array_t *a, ici_obj_t *o) {
        return a->push_back(o);
    }

} // extern "C"

