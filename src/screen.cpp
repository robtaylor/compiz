/*
 * Copyright © 2005 Novell, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Novell, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Novell, Inc. makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * NOVELL, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL NOVELL, INC. BE LIABLE FOR ANY SPECI<<<<<fAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: David Reveman <davidr@novell.com>
 */

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <dlfcn.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>
#include <limits.h>
#include <poll.h>
#include <algorithm>

#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/shape.h>
#include <X11/cursorfont.h>


#include <core/core.h>

#include <core/screen.h>
#include <core/icon.h>
#include <core/atoms.h>
#include "privatescreen.h"
#include "privatewindow.h"

static unsigned int virtualModMask[] = {
    CompAltMask, CompMetaMask, CompSuperMask, CompHyperMask,
    CompModeSwitchMask, CompNumLockMask, CompScrollLockMask
};

bool inHandleEvent = false;

CompScreen *targetScreen = NULL;
CompOutput *targetOutput;

int lastPointerX = 0;
int lastPointerY = 0;
int pointerX     = 0;
int pointerY     = 0;

#define MwmHintsFunctions   (1L << 0)
#define MwmHintsDecorations (1L << 1)
#define PropMotifWmHintElements 3

typedef struct {
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
} MwmHints;



CompScreen *screen;

#define NUM_OPTIONS(s) (sizeof ((s)->priv->opt) / sizeof (CompOption))

CompPrivateStorage::Indices screenPrivateIndices (0);

int
CompScreen::allocPrivateIndex ()
{
    int i = CompPrivateStorage::allocatePrivateIndex (&screenPrivateIndices);
    if (screenPrivateIndices.size () != screen->privates.size ())
	screen->privates.resize (screenPrivateIndices.size ());
    return i;
}

void
CompScreen::freePrivateIndex (int index)
{
    CompPrivateStorage::freePrivateIndex (&screenPrivateIndices, index);
    if (screenPrivateIndices.size () != screen->privates.size ())
	screen->privates.resize (screenPrivateIndices.size ());
}



#define TIMEVALDIFF(tv1, tv2)						   \
    ((tv1)->tv_sec == (tv2)->tv_sec || (tv1)->tv_usec >= (tv2)->tv_usec) ? \
    ((((tv1)->tv_sec - (tv2)->tv_sec) * 1000000) +			   \
     ((tv1)->tv_usec - (tv2)->tv_usec)) / 1000 :			   \
    ((((tv1)->tv_sec - 1 - (tv2)->tv_sec) * 1000000) +			   \
     (1000000 + (tv1)->tv_usec - (tv2)->tv_usec)) / 1000


void
CompScreen::eventLoop ()
{
    struct timeval    tv;
    CompTimer         *t;
    int               time;
    CompWatchFdHandle watchFdHandle;

    watchFdHandle = addWatchFd (ConnectionNumber (priv->dpy), POLLIN, NULL);

    for (;;)
    {
	if (restartSignal || shutDown)
	    break;

	priv->processEvents ();

	if (!priv->timers.empty())
	{
	    gettimeofday (&tv, 0);
	    priv->handleTimers (&tv);

	    if (priv->timers.front()->mMinLeft > 0)
	    {
		std::list<CompTimer *>::iterator it = priv->timers.begin();

		t = (*it);
		time = t->mMaxLeft;
		while (it != priv->timers.end())
		{
		    t = (*it);
		    if (t->mMinLeft <= time)
			break;
		    if (t->mMaxLeft < time)
			time = t->mMaxLeft;
		    it++;
		}
		priv->doPoll (time);
		gettimeofday (&tv, 0);
		priv->handleTimers (&tv);
	    }
	}
	else
	{
	    priv->doPoll (-1);
	}
    }

    removeWatchFd (watchFdHandle);
}

CompFileWatchHandle
CompScreen::addFileWatch (const char        *path,
			  int               mask,
			  FileWatchCallBack callBack)
{
    CompFileWatch *fileWatch = new CompFileWatch();
    if (!fileWatch)
	return 0;

    fileWatch->path	= strdup (path);
    fileWatch->mask	= mask;
    fileWatch->callBack = callBack;
    fileWatch->handle   = priv->lastFileWatchHandle++;

    if (priv->lastFileWatchHandle == MAXSHORT)
	priv->lastFileWatchHandle = 1;

    priv->fileWatch.push_front(fileWatch);

    fileWatchAdded (fileWatch);

    return fileWatch->handle;
}

void
CompScreen::removeFileWatch (CompFileWatchHandle handle)
{
    std::list<CompFileWatch *>::iterator it;
    CompFileWatch                        *w;

    for (it = priv->fileWatch.begin(); it != priv->fileWatch.end(); it++)
	if ((*it)->handle == handle)
	    break;

    if (it == priv->fileWatch.end())
	return;

    w = (*it);
    priv->fileWatch.erase (it);

    fileWatchRemoved (w);

    delete w;
}

void
PrivateScreen::addTimer (CompTimer *timer)
{
    std::list<CompTimer *>::iterator it;

    it = std::find (timers.begin (), timers.end (), timer);

    if (it != timers.end ())
	return;

    for (it = timers.begin(); it != timers.end(); it++)
    {
	if ((int) timer->mMinTime < (*it)->mMinLeft)
	    break;
    }

    timer->mMinLeft = timer->mMinTime;
    timer->mMaxLeft = timer->mMaxTime;

    timers.insert (it, timer);
}

void
PrivateScreen::removeTimer (CompTimer *timer)
{
    std::list<CompTimer *>::iterator it;

    it = std::find (timers.begin (), timers.end (), timer);

    if (it == timers.end ())
	return;

    timers.erase (it);
}

CompWatchFdHandle
CompScreen::addWatchFd (int             fd,
			short int       events,
			FdWatchCallBack callBack)
{
    CompWatchFd *watchFd = new CompWatchFd();

    if (!watchFd)
	return 0;

    watchFd->fd	      = fd;
    watchFd->callBack = callBack;
    watchFd->handle   = priv->lastWatchFdHandle++;

    if (priv->lastWatchFdHandle == MAXSHORT)
	priv->lastWatchFdHandle = 1;

    priv->watchFds.push_front (watchFd);

    priv->nWatchFds++;

    priv->watchPollFds = (struct pollfd *) realloc (priv->watchPollFds,
			  priv->nWatchFds * sizeof (struct pollfd));

    priv->watchPollFds[priv->nWatchFds - 1].fd     = fd;
    priv->watchPollFds[priv->nWatchFds - 1].events = events;

    return watchFd->handle;
}

void
CompScreen::removeWatchFd (CompWatchFdHandle handle)
{
    std::list<CompWatchFd *>::iterator it;
    CompWatchFd                        *w;
    int                                i;

    for (it = priv->watchFds.begin(), i = priv->nWatchFds - 1;
	 it != priv->watchFds.end(); it++, i--)
	if ((*it)->handle == handle)
	    break;

    if (it == priv->watchFds.end())
	return;

    w = (*it);
    priv->watchFds.erase (it);

    priv->nWatchFds--;

    if (i < priv->nWatchFds)
	memmove (&priv->watchPollFds[i], &priv->watchPollFds[i + 1],
		 (priv->nWatchFds - i) * sizeof (struct pollfd));

    delete w;
}

void
CompScreen::storeValue (CompString key, CompPrivate value)
{
    std::map<CompString,CompPrivate>::iterator it;
    it = priv->valueMap.find (key);
    if (it != priv->valueMap.end ())
    {
	it->second = value;
    }
    else
    {
	priv->valueMap.insert (std::pair<CompString,CompPrivate> (key, value));
    }
}

bool
CompScreen::hasValue (CompString key)
{
    return (priv->valueMap.find (key) != priv->valueMap.end ());
}

CompPrivate
CompScreen::getValue (CompString key)
{
    CompPrivate p;

    std::map<CompString,CompPrivate>::iterator it;
    it = priv->valueMap.find (key);

    if (it != priv->valueMap.end ())
    {
	return it->second;
    }
    else
    {
	p.uval = 0;
	return p;
    }
}

void
CompScreen::eraseValue (CompString key)
{
    std::map<CompString,CompPrivate>::iterator it;
    it = priv->valueMap.find (key);

    if (it != priv->valueMap.end ())
    {
	priv->valueMap.erase (key);
    }
}

short int
PrivateScreen::watchFdEvents (CompWatchFdHandle handle)
{
    std::list<CompWatchFd *>::iterator it;
    int                                i;

    for (it = watchFds.begin(), i = nWatchFds - 1; it != watchFds.end();
	 it++, i--)
	if ((*it)->handle == handle)
	    return watchPollFds[i].revents;

    return 0;
}

int
PrivateScreen::doPoll (int timeout)
{
    int rv;

    rv = poll (watchPollFds, nWatchFds, timeout);
    if (rv)
    {
	std::list<CompWatchFd *>::iterator it;
	int                                i;

	for (it = watchFds.begin(), i = nWatchFds - 1; it != watchFds.end();
	    it++, i--)
	    if (watchPollFds[i].revents != 0 && (*it)->callBack)
		(*it)->callBack ();
    }

    return rv;
}

void
PrivateScreen::handleTimers (struct timeval *tv)
{
    CompTimer                        *t;
    int		                     timeDiff;
    std::list<CompTimer *>::iterator it;

    timeDiff = TIMEVALDIFF (tv, &lastTimeout);

    /* handle clock rollback */
    if (timeDiff < 0)
	timeDiff = 0;

    for (it = timers.begin(); it != timers.end(); it++)
    {
	t = (*it);
	t->mMinLeft -= timeDiff;
	t->mMaxLeft -= timeDiff;
    }

    while (timers.begin() != timers.end() &&
	   (*timers.begin())->mMinLeft <= 0)
    {
	t = (*timers.begin());
	timers.pop_front();

	t->mActive = false;
	if (t->mCallBack ())
	{
	    addTimer (t);
	    t->mActive = true;
	}
    }

    lastTimeout = *tv;
}


void
CompScreen::fileWatchAdded (CompFileWatch *watch)
    WRAPABLE_HND_FUNC(0, fileWatchAdded, watch)

void
CompScreen::fileWatchRemoved (CompFileWatch *watch)
    WRAPABLE_HND_FUNC(1, fileWatchRemoved, watch)
	
bool
CompScreen::setOptionForPlugin (const char        *plugin,
				const char        *name,
				CompOption::Value &value)
{
    WRAPABLE_HND_FUNC_RETURN(4, bool, setOptionForPlugin,
			     plugin, name, value)

    CompPlugin *p = CompPlugin::find (plugin);
    if (p)
	return p->vTable->setOption (name, value);

    return false;
}

void
CompScreen::sessionEvent (CompSession::Event event,
			CompOption::Vector &arguments)
    WRAPABLE_HND_FUNC(5, sessionEvent, event, arguments)

void
ScreenInterface::fileWatchAdded (CompFileWatch *watch)
    WRAPABLE_DEF (fileWatchAdded, watch)

void
ScreenInterface::fileWatchRemoved (CompFileWatch *watch)
    WRAPABLE_DEF (fileWatchRemoved, watch)

bool
ScreenInterface::initPluginForScreen (CompPlugin *plugin)
    WRAPABLE_DEF (initPluginForScreen, plugin)

void
ScreenInterface::finiPluginForScreen (CompPlugin *plugin)
    WRAPABLE_DEF (finiPluginForScreen, plugin)

	
bool
ScreenInterface::setOptionForPlugin (const char        *plugin,
				     const char	     *name,
				     CompOption::Value &value)
    WRAPABLE_DEF (setOptionForPlugin, plugin, name, value)

void
ScreenInterface::sessionEvent (CompSession::Event event,
			       CompOption::Vector &arguments)
    WRAPABLE_DEF (sessionEvent, event, arguments)



const CompMetadata::OptionInfo coreOptionInfo[COMP_OPTION_NUM] = {
    { "active_plugins", "list", "<type>string</type>", 0, 0 },
    { "click_to_focus", "bool", 0, 0, 0 },
    { "autoraise", "bool", 0, 0, 0 },
    { "autoraise_delay", "int", 0, 0, 0 },
    { "close_window_key", "key", 0, CompScreen::closeWin, 0 },
    { "close_window_button", "button", 0, CompScreen::closeWin, 0 },
    { "main_menu_key", "key", 0, CompScreen::mainMenu, 0 },
    { "run_key", "key", 0, CompScreen::runDialog, 0 },
    { "command0", "string", 0, 0, 0 },
    { "command1", "string", 0, 0, 0 },
    { "command2", "string", 0, 0, 0 },
    { "command3", "string", 0, 0, 0 },
    { "command4", "string", 0, 0, 0 },
    { "command5", "string", 0, 0, 0 },
    { "command6", "string", 0, 0, 0 },
    { "command7", "string", 0, 0, 0 },
    { "command8", "string", 0, 0, 0 },
    { "command9", "string", 0, 0, 0 },
    { "command10", "string", 0, 0, 0 },
    { "command11", "string", 0, 0, 0 },
    { "run_command0_key", "key", 0, CompScreen::runCommandDispatch, 0 },
    { "run_command1_key", "key", 0, CompScreen::runCommandDispatch, 0 },
    { "run_command2_key", "key", 0, CompScreen::runCommandDispatch, 0 },
    { "run_command3_key", "key", 0, CompScreen::runCommandDispatch, 0 },
    { "run_command4_key", "key", 0, CompScreen::runCommandDispatch, 0 },
    { "run_command5_key", "key", 0, CompScreen::runCommandDispatch, 0 },
    { "run_command6_key", "key", 0, CompScreen::runCommandDispatch, 0 },
    { "run_command7_key", "key", 0, CompScreen::runCommandDispatch, 0 },
    { "run_command8_key", "key", 0, CompScreen::runCommandDispatch, 0 },
    { "run_command9_key", "key", 0, CompScreen::runCommandDispatch, 0 },
    { "run_command10_key", "key", 0, CompScreen::runCommandDispatch, 0 },
    { "run_command11_key", "key", 0, CompScreen::runCommandDispatch, 0 },
    { "raise_window_key", "key", 0, CompScreen::raiseWin, 0 },
    { "raise_window_button", "button", 0, CompScreen::raiseWin, 0 },
    { "lower_window_key", "key", 0, CompScreen::lowerWin, 0 },
    { "lower_window_button", "button", 0, CompScreen::lowerWin, 0 },
    { "unmaximize_window_key", "key", 0, CompScreen::unmaximizeWin, 0 },
    { "minimize_window_key", "key", 0, CompScreen::minimizeWin, 0 },
    { "minimize_window_button", "button", 0, CompScreen::minimizeWin, 0 },
    { "maximize_window_key", "key", 0, CompScreen::maximizeWin, 0 },
    { "maximize_window_horizontally_key", "key", 0,
      CompScreen::maximizeWinHorizontally, 0 },
    { "maximize_window_vertically_key", "key", 0,
      CompScreen::maximizeWinVertically, 0 },
    { "command_screenshot", "string", 0, 0, 0 },
    { "run_command_screenshot_key", "key", 0,
      CompScreen::runCommandScreenshot, 0 },
    { "command_window_screenshot", "string", 0, 0, 0 },
    { "run_command_window_screenshot_key", "key", 0,
      CompScreen::runCommandWindowScreenshot, 0 },
    { "window_menu_button", "button", 0, CompScreen::windowMenu, 0 },
    { "window_menu_key", "key", 0, CompScreen::windowMenu, 0 },
    { "show_desktop_key", "key", 0, CompScreen::showDesktop, 0 },
    { "show_desktop_edge", "edge", 0, CompScreen::showDesktop, 0 },
    { "raise_on_click", "bool", 0, 0, 0 },
    { "audible_bell", "bool", 0, 0, 0 },
    { "toggle_window_maximized_key", "key", 0,
      CompScreen::toggleWinMaximized, 0 },
    { "toggle_window_maximized_button", "button", 0,
      CompScreen::toggleWinMaximized, 0 },
    { "toggle_window_maximized_horizontally_key", "key", 0,
      CompScreen::toggleWinMaximizedHorizontally, 0 },
    { "toggle_window_maximized_vertically_key", "key", 0,
      CompScreen::toggleWinMaximizedVertically, 0 },
    { "hide_skip_taskbar_windows", "bool", 0, 0, 0 },
    { "toggle_window_shaded_key", "key", 0, CompScreen::shadeWin, 0 },
    { "ignore_hints_when_maximized", "bool", 0, 0, 0 },
    { "command_terminal", "string", 0, 0, 0 },
    { "run_command_terminal_key", "key", 0,
      CompScreen::runCommandTerminal, 0 },
    { "ping_delay", "int", "<min>1000</min>", 0, 0 },
    { "edge_delay", "int", "<min>0</min>", 0, 0 },
    { "hsize", "int", "<min>1</min><max>32</max>", 0, 0 },
    { "vsize", "int", "<min>1</min><max>32</max>", 0, 0 },
    { "default_icon", "string", 0, 0, 0 },
    { "number_of_desktops", "int", "<min>1</min>", 0, 0 },
    { "detect_outputs", "bool", 0, 0, 0 },
    { "outputs", "list", "<type>string</type>", 0, 0 },
    { "overlapping_outputs", "int",
      RESTOSTRING (0, OUTPUT_OVERLAP_MODE_LAST), 0, 0 },
    { "focus_prevention_level", "int",
      RESTOSTRING (0, FOCUS_PREVENTION_LEVEL_LAST), 0, 0 },
    { "focus_prevention_match", "match", 0, 0, 0 }
};

