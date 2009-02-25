/*
 * Copyright (C) 2001 Havoc Pennington
 * Copyright (C) 2002, 2003 Red Hat, Inc.
 * Copyright (C) 2003 Rob Adams
 * Copyright (C) 2005 Novell, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "place.h"

COMPIZ_PLUGIN_20081216 (place, PlacePluginVTable)

static const CompMetadata::OptionInfo placeOptionInfo[] = {
    { "workarounds", "bool", 0, 0, 0 },
    { "mode", "int", RESTOSTRING (0, PLACE_MODE_LAST), 0, 0 },
    { "multioutput_mode", "int", RESTOSTRING (0, PLACE_MOMODE_LAST), 0, 0 },
    { "force_placement_match", "match", 0, 0, 0 },
    { "position_matches", "list", "<type>match</type>", 0, 0 },
    { "position_x_values", "list", "<type>int</type>", 0, 0 },
    { "position_y_values", "list", "<type>int</type>", 0, 0 },
    { "viewport_matches", "list", "<type>match</type>", 0, 0 },
    { "viewport_x_values", "list",
	"<type>int</type><min>1</min><max>32</max>", 0, 0 },
    { "viewport_y_values", "list",
	"<type>int</type><min>1</min><max>32</max>", 0, 0 },
};

PlaceScreen::PlaceScreen (CompScreen *screen) :
    PrivateHandler<PlaceScreen, CompScreen> (screen),
    opt (PLACE_OPTION_NUM)
{
    if (!placeVTable->getMetadata ()->initOptions (placeOptionInfo,
						   PLACE_OPTION_NUM, opt))
    {
	setFailed ();
	return;
    }

    ScreenInterface::setHandler (screen);
}

PlaceScreen::~PlaceScreen ()
{
}

CompOption::Vector &
PlaceScreen::getOptions ()
{
    return opt;
}

bool
PlaceScreen::setOption (const char        *name,
			CompOption::Value &value)
{
    CompOption   *o;
    unsigned int index;

    o = CompOption::findOption (opt, name, &index);
    if (!o)
	return false;

    if (!CompOption::setOption (*o, value))
	return false;

    if (index == PLACE_OPTION_POSITION_MATCHES ||
	index == PLACE_OPTION_VIEWPORT_MATCHES)
    {
	foreach (CompOption::Value &ov, o->value ().list ())
	    ov.match ().update ();
    }

    return true;
}

/* sort functions */

static bool
compareLeftmost (CompWindow *a,
		 CompWindow *b)
{
    int ax, bx;

    ax = a->serverX () - a->input ().left;
    bx = b->serverX () - b->input ().left;

    return (ax <= bx);
}

static bool
compareTopmost (CompWindow *a,
		CompWindow *b)
{
    int ay, by;

    ay = a->serverY () - a->input ().top;
    by = b->serverY () - b->input ().top;

    return (ay <= by);
}

static bool
compareNorthWestCorner (CompWindow *a,
			CompWindow *b)
{
    int fromOriginA;
    int fromOriginB;
    int ax, ay, bx, by;

    ax = a->serverX () - a->input ().left;
    ay = a->serverY () - a->input ().top;

    bx = b->serverX () - b->input ().left;
    by = b->serverY () - b->input ().top;

    /* probably there's a fast good-enough-guess we could use here. */
    fromOriginA = sqrt (ax * ax + ay * ay);
    fromOriginB = sqrt (bx * bx + by * by);

    return (fromOriginA <= fromOriginB);
}

PlaceWindow::PlaceWindow (CompWindow *w) :
    PrivateHandler<PlaceWindow, CompWindow> (w),
    window (w)
{
    WindowInterface::setHandler (w);
}

PlaceWindow::~PlaceWindow ()
{
}

bool
PlaceWindow::place (CompPoint &pos)
{
    Bool      status = window->place (pos);
    CompPoint viewport;

    if (status)
	return status;

    doPlacement (pos);
    if (matchViewport (viewport))
    {
	int x, y;

	viewport.setX (MAX (MIN (viewport.x (),
				 screen->vpSize ().width ()), 0));
	viewport.setY (MAX (MIN (viewport.y (),
				 screen->vpSize ().height ()), 0));

	x = pos.x () % screen->width ();
	if (x < 0)
	    x += screen->width ();
	y = pos.y () % screen->height ();
	if (y < 0)
	    y += screen->height ();

	pos.setX (x + (viewport.x () - screen->vp ().x ()) * screen->width ());
	pos.setY (y + (viewport.y () - screen->vp ().y ()) * screen->height ());
    }

    return true;
}

