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
 * NO EVENT SHALL NOVELL, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
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
#include <string.h>
#include <sys/poll.h>
#include <assert.h>

#define XK_MISCELLANY
#include <X11/keysymdef.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/shape.h>

#include <boost/bind.hpp>

#include <compiz-core.h>
#include "privatedisplay.h"
#include "privatescreen.h"
#include "privatewindow.h"

static unsigned int virtualModMask[] = {
    CompAltMask, CompMetaMask, CompSuperMask, CompHyperMask,
    CompModeSwitchMask, CompNumLockMask, CompScrollLockMask
};

CompScreen *targetScreen = NULL;
CompOutput *targetOutput;

Bool inHandleEvent = FALSE;


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

#define NUM_OPTIONS(d) (sizeof ((d)->priv->opt) / sizeof (CompOption))


CompObject::indices displayPrivateIndices (0);

int
CompDisplay::allocPrivateIndex ()
{
    return CompObject::allocatePrivateIndex (COMP_OBJECT_TYPE_DISPLAY,
					     &displayPrivateIndices);
}

void
CompDisplay::freePrivateIndex (int index)
{
    CompObject::freePrivateIndex (COMP_OBJECT_TYPE_DISPLAY,
				  &displayPrivateIndices, index);
}

bool
CompWindow::closeWin (CompDisplay     *d,
		      CompAction      *action,
		      CompActionState state,
		      CompOption      *option,
		      int	      nOption)
{
    CompWindow   *w;
    Window       xid;
    unsigned int time;

    xid  = getIntOptionNamed (option, nOption, "window", 0);
    time = getIntOptionNamed (option, nOption, "time", CurrentTime);

    w = d->findTopLevelWindow (xid);
    if (w && (w->priv->actions  & CompWindowActionCloseMask))
	w->close (time);

    return true;
}

bool
CompScreen::mainMenu (CompDisplay     *d,
		      CompAction      *action,
		      CompActionState state,
		      CompOption      *option,
		      int	      nOption)
{
    CompScreen   *s;
    Window       xid;
    unsigned int time;

    xid  = getIntOptionNamed (option, nOption, "root", 0);
    time = getIntOptionNamed (option, nOption, "time", CurrentTime);

    s = d->findScreen (xid);
    if (s && s->priv->grabs.empty ())
	s->toolkitAction (s->display ()->atoms().toolkitActionMainMenu, time,
			  s->priv->root, 0, 0, 0);

    return true;
}

bool
CompScreen::runDialog (CompDisplay     *d,
		       CompAction      *action,
		       CompActionState state,
		       CompOption      *option,
		       int	       nOption)
{
    CompScreen   *s;
    Window       xid;
    unsigned int time;

    xid  = getIntOptionNamed (option, nOption, "root", 0);
    time = getIntOptionNamed (option, nOption, "time", CurrentTime);

    s = d->findScreen (xid);
    if (s && s->priv->grabs.empty ())
	s->toolkitAction (s->display ()->atoms().toolkitActionRunDialog, time,
			  s->priv->root , 0, 0, 0);

    return true;
}

bool
CompWindow::unmaximize (CompDisplay     *d,
			CompAction      *action,
			CompActionState state,
			CompOption      *option,
			int             nOption)
{
    CompWindow *w;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "window", 0);

    w = d->findTopLevelWindow (xid);
    if (w)
	w->maximize (0);

    return true;
}

bool
CompWindow::minimize (CompDisplay     *d,
		      CompAction      *action,
		      CompActionState state,
		      CompOption      *option,
		      int             nOption)
{
    CompWindow *w;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "window", 0);

    w = d->findTopLevelWindow (xid);
    if (w && (w->actions () & CompWindowActionMinimizeMask))
	w->minimize ();

    return true;
}

bool
CompWindow::maximize (CompDisplay     *d,
		      CompAction      *action,
		      CompActionState state,
		      CompOption      *option,
		      int             nOption)
{
    CompWindow *w;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "window", 0);

    w = d->findTopLevelWindow (xid);
    if (w)
	w->maximize (MAXIMIZE_STATE);

    return true;
}

bool
CompWindow::maximizeHorizontally (CompDisplay     *d,
				  CompAction      *action,
				  CompActionState state,
				  CompOption      *option,
				  int             nOption)
{
    CompWindow *w;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "window", 0);

    w = d->findTopLevelWindow (xid);
    if (w)
	w->maximize (w->state () | CompWindowStateMaximizedHorzMask);

    return true;
}

bool
CompWindow::maximizeVertically (CompDisplay     *d,
				CompAction      *action,
				CompActionState state,
				CompOption      *option,
				int             nOption)
{
    CompWindow *w;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "window", 0);

    w = d->findTopLevelWindow (xid);
    if (w)
	w->maximize (w->state () | CompWindowStateMaximizedVertMask);

    return true;
}

bool
CompScreen::showDesktop (CompDisplay     *d,
			 CompAction      *action,
			 CompActionState state,
			 CompOption      *option,
			 int             nOption)
{
    CompScreen *s;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "root", 0);

    s = d->findScreen (xid);
    if (s)
    {
	if (s->priv->showingDesktopMask == 0)
	    s->enterShowDesktopMode ();
	else
	    s->leaveShowDesktopMode (NULL);
    }

    return true;
}

bool
CompScreen::toggleSlowAnimations (CompDisplay     *d,
				  CompAction      *action,
				  CompActionState state,
				  CompOption      *option,
				  int             nOption)
{
    CompScreen *s;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "root", 0);

    s = d->findScreen (xid);
    if (s)
	s->priv->slowAnimations = !s->priv->slowAnimations;

    return true;
}

bool
CompWindow::raiseInitiate (CompDisplay     *d,
			   CompAction      *action,
			   CompActionState state,
			   CompOption      *option,
			   int             nOption)
{
    CompWindow *w;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "window", 0);

    w = d->findTopLevelWindow (xid);
    if (w)
	w->raise ();

    return true;
}

bool
CompWindow::lowerInitiate (CompDisplay     *d,
			   CompAction      *action,
			   CompActionState state,
			   CompOption      *option,
			   int             nOption)
{
    CompWindow *w;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "window", 0);

    w = d->findTopLevelWindow (xid);
    if (w)
	w->lower ();

    return true;
}

bool
CompDisplay::runCommandDispatch (CompDisplay     *d,
				 CompAction      *action,
				 CompActionState state,
				 CompOption      *option,
				 int             nOption)
{
    CompScreen *s;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "root", 0);

    s = d->findScreen (xid);
    if (s)
    {
	int index = -1;
	int i = COMP_DISPLAY_OPTION_RUN_COMMAND0_KEY;

	while (i <= COMP_DISPLAY_OPTION_RUN_COMMAND11_KEY)
	{
	    if (action == &d->priv->opt[i].value.action)
	    {
		index = i - COMP_DISPLAY_OPTION_RUN_COMMAND0_KEY +
		    COMP_DISPLAY_OPTION_COMMAND0;
		break;
	    }

	    i++;
	}

	if (index > 0)
	    s->runCommand (d->priv->opt[index].value.s);
    }

    return true;
}

bool
CompDisplay::runCommandScreenshot (CompDisplay     *d,
				   CompAction      *action,
				   CompActionState state,
				   CompOption      *option,
				   int             nOption)
{
    CompScreen *s;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "root", 0);

    s = d->findScreen (xid);
    if (s)
	s->runCommand (d->priv->opt[COMP_DISPLAY_OPTION_SCREENSHOT].value.s);

    return true;
}

bool
CompDisplay::runCommandWindowScreenshot (CompDisplay     *d,
					 CompAction      *action,
					 CompActionState state,
					 CompOption      *option,
					 int	         nOption)
{
    CompScreen *s;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "root", 0);

    s = d->findScreen (xid);
    if (s)
	s->runCommand (
	    d->priv->opt[COMP_DISPLAY_OPTION_WINDOW_SCREENSHOT].value.s);

    return true;
}

bool
CompDisplay::runCommandTerminal (CompDisplay     *d,
				 CompAction      *action,
				 CompActionState state,
				 CompOption      *option,
				 int             nOption)
{
    CompScreen *s;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "root", 0);

    s = d->findScreen (xid);
    if (s)
	s->runCommand (d->priv->opt[COMP_DISPLAY_OPTION_TERMINAL].value.s);

    return true;
}