static const int maskTable[] = {
    ShiftMask, LockMask, ControlMask, Mod1Mask,
    Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask
};
static const int maskTableSize = sizeof (maskTable) / sizeof (int);

static int errors = 0;

static int
errorHandler (Display     *dpy,
	      XErrorEvent *e)
{

#ifdef DEBUG
    char str[128];
#endif

    errors++;

#ifdef DEBUG
    XGetErrorDatabaseText (dpy, "XlibMessage", "XError", "", str, 128);
    fprintf (stderr, "%s", str);

    XGetErrorText (dpy, e->error_code, str, 128);
    fprintf (stderr, ": %s\n  ", str);

    XGetErrorDatabaseText (dpy, "XlibMessage", "MajorCode", "%d", str, 128);
    fprintf (stderr, str, e->request_code);

    sprintf (str, "%d", e->request_code);
    XGetErrorDatabaseText (dpy, "XRequest", str, "", str, 128);
    if (strcmp (str, ""))
	fprintf (stderr, " (%s)", str);
    fprintf (stderr, "\n  ");

    XGetErrorDatabaseText (dpy, "XlibMessage", "MinorCode", "%d", str, 128);
    fprintf (stderr, str, e->minor_code);
    fprintf (stderr, "\n  ");

    XGetErrorDatabaseText (dpy, "XlibMessage", "ResourceID", "%d", str, 128);
    fprintf (stderr, str, e->resourceid);
    fprintf (stderr, "\n");

    /* abort (); */
#endif

    return 0;
}

int
CompScreen::checkForError (Display *dpy)
{
    int e;

    XSync (dpy, FALSE);

    e = errors;
    errors = 0;

    return e;
}

Display *
CompScreen::dpy ()
{
    return priv->dpy;
}


CompOption *
CompScreen::getOption (const char *name)
{
    CompOption *o = CompOption::findOption (priv->opt, name);
    return o;
}

bool
CompScreen::XRandr ()
{
    return priv->randrExtension;
}

int
CompScreen::randrEvent ()
{
    return priv->randrEvent;
}

bool
CompScreen::XShape ()
{
    return priv->shapeExtension;
}

int
CompScreen::shapeEvent ()
{
    return priv->shapeEvent;
}

int
CompScreen::syncEvent ()
{
    return priv->syncEvent;
}


SnDisplay *
CompScreen::snDisplay ()
{
    return priv->snDisplay;
}

Window
CompScreen::activeWindow ()
{
    return priv->activeWindow;
}

Window
CompScreen::autoRaiseWindow ()
{
    return priv->autoRaiseWindow;
}

const char *
CompScreen::displayString ()
{
    return priv->displayString;
}

void
PrivateScreen::updateScreenInfo ()
{
    if (xineramaExtension)
    {
	int nInfo;
	XineramaScreenInfo *info = XineramaQueryScreens (dpy, &nInfo);
	
	screenInfo = std::vector<XineramaScreenInfo> (info, info + nInfo);

	if (info)
	    XFree (info);
    }
}



void
PrivateScreen::setAudibleBell (bool audible)
{
    if (xkbExtension)
	XkbChangeEnabledControls (dpy,
				  XkbUseCoreKbd,
				  XkbAudibleBellMask,
				  audible ? XkbAudibleBellMask : 0);
}

bool
PrivateScreen::handlePingTimeout ()
{
    XEvent      ev;
    int		ping = lastPing + 1;

    ev.type		    = ClientMessage;
    ev.xclient.window	    = 0;
    ev.xclient.message_type = Atoms::wmProtocols;
    ev.xclient.format	    = 32;
    ev.xclient.data.l[0]    = Atoms::wmPing;
    ev.xclient.data.l[1]    = ping;
    ev.xclient.data.l[2]    = 0;
    ev.xclient.data.l[3]    = 0;
    ev.xclient.data.l[4]    = 0;

    foreach (CompWindow *w, windows)
    {
	if (w->priv->handlePingTimeout (lastPing))
	{
	    ev.xclient.window    = w->id ();
	    ev.xclient.data.l[2] = w->id ();

	    XSendEvent (dpy, w->id (), false, NoEventMask, &ev);
	}
    }

    lastPing = ping;

    return true;
}

CompOption::Vector &
CompScreen::getOptions ()
{
    return priv->opt;
}

bool
CompScreen::setOption (const char        *name,
		       CompOption::Value &value)
{
    CompOption   *o;
    unsigned int index;

    o = CompOption::findOption (priv->opt, name, &index);
    if (!o)
	return false;

    switch (index) {
	case COMP_OPTION_ACTIVE_PLUGINS:
	    if (o->set (value))
	    {
		priv->dirtyPluginList = true;
		return true;
	    }
	    break;
	case COMP_OPTION_PING_DELAY:
	    if (o->set (value))
	    {
		priv->pingTimer.setTimes (o->value ().i (),
					  o->value ().i () + 500);
		return true;
	    }
	    break;
	case COMP_OPTION_AUDIBLE_BELL:
	    if (o->set (value))
	    {
		priv->setAudibleBell (o->value ().b ());
		return true;
	    }
	    break;
	case COMP_OPTION_DETECT_OUTPUTS:
	    if (o->set (value))
	    {
		if (value.b ())
		    priv->detectOutputDevices ();

		return true;
	    }
	    break;
	case COMP_OPTION_HSIZE:
	    if (o->set (value))
	    {
		CompOption *vsize;

		vsize = CompOption::findOption (priv->opt, "vsize");

		if (!vsize)
		    return false;

		if (o->value ().i () * priv->size.width () > MAXSHORT)
		    return false;

		priv->setVirtualScreenSize (o->value ().i (), vsize->value ().i ());
		return true;
	    }
	    break;
	case COMP_OPTION_VSIZE:
	    if (o->set (value))
	    {
		CompOption *hsize;

		hsize = CompOption::findOption (priv->opt, "hsize");

		if (!hsize)
		    return false;

		if (o->value ().i () * priv->size.height () > MAXSHORT)
		    return false;

		priv->setVirtualScreenSize (hsize->value ().i (), o->value ().i ());
		return true;
	    }
	    break;
	case COMP_OPTION_NUMBER_OF_DESKTOPS:
	    if (o->set (value))
	    {
		priv->setNumberOfDesktops (o->value ().i ());
		return true;
	    }
	    break;
	case COMP_OPTION_DEFAULT_ICON:
	    if (o->set (value))
		return priv->updateDefaultIcon ();
	    break;
	case COMP_OPTION_OUTPUTS:
	    if (!noDetection &&
		priv->opt[COMP_OPTION_DETECT_OUTPUTS].value ().b ())
		return false;

	    if (o->set (value))
	    {
		priv->updateOutputDevices ();
		return true;
	    }
	    break;
	default:
	    if (CompOption::setOption (*o, value))
		return true;
	    break;
    }

    return false;
}

void
PrivateScreen::updateModifierMappings ()
{
    unsigned int    modMask[CompModNum];
    int		    i, minKeycode, maxKeycode, keysymsPerKeycode = 0;
    KeySym*         key;

    for (i = 0; i < CompModNum; i++)
	modMask[i] = 0;

    XDisplayKeycodes (this->dpy, &minKeycode, &maxKeycode);
    key = XGetKeyboardMapping (this->dpy,
			       minKeycode, (maxKeycode - minKeycode + 1),
			       &keysymsPerKeycode);

    if (this->modMap)
	XFreeModifiermap (this->modMap);

    this->modMap = XGetModifierMapping (this->dpy);
    if (this->modMap && this->modMap->max_keypermod > 0)
    {
	KeySym keysym;
	int    index, size, mask;

	size = maskTableSize * this->modMap->max_keypermod;

	for (i = 0; i < size; i++)
	{
	    if (!this->modMap->modifiermap[i])
		continue;

	    index = 0;
	    do
	    {
		keysym = XKeycodeToKeysym (this->dpy,
					   this->modMap->modifiermap[i],
					   index++);
	    } while (!keysym && index < keysymsPerKeycode);

	    if (keysym)
	    {
		mask = maskTable[i / this->modMap->max_keypermod];

		if (keysym == XK_Alt_L ||
		    keysym == XK_Alt_R)
		{
		    modMask[CompModAlt] |= mask;
		}
		else if (keysym == XK_Meta_L ||
			 keysym == XK_Meta_R)
		{
		    modMask[CompModMeta] |= mask;
		}
		else if (keysym == XK_Super_L ||
			 keysym == XK_Super_R)
		{
		    modMask[CompModSuper] |= mask;
		}
		else if (keysym == XK_Hyper_L ||
			 keysym == XK_Hyper_R)
		{
		    modMask[CompModHyper] |= mask;
		}
		else if (keysym == XK_Mode_switch)
		{
		    modMask[CompModModeSwitch] |= mask;
		}
		else if (keysym == XK_Scroll_Lock)
		{
		    modMask[CompModScrollLock] |= mask;
		}
		else if (keysym == XK_Num_Lock)
		{
		    modMask[CompModNumLock] |= mask;
		}
	    }
	}

	for (i = 0; i < CompModNum; i++)
	{
	    if (!modMask[i])
		modMask[i] = CompNoMask;
	}

	if (memcmp (modMask, this->modMask, sizeof (modMask)))
	{
	    memcpy (this->modMask, modMask, sizeof (modMask));

	    this->ignoredModMask = LockMask |
		(modMask[CompModNumLock]    & ~CompNoMask) |
		(modMask[CompModScrollLock] & ~CompNoMask);

	    this->updatePassiveKeyGrabs ();
	}
    }

    if (key)
	XFree (key);
}

unsigned int
PrivateScreen::virtualToRealModMask (unsigned int modMask)
{
    int i;

    for (i = 0; i < CompModNum; i++)
    {
	if (modMask & virtualModMask[i])
	{
	    modMask &= ~virtualModMask[i];
	    modMask |= this->modMask[i];
	}
    }

    return modMask;
}

unsigned int
PrivateScreen::keycodeToModifiers (int keycode)
{
    unsigned int mods = 0;
    int mod, k;

    for (mod = 0; mod < maskTableSize; mod++)
    {
	for (k = 0; k < modMap->max_keypermod; k++)
	{
	    if (modMap->modifiermap[mod *
		modMap->max_keypermod + k] == keycode)
		mods |= maskTable[mod];
	}
    }

    return mods;
}

void
PrivateScreen::processEvents ()
{
    XEvent event;

    /* remove destroyed windows */
    removeDestroyed ();

    if (dirtyPluginList)
	updatePlugins ();

    while (XPending (dpy))
    {
	XNextEvent (dpy, &event);

	switch (event.type) {
	case ButtonPress:
	case ButtonRelease:
	    pointerX = event.xbutton.x_root;
	    pointerY = event.xbutton.y_root;
	    break;
	case KeyPress:
	case KeyRelease:
	    pointerX = event.xkey.x_root;
	    pointerY = event.xkey.y_root;
	    break;
	case MotionNotify:
	    pointerX = event.xmotion.x_root;
	    pointerY = event.xmotion.y_root;
	    break;
	case EnterNotify:
	case LeaveNotify:
	    pointerX = event.xcrossing.x_root;
	    pointerY = event.xcrossing.y_root;
	    break;
	case ClientMessage:
	    if (event.xclient.message_type == Atoms::xdndPosition)
	    {
		pointerX = event.xclient.data.l[2] >> 16;
		pointerY = event.xclient.data.l[2] & 0xffff;
	    }
	default:
	    break;
        }

	sn_display_process_event (snDisplay, &event);

	inHandleEvent = true;
	screen->handleEvent (&event);
	inHandleEvent = false;

	lastPointerX = pointerX;
	lastPointerY = pointerY;
    }
}

void
PrivateScreen::updatePlugins ()
{
    CompOption              *o;
    CompPlugin              *p;
    unsigned int            nPop, i, j;
    CompPlugin::List        pop;

    dirtyPluginList = false;

    o = &opt[COMP_OPTION_ACTIVE_PLUGINS];

    /* The old plugin list always begins with the core plugin. To make sure
       we don't unnecessarily unload plugins if the new plugin list does not
       contain the core plugin, we have to use an offset */

    if (o->value ().list ().size () > 0 &&
	o->value ().list ()[0]. s (). compare ("core"))
	i = 0;
    else
	i = 1;

    /* j is initialized to 1 to make sure we never pop the core plugin */
    for (j = 1; j < plugin.list ().size () &&
	 i < o->value ().list ().size (); i++, j++)
    {
	if (plugin.list ()[j].s ().compare (o->value ().list ()[i].s ()))
	    break;
    }

    nPop = plugin.list ().size () - j;

    for (j = 0; j < nPop; j++)
    {
	pop.push_back (CompPlugin::pop ());
	plugin.list ().pop_back ();
    }

    for (; i < o->value ().list ().size (); i++)
    {
	p = NULL;
	foreach (CompPlugin *pp, pop)
	{
	    if (o->value ().list ()[i]. s ().compare (pp->vTable->name ()) == 0)
	    {
		if (CompPlugin::push (pp))
		{
		    p = pp;
		    pop.erase (std::find (pop.begin (), pop.end (), pp));
		    break;
		}
	    }
	}

	if (p == 0)
	{
	    p = CompPlugin::load (o->value ().list ()[i].s ().c_str ());
	    if (p)
	    {
		if (!CompPlugin::push (p))
		{
		    CompPlugin::unload (p);
		    p = 0;
		}
	    }
	}

	if (p)
	{
	    plugin.list ().push_back (p->vTable->name ());
	}
    }

    foreach (CompPlugin *pp, pop)
    {
	CompPlugin::unload (pp);
    }

    screen->setOptionForPlugin ("core", o->name ().c_str (), plugin);
}

/* from fvwm2, Copyright Matthias Clasen, Dominik Vogt */
static bool
convertProperty (Display     *dpy,
		 Time        time,
		 Window      w,
		 Atom        target,
		 Atom        property)
{

#define N_TARGETS 4

    Atom conversionTargets[N_TARGETS];
    long icccmVersion[] = { 2, 0 };

    conversionTargets[0] = Atoms::targets;
    conversionTargets[1] = Atoms::multiple;
    conversionTargets[2] = Atoms::timestamp;
    conversionTargets[3] = Atoms::version;

    if (target == Atoms::targets)
	XChangeProperty (dpy, w, property,
			 XA_ATOM, 32, PropModeReplace,
			 (unsigned char *) conversionTargets, N_TARGETS);
    else if (target == Atoms::timestamp)
	XChangeProperty (dpy, w, property,
			 XA_INTEGER, 32, PropModeReplace,
			 (unsigned char *) &time, 1);
    else if (target == Atoms::version)
	XChangeProperty (dpy, w, property,
			 XA_INTEGER, 32, PropModeReplace,
			 (unsigned char *) icccmVersion, 2);
    else
	return false;

    /* Be sure the PropertyNotify has arrived so we
     * can send SelectionNotify
     */
    XSync (dpy, FALSE);

    return true;
}

