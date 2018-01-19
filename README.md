# ici - a heavily modified version of Tim Long's ICI interpreter

This is a heavily modified version of the ICI programming language
interpreter, originally written by Tim Long and worked on by myself
and others over the years.

This version of ICI has been converted to be a C++ program and
modifies and extends the language in a number of ways. Some
fundamental language keywords have been changed and some, what were
extension modules, incorporated into the base interpreter. This
provides a more feature-ful environment.  In particular this version
of ICI includes networking and object serialiation as standard
features allowing it to be used for my _anici_ experiment (mobile
agent programming).

The file (README.C++)[README.C++] documents the C++ conversion and
keyword changes.

The ketword changes technically makes this version of ICI a different
language - ICI programs that worked with the previous C-implemented
interpreter are no longer valid. Sorry. However conversion can be
automated, its just naming, but I haven't done that.

As ICI development has largely been dormant for the past decade I
don't feel the change to the language is a bad thing. The re-casting
of the interpreter as a C++ program is a major change.

The file (README)[README] is the original _readme_ file from
the C distribution.

The ICI documentation has **not** been updated to reflect the
changes and additions.

## License

ICI was previously distributed under a public domain license and the
incorporated extension modules were either in the public domain or my
work and copyright.

This version of ICI is copyright "me" and licensed under an MIT style
license to permit free use. The C++ conversion creates a derived work
from the original public domain code but that work has signficant
differences.
