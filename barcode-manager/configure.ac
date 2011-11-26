AC_PREREQ(2.52)

dnl The NM version number
m4_define([bm_major_version], [0])
m4_define([bm_minor_version], [0])
m4_define([bm_micro_version], [1])
m4_define([bm_nano_version],  [1])
m4_define([bm_version],
          [bm_major_version.bm_minor_version.bm_micro_version.bm_nano_version])

AC_INIT([BarcodeManager], [bm_version],
        [https://github.com/koppi/barcode-utils],
        [BarcodeManager])
AM_INIT_AUTOMAKE([1.9 subdir-objects tar-ustar no-dist-gzip dist-bzip2])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([no])])
AM_MAINTAINER_MODE

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_HEADERS([config.h])

dnl Define _GNU_SOURCE for various things like strcasestr()
AC_GNU_SOURCE

dnl
dnl Require programs
dnl
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LIBTOOL

dnl ensure that when the Automake generated makefile calls aclocal,
dnl it honours the $ACLOCAL_FLAGS environment variable
ACLOCAL_AMFLAGS="\${ACLOCAL_FLAGS}"
if test -n "$ac_macro_dir"; then
    ACLOCAL_AMFLAGS="-I $ac_macro_dir $ACLOCAL_AMFLAGS"
fi
AC_SUBST([ACLOCAL_AMFLAGS])

dnl maintainer mode stuff
if test $USE_MAINTAINER_MODE = yes; then
	DISABLE_DEPRECATED="-DG_DISABLE_DEPRECATED"
else
	DISABLE_DEPRECATED=""
fi
AC_SUBST(DISABLE_DEPRECATED)

dnl Version stuff
BM_MAJOR_VERSION=bm_major_version
BM_MINOR_VERSION=bm_minor_version
BM_MICRO_VERSION=bm_micro_version
BM_VERSION=bm_version
AC_SUBST(BM_MAJOR_VERSION)
AC_SUBST(BM_MINOR_VERSION)
AC_SUBST(BM_MICRO_VERSION)
AC_SUBST(BM_VERSION)

dnl
dnl Required headers
dnl
AC_HEADER_STDC
AC_CHECK_HEADERS(fcntl.h paths.h sys/ioctl.h sys/time.h syslog.h unistd.h)

dnl
dnl Checks for typedefs, structures, and compiler characteristics.
dnl
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_HEADER_TIME

dnl
dnl Checks for library functions.
dnl
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MEMCMP
AC_CHECK_FUNCS(select socket uname)

dnl
dnl translation support
dnl
GETTEXT_PACKAGE=BarcodeManager
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package])
IT_PROG_INTLTOOL([0.35.0])
AM_GLIB_GNU_GETTEXT

dnl
dnl Documentation
dnl
AC_ARG_WITH(docs, AS_HELP_STRING([--with-docs], [Build BarcodeManager documentation]))
AM_CONDITIONAL(WITH_DOCS, test "x$with_docs" = "xyes")
case $with_docs in
    yes)
        enable_gtk_doc=yes
        ;;
    *)
        with_docs=no
        ;;
esac

dnl
dnl Make sha1.c happy on big endian systems
dnl
AC_C_BIGENDIAN

AC_ARG_WITH(distro, AS_HELP_STRING([--with-distro=DISTRO], [Specify the Linux distribution to target: One of redhat, suse, gentoo, debian, arch, slackware, paldo, mandriva, pardus, linexa or exherbo]))
if test "z$with_distro" = "z"; then
	AC_CHECK_FILE(/etc/redhat-release,with_distro="redhat")
	AC_CHECK_FILE(/etc/SuSE-release,with_distro="suse")
	AC_CHECK_FILE(/etc/fedora-release,with_distro="redhat")
	AC_CHECK_FILE(/etc/gentoo-release,with_distro="gentoo")
	AC_CHECK_FILE(/etc/debian_version,with_distro="debian")
	AC_CHECK_FILE(/etc/arch-release,with_distro="arch")
	AC_CHECK_FILE(/etc/slackware-version,with_distro="slackware")
	AC_CHECK_FILE(/etc/frugalware-release,with_distro="frugalware")
	AC_CHECK_FILE(/etc/mandriva-release,with_distro="mandriva")
	AC_CHECK_FILE(/etc/pardus-release,with_distro="pardus")
	AC_CHECK_FILE(/etc/linexa-release,with_distro="linexa")
	AC_CHECK_FILE(/etc/exherbo-release,with_distro="exherbo")
	if test "z$with_distro" = "z"; then
		with_distro=`lsb_release -is`
	fi
fi
with_distro=`echo ${with_distro} | tr '[[:upper:]]' '[[:lower:]]' `

if test "z$with_distro" = "z"; then
	echo "Linux distribution autodetection failed, you must specify the distribution to target using --with-distro=DISTRO"
	exit 1
