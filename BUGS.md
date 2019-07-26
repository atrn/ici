# Broken Things

## Loadable Modules Issue

Modules get built by linking them against the ICI shared
library. Because that library has internal state that requires
initialization by calling its ici::init() function, loading a modules
that reference the library into a program that does NOT use the
library, e.g. a statically linked ICI executable, results in a
crash. The library's version of the ICI object pool has not been
created and the module wants to use it.
