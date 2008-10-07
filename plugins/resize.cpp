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

#include <X11/Xatom.h>
#include <X11/cursorfont.h>

#include <core/core.h>
#include <core/atoms.h>
#include "resize.h"

static CompMetadata *resizeMetadata;

void
ResizeScreen::getPaintRectangle (BoxPtr pBox)
{
    pBox->x1 = geometry.x - w->input ().left;
    pBox->y1 = geometry.y - w->input ().top;
    pBox->x2 = geometry.x +
	geometry.width + w->serverGeometry ().border () * 2 +
	w->input ().right;

    if (w->shaded ())
    {
	pBox->y2 = geometry.y + w->size ().height () + w->input ().bottom;
    }
    else
    {
	pBox->y2 = geometry.y +
	    geometry.height + w->serverGeometry ().border () * 2 +
	    w->input ().bottom;
    }
}

void
ResizeWindow::getStretchScale (BoxPtr pBox, float *xScale, float *yScale)
{
    int width, height;

    width  = window->size ().width () + window->input ().left +
	     window->input ().right;
    height = window->size ().height () + window->input ().top +
	     window->input ().bottom;

    *xScale = (width)  ? (pBox->x2 - pBox->x1) / (float) width  : 1.0f;
    *yScale = (height) ? (pBox->y2 - pBox->y1) / (float) height : 1.0f;
}

void
ResizeScreen::getStretchRectangle (BoxPtr pBox)
{
    BoxRec box;
    float  xScale, yScale;

    getPaintRectangle (&box);
    ResizeWindow::get (w)->getStretchScale (&box, &xScale, &yScale);

    pBox->x1 = (int)(box.x1 - (w->output ().left - w->input ().left) * xScale);
    pBox->y1 = (int)(box.y1 - (w->output ().top - w->input ().top) * yScale);
    pBox->x2 = (int)(box.x2 + w->output ().right * xScale);
    pBox->y2 = (int)(box.y2 + w->output ().bottom * yScale);
}

void
ResizeScreen::damageRectangle (BoxPtr pBox)
{
    int x1, x2, y1, y2;

    x1 = pBox->x1 - 1;
    y1 = pBox->y1 - 1;
    x2 = pBox->x2 + 1;
    y2 = pBox->y2 + 1;

    if (cScreen)
	cScreen->damageRegion (CompRegion (CompRect (x1, x2, y1, y2)));
}

Cursor
ResizeScreen::cursorFromResizeMask (unsigned int mask)
{
    Cursor cursor;

    if (mask & ResizeLeftMask)
    {
	if (mask & ResizeDownMask)
	    cursor = downLeftCursor;
	else if (mask & ResizeUpMask)
	    cursor = upLeftCursor;
	else
	    cursor = leftCursor;
    }
    else if (mask & ResizeRightMask)
    {
	if (mask & ResizeDownMask)
	    cursor = downRightCursor;
	else if (mask & ResizeUpMask)
	    cursor = upRightCursor;
	else
	    cursor = rightCursor;
    }
    else if (mask & ResizeUpMask)
    {
	cursor = upCursor;
    }
    else
    {
	cursor = downCursor;
    }

    return cursor;
}

void
ResizeScreen::sendResizeNotify ()
{
    XEvent xev;

    xev.xclient.type    = ClientMessage;
    xev.xclient.display = screen->dpy ();
    xev.xclient.format  = 32;

    xev.xclient.message_type = resizeNotifyAtom;
    xev.xclient.window	     = w->id ();

    xev.xclient.data.l[0] = geometry.x;
    xev.xclient.data.l[1] = geometry.y;
    xev.xclient.data.l[2] = geometry.width;
    xev.xclient.data.l[3] = geometry.height;
    xev.xclient.data.l[4] = 0;

    XSendEvent (screen->dpy (), screen->root (),	FALSE,
		SubstructureRedirectMask | SubstructureNotifyMask, &xev);
}

void
ResizeScreen::updateWindowProperty ()
{
    unsigned long data[4];

    data[0] = geometry.x;
    data[1] = geometry.y;
    data[2] = geometry.width;
    data[3] = geometry.height;

    XChangeProperty (screen->dpy (), w->id (), resizeInformationAtom,
		     XA_CARDINAL, 32, PropModeReplace,
        	     (unsigned char*) data, 4);
}

