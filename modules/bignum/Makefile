ICI_MODULES_DIR=..
ICI_MODULE_NAME=bignum
include $(ICI_MODULES_DIR)/Makefile.module

plugin: bignum-lib
clean: clean-bignum-lib

.PHONY: bignum-lib clean-bignum-lib

bignum-lib:; @$(MAKE) -s -C mit BigNum.a
clean-bignum-lib:; @$(MAKE) -s -C mit clean