void
PlaceWindow::validateResizeRequest (unsigned int   &mask,
				    XWindowChanges *xwc,
				    unsigned int   source)
{
    XRectangle           workArea;
    int                  x, y, left, right, top, bottom;
    int                  output;
    CompWindow::Geometry geom;
    bool                 sizeOnly = false;

    window->validateResizeRequest (mask, xwc, source);

    if (window->state () & CompWindowStateFullscreenMask)
	return;

    if (window->wmType () & (CompWindowTypeDockMask |
			     CompWindowTypeDesktopMask))
	return;

    if (window->sizeHints ().flags & USPosition)
    {
	/* only respect USPosition on normal windows if
	   workarounds are disabled, reason see above */
	PLACE_SCREEN (screen);

	if (ps->opt[PLACE_OPTION_WORKAROUND].value ().b () ||
	    (window->type () & CompWindowTypeNormalMask))
	{
	    /* try to keep the window position intact for USPosition -
	       obviously we can't do that if we need to change the size */
	    sizeOnly = true;
	}
    }

    /* left, right, top, bottom target coordinates, clamped to viewport
       sizes as we don't need to validate movements to other viewports;
       we are only interested in inner-viewport movements */
    x = xwc->x % screen->width ();
    if (x < 0)
	x += screen->width ();

    y = xwc->y % screen->height ();
    if (y < 0)
	y += screen->height ();

    left   = x - window->input ().left;
    right  = x + xwc->width + window->input ().right;
    top    = y - window->input ().top;
    bottom = y + xwc->height + window->input ().bottom;

    geom.set (xwc->x, xwc->y, xwc->width, xwc->height,
	      window->serverGeometry ().border ());
    output = screen->outputDeviceForGeometry (geom);
    screen->getWorkareaForOutput (output, &workArea);

    if (xwc->width >= workArea.width &&
	xwc->height >= workArea.height)
    {
	sendMaximizationRequest ();
    }

    if ((right - left) > workArea.width)
    {
	left  = workArea.x;
	right = left + workArea.width;
    }
    else
    {
	if (left < workArea.x)
	{
	    right += workArea.x - left;
	    left  = workArea.x;
	}

	if (right > (workArea.x + workArea.width))
	{
	    left -= right - (workArea.x + workArea.width);
	    right = workArea.x + workArea.width;
	}
    }

    if ((bottom - top) > workArea.height)
    {
	top    = workArea.y;
	bottom = top + workArea.height;
    }
    else
    {
	if (top < workArea.y)
	{
	    bottom += workArea.y - top;
	    top    = workArea.y;
	}

	if (bottom > (workArea.y + workArea.height))
	{
	    top   -= bottom - (workArea.y + workArea.height);
	    bottom = workArea.y + workArea.height;
	}
    }

    /* bring left/right/top/bottom to actual window coordinates */
    left   += window->input ().left;
    right  -= window->input ().right;
    top    += window->input ().top;
    bottom -= window->input ().bottom;

    if ((right - left) != xwc->width)
    {
	xwc->width = right - left;
	mask       |= CWWidth;
	sizeOnly   = false;
    }

    if ((bottom - top) != xwc->height)
    {
	xwc->height = bottom - top;
	mask        |= CWHeight;
	sizeOnly    = false;
    }

    if (!sizeOnly)
    {
	if (left != x)
	{
	    xwc->x += left - x;
	    mask   |= CWX;
	}

	if (top != y)
	{
	    xwc->y += top - y;
	    mask   |= CWY;
	}
    }
}

