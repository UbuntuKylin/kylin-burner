dnl Process this file with autoconf to produce a configure script.
AC_INIT([burner],[3.12.1])
AC_CONFIG_SRCDIR(src/main.c)

BURNER_MAJOR_VERSION=3
BURNER_MINOR_VERSION=12
BURNER_SUB=1
BURNER_VERSION=$BURNER_MAJOR_VERSION.$BURNER_MINOR_VERSION.$BURNER_SUB

AC_SUBST(BURNER_MAJOR_VERSION)
AC_SUBST(BURNER_MINOR_VERSION)
AC_SUBST(BURNER_SUB)
AC_SUBST(BURNER_VERSION)

dnl Before making a release, the BURN_LT_VERSION string should be modified.
dnl The string is of the form C:R:A.
dnl - If interfaces have been changed or added, but binary compatibility has
dnl   been preserved, change to C+1:0:A+1
dnl - If binary compatibility has been broken (eg removed or changed interfaces)
dnl   change to C+1:0:0
dnl - If the interface is the same as the previous version, change to C:R+1:A

LT_CURRENT=3
LT_REVISION=6
LT_AGE=2

AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)

LIBBURNER_LT_VERSION=$LT_CURRENT:$LT_REVISION:$LT_AGE
AC_SUBST(LIBBURNER_LT_VERSION) 

TYPELIB_VERSION=$BURNER_MAJOR_VERSION.$((LT_CURRENT-LT_AGE))
AC_SUBST(TYPELIB_VERSION)

AC_DEFINE_UNQUOTED(BURNER_MAJOR_VERSION, $BURNER_MAJOR_VERSION, [major version])
AC_DEFINE_UNQUOTED(BURNER_MINOR_VERSION, $BURNER_MINOR_VERSION, [minor version])
AC_DEFINE_UNQUOTED(BURNER_SUB, $BURNER_SUB, [sub version])
AC_DEFINE_UNQUOTED(BURNER_VERSION, $BURNER_VERSION, [version])

dnl this is what it used to be
dnl AM_INIT_AUTOMAKE(burner, $BURNER_VERSION)

AM_INIT_AUTOMAKE([foreign])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_CONFIG_HEADERS([config.h])

AC_DISABLE_STATIC
LT_INIT

AM_MAINTAINER_MODE([enable])

dnl **************** Support GSettings *************************
GLIB_GSETTINGS

dnl **************** documentation *****************************
YELP_HELP_INIT
GTK_DOC_CHECK([1.12])

dnl ***************** GNOME MACROS *****************************

dnl This allows to set warnings
GNOME_COMPILE_WARNINGS([maximum])
GNOME_CXX_WARNINGS

dnl This could be use one day not to compile all debugging message
dnl GNOME_DEBUG_CHECK

GNOME_MAINTAINER_MODE_DEFINES
DISABLE_DEPRECATED="$DISABLE_DEPRECATED -DGSEAL_ENABLE"
AC_SUBST(DISABLE_DEPRECATED)

dnl ***************** GENERAL **********************************

dnl ** reminder: the following set CFLAGS to -O2 -g if empty
AC_PROG_CC
AC_HEADER_STDC

dnl Set PACKAGE_DATA_DIR in config.h.
if test "x${datadir}" = 'x${prefix}/share'; then
  if test "x${prefix}" = "xNONE"; then
    AC_DEFINE_UNQUOTED([PACKAGE_DATA_DIR], "${ac_default_prefix}/share/", [Define the PACKAGE_DATA_DIR.])
  else
    AC_DEFINE_UNQUOTED([PACKAGE_DATA_DIR], "${prefix}/share/", [Define the PACKAGE_DATA_DIR.])
  fi
  elif test "x${datadir}" = 'x${datarootdir}'; then
  if test "x${datarootdir}" = 'x${prefix}/share'; then
  	if test "x${prefix}" = "xNONE"; then
 	AC_DEFINE_UNQUOTED([PACKAGE_DATA_DIR], "${ac_default_prefix}/share/", [Define the PACKAGE_DATA_DIR.])
 	else
 	AC_DEFINE_UNQUOTED([PACKAGE_DATA_DIR], "${prefix}/share/", [Define the PACKAGE_DATA_DIR.])
 	fi
  else
 	AC_DEFINE_UNQUOTED([PACKAGE_DATA_DIR], "${datarootdir}/", [Define the PACKAGE_DATA_DIR.])
 	fi
  else
	AC_DEFINE_UNQUOTED([PACKAGE_DATA_DIR], "${datadir}/", [Define the PACKAGE_DATA_DIR.])
