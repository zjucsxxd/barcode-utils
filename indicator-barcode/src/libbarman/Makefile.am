noinst_LTLIBRARIES = libbarman.la

libbarman_la_SOURCES = \
	barman.h \
	barman-manager.c \
	barman-manager.h \
	barman-service.c \
	barman-service.h \
	barman-ipv4.c \
	barman-ipv4.h \
	barman-ipv6.c \
	barman-ipv6.h

libbarman_la_LIBADD = \
	$(GLIB_LIBS) \
	$(GIO_LIBS)

libbarman_la_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	$(GIO_CFLAGS) \
	-I${top_builddir}/src/common \
	-DG_LOG_DOMAIN=\"libbarman\"