void
PlaceWindow::doPlacement (CompPoint &pos)
{
    XRectangle        workArea;
    CompPoint         targetVp;
    PlacementStrategy strategy;

    PLACE_SCREEN (screen);

    strategy = getStrategy ();
    if (strategy == NoPlacement)
	return;

    if (matchPosition (pos))
    {
	/* FIXME: perhaps ConstrainOnly? */
	strategy = NoPlacement;
    }

    const CompOutput &output = getPlacementOutput (strategy, pos);
    workArea = output.workArea ();

    targetVp = window->initialViewport ();

    if (strategy == PlaceOverParent)
    {
	CompWindow *parent;

	parent = screen->findWindow (window->transientFor ());
	if (parent)
	{
	    /* center over parent horizontally */
	    pos.setX (parent->serverX () +
		      (parent->serverGeometry ().width () / 2) -
		      (window->serverGeometry ().width () / 2));

	    /* "visually" center vertically, leaving twice as much space below
	       as on top */
	    pos.setY (parent->serverY () +
		      (parent->serverGeometry ().height () -
		       window->serverGeometry ().height ()) / 3);

	    /* if parent is visible on current viewport, clip to work area;
	       don't constrain further otherwise */
	    if (parent->serverX () < screen->width ()           &&
		parent->serverX () + parent->serverWidth () > 0 &&
		parent->serverY () < screen->height ()          &&
		parent->serverY () + parent->serverHeight () > 0)
	    {
		targetVp = parent->defaultViewport ();
		strategy = ConstrainOnly;
	    }
	    else
	    {
		strategy = NoPlacement;
	    }
	}
    }

    if (strategy == PlaceCenteredOnScreen)
    {
	/* center window on current output device */

	pos.setX (output.x () +
		  (output.width () - window->serverGeometry ().width ()) /2);
	pos.setY (output.y () +
		  (output.height () - window->serverGeometry ().height ()) / 2);

	strategy = ConstrainOnly;
    }

    workArea.x += (targetVp.x () - screen->vp ().x ()) * screen->width ();
    workArea.y += (targetVp.y () - screen->vp ().y ()) * screen->height ();

    if (strategy == PlaceOnly || strategy == PlaceAndConstrain)
    {
	switch (ps->opt[PLACE_OPTION_MODE].value ().i ()) {
	case PLACE_MODE_CASCADE:
	    placeCascade (workArea, pos);
	    break;
	case PLACE_MODE_CENTERED:
	    placeCentered (workArea, pos);
	    break;
	case PLACE_MODE_RANDOM:
	    placeRandom (workArea, pos);
	    break;
	case PLACE_MODE_MAXIMIZE:
	    sendMaximizationRequest ();
	    break;
	case PLACE_MODE_SMART:
	    placeSmart (workArea, pos);
	    break;
	}

	/* When placing to the fullscreen output, constrain to one
	   output nevertheless */
	if (output.id () == ~0)
	{
	    int                  id;
	    CompWindow::Geometry geom (window->serverGeometry ());

	    geom.setX (pos.x ());
	    geom.setY (pos.y ());

	    id = screen->outputDeviceForGeometry (geom);
	    screen->getWorkareaForOutput (id, &workArea);

	    workArea.x += (targetVp.x () - screen->vp ().x ()) *
			  screen->width ();
	    workArea.y += (targetVp.y () - screen->vp ().y ()) *
			  screen->height ();
	}

	/* Maximize windows if they are too big for their work area (bit of
	 * a hack here). Assume undecorated windows probably don't intend to
	 * be maximized.
	 */
	if ((window->actions () & MAXIMIZE_STATE) == MAXIMIZE_STATE &&
	    (window->mwmDecor () & (MwmDecorAll | MwmDecorTitle))   &&
	    !(window->state () & CompWindowStateFullscreenMask))
	{
	    if (window->serverWidth () >= workArea.width &&
		window->serverHeight () >= workArea.height)
	    {
		sendMaximizationRequest ();
	    }
	}
    }

    if (strategy == ConstrainOnly || strategy == PlaceAndConstrain)
	constrainToWorkarea (workArea, pos);
}

void
PlaceWindow::placeCascade (XRectangle &workArea,
			   CompPoint  &pos)
{
    CompWindowList windows;

    /* Find windows that matter (not minimized, on same workspace
     * as placed window, may be shaded - if shaded we pretend it isn't
     * for placement purposes)
     */
    foreach (CompWindow *w, screen->windows ())
    {
	if (!windowIsPlaceRelevant (w))
	    continue;

	if (w->type () & (CompWindowTypeFullscreenMask |
			  CompWindowTypeUnknownMask))
	    continue;

	if (w->serverX () >= workArea.x + workArea.width                 ||
	    w->serverX () + w->serverGeometry ().width () <= workArea.x  ||
	    w->serverY () >= workArea.y + workArea.height                ||
	    w->serverY () + w->serverGeometry ().height () <= workArea.y)
	    continue;

	windows.push_back (w);
    }

    if (!cascadeFindFirstFit (windows, workArea, pos))
    {
	/* if the window wasn't placed at the origin of screen,
	 * cascade it onto the current screen
	 */
	cascadeFindNext (windows, workArea, pos);
    }
}

void
PlaceWindow::placeCentered (XRectangle &workArea,
			    CompPoint  &pos)
{
    pos.setX (workArea.x +
	      (workArea.width - window->serverGeometry ().width ()) / 2);
    pos.setY (workArea.y +
	      (workArea.height - window->serverGeometry ().height ()) / 2);
}

