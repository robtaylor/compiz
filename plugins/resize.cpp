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

#include <X11/Xatom.h>
#include <X11/cursorfont.h>

#include <compiz-core.h>
#include "resize.h"

static CompMetadata *resizeMetadata;

void
ResizeDisplay::getPaintRectangle (BoxPtr pBox)
{
    pBox->x1 = geometry.x - w->input ().left;
    pBox->y1 = geometry.y - w->input ().top;
    pBox->x2 = geometry.x +
	geometry.width + w->serverGeometry ().border () * 2 +
	w->input ().right;

    if (w->shaded ())
    {
	pBox->y2 = geometry.y + w->height () + w->input ().bottom;
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

    width  = window->width () + window->input ().left + window->input ().right;
    height = window->height () + window->input ().top + window->input ().bottom;

    *xScale = (width)  ? (pBox->x2 - pBox->x1) / (float) width  : 1.0f;
    *yScale = (height) ? (pBox->y2 - pBox->y1) / (float) height : 1.0f;
}

void
ResizeDisplay::getStretchRectangle (BoxPtr pBox)
{
    BoxRec box;
    float  xScale, yScale;

    getPaintRectangle (&box);
    ResizeWindow::get (w)->getStretchScale (&box, &xScale, &yScale);

    pBox->x1 = box.x1 - (w->output ().left - w->input ().left) * xScale;
    pBox->y1 = box.y1 - (w->output ().top - w->input ().top) * yScale;
    pBox->x2 = box.x2 + w->output ().right * xScale;
    pBox->y2 = box.y2 + w->output ().bottom * yScale;
}

void
ResizeScreen::damageRectangle (BoxPtr pBox)
{
    REGION reg;

    reg.rects    = &reg.extents;
    reg.numRects = 1;

    reg.extents = *pBox;

    reg.extents.x1 -= 1;
    reg.extents.y1 -= 1;
    reg.extents.x2 += 1;
    reg.extents.y2 += 1;

    if (cScreen)
	cScreen->damageRegion (&reg);
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
ResizeDisplay::sendResizeNotify ()
{
    XEvent xev;

    xev.xclient.type    = ClientMessage;
    xev.xclient.display = display->dpy ();
    xev.xclient.format  = 32;

    xev.xclient.message_type = resizeNotifyAtom;
    xev.xclient.window	     = w->id ();

    xev.xclient.data.l[0] = geometry.x;
    xev.xclient.data.l[1] = geometry.y;
    xev.xclient.data.l[2] = geometry.width;
    xev.xclient.data.l[3] = geometry.height;
    xev.xclient.data.l[4] = 0;

    XSendEvent (display->dpy (), w->screen ()->root (),	FALSE,
		SubstructureRedirectMask | SubstructureNotifyMask, &xev);
}

void
ResizeDisplay::updateWindowProperty ()
{
    unsigned long data[4];

    data[0] = geometry.x;
    data[1] = geometry.y;
    data[2] = geometry.width;
    data[3] = geometry.height;

    XChangeProperty (display->dpy (), w->id (), resizeInformationAtom,
		     XA_CARDINAL, 32, PropModeReplace,
        	     (unsigned char*) data, 4);
}

void
ResizeDisplay::finishResizing ()
{
    w->ungrabNotify ();

    XDeleteProperty (display->dpy (), w->id (), resizeInformationAtom);

    w = NULL;
}

static bool
resizeInitiate (CompDisplay        *d,
	        CompAction         *action,
	        CompAction::State  state,
	        CompOption::Vector &options)
{
    CompWindow *w;
    Window     xid;

    RESIZE_DISPLAY (d);

    xid = CompOption::getIntOptionNamed (options, "window");

    w = d->findWindow (xid);
    if (w && (w->actions () & CompWindowActionResizeMask))
    {
	unsigned int mask;
	int          x, y;
	int	     button;
	int	     i;

	RESIZE_SCREEN (w->screen ());

	CompWindow::Geometry server = w->serverGeometry ();
	
	x = CompOption::getIntOptionNamed (options, "x", pointerX);
	y = CompOption::getIntOptionNamed (options, "y", pointerY);

	button = CompOption::getIntOptionNamed (options, "button", -1);

	mask = CompOption::getIntOptionNamed (options, "direction");

	/* Initiate the resize in the direction suggested by the
	 * quarter of the window the mouse is in, eg drag in top left
	 * will resize up and to the left.  Keyboard resize starts out
	 * with the cursor in the middle of the window and then starts
	 * resizing the edge corresponding to the next key press. */
	if (state & CompAction::StateInitKey)
	{
	    mask = 0;
	}
	else if (!mask)
	{
	    mask |= ((x - server.x ()) < ((int) server.width () / 2)) ?
		ResizeLeftMask : ResizeRightMask;

	    mask |= ((y - server.y ()) < ((int) server.height () / 2)) ?
		ResizeUpMask : ResizeDownMask;
	}

	if (w->screen ()->otherGrabExist ("resize", 0))
	    return false;

	if (rd->w)
	    return false;

	if (w->type () & (CompWindowTypeDesktopMask |
		          CompWindowTypeDockMask	 |
		          CompWindowTypeFullscreenMask))
	    return false;

	if (w->attrib ().override_redirect)
	    return false;

	if (state & CompAction::StateInitButton)
	    action->setState (action->state () | CompAction::StateTermButton);

	if (w->shaded ())
	    mask &= ~(ResizeUpMask | ResizeDownMask);

	rd->w	 = w;
	rd->mask = mask;

	rd->savedGeometry.x	 = server.x ();
	rd->savedGeometry.y	 = server.y ();
	rd->savedGeometry.width  = server.width ();
	rd->savedGeometry.height = server.height ();

	rd->geometry = rd->savedGeometry;

	rd->pointerDx = x - pointerX;
	rd->pointerDy = y - pointerY;

	if ((w->state () & MAXIMIZE_STATE) == MAXIMIZE_STATE)
	{
	    /* if the window is fully maximized, showing the outline or
	       rectangle would be visually distracting as the window can't
	       be resized anyway; so we better don't use them in this case */
	    rd->mode = RESIZE_MODE_NORMAL;
	}
	else
	{
	    rd->mode = rd->opt[RESIZE_DISPLAY_OPTION_MODE].value ().i ();
	    for (i = 0; i <= RESIZE_MODE_LAST; i++)
	    {
		if (action == &rd->opt[i].value ().action ())
		{
		    rd->mode = i;
		    break;
		}
	    }

	    if (i > RESIZE_MODE_LAST)
	    {
		int index;

		for (i = 0; i <= RESIZE_MODE_LAST; i++)
		{
		    index = RESIZE_DISPLAY_OPTION_NORMAL_MATCH + i;
		    if (rd->opt[index].value ().match ().evaluate (w))
		    {
			rd->mode = i;
			break;
		    }
		}
	    }

	    if (!GLScreen::get (w->screen ()) ||
		!CompositeScreen::get (w->screen ()) ||
		!CompositeScreen::get (w->screen ())->compositingActive ())
		rd->mode = RESIZE_MODE_NORMAL;
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

	    rs->grabIndex = w->screen ()->pushGrab (cursor, "resize");
	}

	if (rs->grabIndex)
	{
	    BoxRec box;

	    rd->releaseButton = button;

	    w->grabNotify (x, y, state, CompWindowGrabResizeMask |
			   CompWindowGrabButtonMask);

	    /* using the paint rectangle is enough here
	       as we don't have any stretch yet */
	    rd->getPaintRectangle (&box);
	    rs->damageRectangle (&box);

	    if (state & CompAction::StateInitKey)
	    {
		int xRoot, yRoot;

		xRoot = server.x () + (server.width () / 2);
		yRoot = server.y () + (server.height () / 2);

		w->screen ()->warpPointer (xRoot - pointerX, yRoot - pointerY);
	    }
	}
    }

    return false;
}

static bool
resizeTerminate (CompDisplay        *d,
	         CompAction         *action,
	         CompAction::State  state,
	         CompOption::Vector &options)
{
    RESIZE_DISPLAY (d);

    if (rd->w)
    {
	CompWindow     *w = rd->w;
	XWindowChanges xwc;
	unsigned int   mask = 0;

	RESIZE_SCREEN (w->screen ());

	if (rd->mode == RESIZE_MODE_NORMAL)
	{
	    if (state & CompAction::StateCancel)
	    {
		xwc.x      = rd->savedGeometry.x;
		xwc.y      = rd->savedGeometry.y;
		xwc.width  = rd->savedGeometry.width;
		xwc.height = rd->savedGeometry.height;

		mask = CWX | CWY | CWWidth | CWHeight;
	    }
	}
	else
	{
	    XRectangle geometry;

	    if (state & CompAction::StateCancel)
		geometry = rd->savedGeometry;
	    else
		geometry = rd->geometry;

	    if (memcmp (&geometry, &rd->savedGeometry, sizeof (geometry)) == 0)
	    {
		BoxRec box;

		if (rd->mode == RESIZE_MODE_STRETCH)
		    rd->getStretchRectangle (&box);
		else
		    rd->getPaintRectangle (&box);

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
	    rd->finishResizing ();

	if (rs->grabIndex)
	{
	    w->screen ()->removeGrab (rs->grabIndex, NULL);
	    rs->grabIndex = 0;
	}

	rd->releaseButton = 0;
    }

    action->setState (action->state () & ~(CompAction::StateTermKey |
		      CompAction::StateTermButton));

    return false;
}


void
ResizeDisplay::updateWindowSize ()
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
    if (grabIndex && rDisplay->w)
    {
	CompWindow *w = rDisplay->w;
	int	   widthInc, heightInc;

	widthInc  = w->sizeHints ().width_inc;
	heightInc = w->sizeHints ().height_inc;

	if (widthInc < MIN_KEY_WIDTH_INC)
	    widthInc = MIN_KEY_WIDTH_INC;

	if (heightInc < MIN_KEY_HEIGHT_INC)
	    heightInc = MIN_KEY_HEIGHT_INC;

	for (unsigned int i = 0; i < NUM_KEYS; i++)
	{
	    if (keycode != rDisplay->key[i])
		continue;

	    if (rDisplay->mask & rKeys[i].warpMask)
	    {
		XWarpPointer (rDisplay->display->dpy (), None, None, 0, 0, 0, 0,
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

		rDisplay->mask = rKeys[i].resizeMask;

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
	int    w, h;

	w = rDisplay->savedGeometry.width;
	h = rDisplay->savedGeometry.height;

	if (!rDisplay->mask)
	{
	    CompWindow *w = rDisplay->w;
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
		    rDisplay->mask |= ResizeRightMask;
		else
		    rDisplay->mask |= ResizeLeftMask;
	    }

	    if (abs (yDist) > minPointerOffsetY)
	    {
		if (yDist > 0)
		    rDisplay->mask |= ResizeDownMask;
		else
		    rDisplay->mask |= ResizeUpMask;
	    }

	    /* if the pointer movement was enough to determine a
	       direction, warp the pointer to the appropriate edge
	       and set the right cursor */
	    if (rDisplay->mask)
	    {
		Cursor     cursor;
		CompAction *action;
		int        pointerAdjustX = 0;
		int        pointerAdjustY = 0;
		int	   option = RESIZE_DISPLAY_OPTION_INITIATE_KEY;

		action = &rDisplay->opt[option].value ().action ();
		action->setState (action->state () |
				  CompAction::StateTermButton);

		if (rDisplay->mask & ResizeRightMask)
			pointerAdjustX = server.x () + server.width () +
					 w->input ().right - xRoot;
		else if (rDisplay->mask & ResizeLeftMask)
			pointerAdjustX = server.x () - w->input ().left -
					 xRoot;

		if (rDisplay->mask & ResizeDownMask)
			pointerAdjustY = server.x () + server.height () +
					 w->input ().bottom - yRoot;
		else if (rDisplay->mask & ResizeUpMask)
			pointerAdjustY = server.y () - w->input ().top - yRoot;

		screen->warpPointer (pointerAdjustX, pointerAdjustY);

		cursor = cursorFromResizeMask (rDisplay->mask);
		screen->updateGrab (grabIndex, cursor);
	    }
	}
	else
	{
	    /* only accumulate pointer movement if a mask is
	       already set as we don't have a use for the
	       difference information otherwise */
	    rDisplay->pointerDx += xRoot - lastPointerX;
	    rDisplay->pointerDy += yRoot - lastPointerY;
	}

	if (rDisplay->mask & ResizeLeftMask)
	    w -= rDisplay->pointerDx;
	else if (rDisplay->mask & ResizeRightMask)
	    w += rDisplay->pointerDx;

	if (rDisplay->mask & ResizeUpMask)
	    h -= rDisplay->pointerDy;
	else if (rDisplay->mask & ResizeDownMask)
	    h += rDisplay->pointerDy;

	if (rDisplay->w->state () & CompWindowStateMaximizedVertMask)
	    h = rDisplay->w->serverGeometry ().height ();

	if (rDisplay->w->state () & CompWindowStateMaximizedHorzMask)
	    w = rDisplay->w->serverGeometry ().width ();

	rDisplay->w->constrainNewWindowSize (w, h, &w, &h);

	if (rDisplay->mode != RESIZE_MODE_NORMAL)
	{
	    if (rDisplay->mode == RESIZE_MODE_STRETCH)
		rDisplay->getStretchRectangle (&box);
	    else
		rDisplay->getPaintRectangle (&box);

	    damageRectangle (&box);
	}

	if (rDisplay->mask & ResizeLeftMask)
	    rDisplay->geometry.x -= w - rDisplay->geometry.width;

	if (rDisplay->mask & ResizeUpMask)
	    rDisplay->geometry.y -= h - rDisplay->geometry.height;

	rDisplay->geometry.width  = w;
	rDisplay->geometry.height = h;

	if (rDisplay->mode != RESIZE_MODE_NORMAL)
	{
	    if (rDisplay->mode == RESIZE_MODE_STRETCH)
		rDisplay->getStretchRectangle (&box);
	    else
		rDisplay->getPaintRectangle (&box);

	    damageRectangle (&box);
	}
	else
	{
	    rDisplay->updateWindowSize ();
	}

	rDisplay->updateWindowProperty ();
	rDisplay->sendResizeNotify ();
    }
}

void
ResizeDisplay::handleEvent (XEvent *event)
{
    CompScreen *s;

    switch (event->type) {
	case KeyPress:
	    s = display->findScreen (event->xkey.root);
	    if (s)
		ResizeScreen::get (s)->handleKeyEvent (event->xkey.keycode);
	    break;
	case ButtonRelease:
	    s = display->findScreen (event->xbutton.root);
	    if (s)
	    {
		RESIZE_SCREEN (s);

		if (rs->grabIndex)
		{
		    if (releaseButton         == -1 ||
			(int) event->xbutton.button == releaseButton)
		    {
			int        opt = RESIZE_DISPLAY_OPTION_INITIATE_BUTTON;
			CompAction *action = &this->opt[opt].value ().action ();

			resizeTerminate (display, action,
					 CompAction::StateTermButton,
					 noOptions);
		    }
		}
	    }
	    break;
	case MotionNotify:
	    s = display->findScreen (event->xmotion.root);
	    if (s)
		ResizeScreen::get (s)->handleMotionEvent (pointerX, pointerY);
	    break;
	case EnterNotify:
	case LeaveNotify:
	    s = display->findScreen (event->xcrossing.root);
	    if (s)
		ResizeScreen::get (s)->handleMotionEvent (pointerX, pointerY);
	    break;
	case ClientMessage:
	    if (event->xclient.message_type ==
		display->atoms ().wmMoveResize)
	    {
		CompWindow *w;

		if (event->xclient.data.l[2] <= WmMoveResizeSizeLeft ||
		    event->xclient.data.l[2] == WmMoveResizeSizeKeyboard)
		{
		    w = display->findWindow (event->xclient.window);
		    if (w)
		    {
			CompOption::Vector o (0);
			int	       option;

			o.push_back (CompOption ("window",
				     CompOption::TypeInt));
			o[0].value ().set ((int) event->xclient.window);

			if (event->xclient.data.l[2] == WmMoveResizeSizeKeyboard)
			{
			    option = RESIZE_DISPLAY_OPTION_INITIATE_KEY;

			    resizeInitiate (display,
					    &opt[option].value ().action (),
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

			    option = RESIZE_DISPLAY_OPTION_INITIATE_BUTTON;

			    XQueryPointer (display->dpy (),
					   w->screen ()->root (),
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
			
				resizeInitiate (display,
						&opt[option].value ().action (),
						CompAction::StateInitButton, o);

				ResizeScreen::get (w->screen ())->
				    handleMotionEvent (xRoot, yRoot);
			    }
			}
		    }
		}
	    }
	    break;
	case DestroyNotify:
	    if (w && w->id () == event->xdestroywindow.window)
	    {
		int option;

		option = RESIZE_DISPLAY_OPTION_INITIATE_BUTTON;
		resizeTerminate (display, &opt[option].value ().action (), 0,
				 noOptions);
		option = RESIZE_DISPLAY_OPTION_INITIATE_KEY;
		resizeTerminate (display, &opt[option].value ().action (), 0,
				 noOptions);
	    }
	    break;
	case UnmapNotify:
	    if (w && w->id () == event->xunmap.window)
	    {
		int option;

		option = RESIZE_DISPLAY_OPTION_INITIATE_BUTTON;
		resizeTerminate (display, &opt[option].value ().action (), 0,
				 noOptions);
		option = RESIZE_DISPLAY_OPTION_INITIATE_KEY;
		resizeTerminate (display, &opt[option].value ().action (), 0,
				 noOptions);
	    }
	default:
	    break;
    }

    display->handleEvent (event);

    if (event->type == display->syncEvent () + XSyncAlarmNotify)
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

    if (rDisplay->w == window && !rScreen->grabIndex)
	rDisplay->finishResizing ();
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

    rDisplay->getPaintRectangle (&box);

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
			     Region                    region,
			     CompOutput                *output,
			     unsigned int              mask)
{
    bool status;

    if (rDisplay->w && (screen == rDisplay->w->screen ()))
    {
	if (rDisplay->mode == RESIZE_MODE_STRETCH)
	    mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;
    }

    status = gScreen->glPaintOutput (sAttrib, transform, region, output, mask);

    if (status && rDisplay->w && (screen == rDisplay->w->screen ()))
    {
	unsigned short *border, *fill;

	border =
	    rDisplay->opt[RESIZE_DISPLAY_OPTION_BORDER_COLOR].value ().c ();
	fill   = rDisplay->opt[RESIZE_DISPLAY_OPTION_FILL_COLOR].value ().c ();

	switch (rDisplay->mode) {
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
		       Region                    region,
		       unsigned int              mask)
{
    bool       status;

    if (window == rDisplay->w && rDisplay->mode == RESIZE_MODE_STRETCH)
    {
	GLMatrix       wTransform (transform);
	BoxRec	       box;
	float	       xOrigin, yOrigin;
	float	       xScale, yScale;

	if (mask & PAINT_WINDOW_OCCLUSION_DETECTION_MASK)
	    return false;

	status = gWindow->glPaint (attrib, transform, region,
				   mask | PAINT_WINDOW_NO_CORE_INSTANCE_MASK);

	GLFragment::Attrib fragment (gWindow->lastPaintAttrib ());

	if (window->alpha () || fragment.getOpacity () != OPAQUE)
	    mask |= PAINT_WINDOW_TRANSLUCENT_MASK;

	rDisplay->getPaintRectangle (&box);
	getStretchScale (&box, &xScale, &yScale);

	xOrigin = window->attrib ().x - window->input ().left;
	yOrigin = window->attrib ().y - window->input ().top;

	wTransform.translate (xOrigin, yOrigin, 0.0f);
	wTransform.scale (xScale, yScale, 1.0f);
	wTransform.translate (
	    (rDisplay->geometry.x - window->attrib ().x) / xScale - xOrigin,
	    (rDisplay->geometry.y - window->attrib ().y) / yScale - yOrigin,
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
ResizeWindow::damageRect (bool initial, BoxPtr rect)
{
    bool status = false;

    if (window == rDisplay->w && rDisplay->mode == RESIZE_MODE_STRETCH)
    {
	BoxRec box;

	rDisplay->getStretchRectangle (&box);
	rScreen->damageRectangle (&box);

	status = true;
    }

    status |= cWindow->damageRect (initial, rect);

    return status;
}

CompOption::Vector &
ResizeDisplay::getOptions ()
{
    return opt;
}
 
bool
ResizeDisplay::setOption (const char        *name,
			  CompOption::Value &value)
{
    CompOption   *o;
    unsigned int index;

    o = CompOption::findOption (opt, name, &index);
    if (!o)
	return false;

    return CompOption::setDisplayOption (display, *o, value);

}

static const CompMetadata::OptionInfo resizeDisplayOptionInfo[] = {
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

ResizeDisplay::ResizeDisplay (CompDisplay *d) :
    PrivateHandler<ResizeDisplay,CompDisplay> (d),
    display (d),
    w (NULL),
    releaseButton (0),
    opt (RESIZE_DISPLAY_OPTION_NUM)
{

    if (!resizeMetadata->initDisplayOptions (d, resizeDisplayOptionInfo,
					     RESIZE_DISPLAY_OPTION_NUM, opt))
    {
	setFailed ();
	return;
    }

    resizeNotifyAtom      = XInternAtom (d->dpy (),
					 "_COMPIZ_RESIZE_NOTIFY", 0);
    resizeInformationAtom = XInternAtom (d->dpy (),
					 "_COMPIZ_RESIZE_INFORMATION", 0);

    for (unsigned int i = 0; i < NUM_KEYS; i++)
	key[i] = XKeysymToKeycode (d->dpy (), XStringToKeysym (rKeys[i].name));

    DisplayInterface::setHandler (d);
}

ResizeDisplay::~ResizeDisplay ()
{
    CompOption::finiDisplayOptions (display, opt);
}

ResizeScreen::ResizeScreen (CompScreen *s) :
    PrivateHandler<ResizeScreen,CompScreen> (s),
    screen (s),
    gScreen (GLScreen::get (s)),
    cScreen (CompositeScreen::get (s)),
    rDisplay (ResizeDisplay::get (s->display ()))
{

    Display *dpy = screen->display ()->dpy ();

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

    if (gScreen)
	GLScreenInterface::setHandler (gScreen);
}

ResizeScreen::~ResizeScreen ()
{
    Display *dpy = screen->display ()->dpy ();

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
    rScreen (ResizeScreen::get (w->screen ())),
    rDisplay (ResizeDisplay::get (w->screen ()->display ()))
{
    WindowInterface::setHandler (window);

    if (cWindow)
	CompositeWindowInterface::setHandler (cWindow);

    if (gWindow)
	GLWindowInterface::setHandler (gWindow);
}


ResizeWindow::~ResizeWindow ()
{

}

bool
ResizePluginVTable::initObject (CompObject *o)
{
    INIT_OBJECT (o,_,X,X,X,,ResizeDisplay,ResizeScreen,ResizeWindow)
    return true;
}

void
ResizePluginVTable::finiObject (CompObject *o)
{
    FINI_OBJECT (o,_,X,X,X,,ResizeDisplay,ResizeScreen,ResizeWindow)
}

CompOption::Vector &
ResizePluginVTable::getObjectOptions (CompObject *object)
{
    GET_OBJECT_OPTIONS (object,X,_,ResizeDisplay,)
    return noOptions;
}

bool
ResizePluginVTable::setObjectOption (CompObject        *object,
				     const char        *name,
				     CompOption::Value &value)
{
    SET_OBJECT_OPTION (object,X,_,ResizeDisplay,)
    return false;
}

bool
ResizePluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	 return false;

    resizeMetadata = new CompMetadata (name (),
				       resizeDisplayOptionInfo,
				       RESIZE_DISPLAY_OPTION_NUM,
				       0, 0);
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