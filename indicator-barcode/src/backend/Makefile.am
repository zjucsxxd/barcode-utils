libexec_PROGRAMS = indicator-barcode-service

INCLUDES = \
	-I${top_srcdir}/src/common \
	-I${top_builddir}/src/common

indicator_barcode_service_SOURCES = \
    manager.h \
    manager.c \
    main.c \
    manager-proxy.h \
    manager-proxy.c \
    service.c \
    service.h \
    service-manager.c \
    service-manager.h \
    barcode-menu.c \
    barcode-menu.h \
    ui-proxy.c \
    ui-proxy.h

indicator_barcode_service_CFLAGS = \
	$(GLIB_CFLAGS) \
	$(DBUSMENUGLIB_CFLAGS) \
	$(INDICATOR_CFLAGS) \
	$(LIBNOTIFY_CFLAGS) \
	$(BARMAN_CFLAGS) \
	-DLIBEXECDIR=\"$(libexecdir)\" \
	-DBINDIR=\"$(bindir)\"

indicator_barcode_service_LDADD = \
	$(GLIB_LIBS) \
	$(DBUSMENUGLIB_LIBS) \
	$(INDICATOR_LIBS) \
	$(LIBNOTIFY_LIBS) \
	$(BARMAN_LIBS) \
	$(top_builddir)/src/common/libmarshal.la \
	$(top_builddir)/src/common/liblog.la
