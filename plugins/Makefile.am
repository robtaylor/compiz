SUBDIRS = composite opengl

# libfade_la_LDFLAGS = -module -avoid-version -no-undefined
# libfade_la_SOURCES = fade.cpp
# 
# libcube_la_LDFLAGS = -module -avoid-version -no-undefined
# libcube_la_SOURCES = cube.cpp
# 
# librotate_la_LDFLAGS = -module -avoid-version -no-undefined
# librotate_la_SOURCES = rotate.cpp

libzoom_la_LDFLAGS = -module -avoid-version -no-undefined
libzoom_la_SOURCES = zoom.cpp

# libscale_la_LDFLAGS = -module -avoid-version -no-undefined
# libscale_la_SOURCES = scale.cpp
# 
# libwobbly_la_LDFLAGS = -module -avoid-version -no-undefined
# libwobbly_la_SOURCES = wobbly.cpp
# 
# libminimize_la_LDFLAGS = -module -avoid-version -no-undefined
# libminimize_la_SOURCES = minimize.cpp

libmove_la_DEPENDENCIES = \
	$(top_builddir)/plugins/composite/libcomposite.la \
	$(top_builddir)/plugins/opengl/libopengl.la

libmove_la_LDFLAGS = -module -avoid-version -no-undefined
libmove_la_LIBADD = \
	$(top_builddir)/plugins/composite/libcomposite.la \
	$(top_builddir)/plugins/opengl/libopengl.la
libmove_la_SOURCES = move.cpp

libresize_la_DEPENDENCIES = \
	$(top_builddir)/plugins/composite/libcomposite.la \
	$(top_builddir)/plugins/opengl/libopengl.la

libresize_la_LDFLAGS = -module -avoid-version -no-undefined
libresize_la_LIBADD = \
	$(top_builddir)/plugins/composite/libcomposite.la \
	$(top_builddir)/plugins/opengl/libopengl.la
libresize_la_SOURCES = resize.cpp

# libplace_la_LDFLAGS = -module -avoid-version -no-undefined
# libplace_la_SOURCES = place.cpp

libdecor_la_DEPENDENCIES = \
	$(top_builddir)/libdecoration/libdecoration.la \
	$(top_builddir)/plugins/composite/libcomposite.la \
	$(top_builddir)/plugins/opengl/libopengl.la

libdecor_la_LDFLAGS = -module -avoid-version -no-undefined
libdecor_la_LIBADD = \
	$(top_builddir)/libdecoration/libdecoration.la \
	$(top_builddir)/plugins/composite/libcomposite.la \
	$(top_builddir)/plugins/opengl/libopengl.la

libdecor_la_SOURCES = decor.cpp

# libswitcher_la_LDFLAGS = -module -avoid-version -no-undefined
# libswitcher_la_SOURCES = switcher.cpp
# 
# libwater_la_LDFLAGS = -module -avoid-version -no-undefined
# libwater_la_SOURCES = water.cpp
# 
# libscreenshot_la_LDFLAGS = -module -avoid-version -no-undefined
# libscreenshot_la_SOURCES = screenshot.cpp
# 
# libclone_la_LDFLAGS = -module -avoid-version -no-undefined
# libclone_la_SOURCES = clone.cpp
# 
# libpng_la_LDFLAGS = -module -avoid-version -no-undefined
# libpng_la_LIBADD = @LIBPNG_LIBS@
# libpng_la_SOURCES = png.cpp
# 
# libblur_la_DEPENDENCIES = $(top_builddir)/libdecoration/libdecoration.la
# libblur_la_LDFLAGS = -module -avoid-version -no-undefined
# libblur_la_LIBADD = $(top_builddir)/libdecoration/libdecoration.la -lGLU
# libblur_la_SOURCES = blur.cpp
# 
# libregex_la_LDFLAGS = -module -avoid-version -no-undefined
# libregex_la_SOURCES = regex.cpp
# 
# libvideo_la_DEPENDENCIES = $(top_builddir)/libdecoration/libdecoration.la
# libvideo_la_LDFLAGS = -module -avoid-version -no-undefined
# libvideo_la_LIBADD = $(top_builddir)/libdecoration/libdecoration.la
# libvideo_la_SOURCES = video.cpp
# 
# libini_la_LDFLAGS = -module -avoid-version -no-undefined
# libini_la_SOURCES = ini.cpp
# 
# if USE_LIBRSVG
# libsvg_la_DEPENDENCIES = $(top_builddir)/libdecoration/libdecoration.la
# libsvg_la_LDFLAGS = -module -avoid-version -no-undefined
# libsvg_la_LIBADD =				       \
# 	$(top_builddir)/libdecoration/libdecoration.la \
# 	@LIBRSVG_LIBS@
# libsvg_la_SOURCES = svg.cpp
# libsvg_module = libsvg.la
# endif
# 
# if ANNOTATE_PLUGIN
# libannotate_la_LDFLAGS = -module -avoid-version -no-undefined
# libannotate_la_LIBADD = @ANNOTATE_LIBS@
# libannotate_la_SOURCES = annotate.cpp
# libannotate_module = libannotate.la
# endif