void
PlaceWindow::placeRandom (XRectangle &workArea,
			  CompPoint  &pos)
{
    int remainX, remainY;

    pos.setX (workArea.x);
    pos.setY (workArea.y);

    remainX = workArea.width - window->serverGeometry ().width ();
    if (remainX > 0)
	pos.setX (pos.x () + (rand () % remainX));

    remainY = workArea.height - window->serverGeometry ().height ();
    if (remainY > 0)
	pos.setY (pos.y () + (rand () % remainY));
}

/* overlap types */
#define NONE    0
#define H_WRONG -1
#define W_WRONG -2

void
PlaceWindow::placeSmart (XRectangle &workArea,
			 CompPoint  &pos)
{
    /*
     * SmartPlacement by Cristian Tibirna (tibirna@kde.org)
     * adapted for kwm (16-19jan98) and for kwin (16Nov1999) using (with
     * permission) ideas from fvwm, authored by
     * Anthony Martin (amartin@engr.csulb.edu).
     * Xinerama supported added by Balaji Ramani (balaji@yablibli.com)
     * with ideas from xfce.
     * adapted for Compiz by Bellegarde Cedric (gnumdk(at)gmail.com)
     */
    int overlap, minOverlap = 0;
    int xOptimal, yOptimal;
    int possible;

    /* temp coords */
    int cxl, cxr, cyt, cyb;
    /* temp coords */
    int xl,  xr,  yt,  yb;
    /* temp holder */
    int basket;
    /* CT lame flag. Don't like it. What else would do? */
    Bool firstPass = TRUE;

    /* get the maximum allowed windows space */
    int xTmp = workArea.x;
    int yTmp = workArea.y;

    /* client gabarit */
    int cw = window->serverWidth () - 1;
    int ch = window->serverHeight () - 1;

    xOptimal = xTmp;
    yOptimal = yTmp;

    /* loop over possible positions */
    do
    {
	/* test if enough room in x and y directions */
	if (yTmp + ch > workArea.y + workArea.height && ch < workArea.height)
	    overlap = H_WRONG; /* this throws the algorithm to an exit */
	else if (xTmp + cw > workArea.x + workArea.width)
	    overlap = W_WRONG;
	else
	{
	    overlap = NONE; /* initialize */

	    cxl = xTmp;
	    cxr = xTmp + cw;
	    cyt = yTmp;
	    cyb = yTmp + ch;

	    foreach (CompWindow *w, screen->windows ())
	    {
		if (!windowIsPlaceRelevant (w))
		    continue;

		xl = w->serverX () - w->input ().left;
		yt = w->serverY () - w->input ().top;
		xr = xl + w->serverWidth ();
		yb = yt + w->serverHeight ();

		/* if windows overlap, calc the overall overlapping */
		if (cxl < xr && cxr > xl && cyt < yb && cyb > yt)
		{
		    xl = MAX (cxl, xl);
		    xr = MIN (cxr, xr);
		    yt = MAX (cyt, yt);
		    yb = MIN (cyb, yb);

		    if (w->state () & CompWindowStateAboveMask)
			overlap += 16 * (xr - xl) * (yb - yt);
		    else if (w->state () & CompWindowStateBelowMask)
			overlap += 0;
		    else
			overlap += (xr - xl) * (yb - yt);
		}
	    }
	}

	/* CT first time we get no overlap we stop */
	if (overlap == NONE)
	{
	    xOptimal = xTmp;
	    yOptimal = yTmp;
	    break;
	}

	if (firstPass)
	{
	    firstPass  = FALSE;
	    minOverlap = overlap;
	}
	/* CT save the best position and the minimum overlap up to now */
	else if (overlap >= NONE && overlap < minOverlap)
	{
	    minOverlap = overlap;
	    xOptimal = xTmp;
	    yOptimal = yTmp;
	}

	/* really need to loop? test if there's any overlap */
	if (overlap > NONE)
	{
	    possible = workArea.x + workArea.width;

	    if (possible - cw > xTmp)
		possible -= cw;

	    /* compare to the position of each client on the same desk */
	    foreach (CompWindow *w, screen->windows ())
	    {
		if (!windowIsPlaceRelevant (w))
		    continue;

		xl = w->serverX () - w->input ().left;
		yt = w->serverY () - w->input ().top;
		xr = xl + w->serverWidth ();
		yb = yt + w->serverHeight ();

		/* if not enough room above or under the current
		 * client determine the first non-overlapped x position
		 */
		if (yTmp < yb && yt < ch + yTmp)
		{
		    if (xr > xTmp && possible > xr)
			possible = xr;

		    basket = xl - cw;
		    if (basket > xTmp && possible > basket)
			possible = basket;
		}
	    }
	    xTmp = possible;
	}
	/* else ==> not enough x dimension (overlap was wrong on horizontal) */
	else if (overlap == W_WRONG)
	{
	    xTmp     = workArea.x;
	    possible = workArea.y + workArea.height;

	    if (possible - ch > yTmp)
		possible -= ch;

	    /* test the position of each window on the desk */
	    foreach (CompWindow *w, screen->windows ())
	    {
		if (!windowIsPlaceRelevant (w))
		    continue;

		xl = w->serverX () - w->input ().left;
		yt = w->serverY () - w->input ().top;
		xr = xl + w->serverWidth ();
		yb = yt + w->serverHeight ();

		/* if not enough room to the left or right of the current
		 * client determine the first non-overlapped y position
		 */
		if (yb > yTmp && possible > yb)
		    possible = yb;

		basket = yt - ch;
		if (basket > yTmp && possible > basket)
		    possible = basket;
	    }
	    yTmp = possible;
	}
    }
    while (overlap != NONE && overlap != H_WRONG &&
	   yTmp < workArea.y + workArea.height);

    if (ch >= workArea.height)
	yOptimal = workArea.y;

    pos.setX (xOptimal + window->input ().left);
    pos.setY (yOptimal + window->input ().top);
}