bool
CompScreen::windowMenu (CompDisplay     *d,
			CompAction      *action,
			CompActionState state,
			CompOption      *option,
			int             nOption)
{
    CompWindow *w;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "window", 0);

    w = d->findTopLevelWindow (xid);
    if (w && w->screen ()->priv->grabs.empty ())
    {
	int  x, y, button;
	Time time;

	time   = getIntOptionNamed (option, nOption, "time", CurrentTime);
	button = getIntOptionNamed (option, nOption, "button", 0);
	x      = getIntOptionNamed (option, nOption, "x", w->attrib ().x);
	y      = getIntOptionNamed (option, nOption, "y", w->attrib ().y);

	w->screen ()->toolkitAction (
	    w->screen ()->display ()->atoms().toolkitActionWindowMenu,
	    time, w->id (), button, x, y);
    }

    return true;
}

bool
CompWindow::toggleMaximized (CompDisplay     *d,
			     CompAction      *action,
			     CompActionState state,
			     CompOption      *option,
			     int             nOption)
{
    CompWindow *w;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "window", 0);

    w = d->findTopLevelWindow (xid);
    if (w)
    {
	if ((w->priv->state & MAXIMIZE_STATE) == MAXIMIZE_STATE)
	    w->maximize (0);
	else
	    w->maximize (MAXIMIZE_STATE);
    }

    return true;
}

bool
CompWindow::toggleMaximizedHorizontally (CompDisplay     *d,
					 CompAction      *action,
					 CompActionState state,
					 CompOption      *option,
					 int             nOption)
{
    CompWindow *w;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "window", 0);

    w = d->findTopLevelWindow (xid);
    if (w)
	w->maximize (w->priv->state ^ CompWindowStateMaximizedHorzMask);

    return true;
}

bool
CompWindow::toggleMaximizedVertically (CompDisplay     *d,
				       CompAction      *action,
				       CompActionState state,
				       CompOption      *option,
				       int             nOption)
{
    CompWindow *w;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "window", 0);

    w = d->findTopLevelWindow (xid);
    if (w)
	w->maximize (w->priv->state ^ CompWindowStateMaximizedVertMask);

    return true;
}

bool
CompWindow::shade (CompDisplay     *d,
		   CompAction      *action,
		   CompActionState state,
		   CompOption      *option,
		   int             nOption)
{
    CompWindow *w;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "window", 0);

    w = d->findTopLevelWindow (xid);
    if (w && (w->priv->actions & CompWindowActionShadeMask))
    {
	w->priv->state ^= CompWindowStateShadedMask;
	w->updateAttributes (CompStackingUpdateModeNone);
    }

    return true;
}

const CompMetadataOptionInfo coreDisplayOptionInfo[COMP_DISPLAY_OPTION_NUM] = {
    { "abi", "int", 0, 0, 0 },
    { "active_plugins", "list", "<type>string</type>", 0, 0 },
    { "texture_filter", "int", RESTOSTRING (0, 2), 0, 0 },
    { "click_to_focus", "bool", 0, 0, 0 },
    { "autoraise", "bool", 0, 0, 0 },
    { "autoraise_delay", "int", 0, 0, 0 },
    { "close_window_key", "key", 0, CompWindow::closeWin, 0 },
    { "close_window_button", "button", 0, CompWindow::closeWin, 0 },
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
    { "run_command0_key", "key", 0, CompDisplay::runCommandDispatch, 0 },
    { "run_command1_key", "key", 0, CompDisplay::runCommandDispatch, 0 },
    { "run_command2_key", "key", 0, CompDisplay::runCommandDispatch, 0 },
    { "run_command3_key", "key", 0, CompDisplay::runCommandDispatch, 0 },
    { "run_command4_key", "key", 0, CompDisplay::runCommandDispatch, 0 },
    { "run_command5_key", "key", 0, CompDisplay::runCommandDispatch, 0 },
    { "run_command6_key", "key", 0, CompDisplay::runCommandDispatch, 0 },
    { "run_command7_key", "key", 0, CompDisplay::runCommandDispatch, 0 },
    { "run_command8_key", "key", 0, CompDisplay::runCommandDispatch, 0 },
    { "run_command9_key", "key", 0, CompDisplay::runCommandDispatch, 0 },
    { "run_command10_key", "key", 0, CompDisplay::runCommandDispatch, 0 },
    { "run_command11_key", "key", 0, CompDisplay::runCommandDispatch, 0 },
    { "slow_animations_key", "key", 0, CompScreen::toggleSlowAnimations, 0 },
    { "raise_window_key", "key", 0, CompWindow::raiseInitiate, 0 },
    { "raise_window_button", "button", 0, CompWindow::raiseInitiate, 0 },
    { "lower_window_key", "key", 0, CompWindow::lowerInitiate, 0 },
    { "lower_window_button", "button", 0, CompWindow::lowerInitiate, 0 },
    { "unmaximize_window_key", "key", 0, CompWindow::unmaximize, 0 },
    { "minimize_window_key", "key", 0, CompWindow::minimize, 0 },
    { "minimize_window_button", "button", 0, CompWindow::minimize, 0 },
    { "maximize_window_key", "key", 0, CompWindow::maximize, 0 },
    { "maximize_window_horizontally_key", "key", 0,
      CompWindow::maximizeHorizontally, 0 },
    { "maximize_window_vertically_key", "key", 0,
      CompWindow::maximizeVertically, 0 },
    { "command_screenshot", "string", 0, 0, 0 },
    { "run_command_screenshot_key", "key", 0,
      CompDisplay::runCommandScreenshot, 0 },
    { "command_window_screenshot", "string", 0, 0, 0 },
    { "run_command_window_screenshot_key", "key", 0,
      CompDisplay::runCommandWindowScreenshot, 0 },
    { "window_menu_button", "button", 0, CompScreen::windowMenu, 0 },
    { "window_menu_key", "key", 0, CompScreen::windowMenu, 0 },
    { "show_desktop_key", "key", 0, CompScreen::showDesktop, 0 },
    { "show_desktop_edge", "edge", 0, CompScreen::showDesktop, 0 },
    { "raise_on_click", "bool", 0, 0, 0 },
    { "audible_bell", "bool", 0, 0, 0 },
    { "toggle_window_maximized_key", "key", 0,
      CompWindow::toggleMaximized, 0 },
    { "toggle_window_maximized_button", "button", 0,
      CompWindow::toggleMaximized, 0 },
    { "toggle_window_maximized_horizontally_key", "key", 0,
      CompWindow::toggleMaximizedHorizontally, 0 },
    { "toggle_window_maximized_vertically_key", "key", 0,
      CompWindow::toggleMaximizedVertically, 0 },
    { "hide_skip_taskbar_windows", "bool", 0, 0, 0 },
    { "toggle_window_shaded_key", "key", 0, CompWindow::shade, 0 },
    { "ignore_hints_when_maximized", "bool", 0, 0, 0 },
    { "command_terminal", "string", 0, 0, 0 },
    { "run_command_terminal_key", "key", 0,
      CompDisplay::runCommandTerminal, 0 },
    { "ping_delay", "int", "<min>1000</min>", 0, 0 },
    { "edge_delay", "int", "<min>0</min>", 0, 0 }
};

CompOption *
CompDisplay::getDisplayOptions (CompObject  *object,
				int         *count)
{
    CompDisplay *display = (CompDisplay *) object;
    *count = NUM_OPTIONS (display);
    return display->priv->opt;
}

bool
setDisplayOption (CompObject      *object,
		  const char      *name,
		  CompOptionValue *value)
{
    return ((CompDisplay *) object)->setOption (name, value);
}


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
compCheckForError (Display *dpy)
{
    int e;

    XSync (dpy, FALSE);

    e = errors;
    errors = 0;

    return e;
}

/* add actions that should be automatically added as no screens
   existed when they were initialized. */
void
CompDisplay::addScreenActions (CompScreen *s)
{
    int i;

    for (i = 0; i < COMP_DISPLAY_OPTION_NUM; i++)
    {
	if (!isActionOption (&priv->opt[i]))
	    continue;

	if (priv->opt[i].value.action.state & CompActionStateAutoGrab)
	    s->addAction (&priv->opt[i].value.action);
    }
}

