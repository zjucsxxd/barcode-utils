noinst_LTLIBRARIES = libbm-logging.la

libbm_logging_la_SOURCES = \
	bm-logging.c \
	bm-logging.h

libbm_logging_la_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	-DLIBEXECDIR=\"$(libexecdir)\" \
	-DG_DISABLE_DEPRECATED

libbm_logging_la_LIBADD = \
	-ldl \
	$(GLIB_LIBS)

