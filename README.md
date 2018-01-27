# ici - a modified version of Tim Long's ICI interpreter

This is a heavily modified version of the ICI programming language
interpreter, originally written by Tim Long and worked on by myself
and others over the years.

ICI is a general purpose interpretive programming language that has
dynamic typing and flexible data types with the flow control
constructs and operators of C.

It is designed for use in many environments, including embedded
systems, as an adjunct to other programs, as a text-based interface to
compiled libraries, and as a cross-platform scripting language with
good string-handling capabilities.

The original ICI implementation was in C and distributed under a
public domain license. This version of ICI has been converted to be a
C++ program, modifies and extends the language in a number of ways and
is distributed uing the MIT license.

This version changes some language keywords, and standard function
names, and includes a number of extension modules in the base
interpreter. This provides more features in the basic environment and
specifically includes networking and object serialization as standard
features to support experiments in mobile agent programming.

The file [doc/CPLUSPLUS.md](doc/CPLUSPLUS.md) documents the C++
conversion and language keyword changes.

The keyword changes technically makes this version of ICI a different
language - ICI programs that worked with the previous C-implemented
interpreter are no longer valid. Sorry. However conversion can be
automated, its just naming, but I haven't done that. To be fair I had
previously called this version _anici_ (pronouced as in _anarchy_) and
had it use different file name extensions and naming but went back
to _ici_.

Since ICI development has been largely dormant for the past decade I
don't feel the changes to the language are a bad thing. The re-casting
of the interpreter as a C++ program is a major change and its a good
time to make such changes (ignoring the argument about if they should
be made at all :))

## Building

See [doc/BUILDING.md](doc/BUILDING.md) for details on how to build.

## Documentation

**Most documentation has not updated.**

The majority of the documentation under the `doc` directory is for the
older version of ICI. While the fundamentals of the language have not
changed some detail has, e.g. `struct` is now called `map`, and the
newer functions and types are not documented. Careful reading of the
[CHANGES](CHANGES) file is the suggested work-around.

The fundamental issue is extracting the documentation from the
FrameMaker `.fm` files and turning it into something that can be
maintained.
