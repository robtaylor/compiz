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

#include <X11/cursorfont.h>

#include <core/atoms.h>
#include "move.h"

static CompMetadata *moveMetadata;

static bool
moveInitiate (CompAction      *action,
	      CompAction::State state,
	      CompOption::Vector &options)
{
    CompWindow *w;
    Window     xid;

    MOVE_SCREEN (screen);

    xid = CompOption::getIntOptionNamed (options, "window");

    w = screen->findWindow (xid);
    if (w && (w->actions () & CompWindowActionMoveMask))
    {
	XRectangle   workArea;
	unsigned int mods;
	int          x, y, button;

	CompScreen *s = screen;

	mods = CompOption::getIntOptionNamed (options, "modifiers", 0);

	x = CompOption::getIntOptionNamed (options, "x", w->geometry ().x () +
					   (w->size ().width () / 2));
	y = CompOption::getIntOptionNamed (options, "y", w->geometry ().y () +
					   (w->size ().height () / 2));

	button = CompOption::getIntOptionNamed (options, "button", -1);

	if (s->otherGrabExist ("move", 0))
	    return false;

	if (ms->w)
	    return false;

	if (w->type () & (CompWindowTypeDesktopMask |
		       CompWindowTypeDockMask    |
		       CompWindowTypeFullscreenMask))
	    return false;

	if (w->overrideRedirect ())
	    return false;

	if (state & CompAction::StateInitButton)
	    action->setState (action->state () | CompAction::StateTermButton);

	if (ms->region)
	{
	    XDestroyRegion (ms->region);
	    ms->region = NULL;
	}

	ms->status = RectangleOut;

	ms->savedX = w->serverGeometry ().x ();
	ms->savedY = w->serverGeometry ().y ();

	ms->x = 0;
	ms->y = 0;

	lastPointerX = x;
	lastPointerY = y;

	ms->origState = w->state ();

	s->getWorkareaForOutput (w->outputDevice (), &workArea);

	ms->snapBackY = w->serverGeometry ().y () - workArea.y;
	ms->snapOffY  = y - workArea.y;

	if (!ms->grab)
	    ms->grab = s->pushGrab (ms->moveCursor, "move");

	if (ms->grab)
	{
	    ms->w = w;

	    ms->releaseButton = button;

	    w->grabNotify (x, y, mods,CompWindowGrabMoveMask |
			   CompWindowGrabButtonMask);

	    if (state & CompAction::StateInitKey)
	    {
		int xRoot, yRoot;

		xRoot = w->geometry ().x () + (w->size ().width () / 2);
		yRoot = w->geometry ().y () + (w->size ().height () / 2);

		s->warpPointer (xRoot - pointerX, yRoot - pointerY);
	    }

	    if (ms->moveOpacity != OPAQUE)
	    {
		MOVE_WINDOW (w);

		if (mw->cWindow)
		    mw->cWindow->addDamage ();
		if (mw->gWindow)
		mw->gWindow->glPaintSetEnabled (mw, true);
	    }
	}
    }

    return false;
}

static bool
moveTerminate (CompAction      *action,
	       CompAction::State state,
	       CompOption::Vector &options)
{
    MOVE_SCREEN (screen);

    if (ms->w)
    {
	if (state & CompAction::StateCancel)
	    ms->w->move (ms->savedX - ms->w->geometry ().x (),
			 ms->savedY - ms->w->geometry ().y (), false);

	ms->w->syncPosition ();

	/* update window attributes as window constraints may have
	   changed - needed e.g. if a maximized window was moved
	   to another output device */
	ms->w->updateAttributes (CompStackingUpdateModeNone);

	ms->w->ungrabNotify ();

	if (ms->grab)
	{
	    screen->removeGrab (ms->grab, NULL);
	    ms->grab = NULL;
	}

	if (ms->moveOpacity != OPAQUE)
	{
	    MOVE_WINDOW (ms->w);

	    if (mw->cWindow)
		mw->cWindow->addDamage ();
	    if (mw->gWindow)
		mw->gWindow->glPaintSetEnabled (mw, false);
	}

	ms->w             = 0;
	ms->releaseButton = 0;
    }

    action->setState (action->state () & ~(CompAction::StateTermKey |
		      CompAction::StateTermButton));

    return false;
}

