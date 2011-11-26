SUBDIRS = \
	marshallers \
	libbm-util \
	libbm-glib \
	src \
	include \
	introspection \
	callouts \
	system-settings \
	cli \
	tools \
	policy \
	data \
	initscript \
	test \
	po \
	man \
	docs \
	examples

EXTRA_DIST = \
	BarcodeManager.pc.in \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in		

DISTCHECK_CONFIGURE_FLAGS = \
	--with-tests=yes \
	--with-docs=yes \
	--with-udev-dir=$$dc_install_base/lib/udev \
	--with-systemdsystemunitdir=$$dc_install_base/$(systemdsystemunitdir)

DISTCLEANFILES = intltool-extract intltool-merge intltool-update

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = BarcodeManager.pc

ACLOCAL_AMFLAGS = -I m4
