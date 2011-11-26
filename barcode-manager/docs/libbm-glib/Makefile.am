## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = 1.6

# The name of the module
DOC_MODULE=libbm-glib

# The top-level SGML file.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.sgml

# The directory containing the source code. Relative to $(srcdir).
# gtk-doc will search all .c & .h files beneath here for inline comments
# documenting functions and macros.
DOC_SOURCE_DIR=../../libbm-glib

# Extra options to supply to gtkdoc-scan.
SCAN_OPTIONS=

# Extra options to supply to gtkdoc-scangobj.
SCANGOBJ_OPTIONS=--type-init-func 'g_type_init();'

# Extra options to supply to gtkdoc-mkdb.
MKDB_OPTIONS=--sgml-mode --output-format=xml

# Extra options to supply to gtkdoc-fixref.
FIXXREF_OPTIONS=

# Used for dependencies.
HFILE_GLOB=$(top_srcdir)/libbm-glib/*.h
CFILE_GLOB=$(top_srcdir)/libbm-glib/*.c

# Header files to ignore when scanning.
IGNORE_HFILES= \
	bm-dbus-utils.h \
	bm-device-private.h \
	bm-object-cache.h \
	bm-object-private.h \
	bm-remote-connection-private.h \
	bm-types-private.h \
	bm-active-connection-bindings.h \
	bm-client-bindings.h \
	bm-device-bindings.h \
	bm-device-bt-bindings.h \
	bm-exported-connection-bindings.h \
	bm-exported-connection-glue.h \
	bm-settings-bindings.h \
	bm-settings-glue.h \
	bm-settings-system-bindings.h

# Images to copy into HTML directory.
HTML_IMAGES = 

# Extra XML files that are included by $(DOC_MAIN_SGML_FILE).
content_files = 

# Other files to distribute.
extra_files =


# CFLAGS and LDFLAGS for compiling scan program. Only needed
# if $(DOC_MODULE).types is non-empty.
GTKDOC_CFLAGS =				\
	-I$(top_srcdir)/include		\
	-I$(top_srcdir)/libbm-util	\
	-I$(top_builddir)/marshallers	\
	-I$(top_srcdir)/libbm-glib	\
	$(GLIB_CFLAGS)			\
	$(DBUS_CFLAGS)

GTKDOC_LIBS =  \
	$(top_builddir)/libbm-glib/libbm-glib.la \
	$(GLIB_LIBS)


# include common portion ...
include $(top_srcdir)/gtk-doc.make

CLEANFILES += \
	libbm-glib-sections.txt \
	html/* \
	tmpl/* \
	xml/*

