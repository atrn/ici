# Ideas for ICI

Wild and crazy ideas that likely will never be implemented.

## Generational GC.

Split the object pool into arenas of different _ages_ to limit the
scope of initial GC mark/sweeps - GC more recent objects first and
only (re-)visit older objects if that fails to release sufficient
memory or perhaps at some _rate_ (every N calls to collect()) to
detect now dead old objects.

## Make integers arbitrary precision

Make the default `int` type an arbitray precision integer, a
_bigint_. Do the same for "float", make it an arbitrary precision,
real valued _bignum_ type (perhaps with user control of the actual
representation a la REXX).

Effectively removes any size limitations from the language. Arrays are
index by bigint and can be as large as is permitted.

Then add exact sized types to allow for those:

- int32
- int64
- float32
- float64

Possibly do some internal optimizations to cope with common integer
sizes, i.e. map things to int64_t if possible. That can be done in the
bigint itself I suppose.

### Cons

Performance will go down and memory use will go up. The impact on
performance should not be too much of an issue due to the way ICI is
used - it's not high performance to begin with and any _heavy lifting_
is done in C/C++ (Rust?) libraries.

The binop switch will _blow out_ with a large increase in the number
of combinations of arithmetic operator cases - coping with the
_overloads_ for the, now, **six** numeric types as opposed to the
current two. Perhaps there's a nice way to generalize things (without
C++ templates and the assoicated code replication).

Formatting (f_sprintf) will need to cope correctly with bigint and
bignum values and do the right thing re. precision/width specs.

## Re-implement my user-defined operator "hack"

This feature allowed code to register functions to be called when
unknown operator/type combinations were encountered providing a form
of user-defined operators.

## Move more things into the 'ici' map

e.g. `reclaim()` -> `ici.reclaim()`

## Allow classes to implement fetch/assign/forall/keys

Resurect my old _utype_ (user type) module, adding support the newly
added _forall_ and _keys_ per-type functions, and incorporate it into
the language in some manner.

The _utype_ modules allows users to implement ICI types in ICI. User
code implemenenting a number of ici::type functions to implement
types, specifically it lets users implement _fetch()_, _assign()_,
_forall()_ and _keys()_. This provides type-specific _indexing_,
`forall`, `keys()` and serialization.