void
ResizeScreen::finishResizing ()
{
    w->ungrabNotify ();

    XDeleteProperty (screen->dpy (), w->id (), resizeInformationAtom);

    w = NULL;
}

static bool
resizeInitiate (CompAction         *action,
	        CompAction::State  state,
	        CompOption::Vector &options)
{
    CompWindow *w;
    Window     xid;

    xid = CompOption::getIntOptionNamed (options, "window");

    w = screen->findWindow (xid);
    if (w && (w->actions () & CompWindowActionResizeMask))
    {
	unsigned int mask;
	int          x, y;
	int	     button;
	int	     i;

	RESIZE_SCREEN (screen);

	CompWindow::Geometry server = w->serverGeometry ();
	
	x = CompOption::getIntOptionNamed (options, "x", pointerX);
	y = CompOption::getIntOptionNamed (options, "y", pointerY);

	button = CompOption::getIntOptionNamed (options, "button", -1);

	mask = CompOption::getIntOptionNamed (options, "direction");

	/* Initiate the resize in the direction suggested by the
	 * sector of the window the mouse is in, eg drag in top left
	 * will resize up and to the left.  Keyboard resize starts out
	 * with the cursor in the middle of the window and then starts
	 * resizing the edge corresponding to the next key press. */
	if (state & CompAction::StateInitKey)
	{
	    mask = 0;
	}
	else if (!mask)
	{
	    unsigned int sectorSizeX = server.width () / 3;
	    unsigned int sectorSizeY = server.height () / 3;
	    unsigned int posX        = x - server.x ();
	    unsigned int posY        = y - server.y ();

	    if (posX < sectorSizeX)
		mask |= ResizeLeftMask;
	    else if (posX > (2 * sectorSizeX))
		mask |= ResizeRightMask;

	    if (posY < sectorSizeY)
		mask |= ResizeUpMask;
	    else if (posY > (2 * sectorSizeY))
		mask |= ResizeDownMask;

	    /* if the pointer was in the middle of the window,
	       do nothing */

	    if (!mask)
		return false;
	}

	if (screen->otherGrabExist ("resize", 0))
	    return false;

	if (rs->w)
	    return false;

	if (w->type () & (CompWindowTypeDesktopMask |
		          CompWindowTypeDockMask	 |
		          CompWindowTypeFullscreenMask))
	    return false;

	if (w->overrideRedirect ())
	    return false;

	if (state & CompAction::StateInitButton)
	    action->setState (action->state () | CompAction::StateTermButton);

	if (w->shaded ())
	    mask &= ~(ResizeUpMask | ResizeDownMask);

	rs->w	 = w;
	rs->mask = mask;

	rs->savedGeometry.x	 = server.x ();
	rs->savedGeometry.y	 = server.y ();
	rs->savedGeometry.width  = server.width ();
	rs->savedGeometry.height = server.height ();

	rs->geometry = rs->savedGeometry;

	rs->pointerDx = x - pointerX;
	rs->pointerDy = y - pointerY;

	if ((w->state () & MAXIMIZE_STATE) == MAXIMIZE_STATE)
	{
	    /* if the window is fully maximized, showing the outline or
	       rectangle would be visually distracting as the window can't
	       be resized anyway; so we better don't use them in this case */
	    rs->mode = RESIZE_MODE_NORMAL;
	}
	else
	{
	    rs->mode = rs->opt[RESIZE_OPTION_MODE].value ().i ();
	    for (i = 0; i <= RESIZE_MODE_LAST; i++)
	    {
		if (action == &rs->opt[i].value ().action ())
		{
		    rs->mode = i;
		    break;
		}
	    }

	    if (i > RESIZE_MODE_LAST)
	    {
		int index;

		for (i = 0; i <= RESIZE_MODE_LAST; i++)
		{
		    index = RESIZE_OPTION_NORMAL_MATCH + i;
		    if (rs->opt[index].value ().match ().evaluate (w))
		    {
			rs->mode = i;
			break;
		    }
		}
	    }

	    if (!rs->gScreen || !rs->cScreen ||
		rs->cScreen->compositingActive ())
		rs->mode = RESIZE_MODE_NORMAL;
	}

	if (rs->mode != RESIZE_MODE_NORMAL)
	{
	    RESIZE_WINDOW (w);
	    if (rw->gWindow && rs->mode == RESIZE_MODE_STRETCH)
		rw->gWindow->glPaintSetEnabled (rw, true);
	    if (rw->cWindow && rs->mode == RESIZE_MODE_STRETCH)
		rw->cWindow->damageRectSetEnabled (rw, true);
	    rs->gScreen->glPaintOutputSetEnabled (rs, true);
	}

	if (!rs->grabIndex)
	{
	    Cursor cursor;

	    if (state & CompAction::StateInitKey)
	    {
		cursor = rs->middleCursor;
	    }
	    else
	    {
		cursor = rs->cursorFromResizeMask (mask);
	    }

	    rs->grabIndex = screen->pushGrab (cursor, "resize");
	}

	if (rs->grabIndex)
	{
	    BoxRec box;

	    rs->releaseButton = button;

	    w->grabNotify (x, y, state, CompWindowGrabResizeMask |
			   CompWindowGrabButtonMask);

	    /* using the paint rectangle is enough here
	       as we don't have any stretch yet */
	    rs->getPaintRectangle (&box);
	    rs->damageRectangle (&box);

	    if (state & CompAction::StateInitKey)
	    {
		int xRoot, yRoot;

		xRoot = server.x () + (server.width () / 2);
		yRoot = server.y () + (server.height () / 2);

		screen->warpPointer (xRoot - pointerX, yRoot - pointerY);
	    }
	}
    }

    return false;
}

