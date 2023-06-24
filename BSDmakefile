# This file is read by BSD make in preference to the Makefile and is used on
# systems who's make(1) is not GNU make.
#
# The default values for build and prefix should be kept in sync
# with those defined in Makefile.
#

gmake?=gmake
build?=dll
prefix?=/usr/local
sudo?=
silent?=@

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
	distclean-modules \
	install-modules \
	modules-clean \
	modules-distclean \
	modules-install \
	with-cmake \
	configure-cmake \
	clean-cmake \


$(targets) !; $(silent) $(gmake) --no-print-directory $@ build=$(build) prefix=$(prefix) sudo=$(sudo) silent=$(silent)
