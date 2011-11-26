AC_INIT([indicator-barcode], [0.3.8])
AC_CONFIG_SRCDIR(src/indicator/indicator.c)
AM_INIT_AUTOMAKE([tar-ustar])

AC_PREREQ(2.53)

AM_CONFIG_HEADER(config.h)

AM_MAINTAINER_MODE

IT_PROG_INTLTOOL([0.35.0])

COMPILER_FLAGS

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_C_O
AC_STDC_HEADERS
AC_PROG_LIBTOOL
AC_PROG_LN_S
AC_PROG_MKDIR_P
AM_PROG_VALAC

AC_SUBST(VERSION)
AC_CONFIG_MACRO_DIR([m4])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

###########################
# Dependencies 
###########################

GTK_REQUIRED_VERSION=2.12
INDICATOR_REQUIRED_VERSION=0.3.0
DBUSMENUGTK_REQUIRED_VERSION=0.3.90
LIBNOTIFY_REQUIRED_VERSION=0.7.1
DBUSMENUGLIB_REQUIRED_VERSION=0.3.90
GIO_REQUIRED_VERSION=2.26

PKG_CHECK_MODULES([GTK], [gtk+-2.0 >= $GTK_REQUIRED_VERSION])
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

PKG_CHECK_MODULES([GLIB], [glib-2.0])
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

PKG_CHECK_MODULES([GIO], [gio-2.0 >= $GIO_REQUIRED_VERSION])
AC_SUBST(GIO_CFLAGS)
AC_SUBST(GIO_LIBS)

PKG_CHECK_MODULES([GOBJECT], [gobject-2.0])
AC_SUBST(GOBJECT_CFLAGS)
AC_SUBST(GOBJECT_LIBS)

PKG_CHECK_MODULES([INDICATOR], [indicator >= $INDICATOR_REQUIRED_VERSION])
AC_SUBST(INDICATOR_CFLAGS)
AC_SUBST(INDICATOR_LIBS)

PKG_CHECK_MODULES([DBUSMENUGTK],
		[dbusmenu-gtk-0.4 >= $DBUSMENUGTK_REQUIRED_VERSION])
AC_SUBST(DBUSMENUGTK_CFLAGS)
AC_SUBST(DBUSMENUGTK_LIBS)

PKG_CHECK_MODULES([DBUSMENUGLIB], 
		[dbusmenu-glib-0.4 >= $DBUSMENUGLIB_REQUIRED_VERSION])
AC_SUBST(DBUSMENUGLIB_CFLAGS)
AC_SUBST(DBUSMENUGLIB_LIBS)

PKG_CHECK_MODULES([LIBNOTIFY], [libnotify >= $LIBNOTIFY_REQUIRED_VERSION])
AC_SUBST(LIBNOTIFY_CFLAGS)
AC_SUBST(LIBNOTIFY_LIBS)

PKG_CHECK_MODULES([LIBGEE], [gee-1.0])
AC_SUBST(LIBGEE_CFLAGS)
AC_SUBST(LIBGEE_LIBS)

###########################
# Check to see if we're local
###########################

with_localinstall="no"
AC_ARG_ENABLE(localinstall, AS_HELP_STRING([--enable-localinstall], [install all of the files localy instead of system directories (for distcheck)]), with_localinstall=$enableval, with_localinstall=no)

###########################
# Indicator Info
###########################

if test "x$with_localinstall" = "xyes"; then
	INDICATORDIR="${libdir}/indicators/2/"
	INDICATORICONSDIR="${datadir}/indicator-applet/icons/"
else
	INDICATORDIR=`$PKG_CONFIG --variable=indicatordir indicator`
	INDICATORICONSDIR=`$PKG_CONFIG --variable=iconsdir indicator`
fi
AC_SUBST(INDICATORDIR)
AC_SUBST(INDICATORICONSDIR)

###########################
# DBus Service Info
###########################

if test "x$with_localinstall" = "xyes"; then
	DBUSSERVICEDIR="${datadir}/dbus-1/services/"
else
	DBUSSERVICEDIR=`$PKG_CONFIG --variable=session_bus_services_dir dbus-1`
fi
AC_SUBST(DBUSSERVICEDIR)

##############################
# Custom Junk
##############################

AC_DEFUN([AC_DEFINE_PATH], [
	test "x$prefix" = xNONE && prefix="$ac_default_prefix"
	test "x$exec_prefix" = xNONE && exec_prefix='${prefix}'
	ac_define_path=`eval echo [$]$2`
	ac_define_path=`eval echo [$]ac_define_path`
	$1="$ac_define_path"
	AC_SUBST($1)
	ifelse($3, ,
		AC_DEFINE_UNQUOTED($1, "$ac_define_path"),
		AC_DEFINE_UNQUOTED($1, "$ac_define_path", $3))
])

AC_CHECK_LIB([readline], [readline], [
	READLINE_LIBS=-lreadline
	AC_DEFINE([HAVE_LIBREADLINE], [1], 
		[Define to 1 if you have the `readline' library (-lreadline).])
	],
	[AC_MSG_ERROR([cannot find readline devel library])]
)
AC_SUBST(READLINE_LIBS)

BARMAN_CFLAGS="-I\$(top_srcdir)/src/libbarman"
BARMAN_LIBS="\$(top_builddir)/src/libbarman/libbarman.la"

AC_SUBST(BARMAN_CFLAGS)
AC_SUBST(BARMAN_LIBS)

###########################
# Internationalization
###########################

GETTEXT_PACKAGE=indicator-barcode
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Name of the default get text domain])
AC_DEFINE_PATH(GNOMELOCALEDIR, "${datadir}/locale", [locale directory])

AM_GLIB_GNU_GETTEXT

###########################
# Files
###########################

AC_OUTPUT([
Makefile
src/Makefile
src/indicator/Makefile
src/common/Makefile
src/libbarman/Makefile
src/backend/Makefile
src/agent/Makefile
src/settings/Makefile
src/settings/icons/Makefile
src/settings/icons/hicolor/Makefile
src/settings/icons/hicolor/22x22/Makefile
src/settings/icons/hicolor/22x22/status/Makefile
src/settings/ui/Makefile
data/Makefile
po/Makefile.in
scripts/Makefile
tests/Makefile
tests/barman-mock/Makefile
tests/libbarman-vala/Makefile
])

###########################
# Results
###########################

AC_MSG_NOTICE([

SUS Indicator Configuration:

	Prefix:        $prefix
])