static bool
resizeTerminate (CompAction         *action,
	         CompAction::State  state,
	         CompOption::Vector &options)
{
    RESIZE_SCREEN (screen);

    if (rs->w)
    {
	CompWindow     *w = rs->w;
	XWindowChanges xwc;
	unsigned int   mask = 0;

	if (rs->mode == RESIZE_MODE_NORMAL)
	{
	    if (state & CompAction::StateCancel)
	    {
		xwc.x      = rs->savedGeometry.x;
		xwc.y      = rs->savedGeometry.y;
		xwc.width  = rs->savedGeometry.width;
		xwc.height = rs->savedGeometry.height;

		mask = CWX | CWY | CWWidth | CWHeight;
	    }
	}
	else
	{
	    XRectangle geometry;

	    if (state & CompAction::StateCancel)
		geometry = rs->savedGeometry;
	    else
		geometry = rs->geometry;

	    if (memcmp (&geometry, &rs->savedGeometry, sizeof (geometry)) == 0)
	    {
		BoxRec box;

		if (rs->mode == RESIZE_MODE_STRETCH)
		    rs->getStretchRectangle (&box);
		else
		    rs->getPaintRectangle (&box);

		rs->damageRectangle (&box);
	    }
	    else
	    {
		xwc.x      = geometry.x;
		xwc.y      = geometry.y;
		xwc.width  = geometry.width;
		xwc.height = geometry.height;

		mask = CWX | CWY | CWWidth | CWHeight;
	    }

	    if (rs->mode != RESIZE_MODE_NORMAL)
	    {
		RESIZE_WINDOW (rs->w);
		if (rw->gWindow && rs->mode == RESIZE_MODE_STRETCH)
		    rw->gWindow->glPaintSetEnabled (rw, false);
		if (rw->cWindow && rs->mode == RESIZE_MODE_STRETCH)
		    rw->cWindow->damageRectSetEnabled (rw, false);
		rs->gScreen->glPaintOutputSetEnabled (rs, false);
	    }
	}

	if ((mask & CWWidth) &&
	    xwc.width == (int) w->serverGeometry ().width ())
	    mask &= ~CWWidth;

	if ((mask & CWHeight) &&
	    xwc.height == (int) w->serverGeometry ().height ())
	    mask &= ~CWHeight;

	if (mask)
	{
	    if (mask & (CWWidth | CWHeight))
		w->sendSyncRequest ();

	    w->configureXWindow (mask, &xwc);
	}

	if (!(mask & (CWWidth | CWHeight)))
	    rs->finishResizing ();

	if (rs->grabIndex)
	{
	    screen->removeGrab (rs->grabIndex, NULL);
	    rs->grabIndex = 0;
	}

	rs->releaseButton = 0;
    }

    action->setState (action->state () & ~(CompAction::StateTermKey |
		      CompAction::StateTermButton));

    return false;
}


void
ResizeScreen::updateWindowSize ()
{
    if (w->syncWait ())
	return;

    if (w->serverGeometry ().width ()  != geometry.width ||
	w->serverGeometry ().height () != geometry.height)
    {
	XWindowChanges xwc;

	xwc.x	   = geometry.x;
	xwc.y	   = geometry.y;
	xwc.width  = geometry.width;
	xwc.height = geometry.height;

	w->sendSyncRequest ();

	w->configureXWindow (CWX | CWY | CWWidth | CWHeight, &xwc);
    }
}

