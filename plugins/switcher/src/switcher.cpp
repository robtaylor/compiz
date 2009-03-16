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

#include "switcher.h"

COMPIZ_PLUGIN_20090315 (switcher, SwitchPluginVTable)

static float _boxVertices[] =
{
    -(WIDTH >> 1), 0,
    -(WIDTH >> 1), BOX_WIDTH,
     (WIDTH >> 1), BOX_WIDTH,
     (WIDTH >> 1), 0,

    -(WIDTH >> 1),	       BOX_WIDTH,
    -(WIDTH >> 1),	       HEIGHT - BOX_WIDTH,
    -(WIDTH >> 1) + BOX_WIDTH, HEIGHT - BOX_WIDTH,
    -(WIDTH >> 1) + BOX_WIDTH, 0,

     (WIDTH >> 1) - BOX_WIDTH, BOX_WIDTH,
     (WIDTH >> 1) - BOX_WIDTH, HEIGHT - BOX_WIDTH,
     (WIDTH >> 1),	       HEIGHT - BOX_WIDTH,
     (WIDTH >> 1),	       0,

    -(WIDTH >> 1), HEIGHT - BOX_WIDTH,
    -(WIDTH >> 1), HEIGHT,
     (WIDTH >> 1), HEIGHT,
     (WIDTH >> 1), HEIGHT - BOX_WIDTH
};


void
SwitchScreen::setSelectedWindowHint ()
{
    XChangeProperty (screen->dpy (), popupWindow, selectWinAtom,
		     XA_WINDOW, 32, PropModeReplace,
		     (unsigned char *) &selectedWindow, 1);
}

bool
SwitchWindow::isSwitchWin ()
{
    if (!window->isViewable ())
    {
	if (sScreen->optionGetMinimized ())
	{
	    if (!window->minimized () && !window->inShowDesktopMode () &&
		!window->shaded ())
		return false;
	}
	else
	{
	    return false;
	}
    }

    if (!window->isFocussable ())
	return false;

    if (window->overrideRedirect ())
	return false;

    if (sScreen->selection == Panels)
    {
	if (!(window->type () &
	      (CompWindowTypeDockMask | CompWindowTypeDesktopMask)))
	    return false;
    }
    else
    {
	CompMatch *match;

	if (window->wmType () &
	    (CompWindowTypeDockMask | CompWindowTypeDesktopMask))
	    return false;

	if (window->state () & CompWindowStateSkipTaskbarMask)
	    return false;

	match = &sScreen->optionGetWindowMatch ();
	if (!match->evaluate (window))
	    return false;

    }

    if (sScreen->selection == CurrentViewport)
    {
	if (!window->mapNum () || !window->isViewable ())
	{
	    CompWindow::Geometry &sg = window->serverGeometry ();
	    if (sg.x () + sg.width ()  <= 0    ||
		sg.y () + sg.height () <= 0    ||
		sg.x () >= screen->width () ||
		sg.y () >= screen->height ())
		return false;
	}
	else
	{
	    if (!window->focus ())
		return false;
	}
    }

    return true;
}

void
SwitchScreen::activateEvent (bool activating)
{
    CompOption::Vector o (0);

    o.push_back (CompOption ("root", CompOption::TypeInt));
    o.push_back (CompOption ("active", CompOption::TypeBool));

    o[0].value ().set ((int) screen->root ());
    o[1].value ().set (activating);

    screen->handleCompizEvent ("switcher", "activate", o);
}


static bool
compareWindows (CompWindow *w1,
		CompWindow *w2)
{

    if (w1->mapNum () && !w2->mapNum ())
	return true;

    if (w2->mapNum () && !w1->mapNum ())
	return false;

    return w2->activeNum () < w1->activeNum ();
}


void
SwitchScreen::updateWindowList (int count)
{
    int x, y;

    if (count > 1)
    {
	count -= (count + 1) & 1;
	if (count < 3)
	    count = 3;
    }

    pos  = ((count >> 1) - windows.size ()) * WIDTH;
    move = 0;

    selectedWindow = windows.front ()->id ();

    x = screen->currentOutputDev ().x1 () +
	screen->currentOutputDev ().width () / 2;
    y = screen->currentOutputDev ().y1 () +
	screen->currentOutputDev ().height () / 2;

    if (popupWindow)
    {
	CompWindow *w = screen->findWindow (popupWindow);

	if (w)
	    w->resize (x - WINDOW_WIDTH (count) / 2,
		       y - WINDOW_HEIGHT / 2,
		       WINDOW_WIDTH (count),
		       WINDOW_HEIGHT);
	else
	    XMoveResizeWindow (screen->dpy (), popupWindow,
			       x - WINDOW_WIDTH (count) / 2,
			       y - WINDOW_HEIGHT / 2,
			       WINDOW_WIDTH (count),
			       WINDOW_HEIGHT);
    }
}

void
SwitchScreen::createWindowList (int count)
{

    windows.clear ();

    foreach (CompWindow *w, screen->windows ())
    {
	if (SwitchWindow::get (w)->isSwitchWin ())
	{
	    SWITCH_WINDOW (w);
	    windows.push_back (w);

	    sw->cWindow->damageRectSetEnabled (sw, true);
	}
    }

    windows.sort (compareWindows);

    if (windows.size () == 2)
    {
	windows.push_back (windows.front ());
	windows.push_back ((*++windows.begin ()));
    }

    updateWindowList (count);
}

