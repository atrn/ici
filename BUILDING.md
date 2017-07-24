# How To Build

Building uses my dcc build tool.

Dcc is available at gitlab.com/atrn/dcc. Dcc is written in Go
and requires a Go installation to build:

0.  If required, download and install Go from http://golang.org/.

    Go usually lives under /usr/local/go and you want to add the
    directory /usr/local/go/bin to your $PATH. If you install Go using
    a system package manager such as apt and yum it may reside
    elsewhere and have different locations.

    You should be able to run "go version" in a shell.

    Establish a "Go Workspace", i.e. create the directory $HOME/go and
    add $HOME/go/bin to your $PATH.

1.  Run "go install gitlab.com/atrn/dcc"

2.  The dcc executable is $HOME/go/bin/dcc

## Summary

Supported platforms:

- MacOS
- BSD's
- Most Linux-based OSes
- Windows (with MS Visual C++)

## Building

You can build using a simple "go build" command or use the
included Makefile 

$ make

## Without dcc

Building a static version of ici is essentially trivial on current
POSIX-ish systems. The following command builds a static ici
executable,

    c++ -std=c++14 -I. -UNDEBUG *.cc etc/main.cc -o ici
