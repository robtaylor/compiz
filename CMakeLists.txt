project (compiz)

set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include (cmake/base.cmake)
include (cmake/macros.cmake)
include (CheckFunctionExists)

set (CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRe" CACHE INTERNAL "" FORCE)
if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type (Debug/Release/RelWithDebInfo/MinSizeRe)" FORCE)
endif (NOT CMAKE_BUILD_TYPE)

# compiz package version number
# An odd micro number indicates in-progress development.
# An even micro number indicates a released version.
set (COMPIZ_VERSION_MAJOR 1)
set (COMPIZ_VERSION_MINOR 91)
set (COMPIZ_VERSION_MICRO 1)
set (VERSION ${COMPIZ_VERSION_MAJOR}.${COMPIZ_VERSION_MINOR}.${COMPIZ_VERSION_MICRO})

set (DECOR_INTERFACE_VERSION 20080901)

set (prefix ${CMAKE_INSTALL_PREFIX})
set (exec_prefix ${CMAKE_INSTALL_PREFIX}/bin)
set (libdir ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX})
set (includedir ${CMAKE_INSTALL_PREFIX}/include)
set (datadir ${CMAKE_INSTALL_PREFIX}/share)

set (compiz_plugindir ${libdir}/compiz)
set (compiz_imagedir ${datadir}/compiz)
set (compiz_metadatadir ${datadir}/compiz)

set (ALL_LINGUAS af ar bg bn bn_IN bs ca cs cy da de el en_GB en_US es eu et fi fr gl gu he hi hr hu id it ja ka km ko lo lt mk mr nb nl or pa pl pt pt_BR ro ru sk sl sr sv ta tr uk vi xh zh_CN zh_TW zu)
set (GETTEXT_PACKAGE compiz)

find_package (Boost)
if (NOT Boost_FOUND OR Boost_MAJOR_VERSION EQUAL 1 AND Boost_MINOR_VERSION LESS 34)
    message (FATAL_ERROR "Boost version >= 1.34.0 needed")
endif (NOT Boost_FOUND OR Boost_MAJOR_VERSION EQUAL 1 AND Boost_MINOR_VERSION LESS 34)

set (COMPIZ_REQUIRES
    x11-xcb
    xrandr
    xinerama
    ice
    sm
    libxml-2.0
    libxslt
    libstartup-notification-1.0>=0.7
)

set (DECORATION_REQUIRES xrender)

pkg_check_modules (COMPIZ REQUIRED ${COMPIZ_REQUIRES})
pkg_check_modules (LIBDECORATION REQUIRED ${DECORATION_REQUIRES})

set (COMPIZ_CFLAGS "${COMPIZ_CFLAGS} -I${Boost_INCLUDE_DIR}")

check_pkg_module (xrender>=0.9.3 HAVE_XRENDER_0_9_3)

set (USE_GTK 1 CACHE BOOL "Build gtk-window-decorator")
set (USE_METACITY 1 CACHE BOOL "Gtk-window-decorator metacity theme support")
set (USE_GNOME 1 CACHE BOOL "Gnome settings module")
set (USE_GNOME_KEYBINDINGS 1 CACHE BOOL "Gnome keybindings")
set (USE_GCONF 1 CACHE BOOL "Install GConf schemas")

if (USE_GCONF)
    pkg_check_modules (GCONF gconf-2.0)

    find_program (GCONFTOOL_EXECUTABLE gconftool-2)
    mark_as_advanced (FORCE GCONFTOOL_EXECUTABLE)

    if (NOT GCONF_FOUND OR NOT GCONFTOOL_EXECUTABLE)
	set (USE_GCONF 0)
    endif (NOT GCONF_FOUND OR NOT GCONFTOOL_EXECUTABLE)
endif (USE_GCONF)

if (USE_GTK)
    pkg_check_modules (GTK_WINDOW_DECORATOR
	xrender>=0.8.4
	gtk+-2.0>=2.8.0
	libwnck-1.0
	pangocairo
    )
    if (GTK_WINDOW_DECORATOR_FOUND)
        set (CMAKE_REQUIRED_FLAGS ${GTK_WINDOW_DECORATOR_CFLAGS})
        set (CMAKE_REQUIRED_LIBRARIES ${GTK_WINDOW_DECORATOR_LIBRARIES})
        check_function_exists (wnck_window_has_name HAVE_WNCK_WINDOW_HAS_NAME)
        set (CMAKE_REQUIRED_FLAGS "")
        set (CMAKE_REQUIRED_LIBRARIES "")

        check_pkg_module (libwnck-1.0>=2.18.1 HAVE_LIBWNCK_2_18_1)
        check_pkg_module (libwnck-1.0>=2.19.4 HAVE_LIBWNCK_2_19_4)

        if (USE_METACITY)
            pkg_check_modules (METACITY libmetacity-private)
            if (METACITY_FOUND)
		check_pkg_module (libmetacity-private>=2.15.21 HAVE_METACITY_2_15_21)
		check_pkg_module (libmetacity-private>=2.17.0 HAVE_METACITY_2_17_0)
		check_pkg_module (libmetacity-private>=2.23.2 HAVE_METACITY_2_23_2)
            else (METACITY_FOUND)
		set (USE_METACITY 0)
            endif (METACITY_FOUND)
        endif (USE_METACITY)

	if (USE_GNOME)
	    pkg_check_modules (GNOME_WINDOW_SETTINGS
		gnome-window-settings-2.0
		gnome-desktop-2.0
	    )

	    if (GNOME_WINDOW_SETTINGS_FOUND)
	        exec_program (${PKG_CONFIG_EXECUTABLE} 
		  ARGS "--variable=prefix gnome-window-settings-2.0" 
		  OUTPUT_VARIABLE windowsettingsdatadir
		)
		set (windowsettingsdatadir ${windowsettingsdatadir}/share)
		exec_program (${PKG_CONFIG_EXECUTABLE} 
		  ARGS "--variable=libdir gnome-window-settings-2.0"
		  OUTPUT_VARIABLE windowsettingslibdir
		)
	    else (GNOME_WINDOW_SETTINGS_FOUND)
		set (USE_GNOME 0)
	    endif (GNOME_WINDOW_SETTINGS_FOUND)
	endif (USE_GNOME)

	if (USE_GNOME_KEYBINDINGS)
	    pkg_check_modules (GNOME_KEY_BINDINGS gnome-keybindings)
	    if (GNOME_KEY_BINDINGS_FOUND)
		exec_program (${PKG_CONFIG_EXECUTABLE}
		  ARGS "--variable=keysdir gnome-keybindings"
		  OUTPUT_VARIABLE keybindingsdir
		)
	    else (GNOME_KEY_BINDINGS_FOUND)
		set (USE_GNOME_KEYBINDINGS 0)
	    endif (GNOME_KEY_BINDINGS_FOUND)
        endif (USE_GNOME_KEYBINDINGS)
    else (GTK_WINDOW_DECORATOR_FOUND)
	set (USE_GTK 0)
    endif (GTK_WINDOW_DECORATOR_FOUND)
endif (USE_GTK)

if (!USE_GTK)
    set (USE_METACITY 0)
    set (USE_GNOME 0)
    set (USE_GNOME_KEYBINDINGS 0)
endif (!USE_GTK)

set (_compiz_package_string "compiz ${VERSION}")

configure_file (${compiz_SOURCE_DIR}/config.h.cmake.in ${compiz_BINARY_DIR}/config.h)


add_subdirectory (src)
add_subdirectory (include)
add_subdirectory (images)
add_subdirectory (libdecoration)
add_subdirectory (gtk)
add_subdirectory (kde)
add_subdirectory (po)
add_subdirectory (metadata)
add_subdirectory (plugins)