void
SwitchScreen::switchToWindow (bool toNext)
{
    CompWindow               *w = NULL;
    CompWindowList::iterator it;

    if (!grabIndex)
	return;

    for (it = windows.begin (); it != windows.end (); it++)
    {
	if ((*it)->id () == selectedWindow)
	    break;
    }

    if (it == windows.end ())
	return;

    if (toNext)
    {
	it++;
	if (it == windows.end ())
	    w = windows.front ();
	else
	    w = *it;
    }
    else
    {
	if (it == windows.begin ())
	    w = windows.back ();
	else
	    w = *--it;
    }

    if (w)
    {
	Window old = selectedWindow;

	if (selection == AllViewports && optionGetAutoRotate ())
	{
	    XEvent xev;
	    CompPoint pnt = w->defaultViewport ();

	    xev.xclient.type = ClientMessage;
	    xev.xclient.display = screen->dpy ();
	    xev.xclient.format = 32;

	    xev.xclient.message_type = Atoms::desktopViewport;
	    xev.xclient.window = screen->root ();

	    xev.xclient.data.l[0] = pnt.x () * screen->width ();
	    xev.xclient.data.l[1] = pnt.y () * screen->height ();
	    xev.xclient.data.l[2] = 0;
	    xev.xclient.data.l[3] = 0;
	    xev.xclient.data.l[4] = 0;

	    XSendEvent (screen->dpy (), screen->root (), FALSE,
			SubstructureRedirectMask | SubstructureNotifyMask,
			&xev);
	}

	lastActiveNum  = w->activeNum ();
	selectedWindow = w->id ();

	if (!zoomedWindow)
	    zoomedWindow = selectedWindow;

	if (old != w->id ())
	{
	    if (toNext)
		move -= WIDTH;
	    else
		move += WIDTH;

	    moreAdjust = true;
	}

	if (popupWindow)
	{
	    CompWindow *popup;

	    popup = screen->findWindow (popupWindow);
	    if (popup)
		CompositeWindow::get (popup)->addDamage ();

	    setSelectedWindowHint ();
	}

	CompositeWindow::get (w)->addDamage ();

	if (old)
	{
	    w = screen->findWindow (old);
	    if (w)
		CompositeWindow::get (w)->addDamage ();
	}
    }
}

int
SwitchScreen::countWindows ()
{
    int count = 0;

    foreach (CompWindow *w, screen->windows ())
	if (SwitchWindow::get (w)->isSwitchWin ())
	{
	    count++;
	    if (count == 5)
		break;
	}

    if (count == 5 && screen->width () <= WINDOW_WIDTH (5))
	count = 3;

    return count;
}

static Visual *
findArgbVisual (Display *dpy, int scr)
{
    XVisualInfo		*xvi;
    XVisualInfo		temp;
    int			nvi;
    int			i;
    XRenderPictFormat	*format;
    Visual		*visual;

    temp.screen  = scr;
    temp.depth   = 32;
    temp.c_class = TrueColor;

    xvi = XGetVisualInfo (dpy,
			  VisualScreenMask |
			  VisualDepthMask  |
			  VisualClassMask,
			  &temp,
			  &nvi);
    if (!xvi)
	return 0;

    visual = 0;
    for (i = 0; i < nvi; i++)
    {
	format = XRenderFindVisualFormat (dpy, xvi[i].visual);
	if (format->type == PictTypeDirect && format->direct.alphaMask)
	{
	    visual = xvi[i].visual;
	    break;
	}
    }

    XFree (xvi);

    return visual;
}