Bool
setDisplayAction (CompDisplay     *display,
		  CompOption      *o,
		  CompOptionValue *value)
{
    CompScreen *s;

    for (s = display->screens (); s; s = s->next)
	if (!s->addAction (&value->action))
	    break;

    if (s)
    {
	CompScreen *failed = s;

	for (s = display->screens (); s && s != failed; s = s->next)
	    s->removeAction (&value->action);

	return FALSE;
    }
    else
    {
	for (s = display->screens (); s; s = s->next)
	    s->removeAction (&o->value.action);
    }

    if (compSetActionOption (o, value))
	return TRUE;

    return FALSE;
}

CompDisplay::CompDisplay () :
    CompObject (COMP_OBJECT_TYPE_DISPLAY, "display", &displayPrivateIndices),
    next (0)
{
    WRAPABLE_INIT_HND(handleEvent);
    WRAPABLE_INIT_HND(handleCompizEvent);
    WRAPABLE_INIT_HND(fileToImage);
    WRAPABLE_INIT_HND(imageToFile);
    WRAPABLE_INIT_HND(matchInitExp);
    WRAPABLE_INIT_HND(matchExpHandlerChanged);
    WRAPABLE_INIT_HND(matchPropertyChanged);
    WRAPABLE_INIT_HND(logMessage);

    priv = new PrivateDisplay (this);
    assert (priv);
}


CompDisplay::~CompDisplay ()
{
    while (priv->screens)
	removeScreen (priv->screens);

    objectFiniPlugins (this);

    if (priv->snDisplay)
	sn_display_unref (priv->snDisplay);

    XSync (priv->dpy, False);
    XCloseDisplay (priv->dpy);

    XDestroyRegion (mOutputRegion);
    XDestroyRegion (mTmpRegion);


    compFiniDisplayOptions (this, priv->opt, COMP_DISPLAY_OPTION_NUM);

    compFiniOptionValue (&priv->plugin, CompOptionTypeList);

    if (priv->modMap)
	XFreeModifiermap (priv->modMap);

    delete priv;
}

bool
CompDisplay::init (const char *name)
{
    Window	focus;
    int		revertTo, i;
    int		compositeMajor, compositeMinor;
    int		fixesMinor;
    int		xkbOpcode;
    int		firstScreen, lastScreen;

    mTmpRegion = XCreateRegion ();
    if (!mTmpRegion)
	return false;

    mOutputRegion = XCreateRegion ();
    if (!mOutputRegion)
    {
	XDestroyRegion (mTmpRegion);
	return false;
    }

    priv->plugin.list.type   = CompOptionTypeString;
    priv->plugin.list.nValue = 1;
    priv->plugin.list.value  =
	(CompOptionValue *) malloc (sizeof (CompOptionValue));

    if (!priv->plugin.list.value) {
	return false;
    }

    priv->plugin.list.value->s = strdup ("core");
    if (!priv->plugin.list.value->s) {
        free (priv->plugin.list.value);
	return false;
    }

    priv->dpy = XOpenDisplay (name);
    if (!priv->dpy)
    {
	compLogMessage (this, "core", CompLogLevelFatal,
		        "Couldn't open display %s", XDisplayName (name));
	return false;
    }

//    priv->connection = XGetXCBConnection (priv->dpy);

    if (!compInitDisplayOptionsFromMetadata (this,
					     &coreMetadata,
					     coreDisplayOptionInfo,
					     priv->opt,
					     COMP_DISPLAY_OPTION_NUM))
	return true;

    priv->opt[COMP_DISPLAY_OPTION_ABI].value.i = CORE_ABIVERSION;

    snprintf (priv->displayString, 255, "DISPLAY=%s",
	      DisplayString (priv->dpy));

#ifdef DEBUG
    XSynchronize (priv->dpy, TRUE);
#endif

    priv->initAtoms ();

    XSetErrorHandler (errorHandler);

    updateModifierMappings ();

    priv->snDisplay = sn_display_new (priv->dpy, NULL, NULL);
    if (!priv->snDisplay)
	return true;

    priv->lastPing = 1;

    if (!XQueryExtension (priv->dpy,
			  COMPOSITE_NAME,
			  &priv->compositeOpcode,
			  &priv->compositeEvent,
			  &priv->compositeError))
    {
	compLogMessage (this, "core", CompLogLevelFatal,
		        "No composite extension");
	return false;
    }

    XCompositeQueryVersion (priv->dpy, &compositeMajor, &compositeMinor);
    if (compositeMajor == 0 && compositeMinor < 2)
    {
	compLogMessage (this, "core", CompLogLevelFatal,
		        "Old composite extension");
	return false;
    }

    if (!XDamageQueryExtension (priv->dpy, &priv->damageEvent,
	 			&priv->damageError))
    {
	compLogMessage (this, "core", CompLogLevelFatal,
		        "No damage extension");
	return false;
    }

    if (!XSyncQueryExtension (priv->dpy, &priv->syncEvent, &priv->syncError))
    {
	compLogMessage (this, "core", CompLogLevelFatal,
		        "No sync extension");
	return false;
    }

    if (!XFixesQueryExtension (priv->dpy, &priv->fixesEvent, &priv->fixesError))
    {
	compLogMessage (this, "core", CompLogLevelFatal,
		        "No fixes extension");
	return false;
    }

    XFixesQueryVersion (priv->dpy, &priv->fixesVersion, &fixesMinor);
    /*
    if (d->fixesVersion < 5)
    {
	fprintf (stderr, "%s: Need fixes extension version 5 or later "
		 "for client-side cursor\n", programName);
    }
    */

    priv->randrExtension = XRRQueryExtension (priv->dpy, &priv->randrEvent,
					      &priv->randrError);

    priv->shapeExtension = XShapeQueryExtension (priv->dpy, &priv->shapeEvent,
						 &priv->shapeError);

    priv->xkbExtension = XkbQueryExtension (priv->dpy, &xkbOpcode,
					    &priv->xkbEvent, &priv->xkbError,
					    NULL, NULL);
    if (priv->xkbExtension)
    {
	XkbSelectEvents (priv->dpy,
			 XkbUseCoreKbd,
			 XkbBellNotifyMask | XkbStateNotifyMask,
			 XkbAllEventsMask);
    }
    else
    {
	compLogMessage (this, "core", CompLogLevelFatal,
		        "No XKB extension");

	priv->xkbEvent = priv->xkbError = -1;
    }

    priv->xineramaExtension = XineramaQueryExtension (priv->dpy,
						      &priv->xineramaEvent,
						      &priv->xineramaError);


    updateScreenInfo();

    priv->escapeKeyCode =
	XKeysymToKeycode (priv->dpy, XStringToKeysym ("Escape"));
    priv->returnKeyCode =
	XKeysymToKeycode (priv->dpy, XStringToKeysym ("Return"));

    core->addChild (this);

    /* TODO: bailout properly when objectInitPlugins fails */
    assert (objectInitPlugins (this));

    if (onlyCurrentScreen)
    {
	firstScreen = DefaultScreen (priv->dpy);
	lastScreen  = DefaultScreen (priv->dpy);
    }
    else
    {
	firstScreen = 0;
	lastScreen  = ScreenCount (priv->dpy) - 1;
    }

    for (i = firstScreen; i <= lastScreen; i++)
    {
	addScreen (i);
    }

    if (!priv->screens)
    {
	compLogMessage (this, "core", CompLogLevelFatal,
		        "No manageable screens found on display %s",
		        XDisplayName (name));
	return false;
    }

    priv->setAudibleBell (priv->opt[COMP_DISPLAY_OPTION_AUDIBLE_BELL].value.b);

    XGetInputFocus (priv->dpy, &focus, &revertTo);

    /* move input focus to root window so that we get a FocusIn event when
       moving it to the default window */
    XSetInputFocus (priv->dpy, priv->screens->root (), RevertToPointerRoot,
		    CurrentTime);

    if (focus == None || focus == PointerRoot)
    {
	priv->screens->focusDefaultWindow ();
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
	    priv->screens->focusDefaultWindow ();
    }

    priv->pingTimer.start (
	boost::bind(&PrivateDisplay::handlePingTimeout, priv),
	priv->opt[COMP_DISPLAY_OPTION_PING_DELAY].value.i,
	priv->opt[COMP_DISPLAY_OPTION_PING_DELAY].value.i + 500);

    return true;
}

