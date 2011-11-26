if WITH_DOCS

SUBDIRS = libbm-glib libbm-util

INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/libbm-util

noinst_PROGRAMS = \
	generate-settings-spec

generate_settings_spec_SOURCES = \
	generate-settings-spec.c

generate_settings_spec_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	$(DBUS_CFLAGS)

generate_settings_spec_LDADD = \
	$(top_builddir)/libbm-util/libbm-util.la \
	$(GLIB_LIBS) \
	$(DBUS_LIBS)

XSLTPROC = xsltproc --xinclude --nonet
XMLS = $(wildcard $(top_srcdir)/introspection/bm-*.xml)

OTHER_FILES= \
	$(top_srcdir)/introspection/all.xml \
	$(top_srcdir)/introspection/generic-types.xml \
	$(top_srcdir)/introspection/errors.xml \
	$(top_srcdir)/tools/doc-generator.xsl \
	$(top_srcdir)/introspection/generic-types.xml

GENERATED_FILES = spec.html settings-spec.html

spec.html: $(XMLS) $(OTHER_FILES)
	$(XSLTPROC) $(top_srcdir)/tools/doc-generator.xsl $(top_srcdir)/introspection/all.xml > $@

settings-spec.html: generate-settings-spec $(top_builddir)/libbm-util/libbm-util.la
	$(builddir)/generate-settings-spec $(builddir)/settings-spec.html

all: $(GENERATED_FILES)

EXTRA_DIST = $(GENERATED_FILES)

CLEANFILES = $(GENERATED_FILES)

endif

