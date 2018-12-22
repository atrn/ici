.PHONY: all dll clean test-load

OS=$(shell uname)

ifeq ($(OS),Darwin)
MODULE=ici-example.dylib
endif
ifeq ($(OS),FreeBSD)
MODULE=ici-example.so
endif
ifeq ($(OS),Linux)
MODULE=ici-example.so
endif

all dll:; @dmake -o $(MODULE) dll
clean:; @dmake -o $(MODULE) dll clean

test-load: dll
	@ici -e 'load("example"); println(example.function(argv[0]));'
