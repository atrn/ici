#include <ici.h>

#include "util.h"

/*
 * array = pick(array, array|set)
 *
 * pick selects elements from an array using a collection of indices
 * and returns a new array containing only those elements.
 *
 * Indices may be supplied as an array or a set.
 */
int f_util_pick()
{
    if (ici::NARGS() != 2)
    {
        return ici::argcount(2);
    }

    if (!ici::isarray(ici::ARG(0)))
    {
        return ici::argerror(0);
    }

    if (!(ici::isarray(ici::ARG(1)) || ici::isset(ici::ARG(1))))
    {
        return ici::argerror(1);
    }

    auto source = ici::arrayof(ici::ARG(0));
    auto dest = ici::make_ref(ici::new_array());

    auto pick = [&source,&dest](ici::object *o, const char *from)
    {
        if (!ici::isint(o))
        {
            return ici::set_error("non-int in index %s", from);
        }
        auto e = source->fetch(o);
        if (e == ici::null)
        {
            return 0;
        }
        return dest->push_checked(e) ? 1 : 0;
    };

    if (ici::isarray(ici::ARG(1)))
    {
        auto index = ici::arrayof(ici::ARG(1));
        for (auto po = index->astart(); po != index->alimit(); po = index->anext(po))
        {
            if (pick(*po, "array"))
            {
                return 1;
            }
        }
    }
    else // isset
    {
        auto index = ici::setof(ici::ARG(1));
        for (size_t i = 0; i < index->s_nslots; ++i)
        {
            if (index->s_slots[i] != nullptr)
            {
                if (pick(index->s_slots[i], "set"))
                {
                    return 1;
                }
            }
        }
    }
    return ici::ret_no_decref(dest.release());
}
