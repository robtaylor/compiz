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

#ifndef _RESIZE_H
#define _RESIZE_H

#include <core/core.h>
#include <core/pluginclasshandler.h>

#include <composite/composite.h>
#include <opengl/opengl.h>

#define RESIZE_OPTION_INITIATE_NORMAL_KEY    0
#define RESIZE_OPTION_INITIATE_OUTLINE_KEY   1
#define RESIZE_OPTION_INITIATE_RECTANGLE_KEY 2
#define RESIZE_OPTION_INITIATE_STRETCH_KEY   3
#define RESIZE_OPTION_INITIATE_BUTTON	     4
#define RESIZE_OPTION_INITIATE_KEY	     5
#define RESIZE_OPTION_MODE	             6
#define RESIZE_OPTION_BORDER_COLOR           7
#define RESIZE_OPTION_FILL_COLOR             8
#define RESIZE_OPTION_NORMAL_MATCH	     9
#define RESIZE_OPTION_OUTLINE_MATCH	     10
#define RESIZE_OPTION_RECTANGLE_MATCH	     11
#define RESIZE_OPTION_STRETCH_MATCH	     12
#define RESIZE_OPTION_NUM		     13

#define RESIZE_SCREEN(s) ResizeScreen *rs = ResizeScreen::get(s)
#define RESIZE_WINDOW(w) ResizeWindow *rw = ResizeWindow::get(w)

#define ResizeUpMask    (1L << 0)
#define ResizeDownMask  (1L << 1)
#define ResizeLeftMask  (1L << 2)
#define ResizeRightMask (1L << 3)

#define RESIZE_MODE_NORMAL    0
#define RESIZE_MODE_OUTLINE   1
#define RESIZE_MODE_RECTANGLE 2
#define RESIZE_MODE_STRETCH   3
#define RESIZE_MODE_LAST      RESIZE_MODE_STRETCH

struct _ResizeKeys {
    const char	 *name;
    int		 dx;
    int		 dy;
    unsigned int warpMask;
    unsigned int resizeMask;
} rKeys[] = {
    { "Left",  -1,  0, ResizeLeftMask | ResizeRightMask, ResizeLeftMask },
    { "Right",  1,  0, ResizeLeftMask | ResizeRightMask, ResizeRightMask },
    { "Up",     0, -1, ResizeUpMask | ResizeDownMask,    ResizeUpMask },
    { "Down",   0,  1, ResizeUpMask | ResizeDownMask,    ResizeDownMask }
};

#define NUM_KEYS (sizeof (rKeys) / sizeof (rKeys[0]))

#define MIN_KEY_WIDTH_INC  24
#define MIN_KEY_HEIGHT_INC 24

class ResizeScreen :
    public PluginClassHandler<ResizeScreen,CompScreen>,
    public GLScreenInterface,
    public ScreenInterface
{
    public:
	ResizeScreen (CompScreen *s);
	~ResizeScreen ();

	CompOption::Vector & getOptions ();
	bool setOption (const char *name, CompOption::Value &value);

	void handleEvent (XEvent *event);

	void getPaintRectangle (BoxPtr pBox);
	void getStretchRectangle (BoxPtr pBox);

	void sendResizeNotify ();
	void updateWindowProperty ();

	void finishResizing ();

	void updateWindowSize ();

	bool glPaintOutput (const GLScreenPaintAttrib &,
			    const GLMatrix &, const CompRegion &, CompOutput *,
			    unsigned int);

	void damageRectangle (BoxPtr pBox);
	Cursor cursorFromResizeMask (unsigned int mask);

	void handleKeyEvent (KeyCode keycode);
	void handleMotionEvent (int xRoot, int yRoot);

	void glPaintRectangle (const GLScreenPaintAttrib &sAttrib,
			       const GLMatrix            &transform,
			       CompOutput                *output,
			       unsigned short            *borderColor,
			       unsigned short            *fillColor);

    public:

	GLScreen        *gScreen;
	CompositeScreen *cScreen;

	Atom resizeNotifyAtom;
	Atom resizeInformationAtom;

	CompWindow	 *w;
	int		 mode;
	XRectangle	 savedGeometry;
	XRectangle	 geometry;

	int          releaseButton;
	unsigned int mask;
	int          pointerDx;
	int          pointerDy;
	KeyCode      key[NUM_KEYS];
	
	CompScreen::GrabHandle grabIndex;

	Cursor leftCursor;
	Cursor rightCursor;
	Cursor upCursor;
	Cursor upLeftCursor;
	Cursor upRightCursor;
	Cursor downCursor;
	Cursor downLeftCursor;
	Cursor downRightCursor;
	Cursor middleCursor;
	Cursor cursor[NUM_KEYS];

	CompOption::Vector opt;
};

class ResizeWindow :
    public WindowInterface,
    public CompositeWindowInterface,
    public GLWindowInterface,
    public PluginClassHandler<ResizeWindow,CompWindow>
{
    public:
	ResizeWindow (CompWindow *w);
	~ResizeWindow ();
	
	void resizeNotify (int, int, int, int);

	bool damageRect (bool, const CompRect &);

	bool glPaint (const GLWindowPaintAttrib &, const GLMatrix &,
		      const CompRegion &, unsigned int);

	void getStretchScale (BoxPtr pBox, float *xScale, float *yScale);

	
    public:

	CompWindow      *window;
	GLWindow        *gWindow;
	CompositeWindow *cWindow;
	ResizeScreen    *rScreen;
};

class ResizePluginVTable :
    public CompPlugin::VTableForScreenAndWindow<ResizeScreen, ResizeWindow>
{
    public:
	bool init ();

	PLUGIN_OPTION_HELPER (ResizeScreen)

};

#endif