void
SwitchScreen::initiate (SwitchWindowSelection selection,
			bool                  showPopup)
{
    int count;

    if (screen->otherGrabExist ("switcher", "scale", "cube", 0))
	return;

    this->selection      = selection;
    selectedWindow = None;

    count = countWindows ();
    if (count < 1)
	return;

    if (!popupWindow && showPopup)
    {
	Display		     *dpy = screen->dpy ();
	XSizeHints	     xsh;
	XWMHints	     xwmh;
	Atom		     state[4];
	int		     nState = 0;
	XSetWindowAttributes attr;
	Visual		     *visual;

	visual = findArgbVisual (dpy, screen->screenNum ());
	if (!visual)
	    return;

	if (count > 1)
	{
	    count -= (count + 1) & 1;
	    if (count < 3)
		count = 3;
	}

	xsh.flags       = PSize | PPosition | PWinGravity;
	xsh.width       = WINDOW_WIDTH (count);
	xsh.height      = WINDOW_HEIGHT;
	xsh.win_gravity = StaticGravity;

	xwmh.flags = InputHint;
	xwmh.input = 0;

	attr.background_pixel = 0;
	attr.border_pixel     = 0;
	attr.colormap	      = XCreateColormap (dpy, screen->root (), visual,
						 AllocNone);

	popupWindow =
	    XCreateWindow (dpy, screen->root (),
			   screen->width () / 2 - xsh.width / 2,
			   screen->height () / 2 - xsh.height / 2,
			   xsh.width, xsh.height, 0,
			   32, InputOutput, visual,
			   CWBackPixel | CWBorderPixel | CWColormap, &attr);

	XSetWMProperties (dpy, popupWindow, NULL, NULL,
			  programArgv, programArgc,
			  &xsh, &xwmh, NULL);

	state[nState++] = Atoms::winStateAbove;
	state[nState++] = Atoms::winStateSticky;
	state[nState++] = Atoms::winStateSkipTaskbar;
	state[nState++] = Atoms::winStateSkipPager;

	XChangeProperty (dpy, popupWindow,
			 Atoms::winState,
			 XA_ATOM, 32, PropModeReplace,
			 (unsigned char *) state, nState);

	XChangeProperty (dpy, popupWindow,
			 Atoms::winType,
			 XA_ATOM, 32, PropModeReplace,
			 (unsigned char *) &Atoms::winTypeUtil, 1);

	screen->setWindowProp (popupWindow, Atoms::winDesktop, 0xffffffff);

	setSelectedWindowHint ();
    }

    if (!grabIndex)
	grabIndex = screen->pushGrab (screen->invisibleCursor (), "switcher");

    if (grabIndex)
    {
	if (!switching)
	{
	    lastActiveNum = screen->activeNum ();

	    createWindowList (count);

	    sTranslate = zoom;

	    if (popupWindow && showPopup)
	    {
		CompWindow *w;

		w = screen->findWindow (popupWindow);
		if (w && (w->state () & CompWindowStateHiddenMask))
		    w->unminimize ();
		else
		    XMapWindow (screen->dpy (), popupWindow);

		setSelectedWindowHint ();
	    }

	    activateEvent (true);
	}

	cScreen->damageScreen ();

	switching  = true;
	moreAdjust = true;

	screen->handleEventSetEnabled (this, true);
	cScreen->preparePaintSetEnabled (this, true);
	cScreen->donePaintSetEnabled (this, true);
	gScreen->glPaintOutputSetEnabled (this, true);

	foreach (CompWindow *w, screen->windows ())
	{
	    SWITCH_WINDOW (w);

	    sw->gWindow->glPaintSetEnabled (sw, true);
	}
    }
}


static bool
switchTerminate (CompAction         *action,
	         CompAction::State  state,
	         CompOption::Vector &options)
{
    CompScreen *s;
    Window     xid;

    xid = CompOption::getIntOptionNamed (options, "root");

    if (action)
	action->setState (action->state () & ~(CompAction::StateTermKey |
					       CompAction::StateTermButton));

    if (xid && xid != screen->root ())
	return false;

    SWITCH_SCREEN (screen);


    if (ss->grabIndex)
    {
	CompWindow *w;

	if (ss->popupWindow)
	{
	    w = screen->findWindow (ss->popupWindow);
	    if (w && w->managed () && w->mapNum ())
	    {
		w->minimize ();
	    }
	    else
	    {
		XUnmapWindow (screen->dpy (), ss->popupWindow);
	    }
	}

	ss->switching = false;

	if (state & CompAction::StateCancel)
	{
	    ss->selectedWindow = None;
	    ss->zoomedWindow   = None;
	}

	if (state && ss->selectedWindow)
	{
	    w = screen->findWindow (ss->selectedWindow);
	    if (w)
		screen->sendWindowActivationRequest (w->id ());
	}

	screen->removeGrab (ss->grabIndex, 0);
	ss->grabIndex = NULL;

	if (!ss->popupWindow)
	    screen->handleEventSetEnabled (ss, false);

	if (!ss->zooming)
	{
	    ss->selectedWindow = None;
	    ss->zoomedWindow   = None;

	    ss->activateEvent (false);
	}
	else
	{
	    ss->moreAdjust = true;
	}

	ss->selectedWindow = None;
	ss->setSelectedWindowHint ();

	ss->lastActiveNum = 0;

	ss->cScreen->damageScreen ();
    }


    return false;
}

static bool
switchInitiateCommon (CompAction            *action,
		      CompAction::State     state,
		      CompOption::Vector    &options,
		      SwitchWindowSelection selection,
		      bool                  showPopup,
		      bool                  nextWindow)
{
    CompScreen *s;
    Window     xid;

    xid = CompOption::getIntOptionNamed (options, "root");

    if (xid != screen->root ())
	return false;

    SWITCH_SCREEN (screen);

    if (!ss->switching)
    {
	ss->initiate (selection, showPopup);

	if (state & CompAction::StateInitKey)
	    action->setState (action->state () | CompAction::StateTermKey);

	if (state & CompAction::StateInitEdge)
	    action->setState (action->state () | CompAction::StateTermEdge);
	else if (state & CompAction::StateInitButton)
	    action->setState (action->state () | CompAction::StateTermButton);
    }

    ss->switchToWindow (nextWindow);

    return false;
}