# AC_ISC_POSIX
# AC_PROG_CC
# AC_PROG_CPP
# AC_PROG_CXX
# AC_PROG_LIBTOOL
# AC_HEADER_STDC
# AC_CHECK_HEADERS([stdlib.h sys/time.h unistd.h])
# 
# ALL_LINGUAS="af ar bg bn bn_IN bs ca cs cy da de el en_GB en_US es eu et fi fr gl gu he hi hr hu id it ja ka km ko lo lt mk mr nb nl or pa pl pt pt_BR ro ru sk sl sr sv ta tr uk vi xh zh_CN zh_TW zu"
# AC_SUBST(ALL_LINGUAS)
# AM_GLIB_GNU_GETTEXT
# GETTEXT_PACKAGE=compiz
# AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package.])
# AC_SUBST(GETTEXT_PACKAGE)
# 
# AX_BOOST_BASE([1.34.0])
# 
# if test "x$GCC" = "xyes"; then
#   case " $CFLAGS " in
#   *[[\ \	]]-Wall[[\ \	]]*) ;;
#   *) CFLAGS="$CFLAGS -Wall" ;;
#   esac
# 
#   case " $CFLAGS " in
#   *[[\ \	]]-Wpointer-arith[[\ \	]]*) ;;
#   *) CFLAGS="$CFLAGS -Wpointer-arith" ;;
#   esac
# 
#   case " $CFLAGS " in
#   *[[\ \	]]-Wstrict-prototypes[[\ \	]]*) ;;
#   *) CFLAGS="$CFLAGS -Wstrict-prototypes" ;;
#   esac
# 
#   case " $CFLAGS " in
#   *[[\ \	]]-Wmissing-prototypes[[\ \	]]*) ;;
#   *) CFLAGS="$CFLAGS -Wmissing-prototypes" ;;
#   esac
# 
#   case " $CFLAGS " in
#   *[[\ \	]]-Wmissing-declarations[[\ \	]]*) ;;
#   *) CFLAGS="$CFLAGS -Wmissing-declarations" ;;
#   esac
# 
#   case " $CFLAGS " in
#   *[[\ \	]]-Wnested-externs[[\ \	]]*) ;;
#   *) CFLAGS="$CFLAGS -Wnested-externs" ;;
#   esac
# 
#   case " $CFLAGS " in
#   *[[\ \	]]-D_FORTIFY_SOURCE=2[[\ \	]]*) ;;
#   *) CFLAGS="$CFLAGS -D_FORTIFY_SOURCE=2" ;;
#   esac
# 
#   if test "x$enable_ansi" = "xyes"; then
#     case " $CFLAGS " in
#     *[[\ \	]]-ansi[[\ \	]]*) ;;
#     *) CFLAGS="$CFLAGS -ansi" ;;
#     esac
# 
#     case " $CFLAGS " in
#     *[[\ \	]]-pedantic[[\ \	]]*) ;;
#     *) CFLAGS="$CFLAGS -pedantic" ;;
#     esac
#   fi
# fi
# 
# if test "x$GCC" = "xyes"; then
#   case " $CXXFLAGS " in
#   *[[\ \	]]-Wall[[\ \	]]*) ;;
#   *) CXXFLAGS="$CXXFLAGS -Wall" ;;
#   esac
# 
#   case " $CXXFLAGS " in
#   *[[\ \	]]-D_FORTIFY_SOURCE=2[[\ \	]]*) ;;
#   *) CXXFLAGS="$CXXFLAGS -D_FORTIFY_SOURCE=2" ;;
#   esac
# fi
# 
# AC_C_BIGENDIAN
# 

