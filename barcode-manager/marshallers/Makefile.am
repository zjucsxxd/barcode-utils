noinst_LTLIBRARIES = libmarshallers.la

BUILT_SOURCES = \
	bm-marshal.h \
	bm-marshal.c

libmarshallers_la_SOURCES = \
	bm-marshal-main.c

libmarshallers_la_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	-DG_DISABLE_DEPRECATED

libmarshallers_la_LIBADD = $(GLIB_LIBS)

EXTRA_DIST = bm-marshal.list
CLEANFILES = $(BUILT_SOURCES)

bm-marshal.h: bm-marshal.list
	$(GLIB_GENMARSHAL) $< --prefix=_nm_marshal --header > $@

bm-marshal.c: bm-marshal.list
	$(GLIB_GENMARSHAL) $< --prefix=_nm_marshal --body > $@

bm-marshal-main.c: bm-marshal.c bm-marshal.h

