.PHONY: all dll clean
all dll:; @make -s -C mit BigNum.a && dmake -o ici-test.dylib dll
clean:; @dmake -o ici-test.dylib dll clean; make -s -C mit scratch