if USE_GLIB
libglib_la_LDFLAGS = -module -avoid-version -no-undefined
libglib_la_LIBADD = @GLIB_LIBS@
libglib_la_SOURCES = glib.cpp
libglib_module = libglib.la
endif

if USE_GCONF
libgconf_la_LDFLAGS = -module -avoid-version -no-undefined
libgconf_la_LIBADD = @GCONF_LIBS@
libgconf_la_SOURCES = gconf.cpp
libgconf_module = libgconf.la
endif

# if USE_KCONFIG
# libkconfig_la_LDFLAGS = -module -avoid-version -no-undefined
# libkconfig_la_LIBADD = @KCONFIG_LIBS@
# libkconfig_la_SOURCES = kconfig.cpp
# libkconfig_module = libkconfig.la
# endif
# 
# if DBUS_PLUGIN
# libdbus_la_LDFLAGS = -module -avoid-version -no-undefined
# libdbus_la_LIBADD = @DBUS_LIBS@
# libdbus_la_SOURCES = dbus.cpp
# libdbus_module = libdbus.la
# endif
# 
# if INOTIFY_PLUGIN
# libinotify_la_LDFLAGS = -module -avoid-version -no-undefined
# libinotify_la_SOURCES = inotify.cpp
# libinotify_module = libinotify.la
# endif
# 
# if FUSE_PLUGIN
# libfs_la_LDFLAGS = -module -avoid-version -no-undefined
# libfs_la_LIBADD = @FUSE_LIBS@
# libfs_la_SOURCES = fuse.cpp
# libfs_module = libfs.la
# endif

INCLUDES =					\
	@COMPIZ_CFLAGS@				\
	@LIBPNG_CFLAGS@				\
	@LIBRSVG_CFLAGS@			\
	@ANNOTATE_CFLAGS@			\
	@GCONF_CFLAGS@				\
	@KCONFIG_CFLAGS@			\
	@DBUS_CFLAGS@				\
	@GLIB_CFLAGS@				\
	@FUSE_CFLAGS@				\
	-DFUSE_USE_VERSION=26			\
	-DALL_LINGUAS="\"@ALL_LINGUAS@\""	\
	-DLOCALEDIR="\"@datadir@/locale\""	\
	-DPLUGINDIR=\"$(plugindir)\"		\
	-DIMAGEDIR=\"$(imagedir)\"		\
	-I$(top_srcdir)/include                 \
	-I$(top_builddir)/include               \
	-DMETADATADIR=\"$(metadatadir)\"

moduledir = $(plugindir)

module_LTLIBRARIES =		\
	libmove.la		\
	libdecor.la		\
	libresize.la


#module_LTLIBRARIES =		\
#	$(libglib_module)	\
#	$(libgconf_module)	\
#	$(libkconfig_module)	\
#	libdecoration.la	\
#	libzoom.la		\
#	libmove.la		\
#	libresize.la		\
# 	libwobbly.la		\
# 	libfade.la		\
# 	libminimize.la		\
# 	libcube.la		\
# 	librotate.la		\
# 	libscale.la		\
# 	libplace.la             \
# 	$(libdbus_module)	\
# 	libswitcher.la		\
# 	libwater.la		\
# 	libscreenshot.la	\
# 	libclone.la		\
# 	libpng.la		\
# 	libblur.la		\
# 	libregex.la		\
# 	libvideo.la		\
# 	libini.la		\
# 	$(libsvg_module)	\
# 	$(libannotate_module)	\
# 	$(libinotify_module)	\
# 	$(libfs_module)

