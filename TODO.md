# ICI TODO

This file documents a list of things that ICI needs. If you feel like
doing one of them then by all means do so and if you are public minded
donate your changes.

## General

### strings

#### Remove reliance on C strings

So strings with NULs can be handled properly. There is a general
problem all over the interpreter - we assume C strings. We pass
ICI's string characters directly to C functions which can see a
truncated string if the ICI string has an embedded NUL.

#### UTF-8

Should adopt UTF-8 everywhere. And all it entails.

## Modules

Native code modules need to be collectable and unused modules
unloaded. This requires cfuncs and mem objects that reference
the module have references to it which can be dropped and gc
used to unload.

## Other Fixes

### Add esc() function.

### Make waitfor be able to wait on a set.

### Mention the old form of smash in the documentation.

### Document things defined in icicore.ici

### Resurrect win stuff as an extension module and document.

### Have a nicer way of convering an array to a set.

setof(array) ?  It can be done via "call(set, array)" but that's
a little idiosyncratic.

### Add optional seperator string to implode().

### top() and pop() should work on sets.

### copy() of a string results in a non-atomic string

I don't think this is made clear enough in the documentation.
