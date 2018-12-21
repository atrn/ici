.PHONY: all dll clean try-load
all dll:; @dmake -o ici-test.dylib dll
clean:; @dmake -o ici-test.dylib dll clean
try-load: dll; @ici -e 'load("test"); test.func1(argv[0]);'
