noinst_LTLIBRARIES = libmarshallers.la

BUILT_SOURCES = \
	bma-marshal.h \
	bma-marshal.c

libmarshallers_la_SOURCES = \
	bma-marshal-main.c

libmarshallers_la_CPPFLAGS = \
	$(GOBJECT_CFLAGS) \
	-DG_DISABLE_DEPRECATED


libmarshallers_la_LIBADD = $(GOBJECT_LIBS)

EXTRA_DIST = bma-marshal.list
CLEANFILES = $(BUILT_SOURCES)

bma-marshal.h: bma-marshal.list
	$(GLIB_GEBMARSHAL) $< --prefix=bma_marshal --header > $@

bma-marshal.c: bma-marshal.list
	$(GLIB_GEBMARSHAL) $< --prefix=bma_marshal --body > $@

bma-marshal-main.c: bma-marshal.c bma-marshal.h