/* from fvwm2, Copyright Matthias Clasen, Dominik Vogt */
void
PrivateScreen::handleSelectionRequest (XEvent *event)
{
    XSelectionEvent reply;

    if (wmSnSelectionWindow != event->xselectionrequest.owner ||
	wmSnAtom != event->xselectionrequest.selection)
	return;

    reply.type	    = SelectionNotify;
    reply.display   = dpy;
    reply.requestor = event->xselectionrequest.requestor;
    reply.selection = event->xselectionrequest.selection;
    reply.target    = event->xselectionrequest.target;
    reply.property  = None;
    reply.time	    = event->xselectionrequest.time;

    if (event->xselectionrequest.target == Atoms::multiple)
    {
	if (event->xselectionrequest.property != None)
	{
	    Atom	  type, *adata;
	    int		  i, format;
	    unsigned long num, rest;
	    unsigned char *data;

	    if (XGetWindowProperty (dpy,
				    event->xselectionrequest.requestor,
				    event->xselectionrequest.property,
				    0, 256, FALSE,
				    Atoms::atomPair,
				    &type, &format, &num, &rest,
				    &data) != Success)
		return;

	    /* FIXME: to be 100% correct, should deal with rest > 0,
	     * but since we have 4 possible targets, we will hardly ever
	     * meet multiple requests with a length > 8
	     */
	    adata = (Atom *) data;
	    i = 0;
	    while (i < (int) num)
	    {
		if (!convertProperty (dpy, wmSnTimestamp,
				      event->xselectionrequest.requestor,
				      adata[i], adata[i + 1]))
		    adata[i + 1] = None;

		i += 2;
	    }

	    XChangeProperty (dpy,
			     event->xselectionrequest.requestor,
			     event->xselectionrequest.property,
			     Atoms::atomPair,
			     32, PropModeReplace, data, num);
	}
    }
    else
    {
	if (event->xselectionrequest.property == None)
	    event->xselectionrequest.property = event->xselectionrequest.target;

	if (convertProperty (dpy, wmSnTimestamp,
			     event->xselectionrequest.requestor,
			     event->xselectionrequest.target,
			     event->xselectionrequest.property))
	    reply.property = event->xselectionrequest.property;
    }

    XSendEvent (dpy, event->xselectionrequest.requestor,
		FALSE, 0L, (XEvent *) &reply);
}

void
PrivateScreen::handleSelectionClear (XEvent *event)
{
    /* We need to unmanage the screen on which we lost the selection */
    if (wmSnSelectionWindow != event->xselectionrequest.owner ||
	wmSnAtom != event->xselectionrequest.selection)
	return;

    shutDown = TRUE;
}


#define HOME_IMAGEDIR ".compiz/images"

bool
CompScreen::readImageFromFile (const char *name,
			       int        *width,
			       int        *height,
			       void       **data)
{
    Bool status;
    int  stride;

    status = fileToImage (NULL, name, width, height, &stride, data);
    if (!status)
    {
	char *home;

	home = getenv ("HOME");
	if (home)
	{
	    char *path;

	    path = (char *) malloc (strlen (home) + strlen (HOME_IMAGEDIR) + 2);
	    if (path)
	    {
		sprintf (path, "%s/%s", home, HOME_IMAGEDIR);
		status = fileToImage (path, name, width, height, &stride, data);

		free (path);

		if (status)
		    return TRUE;
	    }
	}

	status = fileToImage (IMAGEDIR, name, width, height, &stride, data);
    }

    return status;
}

bool
CompScreen::writeImageToFile (const char *path,
			      const char *name,
			      const char *format,
			      int        width,
			      int        height,
			      void       *data)
{
        return imageToFile (path, name, format, width, height, width * 4, data);
}

Window
PrivateScreen::getActiveWindow (Window root)
{
    Atom	  actual;
    int		  result, format;
    unsigned long n, left;
    unsigned char *data;
    Window	  w = None;

    result = XGetWindowProperty (priv->dpy, root,
				 Atoms::winActive, 0L, 1L, FALSE,
				 XA_WINDOW, &actual, &format,
				 &n, &left, &data);

    if (result == Success && n && data)
    {
	memcpy (&w, data, sizeof (Window));
	XFree (data);
    }

    return w;
}


bool
CompScreen::fileToImage (const char *path,
			 const char *name,
			 int        *width,
			 int        *height,
			 int        *stride,
			 void       **data)
{
    WRAPABLE_HND_FUNC_RETURN(8, bool, fileToImage, path, name, width, height,
			     stride, data)
    return false;
}

bool
CompScreen::imageToFile (const char *path,
			 const char *name,
			 const char *format,
			 int        width,
			 int        height,
			 int        stride,
			 void       *data)
{
    WRAPABLE_HND_FUNC_RETURN(9, bool, imageToFile, path, name, format, width,
			     height, stride, data)
    return false;
}

const char *
logLevelToString (CompLogLevel level)
{
    switch (level) {
    case CompLogLevelFatal:
	return "Fatal";
    case CompLogLevelError:
	return "Error";
    case CompLogLevelWarn:
	return "Warn";
    case CompLogLevelInfo:
	return "Info";
    case CompLogLevelDebug:
	return "Debug";
    default:
	break;
    }

    return "Unknown";
}

static void
logMessage (const char   *componentName,
	    CompLogLevel level,
	    const char   *message)
{
    fprintf (stderr, "%s (%s) - %s: %s\n",
	     programName, componentName,
	     logLevelToString (level), message);
}

void
CompScreen::logMessage (const char   *componentName,
			CompLogLevel level,
			const char   *message)
{
    WRAPABLE_HND_FUNC(13, logMessage, componentName, level, message)
    ::logMessage (componentName, level, message);
}

void
compLogMessage (const char *componentName,
	        CompLogLevel level,
	        const char   *format,
	        ...)
{
    va_list args;
    char    message[2048];

    va_start (args, format);

    vsnprintf (message, 2048, format, args);

    if (screen)
	screen->logMessage (componentName, level, message);
    else
	logMessage (componentName, level, message);

    va_end (args);
}

int
PrivateScreen::getWmState (Window id)
{
    Atom	  actual;
    int		  result, format;
    unsigned long n, left;
    unsigned char *data;
    unsigned long state = NormalState;

    result = XGetWindowProperty (priv->dpy, id,
				 Atoms::wmState, 0L, 2L, FALSE,
				 Atoms::wmState, &actual, &format,
				 &n, &left, &data);

    if (result == Success && n && data)
    {
	memcpy (&state, data, sizeof (unsigned long));
	XFree ((void *) data);
    }

    return state;
}

void
PrivateScreen::setWmState (int state, Window id)
{
    unsigned long data[2];

    data[0] = state;
    data[1] = None;

    XChangeProperty (priv->dpy, id,
		     Atoms::wmState, Atoms::wmState,
		     32, PropModeReplace, (unsigned char *) data, 2);
}

unsigned int
PrivateScreen::windowStateMask (Atom state)
{
    if (state == Atoms::winStateModal)
	return CompWindowStateModalMask;
    else if (state == Atoms::winStateSticky)
	return CompWindowStateStickyMask;
    else if (state == Atoms::winStateMaximizedVert)
	return CompWindowStateMaximizedVertMask;
    else if (state == Atoms::winStateMaximizedHorz)
	return CompWindowStateMaximizedHorzMask;
    else if (state == Atoms::winStateShaded)
	return CompWindowStateShadedMask;
    else if (state == Atoms::winStateSkipTaskbar)
	return CompWindowStateSkipTaskbarMask;
    else if (state == Atoms::winStateSkipPager)
	return CompWindowStateSkipPagerMask;
    else if (state == Atoms::winStateHidden)
	return CompWindowStateHiddenMask;
    else if (state == Atoms::winStateFullscreen)
	return CompWindowStateFullscreenMask;
    else if (state == Atoms::winStateAbove)
	return CompWindowStateAboveMask;
    else if (state == Atoms::winStateBelow)
	return CompWindowStateBelowMask;
    else if (state == Atoms::winStateDemandsAttention)
	return CompWindowStateDemandsAttentionMask;
    else if (state == Atoms::winStateDisplayModal)
	return CompWindowStateDisplayModalMask;

    return 0;
}

unsigned int
PrivateScreen::windowStateFromString (const char *str)
{
    if (strcasecmp (str, "modal") == 0)
	return CompWindowStateModalMask;
    else if (strcasecmp (str, "sticky") == 0)
	return CompWindowStateStickyMask;
    else if (strcasecmp (str, "maxvert") == 0)
	return CompWindowStateMaximizedVertMask;
    else if (strcasecmp (str, "maxhorz") == 0)
	return CompWindowStateMaximizedHorzMask;
    else if (strcasecmp (str, "shaded") == 0)
	return CompWindowStateShadedMask;
    else if (strcasecmp (str, "skiptaskbar") == 0)
	return CompWindowStateSkipTaskbarMask;
    else if (strcasecmp (str, "skippager") == 0)
	return CompWindowStateSkipPagerMask;
    else if (strcasecmp (str, "hidden") == 0)
	return CompWindowStateHiddenMask;
    else if (strcasecmp (str, "fullscreen") == 0)
	return CompWindowStateFullscreenMask;
    else if (strcasecmp (str, "above") == 0)
	return CompWindowStateAboveMask;
    else if (strcasecmp (str, "below") == 0)
	return CompWindowStateBelowMask;
    else if (strcasecmp (str, "demandsattention") == 0)
	return CompWindowStateDemandsAttentionMask;

    return 0;
}

unsigned int
PrivateScreen::getWindowState (Window id)
{
    Atom	  actual;
    int		  result, format;
    unsigned long n, left;
    unsigned char *data;
    unsigned int  state = 0;

    result = XGetWindowProperty (priv->dpy, id,
				 Atoms::winState,
				 0L, 1024L, FALSE, XA_ATOM, &actual, &format,
				 &n, &left, &data);

    if (result == Success && data)
    {
	Atom *a = (Atom *) data;

	while (n--)
	    state |= windowStateMask (*a++);

	XFree ((void *) data);
    }

    return state;
}

void
PrivateScreen::setWindowState (unsigned int state, Window id)
{
    Atom data[32];
    int	 i = 0;

    if (state & CompWindowStateModalMask)
	data[i++] = Atoms::winStateModal;
    if (state & CompWindowStateStickyMask)
	data[i++] = Atoms::winStateSticky;
    if (state & CompWindowStateMaximizedVertMask)
	data[i++] = Atoms::winStateMaximizedVert;
    if (state & CompWindowStateMaximizedHorzMask)
	data[i++] = Atoms::winStateMaximizedHorz;
    if (state & CompWindowStateShadedMask)
	data[i++] = Atoms::winStateShaded;
    if (state & CompWindowStateSkipTaskbarMask)
	data[i++] = Atoms::winStateSkipTaskbar;
    if (state & CompWindowStateSkipPagerMask)
	data[i++] = Atoms::winStateSkipPager;
    if (state & CompWindowStateHiddenMask)
	data[i++] = Atoms::winStateHidden;
    if (state & CompWindowStateFullscreenMask)
	data[i++] = Atoms::winStateFullscreen;
    if (state & CompWindowStateAboveMask)
	data[i++] = Atoms::winStateAbove;
    if (state & CompWindowStateBelowMask)
	data[i++] = Atoms::winStateBelow;
    if (state & CompWindowStateDemandsAttentionMask)
	data[i++] = Atoms::winStateDemandsAttention;
    if (state & CompWindowStateDisplayModalMask)
	data[i++] = Atoms::winStateDisplayModal;

    XChangeProperty (priv->dpy, id, Atoms::winState,
		     XA_ATOM, 32, PropModeReplace,
		     (unsigned char *) data, i);
}

unsigned int
PrivateScreen::getWindowType (Window id)
{
    Atom	  actual;
    int		  result, format;
    unsigned long n, left;
    unsigned char *data;

    result = XGetWindowProperty (priv->dpy , id,
				 Atoms::winType,
				 0L, 1L, FALSE, XA_ATOM, &actual, &format,
				 &n, &left, &data);

    if (result == Success && n && data)
    {
	Atom a;

	memcpy (&a, data, sizeof (Atom));
	XFree ((void *) data);

	if (a == Atoms::winTypeNormal)
	    return CompWindowTypeNormalMask;
	else if (a == Atoms::winTypeMenu)
	    return CompWindowTypeMenuMask;
	else if (a == Atoms::winTypeDesktop)
	    return CompWindowTypeDesktopMask;
	else if (a == Atoms::winTypeDock)
	    return CompWindowTypeDockMask;
	else if (a == Atoms::winTypeToolbar)
	    return CompWindowTypeToolbarMask;
	else if (a == Atoms::winTypeUtil)
	    return CompWindowTypeUtilMask;
	else if (a == Atoms::winTypeSplash)
	    return CompWindowTypeSplashMask;
	else if (a == Atoms::winTypeDialog)
	    return CompWindowTypeDialogMask;
	else if (a == Atoms::winTypeDropdownMenu)
	    return CompWindowTypeDropdownMenuMask;
	else if (a == Atoms::winTypePopupMenu)
	    return CompWindowTypePopupMenuMask;
	else if (a == Atoms::winTypeTooltip)
	    return CompWindowTypeTooltipMask;
	else if (a == Atoms::winTypeNotification)
	    return CompWindowTypeNotificationMask;
	else if (a == Atoms::winTypeCombo)
	    return CompWindowTypeComboMask;
	else if (a == Atoms::winTypeDnd)
	    return CompWindowTypeDndMask;
    }

    return CompWindowTypeUnknownMask;
}

void
PrivateScreen::getMwmHints (Window       id,
			    unsigned int *func,
			    unsigned int *decor)
{
    Atom	  actual;
    int		  result, format;
    unsigned long n, left;
    unsigned char *data;

    *func  = MwmFuncAll;
    *decor = MwmDecorAll;

    result = XGetWindowProperty (priv->dpy, id,
				 Atoms::mwmHints,
				 0L, 20L, FALSE, Atoms::mwmHints,
				 &actual, &format, &n, &left, &data);

    if (result == Success && n && data)
    {
	MwmHints *mwmHints = (MwmHints *) data;

	if (n >= PropMotifWmHintElements)
	{
	    if (mwmHints->flags & MwmHintsDecorations)
		*decor = mwmHints->decorations;

	    if (mwmHints->flags & MwmHintsFunctions)
		*func = mwmHints->functions;
	}

	XFree (data);
    }
}

unsigned int
PrivateScreen::getProtocols (Window id)
{
    Atom         *protocol;
    int          count;
    unsigned int protocols = 0;

    if (XGetWMProtocols (priv->dpy, id, &protocol, &count))
    {
	int  i;

	for (i = 0; i < count; i++)
	{
	    if (protocol[i] == Atoms::wmDeleteWindow)
		protocols |= CompWindowProtocolDeleteMask;
	    else if (protocol[i] == Atoms::wmTakeFocus)
		protocols |= CompWindowProtocolTakeFocusMask;
	    else if (protocol[i] == Atoms::wmPing)
		protocols |= CompWindowProtocolPingMask;
	    else if (protocol[i] == Atoms::wmSyncRequest)
		protocols |= CompWindowProtocolSyncRequestMask;
	}

	XFree (protocol);
    }

    return protocols;
}

