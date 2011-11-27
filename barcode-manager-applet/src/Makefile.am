SUBDIRS = marshallers utils gconf-helpers connection-editor gnome-bluetooth

bin_PROGRAMS = bm-applet

bm_applet_CPPFLAGS = \
	$(BMA_CFLAGS) \
	$(NOTIFY_CFLAGS) \
	$(APPINDICATOR_CFLAGS) \
	-DICONDIR=\""$(datadir)/icons"\"						\
	-DUIDIR=\""$(uidir)"\"							\
	-DBINDIR=\""$(bindir)"\"								\
	-DSYSCONFDIR=\""$(sysconfdir)"\"						\
	-DLIBEXECDIR=\""$(libexecdir)"\" \
	-DAUTOSTARTDIR=\""$(sysconfdir)/xdg/autostart"\"			\
	-DVPN_NAME_FILES_DIR=\""$(sysconfdir)/BarcodeManager/VPN"\"	\
	-DBMALOCALEDIR=\"$(datadir)/locale\"					\
	$(DBUS_CFLAGS)										\
	$(DISABLE_DEPRECATED)	\
	-I${top_builddir}/src/marshallers \
	-I${top_srcdir}/src/utils \
	-I${top_srcdir}/src/gconf-helpers  \
	-I${top_srcdir}/src/wireless-security

bm_applet_SOURCES =			\
	main.c				\
	applet.c				\
	applet.h				\
	applet-dbus-manager.c	\
	applet-dbus-manager.h	\
	applet-dialogs.h \
	applet-dialogs.c \
	applet-device-bt.h \
	applet-device-bt.c \
	fallback-icon.h

bm_applet_LDADD = \
	-lm \
	$(BMA_LIBS) \
	$(NOTIFY_LIBS) \
	$(APPINDICATOR_LIBS) \
	${top_builddir}/src/marshallers/libmarshallers.la \
	${top_builddir}/src/utils/libutils.la \
	${top_builddir}/src/gconf-helpers/libgconf-helpers.la

uidir = $(datadir)/bm-applet
ui_DATA = applet.ui

CLEANFILES = *.bak

EXTRA_DIST = 				\
	$(ui_DATA)