# AC_ARG_WITH(default-plugins, [  --with-default-plugins=PLUGINS ],
# 		       [default_plugins="$withval"],
# 		       [default_plugins=""])
# AC_SUBST(default_plugins)
# 
# 
# AC_MSG_CHECKING(for GL_CFLAGS)
# AC_ARG_WITH(gl-cflags, [  --with-gl-cflags=CFLAGS ],
# 		       [GL_CFLAGS="$withval"],
# 		       [GL_CFLAGS=""])
# 
# AC_MSG_RESULT($GL_CFLAGS)
# AC_MSG_CHECKING(for GL_LIBS)
# AC_ARG_WITH(gl-libs, [  --with-gl-libs=LIBS ],
# 		     [GL_LIBS="$withval"],
# 		     [GL_LIBS="-lGL"])
# AC_MSG_RESULT($GL_LIBS)
# 
# AC_SUBST(GL_CFLAGS)
# AC_SUBST(GL_LIBS)
# 
# AC_CHECK_PROGS(XSLTPROC, xsltproc)
# if test -z "$XSLTPROC"; then
#   AC_MSG_ERROR([Couldn't find xsltproc])
# fi
# 
# PKG_CHECK_MODULES(LIBPNG, libpng)
# 
# AC_ARG_ENABLE(glib,
#   [  --disable-glib          Disable glib plugin],
#   [use_glib=$enableval], [use_glib=yes])
# 
# if test "x$use_glib" = "xyes"; then
#   PKG_CHECK_MODULES(GLIB, glib-2.0)
# fi
# 
# AM_CONDITIONAL(USE_GLIB, test "x$use_glib" = "xyes")
# if test "$use_glib" = yes; then
#   AC_DEFINE(USE_GLIB, 1, [Build glib plugin])
# fi
# 
# AC_ARG_ENABLE(gconf,
#   [  --disable-gconf         Disable gconf plugin],
#   [use_gconf=$enableval], [use_gconf=yes])
# 
# if test "x$use_gconf" = "xyes"; then
#   PKG_CHECK_MODULES(GCONF, gconf-2.0)
# 
#   AC_PATH_PROG(GCONFTOOL, gconftool-2, no)
#   if test x"$GCONFTOOL" = xno; then
#     AC_MSG_ERROR([gconftool-2 executable not found in your path - should be installed with GConf])
#   fi
# fi
# 
# AM_GCONF_SOURCE_2
# 
# AM_CONDITIONAL(USE_GCONF, test "x$use_gconf" = "xyes")
# if test "$use_gconf" = yes; then
#   AC_DEFINE(USE_GCONF, 1, [Build gconf plugin])
# fi
# 
# stylesheetdir=$datadir/compiz
# AC_SUBST(stylesheetdir)
# 
# AC_ARG_ENABLE(dbus,
#   [  --disable-dbus          Disable dbus plugin],
#   [use_dbus=$enableval], [use_dbus=yes])
# 
# if test "x$use_dbus" = "xyes"; then
#   PKG_CHECK_MODULES(DBUS, dbus-1 libxml-2.0, [use_dbus=yes], [use_dbus=no])
# fi
# 
# AM_CONDITIONAL(DBUS_PLUGIN, test "x$use_dbus" = "xyes")
# if test "$use_dbus" = yes; then
#   AC_DEFINE(USE_DBUS, 1, [Build dbus plugin])
# fi
# 
# AC_ARG_ENABLE(dbus-glib,
#   [  --disable-dbus-glib     Disable dbus-glib support],
#   [use_dbus_glib=$enableval], [use_dbus_glib=yes])
# 
# if test "x$use_dbus_glib" = "xyes"; then
#   PKG_CHECK_MODULES(DBUS_GLIB, dbus-glib-1, [use_dbus_glib=yes], [use_dbus_glib=no])
# fi
# 
# if test "$use_dbus" = yes; then
#   AC_DEFINE(USE_DBUS_GLIB, 1, [Build dbus glib support])
# fi
# 
# AC_ARG_ENABLE(inotify,
#   [  --disable-inotify       Disable inotify plugin],
#   [use_inotify=$enableval], [use_inotify=yes])
# 
# if test "x$use_inotify" = "xyes"; then
#   AC_CHECK_HEADERS([sys/inotify.h], [use_inotify=yes], [use_inotify=no])
# fi
# 
# AM_CONDITIONAL(INOTIFY_PLUGIN, test "x$use_inotify" = "xyes")
# if test "$use_inotify" = yes; then
#   AC_DEFINE(USE_INOTIFY, 1, [Build inotify plugin])
# fi
# 
# AC_ARG_ENABLE(fuse,
#   [  --disable-fuse          Disable fuse plugin],
#   [use_fuse=$enableval], [use_fuse=yes])
# 
# if test "x$use_fuse" = "xyes"; then
#   PKG_CHECK_MODULES(FUSE, fuse, [use_fuse=yes], [use_fuse=no])
# fi
# 
# AM_CONDITIONAL(FUSE_PLUGIN, test "x$use_fuse" = "xyes")
# if test "$use_fuse" = yes; then
#   AC_DEFINE(USE_FUSE, 1, [Build fuse plugin])
# fi
# 
# AC_ARG_ENABLE(annotate,
#   [  --disable-annotate      Disable annotate plugin],
#   [use_annotate=$enableval], [use_annotate=yes])
# 
# if test "x$use_annotate" = "xyes"; then
#   PKG_CHECK_MODULES(ANNOTATE, cairo-xlib-xrender, [use_annotate=yes], [use_annotate=no])
# fi
# 
# AM_CONDITIONAL(ANNOTATE_PLUGIN, test "x$use_annotate" = "xyes")
# if test "$use_annotate" = yes; then
#   AC_DEFINE(USE_ANNOTATE, 1, [Build annotate plugin])
# fi
# 
# AC_ARG_ENABLE(librsvg,
#   [  --disable-librsvg       Disable svg support],
#   [use_librsvg=$enableval], [use_librsvg=yes])
# 
# if test "x$use_librsvg" = "xyes"; then
#   PKG_CHECK_MODULES(LIBRSVG, [cairo >= 1.0 librsvg-2.0 >= 2.14.0], [use_librsvg=yes], [use_librsvg=no])
# fi
# 
# AM_CONDITIONAL(USE_LIBRSVG, test "x$use_librsvg" = "xyes")
# if test "$use_librsvg" = yes; then
#   AC_DEFINE(USE_LIBRSVG, 1, [librsvg for SVG support])
# fi
# 

