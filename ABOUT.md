# anici

This is anici, a programming language based on Tim Long's ICI.  anici
has numerous changes and extensions to ICI but is fundamentally the
same language (and implementation).

Like ICI anici is a C-like interpretive language with dynamic types,
automatic memory management, error reporting, and other nicities
afforded by a runtime environment.

anici changes ICI keywords and function names for aesthetic reasons
and programs written using anici are *not* ICI programs. For instance
ICI defines a type called "struct" (a hierarchical dictionary) which
in anici is renamed to "map".

anici also changes the definition of ICI's "int" type. An anici "int"
is always 64 bits whereas ICI defines it to be same as C's long who's
size is platform-specific.

anici incorporates a number of what where previously dynamically
loaded modules. The "archive" (object serialization), "net" (network
sockets) and "sys" (system calls) modules are now intrinsic and their
functions, types and other objects part of the base environment.

Further additional standard functions are provided using the auto-
loaded "core" modules. anici includes standard functional programming
style constructs.

# implementation changes

anici is implemented using C++ and changes the previous ICI C code in
substantial ways to make use of C++ features. The overall structure of
the interpreter is the same but the details are very different in
places. Often the changes "mechanical", re-casting a previous C-style
construct in C++. In some places ICI implemented, in C, its own
versions of typical C++ mechanisms (virtual functions) and these
instances have been replaced.

## naming

The C implementation uses "ici\_" as a prefix on names. The anici C++
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

anici replaces ICI's two different, optional, thread implementations
(one for Windows and one for POSIX threads) with a single
implementation based on C++ standard threads.

# object serialization

The goal of anici has always been to use ici as a language for
mobile programs - programs that can re-locate their execution
location while preserving state.

This was acheived by adding a generic object serialization mechanism
to ICI. As ICI code has the same basic representation as data this
allows code, i.e. functions, to be serialized in addition to code.
Mobile programs can then be realized by exchanging serialized
functions objects over network links.

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