else
	case $with_distro in
		redhat|suse|gentoo|debian|slackware|arch|paldo|frugalware|mandriva|pardus|linexa|exherbo) ;;
		*)
			echo "Your distribution (${with_distro}) is not yet supported!  (patches welcome)"
			exit 1
			;;
	esac
fi

AM_CONDITIONAL(TARGET_REDHAT, test x"$with_distro" = xredhat)
if test x"$with_distro" = xredhat; then
  AC_DEFINE(TARGET_REDHAT, 1, [Define if you have Fedora or RHEL])
fi

AM_CONDITIONAL(TARGET_SUSE, test x"$with_distro" = xsuse)
if test x"$with_distro" = xsuse; then
  AC_DEFINE(TARGET_SUSE, 1, [Define if you have OpenSUSE or SLES])
fi

AM_CONDITIONAL(TARGET_GENTOO, test x"$with_distro" = xgentoo)
if test x"$with_distro" = xgentoo; then
  AC_DEFINE(TARGET_GENTOO, 1, [Define if you have Gentoo])
fi

AM_CONDITIONAL(TARGET_DEBIAN, test x"$with_distro" = xdebian)
if test x"$with_distro" = xdebian; then
  AC_DEFINE(TARGET_DEBIAN, 1, [Define if you have Debian])
fi

AM_CONDITIONAL(TARGET_SLACKWARE, test x"$with_distro" = xslackware)
if test x"$with_distro" = xslackware; then
  AC_DEFINE(TARGET_SLACKWARE, 1, [Define if you have Slackware])
fi

AM_CONDITIONAL(TARGET_ARCH, test x"$with_distro" = xarch)
if test x"$with_distro" = xarch; then
  AC_DEFINE(TARGET_ARCH, 1, [Define if you have Arch])
fi

AM_CONDITIONAL(TARGET_PALDO, test x"$with_distro" = xpaldo)
if test x"$with_distro" = xpaldo; then
  AC_DEFINE(TARGET_PALDO, 1, [Define if you have Paldo])
fi

AM_CONDITIONAL(TARGET_FRUGALWARE, test x"$with_distro" = xfrugalware)
if test x"$with_distro" = xfrugalware; then
  AC_DEFINE(TARGET_FRUGALWARE, 1, [Define if you have Frugalware])
fi

AM_CONDITIONAL(TARGET_MANDRIVA, test x"$with_distro" = xmandriva)
if test x"$with_distro" = xmandriva; then
  AC_DEFINE(TARGET_MANDRIVA, 1, [Define if you have Mandriva])
fi

AM_CONDITIONAL(TARGET_PARDUS, test x"$with_distro" = xpardus)
if test x"$with_distro" = xpardus; then
  AC_DEFINE(TARGET_PARDUS, 1, [Define if you have Pardus])
fi

AM_CONDITIONAL(TARGET_LINEXA, test x"$with_distro" = xlinexa)
if test x"$with_distro" = xlinexa; then
  AC_DEFINE(TARGET_LINEXA, 1, [Define if you have linexa])
fi

AM_CONDITIONAL(TARGET_EXHERBO, test x"$with_distro" = xexherbo)
if test x"$with_distro" = xexherbo; then
  AC_DEFINE(TARGET_EXHERBO, 1, [Define if you have Exherbo])
fi