# AC_ARG_ENABLE(kde,
#   [  --disable-kde           Disable kde window decorator],
#   [use_kde=$enableval], [use_kde=yes])
# 
# if test "x$use_kde" = "xyes"; then
#   qt_incdirs="$QTINC /usr/local/qt/include /usr/include/qt /usr/include /usr/X11R6/include/X11/qt /usr/X11R6/include/qt /usr/lib/qt3/include /usr/lib/qt/include /usr/share/qt3/include"
#   qt_libdirs="$QTLIB /usr/local/qt/lib /usr/lib/qt /usr/lib /usr/X11R6/lib/X11/qt /usr/X11R6/lib/qt /usr/lib/qt3/lib /usr/lib/qt/lib /usr/share/qt3/lib"
# 
#   if test -n "$QTDIR" ; then
#     qt_incdirs="$QTDIR/include $qt_incdirs"
#     qt_libdirs="$QTDIR/lib $qt_libdirs"
#   fi
# 
#   qt_test_include="qstyle.h"
#   qt_test_library="libqt-mt.so"
# 
#   dnl Check for Qt headers
#   AC_MSG_CHECKING([for Qt headers])
#   qt_incdir="no"
#   for it in $qt_incdirs ; do
#     if test -r "$it/$qt_test_include" ; then
#       qt_incdir="$it"
#       break
#     fi
#   done
#   AC_MSG_RESULT([$qt_incdir])
# 
#   dnl Check for Qt libraries
#   AC_MSG_CHECKING([for Qt libraries])
#   qt_libdir="no"
#   for qt_check in $qt_libdirs ; do
#     if test -r "$qt_check/$qt_test_library" ; then
#       qt_libdir="$qt_check"
#       break
#     fi
#   done
#   AC_MSG_RESULT([$qt_libdir])
# 
#   use_kde=no;
#   if test "x$qt_libdir" != "xno" ; then
#     if test "x$qt_incdir" != "xno" ; then
#       use_kde=yes;
#     fi
#   fi
# 
#   QT_CXXFLAGS="-I$qt_incdir"
#   QT_LIBS="-L$qt_libdir"
# 
#   if test "x$use_kde" = xyes; then
#     kdedir=`kde-config --prefix --expandvars 2>/dev/null`
#     kdelibs=`kde-config --install lib --expandvars 2>/dev/null`
#     kdeincs=`kde-config --install include --expandvars 2>/dev/null`
#     if test -d "$kdelibs"; then
#       PKG_CHECK_MODULES(KDE_WINDOW_DECORATOR,
# 			dbus-1 xdamage xcomposite,
# 			[use_kde=yes], [use_kde=no])
# 
#       KDE_CFLAGS="$QT_CXXFLAGS -I$kdeincs"
#       KDE_LIBS="$QT_LIBS -L$kdelibs -lkdecore -lkdecorations -ldbus-qt-1"
# 
#       QT_MOC=$QTDIR/bin/moc
#       DCOPIDL=$kdedir/bin/dcopidl
#       DCOPIDL2CPP=$kdedir/bin/dcopidl2cpp
#     else
#       use_kde=no;
#     fi
#   fi
# fi
# 
# AM_CONDITIONAL(USE_KDE, test "x$use_kde" = "xyes")
# if test "$use_kde" = yes; then
#   AC_DEFINE(USE_KDE, 1, [Build kde window decorator])
# fi
# 
# AC_SUBST(KDE_CFLAGS)
# AC_SUBST(KDE_LIBS)
# AC_SUBST(QT_MOC)
# AC_SUBST(DCOPIDL)
# AC_SUBST(DCOPIDL2CPP)
# 
# AC_ARG_ENABLE(kde4,
#   [  --disable-kde4          Disable kde4 window decorator],
#   [use_kde4=$enableval], [use_kde4=yes])
# 
# if test "x$use_kde4" = "xyes"; then
#   
#   kde4dir=`kde4-config --prefix --expandvars 2>/dev/null`
#   kde4libs=`kde4-config --install lib --expandvars 2>/dev/null`
#   kde4incs=`kde4-config --install include --expandvars 2>/dev/null`
# 
#   kde4_test_includes="kdecoration.h kcommondecoration.h kdecorationbridge.h plasma/panelsvg.h"
# 
#   dnl Check for KWin headers
#   AC_MSG_CHECKING([for KWin headers])
# 
#   kde4_notfound=""
#   for file in $kde4_test_includes ; do
#     status=" $file"
#     for it in $kde4incs ; do
#       if test -r "$it/$file" ; then
#         status=""
#         break;
#       fi
#     done
#     kde4_notfound="$kde4_notfound$status"
#   done
#   if test -z "$kde4_notfound" ; then
#       kde4_notfound="yes"
#   else
#       kde4_notfound="$kde4_notfound not found"
#       kde4libs=""
#   fi
# 
#   AC_MSG_RESULT([$kde4_notfound])
# 
#   if test -d "$kde4libs"; then
#     PKG_CHECK_MODULES(KDE4_WINDOW_DECORATOR,
# 		      dbus-1 xdamage xcomposite QtCore >= 4.0.0 QtGui QtDBus,
# 			[use_kde4=yes], [use_kde4=no])
# 
#     KDE4_CFLAGS="-I$kde4incs"
#     KDE4_LIBS="-L$kde4libs -lkdecore -lkdecorations -lplasma"
# 
#     QT4_MOC=`pkg-config QtGui --variable=moc_location`
#     QDBUSXML2CPP=`pkg-config QtGui --variable=exec_prefix`"/bin/qdbusxml2cpp"
#   else
#     use_kde4=no;
#   fi
# fi
# 
# AM_CONDITIONAL(USE_KDE4, test "x$use_kde4" = "xyes")
# if test "$use_kde4" = yes; then
#   AC_DEFINE(USE_KDE4, 1, [Build kde4 window decorator])
# fi
# 
# AC_SUBST(KDE4_CFLAGS)
# AC_SUBST(KDE4_LIBS)
# AC_SUBST(QT4_MOC)
# AC_SUBST(QDBUSXML2CPP)
# 
# AC_ARG_ENABLE(kconfig,
#   [  --disable-kconfig       Disable kconfig plugin],
#   [use_kconfig=$enableval], [use_kconfig=yes])
# 
# if test "x$use_kde" != "xyes"; then
#   use_kconfig=no;
# fi
# 
# if test "x$use_kconfig" = "xyes"; then
#   KCONFIG_CFLAGS=$KDE_CFLAGS
#   KCONFIG_LIBS=$KDE_LIBS
#   KDE_KCFG_DIR=`kde-config --install kcfg --expandvars 2>/dev/null`
#   KDE_KCONFIG_DIR=`kde-config --install config --expandvars 2>/dev/null`
# fi
# 
# AM_CONDITIONAL(USE_KCONFIG, test "x$use_kconfig" = "xyes")
# if test "$use_kconfig" = yes; then
#   AC_DEFINE(USE_KCONFIG, 1, [Build kconfig plugin])
# fi
# 
# AC_SUBST(KCONFIG_CFLAGS)
# AC_SUBST(KCONFIG_LIBS)
# AC_SUBST(KDE_KCFG_DIR)
# AC_SUBST(KDE_KCONFIG_DIR)
# 
# kcfgdir=$KDE_KCFG_DIR
# AC_SUBST(kcfgdir)
# 
# AC_ARG_WITH(max-desktops,
#   [  --with-max-desktops     Max reasonable desktops],
#   [MAX_DESKTOPS=$withval],
#   [MAX_DESKTOPS=36])
# 
# if test -n "$MAX_DESKTOPS"; then
#   AC_DEFINE_UNQUOTED(MAX_DESKTOPS, $MAX_DESKTOPS, [Max reasonable desktops])
# fi
# 
# AC_CONFIG_COMMANDS([include/compiz-common.h],
# [
# 	commonfile=include/compiz-common.h
# 	outfile=$commonfile.tmp
# 	AC_MSG_NOTICE([creating $commonfile])
# 	cat > $outfile <<_EOF
# #ifndef COMPIZ_COMMON_H
# #define COMPIZ_COMMON_H
# 
# #ifdef  __cplusplus
# # define COMPIZ_BEGIN_DECLS  extern "C" {
# # define COMPIZ_END_DECLS    }
# #else
# # define COMPIZ_BEGIN_DECLS
# # define COMPIZ_END_DECLS
# #endif
# 
# #define COMPIZ_VERSION_MAJOR $COMPIZ_VERSION_MAJOR
# #define COMPIZ_VERSION_MINOR $COMPIZ_VERSION_MINOR
# #define COMPIZ_VERSION_MICRO $COMPIZ_VERSION_MICRO
# 
# #define COMPIZ_VERSION_STRING "$COMPIZ_VERSION_MAJOR.$COMPIZ_VERSION_MINOR.$COMPIZ_VERSION_MICRO"
# 
# _EOF
# 	echo '#endif' >> $outfile
# 
# 	if cmp -s $outfile $commonfile; then
# 	  AC_MSG_NOTICE([$commonfile is unchanged])
# 	  rm -f $outfile
# 	else
# 	  mv $outfile $commonfile
# 	fi
# ],[
# 	COMPIZ_VERSION_MAJOR=$COMPIZ_VERSION_MAJOR
# 	COMPIZ_VERSION_MINOR=$COMPIZ_VERSION_MINOR
# 	COMPIZ_VERSION_MICRO=$COMPIZ_VERSION_MICRO
# ])
# 
# AC_OUTPUT([
# compiz.pc
# compiz-cube.pc
# compiz-scale.pc
# compiz-gconf.pc
# compiz-kconfig.pc
# Makefile
# src/Makefile
# libdecoration/Makefile
# libdecoration/libdecoration.pc
# include/Makefile
# plugins/Makefile
# images/Makefile
# gtk/Makefile
# gtk/window-decorator/Makefile
# gtk/gnome/Makefile
# kde/Makefile
# kde/window-decorator/Makefile
# kde/window-decorator-kde4/Makefile
# po/Makefile.in
# metadata/Makefile
# plugins/composite/Makefile
# plugins/opengl/Makefile
# ])
# 
# echo ""
# echo "the following optional plugins will be compiled:"
# echo "  glib:     $use_glib"
# echo "  gconf:    $use_gconf"
# echo "  kconfig:  $use_kconfig"
# echo "  dbus:     $use_dbus"
# echo "  annotate: $use_annotate"
# echo "  svg:      $use_librsvg"
# echo "  inotify:  $use_inotify"
# echo "  fuse:     $use_fuse"
# echo ""
# echo "and the following optional features will be compiled:"
# echo "  gtk:      $use_gtk"
# echo "  metacity: $use_metacity"
# echo "  gnome:    $use_gnome"
# echo "  kde:      $use_kde"
# echo "  kde4:     $use_kde4"
# echo ""
