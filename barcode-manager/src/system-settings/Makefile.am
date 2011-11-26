INCLUDES = -I${top_srcdir} \
           -I${top_srcdir}/include \
           -I${top_srcdir}/libbm-util \
           -I${top_srcdir}/libbm-glib \
           -I${top_srcdir}/src/logging \
           -I${top_srcdir}/src \
           -I${top_builddir}/marshallers

noinst_LTLIBRARIES = libsystem-settings.la

BUILT_SOURCES = \
	bm-settings-system-glue.h

libsystem_settings_la_SOURCES = \
	bm-sysconfig-settings.c \
	bm-sysconfig-settings.h \
	bm-inotify-helper.c \
	bm-inotify-helper.h \
	bm-polkit-helpers.h \
	bm-system-config-error.c \
	bm-system-config-error.h \
	bm-system-config-interface.c \
	bm-system-config-interface.h \
	bm-sysconfig-connection.c \
	bm-sysconfig-connection.h \
	bm-default-wired-connection.c \
	bm-default-wired-connection.h

libsystem_settings_la_CPPFLAGS = \
	$(DBUS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GMODULE_CFLAGS) \
	$(POLKIT_CFLAGS) \
	-DG_DISABLE_DEPRECATED	\
	-DBINDIR=\"$(bindir)\" \
	-DSBINDIR=\"$(sbindir)\" \
	-DLIBEXECDIR=\"$(libexecdir)\" \
	-DDATADIR=\"$(datadir)\" \
	-DSYSCONFDIR=\"$(sysconfdir)\" \
	-DLOCALSTATEDIR=\"$(localstatedir)\" \
	-DGNOMELOCALEDIR=\"$(datadir)/locale\" \
	-DPLUGINDIR=\"$(pkglibdir)\"

libsystem_settings_la_LIBADD = \
	$(top_builddir)/libbm-util/libbm-util.la \
	$(top_builddir)/libbm-glib/libbm-glib.la \
	$(top_builddir)/marshallers/libmarshallers.la \
	$(top_builddir)/src/logging/libbm-logging.la \
	$(DBUS_LIBS) \
	$(GLIB_LIBS) \
	$(GMODULE_LIBS) \
	$(POLKIT_LIBS)

libsystem_settings_la_LDFLAGS = -rdynamic

bm-settings-system-glue.h: $(top_srcdir)/introspection/bm-settings-system.xml
	$(AM_V_GEN) dbus-binding-tool --prefix=bm_settings_system --mode=glib-server --output=$@ $<

CLEANFILES = \
	$(BUILT_SOURCES)

