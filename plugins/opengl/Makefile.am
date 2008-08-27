libopengl_la_LDFLAGS = -avoid-version -no-undefined
libopengl_la_LIBADD = $(top_builddir)/plugins/composite/libcomposite.la
libopengl_la_SOURCES = \
	opengl.cpp \
	display.cpp \
	screen.cpp \
	window.cpp \
	paint.cpp \
	matrix.cpp \
	vector.cpp \
	texture.cpp \
	fragment.cpp

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
	libopengl.la
