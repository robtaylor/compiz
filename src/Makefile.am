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
        region.cpp \
	atoms.cpp \
	timer.cpp \
	main.cpp \
	actions.cpp \
	screen.cpp   \
	window.cpp   \
	metadata.cpp \
	action.cpp \
	option.cpp \
	string.cpp \
	match.cpp    \
	privates.cpp   \
	event.cpp    \
	plugin.cpp   \
	session.cpp  \
	output.cpp   \
	rect.cpp     \
	size.cpp     \
	point.cpp    \
	windowgeometry.cpp \
	icon.cpp