void
SwitchScreen::windowRemove (Window id)
{
    CompWindow *w;

    w = screen->findWindow (id);
    if (w)
    {
	bool   inList = false;
	int    count, j, i = 0;
	Window selected, old;

	SWITCH_WINDOW (w);

	if (!sw->isSwitchWin ())
	    return;

	sw->cWindow->damageRectSetEnabled (sw, false);
	sw->gWindow->glPaintSetEnabled (sw, false);

	old = selected = selectedWindow;

	CompWindowList::iterator it = windows.begin ();
	while (it != windows.end ())
	{
	    if (*it == w)
	    {
		inList = true;

		if (w->id () == selected)
		{
		    it++;
		    if (it == windows.end ())
			selected = windows.front ()->id ();
		    else
			selected = (*it)->id ();
		    it--;
		}

		CompWindowList::iterator del = it;
		it++;
		windows.erase (del);
	    }
	    else
		it++;
	}

	if (!inList)
	    return;

	count = windows.size ();

	if (windows.size () == 2)
	{
	    if (windows.front () == windows.back ())
	    {
		windows.pop_back ();
		count = 1;
	    }
	    else
	    {
		windows.push_back (windows.front ());
		windows.push_back ((*++windows.begin ()));
	    }
	}

	if (windows.size () == 0)
	{
	    CompOption::Vector o (0);

	    o.push_back (CompOption ("root", CompOption::TypeInt));

	    o[0].value ().set ((int) screen->root ());

	    switchTerminate (NULL, 0, o);
	    return;
	}

	if (!grabIndex)
	    return;

	updateWindowList (count);

	foreach (CompWindow *w, windows)
	{
	    selectedWindow = w->id ();

	    if (selectedWindow == selected)
		break;

	    pos -= WIDTH;
	    if (pos < -windows.size () * WIDTH)
		pos += windows.size () * WIDTH;
	}

	if (popupWindow)
	{
	    CompWindow *popup;

	    popup = screen->findWindow (popupWindow);
	    if (popup)
		CompositeWindow::get (popup)->addDamage ();

	    setSelectedWindowHint ();
	}

	if (old != selectedWindow)
	{
	    CompositeWindow::get (w)->addDamage ();

	    w = screen->findWindow (old);
	    if (w)
		CompositeWindow::get (w)->addDamage ();

	    moreAdjust = true;
	}
    }
}

void
SwitchScreen::updateForegroundColor ()
{
    Atom	  actual;
    int		  result, format;
    unsigned long n, left;
    unsigned char *propData;

    if (!popupWindow)
	return;


    result = XGetWindowProperty (screen->dpy (), popupWindow,
				 selectFgColorAtom, 0L, 4L, FALSE,
				 XA_INTEGER, &actual, &format,
				 &n, &left, &propData);

    if (result == Success && n && propData)
    {
	if (n == 3 || n == 4)
	{
	    long *data = (long *) propData;

	    fgColor[0] = MIN (0xffff, data[0]);
	    fgColor[1] = MIN (0xffff, data[1]);
	    fgColor[2] = MIN (0xffff, data[2]);

	    if (n == 4)
		fgColor[3] = MIN (0xffff, data[3]);
	}

	XFree (propData);
    }
    else
    {
	fgColor[0] = 0;
	fgColor[1] = 0;
	fgColor[2] = 0;
	fgColor[3] = 0xffff;
    }
}



void
SwitchScreen::handleEvent (XEvent *event)
{
    CompWindow *w;

    screen->handleEvent (event);

    switch (event->type) {
	case UnmapNotify:
	    windowRemove (event->xunmap.window);
	    break;
	case DestroyNotify:
	    windowRemove (event->xdestroywindow.window);
	    break;
	case PropertyNotify:
	    if (event->xproperty.atom == selectFgColorAtom)
	    {
		if (event->xproperty.window == popupWindow)
		    updateForegroundColor ();
	    }

	default:
	    break;
    }
}

bool
SwitchScreen::adjustVelocity ()
{
    float dx, adjust, amount;

    dx = move;

    adjust = dx * 0.15f;
    amount = fabs (dx) * 1.5f;
    if (amount < 0.2f)
	amount = 0.2f;
    else if (amount > 2.0f)
	amount = 2.0f;

    mVelocity = (amount * mVelocity + adjust) / (amount + 1.0f);

    if (zooming)
    {
	float dt, ds;

	if (switching)
	    dt = zoom - translate;
	else
	    dt = 0.0f - translate;

	adjust = dt * 0.15f;
	amount = fabs (dt) * 1.5f;
	if (amount < 0.2f)
	    amount = 0.2f;
	else if (amount > 2.0f)
	    amount = 2.0f;

	tVelocity = (amount * tVelocity + adjust) / (amount + 1.0f);

	if (selectedWindow == zoomedWindow)
	    ds = zoom - sTranslate;
	else
	    ds = 0.0f - sTranslate;

	adjust = ds * 0.5f;
	amount = fabs (ds) * 5.0f;
	if (amount < 1.0f)
	    amount = 1.0f;
	else if (amount > 6.0f)
	    amount = 6.0f;

	sVelocity = (amount * sVelocity + adjust) / (amount + 1.0f);

	if (selectedWindow == zoomedWindow)
	{
	    if (fabs (dx) < 0.1f   && fabs (mVelocity) < 0.2f   &&
		fabs (dt) < 0.001f && fabs (tVelocity) < 0.001f &&
		fabs (ds) < 0.001f && fabs (sVelocity) < 0.001f)
	    {
		mVelocity = tVelocity = sVelocity = 0.0f;
		return false;
	    }
	}
    }
    else
    {
	if (fabs (dx) < 0.1f  && fabs (mVelocity) < 0.2f)
	{
	    mVelocity = 0.0f;
	    return false;
	}
    }

    return true;
}

