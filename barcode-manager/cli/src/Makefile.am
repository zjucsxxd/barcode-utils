bin_PROGRAMS = \
	bmcli

INCLUDES = \
	-I${top_srcdir} \
	-I${top_srcdir}/include \
	-I${top_builddir}/marshallers \
	-I${top_srcdir}/libbm-util \
	-I${top_srcdir}/libbm-glib

bmcli_SOURCES = \
	connections.c \
	connections.h \
	devices.c \
	devices.h \
	barcode-manager.c \
	barcode-manager.h \
	settings.c \
	settings.h \
	bmcli.c \
	bmcli.h \
	utils.c \
	utils.h

bmcli_CPPFLAGS = \
	$(DBUS_CFLAGS) \
	$(GLIB_CFLAGS) \
	-DBMCLI_LOCALEDIR=\"$(datadir)/locale\" \
	-DG_DISABLE_DEPRECATED

bmcli_LDADD = \
	$(DBUS_LIBS) \
	$(GLIB_LIBS) \
	$(top_builddir)/marshallers/libmarshallers.la \
	$(top_builddir)/libbm-util/libbm-util.la \
	$(top_builddir)/libbm-glib/libbm-glib.la


