# This file is read by BSD make (aka pmake) and is used
# to redirect things to GNU make.
#
#

build?=exe
dest?=/usr/local
sudo?=

targets=all \
	test \
	ici.h \
	ici \
	lib \
	debug \
	lto \
	clean \
	install \
	install-ici-dot-h \
	install-libici \
	install-ici-exe \
	full-install

$(targets) !; @gmake --no-print-directory $@ build=$(build) dest=$(dest) sudo=$(sudo)
