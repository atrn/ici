.PHONY: all dll clean test-loading install

#  Our output filename is OS specific
_OS=$(shell uname)
ifeq ($(_OS),Darwin)
MODULE=ici-example.dylib
else
MODULE=ici-example.so
endif

all : dll

dll:; @dmake -o $(MODULE) dll

clean:; @dmake -o $(MODULE) dll clean

test-loading: dll
	@ici -e 'printf("len(\"$(_OS)\") -> %d (function)\n", example.function("$(_OS)"));'
	@ici -e 'printf("len(\"$(_OS)\") -> %d (other_function)\n", example.other_function("$(_OS)"));'

# Giving install a command, that does nothing, (: with no args)
# stops (GNU) make issuing a diagnostic that makes the build
# output look ugly.
install:
	@:
