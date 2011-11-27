SUBDIRS=. tests

noinst_LTLIBRARIES = libutils.la

libutils_la_SOURCES = \
	utils.c \
	utils.h \
	bma-bling-spinner.h \
	bma-bling-spinner.c

libutils_la_CPPFLAGS = \
	$(BMA_CFLAGS) \
	-DUIDIR=\""$(uidir)"\" \
	-DDATADIR=\""$(datadir)"\" \
	$(DISABLE_DEPRECATED) \
	-I${top_srcdir}/src/gconf-helpers \
	-I${top_srcdir}/src

libutils_la_LIBADD = $(BMA_LIBS)
