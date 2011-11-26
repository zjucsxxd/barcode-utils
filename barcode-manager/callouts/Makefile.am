dbusservicedir = $(DBUS_SYS_DIR)
dbusservice_DATA = \
	bm-dispatcher.conf

libexec_PROGRAMS = \
	bm-dispatcher.action

bm_dispatcher_action_SOURCES = \
	bm-dispatcher-action.c \
	bm-dispatcher-action.h

bm_dispatcher_action_CPPFLAGS = \
	-I${top_srcdir} \
	-I${top_srcdir}/include \
	-I${top_srcdir}/libbm-util \
	$(DBUS_CFLAGS) \
	$(GLIB_CFLAGS) \
	-DG_DISABLE_DEPRECATED \
	-DSYSCONFDIR=\"$(sysconfdir)\" \
	-DLIBEXECDIR=\"$(libexecdir)\"

bm_dispatcher_action_LDADD = \
	$(top_builddir)/libbm-glib/libbm-glib.la \
	$(top_builddir)/libbm-util/libbm-util.la \
	$(DBUS_LIBS) \
	$(GLIB_LIBS)

bm-dispatcher-glue.h: bm-dispatcher.xml
	$(AM_V_GEN) dbus-binding-tool --prefix=bm_dispatcher --mode=glib-server --output=$@ $<

udevrulesdir = $(UDEV_BASE_DIR)/rules.d
udevrules_DATA = 40-barcode-scanners.rules

dbusactivationdir = $(datadir)/dbus-1/system-services
dbusactivation_in_files = org.freedesktop.bm_dispatcher.service.in
dbusactivation_DATA = $(dbusactivation_in_files:.service.in=.service)

%.service: %.service.in
	$(edit) $< >$@

edit = @sed \
	-e 's|@sbindir[@]|$(sbindir)|g' \
	-e 's|@sysconfdir[@]|$(sysconfdir)|g' \
	-e 's|@localstatedir[@]|$(localstatedir)|g' \
	-e 's|@libexecdir[@]|$(libexecdir)|g'

dispatcherdir=$(sysconfdir)/BarcodeManager/dispatcher.d
install-data-hook:
	   $(mkinstalldirs) -m 0755 $(DESTDIR)$(dispatcherdir)

CLEANFILES = $(BUILT_SOURCES) $(dbusactivation_DATA)

EXTRA_DIST = \
	$(dbusservice_DATA) \
	$(udevrules_DATA) \
	$(dbusactivation_in_files) \
	bm-dispatcher.xml