unsigned int
CompScreen::getWindowProp (Window       id,
			   Atom         property,
			   unsigned int defaultValue)
{
    Atom	  actual;
    int		  result, format;
    unsigned long n, left;
    unsigned char *data;

    result = XGetWindowProperty (priv->dpy, id, property,
				 0L, 1L, FALSE, XA_CARDINAL, &actual, &format,
				 &n, &left, &data);

    if (result == Success && n && data)
    {
	unsigned long value;

	memcpy (&value, data, sizeof (unsigned long));

	XFree (data);

	return (unsigned int) value;
    }

    return defaultValue;
}

void
CompScreen::setWindowProp (Window       id,
			   Atom         property,
			   unsigned int value)
{
    unsigned long data = value;

    XChangeProperty (priv->dpy, id, property,
		     XA_CARDINAL, 32, PropModeReplace,
		     (unsigned char *) &data, 1);
}

bool
PrivateScreen::readWindowProp32 (Window         id,
				 Atom           property,
				 unsigned short *returnValue)
{
    Atom	  actual;
    int		  result, format;
    unsigned long n, left;
    unsigned char *data;

    result = XGetWindowProperty (priv->dpy, id, property,
				 0L, 1L, FALSE, XA_CARDINAL, &actual, &format,
				 &n, &left, &data);

    if (result == Success && n && data)
    {
	CARD32 value;

	memcpy (&value, data, sizeof (CARD32));

	XFree (data);

	*returnValue = value >> 16;

	return true;
    }

    return false;
}

unsigned short
CompScreen::getWindowProp32 (Window         id,
			     Atom           property,
			     unsigned short defaultValue)
{
    unsigned short result;

    if (priv->readWindowProp32 (id, property, &result))
	return result;

    return defaultValue;
}

void
CompScreen::setWindowProp32 (Window         id,
			     Atom           property,
			     unsigned short value)
{
    CARD32 value32;

    value32 = value << 16 | value;

    XChangeProperty (priv->dpy, id, property,
		     XA_CARDINAL, 32, PropModeReplace,
		     (unsigned char *) &value32, 1);
}

void
ScreenInterface::handleEvent (XEvent *event)
    WRAPABLE_DEF (handleEvent, event)

void
ScreenInterface::handleCompizEvent (const char         *plugin,
				    const char         *event,
				    CompOption::Vector &options)
    WRAPABLE_DEF (handleCompizEvent, plugin, event, options)

bool
ScreenInterface::fileToImage (const char *path,
			      const char *name,
			      int        *width,
			      int        *height,
			      int        *stride,
			      void       **data)
    WRAPABLE_DEF (fileToImage, path, name, width, height, stride, data)

bool
ScreenInterface::imageToFile (const char *path,
			      const char *name,
			      const char *format,
			      int        width,
			      int        height,
			      int        stride,
			      void       *data)
    WRAPABLE_DEF (imageToFile, path, name, format, width, height, stride, data)

CompMatch::Expression *
ScreenInterface::matchInitExp (const CompString value)
    WRAPABLE_DEF (matchInitExp, value)

void
ScreenInterface::matchExpHandlerChanged ()
    WRAPABLE_DEF (matchExpHandlerChanged)

void
ScreenInterface::matchPropertyChanged (CompWindow *window)
    WRAPABLE_DEF (matchPropertyChanged, window)

void
ScreenInterface::logMessage (const char   *componentName,
			     CompLogLevel level,
			     const char   *message)
    WRAPABLE_DEF (logMessage, componentName, level, message)


bool
PrivateScreen::desktopHintEqual (unsigned long *data,
				 int           size,
				 int           offset,
				 int           hintSize)
{
    if (size != desktopHintSize)
	return false;

    if (memcmp (data + offset,
		desktopHintData + offset,
		hintSize * sizeof (unsigned long)) == 0)
	return true;

    return false;
}

void
PrivateScreen::setDesktopHints ()
{
    unsigned long *data;
    int		  dSize, offset, hintSize;
    unsigned int  i;

    dSize = nDesktop * 2 + nDesktop * 2 + nDesktop * 4 + 1;

    data = (unsigned long *) malloc (sizeof (unsigned long) * dSize);
    if (!data)
	return;

    offset   = 0;
    hintSize = nDesktop * 2;

    for (i = 0; i < nDesktop; i++)
    {
	data[offset + i * 2 + 0] = vp.x () * size.width ();
	data[offset + i * 2 + 1] = vp.y () * size.height ();
    }

    if (!desktopHintEqual (data, dSize, offset, hintSize))
	XChangeProperty (dpy, root,
			 Atoms::desktopViewport,
			 XA_CARDINAL, 32, PropModeReplace,
			 (unsigned char *) &data[offset], hintSize);

    offset += hintSize;

    for (i = 0; i < nDesktop; i++)
    {
	data[offset + i * 2 + 0] = size.width () * vpSize.width ();
	data[offset + i * 2 + 1] = size.height () * vpSize.height ();
    }

    if (!desktopHintEqual (data, dSize, offset, hintSize))
	XChangeProperty (dpy, root,
			 Atoms::desktopGeometry,
			 XA_CARDINAL, 32, PropModeReplace,
			 (unsigned char *) &data[offset], hintSize);

    offset += hintSize;
    hintSize = nDesktop * 4;

    for (i = 0; i < nDesktop; i++)
    {
	data[offset + i * 4 + 0] = workArea.x;
	data[offset + i * 4 + 1] = workArea.y;
	data[offset + i * 4 + 2] = workArea.width;
	data[offset + i * 4 + 3] = workArea.height;
    }

    if (!desktopHintEqual (data, dSize, offset, hintSize))
	XChangeProperty (dpy, root,
			 Atoms::workarea,
			 XA_CARDINAL, 32, PropModeReplace,
			 (unsigned char *) &data[offset], hintSize);

    offset += hintSize;

    data[offset] = nDesktop;
    hintSize = 1;

    if (!desktopHintEqual (data, dSize, offset, hintSize))
	XChangeProperty (dpy, root,
			 Atoms::numberOfDesktops,
			 XA_CARDINAL, 32, PropModeReplace,
			 (unsigned char *) &data[offset], hintSize);

    if (desktopHintData)
	free (desktopHintData);

    desktopHintData = data;
    desktopHintSize = dSize;
}

void
PrivateScreen::setVirtualScreenSize (int newh, int newv)
{
    vpSize.setWidth (newh);
    vpSize.setHeight (newv);

    setDesktopHints ();
}

void
PrivateScreen::updateOutputDevices ()
{
    CompOption::Value::Vector &list =
	opt[COMP_OPTION_OUTPUTS].value ().list ();

    unsigned int  nOutput = 0;
    int		  x, y, bits;
    unsigned int  width, height;
    int		  x1, y1, x2, y2;
    char          str[10];

    foreach (CompOption::Value &value, list)
    {
	x      = 0;
	y      = 0;
	width  = size.width ();
	height = size.height ();

	bits = XParseGeometry (value.s ().c_str (), &x, &y, &width, &height);

	if (bits & XNegative)
	    x = size.width () + x - width;

	if (bits & YNegative)
	    y = size.height () + y - height;

	x1 = x;
	y1 = y;
	x2 = x + width;
	y2 = y + height;

	if (x1 < 0)
	    x1 = 0;
	if (y1 < 0)
	    y1 = 0;
	if (x2 > (int) size.width ())
	    x2 = size.width ();
	if (y2 > (int) size.height ())
	    y2 = size.height ();

	if (x1 < x2 && y1 < y2)
	{
	    if (outputDevs.size () < nOutput + 1)
		outputDevs.resize (nOutput + 1);

	    outputDevs[nOutput].setGeometry (x1, x2, y1, y2);
	    nOutput++;
	}
    }

    /* make sure we have at least one output */
    if (!nOutput)
    {
	if (outputDevs.size () < 1)
	    outputDevs.resize (1);

	outputDevs[0].setGeometry (0, size.width (), 0, size.height ());
	nOutput = 1;
    }

    if (outputDevs.size () > nOutput)
	outputDevs.resize (nOutput);

    /* set name, width, height and update rect pointers in all regions */
    for (unsigned int i = 0; i < nOutput; i++)
    {
	snprintf (str, 10, "Output %d", i);
	outputDevs[i].setId (str, i);
    }


    hasOverlappingOutputs = false;

    setCurrentOutput (currentOutputDev);

    updateWorkarea ();

    screen->outputChangeNotify ();
}

void
PrivateScreen::detectOutputDevices ()
{
    if (!noDetection && opt[COMP_OPTION_DETECT_OUTPUTS].value ().b ())
    {
	CompString	  name;
	CompOption::Value value;

	if (screenInfo.size ())
	{
	    CompOption::Value::Vector l;
	    foreach (XineramaScreenInfo xi, screenInfo)
	    {
		l.push_back (compPrintf ("%dx%d+%d+%d", xi.width, xi.height,
					 xi.x_org, xi.y_org));
	    }

	    value.set (CompOption::TypeString, l);
	}
	else
	{
	    CompOption::Value::Vector l;
	    l.push_back (compPrintf ("%dx%d+%d+%d", this->size.width(),
				     this->size.height (), 0, 0));
	    value.set (CompOption::TypeString, l);
	}

	name = opt[COMP_OPTION_OUTPUTS].name ();

	opt[COMP_OPTION_DETECT_OUTPUTS].value ().set (false);
	screen->setOptionForPlugin ("core", name.c_str (), value);
	opt[COMP_OPTION_DETECT_OUTPUTS].value ().set (true);

    }
    else
    {
	updateOutputDevices ();
    }
}


void
PrivateScreen::updateStartupFeedback ()
{
    if (!startupSequences.empty ())
	XDefineCursor (dpy, root, busyCursor);
    else
	XDefineCursor (dpy, root, normalCursor);
}

#define STARTUP_TIMEOUT_DELAY 15000

bool
PrivateScreen::handleStartupSequenceTimeout()
{
    struct timeval	now, active;
    double		elapsed;

    gettimeofday (&now, NULL);

    foreach (CompStartupSequence *s, startupSequences)
    {
	sn_startup_sequence_get_last_active_time (s->sequence,
						  &active.tv_sec,
						  &active.tv_usec);

	elapsed = ((((double) now.tv_sec - active.tv_sec) * 1000000.0 +
		    (now.tv_usec - active.tv_usec))) / 1000.0;

	if (elapsed > STARTUP_TIMEOUT_DELAY)
	    sn_startup_sequence_complete (s->sequence);
    }

    return true;
}

void
PrivateScreen::addSequence (SnStartupSequence *sequence)
{
    CompStartupSequence *s;

    s = new CompStartupSequence ();
    if (!s)
	return;

    sn_startup_sequence_ref (sequence);

    s->sequence = sequence;
    s->viewportX = vp.x ();
    s->viewportY = vp.y ();

    startupSequences.push_front (s);

    if (!startupSequenceTimer.active ())
	startupSequenceTimer.start (
	    boost::bind (&PrivateScreen::handleStartupSequenceTimeout, this),
	    1000, 1500);

    updateStartupFeedback ();
}

void
PrivateScreen::removeSequence (SnStartupSequence *sequence)
{
    CompStartupSequence *s = NULL;

    std::list<CompStartupSequence *>::iterator it = startupSequences.begin ();

    for (; it != startupSequences.end (); it++)
    {
	if ((*it)->sequence == sequence)
	{
	    s = (*it);
	    break;
	}
    }

    if (!s)
	return;

    sn_startup_sequence_unref (sequence);

    startupSequences.erase (it);

    if (startupSequences.empty () && startupSequenceTimer.active ())
	startupSequenceTimer.stop ();

    updateStartupFeedback ();
}

void
CompScreen::compScreenSnEvent (SnMonitorEvent *event,
			       void           *userData)
{
    CompScreen	      *screen = (CompScreen *) userData;
    SnStartupSequence *sequence;

    sequence = sn_monitor_event_get_startup_sequence (event);

    switch (sn_monitor_event_get_type (event)) {
    case SN_MONITOR_EVENT_INITIATED:
	screen->priv->addSequence (sequence);
	break;
    case SN_MONITOR_EVENT_COMPLETED:
	screen->priv->removeSequence (sn_monitor_event_get_startup_sequence (event));
	break;
    case SN_MONITOR_EVENT_CHANGED:
    case SN_MONITOR_EVENT_CANCELED:
	break;
    }
}

void
PrivateScreen::updateScreenEdges ()
{
    struct screenEdgeGeometry {
	int xw, x0;
	int yh, y0;
	int ww, w0;
	int hh, h0;
    } geometry[SCREEN_EDGE_NUM] = {
	{ 0,  0,   0,  2,   0,  2,   1, -4 }, /* left */
	{ 1, -2,   0,  2,   0,  2,   1, -4 }, /* right */
	{ 0,  2,   0,  0,   1, -4,   0,  2 }, /* top */
	{ 0,  2,   1, -2,   1, -4,   0,  2 }, /* bottom */
	{ 0,  0,   0,  0,   0,  2,   0,  2 }, /* top-left */
	{ 1, -2,   0,  0,   0,  2,   0,  2 }, /* top-right */
	{ 0,  0,   1, -2,   0,  2,   0,  2 }, /* bottom-left */
	{ 1, -2,   1, -2,   0,  2,   0,  2 }  /* bottom-right */
    };
    int i;

    for (i = 0; i < SCREEN_EDGE_NUM; i++)
    {
	if (screenEdge[i].id)
	    XMoveResizeWindow (dpy, screenEdge[i].id,
			       geometry[i].xw * size.width () + geometry[i].x0,
			       geometry[i].yh * size.height () + geometry[i].y0,
			       geometry[i].ww * size.width () + geometry[i].w0,
			       geometry[i].hh * size.height () + geometry[i].h0);
    }
}


void
PrivateScreen::setCurrentOutput (unsigned int outputNum)
{
    if (outputNum >= priv->outputDevs.size ())
	outputNum = 0;

    priv->currentOutputDev = outputNum;
}

void
PrivateScreen::reshape (int w, int h)
{
    updateScreenInfo();

    region.rects = &region.extents;
    region.numRects = 1;
    region.extents.x1 = 0;
    region.extents.y1 = 0;
    region.extents.x2 = w;
    region.extents.y2 = h;
    region.size = 1;

    size.setWidth (w);
    size.setHeight (h);

    fullscreenOutput.setId ("fullscreen", ~0);
    fullscreenOutput.setGeometry (0, 0, w, h);

    updateScreenEdges ();
}

void
PrivateScreen::configure (XConfigureEvent *ce)
{
    if (priv->attrib.width  != ce->width ||
	priv->attrib.height != ce->height)
    {
	priv->attrib.width  = ce->width;
	priv->attrib.height = ce->height;

	priv->reshape (ce->width, ce->height);

	priv->detectOutputDevices ();
    }
}



void
PrivateScreen::setSupportingWmCheck ()
{
    XChangeProperty (dpy, grabWindow,
		     Atoms::supportingWmCheck,
		     XA_WINDOW, 32, PropModeReplace,
		     (unsigned char *) &grabWindow, 1);

    XChangeProperty (dpy, grabWindow, Atoms::wmName,
		     Atoms::utf8String, 8, PropModeReplace,
		     (unsigned char *) PACKAGE, strlen (PACKAGE));
    XChangeProperty (dpy, grabWindow, Atoms::winState,
		     XA_ATOM, 32, PropModeReplace,
		     (unsigned char *) &Atoms::winStateSkipTaskbar,
		     1);
    XChangeProperty (dpy, grabWindow, Atoms::winState,
		     XA_ATOM, 32, PropModeAppend,
		     (unsigned char *) &Atoms::winStateSkipPager, 1);
    XChangeProperty (dpy, grabWindow, Atoms::winState,
		     XA_ATOM, 32, PropModeAppend,
		     (unsigned char *) &Atoms::winStateHidden, 1);

    XChangeProperty (dpy, root, Atoms::supportingWmCheck,
		     XA_WINDOW, 32, PropModeReplace,
		     (unsigned char *) &grabWindow, 1);
}

