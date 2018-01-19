# How To Build

Building uses GNU `make` and my program `dcc`. GNU `make` is used to
_direct_ overall build processing but compilation, linking and library
creation is handled by `dcc`.  `dcc` is a compiler driver, like
`cc(1)`, but one that automatically creates and uses dependencies to
**avoid** doing work if at all possible. `dcc` has other features that
let it take care of all compilation, linking and library creation, and
`dcc` do so via an efficient parallel compilation process.  `dcc` is
written in Go and installation Go be installed. Assuming conventional
Go usage, installing `dcc` is a simple matter of running the command,

    $ go get github.com/atrn/dcc


## Summary

Supported platforms:

- MacOS
- BSD's
- Most Linux-based OSes
- Windows (with MS Visual C++)

## Building

    $ make conf=conf/`uname`.h



## Without dcc

Building a static version of ici is essentially trivial on current
POSIX-ish systems. The following command builds a static ici
executable,

    c++ -std=c++14 -I. -UNDEBUG *.cc etc/main.cc -o ici
