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
	main.cpp	   \
	privates.cpp \
	object.cpp   \
	core.cpp	   \
	display.cpp  \
	screen.cpp   \
	window.cpp   \
	event.cpp    \
	texture.cpp  \
	paint.cpp	   \
	option.cpp   \
	plugin.cpp   \
	session.cpp  \
	fragment.cpp \
	matrix.cpp   \
	cursor.cpp   \
	match.cpp    \
	metadata.cpp \
	output.cpp
