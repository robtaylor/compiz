INCLUDES =			     \
	@COMPIZ_CFLAGS@		     \
	@GL_CFLAGS@		     \
	-I$(top_srcdir)/include	     \
	-I$(top_builddir)/include    \
	-DPLUGINDIR=\"$(plugindir)\" \
	-DIMAGEDIR=\"$(imagedir)\"   \
	-DMETADATADIR=\"$(metadatadir)\"

bin_PROGRAMS = compiz

compiz_LDADD = @COMPIZ_LIBS@ @GL_LIBS@ -lm
compiz_LDFLAGS = -export-dynamic
compiz_SOURCES =   \
	action.cpp \
	option.cpp \
	string.cpp \
	fragment.cpp \
	match.cpp    \
	main.cpp	   \
	object.cpp   \
	core.cpp	   \
	display.cpp  \
	screen.cpp   \
	window.cpp   \
	event.cpp    \
	texture.cpp  \
	paint.cpp	   \
	plugin.cpp   \
	session.cpp  \
	matrix.cpp   \
	metadata.cpp \
	output.cpp   \
	rect.cpp     \
	size.cpp     \
	point.cpp    \
	windowgeometry.cpp \
	icon.cpp     \
	vector.cpp
