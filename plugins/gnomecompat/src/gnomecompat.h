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

#include <core/core.h>
#include <core/privatehandler.h>

#define GNOME_OPTION_MAIN_MENU_KEY              0
#define GNOME_OPTION_RUN_DIALOG_KEY             1
#define GNOME_OPTION_SCREENSHOT_CMD             2
#define GNOME_OPTION_RUN_SCREENSHOT_KEY         3
#define GNOME_OPTION_WINDOW_SCREENSHOT_CMD      4
#define GNOME_OPTION_RUN_WINDOW_SCREENSHOT_KEY  5
#define GNOME_OPTION_TERMINAL_CMD               6
#define GNOME_OPTION_RUN_TERMINAL_KEY           7
#define GNOME_OPTION_NUM                        8

class GnomeCompatScreen :
    public PrivateHandler<GnomeCompatScreen, CompScreen>
{
    public:
	GnomeCompatScreen (CompScreen *s);

	CompOption::Vector & getOptions ();
	bool setOption (const char *name, CompOption::Value& value);

	void panelAction (CompOption::Vector& options, Atom action);

	Atom panelActionAtom;
	Atom panelMainMenuAtom;
	Atom panelRunDialogAtom;

	CompOption::Vector opt;
};

#define GNOME_SCREEN(s)                                \
    GnomeCompatScreen *gs = GnomeCompatScreen::get (s)

class GnomeCompatPluginVTable :
    public CompPlugin::VTableForScreen<GnomeCompatScreen>
{
    public:
	bool init ();

	PLUGIN_OPTION_HELPER (GnomeCompatScreen);
};
