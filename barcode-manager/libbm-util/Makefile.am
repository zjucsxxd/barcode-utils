SUBDIRS=.

INCLUDES = -I${top_srcdir} -I${top_srcdir}/include

lib_LTLIBRARIES=libbm-util.la

libbm_util_la_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	$(DBUS_CFLAGS) \
	$(UUID_CFLAGS) \
	-DG_DISABLE_DEPRECATED

libbm_util_include_HEADERS = 		\
	bm-connection.h			\
	bm-setting.h			\
	bm-setting-bluetooth.h		\
	bm-setting-connection.h		\
	bm-setting-serial.h		\
	bm-utils.h

libbm_util_la_SOURCES=			\
	bm-connection.c			\
	bm-param-spec-specialized.c	\
	bm-param-spec-specialized.h \
	bm-setting.c			\
	bm-setting-bluetooth.c		\
	bm-setting-connection.c		\
	bm-setting-serial.c		\
	bm-utils.c			\
	bm-utils-private.h \
	$(libbm_util_include_HEADERS)

libbm_util_la_LIBADD = $(GLIB_LIBS) $(DBUS_LIBS) $(UUID_LIBS)

libbm_util_la_LDFLAGS = -Wl,--version-script=$(srcdir)/libbm-util.ver \
	-version-info "7:1:6"

libbm_util_includedir=$(includedir)/BarcodeManager

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libbm-util.pc

DISTCLEANFILES = libbm-util.pc

EXTRA_DIST = libbm-util.pc.in libbm-util.ver

