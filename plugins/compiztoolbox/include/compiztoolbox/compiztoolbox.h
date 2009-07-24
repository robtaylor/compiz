/*
 * Copyright © 2007 Novell, Inc.
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

#ifndef _COMPIZ_COMPIZTOOLBOX_H
#define _COMPIZ_COMPIZTOOLBOX_H

#include <decoration.h>
#include <core/core.h>
#include <core/atoms.h>

#include <composite/composite.h>
#include <opengl/opengl.h>

#include <X11/Xatom.h>
#include <X11/extensions/Xrender.h>


typedef enum {
    CurrentViewport = 0,
    AllViewports,
    Panels,
    Group
} SwitchWindowSelection;


class BaseSwitchScreen
{
    public:
	BaseSwitchScreen (CompScreen *screen);
	virtual ~BaseSwitchScreen () {}

	void handleEvent (XEvent *);
	void setSelectedWindowHint ();
	void activateEvent (bool activating);
	void updateForegroundColor ();

	CompWindow *switchToWindow (bool toNext, bool autoChangeVPOption);
	static bool compareWindows (CompWindow *w1, CompWindow *w2);
	static Visual *findArgbVisual (Display *dpy, int scr);

	virtual bool shouldShowIcon () { return false; }
	virtual void windowRemove (Window id) {}
	virtual void doWindowDamage (CompWindow *w);
	virtual void handleSelectionChange (bool toNext, int nextIdx) {}
	virtual void getMinimizedAndMatch (bool &minimizedOption,
					   CompMatch *&matchOption);

	CompositeScreen *cScreen;
	GLScreen        *gScreen;

	Atom selectWinAtom;
	Atom selectFgColorAtom;

	CompWindowList windows;

	Window       popupWindow;
	Window	     selectedWindow;
	unsigned int lastActiveNum;

	CompScreen::GrabHandle grabIndex;

	bool moreAdjust;

	SwitchWindowSelection selection;

	unsigned int fgColor[4];

	bool ignoreSwitcher;
};

class BaseSwitchWindow
{
    public:
	BaseSwitchWindow (BaseSwitchScreen *, CompWindow *);

	void paintThumb (const GLWindowPaintAttrib &attrib,
			 const GLMatrix            &transform,
			 unsigned int              mask,
			 int                       x,
			 int                       y,
			 int                       width1,
			 int                       height1,
			 int                       width2,
			 int                       height2);
	virtual void updateIconTexturedWindow (GLWindowPaintAttrib  &sAttrib,
					       int                  &wx,
					       int                  &wy,
					       int                  x,
					       int                  y,
					       GLTexture            *icon) {}
	virtual void updateIconNontexturedWindow (GLWindowPaintAttrib  &sAttrib,
						  int                  &wx,
						  int                  &wy,
						  float                &width,
						  float                &height,
						  int                  x,
						  int                  y,
						  GLTexture            *icon) {}
	virtual void updateIconPos (int   &wx,
				    int   &wy,
				    int   x,
				    int   y,
				    float width,
				    float height) {}
	bool damageRect (bool, const CompRect &);
	bool isSwitchWin ();

	BaseSwitchScreen *baseScreen;
	GLWindow         *gWindow;
	CompositeWindow  *cWindow;
	GLScreen         *gScreen;
	CompWindow       *window;
};

#define ICON_SIZE 64
#define MAX_ICON_SIZE 256

#endif
