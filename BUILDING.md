# How To Build

## Summary

Supported platforms:

- MacOS
- BSD's
- Most Linux-based OSes

Steps:

- install `dcc` (`go get github.com/atrn/dcc`, see below)
- type `make` (`gmake` on BSDs)

## Overview

There are several ways to build ici. The _official_ way (i.e. the
method I used to build and install on my machines) uses GNU make
to direct things and another tool, `dcc`, to look after building.

The _make+dcc_ method supports building ICI in a number of different
ways - standalone executable, static library + standalone executable
or dynamic library and associated executable (read the Makefile).

## cmake

A `CMakeLists.txt` is supplied with the sources which will build an
interpreter executable. Cmake support is minimal but can be used to
generate files for IDEs and other build tools (ninja works very well).

## Example makefiles

A number of more conventional makefiles are located in the `etc`
directory. These can be used as is or as a starting point.

## Building with make and dcc

The supported build system for ICI uses GNU `make` and my compiler
driver program, `dcc`. You likely already have GNU `make` or know
where to get it.

### Install `dcc`

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

### Build ICI

With `dcc` installed building ICI is done using `make,

    $ make

`make` is used to _direct_ the build process, `dcc` runs the
compilation, linking and library creation.

To configure the build process the `Makefile` can be edited to
set installation locations and other variables.

The makefile is quite simple (`dcc` does the real work).

## Building manually

Building a static version of ici is essentially trivial on current
UNIX-ish systems. The following command builds a static executable,

    $ c++ -std=c++14 -I. -UNDEBUG *.cc etc/main.cc -o ici

The `etc` directory contains a number of Makefiles that use this
approach.

### Platform configuration

More control over the configuration is available by setting the
`CONFIG_FILE` macro to the name of a header file that defines the
interpreter's configuration. The file named by `CONFIG_FILE` is
included by the standard header file `fwd.h`. If `CONFIG_FILE` is not
set `fwd.h` automatically selects a file for the host.

The per-system configuration header files are located in the `conf`
directory.

## CMakeLists.txt

The `CMakeLists.txt` is very basic but works to compile a standalone
interpreter executable. The CMakeFile doesn't do the same things the
Makefile does (DLL vs static lib, creating ici.h, installing) but of
course it could with sufficient _cmake-ing_.  However I have **no**
plans to that _cmake-ing_. I'm happy enough with the make and dcc.
Contributions will be gladly accepted however.

Using `cmake` an interpreter can be built via a the command,

    $ cmake -BBUILD -H. && make -CBUILD

Adjust build options can be done at generation time by setting cmake
variables on the command line or by editing the cmake's settings
_cache_ (either manually or by using the ccmake tool) and
re-generating build files.
