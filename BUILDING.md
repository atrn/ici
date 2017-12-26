# How To Build

Building uses GNU make and my dcc tool, installable via
"go get github.com/atrn/dcc" (it is written in Go). dcc
is a compiler driver, like cc(1), but automatically uses
dependencies, of all forms, to **avoid** building where
possible.

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
