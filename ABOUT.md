This is heavily modified version of Tim Long's ICI.  This variant has
numerous changes and extensions but is fundamentally the same language
(and implementation).

Changes include modified ICI keywords and function names for aesthetic
reasons. Programs written using this version are *not* really ICI
programs. For instance ICI defines a type called "struct" (a
hierarchical dictionary) which is renamed "map" in this version.

The definition of ICI's "int" type is also changed. An "int" is always
64 bits in this version whereas ICI defines it to be same as C's long
(which varies in size).

A number of what where previously dynamically loaded modules are now
part of the standard interpreter. The "archive" (object
serialization), "net" (network sockets) and "sys" (system calls)
modules are now intrinsic and their functions, types and other objects
part of the base environment.

Further additional standard functions are provided using the auto-
loaded "core" modules.

# implementation changes

This version of ICI is re-implemented using C++ and changes most of
the previous C code in substantial ways to better use C++ features.
The overall structure of the interpreter is essentially the same but
the details are very different in many places. Often the changes are
"mechanical", re-casting a previous C-style construct in C++. And in
some places where ICI implemented, in C, its own versions of normal
C++ mechanisms (e.g. virtual functions) the C++ native feature is
used.

The C++ standard library is **NOT** used.


## naming

The C implementation uses "ici\_" as a prefix on names. The C++
code all resides with an "ici" namespace and prefixes have been
removed. Interestingly this is really a retraction. In ICI's ealier
implementations there was no "ici\_" prefix.

## C++ transformations

### inline functions replace macros

The first, and theoretically simplest, transformation is that
inline functions replace the previous function-like macros.

### ici::object inheritence

### ici::type classes

### cfunc

### std::thread

ICI's two different, optional, thread implementations (one for Windows
and one for POSIX threads) are replaced with a single implementation
based on C++ standard threads.

# object serialization

The archive module's save and restore functions are now standard.
These can be used to serialize and deserialize arbitrary graphs of ICI
objects, including functions.

## serialization protocol

The save() and restore() functions work with a binary representation
of an object graph.  All standard types are supported including
functions which are communicated in their compiled, virtual machine
form and structures with cycles.  Atomic objects are sent once within
a specific "session" making for moderate efficiency gains for large
numbers of repeated structures and similar objects with repeated
atomic objects, typically strings.

The protocol is a tagged, type protocol.

Each object is sent as a one byte object type identifier followed by
some object specific representation.
