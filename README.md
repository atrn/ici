# ici - a modified version of Tim Long's ICI interpreter

This is a modified version of the ICI programming language
interpreter, originally written by Tim Long and worked on by myself
and others over a number of decades.

ICI is a general purpose interpretive programming language that has
dynamic typing and flexible data types with the flow control
constructs and operators of C.

It is designed for use in many environments, including embedded
systems, as an adjunct to other programs, as a text-based interface to
compiled libraries, and as a cross-platform scripting language with
good string-handling capabilities.

ICI was first made available in the early 1990s with the original
implementation being in C. ICI was donated to the public domain.
Given their some supericial simalarities it probably should be
noted that ICI predates Javascript.

At that time ICI had a number of users and formed the basis for a
number of graphics oriented systems, most notably Canon's OpenPage
graphics langugage.

This version of ICI has been converted use C++ and modifies and
extends the language in a number of ways.  It is distributed under an
MIT license.

## Changes from the previous ICI

The major changes compared to previous versions include,

- some language keywords and standard function names changed
- new types for numeric programming
- useful extension modules made standard, e.g. networking
- adds a simple REPL

### Keywords and functions

To further distance ICI from its C inspired heritage several keywords
have been changed. For instances, the previous `struct` type is now
called `map`, mimicing C++ to a degree atop ICI's generic dictionary
type. Further details are in the `CHANGES` file.

The keyword changes technically makes this version of ICI a different
language - ICI programs that worked with the previous C-implemented
interpreter are no longer valid. Sorry. However, conversion can be
automated, its just naming, but I haven't done that. To be fair I had
previously called this version _anici_ (pronouced _anarchy_) and had
it use different file name extensions and naming but went back to
_ici_.

Since ICI development has been largely dormant for the past decade I
don't feel the changes to the language are a bad thing. The re-casting
of the interpreter as a C++ program is a major change and its a good
time to make other changes (ignoring the argument about if they should
be made at all :))

### New types

Two types, `vec32f` and `vec64f`, have been added to provide for more
efficient numeric programming. A `vec32f` is a, C, array of single-
precision floating point values and a `vec64f` an array of double
precision values. Vecs act as variable sized arrays of up to a defined
_capacity_ and support the basic arithmetic operators and assignment
operators - `+`, `-`, `*`, `/`, `+=`, `-=`, `*=`, `/` - with both vec
and scalar operands. Assignment operators are optimised to avoid
copying data while binary operators create new values of the
appropriate vec type.

On systems with Intel's IPP libraries the different operations can
use the IPP functions for their implementation with a corresponding
"ipp" module providing access to other functions provided by IPP.

### Standard modules

The `sys`, `net` and `channel` modules used with previous version of
ICI are now builtin to the interpreter. This adds more complete UNIX
system call support, networking and channel-based inter-thread comms.
Object serialization, the `save()` and `restore()` functions from my
previous _anici_ project, is now a standard language feature, defined
as a basic function of ICI types.


### Conversion to C++

The file [doc/CPLUSPLUS.md](doc/CPLUSPLUS.md) documents the C++
conversion and language keyword changes.

## Building

See [doc/BUILDING.md](doc/BUILDING.md) for details on how to build.

## Documentation

**MOST DOCUMENTATION HAS NOT BEEN UPDATED.**
**THE PDF IS OUT OF DATE AND I CAN"T CONVERT THE FRAMEMAKER FILE.**

The majority of the documentation under the `doc` directory is for an
older version of ICI. While the fundamentals of the language have not
changed some of the detail has, e.g. the `struct` type is now called
`map`. Also newer functions and types are not documented. Careful
reading of the [CHANGES](CHANGES) file is one suggested work-around.

The file [doc/html/ici.html](doc/html/ici.html) *has* had the
name changes applied and is a good starting point to learn
about the language.

### Why?

The fundamental issue with the documentation is extracting it from
the FrameMaker `.fm` files! Even getting a `.mif` file would be better
than the `.fm` files.
