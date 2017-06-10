# anici

This is anici, a programming language based on Tim Long's ICI
language with additions and changes made for the purposes of
supporting a distributed system known as "the worm farm".

anici is a C-like interpretive language with dynamic types, memory
management, error reporting, and other nicities afforded by the
interpretive runtime environment. anici includes useful builtin
data structures.

## Overview

See the CHANGES file for the full details of the changes but in
summary anici is essentially ICI with the following changes,

- anici uses different keywords or builtin function names in
  a number of places for aesthetic reasons,

- anici incorporates a number of ICI modules as standard
  make for a richer programming environment,

  str           extra string functions
  path          pathname manipulation
  sys           system calls
  net           network sockets
  channel       inter-thread channels
  archive       object serialization
  errno         error processing


And as part of this anici is,

- written using C++
- 64-bit safe


## anici features

### object serialization

The goal of anici has always been to use ici as a language for
mobile programs. This was acheived by adding a generic object
serialization mechanism to ICI. And as ICI code has the same
basic representation as data it too was able to be serialized
and eventually communicated over networks for later restoration
and execution.

Given the ability to transfer ICI code and data a generic model
of a module program, or agent, was defined. An agent is simply
an ICI struct object with an arbitrary number of key/values. One
of the values however must be a function object with the key "ego".
The agent's "ego" function is called when the agent is instantiated
within some execution environment and is called with two actual
arguments, the object's struct and a struct defining access to
the local environment. I.e.

  If we have some object, lets call it "self"....


    self = [struct
        ... stuff

        ego = [func(self, environ) {
        },
    ]

And when communicated over the network and instantiated
within an execution location the basic process is...

    object = restore(network); // receive from network
    if (fork()) // create a new process
      object.ego(object, environment); // run the received object

This object serialization mechanism was used for a distributed
systems experiment using mobile programs as the basis for all
communications. Mobile programs can move around a network taking
their state with them when they relocate their execution location.

Thanks to ici being interpretered we can easily control the execution
environment (in theory) and enforce security policies or tailor standard
functions to specific users and so on. Of course coding mistakes often
introduce holes in this scheme but the code base is small enough to
be audited if need be.


* serialization protocol

The save() and restore() functions work with a binary representation
of an object graph.  All standard types are supported including
functions which are communicated in their compiled, virtual machine
form and structures with cycles.  Atomic objects are sent once within
a specific "session" making for moderate efficiency gains for large
numbers of repeated structures and similar objects with repeated
atomic objects, typically strings.

The protocol is a tagged, type protocol.

Each object is sent as a one byte object type identifier followed by
an object specific representation.