dnl
dnl Distribution version string
dnl
AC_ARG_WITH(dist-version, AS_HELP_STRING([--with-dist-version=<NM-distribution-version>], [Define the NM's distribution version string]), ac_distver=$withval, ac_distver="")
if ! test x"$ac_distver" = x""; then
  AC_DEFINE_UNQUOTED(NM_DIST_VERSION, "$ac_distver", [Define the distribution version string])
fi

dnl
dnl Checks for libm - needed for pow()
dnl
LT_LIB_M

dnl
dnl Checks for libdl - on certain platforms its part of libc
dnl
AC_CHECK_LIB([dl], [dladdr], LIBDL="-ldl", LIBDL="")
AC_SUBST(LIBDL)

dnl
dnl Checks for new dbus-glib property access function
dnl
AC_CHECK_LIB([dbus-glib-1], [dbus_glib_global_set_disable_legacy_property_access], ac_have_dg_prop="1", ac_have_dg_prop="0")
AC_DEFINE_UNQUOTED(HAVE_DBUS_GLIB_DISABLE_LEGACY_PROP_ACCESS, $ac_have_dg_prop, [Define if you have a dbus-glib with dbus_glib_global_set_disable_legacy_property_access()])

PKG_CHECK_MODULES(DBUS, dbus-1 >= 1.1 dbus-glib-1 >= 0.75)
AC_SUBST(DBUS_CFLAGS)
AC_SUBST(DBUS_LIBS)

PKG_CHECK_MODULES(GLIB, gthread-2.0 glib-2.0 >= 2.18 gobject-2.0)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

PKG_CHECK_MODULES(GMODULE, gmodule-2.0)
AC_SUBST(GMODULE_CFLAGS)
AC_SUBST(GMODULE_LIBS)

PKG_CHECK_MODULES(GUDEV, gudev-1.0)
AC_SUBST(GUDEV_CFLAGS)
AC_SUBST(GUDEV_LIBS)

PKG_CHECK_MODULES(GIO, gio-2.0)
AC_SUBST(GIO_CFLAGS)
AC_SUBST(GIO_LIBS)

AC_ARG_WITH(udev-dir, AS_HELP_STRING([--with-udev-dir=DIR], [where the udev base directory is]))
if test -n "$with_udev_dir" ; then
	UDEV_BASE_DIR="$with_udev_dir"
else
	UDEV_BASE_DIR="/lib/udev"
fi
AC_SUBST(UDEV_BASE_DIR)

# systemd
AC_ARG_WITH([systemdsystemunitdir],
 AS_HELP_STRING([--with-systemdsystemunitdir=DIR], [Directory for systemd service files]),
 [],
 [with_systemdsystemunitdir=$($PKG_CONFIG --variable=systemdsystemunitdir systemd)])
if test "x$with_systemdsystemunitdir" != xno; then
	AC_SUBST([systemdsystemunitdir], [$with_systemdsystemunitdir])
fi
AM_CONDITIONAL(HAVE_SYSTEMD, [test -n "$with_systemdsystemunitdir" -a "x$with_systemdsystemunitdir" != xno ])

PKG_CHECK_MODULES(LIBNL, libnl-1 >= 1.0-pre8)
AC_SUBST(LIBNL_CFLAGS)
AC_SUBST(LIBNL_LIBS)
NM_LIBNL_CHECK

PKG_CHECK_MODULES(UUID, uuid)
AC_SUBST(UUID_CFLAGS)
AC_SUBST(UUID_LIBS)

PKG_CHECK_MODULES(POLKIT, polkit-gobject-1)
AC_SUBST(POLKIT_CFLAGS)

# Check for polkit_authority_get_sync()
AC_CHECK_LIB([polkit-gobject-1], [polkit_authority_get_sync], ac_have_pk_auth_get_sync="1", ac_have_pk_auth_get_sync="0")
AC_DEFINE_UNQUOTED(HAVE_POLKIT_AUTHORITY_GET_SYNC, $ac_have_pk_auth_get_sync, [Define if you have a polkit with polkit_authority_get_sync()])

AC_ARG_WITH(crypto, AS_HELP_STRING([--with-crypto=nss | gnutls], [Cryptography library to use for certificate and key operations]),ac_crypto=$withval, ac_crypto=nss)

GLIB_GENMARSHAL=`pkg-config --variable=glib_genmarshal glib-2.0`
AC_SUBST(GLIB_GENMARSHAL)

AC_ARG_WITH(dbus-sys-dir, AS_HELP_STRING([--with-dbus-sys-dir=DIR], [where D-BUS system.d directory is]))

if test -n "$with_dbus_sys_dir" ; then
    DBUS_SYS_DIR="$with_dbus_sys_dir"
else
    DBUS_SYS_DIR="${sysconfdir}/dbus-1/system.d"
fi
AC_SUBST(DBUS_SYS_DIR)

BM_COMPILER_WARNINGS

GTK_DOC_CHECK(1.0)

dnl
dnl Tests
dnl
AC_ARG_WITH(tests, AS_HELP_STRING([--with-tests], [Build BarcodeManager tests]))
AM_CONDITIONAL(WITH_TESTS, test "x$with_tests" = "xyes")
case $with_tests in
    yes)
        with_tests=yes
        ;;
    *)
        with_tests=no
        ;;
esac


AC_CONFIG_FILES([
Makefile
include/Makefile
include/bm-version.h
src/Makefile
src/tests/Makefile
marshallers/Makefile
src/logging/Makefile
src/usb-manager/Makefile
src/bluez-manager/Makefile
src/system-settings/Makefile
src/backends/Makefile
libbm-util/libbm-util.pc
libbm-util/Makefile
libbm-glib/libbm-glib.pc
libbm-glib/Makefile
callouts/Makefile
tools/Makefile
system-settings/Makefile
cli/Makefile
cli/src/Makefile
test/Makefile
initscript/Makefile
initscript/Debian/Makefile
initscript/Debian/BarcodeManager
introspection/Makefile
man/Makefile
man/BarcodeManager.8
man/BarcodeManager.conf.5
man/bm-system-settings.conf.5
man/bm-tool.1
man/bm-online.1
man/bmcli.1
po/Makefile.in
policy/Makefile
data/Makefile
docs/Makefile
docs/libbm-glib/Makefile
docs/libbm-util/Makefile
BarcodeManager.pc
examples/Makefile
examples/python/Makefile
])
AC_OUTPUT

echo
echo Distribution target: ${with_distro}
echo 'if this is not correct, please specifiy your distro with --with-distro=DISTRO'

echo 

echo
echo Building documentation: ${with_docs}
echo Building tests: ${with_tests}
echo