void
PrivateScreen::setSupported ()
{
    Atom	data[256];
    int		i = 0;

    data[i++] = Atoms::supported;
    data[i++] = Atoms::supportingWmCheck;

    data[i++] = Atoms::utf8String;

    data[i++] = Atoms::clientList;
    data[i++] = Atoms::clientListStacking;

    data[i++] = Atoms::winActive;

    data[i++] = Atoms::desktopViewport;
    data[i++] = Atoms::desktopGeometry;
    data[i++] = Atoms::currentDesktop;
    data[i++] = Atoms::numberOfDesktops;
    data[i++] = Atoms::showingDesktop;

    data[i++] = Atoms::workarea;

    data[i++] = Atoms::wmName;
/*
    data[i++] = Atoms::wmVisibleName;
*/

    data[i++] = Atoms::wmStrut;
    data[i++] = Atoms::wmStrutPartial;

/*
    data[i++] = Atoms::wmPid;
*/

    data[i++] = Atoms::wmUserTime;
    data[i++] = Atoms::frameExtents;
    data[i++] = Atoms::frameWindow;

    data[i++] = Atoms::winState;
    data[i++] = Atoms::winStateModal;
    data[i++] = Atoms::winStateSticky;
    data[i++] = Atoms::winStateMaximizedVert;
    data[i++] = Atoms::winStateMaximizedHorz;
    data[i++] = Atoms::winStateShaded;
    data[i++] = Atoms::winStateSkipTaskbar;
    data[i++] = Atoms::winStateSkipPager;
    data[i++] = Atoms::winStateHidden;
    data[i++] = Atoms::winStateFullscreen;
    data[i++] = Atoms::winStateAbove;
    data[i++] = Atoms::winStateBelow;
    data[i++] = Atoms::winStateDemandsAttention;

    data[i++] = Atoms::winOpacity;
    data[i++] = Atoms::winBrightness;

#warning fixme
#if 0
    if (canDoSaturated)
    {
	data[i++] = Atoms::winSaturation;
	data[i++] = Atoms::winStateDisplayModal;
    }
#endif

    data[i++] = Atoms::wmAllowedActions;

    data[i++] = Atoms::winActionMove;
    data[i++] = Atoms::winActionResize;
    data[i++] = Atoms::winActionStick;
    data[i++] = Atoms::winActionMinimize;
    data[i++] = Atoms::winActionMaximizeHorz;
    data[i++] = Atoms::winActionMaximizeVert;
    data[i++] = Atoms::winActionFullscreen;
    data[i++] = Atoms::winActionClose;
    data[i++] = Atoms::winActionShade;
    data[i++] = Atoms::winActionChangeDesktop;
    data[i++] = Atoms::winActionAbove;
    data[i++] = Atoms::winActionBelow;

    data[i++] = Atoms::winType;
    data[i++] = Atoms::winTypeDesktop;
    data[i++] = Atoms::winTypeDock;
    data[i++] = Atoms::winTypeToolbar;
    data[i++] = Atoms::winTypeMenu;
    data[i++] = Atoms::winTypeSplash;
    data[i++] = Atoms::winTypeDialog;
    data[i++] = Atoms::winTypeUtil;
    data[i++] = Atoms::winTypeNormal;

    data[i++] = Atoms::wmDeleteWindow;
    data[i++] = Atoms::wmPing;

    data[i++] = Atoms::wmMoveResize;
    data[i++] = Atoms::moveResizeWindow;
    data[i++] = Atoms::restackWindow;

    XChangeProperty (dpy, root, Atoms::supported,
		     XA_ATOM, 32, PropModeReplace, (unsigned char *) data, i);
}

void
PrivateScreen::getDesktopHints ()
{
    unsigned long data[2];
    Atom	  actual;
    int		  result, format;
    unsigned long n, left;
    unsigned char *propData;

    result = XGetWindowProperty (dpy, root,
				 Atoms::numberOfDesktops,
				 0L, 1L, FALSE, XA_CARDINAL, &actual, &format,
				 &n, &left, &propData);

    if (result == Success && n && propData && useDesktopHints)
    {
	memcpy (data, propData, sizeof (unsigned long));
	XFree (propData);

	if (data[0] > 0 && data[0] < 0xffffffff)
	    nDesktop = data[0];
    }

    result = XGetWindowProperty (dpy, root,
				 Atoms::desktopViewport, 0L, 2L,
				 FALSE, XA_CARDINAL, &actual, &format,
				 &n, &left, &propData);

    if (result == Success && n && propData)
    {
	if (n == 2)
	{
	    memcpy (data, propData, sizeof (unsigned long) * 2);

	    if (data[0] / size.width () < vpSize.width () - 1)
		vp.setX (data[0] / size.width ());

	    if (data[1] / size.height () < vpSize.height () - 1)
		vp.setY (data[1] / size.height ());
	}

	XFree (propData);
    }

    result = XGetWindowProperty (dpy, root,
				 Atoms::currentDesktop,
				 0L, 1L, FALSE, XA_CARDINAL, &actual, &format,
				 &n, &left, &propData);

    if (result == Success && n && propData && useDesktopHints)
    {
	memcpy (data, propData, sizeof (unsigned long));
	XFree (propData);

	if (data[0] < nDesktop)
	    currentDesktop = data[0];
    }

    result = XGetWindowProperty (dpy, root,
				 Atoms::showingDesktop,
				 0L, 1L, FALSE, XA_CARDINAL, &actual, &format,
				 &n, &left, &propData);

    if (result == Success && n && propData)
    {
	memcpy (data, propData, sizeof (unsigned long));
	XFree (propData);

	if (data[0])
	    screen->enterShowDesktopMode ();
    }

    data[0] = currentDesktop;

    XChangeProperty (dpy, root, Atoms::currentDesktop,
		     XA_CARDINAL, 32, PropModeReplace,
		     (unsigned char *) data, 1);

    data[0] = showingDesktopMask ? TRUE : FALSE;

    XChangeProperty (dpy, root, Atoms::showingDesktop,
		     XA_CARDINAL, 32, PropModeReplace,
		     (unsigned char *) data, 1);
}

void
CompScreen::enterShowDesktopMode ()
{
    WRAPABLE_HND_FUNC(14, enterShowDesktopMode)


    unsigned long data = 1;
    int		  count = 0;
    CompOption    &st = priv->opt[COMP_OPTION_HIDE_SKIP_TASKBAR_WINDOWS];

    priv->showingDesktopMask = ~(CompWindowTypeDesktopMask |
				CompWindowTypeDockMask);

    foreach (CompWindow *w, priv->windows)
    {
	if ((priv->showingDesktopMask & w->wmType ()) &&
	    (!(w->state () & CompWindowStateSkipTaskbarMask) ||
	    (st.value ().b ())))
	{
	    if (!w->inShowDesktopMode () && !w->grabbed () &&
		w->managed () && w->focus ())
	    {
		w->setShowDesktopMode (true);
		w->hide ();
	    }
	}

	if (w->inShowDesktopMode ())
	    count++;
    }

    if (!count)
    {
	priv->showingDesktopMask = 0;
	data = 0;
    }

    XChangeProperty (priv->dpy, priv->root,
		     Atoms::showingDesktop,
		     XA_CARDINAL, 32, PropModeReplace,
		     (unsigned char *) &data, 1);
}

void
CompScreen::leaveShowDesktopMode (CompWindow *window)
{
    WRAPABLE_HND_FUNC(15, leaveShowDesktopMode, window)

    unsigned long data = 0;

    if (window)
    {
	if (!window->inShowDesktopMode ())
	    return;

	window->setShowDesktopMode (false);
	window->show ();

	/* return if some other window is still in show desktop mode */
	foreach (CompWindow *w, priv->windows)
	    if (w->inShowDesktopMode ())
		return;

	priv->showingDesktopMask = 0;
    }
    else
    {
	priv->showingDesktopMask = 0;

	foreach (CompWindow *w, priv->windows)
	{
	    if (!w->inShowDesktopMode ())
		continue;

	    w->setShowDesktopMode (false);
	    w->show ();
	}

	/* focus default window - most likely this will be the window
	   which had focus before entering showdesktop mode */
	focusDefaultWindow ();
    }

    XChangeProperty (priv->dpy, priv->root,
		     Atoms::showingDesktop,
		     XA_CARDINAL, 32, PropModeReplace,
		     (unsigned char *) &data, 1);
}

void
CompScreen::forEachWindow (CompWindow::ForEach proc)
{
    foreach (CompWindow *w, priv->windows)
	proc (w);
}

void
CompScreen::focusDefaultWindow ()
{
    CompWindow  *w;
    CompWindow  *focus = NULL;

    if (!priv->opt[COMP_OPTION_CLICK_TO_FOCUS].value ().b ())
    {
	w = findTopLevelWindow (priv->below);
	if (w && !(w->type () & (CompWindowTypeDesktopMask |
				 CompWindowTypeDockMask)))
	{
	    if (w->focus ())
		focus = w;
	}
    }

    if (!focus)
    {
	for (CompWindowList::reverse_iterator rit = priv->windows.rbegin ();
	     rit != priv->windows.rend (); rit++)
	{
	    w = (*rit);

	    if (w->type () & CompWindowTypeDockMask)
		continue;

	    if (w->focus ())
	    {
		if (focus)
		{
		    if (w->type () & (CompWindowTypeNormalMask |
				      CompWindowTypeDialogMask |
				      CompWindowTypeModalDialogMask))
		    {
			if (PrivateWindow::compareWindowActiveness (focus, w) < 0)
			    focus = w;
		    }
		}
		else
		    focus = w;
	    }
	}
    }

    if (focus)
    {
	if (focus->id () != priv->activeWindow)
	    focus->moveInputFocusTo ();
    }
    else
    {
	XSetInputFocus (priv->dpy, priv->root, RevertToPointerRoot,
			CurrentTime);
    }
}

CompWindow *
CompScreen::findWindow (Window id)
{
    if (lastFoundWindow && lastFoundWindow->id () == id)
    {
	return lastFoundWindow;
    }
    else
    {
        CompWindow::Map::iterator it = priv->windowsMap.find (id);

        if (it != priv->windowsMap.end ())
            return (lastFoundWindow = it->second);
    }

    return 0;
}

CompWindow *
CompScreen::findTopLevelWindow (Window id, bool override_redirect)
{
    CompWindow *w;

    w = findWindow (id);

    if (w)
    {
	if (w->overrideRedirect () && !override_redirect)
	    return NULL;
	else
	    return w;
    }

    foreach (CompWindow *w, priv->windows)
	if (w->frame () == id)
	{
	    if (w->overrideRedirect () && !override_redirect)
		return NULL;
	    else
		return w;
	}

    return NULL;
}

void
CompScreen::insertWindow (CompWindow *w, Window	aboveId)
{
    w->prev = NULL;
    w->next = NULL;

    if (!aboveId || priv->windows.empty ())
    {
	if (!priv->windows.empty ())
	{
	    priv->windows.front ()->prev = w;
	    w->next = priv->windows.front ();
	}
	priv->windows.push_front (w);
        if (w->id () != 1)
            priv->windowsMap[w->id ()] = w;

	return;
    }

    CompWindowList::iterator it = priv->windows.begin ();

    while (it != priv->windows.end ())
    {
	if ((*it)->id () == aboveId || ((*it)->frame () && (*it)->frame () == aboveId))
	    break;
	it++;
    }

    if (it == priv->windows.end ())
    {
#ifdef DEBUG
	abort ();
#endif
	return;
    }

    w->next = (*it)->next;
    w->prev = (*it);
    (*it)->next = w;

    if (w->next)
    {
	w->next->prev = w;
    }

    priv->windows.insert (++it, w);
    if (w->id () != 1)
        priv->windowsMap[w->id ()] = w;
}

void
PrivateScreen::eraseWindowFromMap (Window id)
{
    if (id != 1)
        priv->windowsMap.erase (id);
}

void
CompScreen::unhookWindow (CompWindow *w)
{
    CompWindowList::iterator it =
	std::find (priv->windows.begin (), priv->windows.end (), w);

    priv->windows.erase (it);
    priv->eraseWindowFromMap (w->id ());

    if (w->next)
	w->next->prev = w->prev;

    if (w->prev)
	w->prev->next = w->next;

    w->next = NULL;
    w->prev = NULL;

    if (w == lastFoundWindow)
	lastFoundWindow = NULL;
}

#define POINTER_GRAB_MASK (ButtonReleaseMask | \
			   ButtonPressMask   | \
			   PointerMotionMask)
CompScreen::GrabHandle
CompScreen::pushGrab (Cursor cursor, const char *name)
{
    if (priv->grabs.empty())
    {
	int status;

	status = XGrabPointer (priv->dpy, priv->grabWindow, TRUE,
			       POINTER_GRAB_MASK,
			       GrabModeAsync, GrabModeAsync,
			       priv->root, cursor,
			       CurrentTime);

	if (status == GrabSuccess)
	{
	    status = XGrabKeyboard (priv->dpy,
				    priv->grabWindow, TRUE,
				    GrabModeAsync, GrabModeAsync,
				    CurrentTime);
	    if (status != GrabSuccess)
	    {
		XUngrabPointer (priv->dpy, CurrentTime);
		return NULL;
	    }
	}
	else
	    return NULL;
    }
    else
    {
	XChangeActivePointerGrab (priv->dpy, POINTER_GRAB_MASK,
				  cursor, CurrentTime);
    }

    PrivateScreen::Grab *grab = new PrivateScreen::Grab ();
    grab->cursor = cursor;
    grab->name   = name;

    priv->grabs.push_back (grab);

    return grab;
}

void
CompScreen::updateGrab (CompScreen::GrabHandle handle, Cursor cursor)
{
    if (!handle)
	return;

    XChangeActivePointerGrab (priv->dpy, POINTER_GRAB_MASK,
			      cursor, CurrentTime);

    ((PrivateScreen::Grab *) handle)->cursor = cursor;
}

void
CompScreen::removeGrab (CompScreen::GrabHandle handle,
			CompPoint *restorePointer)
{
    if (!handle)
	return;

    std::list<PrivateScreen::Grab *>::iterator it;

    it = std::find (priv->grabs.begin (), priv->grabs.end (), handle);

    if (it != priv->grabs.end ())
    {
	priv->grabs.erase (it);
	delete (static_cast<PrivateScreen::Grab *> (handle));
    }
    if (!priv->grabs.empty ())
    {
	XChangeActivePointerGrab (priv->dpy,
				  POINTER_GRAB_MASK,
				  priv->grabs.back ()->cursor,
				  CurrentTime);
    }
    else
    {
	    if (restorePointer)
		warpPointer (restorePointer->x () - pointerX,
			     restorePointer->y () - pointerY);

	    XUngrabPointer (priv->dpy, CurrentTime);
	    XUngrabKeyboard (priv->dpy, CurrentTime);
    }
}

/* otherScreenGrabExist takes a series of strings terminated by a NULL.
   It returns TRUE if a grab exists but it is NOT held by one of the
   plugins listed, returns FALSE otherwise. */

bool
CompScreen::otherGrabExist (const char *first, ...)
{
    va_list    ap;
    const char *name;

    std::list<PrivateScreen::Grab *>::iterator it;

    for (it = priv->grabs.begin (); it != priv->grabs.end (); it++)
    {
	va_start (ap, first);

	name = first;
	while (name)
	{
	    if (strcmp (name, (*it)->name) == 0)
		break;

	    name = va_arg (ap, const char *);
	}

	va_end (ap);

	if (!name)
	return true;
    }

    return false;
}

void
PrivateScreen::grabUngrabOneKey (unsigned int modifiers,
				 int          keycode,
				 bool         grab)
{
    if (grab)
    {
	XGrabKey (dpy,
		  keycode,
		  modifiers,
		  root,
		  TRUE,
		  GrabModeAsync,
		  GrabModeAsync);
    }
    else
    {
	XUngrabKey (dpy,
		    keycode,
		    modifiers,
		    root);
    }
}

