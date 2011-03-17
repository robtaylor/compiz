/* Copyright */

#include <core/core.h>
#include <composite/composite.h>
#include <opengl/opengl.h>
#include "debugspew_options.h"

class SpewWindow :
    public PluginClassHandler <SpewWindow, CompWindow>
{
    public:

	SpewWindow (CompWindow *w);

    public:

	CompWindow	*w;
	CompositeWindow *cWindow;
	GLWindow	*gWindow;

	void
	spew (CompString &f);
};

class SpewScreen :
    public PluginClassHandler <SpewScreen, CompScreen>,
    public ScreenInterface,
    public DebugspewOptions
{
    public:

	SpewScreen (CompScreen *);

	CompositeScreen *cScreen;
	GLScreen	*gScreen;

    public:

	void
	handleCompizEvent (const char *, const char *, CompOption::Vector &);

	bool
	spew ();

	void
	spewScreenEdge (CompString &f, unsigned int i);
};

class SpewPluginVTable :
    public CompPlugin::VTableForScreenAndWindow <SpewScreen, SpewWindow>
{
    public:

	bool init ();
};
