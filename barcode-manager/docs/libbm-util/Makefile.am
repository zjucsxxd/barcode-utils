## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = 1.6

# The name of the module
DOC_MODULE=libbm-util

# The top-level SGML file.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.sgml

# The directory containing the source code. Relative to $(srcdir).
# gtk-doc will search all .c & .h files beneath here for inline comments
# documenting functions and macros.
DOC_SOURCE_DIR=../../libbm-util

# Extra options to supply to gtkdoc-scan.
SCAN_OPTIONS=--rebuild-types --rebuild-sections

# Extra options to supply to gtkdoc-scangobj.
SCANGOBJ_OPTIONS=--type-init-func 'g_type_init();'

# Extra options to supply to gtkdoc-mkdb.
MKDB_OPTIONS=--sgml-mode --output-format=xml

# Extra options to supply to gtkdoc-fixref.
FIXXREF_OPTIONS=

# Used for dependencies.
HFILE_GLOB=$(top_srcdir)/libbm-util/*.h
CFILE_GLOB=$(top_srcdir)/libbm-util/*.c

# Header files to ignore when scanning.
IGNORE_HFILES= \
	crypto.h \
	bm-setting-template.h \
	bm-utils-private.h \
	bm-param-spec-specialized.h

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
	$(GLIB_CFLAGS)			\
	$(DBUS_CFLAGS)

GTKDOC_LIBS = \
	$(top_builddir)/libbm-util/libbm-util.la \
	$(GLIB_LIBS)



# include common portion ...
include $(top_srcdir)/gtk-doc.make

CLEANFILES += \
	libbm-util-sections.txt \
	html/* \
	tmpl/* \
	xml/*