fi

dnl ***************** SCSI related *****************************
AC_SUBST(BURNER_SCSI_LIBS)
AC_CHECK_HEADERS([camlib.h],[has_cam="yes"],[has_cam="no"])

dnl ***************** check for linux sg interface *************
AC_CHECK_TYPES([sg_io_hdr_t],[has_sg="yes"],[has_sg="no"],
[#include <sys/types.h>
 #include <scsi/sg.h>])

dnl ***************** check for netbsd *************************
AC_CHECK_TYPES([scsireq_t],[has_scsiio="yes"],[has_scsiio="no"],
[#include <sys/types.h>
 #include <sys/scsiio.h>])

dnl ***************** check for solaris uscsi interface ********
AC_CHECK_TYPES([struct uscsi_cmd],[has_uscsi="yes"],[has_uscsi="no"],
[#include <sys/types.h>
 #include <sys/scsi/impl/uscsi.h>])

if test x"$has_cam" = x"yes"; then
    BURNER_SCSI_LIBS="-lcam"
elif test x"$has_sg" = x"yes"; then
	:
elif test x"$has_scsiio" = x"yes"; then
	:
elif test x"$has_uscsi" = x"yes"; then
	:
else
	AC_MSG_ERROR([Support Linux SG, FreeBSD CAM, NetBSD SCSIPI, Solaris USCSI. No supported SCSI interface headers could not be found.])
fi

AM_CONDITIONAL(HAVE_CAM_LIB_H, test x"$has_cam" = "xyes")
AM_CONDITIONAL(HAVE_SG_IO_HDR_T, test x"$has_sg" = "xyes")
AM_CONDITIONAL(HAVE_USCSI_H, test x"$has_uscsi" = "xyes")
AM_CONDITIONAL(HAVE_SCSIIO_H, test x"$has_scsiio" = "xyes")

dnl ***************** LARGE FILE SUPPORT ***********************

AC_SYS_LARGEFILE

dnl ********** Required libraries **********************

GLIB_REQUIRED=2.29.14
GTHREAD_REQUIRED=2.6.0
GMODULE_REQUIRED=2.6.0
GMODULE_EXPORT_REQUIRED=2.6.0
GIO_REQUIRED=2.28.0
GSTREAMER_REQUIRED=0.11.92
GSTREAMER_BASE_REQUIRED=0.11.92
GSTREAMER_MODULE_REQUIRED=0.11.92
LIBXML2_REQUIRED=2.6.0

dnl ** used by burner and one plugin
PKG_CHECK_MODULES(BURNER_GSTREAMER, 			\
	gstreamer-1.0 >= $GSTREAMER_REQUIRED		\
	gstreamer-base-1.0 >= $GSTREAMER_REQUIRED		\
	gstreamer-video-1.0 >= $GSTREAMER_BASE_REQUIRED	\
	gstreamer-pbutils-1.0 >= $GSTREAMER_BASE_REQUIRED \
	gstreamer-tag-1.0 >= $GSTREAMER_BASE_REQUIRED)
BURNER_GSTREAMER_CFLAGS="-DGST_USE_UNSTABLE_API $BURNER_GSTREAMER_CFLAGS"
AC_SUBST(BURNER_GSTREAMER_CFLAGS)
AC_SUBST(BURNER_GSTREAMER_LIBS)

BURNER_GSTREAMER_LIBS="$BURNER_GSTREAMER_LIBS -lgstpbutils-1.0"

dnl ** used by burner and one plugin
PKG_CHECK_MODULES(BURNER_GIO, 	\
	gio-2.0 >= $GIO_REQUIRED)

AC_SUBST(BURNER_GIO_CFLAGS)
AC_SUBST(BURNER_GIO_LIBS)

dnl ** used by burner and one plugin
PKG_CHECK_MODULES(BURNER_LIBXML, 	\
	libxml-2.0 >= $LIBXML2_REQUIRED)

AC_SUBST(BURNER_LIBXML_CFLAGS)
AC_SUBST(BURNER_LIBXML_LIBS)

dnl ** used by burner and both libraries
PKG_CHECK_MODULES(BURNER_GTHREAD, 		\
	gthread-2.0 >= $GTHREAD_REQUIRED)

AC_SUBST(BURNER_GTHREAD_CFLAGS)
AC_SUBST(BURNER_GTHREAD_LIBS)

dnl ** used by burner and all modules
PKG_CHECK_MODULES(BURNER_GLIB, 		\
	gobject-2.0 >= $GLIB_REQUIRED		\
	glib-2.0 >= $GLIB_REQUIRED)

AC_SUBST(BURNER_GLIB_CFLAGS)
AC_SUBST(BURNER_GLIB_LIBS)

PKG_CHECK_MODULES(BURNER_GMODULE,		\
	gmodule-2.0 >= $GMODULE_REQUIRED)

AC_SUBST(BURNER_GMODULE_CFLAGS)
AC_SUBST(BURNER_GMODULE_LIBS)

PKG_CHECK_MODULES(BURNER_GMODULE_EXPORT,		\
	gmodule-export-2.0 >= $GMODULE_EXPORT_REQUIRED)

AC_SUBST(BURNER_GMODULE_EXPORT_CFLAGS)
AC_SUBST(BURNER_GMODULE_EXPORT_LIBS)

PKG_CHECK_MODULES(BURNER_SM,
	sm \
	ice)

AC_SUBST(BURNER_SM_CFLAGS)
AC_SUBST(BURNER_SM_LIBS)

AC_CHECK_LIBM
AC_SUBST(LIBM)

#
# GTK+ stuff (taken and adapted from empathy)
#

GTK_REQUIRED=3.0.0

PKG_CHECK_MODULES([BURNER_GTK],	\
		  gtk+-3.0 >= $GTK_REQUIRED	\
		  gdk-x11-3.0)

AC_SUBST(BURNER_GTK_CFLAGS)
AC_SUBST(BURNER_GTK_LIBS)

#
# libnotify check
#

LIBNOTIFY_REQUIRED=0.6.1

PKG_CHECK_MODULES([BURNER_LIBNOTIFY],[libnotify >= $LIBNOTIFY_REQUIRED])
AC_SUBST(BURNER_LIBNOTIFY_CFLAGS)
AC_SUBST(BURNER_LIBNOTIFY_LIBS)

#
# Canberra
#

CANBERRA_REQUIRED=0.1

PKG_CHECK_MODULES(BURNER_CANBERRA,
	      libcanberra >= $CANBERRA_REQUIRED
	      libcanberra-gtk3 >= $CANBERRA_REQUIRED)

AC_SUBST(BURNER_CANBERRA_CFLAGS)
AC_SUBST(BURNER_CANBERRA_LIBS)

dnl ****************Nautilus**********************************
# Note: the extension is built against gtk+-3.0 so we cannot build extension with gtk+-2.0
NAUTILUS_REQUIRED=2.91.90

AC_ARG_ENABLE(nautilus,
			AS_HELP_STRING([--enable-nautilus],[Build Nautilus extension [[default=yes]]]),
			[enable_nautilus=$enableval],
			[enable_nautilus="yes"])

AC_ARG_WITH(nautilusdir,
			AS_HELP_STRING([--with-nautilusdir=DIR],[Installation path for Nautilus extension @<:@auto@:>@]),
			[ac_with_nautilusdir=$withval],
			[ac_with_nautilusdir=""])

if test x"$enable_nautilus" = "xyes"; then
    	PKG_CHECK_MODULES(NAUTILUS_EXTENSION,
			  libnautilus-extension >= $NAUTILUS_REQUIRED,
			  build_nautilus=yes,
			  build_nautilus=no)
else
	build_nautilus="no"
fi

if test x"$build_nautilus" = "xyes"; then
	NAUTILUS_EXTENSION_CFLAGS="$BURNER_GLIB_CFLAGS $BURNER_UI_CFLAGS $NAUTILUS_EXTENSION_CFLAGS"
	NAUTILUS_EXTENSION_LIBS="$BURNER_GLIB_LIBS $BURNER_UI_LIBS $NAUTILUS_EXTENSION_LIBS"

	if test "${ac_with_nautilusdir}" = ""; then
		ac_with_nautilusdir=`pkg-config --variable=extensiondir libnautilus-extension`
	fi

	AC_MSG_NOTICE([installing plugin in ${ac_with_nautilusdir}])
	AC_SUBST([NAUTILUSDIR],[${ac_with_nautilusdir}])

	AM_GLIB_DEFINE_LOCALEDIR([GNOMELOCALEDIR])
fi

AM_CONDITIONAL(BUILD_NAUTILUS, test x"$build_nautilus" = "xyes")

#
# Installation path for plugins
#

BURNER_PLUGIN_DIRECTORY=${BURNER_PLUGIN_DIRECTORY:-'${libdir}/burner3/plugins'}
AC_SUBST(BURNER_PLUGIN_DIRECTORY)

dnl ****************check for libburn (optional)**************
LIBBURN_REQUIRED=0.4.0
LIBISOFS_REQUIRED=0.6.4

AC_ARG_ENABLE(libburnia,
			AS_HELP_STRING([--enable-libburnia],[Build libburnia plugins [[default=yes]]]),
			[enable_libburnia=$enableval],
			[enable_libburnia="yes"])

if test x"$enable_libburnia" = "xyes"; then
	PKG_CHECK_MODULES(LIBBURNIA, libburn-1 >= $LIBBURN_REQUIRED  libisofs-1 >= $LIBISOFS_REQUIRED, build_libburnia=yes, build_libburnia=no)
else
	build_libburnia="no"
fi

if test x"$build_libburnia" = "xyes"; then
	AC_SUBST(BURNER_LIBBURNIA_CFLAGS)
	AC_SUBST(BURNER_LIBBURNIA_LIBS)

	BURNER_LIBBURNIA_CFLAGS="$LIBBURNIA_CFLAGS"
	BURNER_LIBBURNIA_LIBS="$LIBBURNIA_LIBS"
	AC_DEFINE(BUILD_LIBBURNIA, 1, [define if you  want to build libburnia plugins])
fi
AM_CONDITIONAL(BUILD_LIBBURNIA, test x"$build_libburnia" = "xyes")

dnl **************** check for cdrtools **********************
AC_ARG_ENABLE(cdrtools,
			AS_HELP_STRING([--enable-cdrtools],[Build cdrtools plugins [[default=yes]]]),
			[build_cdrtools=$enableval],
			[build_cdrtools="yes"])

if test x"$build_cdrtools" = "xyes"; then
	AC_DEFINE(BUILD_CDRTOOLS, 1, [define if you  want to build cdrtools plugins])
else
	build_cdrtools="no"
fi
AM_CONDITIONAL(BUILD_CDRTOOLS, test x"$build_cdrtools" = "xyes")

dnl **************** check for cdrdao **********************
AC_ARG_ENABLE(cdrdao,
			AS_HELP_STRING([--enable-cdrdao],[Build cdrdao plugins [[default=yes]]]),
			[build_cdrdao=$enableval],
			[build_cdrdao="yes"])

if test x"$build_cdrdao" = "xyes"; then
	AC_DEFINE(BUILD_CDRDAO, 1, [define if you  want to build cdrdao plugins])
else
	build_cdrdao="no"
fi
AM_CONDITIONAL(BUILD_CDRDAO, test x"$build_cdrdao" = "xyes")

dnl **************** check for cdrkit ************************
AC_ARG_ENABLE(cdrkit,
			AS_HELP_STRING([--enable-cdrkit],[Build cdrkit plugins [[default=yes]]]),
			[build_cdrkit=$enableval],
			[build_cdrkit="yes"])

if test x"$build_cdrkit" = "xyes"; then
	AC_DEFINE(BUILD_CDRKIT, 1, [define if you  want to build cdrkit plugins])
else
	build_cdrkit="no"
fi
AM_CONDITIONAL(BUILD_CDRKIT, test x"$build_cdrkit" = "xyes")

dnl **************** check for growisofs ************************
AC_ARG_ENABLE(growisofs,
			AS_HELP_STRING([--enable-growisofs],[Build growisofs plugins [[default=yes]]]),
			[build_growisofs=$enableval],
			[build_growisofs="yes"])

if test x"$build_growisofs" = "xyes"; then
	AC_DEFINE(BUILD_GROWISOFS, 1, [define if you  want to build growisofs plugins])
else
	build_growisofs="no"
fi
AM_CONDITIONAL(BUILD_GROWISOFS, test x"$build_growisofs" = "xyes")

dnl ****************check for appindicators (optional) **************
APPINDICATOR_REQUIRED=0.0.7

AC_ARG_ENABLE(appindicator,
			AS_HELP_STRING([--enable-appindicator],[Build support for application indicators [[default=yes]]]),
			[enable_appindicator=$enableval],
			[enable_appindicator="yes"])

if test x$enable_appindicator = xyes ; then
	PKG_CHECK_MODULES(APP_INDICATOR,
			appindicator3-0.1 >= $APPINDICATOR_REQUIRED)
	AC_SUBST(APP_INDICATOR_CFLAGS)
	AC_SUBST(APP_INDICATOR_LIBS)
	AC_DEFINE(HAVE_APP_INDICATOR, 1, [Have AppIndicator])
fi
AM_CONDITIONAL(HAVE_APP_INDICATOR, test x"$enable_appindicator" = xyes)

dnl *************** check for unity ***********************

AC_ARG_ENABLE(unity,
            AS_HELP_STRING([--enable-unity],[Build support for integration in Unity launcher [[default=no]]]),
            [enable_unity=$enableval],
            [enable_unity="no"])

if test x"$enable_unity" = "xyes"; then
    PKG_CHECK_MODULES(UNITY, unity >= 4.0.2)
    AC_SUBST(UNITY_CFLAGS)
    AC_SUBST(UNITY_LIBS)
    AC_DEFINE(HAVE_UNITY, 1, [Unity launcher support])
fi
AM_CONDITIONAL(HAVE_UNITY, test x"$enable_unity" = xyes)

TRACKER_REQUIRED=0.10.0

AC_ARG_ENABLE(search,
			AS_HELP_STRING([--enable-search],[Build search pane (if a search backend is available) [[default=auto]]]),
			[case "$enableval" in
				"auto"|"yes"|"no") ;;
				*)         echo "Unknown option"; exit 2 ;;
			esac],
			[enable_search="auto"])

if test x"$enable_search" != "xno"; then
        # Try to figure out the tracker API version to use
        tracker_api="0.10"
        m4_foreach([VERSION], [[0.10], [0.12], [0.14], [0.16], [0.18], [1.0]],
                              [PKG_CHECK_EXISTS([tracker-sparql-VERSION >= $TRACKER_REQUIRED],
                                                [tracker_api="VERSION"])
                              ])
fi

if test x"$enable_search" = "xauto"; then
        PKG_CHECK_EXISTS([tracker-sparql-$tracker_api >= $TRACKER_REQUIRED],
                         [enable_search=yes],
                         [enable_search=no])
fi

if test x"$enable_search" = "xyes"; then
	PKG_CHECK_MODULES(BURNER_SEARCH, tracker-sparql-$tracker_api >= $TRACKER_REQUIRED)
	AC_DEFINE(BUILD_SEARCH, 1, [define if you  want to use search pane])
	AC_DEFINE(BUILD_TRACKER, 1, [define if you  want to use search pane])
	AC_SUBST(BURNER_SEARCH_CFLAGS)
	AC_SUBST(BURNER_SEARCH_LIBS)
	build_tracker="yes"
	build_search="yes"
else
	build_search="no"
fi

AM_CONDITIONAL(BUILD_SEARCH, test x"$build_search" = "xyes")
AM_CONDITIONAL(BUILD_TRACKER, test x"$build_tracker" = "xyes")

dnl ****************check for playlist (optional)**************
TOTEM_REQUIRED=2.29.1

AC_ARG_ENABLE(playlist,
			AS_HELP_STRING([--enable-playlist],[Build playlist pane (if totem-pl-parser is available)[[default=yes]]]),
			[enable_playlist=$enableval],
			[enable_playlist="yes"])

if test x"$enable_playlist" = "xyes"; then
	PKG_CHECK_MODULES(BURNER_PL_PARSER, totem-plparser >= $TOTEM_REQUIRED, build_totem=yes, build_totem=no)
else
	build_totem="no"
fi

if test x"$build_totem" = "xyes"; then
	AC_DEFINE(BUILD_PLAYLIST, 1, [define if you  want to build playlist pane])
fi

AC_SUBST(BURNER_PL_PARSER_CFLAGS)
AC_SUBST(BURNER_PL_PARSER_LIBS)

AM_CONDITIONAL(BUILD_PLAYLIST, test x"$build_totem" = "xyes")

dnl ****************check for preview (optional)**************
AC_ARG_ENABLE(preview,
			AS_HELP_STRING([--enable-preview],[Build preview pane [[default=yes]]]),
			[enable_preview=$enableval],
			[enable_preview="yes"])

if test x"$enable_preview" = "xyes"; then
	PKG_CHECK_MODULES(BURNER_GSTREAMER_BASE, gstreamer-plugins-base-1.0 >= $GSTREAMER_MODULE_REQUIRED, build_preview=yes, build_preview=no)
else
	build_preview="no"
fi

if test x"$build_preview" = "xyes"; then
	AC_DEFINE(BUILD_PREVIEW, 1, [define if you  want to build preview pane])
fi

AC_SUBST(BURNER_GSTREAMER_BASE_CFLAGS)
AC_SUBST(BURNER_GSTREAMER_BASE_LIBS)

AM_CONDITIONAL(BUILD_PREVIEW, test x"$build_preview" = "xyes")

dnl ****************check for inotify (optional)**************
AC_ARG_ENABLE(inotify,
			AS_HELP_STRING([--enable-inotify],[use inotify [[default=yes]]]),
			[enable_inotify=$enableval],
			[enable_inotify="yes"])

if test x"$enable_inotify" = "xyes"; then
	AC_DEFINE(BUILD_INOTIFY, 1, [define if you  want to build inotify])
fi
AM_CONDITIONAL(BUILD_INOTIFY, test x"$enable_inotify" = "xyes")

dnl ****** GObject Introspection **********************
dnl This is a copy of the m4 function in gobject-introspection package
dnl but it was modified so the install path can be forced with a command
dnl line parameter. This is to be able to use make distcheck.

GOBJECT_INTROSPECTION_REQUIRED=0.6.3

AC_ARG_ENABLE(introspection,
                  AS_HELP_STRING([--enable-introspection[=@<:@no/auto/yes@:>@]], [Enable introspection for this build]),
		  [enable_introspection=$enableval],
		  [enable_introspection="auto"])

AC_ARG_WITH(girdir,
			AS_HELP_STRING([--with-girdir=DIR],[Installation path for .gir files @<:@auto@:>@]),
			[ac_with_girdir=$withval],
			[ac_with_girdir=""])

AC_ARG_WITH(typelibdir,
			AS_HELP_STRING([--with-typelibdir=DIR],[Installation path for .typelib files @<:@auto@:>@]),
			[ac_with_typelibdir=$withval],
			[ac_with_typelibdir=""])

AC_MSG_CHECKING([for gobject-introspection])

dnl presence/version checking
AS_CASE([$enable_introspection],
    [no], [dnl
        found_introspection="no (disabled, use --enable-introspection to enable)"
    ],dnl
    [yes],[dnl
        PKG_CHECK_EXISTS([gobject-introspection-1.0],,
                         AC_MSG_ERROR([gobject-introspection-1.0 is not installed]))
        PKG_CHECK_EXISTS([gobject-introspection-1.0 >= $GOBJECT_INTROSPECTION_REQUIRED],
                         found_introspection=yes,
                         AC_MSG_ERROR([You need to have gobject-introspection >= $1 installed to build AC_PACKAGE_NAME]))
    ],dnl
    [auto],[dnl
        PKG_CHECK_EXISTS([gobject-introspection-1.0 >= $GOBJECT_INTROSPECTION_REQUIRED], found_introspection=yes, found_introspection=no)
    ],dnl
    [dnl	
        AC_MSG_ERROR([invalid argument passed to --enable-introspection, should be one of @<:@no/auto/yes@:>@])
])dnl

AC_MSG_RESULT([$found_introspection])

INTROSPECTION_SCANNER=
INTROSPECTION_COMPILER=
INTROSPECTION_GENERATE=
INTROSPECTION_GIRDIR=
INTROSPECTION_TYPELIBDIR=
if test "x$found_introspection" = "xyes"; then
	INTROSPECTION_SCANNER=`$PKG_CONFIG --variable=g_ir_scanner gobject-introspection-1.0`
	INTROSPECTION_COMPILER=`$PKG_CONFIG --variable=g_ir_compiler gobject-introspection-1.0`
	INTROSPECTION_GENERATE=`$PKG_CONFIG --variable=g_ir_generate gobject-introspection-1.0`

	if test "${ac_with_girdir}" = ""; then
		ac_with_girdir=`$PKG_CONFIG --variable=girdir gobject-introspection-1.0`
	fi
	INTROSPECTION_GIRDIR=${ac_with_girdir}
	AC_MSG_NOTICE([installing .gir files in ${INTROSPECTION_GIRDIR}])

	if test "${ac_with_typelibdir}" = ""; then
		ac_with_typelibdir="$($PKG_CONFIG --variable=typelibdir gobject-introspection-1.0)"
	fi
	INTROSPECTION_TYPELIBDIR=${ac_with_typelibdir}
	AC_MSG_NOTICE([installing .typelib files in ${INTROSPECTION_TYPELIBDIR}])
fi
AC_SUBST(INTROSPECTION_SCANNER)
AC_SUBST(INTROSPECTION_COMPILER)
AC_SUBST(INTROSPECTION_GENERATE)
AC_SUBST(INTROSPECTION_GIRDIR)
AC_SUBST(INTROSPECTION_TYPELIBDIR)

AM_CONDITIONAL(HAVE_INTROSPECTION, test "x$found_introspection" = "xyes")

dnl ****** Update mime, desktop and icon caches *******
AC_ARG_ENABLE(caches,
			AS_HELP_STRING([--enable-caches],[Run update-* to update mime, desktop and icon caches when installing [[default=yes]]]),
			[enable_caches=$enableval],
			[enable_caches="yes"])

AM_CONDITIONAL(UPDATE_CACHES, test x"$enable_caches" = "xyes")

dnl ****** INTERNATIONALIZATION **********************
GETTEXT_PACKAGE=burner
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Package name for gettext])

AM_GLIB_GNU_GETTEXT

IT_PROG_INTLTOOL([0.50])

if test "x${prefix}" = "xNONE"; then
  AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, "${ac_default_prefix}/${DATADIRNAME}/locale", [Location of package locale files])
else
  AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, "${prefix}/${DATADIRNAME}/locale", [Location of package locale files])