bool
PrivateScreen::grabUngrabKeys (unsigned int modifiers,
			       int          keycode,
			       bool         grab)
{
    int             mod, k;
    unsigned int    ignore;

    CompScreen::checkForError (dpy);

    for (ignore = 0; ignore <= ignoredModMask; ignore++)
    {
	if (ignore & ~ignoredModMask)
	    continue;

	if (keycode != 0)
	{
	    grabUngrabOneKey (modifiers | ignore, keycode, grab);
	}
	else
	{
	    for (mod = 0; mod < 8; mod++)
	    {
		if (modifiers & (1 << mod))
		{
		    for (k = mod * modMap->max_keypermod;
			 k < (mod + 1) * modMap->max_keypermod;
			 k++)
		    {
			if (modMap->modifiermap[k])
			{
			    grabUngrabOneKey ((modifiers & ~(1 << mod)) |
					      ignore,
					      modMap->modifiermap[k],
					      grab);
			}
		    }
		}
	    }
	}

	if (CompScreen::checkForError (dpy))
	    return false;
    }

    return true;
}

bool
PrivateScreen::addPassiveKeyGrab (CompAction::KeyBinding &key)
{
    KeyGrab                      newKeyGrab;
    unsigned int                 mask;
    std::list<KeyGrab>::iterator it;

    mask = virtualToRealModMask (key.modifiers ());

    for (it = keyGrabs.begin (); it != keyGrabs.end (); it++)
    {
	if (key.keycode () == (*it).keycode &&
	    mask           == (*it).modifiers)
	{
	    (*it).count++;
	    return true;
	}
    }



    if (!(mask & CompNoMask))
    {
	if (!grabUngrabKeys (mask, key.keycode (), true))
	    return false;
    }

    newKeyGrab.keycode   = key.keycode ();
    newKeyGrab.modifiers = mask;
    newKeyGrab.count     = 1;

    keyGrabs.push_back (newKeyGrab);

    return true;
}

void
PrivateScreen::removePassiveKeyGrab (CompAction::KeyBinding &key)
{
    unsigned int                 mask;
    std::list<KeyGrab>::iterator it;

    mask = virtualToRealModMask (key.modifiers ());

    for (it = keyGrabs.begin (); it != keyGrabs.end (); it++)
    {
	if (key.keycode () == (*it).keycode &&
	    mask           == (*it).modifiers)
	{
	    (*it).count--;
	    if ((*it).count)
		return;

	    it = keyGrabs.erase (it);

	    if (!(mask & CompNoMask))
		grabUngrabKeys (mask, key.keycode (), false);
	}
    }
}

void
PrivateScreen::updatePassiveKeyGrabs ()
{
    std::list<KeyGrab>::iterator it;

    XUngrabKey (dpy, AnyKey, AnyModifier, root);

    for (it = keyGrabs.begin (); it != keyGrabs.end (); it++)
    {
	if (!((*it).modifiers & CompNoMask))
	{
	    grabUngrabKeys ((*it).modifiers,
			    (*it).keycode, true);
	}
    }
}

bool
PrivateScreen::addPassiveButtonGrab (CompAction::ButtonBinding &button)
{
    ButtonGrab                      newButtonGrab;
    std::list<ButtonGrab>::iterator it;

    for (it = buttonGrabs.begin (); it != buttonGrabs.end (); it++)
    {
	if (button.button ()    == (*it).button &&
	    button.modifiers () == (*it).modifiers)
	{
	    (*it).count++;
	    return true;
	}
    }

    newButtonGrab.button    = button.button ();
    newButtonGrab.modifiers = button.modifiers ();
    newButtonGrab.count     = 1;

    buttonGrabs.push_back (newButtonGrab);

    return true;
}

void
PrivateScreen::removePassiveButtonGrab (CompAction::ButtonBinding &button)
{
    std::list<ButtonGrab>::iterator it;

    for (it = buttonGrabs.begin (); it != buttonGrabs.end (); it++)
    {
	if (button.button ()    == (*it).button &&
	    button.modifiers () == (*it).modifiers)
	{
	    (*it).count--;
	    if ((*it).count)
		return;

	    it = buttonGrabs.erase (it);
	}
    }
}

bool
CompScreen::addAction (CompAction *action)
{
    if (action->type () & CompAction::BindingTypeKey)
    {
	if (!priv->addPassiveKeyGrab (action->key ()))
	    return true;
    }

    if (action->type () & CompAction::BindingTypeButton)
    {
	if (!priv->addPassiveButtonGrab (action->button ()))
	{
	    if (action->type () & CompAction::BindingTypeKey)
		priv->removePassiveKeyGrab (action->key ());

	    return true;
	}
    }

    if (action->edgeMask ())
    {
	int i;

	for (i = 0; i < SCREEN_EDGE_NUM; i++)
	    if (action->edgeMask () & (1 << i))
		priv->enableEdge (i);
    }

    return true;
}

void
CompScreen::removeAction (CompAction *action)
{
    if (action->type () & CompAction::BindingTypeKey)
	priv->removePassiveKeyGrab (action->key ());

    if (action->type () & CompAction::BindingTypeButton)
	priv->removePassiveButtonGrab (action->button ());

    if (action->edgeMask ())
    {
	int i;

	for (i = 0; i < SCREEN_EDGE_NUM; i++)
	    if (action->edgeMask () & (1 << i))
		priv->disableEdge (i);
    }
}

void
PrivateScreen::computeWorkareaForBox (BoxPtr     pBox,
				      XRectangle *area)
{
    Region     region;
    REGION     r;
    int	       x1, y1, x2, y2;

    region = XCreateRegion ();
    if (!region)
    {
	area->x      = pBox->x1;
	area->y      = pBox->y1;
	area->width  = pBox->x1 - pBox->x1;
	area->height = pBox->y2 - pBox->y1;

	return;
    }

    r.rects    = &r.extents;
    r.numRects = r.size = 1;
    r.extents  = *pBox;

    XUnionRegion (&r, region, region);

    foreach (CompWindow *w, windows)
    {
	if (!w->mapNum ())
	    continue;

	if (w->struts ())
	{
	    r.extents.y1 = pBox->y1;
	    r.extents.y2 = pBox->y2;

	    x1 = w->struts ()->left.x;
	    y1 = w->struts ()->left.y;
	    x2 = x1 + w->struts ()->left.width;
	    y2 = y1 + w->struts ()->left.height;

	    if (y1 < pBox->y2 && y2 > pBox->y1)
	    {
		r.extents.x1 = x1;
		r.extents.x2 = x2;

		XSubtractRegion (region, &r, region);
	    }

	    x1 = w->struts ()->right.x;
	    y1 = w->struts ()->right.y;
	    x2 = x1 + w->struts ()->right.width;
	    y2 = y1 + w->struts ()->right.height;

	    if (y1 < pBox->y2 && y2 > pBox->y1)
	    {
		r.extents.x1 = x1;
		r.extents.x2 = x2;

		XSubtractRegion (region, &r, region);
	    }

	    r.extents.x1 = pBox->x1;
	    r.extents.x2 = pBox->x2;

	    x1 = w->struts ()->top.x;
	    y1 = w->struts ()->top.y;
	    x2 = x1 + w->struts ()->top.width;
	    y2 = y1 + w->struts ()->top.height;

	    if (x1 < pBox->x2 && x2 > pBox->x1)
	    {
		r.extents.y1 = y1;
		r.extents.y2 = y2;

		XSubtractRegion (region, &r, region);
	    }

	    x1 = w->struts ()->bottom.x;
	    y1 = w->struts ()->bottom.y;
	    x2 = x1 + w->struts ()->bottom.width;
	    y2 = y1 + w->struts ()->bottom.height;

	    if (x1 < pBox->x2 && x2 > pBox->x1)
	    {
		r.extents.y1 = y1;
		r.extents.y2 = y2;

		XSubtractRegion (region, &r, region);
	    }
	}
    }

    area->x      = region->extents.x1;
    area->y      = region->extents.y1;
    area->width  = region->extents.x2 - region->extents.x1;
    area->height = region->extents.y2 - region->extents.y1;

    XDestroyRegion (region);
}

void
PrivateScreen::updateWorkarea ()
{
    XRectangle workArea;
    BoxRec     box;
    bool       workAreaChanged = false;

    for (unsigned int i = 0; i < priv->outputDevs.size (); i++)
    {
	XRectangle oldWorkArea = priv->outputDevs[i].workArea ();

	priv->computeWorkareaForBox (&priv->outputDevs[i].region ()->extents,
				     &workArea);

	if (memcmp (&workArea, &oldWorkArea, sizeof (XRectangle)))
	{
	    workAreaChanged = true;
	    priv->outputDevs[i].setWorkArea (workArea);
	}
    }

    box.x1 = 0;
    box.y1 = 0;
    box.x2 = priv->size.width ();
    box.y2 = priv->size.height ();

    priv->computeWorkareaForBox (&box, &workArea);

    if (memcmp (&workArea, &priv->workArea, sizeof (XRectangle)))
    {
	workAreaChanged = true;
	priv->workArea = workArea;

	priv->setDesktopHints ();
    }

    if (workAreaChanged)
    {
	/* as work area changed, update all maximized windows on this
	   screen to snap to the new work area */
	foreach (CompWindow *w, priv->windows)
	    w->priv->updateSize ();
    }
}

static bool
isClientListWindow (CompWindow *w)
{
    /* windows with client id less than 2 have been destroyed and only exists
       because some plugin keeps a reference to them. they should not be in
       client lists */
    if (w->id () < 2)
	return false;

    if (w->overrideRedirect ())
	return false;

    if (!w->isViewable ())
    {
	if (!(w->state () & CompWindowStateHiddenMask))
	    return false;
    }

    return true;
}

static void
countClientListWindow (CompWindow *w,
		       int        *n)
{
    if (isClientListWindow (w))
    {
	*n = *n + 1;
    }
}

static int
compareMappingOrder (const void *w1,
		     const void *w2)
{
    return (*((CompWindow **) w1))->mapNum () -
	   (*((CompWindow **) w2))->mapNum ();
}

void
PrivateScreen::updateClientList ()
{
    Window *clientList;
    Window *clientListStacking;
    Bool   updateClientList = true;
    Bool   updateClientListStacking = true;
    int	   i, n = 0;

    screen->forEachWindow (boost::bind (countClientListWindow, _1, &n));

    if (n == 0)
    {
	if (n != priv->nClientList)
	{
	    free (priv->clientList);

	    priv->clientList  = NULL;
	    priv->nClientList = 0;

	    XChangeProperty (priv->dpy, priv->root,
			     Atoms::clientList,
			     XA_WINDOW, 32, PropModeReplace,
			     (unsigned char *) &priv->grabWindow, 1);
	    XChangeProperty (priv->dpy, priv->root,
			     Atoms::clientListStacking,
			     XA_WINDOW, 32, PropModeReplace,
			     (unsigned char *) &priv->grabWindow, 1);
	}

	return;
    }

    if (n != priv->nClientList)
    {
	CompWindow **list;

	list = (CompWindow **)
	    realloc (priv->clientList, (sizeof (CompWindow *) +
		     sizeof (Window) * 2) * n);
	if (!list)
	    return;

	priv->clientList  = list;
	priv->nClientList = n;

	updateClientList = updateClientListStacking = true;
    }

    clientList	       = (Window *) (priv->clientList + n);
    clientListStacking = clientList + n;

    i = 0;
    foreach (CompWindow *w, priv->windows)
	if (isClientListWindow (w))
	{
	    priv->clientList[i] = w;
	    i++;
	}

    for (i = 0; i < n; i++)
    {
	if (!updateClientListStacking)
	{
	    if (clientListStacking[i] != priv->clientList[i]->id ())
		updateClientListStacking = true;
	}

	clientListStacking[i] = priv->clientList[i]->id ();
    }

    /* sort window list in mapping order */
    qsort (priv->clientList, n, sizeof (CompWindow *), compareMappingOrder);

    for (i = 0; i < n; i++)
    {
	if (!updateClientList)
	{
	    if (clientList[i] != priv->clientList[i]->id ())
		updateClientList = true;
	}

	clientList[i] = priv->clientList[i]->id ();
    }

    if (updateClientList)
	XChangeProperty (priv->dpy, priv->root,
			 Atoms::clientList,
			 XA_WINDOW, 32, PropModeReplace,
			 (unsigned char *) clientList, priv->nClientList);

    if (updateClientListStacking)
	XChangeProperty (priv->dpy, priv->root,
			 Atoms::clientListStacking,
			 XA_WINDOW, 32, PropModeReplace,
			 (unsigned char *) clientListStacking,
			 priv->nClientList);
}

void
CompScreen::toolkitAction (Atom   toolkitAction,
			   Time   eventTime,
			   Window window,
			   long   data0,
			   long   data1,
			   long   data2)
{
    XEvent ev;

    ev.type		    = ClientMessage;
    ev.xclient.window	    = window;
    ev.xclient.message_type = Atoms::toolkitAction;
    ev.xclient.format	    = 32;
    ev.xclient.data.l[0]    = toolkitAction;
    ev.xclient.data.l[1]    = eventTime;
    ev.xclient.data.l[2]    = data0;
    ev.xclient.data.l[3]    = data1;
    ev.xclient.data.l[4]    = data2;

    XUngrabPointer (priv->dpy, CurrentTime);
    XUngrabKeyboard (priv->dpy, CurrentTime);

    XSendEvent (priv->dpy, priv->root, FALSE,
		StructureNotifyMask, &ev);
}

void
CompScreen::runCommand (CompString command)
{
    if (command.size () == 0)
	return;

    if (fork () == 0)
    {
	size_t     pos;
	CompString env (priv->displayString);

	setsid ();
	
	pos = env.find (':');
	if (pos != std::string::npos)
	{
	    if (env.find ('.', pos) != std::string::npos)
	    {
		env.erase (env.find ('.', pos));
	    }
	    else
	    {
		env.erase (pos);
		env.append (":0");
	    }
	}

	env.append (compPrintf (".%d", priv->screenNum));
	
	putenv (const_cast<char *> (env.c_str ()));

	exit (execl ("/bin/sh", "/bin/sh", "-c", command.c_str (), NULL));
    }
}

void
CompScreen::moveViewport (int tx, int ty, bool sync)
{
    CompPoint pnt;

    tx = priv->vp.x () - tx;
    tx = MOD (tx, priv->vpSize.width ());
    tx -= priv->vp.x ();

    ty = priv->vp.y () - ty;
    ty = MOD (ty, priv->vpSize.height ());
    ty -= priv->vp.y ();

    if (!tx && !ty)
	return;

    priv->vp.setX (priv->vp.x () + tx);
    priv->vp.setY (priv->vp.y () + ty);

    tx *= -priv->size.width ();
    ty *= -priv->size.height ();

    foreach (CompWindow *w, priv->windows)
    {
	if (w->onAllViewports ())
	    continue;

	pnt = w->getMovementForOffset (CompPoint (tx, ty));

	if (w->saveMask () & CWX)
	    w->saveWc ().x += pnt.x ();

	if (w->saveMask () & CWY)
	    w->saveWc ().y += pnt.y ();

	/* move */
	w->move (pnt.x (), pnt.y ());

	if (sync)
	    w->syncPosition ();
    }

    if (sync)
    {
	CompWindow *w;

	priv->setDesktopHints ();

	priv->setCurrentActiveWindowHistory (priv->vp.x (), priv->vp.y ());

	w = findWindow (priv->activeWindow);
	if (w)
	{
	    CompPoint dvp;

	    dvp = w->defaultViewport ();

	    /* add window to current history if it's default viewport is
	       still the current one. */
	    if (priv->vp.x () == dvp.x () && priv->vp.y () == dvp.y ())
		priv->addToCurrentActiveWindowHistory (w->id ());
	}
    }
}

CompGroup *
PrivateScreen::addGroup (Window id)
{
    CompGroup *group = new CompGroup ();

    group->refCnt = 1;
    group->id     = id;

    priv->groups.push_back (group);

    return group;
}

