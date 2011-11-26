INCLUDES = \
	-I${top_srcdir}/src/common \
	-I${top_builddir}/src/common

barcodemenulibdir = $(INDICATORDIR)
barcodemenulib_LTLIBRARIES = libbarcodemenu.la
libbarcodemenu_la_SOURCES = \
	service-menuitem.c \
	service-menuitem.h \
	tech-menuitem.c \
	tech-menuitem.h \
	indicator.c \
	$(toggleswitch_CSOURCES)

# -w is needed to disable gcc warnings because of valac, suckfest
libbarcodemenu_la_CFLAGS = \
	$(GTK_CFLAGS) \
	$(INDICATOR_CFLAGS) \
	$(DBUSMENUGTK_CFLAGS) \
	-w

libbarcodemenu_la_LIBADD = \
	$(GTK_LIBS) \
	$(INDICATOR_LIBS) \
	$(DBUSMENUGTK_LIBS) \
	$(top_builddir)/src/common/libmarshal.la

libbarcodemenu_la_LDFLAGS = -module -avoid-version

toggleswitch_VALASOURCES = \
	toggleswitch.vala

toggleswitch_CSOURCES = $(toggleswitch_VALASOURCES:.vala=.c)

toggleswitch_VALAFLAGS = \
	--ccode \
	--header=toggleswitch.h \
        --pkg=gtk+-2.0 \
        --pkg=gdk-2.0 \
        --pkg=gee-1.0 \
        --pkg=pango \
        --pkg=cairo \
	--fatal-warnings

toggleswitch_APIFILES = \
  toggleswitch.h 

toggleswitch.stamp $(toggleswitch_APIFILES): $(toggleswitch_VALASOURCES)
	$(AM_V_GEN) $(VALAC) $(toggleswitch_VALAFLAGS) $^
	touch toggleswitch.stamp

EXTRA_DIST = \
	$(toggleswitch_APIFILES) \
	$(toggleswitch_VALASOURCES)

BUILT_SOURCES = \
	toggleswitch.stamp \
	$(toggleswitch_APIFILES) \
	$(toggleswitch_CSOURCES)

CLEANFILES = \
	$(BUILT_SOURCES)
