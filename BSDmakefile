# This file is read by BSD make (aka pmake) and is used
# to redirect things to GNU make.
#
#

build?=exe
dest?=/usr/local
sudo?=

targets=all \
	ici \
	ici.h \
	lib \
	test \
	debug \
	clean \
	distclean \
	install \
	install-ici-dot-h \
	install-libici \
	install-ici-exe \
	full-install \
	modules \
	clean-modules \
	install-modules \
	with-cmake \
	configure-cmake \
	clean-cmake \


$(targets) !; @gmake --no-print-directory $@ build=$(build) dest=$(dest) sudo=$(sudo)
