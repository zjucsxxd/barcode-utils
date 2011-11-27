INCLUDES = -I${top_srcdir}                    \
           -I${top_srcdir}/libbm-util         \
           -I${top_srcdir}/libbm-glib         \
           -I${top_srcdir}/include

AM_CPPFLAGS = \
	$(DBUS_CFLAGS) \
	$(GLIB_CFLAGS) \
	-DBINDIR=\"$(bindir)\" \
	-DNM_RUN_DIR=\"$(rundir)\" \
	-DDATADIR=\"$(datadir)\"

bin_PROGRAMS = bm-tool bm-online

#noinst_PROGRAMS = libnm-glib-test

bm_tool_SOURCES = bm-tool.c
bm_tool_LDADD = \
	$(top_builddir)/libbm-glib/libbm-glib.la \
	$(top_builddir)/libbm-util/libbm-util.la \
	$(DBUS_LIBS) \
	$(GLIB_LIBS)

bm_online_SOURCES = bm-online.c
bm_online_LDADD = \
	$(top_builddir)/libbm-util/libbm-util.la \
	$(DBUS_LIBS) \
	$(GLIB_LIBS)

libbm_glib_test_SOURCES = libbm-glib-test.c
libbm_glib_test_CFLAGS = \
	-Wno-deprecated-declarations \
	-Wno-deprecated
libbm_glib_test_LDADD = \
	$(top_builddir)/libbm-glib/libbm-glib.la \
	$(top_builddir)/libbm-util/libbm-util.la \
	$(DBUS_LIBS) \
	$(GLIB_LIBS)

