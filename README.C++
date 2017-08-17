-*- mode:markdown -*-

# A notes on the conversion of ICI's code from C to C++

This is a note about the changes in the ICI code resulting from its
re-working to be a C++ program. The note assumes familarity with the
previous C code.

# The general approach

The original C code's structure is essentially retained. All is as
before. Now its expressed using C++ with C++ used as a _better C_.

It is a testament to the previous code just how few changes were
required to compile it as C++.  Most code _just worked_. Code which
violated C++'s stricter ideas around function types and general const-
ness was the only real hurdle.

The transformation has been iterative with the interpreter kept
working following some initial hacking to make it work (some of which
is still left to fix). Once the initial work had been done numerous
changes were applied to transform the existing C code to better take
advantage of C++ behaviour and features.

## Not _modern_ C++

The style of C++ used is a mostly minimalistic approach, the so called
_Better C_ style. There's are no STL containers, no C++ strings,
exceptions or I/O streams. And certainly no BOOST! In general the
C++ standard library is avoided.

C++ features are used in many places but are used to reduce the amount
of code, improve its reliability and avoid repetition and boilerplate.

A small number of template functions are used to help define values of
different types or work around C++'s stricter type checks. But these
are well contained and there is no proliferation of templates which
often leads to slow to build code.

## C-style mostly removed

The various C-like ways of doings things have been mostly replaced
by the C++ ways of doing things.

C++'s constexpr is used to define constants that were previous
defined as macros.

Many of the previous macros are replaced by inline functions for
greater type checking and conversion. Overheads are hopefully
removed by today's fancy compilers.

NULL is largely replaced with C++'s nullptr.

The cstdef and cstdint headers' types are used - size_t, intXX_t
types and so on.

## ICI namespace

All ICI code now resides within a `namespace ici` and many of the
`ICI_` and `ici_` prefixes on identifiers have been removed.

This is really a reversion to the _old names_. The namespace makes
them safe to use now. The code is much easier to read as a result.

## No RAII. Yet.

There are a number of things in the code that could be better expresed
using small RAII classes. RAII, and the destructor, is arguably the
best thing about C++ and helps avoid numerous bugs.

# Major Changes

## ICI language changes

Athough not directly related to the change in the implementation
language the C++ version of ICI changes some keywords and builtin
functions names:

- `struct` -> `map`
- `auto` -> `var`
- `static` -> `local`
- `extern` -> `export`
- `thread` -> `go`

Why? The old names mimic C and while that was sort of nice ICI really
isn't C and the meanings were quite different. Really, the new names
are shorter. Renaming `thread` to `go` obviously shows a more recent
influence and the renaming of `struct` to `map` a C++ bias. That one
however does have somes advantages, `map` **not** being a C++ keyword
lets us use the name in code without disguise. But this is cosmetics.

## ICI objects

The ICI object header is now used as base-class, `ici::object`. All
the ICI object structures are defined inheriting from that class
replacing the C code's `o_head` convention.

The ici::object base class supplies the 32-bit object header and
member functions to do object-related things.

C++ rules mean that all ICI objects types _is-a_ `ici::object`
removing many type casts - `ici_objof` is no longer required.

All of the previous macros defined to work on ICI objects are now
(mostly) inline functions with extra functions defined to avoid direct
object header accesses.

The `ici::object` class defines inline function versions of the
per-type operations that apply to the _this_ object. These replace the
C code's direct access to an object's type table and calling the
per-type functions directly. The result is easier to read code.

## Member functions

Some types such as `ici::array` have many operations the C code
defined using _free_ functions. These are now member functions.  This
is an experiment really and not all typs have changed to this style
(member functions vs. free functions).  pefer the older approach but
its does work well with the array type.

## ICI types

ICI types are now represented by instances of classes derived from a
base `ici::type` class.

The `ici::type` class is a base class. It defined virtual member
functions for the various per-type operations. The different ICI
type's define classes that construct themselves appropriately and
override whatever member functions they need to override. This
replaces the initialized `struct ici_type` structures, function
pointers and static functions used in the C code - a directly
implemented virtual function table using explicitly coded dispatch.

The `ici::type` class provides default implementations of member
functions to provide the default behaviour, e.g. the default _fetch_
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