void
ResizeScreen::handleKeyEvent (KeyCode keycode)
{
    if (grabIndex && w)
    {
	int	   widthInc, heightInc;

	widthInc  = w->sizeHints ().width_inc;
	heightInc = w->sizeHints ().height_inc;

	if (widthInc < MIN_KEY_WIDTH_INC)
	    widthInc = MIN_KEY_WIDTH_INC;

	if (heightInc < MIN_KEY_HEIGHT_INC)
	    heightInc = MIN_KEY_HEIGHT_INC;

	for (unsigned int i = 0; i < NUM_KEYS; i++)
	{
	    if (keycode != key[i])
		continue;

	    if (mask & rKeys[i].warpMask)
	    {
		XWarpPointer (screen->dpy (), None, None, 0, 0, 0, 0,
			      rKeys[i].dx * widthInc, rKeys[i].dy * heightInc);
	    }
	    else
	    {
		int x, y, left, top, width, height;

		CompWindow::Geometry server = w->serverGeometry ();
		CompWindowExtents    input  = w->input ();

		left   = server.x () - input.left;
		top    = server.y () - input.top;
		width  = input.left + server.width () + input.right;
		height = input.top  + server.height () + input.bottom;

		x = left + width  * (rKeys[i].dx + 1) / 2;
		y = top  + height * (rKeys[i].dy + 1) / 2;

		screen->warpPointer (x - pointerX, y - pointerY);

		mask = rKeys[i].resizeMask;

		screen->updateGrab (grabIndex, cursor[i]);
	    }
	    break;
	}
    }
}

void
ResizeScreen::handleMotionEvent (int xRoot, int yRoot)
{
    if (grabIndex)
    {
	BoxRec box;
	int    wi, he;

	wi = savedGeometry.width;
	he = savedGeometry.height;

	if (!mask)
	{
	    int        xDist, yDist;
	    int        minPointerOffsetX, minPointerOffsetY;

	    CompWindow::Geometry server = w->serverGeometry ();

	    xDist = xRoot - (server.x () + (server.width () / 2));
	    yDist = yRoot - (server.y () + (server.height () / 2));

	    /* decision threshold is 10% of window size */
	    minPointerOffsetX = MIN (20, server.width () / 10);
	    minPointerOffsetY = MIN (20, server.height () / 10);

	    /* if we reached the threshold in one direction,
	       make the threshold in the other direction smaller
	       so there is a chance that this threshold also can
	       be reached (by diagonal movement) */
	    if (abs (xDist) > minPointerOffsetX)
		minPointerOffsetY /= 2;
	    else if (abs (yDist) > minPointerOffsetY)
		minPointerOffsetX /= 2;

	    if (abs (xDist) > minPointerOffsetX)
	    {
		if (xDist > 0)
		    mask |= ResizeRightMask;
		else
		    mask |= ResizeLeftMask;
	    }

	    if (abs (yDist) > minPointerOffsetY)
	    {
		if (yDist > 0)
		    mask |= ResizeDownMask;
		else
		    mask |= ResizeUpMask;
	    }

	    /* if the pointer movement was enough to determine a
	       direction, warp the pointer to the appropriate edge
	       and set the right cursor */
	    if (mask)
	    {
		Cursor     cursor;
		CompAction *action;
		int        pointerAdjustX = 0;
		int        pointerAdjustY = 0;
		int	   option = RESIZE_OPTION_INITIATE_KEY;

		action = &opt[option].value ().action ();
		action->setState (action->state () |
				  CompAction::StateTermButton);

		if (mask & ResizeRightMask)
			pointerAdjustX = server.x () + server.width () +
					 w->input ().right - xRoot;
		else if (mask & ResizeLeftMask)
			pointerAdjustX = server.x () - w->input ().left -
					 xRoot;

		if (mask & ResizeDownMask)
			pointerAdjustY = server.x () + server.height () +
					 w->input ().bottom - yRoot;
		else if (mask & ResizeUpMask)
			pointerAdjustY = server.y () - w->input ().top - yRoot;

		screen->warpPointer (pointerAdjustX, pointerAdjustY);

		cursor = cursorFromResizeMask (mask);
		screen->updateGrab (grabIndex, cursor);
	    }
	}
	else
	{
	    /* only accumulate pointer movement if a mask is
	       already set as we don't have a use for the
	       difference information otherwise */
	    pointerDx += xRoot - lastPointerX;
	    pointerDy += yRoot - lastPointerY;
	}

	if (mask & ResizeLeftMask)
	    wi -= pointerDx;
	else if (mask & ResizeRightMask)
	    wi += pointerDx;

	if (mask & ResizeUpMask)
	    he -= pointerDy;
	else if (mask & ResizeDownMask)
	    he += pointerDy;

	if (w->state () & CompWindowStateMaximizedVertMask)
	    he = w->serverGeometry ().height ();

	if (w->state () & CompWindowStateMaximizedHorzMask)
	    wi = w->serverGeometry ().width ();

	w->constrainNewWindowSize (wi, he, &wi, &he);

	if (mode != RESIZE_MODE_NORMAL)
	{
	    if (mode == RESIZE_MODE_STRETCH)
		getStretchRectangle (&box);
	    else
		getPaintRectangle (&box);

	    damageRectangle (&box);
	}

	if (mask & ResizeLeftMask)
	    geometry.x -= wi - geometry.width;

	if (mask & ResizeUpMask)
	    geometry.y -= he - geometry.height;

	geometry.width  = wi;
	geometry.height = he;

	if (mode != RESIZE_MODE_NORMAL)
	{
	    if (mode == RESIZE_MODE_STRETCH)
		getStretchRectangle (&box);
	    else
		getPaintRectangle (&box);

	    damageRectangle (&box);
	}
	else
	{
	    updateWindowSize ();
	}

	updateWindowProperty ();
	sendResizeNotify ();
    }
}