/* creates a region containing top and bottom struts. only struts that are
   outside the screen workarea are considered. */
static Region
moveGetYConstrainRegion (CompScreen *s)
{
    CompWindow   *w;
    Region       region;
    REGION       r;
    XRectangle   workArea;
    BoxRec       extents;
    unsigned int i;

    region = XCreateRegion ();
    if (!region)
	return NULL;

    r.rects    = &r.extents;
    r.numRects = r.size = 1;

    r.extents.x1 = MINSHORT;
    r.extents.y1 = 0;
    r.extents.x2 = 0;
    r.extents.y2 = s->height ();

    XUnionRegion (&r, region, region);

    r.extents.x1 = s->width ();
    r.extents.x2 = MAXSHORT;

    XUnionRegion (&r, region, region);

    for (i = 0; i < s->outputDevs ().size(); i++)
    {
	XUnionRegion (s->outputDevs ()[i].region (), region, region);

	s->getWorkareaForOutput (i, &workArea);
	extents = s->outputDevs ()[i].region ()->extents;

	foreach (w, s->windows ())
	{
	    if (!w->mapNum ())
		continue;

	    if (w->struts ())
	    {
		r.extents.x1 = w->struts ()->top.x;
		r.extents.y1 = w->struts ()->top.y;
		r.extents.x2 = r.extents.x1 + w->struts ()->top.width;
		r.extents.y2 = r.extents.y1 + w->struts ()->top.height;

		if (r.extents.x1 < extents.x1)
		    r.extents.x1 = extents.x1;
		if (r.extents.x2 > extents.x2)
		    r.extents.x2 = extents.x2;
		if (r.extents.y1 < extents.y1)
		    r.extents.y1 = extents.y1;
		if (r.extents.y2 > extents.y2)
		    r.extents.y2 = extents.y2;

		if (r.extents.x1 < r.extents.x2 && r.extents.y1 < r.extents.y2)
		{
		    if (r.extents.y2 <= workArea.y)
			XSubtractRegion (region, &r, region);
		}

		r.extents.x1 = w->struts ()->bottom.x;
		r.extents.y1 = w->struts ()->bottom.y;
		r.extents.x2 = r.extents.x1 + w->struts ()->bottom.width;
		r.extents.y2 = r.extents.y1 + w->struts ()->bottom.height;

		if (r.extents.x1 < extents.x1)
		    r.extents.x1 = extents.x1;
		if (r.extents.x2 > extents.x2)
		    r.extents.x2 = extents.x2;
		if (r.extents.y1 < extents.y1)
		    r.extents.y1 = extents.y1;
		if (r.extents.y2 > extents.y2)
		    r.extents.y2 = extents.y2;

		if (r.extents.x1 < r.extents.x2 && r.extents.y1 < r.extents.y2)
		{
		    if (r.extents.y1 >= (workArea.y + workArea.height))
			XSubtractRegion (region, &r, region);
		}
	    }
	}
    }

    return region;
}

