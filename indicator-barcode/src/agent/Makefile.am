libexec_PROGRAMS = indicator-barcode-agent

INCLUDES = \
	-I${top_srcdir}/src/common \
	-I${top_builddir}/src/common

indicator_barcode_agent_SOURCES = \
	main.c \
	agent.c \
	agent.h \
	passphrase-dialog.c \
	passphrase-dialog.h \
	connect-error-dialog.c \
	connect-error-dialog.h \
	wireless-connect-dialog.c \
	wireless-connect-dialog.h

indicator_barcode_agent_CFLAGS = \
	$(GTK_CFLAGS) \
	$(GIO_CFLAGS) \
	$(BARMAN_CFLAGS)

indicator_barcode_agent_LDADD = \
	$(GTK_LIBS) \
	$(GIO_LIBS) \
	$(top_builddir)/src/common/libmarshal.la \
	$(top_builddir)/src/common/liblog.la \
	$(BARMAN_LIBS)
