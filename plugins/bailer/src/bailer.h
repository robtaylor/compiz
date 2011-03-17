/**
 * Compiz bailer plugin
 *
 * bailer.h
 *
 * Copyright (c) 2010 Canonical Ltd
 *
 * This plugin should be thought of as a "manager" plugin for compiz
 * it handles signals by various other plugins (eg handleCompizEvent)
 * and then takes decisive action on those signals when appropriate
 * based on the configuration.
 *
 * For example, it will handle signals from plugins that someone is
 * doing something stupid and terminate the offending plugin,
 * or handle signals that performance is poor, wait a little while
 * and then take decisive action to restore the user experience.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * AUTHORS: Didier Roche <didrocks@ubuntu.com>
 * 	    Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#include <core/core.h>
#include <core/pluginclasshandler.h>
#include <core/timer.h>

#include "bailer_options.h"

class BailerScreen :
    public PluginClassHandler <BailerScreen, CompScreen>,
    public ScreenInterface,
    public BailerOptions
{
    public:

	BailerScreen (CompScreen *);

	void
	handleCompizEvent (const char *, const char *,
			   CompOption::Vector &options);

    private:

        typedef enum
	{
	    SessionType2D = 0,
	    SessionType3DNoARB,
	    SessionType3DNoComplex,
	    SessionType3DFull
	} SessionType;

	int mPoorPerformanceCount;
	CompTimer mSafeUnloadTimer;

	CompString detectFallbackWM ();
	void ensureShell ();

	void doFatalFallback ();
	void doPerformanceFallback ();

	void changeSessionType (SessionType);

	void unloadPlugins (CompString *);
	void unloadPlugins (std::vector <CompString> );
	bool doUnload (std::vector <CompString>);
};

#define BAILER_SCREEN(s)					       \
    BailerScreen *bs = BailerScreen::get (s);

class BailerPluginVTable :
    public CompPlugin::VTableForScreen <BailerScreen>
{
    public:

	bool init ();
};