static void
moveHandleMotionEvent (CompScreen *s,
		       int	  xRoot,
		       int	  yRoot)
{
    MOVE_SCREEN (s);

    if (ms->grab)
    {
	int	     dx, dy;
	int	     wX, wY;
	unsigned int wWidth, wHeight;
	CompWindow   *w;

	w = ms->w;
	
	wX      = w->serverGeometry ().x ();
	wY      = w->serverGeometry ().y ();
	wWidth  = w->serverGeometry ().width () +
		  w->serverGeometry ().border () * 2;
	wHeight = w->serverGeometry ().height () +
		  w->serverGeometry ().border () * 2;

	ms->x += xRoot - lastPointerX;
	ms->y += yRoot - lastPointerY;

	if (w->type () & CompWindowTypeFullscreenMask)
	{
	    dx = dy = 0;
	}
	else
	{
	    XRectangle workArea;
	    int	       min, max;

	    dx = ms->x;
	    dy = ms->y;

	    s->getWorkareaForOutput (w->outputDevice (), &workArea);

	    if (ms->opt[MOVE_OPTION_CONSTRAIN_Y].value ().b ())
	    {
		if (!ms->region)
		    ms->region = moveGetYConstrainRegion (s);

		/* make sure that the top frame extents or the top row of
		   pixels are within what is currently our valid screen
		   region */
		if (ms->region)
		{
		    int x, y, width, height;
		    int status;

		    x	   = wX + dx - w->input ().left;
		    y	   = wY + dy - w->input ().top;
		    width  = wWidth + w->input ().left + w->input ().right;
		    height = w->input ().top ? w->input ().top : 1;

		    status = XRectInRegion (ms->region, x, y, width, height);

		    /* only constrain movement if previous position was valid */
		    if (ms->status == RectangleIn)
		    {
			int xStatus = status;

			while (dx && xStatus != RectangleIn)
			{
			    xStatus = XRectInRegion (ms->region,
						     x, y - dy,
						     width, height);

			    if (xStatus != RectangleIn)
				dx += (dx < 0) ? 1 : -1;

			    x = wX + dx - w->input ().left;
			}

			while (dy && status != RectangleIn)
			{
			    status = XRectInRegion (ms->region,
						    x, y,
						    width, height);

			    if (status != RectangleIn)
				dy += (dy < 0) ? 1 : -1;

			    y = wY + dy - w->input ().top;
			}
		    }
		    else
		    {
			ms->status = status;
		    }
		}
	    }

	    if (ms->opt[MOVE_OPTION_SNAPOFF_MAXIMIZED].value ().b ())
	    {
		if (w->state () & CompWindowStateMaximizedVertMask)
		{
		    if (abs ((yRoot - workArea.y) - ms->snapOffY) >= SNAP_OFF)
		    {
			if (!s->otherGrabExist ("move", 0))
			{
			    int width = w->serverGeometry ().width ();

			    w->saveMask () |= CWX | CWY;

			    if (w->saveMask ()& CWWidth)
				width = w->saveWc ().width;

			    w->saveWc ().x = xRoot - (width >> 1);
			    w->saveWc ().y = yRoot + (w->input ().top >> 1);

			    ms->x = ms->y = 0;

			    w->maximize (0);

			    ms->snapOffY = ms->snapBackY;

			    return;
			}
		    }
		}
		else if (ms->origState & CompWindowStateMaximizedVertMask)
		{
		    if (abs ((yRoot - workArea.y) - ms->snapBackY) < SNAP_BACK)
		    {
			if (!s->otherGrabExist ("move", 0))
			{
			    int wy;

			    /* update server position before maximizing
			       window again so that it is maximized on
			       correct output */
			    w->syncPosition ();

			    w->maximize (ms->origState);

			    wy  = workArea.y + (w->input ().top >> 1);
			    wy += w->sizeHints ().height_inc >> 1;

			    s->warpPointer (0, wy - pointerY);

			    return;
			}
		    }
		}
	    }

	    if (w->state () & CompWindowStateMaximizedVertMask)
	    {
		min = workArea.y + w->input ().top;
		max = workArea.y + workArea.height - w->input ().bottom - wHeight;

		if (wY + dy < min)
		    dy = min - wY;
		else if (wY + dy > max)
		    dy = max - wY;
	    }

	    if (w->state () & CompWindowStateMaximizedHorzMask)
	    {
		if (wX > (int) s->width () ||
		    wX + w->size ().width () < 0)
		    return;

		if (wX + wWidth < 0)
		    return;

		min = workArea.x + w->input ().left;
		max = workArea.x + workArea.width - w->input ().right - wWidth;

		if (wX + dx < min)
		    dx = min - wX;
		else if (wX + dx > max)
		    dx = max - wX;
	    }
	}

	if (dx || dy)
	{
	    w->move (wX + dx - w->geometry ().x (),
		     wY + dy - w->geometry ().y (), false);

	    if (ms->opt[MOVE_OPTION_LAZY_POSITIONING].value ().b () &&
	        MoveScreen::get (screen)->hasCompositing)
	    {
		/* FIXME: This form of lazy positioning is broken and should
		   be replaced asap. Current code exists just to avoid a
		   major performance regression in the 0.5.2 release. */
		w->serverGeometry ().setX (w->geometry ().x ());
		w->serverGeometry ().setY (w->geometry ().y ());
	    }
	    else
	    {
		w->syncPosition ();
	    }

	    ms->x -= dx;
	    ms->y -= dy;
	}
    }
}