fi

dnl ****** END ***********************

AC_CONFIG_FILES([
Makefile
docs/Makefile
docs/reference/Makefile
docs/reference/libburner-media/Makefile
docs/reference/libburner-burn/Makefile
docs/reference/libburner-media/version.xml
docs/reference/libburner-burn/version.xml
data/Makefile
data/burner.desktop.in
data/icons/Makefile
data/mime/Makefile
help/Makefile
nautilus/Makefile
nautilus/burner-nautilus.desktop.in
libburner-media/Makefile
libburner-media/burner-media.h
libburner-utils/Makefile
libburner-burn/Makefile
libburner-burn/burner-burn-lib.h
plugins/Makefile
plugins/audio2cue/Makefile
plugins/cdrdao/Makefile
plugins/cdrkit/Makefile
plugins/cdrtools/Makefile
plugins/growisofs/Makefile
plugins/libburnia/Makefile
plugins/transcode/Makefile
plugins/dvdcss/Makefile
plugins/dvdauthor/Makefile
plugins/checksum/Makefile
plugins/local-track/Makefile
plugins/vcdimager/Makefile
po/Makefile.in
src/Makefile
libburner-media3.pc
libburner-burn3.pc
])

AC_OUTPUT

echo
echo
echo "$PACKAGE configuration summary:"
echo "----------------------------------"

dnl 	Build libnotify support : ${build_libnotify}

echo "Version: $BURNER_VERSION
	Using GTK+ 3
	Update caches: ${enable_caches}
	Build Nautilus extension : ${build_nautilus}
	Build inotify: ${enable_inotify}
	Build search pane : ${build_search}
	Build playlist pane : ${build_totem}
	Build Preview pane : ${build_preview}
	Plugins installed in : ${BURNER_PLUGIN_DIRECTORY}
	Build cdrdao plugins : ${build_cdrdao}
	Build cdrtools plugins : ${build_cdrtools}
	Build cdrkit plugins : ${build_cdrkit}
	Build growisofs plugins : ${build_growisofs}
	Build libburnia plugins : ${build_libburnia}
	Build GObject-Introspection : ${found_introspection}
	Build Application Indicators : ${enable_appindicator}
"
echo
echo