void
PrivateScreen::removeGroup (CompGroup *group)
{
    group->refCnt--;
    if (group->refCnt)
	return;

    std::list<CompGroup *>::iterator it =
	std::find (priv->groups.begin (), priv->groups.end (), group);

    if (it != priv->groups.end ())
    {
	priv->groups.erase (it);
    }

    delete group;
}

CompGroup *
PrivateScreen::findGroup (Window id)
{
    foreach (CompGroup *g, priv->groups)
	if (g->id == id)
	    return g;

    return NULL;
}

void
PrivateScreen::applyStartupProperties (CompWindow *window)
{
    CompStartupSequence *s = NULL;
    const char	        *startupId = window->startupId ();

    if (!startupId)
    {
	CompWindow *leader;

	leader = screen->findWindow (window->clientLeader ());
	if (leader)
	    startupId = leader->startupId ();

	if (!startupId)
	    return;
    }

    foreach (CompStartupSequence *ss, priv->startupSequences)
    {
	const char *id;

	id = sn_startup_sequence_get_id (ss->sequence);
	if (strcmp (id, startupId) == 0)
	{
	    s = ss;
	    break;
	}
    }

    if (s)
	window->priv->applyStartupProperties (s);
}

void
CompScreen::sendWindowActivationRequest (Window id)
{
    XEvent xev;

    xev.xclient.type    = ClientMessage;
    xev.xclient.display = priv->dpy;
    xev.xclient.format  = 32;

    xev.xclient.message_type = Atoms::winActive;
    xev.xclient.window	     = id;

    xev.xclient.data.l[0] = 2;
    xev.xclient.data.l[1] = 0;
    xev.xclient.data.l[2] = 0;
    xev.xclient.data.l[3] = 0;
    xev.xclient.data.l[4] = 0;

    XSendEvent (priv->dpy, priv->root, FALSE,
		SubstructureRedirectMask | SubstructureNotifyMask, &xev);
}


void
PrivateScreen::enableEdge (int edge)
{
    priv->screenEdge[edge].count++;
    if (priv->screenEdge[edge].count == 1)
	XMapRaised (priv->dpy, priv->screenEdge[edge].id);
}

void
PrivateScreen::disableEdge (int edge)
{
    priv->screenEdge[edge].count--;
    if (priv->screenEdge[edge].count == 0)
	XUnmapWindow (priv->dpy, priv->screenEdge[edge].id);
}

Window
PrivateScreen::getTopWindow ()
{
    /* return first window that has not been destroyed */
    for (CompWindowList::reverse_iterator rit = priv->windows.rbegin ();
	     rit != priv->windows.rend (); rit++)
    {
	if ((*rit)->id () > 1)
	    return (*rit)->id ();
    }

    return None;
}

int
CompScreen::outputDeviceForPoint (int x, int y)
{
    return outputDeviceForGeometry (CompWindow::Geometry (x, y, 1, 1, 0));
}

void
CompScreen::getCurrentOutputExtents (int *x1, int *y1, int *x2, int *y2)
{
    if (x1)
	*x1 = priv->outputDevs[priv->currentOutputDev].x1 ();

    if (y1)
	*y1 = priv->outputDevs[priv->currentOutputDev].y1 ();

    if (x2)
	*x2 = priv->outputDevs[priv->currentOutputDev].x2 ();

    if (y2)
	*y2 = priv->outputDevs[priv->currentOutputDev].y2 ();
}

void
PrivateScreen::setNumberOfDesktops (unsigned int nDesktop)
{
    if (nDesktop < 1 || nDesktop >= 0xffffffff)
	return;

    if (nDesktop == priv->nDesktop)
	return;

    if (priv->currentDesktop >= nDesktop)
	priv->currentDesktop = nDesktop - 1;

    foreach (CompWindow *w, priv->windows)
    {
	if (w->desktop () == 0xffffffff)
	    continue;

	if (w->desktop () >= nDesktop)
	    w->priv->setDesktop (nDesktop - 1);
    }

    priv->nDesktop = nDesktop;

    priv->setDesktopHints ();
}

void
PrivateScreen::setCurrentDesktop (unsigned int desktop)
{
    unsigned long data;

    if (desktop >= priv->nDesktop)
	return;

    if (desktop == priv->currentDesktop)
	return;

    priv->currentDesktop = desktop;

    foreach (CompWindow *w, priv->windows)
    {
	if (w->desktop () == 0xffffffff)
	    continue;

	if (w->desktop () == desktop)
	    w->show ();
	else
	    w->hide ();
    }

    data = desktop;

    XChangeProperty (priv->dpy, priv->root, Atoms::currentDesktop,
		     XA_CARDINAL, 32, PropModeReplace,
		     (unsigned char *) &data, 1);
}

void
CompScreen::getWorkareaForOutput (int output, XRectangle *area)
{
    *area = priv->outputDevs[output].workArea ();
}



void
CompScreen::outputChangeNotify ()
    WRAPABLE_HND_FUNC(16, outputChangeNotify)



/* Returns default viewport for some window geometry. If the window spans
   more than one viewport the most appropriate viewport is returned. How the
   most appropriate viewport is computed can be made optional if necessary. It
   is currently computed as the viewport where the center of the window is
   located, except for when the window is visible in the current viewport as
   the current viewport is then always returned. */
void
CompScreen::viewportForGeometry (CompWindow::Geometry gm,
				 int                  *viewportX,
				 int                  *viewportY)
{
    int	centerX;
    int	centerY;

    gm.setWidth  (gm.width () + (gm.border () * 2));
    gm.setHeight (gm.height () + (gm.border () * 2));

    if ((gm.x () < (int) priv->size.width ()  && gm.x () + gm.width () > 0) &&
	(gm.y () < (int) priv->size.height () && gm.y ()+ gm.height () > 0))
    {
	if (viewportX)
	    *viewportX = priv->vp.x ();

	if (viewportY)
	    *viewportY = priv->vp.y ();

	return;
    }

    if (viewportX)
    {
	centerX = gm.x () + (gm.width () >> 1);
	if (centerX < 0)
	    *viewportX = priv->vp.x () + ((centerX / priv->size.width ()) - 1) %
		priv->vpSize.width ();
	else
	    *viewportX = priv->vp.x () + (centerX / priv->size.width ()) %
		priv->vpSize.width ();
    }

    if (viewportY)
    {
	centerY = gm.y () + (gm.height () >> 1);
	if (centerY < 0)
	    *viewportY = priv->vp.y () +
		((centerY / priv->size.height ()) - 1) % priv->vpSize.height ();
	else
	    *viewportY = priv->vp.y () + (centerY / priv->size.height ()) %
		priv->vpSize.height ();
    }
}

static int
rectangleOverlapArea (BOX *rect1,
		      BOX *rect2)
{
    int left, right, top, bottom;

    /* extents of overlapping rectangle */
    left = MAX (rect1->x1, rect2->x1);
    right = MIN (rect1->x2, rect2->x2);
    top = MAX (rect1->y1, rect2->y1);
    bottom = MIN (rect1->y2, rect2->y2);

    if (left > right || top > bottom)
    {
	/* no overlap */
	return 0;
    }

    return (right - left) * (bottom - top);
}

int
CompScreen::outputDeviceForGeometry (CompWindow::Geometry gm)
{
    int          overlapAreas[priv->outputDevs.size ()];
    int          highest, seen, highestScore;
    int          strategy;
    unsigned int i;
    BOX          geomRect;

    if (priv->outputDevs.size () == 1)
	return 0;

    strategy = priv->opt[COMP_OPTION_OVERLAPPING_OUTPUTS].value ().i ();

    if (strategy == OUTPUT_OVERLAP_MODE_SMART)
    {
	/* for smart mode, calculate the overlap of the whole rectangle
	   with the output device rectangle */
	geomRect.x2 = gm.width () + 2 * gm.border ();
	geomRect.y2 = gm.height () + 2 * gm.border ();

	geomRect.x1 = gm.x () % priv->size.width ();
	if ((geomRect.x1 + geomRect.x2 / 2) < 0)
	    geomRect.x1 += priv->size.width ();
	geomRect.y1 = gm.y () % priv->size.height ();
	if ((geomRect.y1 + geomRect.y2 / 2) < 0)
	    geomRect.y1 += priv->size.height ();

	geomRect.x2 += geomRect.x1;
	geomRect.y2 += geomRect.y1;
    }
    else
    {
	/* for biggest/smallest modes, only use the window center to determine
	   the correct output device */
	geomRect.x1 = (gm.x () + (gm.width () / 2) + gm.border ()) % priv->size.width ();
	if (geomRect.x1 < 0)
	    geomRect.x1 += priv->size.width ();
	geomRect.y1 = (gm.y () + (gm.height () / 2) + gm.border()) % priv->size.height ();
	if (geomRect.y1 < 0)
	    geomRect.y1 += priv->size.height ();

	geomRect.x2 = geomRect.x1 + 1;
	geomRect.y2 = geomRect.y1 + 1;
    }

    /* get amount of overlap on all output devices */
    for (i = 0; i < priv->outputDevs.size (); i++)
	overlapAreas[i] =
		rectangleOverlapArea (&priv->outputDevs[i].region ()->extents,
				      &geomRect);

    /* find output with largest overlap */
    for (i = 0, highest = 0, highestScore = 0; i < priv->outputDevs.size (); i++)
	if (overlapAreas[i] > highestScore)
	{
	    highest = i;
	    highestScore = overlapAreas[i];
	}

    /* look if the highest score is unique */
    for (i = 0, seen = 0; i < priv->outputDevs.size (); i++)
	if (overlapAreas[i] == highestScore)
	    seen++;

    if (seen > 1)
    {
	/* it's not unique, select one output of the matching ones and use the
	   user preferred strategy for that */
	unsigned int currentSize, bestOutputSize;
	Bool         searchLargest;
	
	searchLargest = (strategy != OUTPUT_OVERLAP_MODE_PREFER_SMALLER);
	if (searchLargest)
	    bestOutputSize = 0;
	else
	    bestOutputSize = UINT_MAX;

	for (i = 0, highest = 0; i < priv->outputDevs.size (); i++)
	    if (overlapAreas[i] == highestScore)
	    {
		BOX  *box = &priv->outputDevs[i].region ()->extents;
		Bool bestFit;

		currentSize = (box->x2 - box->x1) * (box->y2 - box->y1);

		if (searchLargest)
		    bestFit = (currentSize > bestOutputSize);
		else
		    bestFit = (currentSize < bestOutputSize);

		if (bestFit)
		{
		    highest = i;
		    bestOutputSize = currentSize;
		}
	    }
    }

    return highest;
}

bool
PrivateScreen::updateDefaultIcon ()
{
    CompString file = priv->opt[COMP_OPTION_DEFAULT_ICON].value ().s ();
    void       *data;
    int        width, height;

    if (priv->defaultIcon)
    {
	delete priv->defaultIcon;
	priv->defaultIcon = NULL;
    }

    if (!screen->readImageFromFile (file.c_str (), &width, &height, &data))
	return false;

    priv->defaultIcon = new CompIcon (screen, width, height);

    memcpy (priv->defaultIcon->data (), data, width * height * sizeof (CARD32));

    free (data);

    return true;
}

void
PrivateScreen::setCurrentActiveWindowHistory (int x, int y)
{
    int	i, min = 0;

    for (i = 0; i < ACTIVE_WINDOW_HISTORY_NUM; i++)
    {
	if (priv->history[i].x == x && priv->history[i].y == y)
	{
	    priv->currentHistory = i;
	    return;
	}
    }

    for (i = 1; i < ACTIVE_WINDOW_HISTORY_NUM; i++)
	if (priv->history[i].activeNum < priv->history[min].activeNum)
	    min = i;

    priv->currentHistory = min;

    priv->history[min].activeNum = priv->activeNum;
    priv->history[min].x         = x;
    priv->history[min].y         = y;

    memset (priv->history[min].id, 0, sizeof (priv->history[min].id));
}

void
PrivateScreen::addToCurrentActiveWindowHistory (Window id)
{
    CompActiveWindowHistory *history = &priv->history[priv->currentHistory];
    Window		    tmp, next = id;
    int			    i;

    /* walk and move history */
    for (i = 0; i < ACTIVE_WINDOW_HISTORY_SIZE; i++)
    {
	tmp = history->id[i];
	history->id[i] = next;
	next = tmp;

	/* we're done when we find an old instance or an empty slot */
	if (tmp == id || tmp == None)
	    break;
    }

    history->activeNum = priv->activeNum;
}

void
ScreenInterface::enterShowDesktopMode ()
    WRAPABLE_DEF (enterShowDesktopMode)

void
ScreenInterface::leaveShowDesktopMode (CompWindow *window)
    WRAPABLE_DEF (leaveShowDesktopMode, window)

void
ScreenInterface::outputChangeNotify ()
    WRAPABLE_DEF (outputChangeNotify)


Window
CompScreen::root ()
{
    return priv->root;
}

void
CompScreen::warpPointer (int dx, int dy)
{
    XEvent      event;

    pointerX += dx;
    pointerY += dy;

    if (pointerX >= (int) priv->size.width ())
	pointerX = priv->size.width () - 1;
    else if (pointerX < 0)
	pointerX = 0;

    if (pointerY >= (int) priv->size.height ())
	pointerY = priv->size.height () - 1;
    else if (pointerY < 0)
	pointerY = 0;

    XWarpPointer (priv->dpy,
		  None, priv->root,
		  0, 0, 0, 0,
		  pointerX, pointerY);

    XSync (priv->dpy, FALSE);

    while (XCheckMaskEvent (priv->dpy,
			    LeaveWindowMask |
			    EnterWindowMask |
			    PointerMotionMask,
			    &event));

    if (!inHandleEvent)
    {
	lastPointerX = pointerX;
	lastPointerY = pointerY;
    }
}

CompWindowList &
CompScreen::windows ()
{
    return priv->windows;
}

Time
CompScreen::getCurrentTime ()
{
    XEvent event;

    XChangeProperty (priv->dpy, priv->grabWindow,
		     XA_PRIMARY, XA_STRING, 8,
		     PropModeAppend, NULL, 0);
    XWindowEvent (priv->dpy, priv->grabWindow,
		  PropertyChangeMask,
		  &event);

    return event.xproperty.time;
}

Window
CompScreen::selectionWindow ()
{
    return priv->wmSnSelectionWindow;
}

int
CompScreen::screenNum ()
{
    return priv->screenNum;
}

CompPoint
CompScreen::vp ()
{
    return priv->vp;
}

CompSize
CompScreen::vpSize ()
{
    return priv->vpSize;
}

CompSize
CompScreen::size ()
{
    return priv->size;
}

int
CompScreen::desktopWindowCount ()
{
    return priv->desktopWindowCount;
}

CompOutput::vector &
CompScreen::outputDevs ()
{
    return priv->outputDevs;
}

XRectangle
CompScreen::workArea ()
{
    return priv->workArea;
}


unsigned int
CompScreen::currentDesktop ()
{
    return priv->currentDesktop;
}

unsigned int
CompScreen::nDesktop ()
{
    return priv->nDesktop;
}

CompActiveWindowHistory *
CompScreen::currentHistory ()
{
    return &priv->history[priv->currentHistory];
}

void
PrivateScreen::removeDestroyed ()
{
    while (pendingDestroys)
    {
	foreach (CompWindow *w, windows)
	{
	    if (w->destroyed ())
	    {
		delete w;
		break;
	    }
	}

	pendingDestroys--;
    }
}

Region
CompScreen::region ()
{
    return &priv->region;
}

bool
CompScreen::hasOverlappingOutputs ()
{
    return priv->hasOverlappingOutputs;
}

CompOutput &
CompScreen::fullscreenOutput ()
{
    return priv->fullscreenOutput;
}


XWindowAttributes
CompScreen::attrib ()
{
    return priv->attrib;
}

std::vector<XineramaScreenInfo> &
CompScreen::screenInfo ()
{
    return priv->screenInfo;
}

