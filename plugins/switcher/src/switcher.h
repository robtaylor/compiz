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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>

#include <decoration.h>
#include <core/core.h>
#include <core/pluginclasshandler.h>
#include <core/atoms.h>

#include <composite/composite.h>
#include <opengl/opengl.h>

#include <X11/Xatom.h>
#include <X11/extensions/Xrender.h>

#include "switcher_options.h"

#define ZOOMED_WINDOW_MASK (1 << 0)
#define NORMAL_WINDOW_MASK (1 << 1)

enum SwitchWindowSelection{
    CurrentViewport = 0,
    AllViewports,
    Panels
};

class SwitchScreen :
    public ScreenInterface,
    public CompositeScreenInterface,
    public GLScreenInterface,
    public PluginClassHandler<SwitchScreen,CompScreen>,
    public SwitcherOptions
{
    public:
	SwitchScreen (CompScreen *screen);
	~SwitchScreen ();

	void setZoom ();

	void handleEvent (XEvent *);

	void preparePaint (int);
	void donePaint ();

	bool glPaintOutput (const GLScreenPaintAttrib &,
			    const GLMatrix &, const CompRegion &,
			    CompOutput *, unsigned int);

	void setSelectedWindowHint ();
	void activateEvent (bool activating);
	void updateWindowList (int count);
	void createWindowList (int count);
	void switchToWindow (bool toNext);
	int countWindows ();
	void initiate (SwitchWindowSelection selection,
		       bool                  showPopup);
	void windowRemove (Window id);
	void updateForegroundColor ();

	bool adjustVelocity ();

	CompositeScreen *cScreen;
	GLScreen        *gScreen;

	Atom selectWinAtom;
	Atom selectFgColorAtom;

	Window popupWindow;

	Window	 selectedWindow;
	Window	 zoomedWindow;
	unsigned int lastActiveNum;

	float zoom;

	CompScreen::GrabHandle grabIndex;

	bool switching;
	bool zooming;
	int  zoomMask;

	bool moreAdjust;

	GLfloat mVelocity;
	GLfloat tVelocity;
	GLfloat sVelocity;

	CompWindowList windows;

	int pos;
	int move;

	float translate;
	float sTranslate;

	SwitchWindowSelection selection;

	unsigned int fgColor[4];

	bool ignoreSwitcher;
};

class SwitchWindow :
    public CompositeWindowInterface,
    public GLWindowInterface,
    public PluginClassHandler<SwitchWindow,CompWindow>
{
    public:
	SwitchWindow (CompWindow *window) :
	    PluginClassHandler<SwitchWindow,CompWindow> (window),
	    window (window),
	    gWindow (GLWindow::get (window)),
	    cWindow (CompositeWindow::get (window)),
	    sScreen (SwitchScreen::get (screen)),
	    gScreen (GLScreen::get (screen))
	{
	    GLWindowInterface::setHandler (gWindow, false);
	    CompositeWindowInterface::setHandler (cWindow, false);

	    if (sScreen->popupWindow && sScreen->popupWindow == window->id ())
		gWindow->glPaintSetEnabled (this, true);
	}

	bool glPaint (const GLWindowPaintAttrib &, const GLMatrix &,
		      const CompRegion &, unsigned int);

	bool damageRect (bool, const CompRect &);

	bool isSwitchWin ();

	void paintThumb (const GLWindowPaintAttrib &, const GLMatrix &,
		         unsigned int, int, int, int, int);

	CompWindow      *window;
	GLWindow        *gWindow;
	CompositeWindow *cWindow;
	SwitchScreen    *sScreen;
	GLScreen        *gScreen;
};

#define MwmHintsDecorations (1L << 1)

typedef struct {
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
} MwmHints;

#define WIDTH  212
#define HEIGHT 192
#define SPACE  10

#define SWITCH_ZOOM 0.1f

#define BOX_WIDTH 3

#define ICON_SIZE 64

#define WINDOW_WIDTH(count) (WIDTH * (count) + (SPACE << 1))
#define WINDOW_HEIGHT (HEIGHT + (SPACE << 1))

#define SWITCH_SCREEN(s) \
    SwitchScreen *ss = SwitchScreen::get (s)

#define SWITCH_WINDOW(w) \
    SwitchWindow *sw = SwitchWindow::get (w)

class SwitchPluginVTable :
    public CompPlugin::VTableForScreenAndWindow<SwitchScreen, SwitchWindow>
{
    public:

	bool init ();
};