void
ResizeScreen::handleEvent (XEvent *event)
{
    switch (event->type) {
	case KeyPress:
	    if (event->xkey.root == screen->root ())
		handleKeyEvent (event->xkey.keycode);
	    break;
	case ButtonRelease:
	    if (event->xbutton.root == screen->root ())
	    {
		if (grabIndex)
		{
		    if (releaseButton         == -1 ||
			(int) event->xbutton.button == releaseButton)
		    {
			int        opt = RESIZE_OPTION_INITIATE_BUTTON;
			CompAction *action = &this->opt[opt].value ().action ();

			resizeTerminate (action, CompAction::StateTermButton,
					 noOptions);
		    }
		}
	    }
	    break;
	case MotionNotify:
	    if (event->xmotion.root == screen->root ())
		handleMotionEvent (pointerX, pointerY);
	    break;
	case EnterNotify:
	case LeaveNotify:
	    if (event->xcrossing.root == screen->root ())
		handleMotionEvent (pointerX, pointerY);
	    break;
	case ClientMessage:
	    if (event->xclient.message_type == Atoms::wmMoveResize)
	    {
		CompWindow    *w;
		unsigned long type = event->xclient.data.l[2];

		RESIZE_SCREEN (screen);

		if (type <= WmMoveResizeSizeLeft ||
		    type == WmMoveResizeSizeKeyboard)
		{
		    w = screen->findWindow (event->xclient.window);
		    if (w)
		    {
			CompOption::Vector o (0);
			int	       option;

			o.push_back (CompOption ("window",
				     CompOption::TypeInt));
			o[0].value ().set ((int) event->xclient.window);

			if (event->xclient.data.l[2] == WmMoveResizeSizeKeyboard)
			{
			    option = RESIZE_OPTION_INITIATE_KEY;

			    resizeInitiate (&opt[option].value ().action (),
					    CompAction::StateInitKey, o);
			}
			else
			{
			    static unsigned int mask[] = {
				ResizeUpMask | ResizeLeftMask,
				ResizeUpMask,
				ResizeUpMask | ResizeRightMask,
				ResizeRightMask,
				ResizeDownMask | ResizeRightMask,
				ResizeDownMask,
				ResizeDownMask | ResizeLeftMask,
				ResizeLeftMask,
			    };
			    unsigned int mods;
			    Window	     root, child;
			    int	     xRoot, yRoot, i;

			    option = RESIZE_OPTION_INITIATE_BUTTON;

			    XQueryPointer (screen->dpy (),
					   screen->root (),
					   &root, &child, &xRoot, &yRoot,
					   &i, &i, &mods);

			    /* TODO: not only button 1 */
			    if (mods & Button1Mask)
			    {
				o.push_back (CompOption ("modifiers",
					     CompOption::TypeInt));
				o.push_back (CompOption ("x",
					     CompOption::TypeInt));
				o.push_back (CompOption ("y",
					     CompOption::TypeInt));
				o.push_back (CompOption ("direction",
					     CompOption::TypeInt));
				o.push_back (CompOption ("button",
					     CompOption::TypeInt));
				
				o[1].value ().set ((int) mods);
				o[2].value ().set
				    ((int) event->xclient.data.l[0]);
				o[3].value ().set
				    ((int) event->xclient.data.l[1]);
				o[4].value ().set
				    ((int) mask[event->xclient.data.l[2]]);
				o[5].value ().set
				    ((int) (event->xclient.data.l[3] ?
				     event->xclient.data.l[3] : -1));
			
				resizeInitiate (&opt[option].value ().action (),
						CompAction::StateInitButton, o);

				ResizeScreen::get (screen)->
				    handleMotionEvent (xRoot, yRoot);
			    }
			}
		    }
		}
		else if (rs->w && type == WmMoveResizeCancel)
		{
		    if (rs->w->id () == event->xclient.window)
		    {
			int option;

			option = RESIZE_OPTION_INITIATE_BUTTON;
			resizeTerminate (&opt[option].value ().action (),
					 CompAction::StateCancel, noOptions);
			option = RESIZE_OPTION_INITIATE_KEY;
			resizeTerminate (&opt[option].value ().action (),
					 CompAction::StateCancel, noOptions);
		    }
		}
	    }
	    break;
	case DestroyNotify:
	    if (w && w->id () == event->xdestroywindow.window)
	    {
		int option;

		option = RESIZE_OPTION_INITIATE_BUTTON;
		resizeTerminate (&opt[option].value ().action (), 0, noOptions);
		option = RESIZE_OPTION_INITIATE_KEY;
		resizeTerminate (&opt[option].value ().action (), 0, noOptions);
	    }
	    break;
	case UnmapNotify:
	    if (w && w->id () == event->xunmap.window)
	    {
		int option;

		option = RESIZE_OPTION_INITIATE_BUTTON;
		resizeTerminate (&opt[option].value ().action (), 0, noOptions);
		option = RESIZE_OPTION_INITIATE_KEY;
		resizeTerminate (&opt[option].value ().action (), 0, noOptions);
	    }
	default:
	    break;
    }

    screen->handleEvent (event);

    if (event->type == screen->syncEvent () + XSyncAlarmNotify)
    {
	if (w)
	{
	    XSyncAlarmNotifyEvent *sa;

	    sa = (XSyncAlarmNotifyEvent *) event;

	    if (w->syncAlarm () == sa->alarm)
		updateWindowSize ();
	}
    }
}