CompScreen::CompScreen ():
    CompPrivateStorage (&screenPrivateIndices, 0)
{
    priv = new PrivateScreen (this);
    assert (priv);
}

bool
CompScreen::init (const char *name)
{
    Window               focus;
    int                  revertTo, i;
    int                  xkbOpcode;
    Display              *dpy;
    Window               newWmSnOwner = None;
    Atom                 wmSnAtom = 0;
    Time                 wmSnTimestamp = 0;
    XEvent               event;
    XSetWindowAttributes attr;
    Window               currentWmSnOwner;
    char                 buf[128];
    static char          data = 0;
    XColor               black;
    Pixmap               bitmap;
    XVisualInfo          templ;
    XVisualInfo          *visinfo;
    Window               rootReturn, parentReturn;
    Window               *children;
    unsigned int         nchildren;
    int                  defaultDepth, nvisinfo;
    XSetWindowAttributes attrib;

    CompOption::Value::Vector vList;

    CompPlugin *corePlugin = CompPlugin::load ("core");
    if (!corePlugin)
    {
	compLogMessage ("core", CompLogLevelFatal,
			"Couldn't load core plugin");
	return false;
    }

    if (!CompPlugin::push (corePlugin))
    {
	compLogMessage ("core", CompLogLevelFatal,
			"Couldn't activate core plugin");
	return false;
    }

    CompPrivate p;
    p.uval = CORE_ABIVERSION;
    storeValue ("core_ABI", p);

    vList.push_back ("core");

    priv->plugin.set (CompOption::TypeString, vList);

    dpy = priv->dpy = XOpenDisplay (name);
    if (!priv->dpy)
    {
	compLogMessage ("core", CompLogLevelFatal,
		        "Couldn't open display %s", XDisplayName (name));
	return false;
    }

//    priv->connection = XGetXCBConnection (priv->dpy);

    if (!coreMetadata->initOptions (coreOptionInfo,
				    COMP_OPTION_NUM, priv->opt))
	return true;

    snprintf (priv->displayString, 255, "DISPLAY=%s",
	      DisplayString (dpy));

#ifdef DEBUG
    XSynchronize (priv->dpy, TRUE);
#endif

    Atoms::init (priv->dpy);

    XSetErrorHandler (errorHandler);

    priv->updateModifierMappings ();

    priv->snDisplay = sn_display_new (dpy, NULL, NULL);
    if (!priv->snDisplay)
	return true;

    priv->lastPing = 1;

    if (!XSyncQueryExtension (dpy, &priv->syncEvent, &priv->syncError))
    {
	compLogMessage ("core", CompLogLevelFatal,
		        "No sync extension");
	return false;
    }

    priv->randrExtension = XRRQueryExtension (dpy, &priv->randrEvent,
					      &priv->randrError);

    priv->shapeExtension = XShapeQueryExtension (dpy, &priv->shapeEvent,
						 &priv->shapeError);

    priv->xkbExtension = XkbQueryExtension (dpy, &xkbOpcode,
					    &priv->xkbEvent, &priv->xkbError,
					    NULL, NULL);
    if (priv->xkbExtension)
    {
	XkbSelectEvents (dpy, XkbUseCoreKbd,
			 XkbBellNotifyMask | XkbStateNotifyMask,
			 XkbAllEventsMask);
    }
    else
    {
	compLogMessage ("core", CompLogLevelFatal,
		        "No XKB extension");

	priv->xkbEvent = priv->xkbError = -1;
    }

    priv->xineramaExtension = XineramaQueryExtension (dpy,
						      &priv->xineramaEvent,
						      &priv->xineramaError);


    priv->updateScreenInfo();

    priv->escapeKeyCode =
	XKeysymToKeycode (dpy, XStringToKeysym ("Escape"));
    priv->returnKeyCode =
	XKeysymToKeycode (dpy, XStringToKeysym ("Return"));










    sprintf (buf, "WM_S%d", DefaultScreen (dpy));
    wmSnAtom = XInternAtom (dpy, buf, 0);

    currentWmSnOwner = XGetSelectionOwner (dpy, wmSnAtom);

    if (currentWmSnOwner != None)
    {
	if (!replaceCurrentWm)
	{
	    compLogMessage ("core", CompLogLevelError,
			    "Screen %d on display \"%s\" already "
			    "has a window manager; try using the "
			    "--replace option to replace the current "
			    "window manager.",
			    DefaultScreen (dpy), DisplayString (dpy));

	    return false;
	}

	XSelectInput (dpy, currentWmSnOwner, StructureNotifyMask);
    }

    attr.override_redirect = TRUE;
    attr.event_mask        = PropertyChangeMask;

    newWmSnOwner =
	XCreateWindow (dpy, XRootWindow (dpy, DefaultScreen (dpy)),
		       -100, -100, 1, 1, 0,
		       CopyFromParent, CopyFromParent,
		       CopyFromParent,
		       CWOverrideRedirect | CWEventMask,
		       &attr);

    XChangeProperty (dpy, newWmSnOwner, Atoms::wmName, Atoms::utf8String, 8,
		     PropModeReplace, (unsigned char *) PACKAGE,
		     strlen (PACKAGE));

    XWindowEvent (dpy, newWmSnOwner, PropertyChangeMask, &event);

    wmSnTimestamp = event.xproperty.time;

    XSetSelectionOwner (dpy, wmSnAtom, newWmSnOwner, wmSnTimestamp);

    if (XGetSelectionOwner (dpy, wmSnAtom) != newWmSnOwner)
    {
	compLogMessage ("core", CompLogLevelError,
		        "Could not acquire window manager "
		        "selection on screen %d display \"%s\"",
		        DefaultScreen (dpy), DisplayString (dpy));

	XDestroyWindow (dpy, newWmSnOwner);

	return false;
    }

    /* Send client message indicating that we are now the WM */
    event.xclient.type	       = ClientMessage;
    event.xclient.window       = XRootWindow (dpy, DefaultScreen (dpy));
    event.xclient.message_type = Atoms::manager;
    event.xclient.format       = 32;
    event.xclient.data.l[0]    = wmSnTimestamp;
    event.xclient.data.l[1]    = wmSnAtom;
    event.xclient.data.l[2]    = 0;
    event.xclient.data.l[3]    = 0;
    event.xclient.data.l[4]    = 0;

    XSendEvent (dpy, XRootWindow (dpy, DefaultScreen (dpy)), FALSE,
		StructureNotifyMask, &event);

    /* Wait for old window manager to go away */
    if (currentWmSnOwner != None)
    {
	do {
	    XWindowEvent (dpy, currentWmSnOwner, StructureNotifyMask, &event);
	} while (event.type != DestroyNotify);
    }

    CompScreen::checkForError (dpy);

    XGrabServer (dpy);

    XSelectInput (dpy, XRootWindow (dpy, DefaultScreen (dpy)),
		  SubstructureRedirectMask |
		  SubstructureNotifyMask   |
		  StructureNotifyMask      |
		  PropertyChangeMask       |
		  LeaveWindowMask          |
		  EnterWindowMask          |
		  KeyPressMask             |
		  KeyReleaseMask           |
		  ButtonPressMask          |
		  ButtonReleaseMask        |
		  FocusChangeMask          |
		  ExposureMask);

    if (CompScreen::checkForError (dpy))
    {
	compLogMessage ("core", CompLogLevelError,
		        "Another window manager is "
		        "already running on screen: %d", DefaultScreen (dpy));

	XUngrabServer (dpy);
	return false;
    }

    priv->vpSize.setWidth (priv->opt[COMP_OPTION_HSIZE].value ().i ());
    priv->vpSize.setHeight (priv->opt[COMP_OPTION_VSIZE].value ().i ());

    for (i = 0; i < SCREEN_EDGE_NUM; i++)
    {
	priv->screenEdge[i].id    = None;
	priv->screenEdge[i].count = 0;
    }

    priv->screenNum = DefaultScreen (dpy);
    priv->colormap  = DefaultColormap (dpy, priv->screenNum);
    priv->root	    = XRootWindow (dpy, priv->screenNum);

    priv->snContext = sn_monitor_context_new (priv->snDisplay, priv->screenNum,
					      compScreenSnEvent, this, NULL);

    priv->wmSnSelectionWindow = newWmSnOwner;
    priv->wmSnAtom            = wmSnAtom;
    priv->wmSnTimestamp       = wmSnTimestamp;

    if (!XGetWindowAttributes (dpy, priv->root, &priv->attrib))
	return false;

    priv->workArea.x      = 0;
    priv->workArea.y      = 0;
    priv->workArea.width  = priv->attrib.width;
    priv->workArea.height = priv->attrib.height;
    priv->grabWindow      = None;

    templ.visualid = XVisualIDFromVisual (priv->attrib.visual);

    visinfo = XGetVisualInfo (dpy, VisualIDMask, &templ, &nvisinfo);
    if (!nvisinfo)
    {
	compLogMessage ("core", CompLogLevelFatal,
			"Couldn't get visual info for default visual");
	return false;
    }

    defaultDepth = visinfo->depth;

    black.red = black.green = black.blue = 0;

    if (!XAllocColor (dpy, priv->colormap, &black))
    {
	compLogMessage ("core", CompLogLevelFatal,
			"Couldn't allocate color");
	XFree (visinfo);
	return false;
    }

    bitmap = XCreateBitmapFromData (dpy, priv->root, &data, 1, 1);
    if (!bitmap)
    {
	compLogMessage ("core", CompLogLevelFatal,
			"Couldn't create bitmap");
	XFree (visinfo);
	return false;
    }

    priv->invisibleCursor = XCreatePixmapCursor (dpy, bitmap, bitmap,
						 &black, &black, 0, 0);
    if (!priv->invisibleCursor)
    {
	compLogMessage ("core", CompLogLevelFatal,
			"Couldn't create invisible cursor");
	XFree (visinfo);
	return false;
    }

    XFreePixmap (dpy, bitmap);
    XFreeColors (dpy, priv->colormap, &black.pixel, 1, 0);

    XFree (visinfo);

    priv->reshape (priv->attrib.width, priv->attrib.height);

    priv->detectOutputDevices ();
    priv->updateOutputDevices ();

    priv->getDesktopHints ();

    /* TODO: bailout properly when objectInitPlugins fails */
    assert (CompPlugin::screenInitPlugins (this));

    XQueryTree (dpy, priv->root,
		&rootReturn, &parentReturn,
		&children, &nchildren);

    for (unsigned int i = 0; i < nchildren; i++)
	new CompWindow (children[i], i ? children[i - 1] : 0);

    foreach (CompWindow *w, priv->windows)
    {
	if (w->isViewable ())
	    w->priv->activeNum = priv->activeNum++;
    }

    XFree (children);

    attrib.override_redirect = 1;
    attrib.event_mask	     = PropertyChangeMask;

    priv->grabWindow = XCreateWindow (dpy, priv->root, -100, -100, 1, 1, 0,
				      CopyFromParent, InputOnly, CopyFromParent,
				      CWOverrideRedirect | CWEventMask,
				      &attrib);
    XMapWindow (dpy, priv->grabWindow);

    for (i = 0; i < SCREEN_EDGE_NUM; i++)
    {
	long xdndVersion = 3;

	priv->screenEdge[i].id = XCreateWindow (dpy, priv->root, -100, -100, 1, 1, 0,
					        CopyFromParent, InputOnly,
					        CopyFromParent, CWOverrideRedirect,
					        &attrib);

	XChangeProperty (dpy, priv->screenEdge[i].id, Atoms::xdndAware,
			 XA_ATOM, 32, PropModeReplace,
			 (unsigned char *) &xdndVersion, 1);

	XSelectInput (dpy, priv->screenEdge[i].id,
		      EnterWindowMask   |
		      LeaveWindowMask   |
		      ButtonPressMask   |
		      ButtonReleaseMask |
		      PointerMotionMask);
    }

    priv->updateScreenEdges ();

    priv->setDesktopHints ();
    priv->setSupportingWmCheck ();
    priv->setSupported ();

    priv->normalCursor = XCreateFontCursor (dpy, XC_left_ptr);
    priv->busyCursor   = XCreateFontCursor (dpy, XC_watch);

    XDefineCursor (dpy, priv->root, priv->normalCursor);

    XUngrabServer (dpy);

    priv->setAudibleBell (
	priv->opt[COMP_OPTION_AUDIBLE_BELL].value ().b ());

    XGetInputFocus (dpy, &focus, &revertTo);

    /* move input focus to root window so that we get a FocusIn event when
       moving it to the default window */
    XSetInputFocus (dpy, priv->root,
		    RevertToPointerRoot, CurrentTime);

    if (focus == None || focus == PointerRoot)
    {
	focusDefaultWindow ();
    }
    else
    {
	CompWindow *w;

	w = findWindow (focus);
	if (w)
	{
	    w->moveInputFocusTo ();
	}
	else
	    focusDefaultWindow ();
    }

    priv->pingTimer.start (
	boost::bind(&PrivateScreen::handlePingTimeout, priv),
	priv->opt[COMP_OPTION_PING_DELAY].value ().i (),
	priv->opt[COMP_OPTION_PING_DELAY].value ().i () + 500);

    return true;
}

CompScreen::~CompScreen ()
{
    CompPlugin  *p;

    while (!priv->windows.empty ())
	delete priv->windows.front ();

    while ((p = CompPlugin::pop ()))
	CompPlugin::unload (p);

    XUngrabKey (priv->dpy, AnyKey, AnyModifier, priv->root);

    for (int i = 0; i < SCREEN_EDGE_NUM; i++)
	XDestroyWindow (priv->dpy, priv->screenEdge[i].id);

    XDestroyWindow (priv->dpy, priv->grabWindow);

    if (priv->defaultIcon)
	delete priv->defaultIcon;

    XFreeCursor (priv->dpy, priv->invisibleCursor);

    if (priv->clientList)
	free (priv->clientList);

    if (priv->desktopHintData)
	free (priv->desktopHintData);

    if (priv->snContext)
	sn_monitor_context_unref (priv->snContext);

    if (priv->snDisplay)
	sn_display_unref (priv->snDisplay);

    XSync (priv->dpy, False);
    XCloseDisplay (priv->dpy);

    if (priv->modMap)
	XFreeModifiermap (priv->modMap);

    if (priv->watchPollFds)
	free (priv->watchPollFds);

    delete priv;

    screen = NULL;
}

PrivateScreen::PrivateScreen (CompScreen *screen) :
    priv (this),
    fileWatch (0),
    lastFileWatchHandle (1),
    timers (0),
    watchFds (0),
    lastWatchFdHandle (1),
    watchPollFds (0),
    nWatchFds (0),
    valueMap (),
    screenInfo (0),
    activeWindow (0),
    below (None),
    modMap (0),
    ignoredModMask (LockMask),
    autoRaiseTimer (),
    autoRaiseWindow (0),
    edgeDelayTimer (),
    plugin (),
    dirtyPluginList (true),
    screen(screen),
    windows (),
    size (0, 0),
    vp (0, 0),
    vpSize (1, 1),
    nDesktop (1),
    currentDesktop (0),
    root (None),
    grabWindow (None),
    desktopWindowCount (0),
    mapNum (1),
    activeNum (1),
    outputDevs (0),
    currentOutputDev (0),
    hasOverlappingOutputs (false),
    currentHistory (0),
    snContext (0),
    startupSequences (0),
    startupSequenceTimer (),
    groups (0),
    defaultIcon (0),
    clientList (0),
    nClientList (0),
    buttonGrabs (0),
    keyGrabs (0),
    grabs (0),
    pendingDestroys (0),
    showingDesktopMask (0),
    desktopHintData (0),
    desktopHintSize (0),
    opt (COMP_OPTION_NUM)
{
    for (int i = 0; i < CompModNum; i++)
	modMask[i] = CompNoMask;
    memset (history, 0, sizeof (history));
    gettimeofday (&lastTimeout, 0);
}

PrivateScreen::~PrivateScreen ()
{
    opt.clear ();
}
