# How To Build

Building uses my build tool, dcc, available at gitlab.com/atrn/dcc
(written in Go, install Go and then 'go install gitlab.com/atrn/dcc').

## Summary

Supported platforms:

- MacOS
- BSD's
- Most Linux-based OSes
- Windows (with MS Visual C++)


## Building

Tyoe,

        make

## Without dcc

Building a static version of anici is trivial. On current POSIX
systems the following command builds a static anici executable
that includes the interpreter.

    c++ -std=c++14 -I. -UNDEBUG *.cc etc/main.cc -o anici
