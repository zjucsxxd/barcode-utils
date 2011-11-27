bin_PROGRAMS = bm-connection-editor

bm_connection_editor_CPPFLAGS = \
	$(BMA_CFLAGS) \
	-DICONDIR=\""$(datadir)/icons"\" \
	-DUIDIR=\""$(uidir)"\" \
	-DBINDIR=\""$(bindir)"\" \
	-DSYSCONFDIR=\""$(sysconfdir)"\" \
	-DLIBDIR=\""$(libdir)"\" \
	-DDATADIR=\""$(datadir)"\" \
	-DBMALOCALEDIR=\"$(datadir)/locale\" \
	$(DBUS_CFLAGS) \
	$(DISABLE_DEPRECATED) \
	-I${top_srcdir}/src/gconf-helpers \
	-I${top_srcdir}/src/utils \
	-I${top_srcdir}/src/wireless-security \
	-I${top_builddir}/src/marshallers

bm_connection_editor_SOURCES = \
	bm-connection-editor.c \
	bm-connection-editor.h \
	bm-connection-list.c \
	bm-connection-list.h \
	main.c \
	ce-page.h \
	ce-page.c \
	ce-polkit-button.c \
	ce-polkit-button.h

bm-connection-editor-service-glue.h: $(top_srcdir)/src/connection-editor/bm-connection-editor-service.xml
	$(AM_V_GEN) dbus-binding-tool --prefix=bm_connection_editor_service --mode=glib-server --output=$@ $<

bm_connection_editor_LDADD = \
	$(top_builddir)/src/gconf-helpers/libgconf-helpers.la \
	${top_builddir}/src/utils/libutils.la \
	${top_builddir}/src/marshallers/libmarshallers.la \
	$(BMA_LIBS) \
	-lm

uidir = $(datadir)/bm-applet
ui_DATA = \
	bm-connection-editor.ui

BUILT_SOURCES = bm-connection-editor-service-glue.h

CLEANFILES = *.bak $(BUILT_SOURCES)

EXTRA_DIST = $(ui_DATA) bm-connection-editor-service.xml

