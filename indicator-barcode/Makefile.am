SUBDIRS = \
	src \
	data \
	po \
	scripts \
	tests

EXTRA_DIST = autogen.sh

DISTCHECK_CONFIGURE_FLAGS = --enable-localinstall

allchecks: all check distcheck
	@cat $(srcdir)/data/ok-banner
