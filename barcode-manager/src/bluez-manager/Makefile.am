INCLUDES = \
	-I${top_srcdir} \
	-I${top_srcdir}/include \
	-I${top_srcdir}/libbm-util \
	-I${top_srcdir}/src \
	-I${top_srcdir}/src/logging \
	-I${top_builddir}/marshallers \
	-I$(top_srcdir)/src/bm-bluez-manager

noinst_LTLIBRARIES = libbluez-manager.la

libbluez_manager_la_SOURCES = \
	bm-bluez-manager.c \
	bm-bluez-manager.h \
	bm-bluez-adapter.c \
	bm-bluez-adapter.h \
	bm-bluez-device.c \
	bm-bluez-device.h \
	bm-bluez-common.h

libbluez_manager_la_CPPFLAGS = \
	$(DBUS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(BLUEZ_CFLAGS) \
	-DG_DISABLE_DEPRECATED \
	-DBINDIR=\"$(bindir)\" \
	-DDATADIR=\"$(datadir)\" \
	-DSYSCONFDIR=\"$(sysconfdir)\" \
	-DLIBEXECDIR=\"$(libexecdir)\" \
	-DLOCALSTATEDIR=\"$(localstatedir)\"

libbluez_manager_la_LIBADD = \
	$(top_builddir)/marshallers/libmarshallers.la \
	$(top_builddir)/src/logging/libbm-logging.la \
	$(DBUS_LIBS) \
	$(GLIB_LIBS) \
	$(BLUEZ_LIBS)

