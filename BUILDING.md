# How To Build

## Summary

Supported platforms:

- MacOS
- Most Linux-based OSes
- BSD's

## Building

    $ make conf=conf/`uname|tr A-Z a-z`.h

## Building with `dcc`

The supported build system for ICI uses GNU `make` and my compiler
driver program, `dcc`. GNU `make` is used to _direct_ the overall
build process with `dcc` taking care of compilation, linking and
library creation.

You likely already have GNU `make` or know where to get it.

### Getting `dcc`

`dcc` is open source (licensed under the GNU GPL v2) and available
from [http://github.com/atrn/dcc](http://github.com/atrn/dcc).

As mentioned `dcc` is a compiler driver. Like `cc(1)` but a compiler
driver that automatically creates and uses object-file dependency
information, and uses other rules, to essentially hard-code a typical
make-like build process into the compiler (driver) itself.

`dcc` is written in Go and installation requires Go be installed. Some
platforms do this via their package manager otherwise Go can be
obtained via [golang.org](http://golang.org/). Installation is
straight forward. Follow the, simple, instructions.

Assuming a conventional Go installation (i.e. you followed the
instructions), installing `dcc` is a single command,

    $ go get github.com/atrn/dcc

By default `go get` will install the binary under your _$GOPATH_,
which is `~/go/bin` if not defined otherwise I assume you know what
you're doing and where to find the executable.

## Building without dcc

Building a static version of ici is essentially trivial on current
UNIX-ish systems. The following command builds a static executable,

    $ c++ -std=c++14 -I. -UNDEBUG *.cc etc/main.cc -o ici

## cmake

There is a primitive `CMakeLists.txt` file that works to compile a
simple interpreter executable. It doesn't all the things the Makefile
driven build does, but of course could with sufficient _cmake-ing_.
There are **no** plans to move builds to use cmake but contributions
will be of course accepted. I'm happy with make+dcc.