void
SwitchScreen::preparePaint (int msSinceLastPaint)
{
    if (moreAdjust)
    {
	int   steps, m;
	float amount, chunk;

	amount = msSinceLastPaint * 0.05f * optionGetSpeed ();
	steps  = amount / (0.5f * optionGetTimestep ());
	if (!steps) steps = 1;
	chunk  = amount / (float) steps;

	while (steps--)
	{
	    moreAdjust = adjustVelocity ();
	    if (!moreAdjust)
	    {
		pos += move;
		move = 0;

		if (zooming)
		{
		    if (switching)
		    {
			translate  = zoom;
			sTranslate = zoom;
		    }
		    else
		    {
			translate  = 0.0f;
			sTranslate = zoom;

			selectedWindow = None;
			zoomedWindow   = None;

			if (grabIndex)
			{
			    screen->removeGrab (grabIndex, 0);
			    grabIndex = 0;
			}

			activateEvent (false);
		    }
		}
		break;
	    }

	    m = mVelocity * chunk;
	    if (!m)
	    {
		if (mVelocity)
		    m = (move > 0) ? 1 : -1;
	    }

	    move -= m;
	    pos  += m;
	    if (pos < -windows.size () * WIDTH)
		pos += windows.size () * WIDTH;
	    else if (pos > 0)
		pos -= windows.size () * WIDTH;

	    translate  += tVelocity * chunk;
	    sTranslate += sVelocity * chunk;

	    if (selectedWindow != zoomedWindow)
	    {
		if (sTranslate < 0.01f)
		    zoomedWindow = selectedWindow;
	    }
	}
    }

    cScreen->preparePaint (msSinceLastPaint);
}

bool
SwitchScreen::glPaintOutput (const GLScreenPaintAttrib &sAttrib,
			     const GLMatrix            &transform,
			     const CompRegion          &region,
			     CompOutput                *output,
			     unsigned int              mask)
{
    bool status;

    zoomMask = ZOOMED_WINDOW_MASK | NORMAL_WINDOW_MASK;

    if (grabIndex || (zooming && translate > 0.001f))
    {
	GLMatrix   sTransform (transform);
	CompWindow *zoomed;
	CompWindow *switcher;
	Window     zoomedAbove = None;

	if (zooming)
	{
	    mask &= ~PAINT_SCREEN_REGION_MASK;
	    mask |= PAINT_SCREEN_TRANSFORMED_MASK | PAINT_SCREEN_CLEAR_MASK;

	    sTransform.translate (0.0f, 0.0f, -translate);

	    zoomMask = NORMAL_WINDOW_MASK;
	}

	if (optionGetBringToFront ())
	{
	    zoomed = screen->findWindow (zoomedWindow);
	    if (zoomed)
	    {
		CompWindow *w;

		for (w = zoomed->prev; w && w->id () <= 1; w = w->prev);
		zoomedAbove = (w) ? w->id () : None;

		screen->unhookWindow (zoomed);
		screen->insertWindow (zoomed,
				      screen->windows ().back ()->id ());
	    }
	}
	else
	{
	    zoomed = NULL;
	}

	ignoreSwitcher = true;

	status = gScreen->glPaintOutput (sAttrib, sTransform, region, output,
					 mask);

	if (zooming)
	{
	    float zTranslate;

	    mask &= ~PAINT_SCREEN_CLEAR_MASK;
	    mask |= PAINT_SCREEN_NO_BACKGROUND_MASK;

	    zoomMask = ZOOMED_WINDOW_MASK;

	    zTranslate = MIN (sTranslate, translate);
	    sTransform.translate (0.0f, 0.0f, zTranslate);

	    status = gScreen->glPaintOutput (sAttrib, sTransform, region,
					     output, mask);
	}

	if (zoomed)
	{
	    screen->unhookWindow (zoomed);
	    screen->insertWindow (zoomed, zoomedAbove);
	}

	ignoreSwitcher = false;

	switcher = screen->findWindow (popupWindow);

	if (switcher)
	{
	    SWITCH_WINDOW (switcher);

	    sTransform = transform;

	    sTransform.toScreenSpace (output, -DEFAULT_Z_CAMERA);

	    glPushMatrix ();
	    glLoadMatrixf (sTransform.getMatrix ());

	    if (!switcher->destroyed () &&
		switcher->isViewable () &&
		sw->cWindow->damaged ())
	    {
		sw->gWindow->glPaint (sw->gWindow->paintAttrib (),
				      sTransform, infiniteRegion, 0);
	    }

	    glPopMatrix ();
	}
    }
    else
    {
	status = gScreen->glPaintOutput (sAttrib, transform, region, output,
					 mask);
    }

    return status;
}

void
SwitchScreen::donePaint ()
{
    if ((grabIndex || zooming) && moreAdjust)
    {
	if (zooming)
	{
	    cScreen->damageScreen ();
	}
	else
	{
	    CompWindow *w;

	    w = screen->findWindow (popupWindow);
	    if (w)
		CompositeWindow::get (w)->addDamage ();
	}
    }
    else if (!grabIndex && !(zooming && translate > 0.001f) && !moreAdjust)
    {
	cScreen->preparePaintSetEnabled (this, false);
	cScreen->donePaintSetEnabled (this, false);
	gScreen->glPaintOutputSetEnabled (this, false);

	foreach (CompWindow *w, screen->windows ())
	{
	    SWITCH_WINDOW (w);
	    sw->cWindow->damageRectSetEnabled (sw, false);
	    sw->gWindow->glPaintSetEnabled (sw, false);
	}
    }

    cScreen->donePaint ();
}


