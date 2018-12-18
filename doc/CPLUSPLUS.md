# A note on the conversion of ICI's code from C to C++

This is a  note about the changes  to the ICI code  resulting from its
re-working  from  C to  C++.  The  note  assumes familarity  with  the
previous C code and C++.

## The general approach

The interpreter's  architecture is retained.  All is as before  but is
now expressed in C++. C++ is mostly  used as a _better C_ and the code
is most definitely **not** in the, so-called, _modern C++_ style.

It is a  testament to the previous code how  few changes were required
to initially compile it as C++.  Most  code _just worked_ and only code
that violated  C++'s stricter  ideas around  function types  and const-ness
required immediate change to permit  compilation. The initial C++ build
required modifications  to only two contructs,  cfuncs and static strings.
The current code uses  a lot more C++.  The  transformation  has  been
iterative with  the  interpreter  kept working after each change.

## Not _modern_ C++

The style  of C++  used is  a mostly  minimalistic, _better  C_ style.
Very little of the C++ standard  library is used. ICI's types remove
the the need to use C++ container types and the C++ standard library
way of doing things is generally avoided.

C++ features are used in many places but are mostly used to reduce the
amount of code, improve its reliability via stricter type checking,
simpler reference count management and the avoidance of repetition and
boilerplate. It works well.

A number of template functions are used to help define values of
different types, track object references, and to easily work with
C++'s stricter type checking. These are well contained and there is no
proliferation of templates which is a hallmark of the modern C++ style
but which often brings slow build times, and larger, slower, programs.

## C-style mostly removed

Various C-isms have been replaced by C++-isms:

- inlines functions replace parameterised macros
- NULL becomes nullptr.
- C++'s constexpr is used to define constants
- cstddef and cstdint types are used - size_t, intXX_t, etc...
- C++ standard threads are assumed

### RAII

The ref<> template wraps a pointer to an ICI object and manages the
reference count. A ref<T> otherwise acts as the T. ref<> is used to
protect against object-related memory leaks.

## Namespaces

All ICI code now resides within a `namespace ici` and many, but not
yet all, of the `ICI_` and `ici_` prefixes used on identifiers have
been removed. This is actually a reversion to the naming used in the
first ICI code, what was termed _old names_ in later version. The
prefixes being added when ICI was a C library to be embedded within
applications C++ namespaces allow us to revert this change and the
code is easier to read as a result.

## ICI language changes

Athough not directly related to the change in implementation language
this version of ICI changes some keywords and builtin functions names:

- `struct` -> `map`
- `auto`   -> `var`
- `static` -> `local`
- `thread` -> `go`

Why? The old names mimic C's names with semantics that somewhat
resemble C's if you don't look at things too closely.  While that was
kind of nice, or at least cute, ICI really isn't C and the actual
semantics of each statement are quite different. But really...the new
names are shorter.

The renaming of `thread` to `go` obviously shows a recent influence as
does the renaming of `struct` to `map`. That particular change does
have one major advantages - `map` is **not** a C++ keyword we can use
it in C++ code without disguise unlike `struct`.  No more
`ici_struct`! 

Adopting `var` is for the Javascript people. The implicit `var` using
the `:=` assignment operator largely replaces much of its use
anyway.

## ICI objects

The ICI object header is now used as base-class `ici::object`, and is
used to provide the standard ICI object header fields. `ici::object`
is **not** a C++ _polymorphic base_ and has no virtual functions.

All ICI object types inherit from `ici::object` struct to embed the
standard object header and allow them to _be an object_.  Inheriting
the header struct replaces the C code's convention of having all
object types start with an `o_head` object header value. The C++
representation is easier to write and read, e.g.

    struct new_object_type : ici::object
    {
        ...  type-specific data
    };

`ici::object` supplies numberous member functions to do object-related
things.  Inline functions are used in place of the C code's use of
macros and much of the direct manipulation of header fields has been
replaced by inline member functions with descriptive names.  Modern
compilers are smart enough to collapse it all.

The C++ type rules mean that all types of ICI objects are _is-a_
`ici::object` (sic).  This removes the need for many casts means
the downcasting `ici_objof` macro is no longer required. It has
been removed.

All of the previous macros defined to work on ICI objects are now
inline functions. Extra functions are defined to avoid direct object
header accesses.

The `ici::object` class defines inline function versions of the
per-type operations that apply to the _this_ object. These replace the
C code's direct access to an object's type table and calling the
per-type functions directly.

The result of all the changes is easier to read code.

## Member functions

Some types such as `ici::array` have many operations the C code
defined using _free_ functions. These are now member functions.  This
is an experiment really and not all types have been changed to use this
style (member vs. free functions).  I generally pefer the free
function approach but members work reasonably well with the array
type.

## ICI types

ICI types are now represented by instances of classes derived from a
base `ici::type` class.

The `ici::type` class is a C++ base class with virtual member functions
for the  various per-type operations.  The different ICI types define
classes that construct  themselves appropriately and override the member
functions  they need  to override.  This approach replaces   the   initialized
`struct   ici_type`   structures   used previously - which was just a manually
implemented virtual function table.

The  `ici::type`  class  provides default  implementations  of  the
different member functions to provide  the default behaviourf for types.
e.g.  the default _fetch_ and _assign_ implementations result in errors
(equivalent to the C code's _fetch_fail_ and _assign_fail_) and so forth.

### More type operations

The per-type operations have been extended. All types now provide
_forall_, _save_ and _restore_ operations. _forall_ is used to
implement the _forall_ statement, _save_ and _restore_ defining
object serialization. This allows forall'ing over user-defined
types and makes serialization more efficient.

The change to a per-type forall allowed channels to be used
in forall statements with only the implementation of a single
function.

## Threads

C++ standard threading support is used for ICI's thread related
code. This makes the code both portable and simpler.

## ICI file types

The old `struct ici_ftype` has been replaced with a class,
`ici::ftype` that defines an I/O interface for ICI files.
The C code again implemented a virtual dispatch table
which is now expressed via a C++ class with virtual member
functions.

This makes things much simpler. In particular the _popen_ file
type is now derived from the _stdio_ type and overrides a single
function to implement itself.

The number of functions in an _ftype_ has been reduced too.

## Now standard _modules_

The `sys`, `net`, `channel` and `serialization` modules are now
included in the base interpreter. Serialization has been embedded
into the language now and the code cleaned up.

## Issues (worked-around)

### cfuncs

The C++ stronger typing makes cfuncs even more problematic. ANSI C
rules made cfuncs rely on undefined behavour and C++ made that
illegal. The workaround is a template'd constructor function that
accepts the different function types and forces the type cast
(undefined behaviour). It works on the system's I've tested.

### Static strings

The old approach to defining static ICI string objects was hackily
replaced with a version to work around me not figuring out the
appropriate C++-ims to statically initialize the array used to store
the static string's characters. There will be a method, perhaps ugly,
but I gave up at the time (one of the first things changed) and copied
the characters at startup.
