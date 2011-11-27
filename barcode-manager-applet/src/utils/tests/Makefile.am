INCLUDES = -I$(top_srcdir)/src/utils

noinst_PROGRAMS = test-utils

test_utils_SOURCES = test-utils.c

test_utils_CPPFLAGS = $(BMA_CFLAGS)

test_utils_LDADD = \
	${top_builddir}/src/utils/libutils.la \
	$(BMA_LIBS)

check-local: test-utils
	$(abs_builddir)/test-utils

