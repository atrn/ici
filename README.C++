-*- mode:markdown -*-

# A notes on the conversion of ICI's code from C to C++

This is a  note about the changes  to the ICI code  resulting from its
re-working  from  C to  C++.  The  note  assumes familarity  with  the
previous C code and C++.

# The general approach

The interpreter's  architecture is retained.  All is as before  but is
now expressed in C++. C++ is mostly  used as a _better C_ and the code
is most definitely **not** in the, so-called, _modern C++_ style.

It is a  testament to the previous code how  few changes were required
to make it compile as C++.  Most  code _just worked_ and only the code
that violated  C++'s stricter  ideas around  function types  and const
required immediate change to allow  compilation. The initial C++ build
only required modifications  to only two contructs,  cfuncs and static
strings, to have  it function.  The current code uses  a lot more C++.
The  transformation  has  been  iterative with  the  interpreter  kept
working after each change.

## Not _modern_ C++

The style  of C++  used is  a mostly  minimalistic, _better  C_ style.
Very little of the C++ standard  library is used. ICI's types supplant
the use of C++ container types.

C++ features are used in many places but are mostly used to reduce the
amount of code, improve its reliability via stricter type checking and
avoid repetition and boilerplate. It works well.

A small number of template functions are used to help define values of
different types  or to work  with C++'s stricter type  checking. These
are well contained and there is no proliferation of templates which is
a hallmark of the modern C++ style but which often leads to slow build
times and larger and slower executables.

## C-style mostly removed

The various C-like ways of doings  things have been mostly replaced by
the C++ ways of doing things.

- C++'s constexpr is used to define constants
- parameterised macros are replaced by inline functions
- NULL is replaced with C++'s nullptr.
- cstddef and cstdint types are used - size_t, intXX_t, etc...
- C++ standard threads are assumed

## ICI namespace

All ICI  code now resides within  a `namespace ici` and  many, but not
all, of the `ICI_` and `ici_` prefixes on identifiers removed. This is
really a reversion  to the naming used in the  original ICI code, what
was termed _old names_. Prefixes were  added so ICI worked more nicely
as a C  library embedded within applications but  C++ namespaces allow
us to revert this. The code is easier to read as a result.

## No RAII. Yet.

There are a  number of things in the interpreter  that could be better
expresed using small RAII classes. This has not been done.

# Major Changes

## ICI language changes

Athough  not directly  related  to the  change  in the  implementation
language  the C++  version of  ICI changes  some keywords  and builtin
functions names:

- `struct` -> `map`
- `auto`   -> `var`
- `static` -> `local`
- `extern` -> `export`
- `thread` -> `go`

Why? The old names mimic C and  while that was sort of nice ICI really
isn't C  and the semantics  are quite  different. But really.  The new
names are shorter. The renaming of  `thread` to `go` obviously shows a
recent  influence and  the  renaming  of `struct`  to  `map` some  C++
bias. However that change does  have advantages. `map` **not** being a
C++ keyword lets us use the name in code without disguise.

## ICI objects

The ICI object header is now used as base-class however C++ `virtual`
is **not** used.

ICI object types  use inheritence to embed a standard  header and _be_
an  object.  The  struct  `ici::object` provides  the standard  object
header fields replacing the C  code's `o_head` convention. E.g.  a new
object type is defined via code that looks like,

    struct new_object_type : ici::object
    {
        ...  type-specific data
    };

`ici::object` supplies member functions to do object-related things.
Inline functions are used in place of the C code's macros and
direct manipulation of the header replaced by member functions.

C++ rules mean that all ICI objects types _is-a_ `ici::object`.  This
removes the need for many type casts and the `ici_objof` macro is no
longer required.

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
is an experiment really and not all typs have been changed to use this
style (member vs. free functions).  I generally pefer the free
function approach but members work reasonably well with the array
type.

## ICI types

ICI types are now represented by instances of classes derived from a
base `ici::type` class.

The `ici::type` class is a proper C++ base class defined using virtual
member functions  for the  various per-type operations.  The different
ICI types  define classes that construct  themselves appropriately and
override the  member functions  they need  to override.  This approach
replaces   the   initialized   `struct   ici_type`   structures   used
previously - a manually implemented virtual function table.

The  `ici::type`  class  provides default  implementations  of  member
functions to provide  the default behaviour, e.g.  the default _fetch_
implementation results in an error and so forth.

### More type operations

The per-type operations have been extended. All types now provide
_forall_, _save_ and _restore_ operations. _forall_ is used to
implement the _forall_ statement, _save_ and _restore_ defining
object serialization.

## Threads

The C++ standard threading support is used for ICI's thread related
code. This makes the code both portable and simpler.

## ICI file types

The old `struct ici_ftype` has been replaced with a class,
`ici::ftype` that defines an I/O interface for different types of
_files_. The C code again implemented a virtual dispatch table
which is now a C++ class with virtual member functions.

This actually makes things much clearer. In particular the
_popen_ file type is now derived from the _stdio_ type and
overrides a single function to implement itself.

The number of functions in the _ftype_ has been reduced.

## Now standard _modules_

The `sys`, `net`, `channel` and `serialization` modules are now
included in the base interpreter.

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
the character.

There will be C++ way to achieve the previous behavious of having the
compiler generate a complete image of the string object (of course
there is, this is the language with `constexpr` functions).
