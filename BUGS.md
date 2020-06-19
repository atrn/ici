
# Broken Windows

## Expression Code

Expressions who's value is not assigned do not generate any code,
even if the expressions have side effects.

E.g. this permits users to write "bad" code such as,

	stdout + float(argc);

Because the expression is not evaluated there is no,
dynamic, check of the operand types and the the
expression produces no error, float() is not , 'argc'
is not evaluated and the `+` is not performed.

## Loadable Modules Issue

Modules get built by linking them against the ICI shared
library. Because that library has internal state that requires
initialization by calling its ici::init() function, loading a modules
that reference the library into a program that does NOT use the
library, e.g. a statically linked ICI executable, results in a
crash. The library's version of the ICI object pool has not been
created and the module wants to use it.