void
ResizeWindow::resizeNotify (int dx, int dy, int dwidth, int dheight)
{
    window->resizeNotify (dx, dy, dwidth, dheight);

    if (rScreen->w == window && !rScreen->grabIndex)
	rScreen->finishResizing ();
}

void
ResizeScreen::glPaintRectangle (const GLScreenPaintAttrib &sAttrib,
				const GLMatrix            &transform,
				CompOutput                *output,
				unsigned short            *borderColor,
				unsigned short            *fillColor)
{
    BoxRec   box;
    GLMatrix sTransform (transform);

    getPaintRectangle (&box);

    glPushMatrix ();

    sTransform.toScreenSpace (output, -DEFAULT_Z_CAMERA);

    glLoadMatrixf (sTransform.getMatrix ());

    glDisableClientState (GL_TEXTURE_COORD_ARRAY);
    glEnable (GL_BLEND);

    /* fill rectangle */
    if (fillColor)
    {
	glColor4usv (fillColor);
	glRecti (box.x1, box.y2, box.x2, box.y1);
    }

    /* draw outline */
    glColor4usv (borderColor);
    glLineWidth (2.0);
    glBegin (GL_LINE_LOOP);
    glVertex2i (box.x1, box.y1);
    glVertex2i (box.x2, box.y1);
    glVertex2i (box.x2, box.y2);
    glVertex2i (box.x1, box.y2);
    glEnd ();

    /* clean up */
    glColor4usv (defaultColor);
    glDisable (GL_BLEND);
    glEnableClientState (GL_TEXTURE_COORD_ARRAY);
    glPopMatrix ();
}

bool
ResizeScreen::glPaintOutput (const GLScreenPaintAttrib &sAttrib,
			     const GLMatrix            &transform,
			     const CompRegion          &region,
			     CompOutput                *output,
			     unsigned int              mask)
{
    bool status;

    if (w)
    {
	if (mode == RESIZE_MODE_STRETCH)
	    mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;
    }

    status = gScreen->glPaintOutput (sAttrib, transform, region, output, mask);

    if (status && w)
    {
	unsigned short *border, *fill;

	border = opt[RESIZE_OPTION_BORDER_COLOR].value ().c ();
	fill   = opt[RESIZE_OPTION_FILL_COLOR].value ().c ();

	switch (mode) {
	    case RESIZE_MODE_OUTLINE:
		glPaintRectangle (sAttrib, transform, output, border, NULL);
		break;
	    case RESIZE_MODE_RECTANGLE:
		glPaintRectangle (sAttrib, transform, output, border, fill);
	    default:
		break;
	}
    }

    return status;
}

bool
ResizeWindow::glPaint (const GLWindowPaintAttrib &attrib,
		       const GLMatrix            &transform,
		       const CompRegion          &region,
		       unsigned int              mask)
{
    bool       status;

    if (window == rScreen->w && rScreen->mode == RESIZE_MODE_STRETCH)
    {
	GLMatrix       wTransform (transform);
	BoxRec	       box;
	float	       xOrigin, yOrigin;
	float	       xScale, yScale;
	int            x, y;

	if (mask & PAINT_WINDOW_OCCLUSION_DETECTION_MASK)
	    return false;

	status = gWindow->glPaint (attrib, transform, region,
				   mask | PAINT_WINDOW_NO_CORE_INSTANCE_MASK);

	GLFragment::Attrib fragment (gWindow->lastPaintAttrib ());

	if (window->alpha () || fragment.getOpacity () != OPAQUE)
	    mask |= PAINT_WINDOW_TRANSLUCENT_MASK;

	rScreen->getPaintRectangle (&box);
	getStretchScale (&box, &xScale, &yScale);

	x = window->geometry (). x ();
	y = window->geometry (). y ();

	xOrigin = x - window->input ().left;
	yOrigin = y - window->input ().top;

	wTransform.translate (xOrigin, yOrigin, 0.0f);
	wTransform.scale (xScale, yScale, 1.0f);
	wTransform.translate ((rScreen->geometry.x - x) / xScale - xOrigin,
			      (rScreen->geometry.y - y) / yScale - yOrigin,
			      0.0f);

	glPushMatrix ();
	glLoadMatrixf (wTransform.getMatrix ());

	gWindow->glDraw (wTransform, fragment, region,
			 mask | PAINT_WINDOW_TRANSFORMED_MASK);

	glPopMatrix ();
    }
    else
    {
	status = gWindow->glPaint (attrib, transform, region, mask);
    }

    return status;
}

bool
ResizeWindow::damageRect (bool initial, const CompRect &rect)
{
    bool status = false;

    if (window == rScreen->w && rScreen->mode == RESIZE_MODE_STRETCH)
    {
	BoxRec box;

	rScreen->getStretchRectangle (&box);
	rScreen->damageRectangle (&box);

	status = true;
    }

    status |= cWindow->damageRect (initial, rect);

    return status;
}

CompOption::Vector &
ResizeScreen::getOptions ()
{
    return opt;
}
 
bool
ResizeScreen::setOption (const char        *name,
			 CompOption::Value &value)
{
    CompOption   *o;
    unsigned int index;

    o = CompOption::findOption (opt, name, &index);
    if (!o)
	return false;

    return CompOption::setOption (*o, value);

}

static const CompMetadata::OptionInfo resizeOptionInfo[] = {
    { "initiate_normal_key", "key", 0, resizeInitiate, resizeTerminate },
    { "initiate_outline_key", "key", 0, resizeInitiate, resizeTerminate },
    { "initiate_rectangle_key", "key", 0, resizeInitiate, resizeTerminate },
    { "initiate_stretch_key", "key", 0, resizeInitiate, resizeTerminate },
    { "initiate_button", "button", 0, resizeInitiate, resizeTerminate },
    { "initiate_key", "key", 0, resizeInitiate, resizeTerminate },
    { "mode", "int", RESTOSTRING (0, RESIZE_MODE_LAST), 0, 0 },
    { "border_color", "color", 0, 0, 0 },
    { "fill_color", "color", 0, 0, 0 },
    { "normal_match", "match", 0, 0, 0 },
    { "outline_match", "match", 0, 0, 0 },
    { "rectangle_match", "match", 0, 0, 0 },
    { "stretch_match", "match", 0, 0, 0 }
};

ResizeScreen::ResizeScreen (CompScreen *s) :
    PrivateHandler<ResizeScreen,CompScreen> (s),
    gScreen (GLScreen::get (s)),
    cScreen (CompositeScreen::get (s)),
    w (NULL),
    releaseButton (0),
    opt (RESIZE_OPTION_NUM)
{

    Display *dpy = s->dpy ();

    if (!resizeMetadata->initOptions (resizeOptionInfo, RESIZE_OPTION_NUM, opt))
    {
	setFailed ();
	return;
    }

    resizeNotifyAtom      = XInternAtom (s->dpy (),
					 "_COMPIZ_RESIZE_NOTIFY", 0);
    resizeInformationAtom = XInternAtom (s->dpy (),
					 "_COMPIZ_RESIZE_INFORMATION", 0);

    for (unsigned int i = 0; i < NUM_KEYS; i++)
	key[i] = XKeysymToKeycode (s->dpy (), XStringToKeysym (rKeys[i].name));

    grabIndex = 0;

    leftCursor      = XCreateFontCursor (dpy, XC_left_side);
    rightCursor     = XCreateFontCursor (dpy, XC_right_side);
    upCursor        = XCreateFontCursor (dpy, XC_top_side);
    upLeftCursor    = XCreateFontCursor (dpy, XC_top_left_corner);
    upRightCursor   = XCreateFontCursor (dpy, XC_top_right_corner);
    downCursor      = XCreateFontCursor (dpy, XC_bottom_side);
    downLeftCursor  = XCreateFontCursor (dpy, XC_bottom_left_corner);
    downRightCursor = XCreateFontCursor (dpy, XC_bottom_right_corner);
    middleCursor    = XCreateFontCursor (dpy, XC_fleur);

    cursor[0] = leftCursor;
    cursor[1] = rightCursor;
    cursor[2] = upCursor;
    cursor[3] = downCursor;

    ScreenInterface::setHandler (s);

    if (gScreen)
	GLScreenInterface::setHandler (gScreen, false);
}

ResizeScreen::~ResizeScreen ()
{
    Display *dpy = screen->dpy ();

    if (leftCursor)
	XFreeCursor (dpy, leftCursor);
    if (rightCursor)
	XFreeCursor (dpy, rightCursor);
    if (upCursor)
	XFreeCursor (dpy, upCursor);
    if (downCursor)
	XFreeCursor (dpy, downCursor);
    if (middleCursor)
	XFreeCursor (dpy, middleCursor);
    if (upLeftCursor)
	XFreeCursor (dpy, upLeftCursor);
    if (upRightCursor)
	XFreeCursor (dpy, upRightCursor);
    if (downLeftCursor)
	XFreeCursor (dpy, downLeftCursor);
    if (downRightCursor)
	XFreeCursor (dpy, downRightCursor);
}

ResizeWindow::ResizeWindow (CompWindow *w) :
    PrivateHandler<ResizeWindow,CompWindow> (w),
    window (w),
    gWindow (GLWindow::get (w)),
    cWindow (CompositeWindow::get (w)),
    rScreen (ResizeScreen::get (screen))
{
    WindowInterface::setHandler (window);

    if (cWindow)
	CompositeWindowInterface::setHandler (cWindow, false);

    if (gWindow)
	GLWindowInterface::setHandler (gWindow, false);
}


ResizeWindow::~ResizeWindow ()
{
}


bool
ResizePluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	 return false;

    resizeMetadata = new CompMetadata (name (),resizeOptionInfo,
				       RESIZE_OPTION_NUM);
    if (!resizeMetadata)
	return false;

    resizeMetadata->addFromFile (name ());

    return true;
}

void
ResizePluginVTable::fini ()
{
    delete resizeMetadata;
}

CompMetadata *
ResizePluginVTable::getMetadata ()
{
    return resizeMetadata;
}

ResizePluginVTable resizeVTable;

CompPlugin::VTable *
getCompPluginInfo20080805 (void)
{
    return &resizeVTable;
}