static void
getWindowExtentsRect (CompWindow *w,
		      XRectangle &rect)
{
    rect.x      = w->serverX () - w->input ().left;
    rect.y      = w->serverY () - w->input ().top;
    rect.width  = w->serverWidth ();
    rect.height = w->serverHeight ();
}

static void
centerTileRectInArea (XRectangle &rect,
		      XRectangle &workArea)
{
    int fluff;

    /* The point here is to tile a window such that "extra"
     * space is equal on either side (i.e. so a full screen
     * of windows tiled this way would center the windows
     * as a group)
     */

    fluff  = (workArea.width % (rect.width + 1)) / 2;
    rect.x = workArea.x + fluff;

    fluff  = (workArea.height % (rect.height + 1)) / 3;
    rect.y = workArea.y + fluff;
}

static bool
rectFitsInWorkarea (XRectangle &workArea,
		    XRectangle &rect)
{
    if (rect.x < workArea.x)
	return false;

    if (rect.y < workArea.y)
	return false;

    if (rect.x + rect.width > workArea.x + workArea.width)
	return false;

    if (rect.y + rect.height > workArea.y + workArea.height)
	return false;

    return true;
}

static bool
rectangleIntersect (XRectangle &src1,
		    XRectangle &src2,
		    XRectangle &dest)
{
    int destX, destY;
    int destW, destH;

    destX = MAX (src1.x, src2.x);
    destY = MAX (src1.y, src2.y);
    destW = MIN (src1.x + src1.width, src2.x + src2.width) - destX;
    destH = MIN (src1.y + src1.height, src2.y + src2.height) - destY;

    if (destW <= 0 || destH <= 0)
    {
	dest.width  = 0;
	dest.height = 0;
	return false;
    }

    dest.x      = destX;
    dest.y      = destY;
    dest.width  = destW;
    dest.height = destH;

    return true;
}

static bool
rectOverlapsWindow (XRectangle     &rect,
		    CompWindowList &windows)
{
    XRectangle dest;

    foreach (CompWindow *other, windows)
    {
	XRectangle otherRect;

	switch (other->type ()) {
	case CompWindowTypeDockMask:
	case CompWindowTypeSplashMask:
	case CompWindowTypeDesktopMask:
	case CompWindowTypeDialogMask:
	case CompWindowTypeModalDialogMask:
	case CompWindowTypeFullscreenMask:
	case CompWindowTypeUnknownMask:
	    break;
	case CompWindowTypeNormalMask:
	case CompWindowTypeUtilMask:
	case CompWindowTypeToolbarMask:
	case CompWindowTypeMenuMask:
	    getWindowExtentsRect (other, otherRect);

	    if (rectangleIntersect (rect, otherRect, dest))
		return true;
	    break;
	}
    }

    return false;
}

/* Find the leftmost, then topmost, empty area on the workspace
 * that can contain the new window.
 *
 * Cool feature to have: if we can't fit the current window size,
 * try shrinking the window (within geometry constraints). But
 * beware windows such as Emacs with no sane minimum size, we
 * don't want to create a 1x1 Emacs.
 */