void
SwitchWindow::paintThumb (const GLWindowPaintAttrib &attrib,
			  const GLMatrix            &transform,
		          unsigned int              mask,
			  int                       x,
			  int                       y,
			  int                       x1,
			  int                       x2)
{
    GLWindowPaintAttrib  sAttrib (attrib);
    int                  wx, wy;
    float                width, height;
    GLTexture            *icon = NULL;
    CompWindow::Geometry &g = window->geometry ();

    mask |= PAINT_WINDOW_TRANSFORMED_MASK;

    if (window->mapNum ())
    {
	if (gWindow->textures ().empty ())
	    gWindow->bind ();
    }

    if (!gWindow->textures ().empty ())
    {
	GLMatrix wTransform (transform);
	int      ww, wh;
	int      addWindowGeometryIndex =
	    gWindow->glAddGeometryGetCurrentIndex ();

	width  = WIDTH  - (SPACE << 1);
	height = HEIGHT - (SPACE << 1);

	ww = g.width () + window->input ().left +
	     window->input ().right;
	wh = g.height () + window->input ().top +
	     window->input ().bottom;

	if (ww > width)
	    sAttrib.xScale = width / ww;
	else
	    sAttrib.xScale = 1.0f;

	if (wh > height)
	    sAttrib.yScale = height / wh;
	else
	    sAttrib.yScale = 1.0f;

	if (sAttrib.xScale < sAttrib.yScale)
	    sAttrib.yScale = sAttrib.xScale;
	else
	    sAttrib.xScale = sAttrib.yScale;

	width  = ww * sAttrib.xScale;
	height = wh * sAttrib.yScale;

	wx = x + SPACE + ((WIDTH  - (SPACE << 1)) - width)  / 2;
	wy = y + SPACE + ((HEIGHT - (SPACE << 1)) - height) / 2;

	sAttrib.xTranslate = wx - g.x () +
			     window->input ().left * sAttrib.xScale;
	sAttrib.yTranslate = wy - g.y () +
			     window->input ().top  * sAttrib.yScale;

	GLFragment::Attrib fragment (sAttrib);

	if (window->alpha () || fragment.getOpacity () != OPAQUE)
	    mask |= PAINT_WINDOW_TRANSLUCENT_MASK;

	wTransform.translate (g.x (), g.y (), 0.0f);
	wTransform.scale (sAttrib.xScale, sAttrib.yScale, 1.0f);
	wTransform.translate (sAttrib.xTranslate / sAttrib.xScale - g.x (),
			      sAttrib.yTranslate / sAttrib.yScale - g.y (),
			      0.0f);

	glPushMatrix ();
	glLoadMatrixf (wTransform.getMatrix ());

	/* XXX: replacing the addWindowGeometry function like this is
	   very ugly but necessary until the vertex stage has been made
	   fully pluggable. */
	gWindow->glAddGeometrySetCurrentIndex (MAXSHORT);
	gWindow->glDraw (wTransform, fragment, infiniteRegion, mask);
	gWindow->glAddGeometrySetCurrentIndex (addWindowGeometryIndex);

	glPopMatrix ();

	if (sScreen->optionGetIcon ())
	{
	    icon = gWindow->getIcon (ICON_SIZE, ICON_SIZE);
	    if (icon)
	    {
		sAttrib.xScale = sAttrib.yScale = 1.0f;

		wx = x + WIDTH  - icon->width ()  - SPACE;
		wy = y + HEIGHT - icon->height () - SPACE;
	    }
	}
    }
    else
    {
	width  = WIDTH  - (WIDTH  >> 2);
	height = HEIGHT - (HEIGHT >> 2);

	icon = gWindow->getIcon (ICON_SIZE, ICON_SIZE);
	if (!icon)
	    icon = gScreen->defaultIcon ();

	if (icon)
	{
	    int iw, ih;

	    iw = width  - SPACE;
	    ih = height - SPACE;

	    if (icon->width () < (iw >> 1))
		sAttrib.xScale = (iw / icon->width ());
	    else
		sAttrib.xScale = 1.0f;

	    if (icon->height () < (ih >> 1))
		sAttrib.yScale = (ih / icon->height ());
	    else
		sAttrib.yScale = 1.0f;

	    if (sAttrib.xScale < sAttrib.yScale)
		sAttrib.yScale = sAttrib.xScale;
	    else
		sAttrib.xScale = sAttrib.yScale;

	    width  = icon->width ()  * sAttrib.xScale;
	    height = icon->height () * sAttrib.yScale;

	    wx = x + SPACE + ((WIDTH  - (SPACE << 1)) - width)  / 2;
	    wy = y + SPACE + ((HEIGHT - (SPACE << 1)) - height) / 2;
	}
    }

    if (icon)
    {
	CompRegion        iconReg (g.x (), g.y (), icon->width (),
				   icon->height ());
	GLTexture::MatrixList matrix (1);
	int               addWindowGeometryIndex =
	    gWindow->glAddGeometryGetCurrentIndex ();

	mask |= PAINT_WINDOW_BLEND_MASK;

	matrix[0] = icon->matrix ();
	matrix[0].x0 -= (g.x () * matrix[0].xx);
	matrix[0].y0 -= (g.y () * matrix[0].yy);

	sAttrib.xTranslate = wx - g.x ();
	sAttrib.yTranslate = wy - g.y ();

	gWindow->geometry ().reset ();

	gWindow->glAddGeometrySetCurrentIndex (MAXSHORT);
	gWindow->glAddGeometry (matrix, iconReg, infiniteRegion);
	gWindow->glAddGeometrySetCurrentIndex (addWindowGeometryIndex);

	if (gWindow->geometry ().vCount)
	{
	    GLFragment::Attrib fragment (sAttrib);
	    GLMatrix           wTransform (transform);

	    wTransform.translate (g.x (), g.y (), 0.0f);
	    wTransform.scale (sAttrib.xScale, sAttrib.yScale, 1.0f);
	    wTransform.translate (sAttrib.xTranslate / sAttrib.xScale - g.x (),
				  sAttrib.yTranslate / sAttrib.yScale - g.y (),
				  0.0f);

	    glPushMatrix ();
	    glLoadMatrixf (wTransform.getMatrix ());

	    gWindow->glDrawTexture (icon, fragment, mask);

	    glPopMatrix ();
	}
    }
}

bool
SwitchWindow::glPaint (const GLWindowPaintAttrib &attrib,
		       const GLMatrix            &transform,
		       const CompRegion          &region,
		       unsigned int              mask)
{
    int	       zoomType = NORMAL_WINDOW_MASK;
    bool       status;

    if (window->id () == sScreen->popupWindow)
    {
	GLenum         filter;
	int            x, y, x1, x2, cx, i;
	unsigned short color[4];

	CompWindow::Geometry &g = window->geometry ();

	if (mask & PAINT_WINDOW_OCCLUSION_DETECTION_MASK ||
	    sScreen->ignoreSwitcher)
	    return false;

	status = gWindow->glPaint (attrib, transform, region, mask);

	if (!(mask & PAINT_WINDOW_TRANSFORMED_MASK) && region.isEmpty ())
	    return true;

	x1 = g.x () + SPACE;
	x2 = g.x () + g.width () - SPACE;

	x = x1 + sScreen->pos;
	y = g.y () + SPACE;

	filter = gScreen->textureFilter ();

	if (sScreen->optionGetMipmap ())
	    gScreen->setTextureFilter (GL_LINEAR_MIPMAP_LINEAR);

	glPushAttrib (GL_SCISSOR_BIT);

	glEnable (GL_SCISSOR_TEST);
	glScissor (x1, 0, x2 - x1, screen->height ());

	foreach (CompWindow *w, sScreen->windows)
	{
	    if (x + WIDTH > x1)
		SwitchWindow::get (w)->paintThumb (
		    gWindow->lastPaintAttrib (), transform,
		    mask, x, y, x1, x2);
	    x += WIDTH;
	}

	foreach (CompWindow *w, sScreen->windows)
	{
	    if (x > x2)
		break;

            SwitchWindow::get (w)->paintThumb (
		gWindow->lastPaintAttrib (), transform,
		mask, x, y, x1, x2);
	    x += WIDTH;
	}

	glPopAttrib ();

	gScreen->setTextureFilter (filter);

	cx = g.x () + (g.width () >> 1);

	glDisableClientState (GL_TEXTURE_COORD_ARRAY);
	glEnable (GL_BLEND);
	for (i = 0; i < 4; i++)
	    color[i] = (unsigned int)sScreen->fgColor[i] *
		       gWindow->lastPaintAttrib ().opacity /
		       0xffff;
	glColor4usv (color);
	glPushMatrix ();
	glTranslatef (cx, y, 0.0f);
	glVertexPointer (2, GL_FLOAT, 0, _boxVertices);
	glDrawArrays (GL_QUADS, 0, 16);
	glPopMatrix ();
	glColor4usv (defaultColor);
	glDisable (GL_BLEND);
	glEnableClientState (GL_TEXTURE_COORD_ARRAY);
    }
    else if (window->id () == sScreen->selectedWindow)
    {
	if (sScreen->optionGetBringToFront () &&
	    sScreen->selectedWindow == sScreen->zoomedWindow)
	    zoomType = ZOOMED_WINDOW_MASK;

	if (!(sScreen->zoomMask & zoomType))
	    return (mask & PAINT_WINDOW_OCCLUSION_DETECTION_MASK) ?
		false : true;

	status = gWindow->glPaint (attrib, transform, region, mask);
    }
    else if (sScreen->switching)
    {
	GLWindowPaintAttrib sAttrib (attrib);
	GLuint              value;

	value = sScreen->optionGetSaturation ();
	if (value != 100)
	    sAttrib.saturation = sAttrib.saturation * value / 100;

	value = sScreen->optionGetBrightness ();
	if (value != 100)
	    sAttrib.brightness = sAttrib.brightness * value / 100;

	if (window->wmType () & ~(CompWindowTypeDockMask |
			          CompWindowTypeDesktopMask))
	{
	    value = sScreen->optionGetOpacity ();
	    if (value != 100)
		sAttrib.opacity = sAttrib.opacity * value / 100;
	}

	if (sScreen->optionGetBringToFront () &&
	    window->id () == sScreen->zoomedWindow)
	    zoomType = ZOOMED_WINDOW_MASK;

	if (!(sScreen->zoomMask & zoomType))
	    return (mask & PAINT_WINDOW_OCCLUSION_DETECTION_MASK) ?
		false : true;

	status = gWindow->glPaint (sAttrib, transform, region, mask);
    }
    else
    {
	if (!(sScreen->zoomMask & zoomType))
	    return (mask & PAINT_WINDOW_OCCLUSION_DETECTION_MASK) ?
		false : true;

	status = gWindow->glPaint (attrib, transform, region, mask);
    }

    return status;
}

bool
SwitchWindow::damageRect (bool initial, const CompRect &rect)
{
    bool status;

    if (sScreen->grabIndex)
    {
	CompWindow *popup;
	int	   i;

	foreach (CompWindow *w, sScreen->windows)
	{
	    if (window == w)
	    {
		popup = screen->findWindow (sScreen->popupWindow);
		if (popup)
		    CompositeWindow::get (popup)->addDamage ();

		break;
	    }
	}
    }

    cWindow->damageRect (initial, rect);

    return status;
}

void
SwitchScreen::setZoom ()
{
    if (optionGetZoom () < 0.05f)
    {
	zooming = false;
	zoom    = 0.0f;
    }
    else
    {
	zooming = true;
	zoom    = optionGetZoom () / 30.0f;
    }

}

SwitchScreen::SwitchScreen (CompScreen *screen) :
    PluginClassHandler<SwitchScreen,CompScreen> (screen),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen)),
    popupWindow (None),
    selectedWindow (None),
    zoomedWindow (None),
    lastActiveNum (0),
    grabIndex (NULL),
    switching (false),
    zoomMask (~0),
    moreAdjust (false),
    mVelocity (0.0),
    tVelocity (0.0),
    sVelocity (0.0),
    windows (),
    pos (0),
    move (0),
    translate (0.0),
    sTranslate (0.0),
    selection (CurrentViewport),
    ignoreSwitcher (false)
{
    zoom = optionGetZoom () / 30.0f;

    zooming = (optionGetZoom () > 0.05f);

    fgColor[0] = 0;
    fgColor[1] = 0;
    fgColor[2] = 0;
    fgColor[3] = 0xffff;

    selectWinAtom =
	XInternAtom (screen->dpy (), DECOR_SWITCH_WINDOW_ATOM_NAME, 0);
    selectFgColorAtom =
	XInternAtom (screen->dpy (), DECOR_SWITCH_FOREGROUND_COLOR_ATOM_NAME, 0);

    optionSetZoomNotify (boost::bind (&SwitchScreen::setZoom, this));

#define SWITCHBIND(a,b,c) boost::bind (switchInitiateCommon, _1, _2, _3, a, b, c)

    optionSetNextButtonInitiate (SWITCHBIND (CurrentViewport, true, true));
    optionSetNextButtonTerminate (switchTerminate);
    optionSetNextKeyInitiate (SWITCHBIND (CurrentViewport, true, true));
    optionSetNextKeyTerminate (switchTerminate);
    optionSetPrevButtonInitiate (SWITCHBIND (CurrentViewport, true, false));
    optionSetPrevButtonTerminate (switchTerminate);
    optionSetPrevKeyInitiate (SWITCHBIND (CurrentViewport, true, false));
    optionSetPrevKeyTerminate (switchTerminate);

    optionSetNextAllButtonInitiate (SWITCHBIND (AllViewports, true, true));
    optionSetNextAllButtonTerminate (switchTerminate);
    optionSetNextAllKeyInitiate (SWITCHBIND (AllViewports, true, true));
    optionSetNextAllKeyTerminate (switchTerminate);
    optionSetPrevAllButtonInitiate (SWITCHBIND (AllViewports, true, false));
    optionSetPrevAllButtonTerminate (switchTerminate);
    optionSetPrevAllKeyInitiate (SWITCHBIND (AllViewports, true, false));
    optionSetPrevAllKeyTerminate (switchTerminate);

    optionSetNextNoPopupButtonInitiate (SWITCHBIND (CurrentViewport, false, true));
    optionSetNextNoPopupButtonTerminate (switchTerminate);
    optionSetNextNoPopupKeyInitiate (SWITCHBIND (CurrentViewport, false, true));
    optionSetNextNoPopupKeyTerminate (switchTerminate);
    optionSetPrevNoPopupButtonInitiate (SWITCHBIND (CurrentViewport, false, false));
    optionSetPrevNoPopupButtonTerminate (switchTerminate);
    optionSetPrevNoPopupKeyInitiate (SWITCHBIND (CurrentViewport, false, false));
    optionSetPrevNoPopupKeyTerminate (switchTerminate);

    optionSetNextPanelButtonInitiate (SWITCHBIND (Panels, false, true));
    optionSetNextPanelButtonTerminate (switchTerminate);
    optionSetNextPanelKeyInitiate (SWITCHBIND (Panels, false, true));
    optionSetNextPanelKeyTerminate (switchTerminate);
    optionSetPrevPanelButtonInitiate (SWITCHBIND (Panels, false, false));
    optionSetPrevPanelButtonTerminate (switchTerminate);
    optionSetPrevPanelKeyInitiate (SWITCHBIND (Panels, false, false));
    optionSetPrevPanelKeyTerminate (switchTerminate);

#undef SWITCHBIND

    ScreenInterface::setHandler (screen, false);
    CompositeScreenInterface::setHandler (cScreen, false);
    GLScreenInterface::setHandler (gScreen, false);
}

SwitchScreen::~SwitchScreen ()
{
    if (popupWindow)
	XDestroyWindow (screen->dpy (), popupWindow);
}

bool
SwitchPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) |
        !CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) |
        !CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
	 return false;

    return true;
}

