.PHONY: all dll clean test-loading install

#  Our output filename is OS specific
_OS=$(shell uname)
ifeq ($(_OS),Darwin)
MODULE=ici-vector.dylib
else
MODULE=ici-vector.so
endif

all : dll

dll:; @dmake -o $(MODULE) dll

clean:; @dmake -o $(MODULE) dll clean

# Giving install a command, that does nothing, (: with no args)
# stops (GNU) make issuing a diagnostic that makes the build
# output look ugly.
install:
	@:
