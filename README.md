# ici - a heavily modified version of Tim Long's ICI interpreter

This is a heavily modified version of the ICI programming language
interpreter, originally written by Tim Long and worked on by myself
and others over the years.

This version of ICI has been converted to be a C++ program and
modifies and extends the language in a number of ways. Some language
keywords have been changed and a number of extension modules
incorporated directly into the base interpreter. This provides more
features in the basic environment, specifically, it includes
networking and object serialization modules as standard features to
support experiments with mobile agent programming.

The file [CPLUSPLUS.md](CPLUSPLUS.md) documents the C++ conversion and
language keyword changes.

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

The file [README](README) is the original _readme_ file from the C
distribution and some of may still apply. I haven't checked. In
general, other than in these _readme_ files, the ICI documentation has
**not** been updated to reflect changes and additions.

## License

ICI was previously distributed under a public domain license and the
incorporated extension modules were either in the public domain or my
work and copyright.

This version of ICI is copyright "me" and licensed under an MIT style
license to permit free use. The C++ conversion creates a derived work
from the original public domain code but that work has signficant
differences.