void
MoveScreen::handleEvent (XEvent *event)
{
    switch (event->type) {
	case ButtonPress:
	case ButtonRelease:
	    if (event->xbutton.root == screen->root ())
	    {
		if (grab)
		{
		    if (releaseButton == -1 ||
			releaseButton == (int) event->xbutton.button)
		    {
			CompAction *action;

			action = &opt[MOVE_OPTION_INITIATE_BUTTON].value ().action ();
			moveTerminate (action, CompAction::StateTermButton,
				       noOptions);
		    }
		}
	    }
	    break;
	case KeyPress:
	    if (event->xkey.root == screen->root ())
	    {
		if (grab)
		{
		    unsigned int i;

		    for (i = 0; i < NUM_KEYS; i++)
		    {
			if (event->xkey.keycode == key[i])
			{
			    XWarpPointer (screen->dpy (), None, None,
					  0, 0, 0, 0,
					  mKeys[i].dx * KEY_MOVE_INC,
					  mKeys[i].dy * KEY_MOVE_INC);
			    break;
			}
		    }
		}
	    }
	    break;
	case MotionNotify:
	    if (event->xmotion.root == screen->root ())
		moveHandleMotionEvent (screen, pointerX, pointerY);
	    break;
	case EnterNotify:
	case LeaveNotify:
	    if (event->xcrossing.root == screen->root ())
		moveHandleMotionEvent (screen, pointerX, pointerY);
	    break;
	case ClientMessage:
	    if (event->xclient.message_type == Atoms::wmMoveResize)
	    {
		CompWindow *w;
		unsigned   long type = event->xclient.data.l[2];

		MOVE_SCREEN (screen);

		if (type == WmMoveResizeMove ||
		    type == WmMoveResizeMoveKeyboard)
		{
		    w = screen->findWindow (event->xclient.window);
		    if (w)
		    {
			CompOption::Vector o;
			int	       xRoot, yRoot;
			int	       option;

			o.push_back (CompOption ("window", CompOption::TypeInt));
			o[0].value ().set ((int) event->xclient.window);

			if (event->xclient.data.l[2] == WmMoveResizeMoveKeyboard)
			{
			    option = MOVE_OPTION_INITIATE_KEY;

			    moveInitiate (&opt[option].value ().action (),
					  CompAction::StateInitKey, o);
			}
			else
			{
			    unsigned int mods;
			    Window	     root, child;
			    int	     i;

			    option = MOVE_OPTION_INITIATE_BUTTON;

			    XQueryPointer (screen->dpy (), screen->root (),
					   &root, &child, &xRoot, &yRoot,
					   &i, &i, &mods);

			    /* TODO: not only button 1 */
			    if (mods & Button1Mask)
			    {
				o.push_back (CompOption ("modifiers", CompOption::TypeInt));
				o[1].value ().set ((int) mods);

				o.push_back (CompOption ("x", CompOption::TypeInt));
				o[2].value ().set ((int) event->xclient.data.l[0]);

				o.push_back (CompOption ("y", CompOption::TypeInt));
				o[3].value ().set ((int) event->xclient.data.l[1]);

				o.push_back (CompOption ("button", CompOption::TypeInt));
				o[4].value ().set ((int) (event->xclient.data.l[3] ?
					       event->xclient.data.l[3] : -1));

				moveInitiate (&opt[option].value ().action (),
					      CompAction::StateInitButton, o);

				moveHandleMotionEvent (screen, xRoot, yRoot);
			    }
			}
		    }
		}
		else if (ms->w && type == WmMoveResizeCancel)
		{
		    if (ms->w->id () == event->xclient.window)
		    {
			int option;

			option = MOVE_OPTION_INITIATE_BUTTON;
			moveTerminate (&opt[option].value ().action (),
				       CompAction::StateCancel, noOptions);
			option = MOVE_OPTION_INITIATE_KEY;
			moveTerminate (&opt[option].value ().action (),
				       CompAction::StateCancel, noOptions);

		    }
		}
	    }
	    break;
	case DestroyNotify:
	    if (w && w->id () == event->xdestroywindow.window)
	    {
		int option;

		option = MOVE_OPTION_INITIATE_BUTTON;
		moveTerminate (&opt[option].value ().action (),
			       0, noOptions);
		option = MOVE_OPTION_INITIATE_KEY;
		moveTerminate (&opt[option].value ().action (),
			       0, noOptions);
	    }
	    break;
	case UnmapNotify:
	    if (w && w->id () == event->xunmap.window)
	    {
		int option;

		option = MOVE_OPTION_INITIATE_BUTTON;
		moveTerminate (&opt[option].value ().action (),
			       0, noOptions);
		option = MOVE_OPTION_INITIATE_KEY;
		moveTerminate (&opt[option].value ().action (),
			       0, noOptions);
	    }
	default:
	    break;
    }

    screen->handleEvent (event);
}