CompString
CompDisplay::name ()
{
    return CompString ("");
}

CompDisplay::Atoms
CompDisplay::atoms ()
{
    return priv->atoms;
}

Display *
CompDisplay::dpy ()
{
    return priv->dpy;
}

CompScreen *
CompDisplay::screens ()
{
    return priv->screens;
}

GLenum
CompDisplay::textureFilter ()
{
    return priv->textureFilter;
}

CompOption *
CompDisplay::getOption (const char *name)
{
    int        index;
    CompOption *o = compFindOption (priv->opt, NUM_OPTIONS (this),
				    name, &index);
    return o;
}

std::vector<XineramaScreenInfo> &
CompDisplay::screenInfo ()
{
    return priv->screenInfo;
}

bool
CompDisplay::XRandr ()
{
    return priv->randrExtension;
}

bool
CompDisplay::XShape ()
{
    return priv->shapeExtension;
}

SnDisplay *
CompDisplay::snDisplay ()
{
    return priv->snDisplay;
}

Window
CompDisplay::below ()
{
    return priv->below;
}

Window
CompDisplay::activeWindow ()
{
    return priv->activeWindow;
}

Window
CompDisplay::autoRaiseWindow ()
{
    return priv->autoRaiseWindow;
}

XModifierKeymap *
CompDisplay::modMap ()
{
    return priv->modMap;
}

unsigned int
CompDisplay::ignoredModMask ()
{
    return priv->ignoredModMask;
}

const char *
CompDisplay::displayString ()
{
    return priv->displayString;
}

unsigned int
CompDisplay::lastPing ()
{
    return priv->lastPing;
}

CompWatchFdHandle
CompDisplay::getWatchFdHandle ()
{
    return priv->watchFdHandle;
}

void
CompDisplay::setWatchFdHandle (CompWatchFdHandle handle)
{
    priv->watchFdHandle = handle;
}

void
CompDisplay::updateScreenInfo ()
{
    if (priv->xineramaExtension)
    {
	int nInfo;
	XineramaScreenInfo *info = XineramaQueryScreens (priv->dpy, &nInfo);
	
	priv->screenInfo = std::vector<XineramaScreenInfo> (info, info + nInfo);

	if (info)
	    XFree (info);
    }
}

bool
CompDisplay::addScreen (int screenNum)
{
    CompScreen           *s, *prev;
    Window               rootDummy, childDummy;
    int                  x, y, dummy;
    unsigned int	 uDummy;

    s = new CompScreen ();
    if (!s)
	return false;

    for (prev = priv->screens; prev && prev->next; prev = prev->next);

    if (prev)
	prev->next = s;
    else
        priv->screens = s;

    if (!s->init (this, screenNum))
    {
	compLogMessage (this, "core", CompLogLevelError,
			"Failed to manage screen: %d", screenNum);
	if (prev)
	    prev->next = NULL;
	else
	    priv->screens = NULL;
    }

    if (XQueryPointer (priv->dpy, XRootWindow (priv->dpy, screenNum),
		       &rootDummy, &childDummy,
		       &x, &y, &dummy, &dummy, &uDummy))
    {
	lastPointerX = pointerX = x;
	lastPointerY = pointerY = y;
    }
    return true;
}

void
CompDisplay::removeScreen (CompScreen *s)
{
    CompScreen  *p;

    for (p = priv->screens; p; p = p->next)
	if (p->next == s)
	    break;

    if (p)
	p->next = s->next;
    else
	priv->screens = NULL;

    delete s;
}

void
PrivateDisplay::setAudibleBell (bool audible)
{
    if (xkbExtension)
	XkbChangeEnabledControls (dpy,
				  XkbUseCoreKbd,
				  XkbAudibleBellMask,
				  audible ? XkbAudibleBellMask : 0);
}

bool
PrivateDisplay::handlePingTimeout ()
{
    CompScreen  *s;
    CompWindow  *w;
    XEvent      ev;
    int		ping = lastPing + 1;

    ev.type		    = ClientMessage;
    ev.xclient.window	    = 0;
    ev.xclient.message_type = atoms.wmProtocols;
    ev.xclient.format	    = 32;
    ev.xclient.data.l[0]    = atoms.wmPing;
    ev.xclient.data.l[1]    = ping;
    ev.xclient.data.l[2]    = 0;
    ev.xclient.data.l[3]    = 0;
    ev.xclient.data.l[4]    = 0;

    for (s = screens; s; s = s->next)
    {
	for (w = s->windows (); w; w = w->next)
	{
	    if (w->handlePingTimeout (lastPing))
	    {
		ev.xclient.window    = w->id ();
		ev.xclient.data.l[2] = w->id ();

		XSendEvent (dpy, w->id (), false, NoEventMask, &ev);
	    }
	}
    }

    lastPing = ping;

    return true;
}

bool
CompDisplay::setOption (const char      *name,
			CompOptionValue *value)
{
    CompOption *o;
    int	       index;

    o = compFindOption (priv->opt, NUM_OPTIONS (this),
			name, &index);
    if (!o)
	return false;

    switch (index) {
    case COMP_DISPLAY_OPTION_ABI:
	break;
    case COMP_DISPLAY_OPTION_ACTIVE_PLUGINS:
	if (compSetOptionList (o, value))
	{
	    priv->dirtyPluginList = true;
	    return true;
	}
	break;
    case COMP_DISPLAY_OPTION_TEXTURE_FILTER:
	if (compSetIntOption (o, value))
	{
	    CompScreen *s;

	    for (s = priv->screens; s; s = s->next)
		s->damageScreen ();

	    if (!o->value.i)
		priv->textureFilter = GL_NEAREST;
	    else
		priv->textureFilter = GL_LINEAR;

	    return true;
	}
	break;
    case COMP_DISPLAY_OPTION_PING_DELAY:
	if (compSetIntOption (o, value))
	{
	    priv->pingTimer.setTimes (o->value.i, o->value.i + 500);
	    return true;
	}
	break;
    case COMP_DISPLAY_OPTION_AUDIBLE_BELL:
	if (compSetBoolOption (o, value))
	{
	    priv->setAudibleBell (o->value.b);
	    return true;
	}
	break;
    default:
	if (compSetDisplayOption (this, o, value))
	    return true;
	break;
    }

    return false;
}

