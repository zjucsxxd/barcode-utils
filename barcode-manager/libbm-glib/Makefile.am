INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/libbm-util \
	-I$(top_builddir)/marshallers

BUILT_SOURCES = \
	bm-client-bindings.h \
	bm-device-bindings.h \
	bm-device-bt-bindings.h \
	bm-exported-connection-glue.h \
	bm-exported-connection-bindings.h \
	bm-settings-glue.h \
	bm-settings-bindings.h \
	bm-settings-system-bindings.h \
	bm-active-connection-bindings.h

#####################################################
# Real libbm-glib stuff
#####################################################

lib_LTLIBRARIES = libbm-glib.la

libbm_glib_la_CFLAGS = \
	$(GLIB_CFLAGS) \
	$(DBUS_CFLAGS) \
	$(GUDEV_CFLAGS)

libbmincludedir = $(includedir)/libbm-glib

libbminclude_HEADERS = \
	libbm_glib.h \
	bm-object.h \
	bm-client.h \
	bm-device.h \
	bm-device-bt.h \
	bm-serial-device.h \
	bm-types.h \
	bm-active-connection.h \
	bm-remote-connection.h \
	bm-settings-interface.h \
	bm-settings-system-interface.h \
	bm-remote-settings.h \
	bm-remote-settings-system.h \
	bm-settings-connection-interface.h \
	bm-exported-connection.h \
	bm-settings-service.h

libbm_glib_la_SOURCES = \
	bm-object.c \
	bm-object-private.h \
	bm-client.c \
	bm-dbus-utils.c \
	bm-dbus-utils.h \
	bm-device.c \
	bm-device-private.h \
	bm-device-bt.c \
	bm-serial-device.c \
	bm-types.c \
	bm-types-private.h \
	bm-object-cache.c \
	bm-object-cache.h \
	bm-active-connection.c \
	bm-remote-connection.c \
	bm-remote-connection-private.h \
	bm-settings-interface.c \
	bm-settings-system-interface.c \
	bm-remote-settings.c \
	bm-remote-settings-system.c \
	bm-settings-connection-interface.c \
	bm-exported-connection.c \
	bm-settings-service.c

libbm_glib_la_LIBADD = \
	$(top_builddir)/libbm-util/libbm-util.la \
	$(top_builddir)/marshallers/libmarshallers.la \
	$(GLIB_LIBS) \
	$(DBUS_LIBS) \
	$(GUDEV_LIBS)

libbm_glib_la_LDFLAGS = -Wl,--version-script=$(srcdir)/libbm-glib.ver \
	-version-info "6:3:4"

noinst_PROGRAMS = libbm-glib-test

libbm_glib_test_SOURCES = libbm-glib-test.c
libbm_glib_test_CFLAGS = $(GLIB_CFLAGS) $(DBUS_CFLAGS)
libbm_glib_test_LDADD = libbm-glib.la $(top_builddir)/libbm-util/libbm-util.la $(GLIB_LIBS) $(DBUS_LIBS)


bm-client-bindings.h: $(top_srcdir)/introspection/bm-manager-client.xml
	$(AM_V_GEN) dbus-binding-tool --prefix=bm_client --mode=glib-client --output=$@ $<

bm-device-bindings.h: $(top_srcdir)/introspection/bm-device.xml
	$(AM_V_GEN) dbus-binding-tool --prefix=bm_device --mode=glib-client --output=$@ $<

bm-device-bt-bindings.h: $(top_srcdir)/introspection/bm-device-bt.xml
	$(AM_V_GEN) dbus-binding-tool --prefix=bm_device_bt --mode=glib-client --output=$@ $<

bm-settings-glue.h: $(top_srcdir)/introspection/bm-settings.xml
	$(AM_V_GEN) dbus-binding-tool --prefix=bm_settings --mode=glib-server --output=$@ $<

bm-settings-bindings.h: $(top_srcdir)/introspection/bm-settings.xml
	$(AM_V_GEN) dbus-binding-tool --prefix=bm_settings --mode=glib-client --output=$@ $<

bm-settings-system-bindings.h: $(top_srcdir)/introspection/bm-settings-system.xml
	$(AM_V_GEN) dbus-binding-tool --prefix=bm_settings_system --mode=glib-client --output=$@ $<

bm-exported-connection-glue.h: $(top_srcdir)/introspection/bm-exported-connection.xml
	$(AM_V_GEN) dbus-binding-tool --prefix=bm_exported_connection --mode=glib-server --output=$@ $<

bm-exported-connection-bindings.h: $(top_srcdir)/introspection/bm-exported-connection.xml
	$(AM_V_GEN) dbus-binding-tool --prefix=bm_exported_connection --mode=glib-client --output=$@ $<

bm-active-connection-bindings.h: $(top_srcdir)/introspection/bm-active-connection.xml
	$(AM_V_GEN) dbus-binding-tool --prefix=bm_active_connection --mode=glib-client --output=$@ $<

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libbm-glib.pc

DISTCLEANFILES = libbm-glib.pc libbm-glib.pc

EXTRA_DIST = libbm-glib.pc.in libbm-glib.ver

CLEANFILES = $(BUILT_SOURCES)