bool
PlaceWindow::cascadeFindFirstFit (CompWindowList &windows,
				  XRectangle     &workArea,
				  CompPoint      &pos)
{
    /* This algorithm is limited - it just brute-force tries
     * to fit the window in a small number of locations that are aligned
     * with existing windows. It tries to place the window on
     * the bottom of each existing window, and then to the right
     * of each existing window, aligned with the left/top of the
     * existing window in each of those cases.
     */
    bool           retval = FALSE;
    unsigned int   i;
    CompWindowList belowSorted, rightSorted;
    XRectangle     rect;

    /* Below each window */
    belowSorted = windows;
    belowSorted.sort (compareLeftmost);
    belowSorted.sort (compareTopmost);

    /* To the right of each window */
    rightSorted = windows;
    rightSorted.sort (compareTopmost);
    rightSorted.sort (compareLeftmost);

    getWindowExtentsRect (window, rect);
    centerTileRectInArea (rect, workArea);

    if (rectFitsInWorkarea (workArea, rect) &&
	!rectOverlapsWindow (rect, windows))
    {
	pos.setX (rect.x + window->input ().left);
	pos.setY (rect.y + window->input ().top);
	retval = true;
    }

    if (!retval)
    {
	/* try below each window */
	foreach (CompWindow *w, belowSorted)
	{
	    XRectangle outerRect;

	    if (retval)
		break;

	    getWindowExtentsRect (w, outerRect);

	    rect.x = outerRect.x;
	    rect.y = outerRect.y + outerRect.height;

	    if (rectFitsInWorkarea (workArea, rect) &&
		!rectOverlapsWindow (rect, belowSorted))
	    {
		pos.setX (rect.x + window->input ().left);
		pos.setY (rect.y + window->input ().top);
		retval = true;
	    }
	}
    }

    if (!retval)
    {
	/* try to the right of each window */
	foreach (CompWindow *w, rightSorted)
	{
	    XRectangle outerRect;

	    if (retval)
		break;

	    getWindowExtentsRect (w, outerRect);

	    rect.x = outerRect.x + outerRect.width;
	    rect.y = outerRect.y;

	    if (rectFitsInWorkarea (workArea, rect) &&
		!rectOverlapsWindow (rect, rightSorted))
	    {
		pos.setX (rect.x + w->input ().left);
		pos.setY (rect.y + w->input ().top);
		retval = true;
	    }
	}
    }

    return retval;
}

void
PlaceWindow::cascadeFindNext (CompWindowList &windows,
			      XRectangle     &workArea,
			      CompPoint      &pos)
{
    CompWindowList           sorted;
    CompWindowList::iterator iter;
    int                      cascadeX, cascadeY;
    int                      xThreshold, yThreshold;
    int                      winWidth, winHeight;
    int                      i, cascadeStage;

    sorted = windows;
    sorted.sort (compareNorthWestCorner);

    /* This is a "fuzzy" cascade algorithm.
     * For each window in the list, we find where we'd cascade a
     * new window after it. If a window is already nearly at that
     * position, we move on.
     */

    /* arbitrary-ish threshold, honors user attempts to
     * manually cascade.
     */
#define CASCADE_FUZZ 15

    xThreshold = MAX (window->input ().left, CASCADE_FUZZ);
    yThreshold = MAX (window->input ().top, CASCADE_FUZZ);

    /* Find furthest-SE origin of all workspaces.
     * cascade_x, cascade_y are the target position
     * of NW corner of window frame.
     */

    cascadeX = MAX (0, workArea.x);
    cascadeY = MAX (0, workArea.y);

    /* Find first cascade position that's not used. */

    winWidth  = window->serverWidth ();
    winHeight = window->serverHeight ();

    cascadeStage = 0;
    for (iter = sorted.begin (); iter != sorted.end (); iter++)
    {
	CompWindow *w = *iter;
	int        wx, wy;

	/* we want frame position, not window position */
	wx = w->serverX () - w->input ().left;
	wy = w->serverY () - w->input ().top;

	if (abs (wx - cascadeX) < xThreshold &&
	    abs (wy - cascadeY) < yThreshold)
	{
	    /* This window is "in the way", move to next cascade
	     * point. The new window frame should go at the origin
	     * of the client window we're stacking above.
	     */
	    wx = cascadeX = w->serverX ();
	    wy = cascadeY = w->serverY ();

	    /* If we go off the screen, start over with a new cascade */
	    if ((cascadeX + winWidth > workArea.x + workArea.width) ||
		(cascadeY + winHeight > workArea.y + workArea.height))
	    {
		cascadeX = MAX (0, workArea.x);
		cascadeY = MAX (0, workArea.y);

#define CASCADE_INTERVAL 50 /* space between top-left corners of cascades */

		cascadeStage += 1;
		cascadeX += CASCADE_INTERVAL * cascadeStage;

		/* start over with a new cascade translated to the right,
		 * unless we are out of space
		 */
		if (cascadeX + winWidth < workArea.x + workArea.width)
		{
		    iter = sorted.begin ();
		    continue;
		}
		else
		{
		    /* All out of space, this cascade_x won't work */
		    cascadeX = MAX (0, workArea.x);
		    break;
		}
	    }
	}
	else
	{
	    /* Keep searching for a further-down-the-diagonal window. */
	}
    }

    /* cascade_x and cascade_y will match the last window in the list
     * that was "in the way" (in the approximate cascade diagonal)
     */

    /* Convert coords to position of window, not position of frame. */
    pos.setX (cascadeX + window->input ().left);
    pos.setY (cascadeY + window->input ().top);
}