void
CompDisplay::updateModifierMappings ()
{
    unsigned int    modMask[CompModNum];
    int		    i, minKeycode, maxKeycode, keysymsPerKeycode = 0;
    KeySym*         key;

    for (i = 0; i < CompModNum; i++)
	modMask[i] = 0;

    XDisplayKeycodes (priv->dpy, &minKeycode, &maxKeycode);
    key = XGetKeyboardMapping (priv->dpy,
			       minKeycode, (maxKeycode - minKeycode + 1),
			       &keysymsPerKeycode);

    if (priv->modMap)
	XFreeModifiermap (priv->modMap);

    priv->modMap = XGetModifierMapping (priv->dpy);
    if (priv->modMap && priv->modMap->max_keypermod > 0)
    {
	KeySym keysym;
	int    index, size, mask;

	size = maskTableSize * priv->modMap->max_keypermod;

	for (i = 0; i < size; i++)
	{
	    if (!priv->modMap->modifiermap[i])
		continue;

	    index = 0;
	    do
	    {
		keysym = XKeycodeToKeysym (priv->dpy,
					   priv->modMap->modifiermap[i],
					   index++);
	    } while (!keysym && index < keysymsPerKeycode);

	    if (keysym)
	    {
		mask = maskTable[i / priv->modMap->max_keypermod];

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

	if (memcmp (modMask, priv->modMask, sizeof (modMask)))
	{
	    CompScreen *s;

	    memcpy (priv->modMask, modMask, sizeof (modMask));

	    priv->ignoredModMask = LockMask |
		(modMask[CompModNumLock]    & ~CompNoMask) |
		(modMask[CompModScrollLock] & ~CompNoMask);

	    for (s = priv->screens; s; s = s->next)
		s->updatePassiveGrabs ();
	}
    }

    if (key)
	XFree (key);
}

unsigned int
CompDisplay::virtualToRealModMask (unsigned int modMask)
{
    int i;

    for (i = 0; i < CompModNum; i++)
    {
	if (modMask & virtualModMask[i])
	{
	    modMask &= ~virtualModMask[i];
	    modMask |= priv->modMask[i];
	}
    }

    return modMask;
}

unsigned int
CompDisplay::keycodeToModifiers (int keycode)
{
    unsigned int mods = 0;
    int mod, k;

    for (mod = 0; mod < maskTableSize; mod++)
    {
	for (k = 0; k < priv->modMap->max_keypermod; k++)
	{
	    if (priv->modMap->modifiermap[mod *
		priv->modMap->max_keypermod + k] == keycode)
		mods |= maskTable[mod];
	}
    }

    return mods;
}

void
CompDisplay::processEvents ()
{
    XEvent event;

    if (priv->dirtyPluginList)
	priv->updatePlugins ();

    while (XPending (priv->dpy))
    {
	XNextEvent (priv->dpy, &event);

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
	    if (event.xclient.message_type == priv->atoms.xdndPosition)
	    {
		pointerX = event.xclient.data.l[2] >> 16;
		pointerY = event.xclient.data.l[2] & 0xffff;
	    }
	default:
	    break;
        }

	sn_display_process_event (priv->snDisplay, &event);

	inHandleEvent = true;
	handleEvent (&event);
	inHandleEvent = false;

	lastPointerX = pointerX;
	lastPointerY = pointerY;
    }
}

void
PrivateDisplay::updatePlugins ()
{
    CompOption *o;
    CompPlugin *p, **pop = 0;
    int	       nPop, i, j;

    dirtyPluginList = false;

    o = &opt[COMP_DISPLAY_OPTION_ACTIVE_PLUGINS];

    /* The old plugin list always begins with the core plugin. To make sure
       we don't unnecessarily unload plugins if the new plugin list does not
       contain the core plugin, we have to use an offset */
    if (o->value.list.nValue > 0 && strcmp (o->value.list.value[0].s, "core"))
	i = 0;
    else
	i = 1;

    /* j is initialized to 1 to make sure we never pop the core plugin */
    for (j = 1; j < plugin.list.nValue &&
	 i < o->value.list.nValue; i++, j++)
    {
	if (strcmp (plugin.list.value[j].s, o->value.list.value[i].s))
	    break;
    }

    nPop = plugin.list.nValue - j;

    if (nPop)
    {
	pop = (CompPlugin **) malloc (sizeof (CompPlugin *) * nPop);
	if (!pop)
	{
	    core->setOptionForPlugin (display, "core", o->name, &plugin);
	    return;
	}
    }

    for (j = 0; j < nPop; j++)
    {
	pop[j] = popPlugin ();
	plugin.list.nValue--;
	free (plugin.list.value[plugin.list.nValue].s);
    }

    for (; i < o->value.list.nValue; i++)
    {
	p = 0;
	for (j = 0; j < nPop; j++)
	{
	    if (pop[j] && strcmp (pop[j]->vTable->name (),
				  o->value.list.value[i].s) == 0)
	    {
		if (pushPlugin (pop[j]))
		{
		    p = pop[j];
		    pop[j] = 0;
		    break;
		}
	    }
	}

	if (p == 0)
	{
	    p = loadPlugin (o->value.list.value[i].s);
	    if (p)
	    {
		if (!pushPlugin (p))
		{
		    unloadPlugin (p);
		    p = 0;
		}
	    }
	}

	if (p)
	{
	    CompOptionValue *value;

	    value = (CompOptionValue *)
		realloc (plugin.list.value, sizeof (CompOptionValue) *
			 (plugin.list.nValue + 1));
	    if (value)
	    {
		value[plugin.list.nValue].s = strdup (p->vTable->name ());

		plugin.list.value = value;
		plugin.list.nValue++;
	    }
	    else
	    {
		p = popPlugin ();
		unloadPlugin (p);
	    }
	}
    }

    for (j = 0; j < nPop; j++)
    {
	if (pop[j])
	    unloadPlugin (pop[j]);
    }

    if (nPop)
	free (pop);

    core->setOptionForPlugin (display, "core", o->name, &plugin);
}

CompScreen *
CompDisplay::findScreen (Window root)
{
    CompScreen *s;

    for (s = priv->screens; s; s = s->next)
    {
	if (s->root () == root)
	    return s;
    }

    return 0;
}

void
CompDisplay::forEachWindow (ForEachWindowProc proc,
			   void              *closure)
{
    CompScreen *s;

    for (s = priv->screens; s; s = s->next)
	s->forEachWindow (proc, closure);
}

CompWindow *
CompDisplay::findWindow (Window id)
{
    CompScreen *s;
    CompWindow *w;

    for (s = priv->screens; s; s = s->next)
    {
	w = s->findWindow (id);
	if (w)
	    return w;
    }

    return 0;
}

CompWindow *
CompDisplay::findTopLevelWindow (Window id)
{
    CompScreen *s;
    CompWindow *w;

    for (s = priv->screens; s; s = s->next)
    {
	w = s->findTopLevelWindow (id);
	if (w)
	    return w;
    }

    return 0;
}

static CompScreen *
findScreenForSelection (CompDisplay *display,
			Window       owner,
			Atom         selection)
{
    CompScreen *s;

    for (s = display->screens(); s; s = s->next)
    {
	if (s->selectionWindow () == owner && s->selectionAtom () == selection)
	    return s;
    }

    return NULL;
}

/* from fvwm2, Copyright Matthias Clasen, Dominik Vogt */
static bool
convertProperty (CompDisplay *display,
		 CompScreen  *screen,
		 Window      w,
		 Atom        target,
		 Atom        property)
{

#define N_TARGETS 4

    Atom conversionTargets[N_TARGETS];
    long icccmVersion[] = { 2, 0 };
    Time time = screen->selectionTimestamp ();

    conversionTargets[0] = display->atoms().targets;
    conversionTargets[1] = display->atoms().multiple;
    conversionTargets[2] = display->atoms().timestamp;
    conversionTargets[3] = display->atoms().version;

    if (target == display->atoms().targets)
	XChangeProperty (display->dpy(), w, property,
			 XA_ATOM, 32, PropModeReplace,
			 (unsigned char *) conversionTargets, N_TARGETS);
    else if (target == display->atoms().timestamp)
	XChangeProperty (display->dpy(), w, property,
			 XA_INTEGER, 32, PropModeReplace,
			 (unsigned char *) &time, 1);
    else if (target == display->atoms().version)
	XChangeProperty (display->dpy(), w, property,
			 XA_INTEGER, 32, PropModeReplace,
			 (unsigned char *) icccmVersion, 2);
    else
	return false;

    /* Be sure the PropertyNotify has arrived so we
     * can send SelectionNotify
     */
    XSync (display->dpy (), FALSE);

    return true;
}

/* from fvwm2, Copyright Matthias Clasen, Dominik Vogt */
void
PrivateDisplay::handleSelectionRequest (XEvent *event)
{
    XSelectionEvent reply;
    CompScreen      *screen;

    screen = findScreenForSelection (display,
				     event->xselectionrequest.owner,
				     event->xselectionrequest.selection);
    if (!screen)
	return;

    reply.type	    = SelectionNotify;
    reply.display   = dpy;
    reply.requestor = event->xselectionrequest.requestor;
    reply.selection = event->xselectionrequest.selection;
    reply.target    = event->xselectionrequest.target;
    reply.property  = None;
    reply.time	    = event->xselectionrequest.time;

    if (event->xselectionrequest.target == atoms.multiple)
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
				    atoms.atomPair,
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
		if (!convertProperty (display, screen,
				      event->xselectionrequest.requestor,
				      adata[i], adata[i + 1]))
		    adata[i + 1] = None;

		i += 2;
	    }

	    XChangeProperty (dpy,
			     event->xselectionrequest.requestor,
			     event->xselectionrequest.property,
			     atoms.atomPair,
			     32, PropModeReplace, data, num);
	}
    }
    else
    {
	if (event->xselectionrequest.property == None)
	    event->xselectionrequest.property = event->xselectionrequest.target;

	if (convertProperty (display, screen,
			     event->xselectionrequest.requestor,
			     event->xselectionrequest.target,
			     event->xselectionrequest.property))
	    reply.property = event->xselectionrequest.property;
    }

    XSendEvent (dpy,
		event->xselectionrequest.requestor,
		FALSE, 0L, (XEvent *) &reply);
}

