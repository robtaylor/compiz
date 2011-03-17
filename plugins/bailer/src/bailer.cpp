/**
 * Compiz bailer plugin
 *
 * bailer.cpp
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

#include "bailer.h"

COMPIZ_PLUGIN_20090315 (bailer, BailerPluginVTable);

static CompString reducedFunctionalityUnload[] = {"opengl", "composite"};
static CompString compizShellUnload[] = {"unityshell"};
static CompString noARBUnload[] = {"colorfilter", "blur", "bicubic", "water", "reflex"};
static CompString noComplexUnload[] = {"blur", "water"};

/*
 * BailerScreen::detectFallbackWM ()
 *
 * Reads the session environment propeties and tries to detect
 * which fallback window manager would be appropriate to launch here
 *
 * returns: a CompString with the fallback wm command
 *
 */
CompString
BailerScreen::detectFallbackWM ()
{
    if (getenv ("KDE_FULL_SESSION") != NULL)
	return "kwin --replace";
    else if (getenv ("GNOME_DESKTOP_SESSION_ID") != NULL)
	return "metacity --replace";
    else if (access ("/usr/bin/xfwm4", F_OK) == 0)
	return "xfwm4 --replace";

    return "";
}

/*
 * BailerScreen::ensureShell
 *
 * Because compiz might be providing the panel shell or whatever
 * (eg, Ubuntu's Unity plugin), we need to ensure that our desktop
 * services come back when compiz goes away or when the Compiz
 * shell(s) can't run.
 *
 */
void
BailerScreen::ensureShell ()
{

    CompString alternative_shell = optionGetCustomAlternativeShell();
    compLogMessage ("bailer",
			        CompLogLevelInfo,
			        "Ensuring a shell for your session");

    /* FIXME: will be nicer to get the detection module at start and so, not loading plugin rather
                   than unloading them, isn't it? */    
    unloadPlugins (compizShellUnload);    
    
    if (strcmp (alternative_shell.c_str (), "") != 0)
    {
	compLogMessage ("bailer",
			            CompLogLevelInfo,
			            "Custom shell set: no detection magic: %s", alternative_shell.c_str ());
	screen->runCommand (alternative_shell.c_str ());
	return;
    }
    
    if (getenv ("GDMSESSION") != NULL && (strcmp (getenv ("GDMSESSION"), "gnome") == 0)) {
	screen->runCommand ("gnome-panel");
    }
}

/*
 * BailerScreen::doUnload
 *
 * Unload given plugins at the top of the main loop
 *
 */
bool
BailerScreen::doUnload (std::vector <CompString> plugins)
{
    foreach (CompString &plugin, plugins)
    {
	CompPlugin *p = CompPlugin::find (plugin.c_str ());

	if (p)
	    (*loaderUnloadPlugin) (p);
    }

    return false;
}

/*
 * BailerScreen::unloadPlugins
 *
 * Add plugins to the unload timer so that we don't unload stuff
 * during wrapped function calls
 *
 */
void
BailerScreen::unloadPlugins (CompString *plugins)
{
    std::vector <CompString> pv (plugins, plugins + sizeof (plugins) / sizeof (*plugins));

    mSafeUnloadTimer.stop ();
    mSafeUnloadTimer.setCallback (boost::bind (&BailerScreen::doUnload, this, pv));
    mSafeUnloadTimer.start ();
}

void
BailerScreen::unloadPlugins (std::vector <CompString> plugins)
{
    mSafeUnloadTimer.stop ();
    mSafeUnloadTimer.setCallback (boost::bind (&BailerScreen::doUnload, this, plugins));
    mSafeUnloadTimer.start ();
}


/*
 * BailerScreen::doFallback ()
 *
 * Do the actual fallback if a plugin asks for it
 *
 */
void
BailerScreen::doFatalFallback ()
{
    switch (optionGetFatalFallbackMode ())
    {
	case BailerOptions::FatalFallbackModeReducedFunctionalityMode:
	    unloadPlugins (reducedFunctionalityUnload); break;
	case BailerOptions::FatalFallbackModeDetectSessionFallback:
	    ensureShell ();
	    screen->runCommand (detectFallbackWM ().c_str ());
	    exit (EXIT_FAILURE);
	    break;
	case BailerOptions::FatalFallbackModeExecuteCustomFallback:
	    ensureShell ();
	    screen->runCommand (optionGetCustomFallbackWindowManager ().c_str ());
	    exit (EXIT_FAILURE);
	case BailerOptions::FatalFallbackModeNoAction:
	default:
	    break;
    }
}

/*
 * BailerScreen::doPerformanceFallback
 *
 * Not as bad as a fatal fallback, but try to recover from bad
 * performance if a plugin thinks we are not doing so well.
 *
 */
void
BailerScreen::doPerformanceFallback ()
{
    switch (optionGetPoorPerformanceFallback ())
    {
	case BailerScreen::PoorPerformanceFallbackLaunchFatalFallback:
	    doFatalFallback ();
	    break;
	case BailerScreen::PoorPerformanceFallbackUnloadACustomListOfPlugins:
	{
	    CompOption::Value::Vector vv = optionGetBadPlugins ();
	    std::vector <CompString> pv;

	    foreach (CompOption::Value &v, vv)
		pv.push_back (v.s ());

	    unloadPlugins (pv);

	    break;
	}
	case BailerScreen::PoorPerformanceFallbackReducedFunctionalityMode:
	    unloadPlugins (reducedFunctionalityUnload); break;
	case BailerScreen::PoorPerformanceFallbackNoAction:
	default:
	    break;
    }
}

/*
 * BailerScreen::changeSessionType
 *
 * Unloads and loads plugins depending on what kind of session we wanted
 *
 */
void
BailerScreen::changeSessionType (SessionType type)
{
    switch (type)
    {
	case SessionType2D:
	    unloadPlugins (reducedFunctionalityUnload);
	    break;
	case SessionType3DNoARB:
	    unloadPlugins (noARBUnload);
	    break;
	case SessionType3DNoComplex:
	    unloadPlugins (noComplexUnload);
	    break;
	case SessionType3DFull:
	default:
	    break;
    }
}

/*
 * BailerScreen::handleCompizEvent
 *
 * Checks the compiz event screen if some plugin is screaming for help
 *
 */
void
BailerScreen::handleCompizEvent (const char *plugin, const char *event,
				 CompOption::Vector &options)
{
    if (strcmp (plugin, "composite") == 0)
    {
	if (strcmp (event, "poor_performance") == 0)
	{
	    mPoorPerformanceCount++;
	    if (mPoorPerformanceCount > optionGetBadPerformanceThreshold ())
		doPerformanceFallback ();
	}
    }

    if (strcmp (event, "fatal_fallback") == 0)
	doFatalFallback ();

    if (strcmp (event, "ensure_shell") == 0)
	ensureShell ();

    screen->handleCompizEvent (plugin, event, options);
}

BailerScreen::BailerScreen (CompScreen *s) :
    PluginClassHandler <BailerScreen, CompScreen> (s),
    mPoorPerformanceCount (0)
{
    ScreenInterface::setHandler (s);
}

bool
BailerPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	return false;

    return true;
}