PlaceWindow::PlacementStrategy
PlaceWindow::getStrategy ()
{
    int opt;

    PLACE_SCREEN (screen);

    if (window->type () & (CompWindowTypeDockMask       |
			   CompWindowTypeDesktopMask    |
			   CompWindowTypeUtilMask       |
			   CompWindowTypeToolbarMask    |
			   CompWindowTypeMenuMask       |
			   CompWindowTypeFullscreenMask |
			   CompWindowTypeUnknownMask))
    {
	/* assume the app knows best how to place these */
	return NoPlacement;
    }

    if (window->wmType () & (CompWindowTypeDockMask |
			     CompWindowTypeDesktopMask))
    {
	/* see above */
	return NoPlacement;
    }

    /* no placement for unmovable windows */
    if (!(window->actions () & CompWindowActionMoveMask))
	return NoPlacement;

    opt = PLACE_OPTION_FORCE_PLACEMENT;
    if (!ps->opt[opt].value ().match ().evaluate (window))
    {
	if ((window->type () & CompWindowTypeNormalMask) ||
	    ps->opt[PLACE_OPTION_WORKAROUND].value ().b ())
	{
	    /* Only accept USPosition on non-normal windows if workarounds are
	     * enabled because apps claiming the user set -geometry for a
	     * dialog or dock are most likely wrong
	     */
	    if (window->sizeHints ().flags & USPosition)
		return ConstrainOnly;
	}

	if (window->sizeHints ().flags & PPosition)
	    return ConstrainOnly;
    }

   if (window->transientFor () &&
       (window->type () & (CompWindowTypeDialogMask |
			   CompWindowTypeModalDialogMask)))
    {
	return PlaceOverParent;
    }

    if (window->type () & (CompWindowTypeDialogMask      |
			   CompWindowTypeModalDialogMask |
			   CompWindowTypeSplashMask))
    {
	return PlaceCenteredOnScreen;
    }

    return PlaceAndConstrain;
}

const CompOutput &
PlaceWindow::getPlacementOutput (PlacementStrategy strategy,
				 CompPoint         pos)
{
    int output = -1;

    PLACE_SCREEN (screen);

    switch (strategy) {
    case PlaceOverParent:
	{
	    CompWindow *parent;

	    parent = screen->findWindow (window->transientFor ());
	    if (parent)
		output = parent->outputDevice ();
	}
	break;
    case ConstrainOnly:
	{
	    CompWindow::Geometry geom = window->serverGeometry ();

	    geom.setX (pos.x ());
	    geom.setY (pos.y ());
	    output = screen->outputDeviceForGeometry (geom);
	}
	break;
    default:
	break;
    }

    if (output >= 0)
	return screen->outputDevs ()[output];

    switch (ps->opt[PLACE_OPTION_MULTIOUTPUT_MODE].value ().i ()) {
    case PLACE_MOMODE_CURRENT:
	return screen->currentOutputDev ();
	break;
    case PLACE_MOMODE_POINTER:
	{
	    Window       wDummy;
	    int          iDummy, xPointer, yPointer;
	    unsigned int uiDummy;

	    /* this means a server roundtrip, which kind of sucks; thus
	       this code should be replaced as soon as we have software
	       cursor rendering and thus have a cached pointer coordinate */
	    if (XQueryPointer (screen->dpy (), screen->root (),
			       &wDummy, &wDummy, &xPointer, &yPointer,
			       &iDummy, &iDummy, &uiDummy))
	    {
		output = screen->outputDeviceForPoint (xPointer, yPointer);
	    }
	}
	break;
    case PLACE_MOMODE_ACTIVEWIN:
	{
	    CompWindow *active;

	    active = screen->findWindow (screen->activeWindow ());
	    if (active)
		output = active->outputDevice ();
	}
	break;
    case PLACE_MOMODE_FULLSCREEN:
	/* only place on fullscreen output if not placing centered, as the
	   constraining will move the window away from the center otherwise */
	if (strategy != PlaceCenteredOnScreen)
	    return screen->fullscreenOutput ();
	break;
    }

    if (output < 0)
	return screen->currentOutputDev ();

    return screen->outputDevs ()[output];
}

