SUBDIRS= \
	logging \
	system-settings \
	.

INCLUDES = -I${top_srcdir}                   \
           -I${top_srcdir}/include           \
           -I${top_builddir}/marshallers   \
           -I${top_srcdir}/src/logging \
           -I$(top_srcdir)/src/system-settings \
           -I${top_srcdir}/libbm-util \
           -I${top_srcdir}/libbm-glib \
           -I${top_srcdir}/callouts

###########################################
# BarcodeManager
###########################################

sbin_PROGRAMS = BarcodeManager

BarcodeManager_SOURCES = \
		bm-call-store.c \
		bm-call-store.h \
		bm-policy.c \
		bm-policy.h \
		bm-activation-request.h \
		bm-activation-request.c \
		bm-secrets-provider-interface.h \
		bm-secrets-provider-interface.c \
		bm-device.c \
		bm-device.h \
		bm-device-interface.c \
		bm-device-interface.h \
		bm-device-private.h \
		bm-device-bt.c \
		bm-device-bt.h \
		bm-dbus-manager.h \
		bm-dbus-manager.c \
		bm-udev-manager.c \
		bm-udev-manager.h \
		main.c \
		BarcodeManagerUtils.c \
		BarcodeManagerUtils.h \
		bm-system.c \
		bm-system.h \
		bm-manager.c \
		bm-manager.h \
		bm-properties-changed-signal.c \
		bm-properties-changed-signal.h

bm-manager-glue.h: $(top_srcdir)/introspection/bm-manager.xml
	$(AM_V_GEN) dbus-binding-tool --prefix=bm_manager --mode=glib-server --output=$@ $<

bm-device-interface-glue.h: $(top_srcdir)/introspection/bm-device.xml
	$(AM_V_GEN) dbus-binding-tool --prefix=bm_device_interface --mode=glib-server --output=$@ $<

bm-device-bt-glue.h: $(top_srcdir)/introspection/bm-device-bt.xml
	$(AM_V_GEN) dbus-binding-tool --prefix=bm_device_bt --mode=glib-server --output=$@ $<

BUILT_SOURCES = \
	bm-manager-glue.h \
	bm-device-interface-glue.h \
	bm-device-bt-glue.h

BarcodeManager_CPPFLAGS = \
	$(DBUS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GUDEV_CFLAGS) \
	$(LIBNL_CFLAGS) \
	$(GMODULE_CFLAGS) \
	$(POLKIT_CFLAGS) \
	-DG_DISABLE_DEPRECATED \
	-DBINDIR=\"$(bindir)\" \
	-DSBINDIR=\"$(sbindir)\" \
	-DLIBEXECDIR=\"$(libexecdir)\" \
	-DDATADIR=\"$(datadir)\" \
	-DSYSCONFDIR=\"$(sysconfdir)\" \
	-DLOCALSTATEDIR=\"$(localstatedir)\" \
	-DBM_RUN_DIR=\"$(rundir)\" \
	-DBMLOCALEDIR=\"$(datadir)/locale\" \
	-DARP_DEBUG

BarcodeManager_LDADD = \
	$(top_builddir)/marshallers/libmarshallers.la \
	./logging/libbm-logging.la \
	./system-settings/libsystem-settings.la \
	$(top_builddir)/libbm-util/libbm-util.la \
	$(DBUS_LIBS) \
	$(GLIB_LIBS) \
	$(GUDEV_LIBS) \
	$(LIBNL_LIBS) \
	$(GMODULE_LIBS) \
	$(POLKIT_LIBS) \
	$(LIBM) \
	$(LIBDL)

BarcodeManager_LDFLAGS = -rdynamic

libexec_PROGRAMS = bm-crash-logger
bm_crash_logger_SOURCES = bm-crash-logger.c
bm_crash_logger_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	-DG_DISABLE_DEPRECATED \
	-DBINDIR=\"$(bindir)\" \
	-DSBINDIR=\"$(sbindir)\" \
	-DDATADIR=\"$(datadir)\" \
	-DSYSCONFDIR=\"$(sysconfdir)\" \
	-DLOCALSTATEDIR=\"$(localstatedir)\"
bm_crash_logger_LDADD = $(GLIB_LIBS)

BarcodeManagerdir = $(datadir)/BarcodeManager
BarcodeManager_DATA = gdb-cmd

dbusservicedir = $(DBUS_SYS_DIR)
dbusservice_DATA = BarcodeManager.conf

EXTRA_DIST = \
	$(dbusservice_DATA) \
	$(BarcodeManager_DATA)

rundir=$(localstatedir)/run/BarcodeManager
statedir=$(localstatedir)/lib/BarcodeManager
install-data-hook:
	$(mkinstalldirs) -m 0700 $(DESTDIR)$(rundir)
	$(mkinstalldirs) -m 0700 $(DESTDIR)$(statedir)

CLEANFILES = $(BUILT_SOURCES)

