# .dcc directory

This directory stores files used by the `dcc` compiler driver.

The CXXFLAGS files define compiler options and dcc selects the
file according to the host OS, e.g. on FreeBSD the file
CXXFLAGS.freebsd is used.

LDFLAGS files define options to the linker, ld(1).

LIBS files define the libraries and library search paths.
