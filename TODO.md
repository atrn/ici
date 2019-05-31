# ICI TODO

This file documents a list of things that ICI needs. If you feel like
doing one of them then by all means do so and if you are public minded
donate your changes.

## Frames

The basic types are there, and functions to get them but that's
about all. They need:

- forall
- vector operators
- serialization
- I/O (printf)

## General

### Remove reliance on C strings and adopt UTF-8 everywhere

So strings with NULs can be handled properly. There is a general
problem all over the interpreter - we assume C strings. We pass
ICI's string characters directly to C functions which can see a
truncated string if the ICI string has an embedded NUL.

### Modules

Native code modules should be collectable with unused modules being
automatically unloaded. This requires the cfuncs and other objects
that reference the contents of the module references some type of
object which holds the module open. When the references are collected
the module can be dlclose'd and unloaded.

### Add esc() function.

What does it do? I forget. Perhaps outputs a "clean string" wit
problematic characters escaped?

### Make waitfor be able to wait on a set.

### Mention the old form of smash in the documentation.

### Document things defined in icicore.ici

### Resurrect win stuff as an extension module and document.

### Have a nicer way of convering an array to a set.

setof(array) ?  It can be done via "call(set, array)" but that's
a little idiosyncratic.

### add a unique() function

It can be done in a single ICI expression,

    keys(call(set, ary))
    
But we can do that a little more efficiently in native code.

Form a set from the array to discard duplicates, get the contents of
the set in a vector via keys().  And if sorted key are desired you
just call sort on the result of keys().

    sort(keys(call(set, v)))

### Add optional separator string to implode().

join() does this and replaces many uses of implode.

### copy() of a string results in a non-atomic string

I don't think this is made clear enough in the documentation.