void
PrivateDisplay::handleSelectionClear (XEvent *event)
{
    /* We need to unmanage the screen on which we lost the selection */
    CompScreen *screen;

    screen = findScreenForSelection (display,
				     event->xselectionclear.window,
				     event->xselectionclear.selection);

    if (screen)
	shutDown = TRUE;
}


void
CompDisplay::clearTargetOutput (unsigned int mask)
{
    if (targetScreen)
	targetScreen->clearOutput (targetOutput, mask);
}

#define HOME_IMAGEDIR ".compiz/images"

bool
CompDisplay::readImageFromFile (const char *name,
				int	   *width,
				int	   *height,
				void	   **data)
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
CompDisplay::writeImageToFile (const char *path,
			       const char *name,
			       const char *format,
			       int	  width,
			       int	  height,
			       void	  *data)
{
    return imageToFile (path, name, format, width, height, width * 4, data);
}


CompCursor *
CompDisplay::findCursor ()
{
    CompScreen *s;

    for (s = priv->screens; s; s = s->next)
	if (s->cursors ())
	    return s->cursors ();

    return NULL;
}

Window
CompDisplay::getActiveWindow (Window root)
{
    Atom	  actual;
    int		  result, format;
    unsigned long n, left;
    unsigned char *data;
    Window	  w = None;

    result = XGetWindowProperty (priv->dpy, root,
				 priv->atoms.winActive, 0L, 1L, FALSE,
				 XA_WINDOW, &actual, &format,
				 &n, &left, &data);

    if (result == Success && n && data)
    {
	memcpy (&w, data, sizeof (Window));
	XFree (data);
    }

    return w;
}

void
PrivateDisplay::initAtoms ()
{
    atoms.supported	      = XInternAtom (dpy, "_NET_SUPPORTED", 0);
    atoms.supportingWmCheck = XInternAtom (dpy, "_NET_SUPPORTING_WM_CHECK", 0);

    atoms.utf8String = XInternAtom (dpy, "UTF8_STRING", 0);

    atoms.wmName = XInternAtom (dpy, "_NET_WM_NAME", 0);

    atoms.winType	   = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE", 0);
    atoms.winTypeDesktop = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_DESKTOP",
					 0);
    atoms.winTypeDock    = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_DOCK", 0);
    atoms.winTypeToolbar = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_TOOLBAR",
					 0);
    atoms.winTypeMenu    = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_MENU", 0);
    atoms.winTypeUtil    = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_UTILITY",
					 0);
    atoms.winTypeSplash  = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_SPLASH", 0);
    atoms.winTypeDialog  = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_DIALOG", 0);
    atoms.winTypeNormal  = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_NORMAL", 0);

    atoms.winTypeDropdownMenu =
	XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU", 0);
    atoms.winTypePopupMenu    =
	XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_POPUP_MENU", 0);
    atoms.winTypeTooltip      =
	XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_TOOLTIP", 0);
    atoms.winTypeNotification =
	XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_NOTIFICATION", 0);
    atoms.winTypeCombo        =
	XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_COMBO", 0);
    atoms.winTypeDnd          =
	XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_DND", 0);

    atoms.winOpacity    = XInternAtom (dpy, "_NET_WM_WINDOW_OPACITY", 0);
    atoms.winBrightness = XInternAtom (dpy, "_NET_WM_WINDOW_BRIGHTNESS", 0);
    atoms.winSaturation = XInternAtom (dpy, "_NET_WM_WINDOW_SATURATION", 0);

    atoms.winActive  = XInternAtom (dpy, "_NET_ACTIVE_WINDOW", 0);
    atoms.winDesktop = XInternAtom (dpy, "_NET_WM_DESKTOP", 0);
    atoms.workarea   = XInternAtom (dpy, "_NET_WORKAREA", 0);

    atoms.desktopViewport  = XInternAtom (dpy, "_NET_DESKTOP_VIEWPORT", 0);
    atoms.desktopGeometry  = XInternAtom (dpy, "_NET_DESKTOP_GEOMETRY", 0);
    atoms.currentDesktop   = XInternAtom (dpy, "_NET_CURRENT_DESKTOP", 0);
    atoms.numberOfDesktops = XInternAtom (dpy, "_NET_NUMBER_OF_DESKTOPS", 0);

    atoms.winState	             = XInternAtom (dpy, "_NET_WM_STATE", 0);
    atoms.winStateModal            =
	XInternAtom (dpy, "_NET_WM_STATE_MODAL", 0);
    atoms.winStateSticky           =
	XInternAtom (dpy, "_NET_WM_STATE_STICKY", 0);
    atoms.winStateMaximizedVert    =
	XInternAtom (dpy, "_NET_WM_STATE_MAXIMIZED_VERT", 0);
    atoms.winStateMaximizedHorz    =
	XInternAtom (dpy, "_NET_WM_STATE_MAXIMIZED_HORZ", 0);
    atoms.winStateShaded           =
	XInternAtom (dpy, "_NET_WM_STATE_SHADED", 0);
    atoms.winStateSkipTaskbar      =
	XInternAtom (dpy, "_NET_WM_STATE_SKIP_TASKBAR", 0);
    atoms.winStateSkipPager        =
	XInternAtom (dpy, "_NET_WM_STATE_SKIP_PAGER", 0);
    atoms.winStateHidden           =
	XInternAtom (dpy, "_NET_WM_STATE_HIDDEN", 0);
    atoms.winStateFullscreen       =
	XInternAtom (dpy, "_NET_WM_STATE_FULLSCREEN", 0);
    atoms.winStateAbove            =
	XInternAtom (dpy, "_NET_WM_STATE_ABOVE", 0);
    atoms.winStateBelow            =
	XInternAtom (dpy, "_NET_WM_STATE_BELOW", 0);
    atoms.winStateDemandsAttention =
	XInternAtom (dpy, "_NET_WM_STATE_DEMANDS_ATTENTION", 0);
    atoms.winStateDisplayModal     =
	XInternAtom (dpy, "_NET_WM_STATE_DISPLAY_MODAL", 0);

    atoms.winActionMove          = XInternAtom (dpy, "_NET_WM_ACTION_MOVE", 0);
    atoms.winActionResize        =
	XInternAtom (dpy, "_NET_WM_ACTION_RESIZE", 0);
    atoms.winActionStick         =
	XInternAtom (dpy, "_NET_WM_ACTION_STICK", 0);
    atoms.winActionMinimize      =
	XInternAtom (dpy, "_NET_WM_ACTION_MINIMIZE", 0);
    atoms.winActionMaximizeHorz  =
	XInternAtom (dpy, "_NET_WM_ACTION_MAXIMIZE_HORZ", 0);
    atoms.winActionMaximizeVert  =
	XInternAtom (dpy, "_NET_WM_ACTION_MAXIMIZE_VERT", 0);
    atoms.winActionFullscreen    =
	XInternAtom (dpy, "_NET_WM_ACTION_FULLSCREEN", 0);
    atoms.winActionClose         =
	XInternAtom (dpy, "_NET_WM_ACTION_CLOSE", 0);
    atoms.winActionShade         =
	XInternAtom (dpy, "_NET_WM_ACTION_SHADE", 0);
    atoms.winActionChangeDesktop =
	XInternAtom (dpy, "_NET_WM_ACTION_CHANGE_DESKTOP", 0);
    atoms.winActionAbove         =
	XInternAtom (dpy, "_NET_WM_ACTION_ABOVE", 0);
    atoms.winActionBelow         =
	XInternAtom (dpy, "_NET_WM_ACTION_BELOW", 0);

    atoms.wmAllowedActions = XInternAtom (dpy, "_NET_WM_ALLOWED_ACTIONS", 0);

    atoms.wmStrut        = XInternAtom (dpy, "_NET_WM_STRUT", 0);
    atoms.wmStrutPartial = XInternAtom (dpy, "_NET_WM_STRUT_PARTIAL", 0);

    atoms.wmUserTime = XInternAtom (dpy, "_NET_WM_USER_TIME", 0);

    atoms.wmIcon         = XInternAtom (dpy,"_NET_WM_ICON", 0);
    atoms.wmIconGeometry = XInternAtom (dpy, "_NET_WM_ICON_GEOMETRY", 0);

    atoms.clientList         = XInternAtom (dpy, "_NET_CLIENT_LIST", 0);
    atoms.clientListStacking =
	XInternAtom (dpy, "_NET_CLIENT_LIST_STACKING", 0);

    atoms.frameExtents = XInternAtom (dpy, "_NET_FRAME_EXTENTS", 0);
    atoms.frameWindow  = XInternAtom (dpy, "_NET_FRAME_WINDOW", 0);

    atoms.wmState        = XInternAtom (dpy, "WM_STATE", 0);
    atoms.wmChangeState  = XInternAtom (dpy, "WM_CHANGE_STATE", 0);
    atoms.wmProtocols    = XInternAtom (dpy, "WM_PROTOCOLS", 0);
    atoms.wmClientLeader = XInternAtom (dpy, "WM_CLIENT_LEADER", 0);

    atoms.wmDeleteWindow = XInternAtom (dpy, "WM_DELETE_WINDOW", 0);
    atoms.wmTakeFocus    = XInternAtom (dpy, "WM_TAKE_FOCUS", 0);
    atoms.wmPing         = XInternAtom (dpy, "_NET_WM_PING", 0);
    atoms.wmSyncRequest  = XInternAtom (dpy, "_NET_WM_SYNC_REQUEST", 0);

    atoms.wmSyncRequestCounter =
	XInternAtom (dpy, "_NET_WM_SYNC_REQUEST_COUNTER", 0);

    atoms.closeWindow      = XInternAtom (dpy, "_NET_CLOSE_WINDOW", 0);
    atoms.wmMoveResize     = XInternAtom (dpy, "_NET_WM_MOVERESIZE", 0);
    atoms.moveResizeWindow = XInternAtom (dpy, "_NET_MOVERESIZE_WINDOW", 0);
    atoms.restackWindow    = XInternAtom (dpy, "_NET_RESTACK_WINDOW", 0);

    atoms.showingDesktop = XInternAtom (dpy, "_NET_SHOWING_DESKTOP", 0);

    atoms.xBackground[0] = XInternAtom (dpy, "_XSETROOT_ID", 0);
    atoms.xBackground[1] = XInternAtom (dpy, "_XROOTPMAP_ID", 0);

    atoms.toolkitAction                =
	XInternAtom (dpy, "_COMPIZ_TOOLKIT_ACTION", 0);
    atoms.toolkitActionMainMenu        =
	XInternAtom (dpy, "_COMPIZ_TOOLKIT_ACTION_MAIN_MENU", 0);
    atoms.toolkitActionRunDialog       =
	XInternAtom (dpy, "_COMPIZ_TOOLKIT_ACTION_RUN_DIALOG", 0);
    atoms.toolkitActionWindowMenu      =
	XInternAtom (dpy, "_COMPIZ_TOOLKIT_ACTION_WINDOW_MENU", 0);
    atoms.toolkitActionForceQuitDialog =
	XInternAtom (dpy, "_COMPIZ_TOOLKIT_ACTION_FORCE_QUIT_DIALOG", 0);

    atoms.mwmHints = XInternAtom (dpy, "_MOTIF_WM_HINTS", 0);

    atoms.xdndAware    = XInternAtom (dpy, "XdndAware", 0);
    atoms.xdndEnter    = XInternAtom (dpy, "XdndEnter", 0);
    atoms.xdndLeave    = XInternAtom (dpy, "XdndLeave", 0);
    atoms.xdndPosition = XInternAtom (dpy, "XdndPosition", 0);
    atoms.xdndStatus   = XInternAtom (dpy, "XdndStatus", 0);
    atoms.xdndDrop     = XInternAtom (dpy, "XdndDrop", 0);

    atoms.manager   = XInternAtom (dpy, "MANAGER", 0);
    atoms.targets   = XInternAtom (dpy, "TARGETS", 0);
    atoms.multiple  = XInternAtom (dpy, "MULTIPLE", 0);
    atoms.timestamp = XInternAtom (dpy, "TIMESTAMP", 0);
    atoms.version   = XInternAtom (dpy, "VERSION", 0);
    atoms.atomPair  = XInternAtom (dpy, "ATOM_PAIR", 0);

    atoms.startupId = XInternAtom (dpy, "_NET_STARTUP_ID", 0);
}