void
PlaceWindow::constrainToWorkarea (XRectangle &workArea,
				  CompPoint  &pos)
{
    CompWindowExtents extents;
    int               width, height;

    extents.left   = pos.x ()- window->input ().left;
    extents.top    = pos.y () - window->input ().top;
    extents.right  = extents.left + window->serverWidth ();
    extents.bottom = extents.top + window->serverHeight ();

    width  = extents.right - extents.left;
    height = extents.bottom - extents.top;

    if (extents.left < workArea.x)
    {
	pos.setX (pos.x () + workArea.x - extents.left);
    }
    else if (width <= workArea.width &&
	     extents.right > workArea.x + workArea.width)
    {
	pos.setX (pos.x () + workArea.x + workArea.width - extents.right);
    }

    if (extents.top < workArea.y)
    {
	pos.setY (pos.y () + workArea.y - extents.top);
    }
    else if (height <= workArea.height &&
	     extents.bottom > workArea.y + workArea.height)
    {
	pos.setY (pos.y () + workArea.y + workArea.height - extents.bottom);
    }
}

bool
PlaceWindow::windowIsPlaceRelevant (CompWindow *w)
{
    if (w->id () == window->id ())
	return false;
    if (!w->isViewable () && !w->shaded ())
	return false;
    if (w->overrideRedirect ())
	return false;
    if (w->wmType () & (CompWindowTypeDockMask | CompWindowTypeDesktopMask))
	return false;

    return true;
}

void
PlaceWindow::sendMaximizationRequest ()
{
    XEvent  xev;
    Display *dpy = screen->dpy ();

    xev.xclient.type    = ClientMessage;
    xev.xclient.display = dpy;
    xev.xclient.format  = 32;

    xev.xclient.message_type = Atoms::winState;
    xev.xclient.window	     = window->id ();

    xev.xclient.data.l[0] = 1;
    xev.xclient.data.l[1] = Atoms::winStateMaximizedHorz;
    xev.xclient.data.l[2] = Atoms::winStateMaximizedVert;
    xev.xclient.data.l[3] = 0;
    xev.xclient.data.l[4] = 0;

    XSendEvent (dpy, screen->root (), FALSE,
		SubstructureRedirectMask | SubstructureNotifyMask, &xev);
}

bool
PlaceWindow::matchXYValue (CompOption::Value::Vector &matches,
			   CompOption::Value::Vector &xValues,
			   CompOption::Value::Vector &yValues,
			   CompPoint                 &pos)
{
    int i, min;

    if (window->type () & CompWindowTypeDesktopMask)
	return false;

    min = MIN (matches.size (), xValues.size ());
    min = MIN (min, yValues.size ());

    for (i = 0; i < min; i++)
    {
	if (matches[i].match ().evaluate (window))
	{
	    pos.setX (xValues[i].i ());
	    pos.setY (yValues[i].i ());

	    return true;
	}
    }

    return false;
}

bool
PlaceWindow::matchPosition (CompPoint &pos)
{
    PLACE_SCREEN (screen);

    return matchXYValue (
	ps->opt[PLACE_OPTION_POSITION_MATCHES].value ().list (),
	ps->opt[PLACE_OPTION_POSITION_X_VALUES].value ().list (),
	ps->opt[PLACE_OPTION_POSITION_Y_VALUES].value ().list (),
	pos);
}

bool
PlaceWindow::matchViewport (CompPoint &pos)
{
    PLACE_SCREEN (screen);

    if (matchXYValue (ps->opt[PLACE_OPTION_VIEWPORT_MATCHES].value ().list (),
		      ps->opt[PLACE_OPTION_VIEWPORT_X_VALUES].value ().list (),
		      ps->opt[PLACE_OPTION_VIEWPORT_Y_VALUES].value ().list (),
		      pos))
    {
	/* Viewport matches are given 1-based, so we need to adjust that */
	pos.setX (pos.x () - 1);
	pos.setY (pos.y () - 1);

	return true;
    }

    return false;
}

bool
PlacePluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	return false;

    getMetadata ()->addFromOptionInfo (placeOptionInfo, PLACE_OPTION_NUM);
    getMetadata ()->addFromFile (name ());

    return true;
}



