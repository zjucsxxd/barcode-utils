# Separate directory and makefile for libbarman-vala because otherwise
# automake would compile C tests with a vala compiler and would fail.
# No idea why that happens.

noinst_PROGRAMS = \
	libbarman-vala

libbarman_vala_SOURCES = \
	libbarman-vala.vala

libbarman_vala_CFLAGS = \
	$(GLIB_CFLAGS) \
	$(BARMAN_CFLAGS) \
	$(READLINE_CFLAGS)

libbarman_vala_LDADD = \
	$(GLIB_LIBS) \
	$(BARMAN_LIBS) \
	$(READLINE_LIBS)

VALAFLAGS = \
	--vapidir=$(srcdir)/../../src/libbarman \
        --pkg=gio-2.0 \
        --pkg=gee-1.0 \
        --pkg=Barman-1.0 \
	--pkg=readline \
	--fatal-warnings
