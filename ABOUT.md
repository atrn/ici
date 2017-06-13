# anici

This is anici, a programming language based on Tim Long's ICI with
numerious additions and changes for the purposes of supporting a
distributed system known as "the worm farm".

Like ICI anici is a C-like interpretive language with dynamic types,
automatic memory management, error reporting, and other nicities
afforded by a runtime environment.

anici extends ICI with direct access to system calls and BSD sockets
networking, object serialiation and inter-thread communications using
channels.

anici changes some ICI keywords and function names for aesthetic
reasons and programs written using anici are *not* ICI programs.

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
