/*
 * Copyright © 2009 Danny Baumann
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Danny Baumann not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Danny Baumann makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * DANNY BAUMANN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL DENNIS KASPRZYK BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Danny Baumann <dannybaumann@web.de>
 */

#include "gnomecompat.h"

COMPIZ_PLUGIN_20081216 (gnomecompat, GnomeCompatPluginVTable);

static bool
runCommand (CompAction          *action,
	    CompAction::State   state,
	    CompOption::Vector& options,
	    unsigned int        commandOption)
{
    Window xid;

    GNOME_SCREEN (screen);

    xid = CompOption::getIntOptionNamed (options, "root", 0);
    if (xid != screen->root ())
	return false;

    screen->runCommand (gs->opt[commandOption].value ().s ());

    return true;
}

void
GnomeCompatScreen::panelAction (CompOption::Vector& options,
				Atom                actionAtom)
{
    Window xid;
    XEvent event;
    Time   time;

    xid = CompOption::getIntOptionNamed (options, "root", 0);
    if (xid != screen->root ())
	return;

    time = CompOption::getIntOptionNamed (options, "time", CurrentTime);

    /* we need to ungrab the keyboard here, otherwise the panel main
       menu won't popup as it wants to grab the keyboard itself */
    XUngrabKeyboard (screen->dpy (), time);

    event.type                 = ClientMessage;
    event.xclient.window       = screen->root ();
    event.xclient.message_type = panelActionAtom;
    event.xclient.format       = 32;
    event.xclient.data.l[0]    = actionAtom;
    event.xclient.data.l[1]    = time;
    event.xclient.data.l[2]    = 0;
    event.xclient.data.l[3]    = 0;
    event.xclient.data.l[4]    = 0;

    XSendEvent (screen->dpy (), screen->root (), FALSE,
		StructureNotifyMask, &event);
}

static bool
showMainMenu (CompAction          *action,
	      CompAction::State   state,
	      CompOption::Vector& options)
{
    GNOME_SCREEN (screen);

    gs->panelAction (options, gs->panelMainMenuAtom);

    return true;
}

static bool
showRunDialog (CompAction          *action,
	       CompAction::State   state,
	       CompOption::Vector& options)
{
    GNOME_SCREEN (screen);

    gs->panelAction (options, gs->panelRunDialogAtom);

    return true;
}

#define COMMAND_BIND(opt) \
    boost::bind (runCommand, _1, _2, _3, opt)

static const CompMetadata::OptionInfo gnomeOptionInfo[] = {
    { "main_menu_key", "key", 0, showMainMenu, 0 },
    { "run_key", "key", 0, showRunDialog, 0 },
    { "command_screenshot", "string", 0, 0, 0 },
    { "run_command_screenshot_key", "key", 0,
	COMMAND_BIND (GNOME_OPTION_SCREENSHOT_CMD), 0 },
    { "command_window_screenshot", "string", 0, 0, 0 },
    { "run_command_window_screenshot_key", "key", 0,
	COMMAND_BIND (GNOME_OPTION_WINDOW_SCREENSHOT_CMD), 0 },
    { "command_terminal", "string", 0, 0, 0 },
    { "run_command_terminal_key", "key", 0,
	COMMAND_BIND (GNOME_OPTION_TERMINAL_CMD), 0 }
};

GnomeCompatScreen::GnomeCompatScreen (CompScreen *s) :
    PluginClassHandler<GnomeCompatScreen, CompScreen> (s)
{
    if (!gnomecompatVTable->getMetadata ()->initOptions (gnomeOptionInfo,
							 GNOME_OPTION_NUM,
							 opt))
    {
	setFailed ();
	return;
    }

    panelActionAtom =
	XInternAtom (screen->dpy (), "_GNOME_PANEL_ACTION", FALSE);
    panelMainMenuAtom =
	XInternAtom (screen->dpy (), "_GNOME_PANEL_ACTION_MAIN_MENU", FALSE);
    panelRunDialogAtom =
	XInternAtom (screen->dpy (), "_GNOME_PANEL_ACTION_RUN_DIALOG", FALSE);
}

CompOption::Vector&
GnomeCompatScreen::getOptions ()
{
    return opt;
}

bool
GnomeCompatScreen::setOption (const char         *name,
			      CompOption::Value& value)
{
    CompOption *o;

    o = CompOption::findOption (opt, name, NULL);
    if (!o)
	return false;

    return CompOption::setOption (*o, value);
}

bool
GnomeCompatPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	return false;

    getMetadata ()->addFromOptionInfo (gnomeOptionInfo, GNOME_OPTION_NUM);
    getMetadata ()->addFromFile (name ());

    return true;
}