bool
CompDisplay::fileToImage (const char *path,
			       const char *name,
			       int	  *width,
			       int	  *height,
			       int	  *stride,
			       void	  **data)
{
    WRAPABLE_HND_FUNC_RETURN(bool, fileToImage, path, name, width, height,
			     stride, data)
    return false;
}

bool
CompDisplay::imageToFile (const char *path,
			  const char *name,
			  const char *format,
			  int	     width,
			  int	     height,
			  int	     stride,
			  void	     *data)
{
    WRAPABLE_HND_FUNC_RETURN(bool, imageToFile, path, name, format, width,
			     height, stride, data)
    return false;
}

void
CompDisplay::logMessage (const char   *componentName,
			 CompLogLevel level,
			 const char   *message)
{
    WRAPABLE_HND_FUNC(logMessage, componentName, level, message)
    compLogMessage (NULL, componentName, level, message);
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
compLogMessage (CompDisplay *d,
	       const char *componentName,
	       CompLogLevel level,
	       const char   *format,
	       ...)
{
    va_list args;
    char    message[2048];

    va_start (args, format);

    vsnprintf (message, 2048, format, args);

    if (d)
	d->logMessage (componentName, level, message);
    else
	logMessage (componentName, level, message);

    va_end (args);
}

int
CompDisplay::getWmState (Window id)
{
    Atom	  actual;
    int		  result, format;
    unsigned long n, left;
    unsigned char *data;
    unsigned long state = NormalState;

    result = XGetWindowProperty (priv->dpy, id,
				 priv->atoms.wmState, 0L, 2L, FALSE,
				 priv->atoms.wmState, &actual, &format,
				 &n, &left, &data);

    if (result == Success && n && data)
    {
	memcpy (&state, data, sizeof (unsigned long));
	XFree ((void *) data);
    }

    return state;
}

void
CompDisplay::setWmState (int state, Window id)
{
    unsigned long data[2];

    data[0] = state;
    data[1] = None;

    XChangeProperty (priv->dpy, id,
		     priv->atoms.wmState, priv->atoms.wmState,
		     32, PropModeReplace, (unsigned char *) data, 2);
}

unsigned int
CompDisplay::windowStateMask (Atom state)
{
    if (state == priv->atoms.winStateModal)
	return CompWindowStateModalMask;
    else if (state == priv->atoms.winStateSticky)
	return CompWindowStateStickyMask;
    else if (state == priv->atoms.winStateMaximizedVert)
	return CompWindowStateMaximizedVertMask;
    else if (state == priv->atoms.winStateMaximizedHorz)
	return CompWindowStateMaximizedHorzMask;
    else if (state == priv->atoms.winStateShaded)
	return CompWindowStateShadedMask;
    else if (state == priv->atoms.winStateSkipTaskbar)
	return CompWindowStateSkipTaskbarMask;
    else if (state == priv->atoms.winStateSkipPager)
	return CompWindowStateSkipPagerMask;
    else if (state == priv->atoms.winStateHidden)
	return CompWindowStateHiddenMask;
    else if (state == priv->atoms.winStateFullscreen)
	return CompWindowStateFullscreenMask;
    else if (state == priv->atoms.winStateAbove)
	return CompWindowStateAboveMask;
    else if (state == priv->atoms.winStateBelow)
	return CompWindowStateBelowMask;
    else if (state == priv->atoms.winStateDemandsAttention)
	return CompWindowStateDemandsAttentionMask;
    else if (state == priv->atoms.winStateDisplayModal)
	return CompWindowStateDisplayModalMask;

    return 0;
}

unsigned int
CompDisplay::windowStateFromString (const char *str)
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
CompDisplay::getWindowState (Window id)
{
    Atom	  actual;
    int		  result, format;
    unsigned long n, left;
    unsigned char *data;
    unsigned int  state = 0;

    result = XGetWindowProperty (priv->dpy, id,
				 priv->atoms.winState,
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
CompDisplay::setWindowState (unsigned int state,
			     Window       id)
{
    Atom data[32];
    int	 i = 0;

    if (state & CompWindowStateModalMask)
	data[i++] = priv->atoms.winStateModal;
    if (state & CompWindowStateStickyMask)
	data[i++] = priv->atoms.winStateSticky;
    if (state & CompWindowStateMaximizedVertMask)
	data[i++] = priv->atoms.winStateMaximizedVert;
    if (state & CompWindowStateMaximizedHorzMask)
	data[i++] = priv->atoms.winStateMaximizedHorz;
    if (state & CompWindowStateShadedMask)
	data[i++] = priv->atoms.winStateShaded;
    if (state & CompWindowStateSkipTaskbarMask)
	data[i++] = priv->atoms.winStateSkipTaskbar;
    if (state & CompWindowStateSkipPagerMask)
	data[i++] = priv->atoms.winStateSkipPager;
    if (state & CompWindowStateHiddenMask)
	data[i++] = priv->atoms.winStateHidden;
    if (state & CompWindowStateFullscreenMask)
	data[i++] = priv->atoms.winStateFullscreen;
    if (state & CompWindowStateAboveMask)
	data[i++] = priv->atoms.winStateAbove;
    if (state & CompWindowStateBelowMask)
	data[i++] = priv->atoms.winStateBelow;
    if (state & CompWindowStateDemandsAttentionMask)
	data[i++] = priv->atoms.winStateDemandsAttention;
    if (state & CompWindowStateDisplayModalMask)
	data[i++] = priv->atoms.winStateDisplayModal;

    XChangeProperty (priv->dpy, id, priv->atoms.winState,
		     XA_ATOM, 32, PropModeReplace,
		     (unsigned char *) data, i);
}

unsigned int
CompDisplay::getWindowType (Window id)
{
    Atom	  actual;
    int		  result, format;
    unsigned long n, left;
    unsigned char *data;

    result = XGetWindowProperty (priv->dpy , id,
				 priv->atoms.winType,
				 0L, 1L, FALSE, XA_ATOM, &actual, &format,
				 &n, &left, &data);

    if (result == Success && n && data)
    {
	Atom a;

	memcpy (&a, data, sizeof (Atom));
	XFree ((void *) data);

	if (a == priv->atoms.winTypeNormal)
	    return CompWindowTypeNormalMask;
	else if (a == priv->atoms.winTypeMenu)
	    return CompWindowTypeMenuMask;
	else if (a == priv->atoms.winTypeDesktop)
	    return CompWindowTypeDesktopMask;
	else if (a == priv->atoms.winTypeDock)
	    return CompWindowTypeDockMask;
	else if (a == priv->atoms.winTypeToolbar)
	    return CompWindowTypeToolbarMask;
	else if (a == priv->atoms.winTypeUtil)
	    return CompWindowTypeUtilMask;
	else if (a == priv->atoms.winTypeSplash)
	    return CompWindowTypeSplashMask;
	else if (a == priv->atoms.winTypeDialog)
	    return CompWindowTypeDialogMask;
	else if (a == priv->atoms.winTypeDropdownMenu)
	    return CompWindowTypeDropdownMenuMask;
	else if (a == priv->atoms.winTypePopupMenu)
	    return CompWindowTypePopupMenuMask;
	else if (a == priv->atoms.winTypeTooltip)
	    return CompWindowTypeTooltipMask;
	else if (a == priv->atoms.winTypeNotification)
	    return CompWindowTypeNotificationMask;
	else if (a == priv->atoms.winTypeCombo)
	    return CompWindowTypeComboMask;
	else if (a == priv->atoms.winTypeDnd)
	    return CompWindowTypeDndMask;
    }

    return CompWindowTypeUnknownMask;
}

void
CompDisplay::getMwmHints (Window	  id,
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
				 priv->atoms.mwmHints,
				 0L, 20L, FALSE, priv->atoms.mwmHints,
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
CompDisplay::getProtocols (Window id)
{
    Atom         *protocol;
    int          count;
    unsigned int protocols = 0;

    if (XGetWMProtocols (priv->dpy, id, &protocol, &count))
    {
	int  i;

	for (i = 0; i < count; i++)
	{
	    if (protocol[i] == priv->atoms.wmDeleteWindow)
		protocols |= CompWindowProtocolDeleteMask;
	    else if (protocol[i] == priv->atoms.wmTakeFocus)
		protocols |= CompWindowProtocolTakeFocusMask;
	    else if (protocol[i] == priv->atoms.wmPing)
		protocols |= CompWindowProtocolPingMask;
	    else if (protocol[i] == priv->atoms.wmSyncRequest)
		protocols |= CompWindowProtocolSyncRequestMask;
	}

	XFree (protocol);
    }

    return protocols;
}

unsigned int
CompDisplay::getWindowProp (Window       id,
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
CompDisplay::setWindowProp (Window       id,
			    Atom         property,
			    unsigned int value)
{
    unsigned long data = value;

    XChangeProperty (priv->dpy, id, property,
		     XA_CARDINAL, 32, PropModeReplace,
		     (unsigned char *) &data, 1);
}

bool
CompDisplay::readWindowProp32 (Window         id,
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
CompDisplay::getWindowProp32 (Window         id,
			      Atom           property,
			      unsigned short defaultValue)
{
    unsigned short result;

    if (readWindowProp32 (id, property, &result))
	return result;

    return defaultValue;
}

void
CompDisplay::setWindowProp32 (Window         id,
			      Atom           property,
			      unsigned short value)
{
    CARD32 value32;

    value32 = value << 16 | value;

    XChangeProperty (priv->dpy, id, property,
		     XA_CARDINAL, 32, PropModeReplace,
		     (unsigned char *) &value32, 1);
}


DisplayInterface::DisplayInterface ()
{
    WRAPABLE_INIT_FUNC(handleEvent);
    WRAPABLE_INIT_FUNC(handleCompizEvent);
    WRAPABLE_INIT_FUNC(fileToImage);
    WRAPABLE_INIT_FUNC(imageToFile);
    WRAPABLE_INIT_FUNC(matchInitExp);
    WRAPABLE_INIT_FUNC(matchExpHandlerChanged);
    WRAPABLE_INIT_FUNC(matchPropertyChanged);
    WRAPABLE_INIT_FUNC(logMessage);
}

void
DisplayInterface::handleEvent (XEvent *event)
    WRAPABLE_DEF_FUNC(handleEvent, event)

void
DisplayInterface::handleCompizEvent (const char  *plugin,
				     const char  *event,
				     CompOption  *option,
				     int         nOption)
    WRAPABLE_DEF_FUNC(handleCompizEvent, plugin, event, option, nOption)

bool
DisplayInterface::fileToImage (const char *path,
			       const char *name,
			       int	  *width,
			       int	  *height,
			       int	  *stride,
			       void	  **data)
    WRAPABLE_DEF_FUNC_RETURN(fileToImage, path, name, width, height,
			     stride, data)

bool
DisplayInterface::imageToFile (const char *path,
			       const char *name,
			       const char *format,
			       int	  width,
			       int	  height,
			       int	  stride,
			       void	  *data)
    WRAPABLE_DEF_FUNC_RETURN(imageToFile, path, name, format, width, height,
			     stride, data)

void
DisplayInterface::matchInitExp (CompMatchExp *exp, const char *value)
    WRAPABLE_DEF_FUNC(matchInitExp, exp, value)

void
DisplayInterface::matchExpHandlerChanged ()
    WRAPABLE_DEF_FUNC(matchExpHandlerChanged)

void
DisplayInterface::matchPropertyChanged (CompWindow *window)
    WRAPABLE_DEF_FUNC(matchPropertyChanged, window)

void
DisplayInterface::logMessage (const char   *componentName,
			      CompLogLevel level,
			      const char   *message)
    WRAPABLE_DEF_FUNC(logMessage, componentName, level, message)

PrivateDisplay::PrivateDisplay (CompDisplay *display) :
    display (display),
    screens (0),
    watchFdHandle (0),
    screenInfo (0),
    textureFilter (GL_LINEAR),
    activeWindow (0),
    below (None),
    modMap (0),
    ignoredModMask (LockMask),
    autoRaiseTimer (),
    autoRaiseWindow (0),
    edgeDelayTimer (),
    dirtyPluginList (true)
{
    for (int i = 0; i < CompModNum; i++)
	modMask[i] = CompNoMask;

    compInitOptionValue (&plugin);
}

PrivateDisplay::~PrivateDisplay ()
{
}
