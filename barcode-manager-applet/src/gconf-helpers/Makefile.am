noinst_LTLIBRARIES = libgconf-helpers.la

libgconf_helpers_la_SOURCES =	\
	gconf-helpers.h		\
	gconf-helpers.c		\
	gconf-upgrade.h		\
	gconf-upgrade.c		\
	bma-gconf-connection.h	\
	bma-gconf-connection.c	\
	bma-gconf-settings.h	\
	bma-gconf-settings.c

libgconf_helpers_la_CPPFLAGS = \
	$(BMA_CFLAGS) \
	$(APPINDICATOR_CFLAGS) \
	$(DISABLE_DEPRECATED) \
	-I${top_srcdir}/src \
	-I${top_builddir}/src/marshallers \
	-I${top_srcdir}/src/utils

libgconf_helpers_la_LIBADD = \
	$(BMA_LIBS) \
	$(APPINDICATOR_LIBS) \
	${top_builddir}/src/marshallers/libmarshallers.la \
	${top_builddir}/src/utils/libutils.la