bool
MoveWindow::glPaint (const GLWindowPaintAttrib &attrib,
		     const GLMatrix            &transform,
		     const CompRegion          &region,
		     unsigned int              mask)
{
    GLWindowPaintAttrib sAttrib = attrib;
    bool                status;

    MOVE_SCREEN (screen);

    if (ms->grab)
    {
	if (ms->w == window && ms->moveOpacity != OPAQUE)
	{
	    /* modify opacity of windows that are not active */
	    sAttrib.opacity = (sAttrib.opacity * ms->moveOpacity) >> 16;
	}
    }

    status = gWindow->glPaint (sAttrib, transform, region, mask);

    return status;
}

CompOption::Vector &
MoveScreen::getOptions ()
{
    return opt;
}
 
bool
MoveScreen::setOption (const char        *name,
		       CompOption::Value &value)
{
    CompOption *o;
    unsigned int index;
 
    o = CompOption::findOption (opt, name, &index);
    if (!o)
	return false;
 
     switch (index) {
     case MOVE_OPTION_OPACITY:
 	if (o->set (value))
 	{
	    moveOpacity = (o->value ().i () * OPAQUE) / 100;
	    return true;
 	}
 	break;
     default:
	return CompOption::setOption (*o, value);
     }
 
    return false;
}

static const CompMetadata::OptionInfo moveOptionInfo[] = {
    { "initiate_button", "button", 0, moveInitiate, moveTerminate },
    { "initiate_key", "key", 0, moveInitiate, moveTerminate },
    { "opacity", "int", "<min>0</min><max>100</max>", 0, 0 },
    { "constrain_y", "bool", 0, 0, 0 },
    { "snapoff_maximized", "bool", 0, 0, 0 },
    { "lazy_positioning", "bool", 0, 0, 0 }
};

MoveScreen::MoveScreen (CompScreen *screen) :
    PrivateHandler<MoveScreen,CompScreen> (screen),
    w (0),
    region (NULL),
    status (RectangleOut),
    releaseButton (0),
    grab(NULL),
    opt(MOVE_OPTION_NUM)
{
    if (!moveMetadata->initOptions (moveOptionInfo, MOVE_OPTION_NUM, opt))
    {
	setFailed ();
	return;
    }
 
    moveOpacity = (opt[MOVE_OPTION_OPACITY].value ().i () * OPAQUE) / 100;
 
    for (unsigned int i = 0; i < NUM_KEYS; i++)
	key[i] = XKeysymToKeycode (screen->dpy (),
				   XStringToKeysym (mKeys[i].name));

    moveCursor = XCreateFontCursor (screen->dpy (), XC_fleur);
    if (CompositeScreen::get (screen))
	hasCompositing =
	    CompositeScreen::get (screen)->compositingActive ();

    ScreenInterface::setHandler (screen);
}

MoveScreen::~MoveScreen ()
{
    if (moveCursor)
	XFreeCursor (screen->dpy (), moveCursor);
}

bool
MovePluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	 return false;

    moveMetadata = new CompMetadata (name (), moveOptionInfo, MOVE_OPTION_NUM);

    if (!moveMetadata)
	return false;

    moveMetadata->addFromFile (name ());

    return true;
}

void
MovePluginVTable::fini ()
{
    delete moveMetadata;
}

CompMetadata *
MovePluginVTable::getMetadata ()
{
    return moveMetadata;
}

MovePluginVTable moveVTable;

CompPlugin::VTable *
getCompPluginInfo20080805 (void)
{
    return &moveVTable;
}
