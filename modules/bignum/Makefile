.PHONY: all dll clean
all dll:; @make -C mit all && dmake -o ici-test.dylib dll
clean:; @dmake -o ici-test.dylib dll clean; make -s -C mit scratch
