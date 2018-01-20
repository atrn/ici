# How To Build

## Summary

Supported platforms:

- MacOS
- Most Linux-based OSes
- BSD's

Steps:

- install `dcc`
- type `make` (`gmake` on BSDs)

## Building

The supported build system for ICI uses GNU `make` and my compiler
driver program, `dcc`. You likely already have GNU `make` or know
where to get it.

### Getting `dcc`

`dcc` is open source (licensed under the GNU GPL v2) and available
from [http://github.com/atrn/dcc](http://github.com/atrn/dcc). `dcc`
is written in Go and installation requires Go be installed. Some
platforms do this via their package manager otherwise Go can be
obtained via [golang.org](http://golang.org/). Installation is
straight forward. Follow their instructions.

Assuming a conventional Go installation (i.e. you followed the
instructions), installing `dcc` is done the command,

    $ go get github.com/atrn/dcc

By default `go get` will install the binary under your _$GOPATH_,
which is `~/go/bin` if not defined otherwise I assume you know what
you're doing and where to find the executable.

## Building ICI

With `dcc` installed building ICI is done using `make,

    $ make

`make` is used to _direct_ the build process, `dcc` runs the
compilation, linking and library creation.

To configure the build process the `Makefile` can be edited to
set installation locations and other variables.

The makefile is quite simple (`dcc` does the real work).

## Building without dcc

Building a static version of ici is essentially trivial on current
UNIX-ish systems. The following command builds a static executable,

    $ c++ -std=c++14 -I. -UNDEBUG *.cc etc/main.cc -o ici

## Cmake

There is a very basic `CMakeLists.txt` file that works to compile a
basic interpreter executable. The CMakeFile doesn't all the things
the Makefile build does (DLL vs static lib, creating ici.h etc...)
but of course it could with sufficient _cmake-ing_.  However there
are **no** plans to move builds to use cmake and no plans to do
such cmake-ing. Contributions will be gladly accepted. I'm happy
enough with the make/dcc combination.
