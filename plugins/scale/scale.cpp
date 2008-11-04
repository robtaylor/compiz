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
#include <sys/time.h>

#include <X11/Xatom.h>
#include <X11/cursorfont.h>

#include <core/atoms.h>
#include <scale/scale.h>
#include "privates.h"

#define EDGE_STATE (CompAction::StateInitEdge)

#define WIN_X(w) ((w)->attrib.x - (w)->input.left)
#define WIN_Y(w) ((w)->attrib.y - (w)->input.top)
#define WIN_W(w) ((w)->width + (w)->input.left + (w)->input.right)
#define WIN_H(w) ((w)->height + (w)->input.top + (w)->input.bottom)

CompMetadata *scaleMetadata;

bool
PrivateScaleWindow::isNeverScaleWin () const
{
    if (window->overrideRedirect ())
	return true;

    if (window->wmType () & (CompWindowTypeDockMask | CompWindowTypeDesktopMask))
	return true;

    return false;
}

bool
PrivateScaleWindow::isScaleWin () const
{
    if (isNeverScaleWin ())
	return false;

    if (!spScreen->type || spScreen->type == ScaleTypeOutput)
    {
	if (!window->focus ())
	    return false;
    }

    if (window->state () & CompWindowStateSkipPagerMask)
	return false;

    if (window->state () & CompWindowStateShadedMask)
	return false;

    if (!window->mapNum () || !window->isViewable ())
	return false;

    switch (sScreen->priv->type) {
	case ScaleTypeGroup:
	    if (spScreen->clientLeader != window->clientLeader () &&
		spScreen->clientLeader != window->id ())
		return false;
	    break;
	case ScaleTypeOutput:
	    if (window->outputDevice () != screen->currentOutputDev ().id ())
		return false;
	default:
	    break;
    }

    if (!spScreen->currentMatch->evaluate (window))
	return false;

    return true;
}

void
PrivateScaleScreen::activateEvent (bool activating)
{
    CompOption::Vector o (0);

    o.push_back (CompOption ("root", CompOption::TypeInt));
    o.push_back (CompOption ("active", CompOption::TypeBool));

    o[0].value ().set ((int) screen->root ());
    o[1].value ().set (activating);

    screen->handleCompizEvent ("scale", "activate", o);
}

void
ScaleWindowInterface::scalePaintDecoration (const GLWindowPaintAttrib &attrib,
					    const GLMatrix &transform,
					    const CompRegion &region,
					    unsigned int mask)
    WRAPABLE_DEF (scalePaintDecoration, attrib, transform, region, mask)

void
ScaleWindow::scalePaintDecoration (const GLWindowPaintAttrib &attrib,
				   const GLMatrix &transform,
				   const CompRegion &region, unsigned int mask)
{
    WRAPABLE_HND_FUNC(0, scalePaintDecoration, attrib, transform,
		      region, mask)

    if (priv->spScreen->opt[SCALE_OPTION_ICON].value ().i () != SCALE_ICON_NONE)
    {
	GLWindowPaintAttrib sAttrib (attrib);
	GLTexture *icon;

	icon = priv->gWindow->getIcon (96, 96);
	if (!icon)
	    icon = priv->spScreen->gScreen->defaultIcon ();

	if (icon)
	{
	    float  scale;
	    float  x, y;
	    int    width, height;
	    int    scaledWinWidth, scaledWinHeight;
	    float  ds;

	    scaledWinWidth  = priv->window->width () * priv->scale;
	    scaledWinHeight = priv->window->height () * priv->scale;

	    switch (priv->spScreen->opt[SCALE_OPTION_ICON].value ().i ()) {
		case SCALE_ICON_NONE:
		case SCALE_ICON_EMBLEM:
		    scale = 1.0f;
		    break;
		case SCALE_ICON_BIG:
		default:
		    sAttrib.opacity /= 3;
		    scale = MIN (((float) scaledWinWidth / icon->width ()),
				((float) scaledWinHeight / icon->height ()));
		    break;
	    }

	    width  = icon->width () * scale;
	    height = icon->height () * scale;

	    switch (priv->spScreen->opt[SCALE_OPTION_ICON].value ().i ()) {
		case SCALE_ICON_NONE:
		case SCALE_ICON_EMBLEM:
		    x = priv->window->x () + scaledWinWidth - icon->width ();
		    y = priv->window->y () + scaledWinHeight - icon->height ();
		    break;
		case SCALE_ICON_BIG:
		default:
		    x = priv->window->x () + scaledWinWidth / 2 - width / 2;
		    y = priv->window->y () + scaledWinHeight / 2 - height / 2;
		    break;
	    }

	    x += priv->tx;
	    y += priv->ty;

	    if (priv->slot)
	    {
		priv->delta =
		    fabs (priv->slot->x1 - priv->window->x ()) +
		    fabs (priv->slot->y1 - priv->window->y ()) +
		    fabs (1.0f - priv->slot->scale) * 500.0f;
	    }

	    if (priv->delta)
	    {
		float o;

		ds =
		    fabs (priv->tx) +
		    fabs (priv->ty) +
		    fabs (1.0f - priv->scale) * 500.0f;

		if (ds > priv->delta)
		    ds = priv->delta;

		o = ds / priv->delta;

		if (priv->slot)
		{
		    if (o < priv->lastThumbOpacity)
			o = priv->lastThumbOpacity;
		}
		else
		{
		    if (o > priv->lastThumbOpacity)
			o = 0.0f;
		}

		priv->lastThumbOpacity = o;

		sAttrib.opacity = sAttrib.opacity * o;
	    }

	    mask |= PAINT_WINDOW_BLEND_MASK;

	    CompRegion iconReg (0, 0, width, height);
	    GLTexture::MatrixList ml (1);
	    ml[0] = icon->matrix ();

	    priv->gWindow->geometry().reset ();
	    if (width && height)
		priv->gWindow->glAddGeometry (ml, iconReg, iconReg);

	    if (priv->gWindow->geometry().vCount)
	    {
		GLFragment::Attrib fragment (sAttrib);
		GLMatrix wTransform (transform);

		wTransform.scale (scale, scale, 1.0f);
		wTransform.translate (x / scale, y / scale, 0.0f);

		glPushMatrix ();
		glLoadMatrixf (wTransform.getMatrix ());

		priv->gWindow->glDrawTexture (icon, fragment, mask);

		glPopMatrix ();
	    }
	}
    }
}

bool
ScaleWindowInterface::setScaledPaintAttributes (GLWindowPaintAttrib &attrib)
    WRAPABLE_DEF (setScaledPaintAttributes, attrib)


bool
ScaleWindow::setScaledPaintAttributes (GLWindowPaintAttrib &attrib)
{
    WRAPABLE_HND_FUNC_RETURN (1, bool, setScaledPaintAttributes, attrib)

    bool drawScaled = false;

    if (priv->adjust || priv->slot)
    {
	if (priv->window->id ()     != priv->spScreen->selectedWindow &&
	    priv->spScreen->opacity != OPAQUE                         &&
	    priv->spScreen->state   != SCALE_STATE_IN)
	{
	    /* modify opacity of windows that are not active */
	    attrib.opacity = (attrib.opacity * priv->spScreen->opacity) >> 16;
	}

	drawScaled = true;
    }
    else if (priv->spScreen->state != SCALE_STATE_IN)
    {
	if (priv->spScreen->opt[SCALE_OPTION_DARKEN_BACK].value ().b ())
	{
	    /* modify brightness of the other windows */
	    attrib.brightness = attrib.brightness / 2;
	}

     	/* hide windows on the outputs used for scaling 
	   that are not in scale mode */
	if (!priv->isNeverScaleWin ())
	{
	    int moMode;
	    moMode = priv->spScreen->opt[SCALE_OPTION_MULTIOUTPUT_MODE].
			value ().i ();

	    switch (moMode) {
		case SCALE_MOMODE_CURRENT:
		    if (priv->window->outputDevice () ==
			screen->currentOutputDev ().id ())
			attrib.opacity = 0;
		    break;
		default:
		    attrib.opacity = 0;
		    break;
	    }
	}
    }

    return drawScaled;
}

bool
PrivateScaleWindow::glPaint (const GLWindowPaintAttrib &attrib,
			     const GLMatrix &transform,
			     const CompRegion &region,
			     unsigned int mask)
{
    bool status;

    if (spScreen->state != SCALE_STATE_NONE)
    {
	GLWindowPaintAttrib sAttrib (attrib);
	bool                scaled;

	scaled = sWindow->setScaledPaintAttributes (sAttrib);

	if (adjust || slot)
	    mask |= PAINT_WINDOW_NO_CORE_INSTANCE_MASK;

	status = gWindow->glPaint (sAttrib, transform, region, mask);

	if (scaled)
	{
	    GLFragment::Attrib fragment (gWindow->lastPaintAttrib ());
	    GLMatrix wTransform (transform);

	    if (mask & PAINT_WINDOW_OCCLUSION_DETECTION_MASK)
		return false;

	    if (window->alpha () || fragment.getOpacity () != OPAQUE)
		mask |= PAINT_WINDOW_TRANSLUCENT_MASK;

	    wTransform.translate (window->x (), window->y (), 0.0f);
	    wTransform.scale (scale, scale, 1.0f);
	    wTransform.translate (tx / scale - window->x (),
				  ty / scale - window->y (), 0.0f);

	    glPushMatrix ();
	    glLoadMatrixf (wTransform.getMatrix ());

	    gWindow->glDraw (wTransform, fragment, region,
			     mask | PAINT_WINDOW_TRANSFORMED_MASK);

	    glPopMatrix ();

	    sWindow->scalePaintDecoration (sAttrib, transform, region, mask);
	}
    }
    else
    {
	status = gWindow->glPaint (attrib, transform, region, mask);
    }

    return status;
}

bool
PrivateScaleWindow::compareWindowsDistance (ScaleWindow *w1,
					    ScaleWindow *w2)
{
    return w2->priv->distance < w1->priv->distance;
}

void
PrivateScaleScreen::layoutSlotsForArea (CompRect workArea, int nWindows)
{
    int i, j;
    int x, y, width, height;
    int lines, n, nSlots;
    int spacing;

    if (!nWindows)
	return;

    lines   = sqrt (nWindows + 1);
    spacing = opt[SCALE_OPTION_SPACING].value ().i ();
    nSlots  = 0;

    y      = workArea.y () + spacing;
    height = (workArea.height () - (lines + 1) * spacing) / lines;

    for (i = 0; i < lines; i++)
    {
	n = MIN (nWindows - nSlots,
		 ceilf ((float)nWindows / lines));

	x     = workArea.x () + spacing;
	width = (workArea.width () - (n + 1) * spacing) / n;

	for (j = 0; j < n; j++)
	{
	    slots[this->nSlots].x1 = x;
	    slots[this->nSlots].y1 = y;
	    slots[this->nSlots].x2 = x + width;
	    slots[this->nSlots].y2 = y + height;

	    slots[this->nSlots].filled = false;

	    x += width + spacing;

	    this->nSlots++;
	    nSlots++;
	}

	y += height + spacing;
    }
}

SlotArea::vector
PrivateScaleScreen::getSlotAreas ()
{
    int                i;
    CompRect           workArea;
    std::vector<float> size;
    float              sizePerWindow, sum = 0.0f;
    int                left;
    SlotArea::vector   slotAreas;

    slotAreas.resize (screen->outputDevs ().size ());
    size.resize (screen->outputDevs ().size ());

    left = windows.size ();

    i = 0;
    foreach (CompOutput &o, screen->outputDevs ())
    {
	/* determine the size of the workarea for each output device */
	workArea = CompRect (o.workArea ());

	size[i] = workArea.width () * workArea.height ();
	sum += size[i];

	slotAreas[i].nWindows = 0;
	slotAreas[i].workArea = workArea;

	i++;
    }

    /* calculate size available for each window */
    sizePerWindow = sum / windows.size ();

    for (i = 0; i < screen->outputDevs ().size () && left; i++)
    {
	/* fill the areas with windows */
	int nw = floor (size[i] / sizePerWindow);

	nw = MIN (nw, left);
	size[i] -= nw * sizePerWindow;
	slotAreas[i].nWindows = nw;
	left -= nw;
    }

    /* add left windows to output devices with the biggest free space */
    while (left > 0)
    {
	int   num = 0;
	float big = 0;

	for (i = 0; i < screen->outputDevs ().size (); i++)
	{
	    if (size[i] > big)
	    {
		num = i;
		big = size[i];
	    }
	}

	size[num] -= sizePerWindow;
	slotAreas[num].nWindows++;
	left--;
    }

    return slotAreas;
}

void
PrivateScaleScreen::layoutSlots ()
{
    int i;
    int moMode;

    moMode  = opt[SCALE_OPTION_MULTIOUTPUT_MODE].value ().i ();

    /* if we have only one head, we don't need the 
       additional effort of the all outputs mode */
    if (screen->outputDevs ().size () == 1)
	moMode = SCALE_MOMODE_CURRENT;

    
    nSlots = 0;

    switch (moMode)
    {
	case SCALE_MOMODE_ALL:
	    {
		SlotArea::vector slotAreas = getSlotAreas ();
		if (slotAreas.size ())
		{
		    foreach (SlotArea &sa, slotAreas)
			layoutSlotsForArea (sa.workArea,
					    sa.nWindows);
		}
	    }
	    break;
	case SCALE_MOMODE_CURRENT:
	default:
	    {
		CompRect workArea (screen->currentOutputDev ().workArea ());
		layoutSlotsForArea (workArea, windows.size ());
	    }
	    break;
    }
}

void
PrivateScaleScreen::findBestSlots ()
{
    CompWindow *w;
    int        i, j, d, d0 = 0;
    float      sx, sy, cx, cy;

    foreach (ScaleWindow *sw, windows)
    {
	w = sw->priv->window;

	if (sw->priv->slot)
	    continue;

	sw->priv->sid      = 0;
	sw->priv->distance = MAXSHORT;

	for (j = 0; j < nSlots; j++)
	{
	    if (!slots[j].filled)
	    {
		sx = (slots[j].x2 + slots[j].x1) / 2;
		sy = (slots[j].y2 + slots[j].y1) / 2;

		cx = w->serverX () + w->width () / 2;
		cy = w->serverY () + w->height () / 2;

		cx -= sx;
		cy -= sy;

		d = sqrt (cx * cx + cy * cy);
		if (d0 + d < sw->priv->distance)
		{
		    sw->priv->sid      = j;
		    sw->priv->distance = d0 + d;
		}
	    }
	}

	d0 += sw->priv->distance;
    }
}

bool
PrivateScaleScreen::fillInWindows ()
{
    CompWindow *w;
    int        i, width, height;
    float      sx, sy, cx, cy;

    foreach (ScaleWindow *sw, windows)
    {
	w = sw->priv->window;

	if (!sw->priv->slot)
	{
	    if (slots[sw->priv->sid].filled)
		return true;

	    sw->priv->slot = &slots[sw->priv->sid];

	    width  = w->width ()  + w->input ().left + w->input ().right;
	    height = w->height () + w->input ().top  + w->input ().bottom;

	    sx = (float) (sw->priv->slot->x2 - sw->priv->slot->x1) / width;
	    sy = (float) (sw->priv->slot->y2 - sw->priv->slot->y1) / height;

	    sw->priv->slot->scale = MIN (MIN (sx, sy), 1.0f);

	    sx = width  * sw->priv->slot->scale;
	    sy = height * sw->priv->slot->scale;
	    cx = (sw->priv->slot->x1 + sw->priv->slot->x2) / 2;
	    cy = (sw->priv->slot->y1 + sw->priv->slot->y2) / 2;

	    cx += w->input ().left * sw->priv->slot->scale;
	    cy += w->input ().top  * sw->priv->slot->scale;

	    sw->priv->slot->x1 = cx - sx / 2;
	    sw->priv->slot->y1 = cy - sy / 2;
	    sw->priv->slot->x2 = cx + sx / 2;
	    sw->priv->slot->y2 = cy + sy / 2;

	    sw->priv->slot->filled = true;

	    sw->priv->lastThumbOpacity = 0.0f;

	    sw->priv->adjust = true;
	}
    }

    return false;
}

bool
ScaleScreenInterface::layoutSlotsAndAssignWindows ()
    WRAPABLE_DEF (layoutSlotsAndAssignWindows)

bool
ScaleScreen::layoutSlotsAndAssignWindows ()
{
    WRAPABLE_HND_FUNC_RETURN (0, bool, layoutSlotsAndAssignWindows)

    /* create a grid of slots */
    priv->layoutSlots ();

    do
    {
	/* find most appropriate slots for windows */
	priv->findBestSlots ();

	/* sort windows, window with closest distance to a slot first */
	priv->windows.sort (PrivateScaleWindow::compareWindowsDistance);

    } while (priv->fillInWindows ());

    return true;
}

bool
PrivateScaleScreen::layoutThumbs ()
{
    windows.clear ();

    /* add windows scale list, top most window first */
    foreach (CompWindow *w, screen->windows ())
    {
	SCALE_WINDOW (w);

	if (sw->priv->slot)
	    sw->priv->adjust = true;

	sw->priv->slot = NULL;

	if (!sw->priv->isScaleWin ())
	    continue;

	windows.push_back (sw);
    }

    if (windows.empty ())
	return false;

    slots.resize (windows.size ());

    return ScaleScreen::get (screen)->layoutSlotsAndAssignWindows ();
}

bool
PrivateScaleWindow::adjustScaleVelocity ()
{
    float dx, dy, ds, adjust, amount;
    float x1, y1, scale;

    if (slot)
    {
	x1 = slot->x1;
	y1 = slot->y1;
	scale = slot->scale;
    }
    else
    {
	x1 = window->x ();
	y1 = window->y ();
	scale = 1.0f;
    }

    dx = x1 - (window->x () + tx);

    adjust = dx * 0.15f;
    amount = fabs (dx) * 1.5f;
    if (amount < 0.5f)
	amount = 0.5f;
    else if (amount > 5.0f)
	amount = 5.0f;

    xVelocity = (amount * xVelocity + adjust) / (amount + 1.0f);

    dy = y1 - (window->y () + ty);

    adjust = dy * 0.15f;
    amount = fabs (dy) * 1.5f;
    if (amount < 0.5f)
	amount = 0.5f;
    else if (amount > 5.0f)
	amount = 5.0f;

    yVelocity = (amount * yVelocity + adjust) / (amount + 1.0f);

    ds = scale - this->scale;

    adjust = ds * 0.1f;
    amount = fabs (ds) * 7.0f;
    if (amount < 0.01f)
	amount = 0.01f;
    else if (amount > 0.15f)
	amount = 0.15f;

    scaleVelocity = (amount * scaleVelocity + adjust) / (amount + 1.0f);

    if (fabs (dx) < 0.1f && fabs (xVelocity) < 0.2f &&
	fabs (dy) < 0.1f && fabs (yVelocity) < 0.2f &&
	fabs (ds) < 0.001f && fabs (scaleVelocity) < 0.002f)
    {
	xVelocity = yVelocity = scaleVelocity = 0.0f;
	tx = x1 - window->x ();
	ty = y1 - window->y ();
	this->scale = scale;

	return false;
    }

    return true;
}

bool
PrivateScaleScreen::glPaintOutput (const GLScreenPaintAttrib &sAttrib,
				   const GLMatrix &transform,
				   const CompRegion &region,
				   CompOutput *output,
				   unsigned int mask)
{
    bool status;

    if (state != SCALE_STATE_NONE)
	mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;

    status = gScreen->glPaintOutput (sAttrib, transform, region, output, mask);

    return status;
}

void
PrivateScaleScreen::preparePaint (int msSinceLastPaint)
{
    if (state != SCALE_STATE_NONE && state != SCALE_STATE_WAIT)
    {
	int        steps;
	float      amount, chunk;

	amount = msSinceLastPaint * 0.05f *
	    opt[SCALE_OPTION_SPEED].value ().f ();
	steps  = amount /
	    (0.5f * opt[SCALE_OPTION_TIMESTEP].value ().f ());
	if (!steps) steps = 1;
	chunk  = amount / (float) steps;

	while (steps--)
	{
	    moreAdjust = 0;

	    foreach (CompWindow *w, screen->windows ())
	    {
		SCALE_WINDOW (w);

		if (sw->priv->adjust)
		{
		    sw->priv->adjust = sw->priv->adjustScaleVelocity ();

		    moreAdjust |= sw->priv->adjust;

		    sw->priv->tx += sw->priv->xVelocity * chunk;
		    sw->priv->ty += sw->priv->yVelocity * chunk;
		    sw->priv->scale += sw->priv->scaleVelocity * chunk;
		}
	    }

	    if (!moreAdjust)
		break;
	}
    }

    cScreen->preparePaint (msSinceLastPaint);
}

void
PrivateScaleScreen::donePaint ()
{
    if (state != SCALE_STATE_NONE)
    {
	if (moreAdjust)
	{
	    cScreen->damageScreen ();
	}
	else
	{
	    if (state == SCALE_STATE_IN)
	    {
		/* The FALSE activate event is sent when scale state
		   goes back to normal, to avoid animation conflicts
		   with other plugins. */
		activateEvent (false);
		state = SCALE_STATE_NONE;

		cScreen->preparePaintSetEnabled (this, false);
		cScreen->donePaintSetEnabled (this, false);
		gScreen->glPaintOutputSetEnabled (this, false);

		foreach (CompWindow *w, screen->windows ())
		{
		    SCALE_WINDOW (w);
		    sw->priv->cWindow->damageRectSetEnabled (sw->priv, false);
		    sw->priv->gWindow->glPaintSetEnabled (sw->priv, false);
		}
	    }
	    else if (state == SCALE_STATE_OUT)
		state = SCALE_STATE_WAIT;
	}
    }

    cScreen->donePaint ();
}

ScaleWindow *
PrivateScaleScreen::checkForWindowAt (int x, int y)
{
    int        x1, y1, x2, y2;
    CompWindowList::reverse_iterator rit = screen->windows ().rbegin ();

    for (; rit != screen->windows ().rend (); rit++)
    {
	CompWindow *w = *rit;
	SCALE_WINDOW (w);

	if (sw->priv->slot)
	{
	    x1 = w->x () - w->input ().left * sw->priv->scale;
	    y1 = w->y () - w->input ().top  * sw->priv->scale;
	    x2 = w->x () + (w->width () + w->input ().right)  * sw->priv->scale;
	    y2 = w->y () + (w->height () + w->input ().bottom) * sw->priv->scale;

	    x1 += sw->priv->tx;
	    y1 += sw->priv->ty;
	    x2 += sw->priv->tx;
	    y2 += sw->priv->ty;

	    if (x1 <= x && y1 <= y && x2 > x && y2 > y)
		return sw;
	}
    }

    return NULL;
}

void
PrivateScaleScreen::sendDndStatusMessage (Window source)
{
    XEvent xev;

    xev.xclient.type    = ClientMessage;
    xev.xclient.display = screen->dpy ();
    xev.xclient.format  = 32;

    xev.xclient.message_type = Atoms::xdndStatus;
    xev.xclient.window	     = source;

    xev.xclient.data.l[0] = dndTarget;
    xev.xclient.data.l[1] = 2;
    xev.xclient.data.l[2] = 0;
    xev.xclient.data.l[3] = 0;
    xev.xclient.data.l[4] = None;

    XSendEvent (screen->dpy (), source, FALSE, 0, &xev);
}

bool
PrivateScaleScreen::scaleTerminate (CompAction         *action,
				    CompAction::State  state,
				    CompOption::Vector &options)
{
    CompScreen *s;
    Window     xid;

    xid = CompOption::getIntOptionNamed (options, "root");

    if (!xid || ::screen->root () == xid)
    {
	SCALE_SCREEN (::screen);

	if (ss->priv->grab)
	{
	    if (ss->priv->grabIndex)
	    {
		::screen->removeGrab (ss->priv->grabIndex, 0);
		ss->priv->grabIndex = 0;
	    }

	    if (ss->priv->dndTarget)
		XUnmapWindow (::screen->dpy (), ss->priv->dndTarget);

	    ss->priv->grab = false;

	    if (ss->priv->state != SCALE_STATE_NONE)
	    {
		foreach (CompWindow *w, ::screen->windows ())
		{
		    SCALE_WINDOW (w);

		    if (sw->priv->slot)
		    {
			sw->priv->slot   = NULL;
			sw->priv->adjust = true;
		    }
		}

		if (state & CompAction::StateCancel)
		{
		    if (::screen->activeWindow () !=
			ss->priv->previousActiveWindow)
		    {
			CompWindow *w =
			    ::screen->findWindow (ss->priv->previousActiveWindow);
			if (w)
			    w->moveInputFocusTo ();
		    }
		}
		else if (ss->priv->state != SCALE_STATE_IN)
		{
		    CompWindow *w =
			::screen->findWindow (ss->priv->selectedWindow);
		    if (w)
			w->activate ();
		}

		ss->priv->state = SCALE_STATE_IN;

		ss->priv->cScreen->damageScreen ();
	    }

	    ss->priv->lastActiveNum = 0;
	}
    }

    action->setState (action->state () &
		      ~(CompAction::StateTermKey |
			CompAction::StateTermButton));

    return false;
}

bool
PrivateScaleScreen::ensureDndRedirectWindow ()
{
    if (!dndTarget)
    {
	XSetWindowAttributes attr;
	long		     xdndVersion = 3;

	attr.override_redirect = TRUE;

	dndTarget = XCreateWindow (screen->dpy (), screen->root (),
				   0, 0, 1, 1, 0, CopyFromParent,
				   InputOnly, CopyFromParent,
				   CWOverrideRedirect, &attr);

	XChangeProperty (screen->dpy (), dndTarget,
			 Atoms::xdndAware,
			 XA_ATOM, 32, PropModeReplace,
			 (unsigned char *) &xdndVersion, 1);
    }

    XMoveResizeWindow (screen->dpy (), dndTarget,
		       0, 0, screen->width (), screen->height ());
    XMapRaised (screen->dpy (), dndTarget);

    return true;
}

bool
PrivateScaleScreen::scaleInitiate (CompAction         *action,
				   CompAction::State  state,
				   CompOption::Vector &options,
				   ScaleType          type)
{
    Window     xid;

    xid = CompOption::getIntOptionNamed (options, "root");

    if (::screen->root () == xid)
    {
	SCALE_SCREEN (::screen);

	if (ss->priv->state != SCALE_STATE_WAIT &&
	    ss->priv->state != SCALE_STATE_OUT)
	{
	    ss->priv->type = type;
	    return ss->priv->scaleInitiateCommon (action, state, options);
	}
	else if (state & EDGE_STATE)
	{
	    if (ss->priv->type == type)
		return scaleTerminate (action, CompAction::StateCancel, options);
	}
    }

    return false;
}

bool
PrivateScaleScreen::scaleInitiateCommon (CompAction         *action,
					 CompAction::State  state,
					 CompOption::Vector &options)
{
    if (screen->otherGrabExist ("scale", 0))
	return false;

    currentMatch = &opt[SCALE_OPTION_WINDOW_MATCH].value ().match ();

    CompMatch empty;
    CompMatch match (CompOption::getMatchOptionNamed (options, "match", empty));

    if (!(match == empty))
    {
	this->match = match;
	this->match.update ();
	currentMatch = &this->match;
    }

    if (!layoutThumbs ())
	return false;

    if (state & CompAction::StateInitEdgeDnd)
    {
	if (ensureDndRedirectWindow ())
	    grab = true;
    }
    else if (!grabIndex)
    {
	grabIndex = screen->pushGrab (cursor, "scale");
	if (grabIndex)
	    grab = true;
    }

    if (grab)
    {
	if (!lastActiveNum)
	    lastActiveNum = screen->activeNum () - 1;

	previousActiveWindow = screen->activeWindow ();
	lastActiveWindow     = screen->activeWindow ();
	selectedWindow       = screen->activeWindow ();
	hoveredWindow        = None;

	this->state = SCALE_STATE_OUT;

	activateEvent (true);

	cScreen->damageScreen ();

	cScreen->preparePaintSetEnabled (this, true);
	cScreen->donePaintSetEnabled (this, true);
	gScreen->glPaintOutputSetEnabled (this, true);

	foreach (CompWindow *w, screen->windows ())
	{
	    SCALE_WINDOW (w);
	    sw->priv->cWindow->damageRectSetEnabled (sw->priv, true);
	    sw->priv->gWindow->glPaintSetEnabled (sw->priv, true);
	}
    }

    if ((state & (CompAction::StateInitButton | EDGE_STATE)) ==
	CompAction::StateInitButton)
    {
	action->setState (action->state () | CompAction::StateTermButton);
    }

    if (state & CompAction::StateInitKey)
	action->setState (action->state () | CompAction::StateTermKey);

    return false;
}

void
ScaleWindowInterface::scaleSelectWindow ()
    WRAPABLE_DEF (scaleSelectWindow)

void
ScaleWindow::scaleSelectWindow ()
{
    WRAPABLE_HND_FUNC(2, scaleSelectWindow)

    if (priv->spScreen->selectedWindow != priv->window->id ())
    {
	CompWindow *oldW, *newW;

	oldW = screen->findWindow (priv->spScreen->selectedWindow);
	newW = screen->findWindow (priv->window->id ());

	priv->spScreen->selectedWindow = priv->window->id ();

	if (oldW)
	    CompositeWindow::get (oldW)->addDamage ();

	if (newW)
	    CompositeWindow::get (newW)->addDamage ();
    }
}

bool
PrivateScaleScreen::selectWindowAt (int x, int y, bool moveInputFocus)
{
    ScaleWindow *w = checkForWindowAt (x, y);
    if (w && w->priv->isScaleWin ())
    {
	w->scaleSelectWindow ();

	if (moveInputFocus)
	{
	    lastActiveNum    = w->priv->window->activeNum ();
	    lastActiveWindow = w->priv->window->id ();

	    w->priv->window->moveInputFocusTo ();
	}

	hoveredWindow = w->priv->window->id ();

	return true;
    }

    hoveredWindow = None;

    return false;
}

void
PrivateScaleScreen::moveFocusWindow (int dx, int dy)
{
    CompWindow *active;
    CompWindow *focus = NULL;

    active = screen->findWindow (screen->activeWindow ());
    if (active)
    {
	SCALE_WINDOW (active);

	if (sw->priv->slot)
	{
	    ScaleSlot  *slot;
	    int	       x, y, cx, cy, d, min = MAXSHORT;

	    cx = (sw->priv->slot->x1 + sw->priv->slot->x2) / 2;
	    cy = (sw->priv->slot->y1 + sw->priv->slot->y2) / 2;

	    foreach (CompWindow *w, screen->windows ())
	    {
		slot = ScaleWindow::get (w)->priv->slot;
		if (!slot)
		    continue;

		x = (slot->x1 + slot->x2) / 2;
		y = (slot->y1 + slot->y2) / 2;

		d = abs (x - cx) + abs (y - cy);
		if (d < min)
		{
		    if ((dx > 0 && slot->x1 < sw->priv->slot->x2) ||
			(dx < 0 && slot->x2 > sw->priv->slot->x1) ||
			(dy > 0 && slot->y1 < sw->priv->slot->y2) ||
			(dy < 0 && slot->y2 > sw->priv->slot->y1))
			continue;

		    min   = d;
		    focus = w;
		}
	    }
	}
    }

    /* move focus to the last focused window if no slot window is currently
       focused */
    if (!focus)
    {
	foreach (CompWindow *w, screen->windows ())
	{
	    if (!ScaleWindow::get (w)->priv->slot)
		continue;

	    if (!focus || focus->activeNum () < w->activeNum ())
		focus = w;
	}
    }

    if (focus)
    {
	ScaleWindow::get (focus)->scaleSelectWindow ();

	lastActiveNum    = focus->activeNum ();
	lastActiveWindow = focus->id ();

	focus->moveInputFocusTo ();
    }
}

bool
PrivateScaleScreen::scaleRelayoutSlots (CompAction         *action,
					CompAction::State  state,
					CompOption::Vector &options)
{
    Window xid = CompOption::getIntOptionNamed (options, "root");

    if (::screen->root () == xid)
    {
	SCALE_SCREEN (::screen);
	if (ss->priv->state != SCALE_STATE_NONE &&
	    ss->priv->state != SCALE_STATE_IN)
	{
	    if (ss->priv->layoutThumbs ())
	    {
		ss->priv->state = SCALE_STATE_OUT;
		ss->priv->moveFocusWindow (0, 0);
		ss->priv->cScreen->damageScreen ();
	    }
	}

	return true;
    }

    return false;
}

void
PrivateScaleScreen::windowRemove (Window id)
{
    CompWindow *w = screen->findWindow (id);
    if (w)
    {
	if (state != SCALE_STATE_NONE && state != SCALE_STATE_IN)
	{
	    foreach (ScaleWindow *lw, windows)
	    {
		if (lw->priv->window == w)
		{
		    if (layoutThumbs ())
		    {
			state = SCALE_STATE_OUT;
			cScreen->damageScreen ();
			break;
		    }
		    else
		    {
    
			CompOption::Vector o (0);
			CompAction *action;

			/* terminate scale mode if the recently closed
			 * window was the last scaled window */

			o.push_back (CompOption ("root", CompOption::TypeInt));
			o[0].value ().set ((int) screen->root ());

			action =
			    &opt[SCALE_OPTION_INITIATE_EDGE].value ().action ();
			scaleTerminate (action, CompAction::StateCancel, o);

			action =
			    &opt[SCALE_OPTION_INITIATE_KEY].value ().action ();
			scaleTerminate (action, CompAction::StateCancel, o);
			break;
		    }
		}
	    }
	}
    }
}

bool
PrivateScaleScreen::hoverTimeout ()
{
    if (grab && state != SCALE_STATE_IN)
    {
	CompWindow *w;
	CompOption::Vector o (0);
	int	   option;

	w = screen->findWindow (selectedWindow);
	if (w)
	{
	    lastActiveNum    = w->activeNum ();
	    lastActiveWindow = w->id ();

	    w->moveInputFocusTo ();
	}

	o.push_back (CompOption ("root", CompOption::TypeInt));
	o[0].value ().set ((int) screen->root ());
	
	option = SCALE_OPTION_INITIATE_EDGE;
	scaleTerminate (&opt[option].value ().action (), 0, o);
	option = SCALE_OPTION_INITIATE_KEY;
	scaleTerminate (&opt[option].value ().action (), 0, o);
    }

    return false;
}

void
PrivateScaleScreen::handleEvent (XEvent *event)
{

    switch (event->type) {
	case KeyPress:
	    if (screen->root () == event->xkey.root)
	    {
		if (grabIndex)
		{
		    if (event->xkey.keycode == leftKeyCode)
			moveFocusWindow (-1, 0);
		    else if (event->xkey.keycode == rightKeyCode)
			moveFocusWindow (1, 0);
		    else if (event->xkey.keycode == upKeyCode)
			moveFocusWindow (0, -1);
		    else if (event->xkey.keycode == downKeyCode)
			moveFocusWindow (0, 1);
		}
	    }
	    break;
	case ButtonPress:
	    if (event->xbutton.button == Button1)
	    {
		if (screen->root () == event->xbutton.root)
		{
		    int option;

		    if (grabIndex && state != SCALE_STATE_IN)
		    {
			CompOption::Vector o (0);
			o.push_back (CompOption ("root", CompOption::TypeInt));
			o[0].value ().set ((int) screen->root ());


			if (selectWindowAt (event->xbutton.x_root,
					    event->xbutton.y_root,
					    true))
			{
			    option = SCALE_OPTION_INITIATE_EDGE;
			    scaleTerminate (&opt[option].value ().action (),
					    0, o);
			    option = SCALE_OPTION_INITIATE_KEY;
			    scaleTerminate (&opt[option].value ().action (),
					    0, o);
			}
			else if (event->xbutton.x_root > screen->workArea ().x &&
				 event->xbutton.x_root < (screen->workArea ().x +
							screen->workArea ().width) &&
				event->xbutton.y_root > screen->workArea ().y &&
				event->xbutton.y_root < (screen->workArea ().y +
							screen->workArea ().height))
			{
			    if (opt[SCALE_OPTION_SHOW_DESKTOP].value ().b ())
			    {
				option = SCALE_OPTION_INITIATE_EDGE;
				scaleTerminate (&opt[option].value ().action (),
						0, o);
				option = SCALE_OPTION_INITIATE_KEY;
				scaleTerminate (&opt[option].value ().action (),
						0, o);
				screen->enterShowDesktopMode ();
			    }
			}
		    }
		}
	    }
	    break;
	case MotionNotify:
	    if (screen->root () == event->xmotion.root)
	    {
		if (grabIndex && state != SCALE_STATE_IN)
		{
		    bool focus = false;
		    CompOption *o = screen->getOption ("click_to_focus");

		    if (o && o->value ().b ())
			focus = true;

		    selectWindowAt (event->xmotion.x_root,
				    event->xmotion.y_root,
				    focus);
		}
	    }
	    break;
	case ClientMessage:
	    if (event->xclient.message_type == Atoms::xdndPosition)
	    {
		CompWindow *w;

		w = screen->findWindow (event->xclient.window);
		if (w)
		{
		    bool focus = false;
		    CompOption *o = screen->getOption ("click_to_focus");

		    if (o && o->value ().b ())
			focus = true;

		    if (w->id () == dndTarget)
			sendDndStatusMessage (event->xclient.data.l[0]);

		    if (grab			&&
			state != SCALE_STATE_IN &&
			w->id () == dndTarget)
		    {
			int x = event->xclient.data.l[2] >> 16;
			int y = event->xclient.data.l[2] & 0xffff;

			ScaleWindow *sw = checkForWindowAt (x, y);
			if (sw && sw->priv->isScaleWin ())
			{
			    int time;

			    time = opt[SCALE_OPTION_HOVER_TIME].value ().i ();

			    if (hover.active ())
			    {
				if (w->id () != selectedWindow)
				{
				    hover.stop ();
				}
			    }

			    if (!hover.active ())
				hover.start (boost::bind (&PrivateScaleScreen::hoverTimeout, this),
					     time, (float) time * 1.2);


			    selectWindowAt (x, y, focus);
			}
			else
			{
			    if (hover.active ())
				hover.stop ();
			}
		    }
		}
	    }
	    else if (event->xclient.message_type == Atoms::xdndDrop ||
		    event->xclient.message_type == Atoms::xdndLeave)
	    {
		CompWindow *w = screen->findWindow (event->xclient.window);
		if (w)
		{
		    int option;

		    if (grab			&&
			state != SCALE_STATE_IN &&
			w->id () == dndTarget)
		    {
			CompOption::Vector o (0);
			o.push_back (CompOption ("root", CompOption::TypeInt));
			o[0].value ().set ((int) screen->root ());

			option = SCALE_OPTION_INITIATE_EDGE;
			scaleTerminate (&opt[option].value ().action (), 0, o);
			option = SCALE_OPTION_INITIATE_KEY;
			scaleTerminate (&opt[option].value ().action (), 0, o);
		    }
		}
	    }
	default:
	    break;
    }

    screen->handleEvent (event);

    switch (event->type) {
	case UnmapNotify:
	    windowRemove (event->xunmap.window);
	    break;
	case DestroyNotify:
	    windowRemove (event->xdestroywindow.window);
	    break;
    }

}

bool
PrivateScaleWindow::damageRect (bool initial, const CompRect &rect)
{
    bool status = false;

    if (initial)
    {
	if (spScreen->grab && isScaleWin ())
	{
	    if (spScreen->layoutThumbs ())
	    {
		spScreen->state = SCALE_STATE_OUT;
		spScreen->cScreen->damageScreen ();
	    }
	}
    }
    else if (spScreen->state == SCALE_STATE_WAIT)
    {
	if (slot)
	{
	    cWindow->damageTransformedRect (scale, scale, tx, ty, rect);

	    status = true;
	}
    }

    status |= cWindow->damageRect (initial, rect);

    return status;
}

#define SCALEBIND(a) boost::bind (PrivateScaleScreen::scaleInitiate, _1, _2, _3, a)

static const CompMetadata::OptionInfo scaleOptionInfo[] = {
    { "initiate_edge", "edge", 0,
      SCALEBIND(ScaleTypeNormal), PrivateScaleScreen::scaleTerminate },
    { "initiate_button", "button", 0,
      SCALEBIND(ScaleTypeNormal), PrivateScaleScreen::scaleTerminate },
    { "initiate_key", "key", 0,
      SCALEBIND(ScaleTypeNormal), PrivateScaleScreen::scaleTerminate },
    { "initiate_all_edge", "edge", 0,
      SCALEBIND(ScaleTypeAll), PrivateScaleScreen::scaleTerminate },
    { "initiate_all_button", "button", 0,
      SCALEBIND(ScaleTypeAll), PrivateScaleScreen::scaleTerminate },
    { "initiate_all_key", "key", 0,
      SCALEBIND(ScaleTypeAll), PrivateScaleScreen::scaleTerminate },
    { "initiate_group_edge", "edge", 0,
      SCALEBIND(ScaleTypeGroup), PrivateScaleScreen::scaleTerminate },
    { "initiate_group_button", "button", 0,
      SCALEBIND(ScaleTypeGroup), PrivateScaleScreen::scaleTerminate },
    { "initiate_group_key", "key", 0,
      SCALEBIND(ScaleTypeGroup), PrivateScaleScreen::scaleTerminate },
    { "initiate_output_edge", "edge", 0,
      SCALEBIND(ScaleTypeOutput), PrivateScaleScreen::scaleTerminate },
    { "initiate_output_button", "button", 0,
      SCALEBIND(ScaleTypeOutput), PrivateScaleScreen::scaleTerminate },
    { "initiate_output_key", "key", 0,
      SCALEBIND(ScaleTypeOutput), PrivateScaleScreen::scaleTerminate },
    { "show_desktop", "bool", 0, 0, 0 },
    { "relayout_slots", "action", 0,
      PrivateScaleScreen::scaleRelayoutSlots, 0 },
    { "spacing", "int", "<min>0</min>", 0, 0 },
    { "speed", "float", "<min>0.1</min>", 0, 0 },
    { "timestep", "float", "<min>0.1</min>", 0, 0 },
    { "window_match", "match", 0, 0, 0 },
    { "darken_back", "bool", 0, 0, 0 },
    { "opacity", "int", "<min>0</min><max>100</max>", 0, 0 },
    { "overlay_icon", "int", RESTOSTRING (0, SCALE_ICON_LAST), 0, 0 },
    { "hover_time", "int", "<min>50</min>", 0, 0 },
    { "multioutput_mode", "int", RESTOSTRING (0, SCALE_MOMODE_LAST), 0, 0 }
};

#undef SCALEBIND

ScaleScreen::ScaleScreen (CompScreen *s) :
    PrivateHandler<ScaleScreen, CompScreen, COMPIZ_SCALE_ABI> (s),
    priv (new PrivateScaleScreen (s))
{
}

ScaleScreen::~ScaleScreen ()
{
    delete priv;
}

ScaleWindow::ScaleWindow (CompWindow *w) :
    PrivateHandler<ScaleWindow, CompWindow, COMPIZ_SCALE_ABI> (w),
    priv (new PrivateScaleWindow (w))
{
}

ScaleWindow::~ScaleWindow ()
{
    delete priv;
}


PrivateScaleScreen::PrivateScaleScreen (CompScreen *s) :
    cScreen (CompositeScreen::get (s)),
    gScreen (GLScreen::get (s)),

    lastActiveNum (0),
    lastActiveWindow (None),

    selectedWindow (None),
    hoveredWindow (None),
    previousActiveWindow (None),
    grab (false),
    grabIndex (0),
    dndTarget (None),
    state (SCALE_STATE_NONE),
    moreAdjust (false),
    cursor (0),
    nSlots (0),
    currentMatch (&match)
{
    if (!scaleMetadata->initOptions (scaleOptionInfo, SCALE_OPTION_NUM, opt))
    {
	ScaleScreen::get (s)->setFailed ();
	return;
    }

    leftKeyCode  = XKeysymToKeycode (screen->dpy (), XStringToKeysym ("Left"));
    rightKeyCode = XKeysymToKeycode (screen->dpy (), XStringToKeysym ("Right"));
    upKeyCode    = XKeysymToKeycode (screen->dpy (), XStringToKeysym ("Up"));
    downKeyCode  = XKeysymToKeycode (screen->dpy (), XStringToKeysym ("Down"));

    cursor = XCreateFontCursor (screen->dpy (), XC_left_ptr);

    opacity = (OPAQUE * opt[SCALE_OPTION_OPACITY].value ().i ()) / 100;


    ScreenInterface::setHandler (s);
    CompositeScreenInterface::setHandler (cScreen, false);
    GLScreenInterface::setHandler (gScreen, false);
}

PrivateScaleScreen::~PrivateScaleScreen ()
{
    if (cursor)
	XFreeCursor (screen->dpy (), cursor);
}


PrivateScaleWindow::PrivateScaleWindow (CompWindow *w) :
    window (w),
    cWindow (CompositeWindow::get (w)),
    gWindow (GLWindow::get (w)),
    sWindow (ScaleWindow::get (w)),
    sScreen (ScaleScreen::get (screen)),
    spScreen (sScreen->priv),
    slot (NULL),
    sid (0),
    distance (0.0),
    xVelocity (0.0),
    yVelocity (0.0),
    scaleVelocity (0.0),
    scale (1.0),
    tx (0.0),
    ty (0.0),
    delta (1.0),
    adjust (false),
    lastThumbOpacity (0.0)
{
    CompositeWindowInterface::setHandler (cWindow,
					  spScreen->state != SCALE_STATE_NONE);
    GLWindowInterface::setHandler (gWindow,
				   spScreen->state != SCALE_STATE_NONE);
}

PrivateScaleWindow::~PrivateScaleWindow ()
{
}

CompOption::Vector &
ScaleScreen::getOptions ()
{
    return priv->opt;
}

CompOption *
ScaleScreen::getOption (const char *name)
{
    CompOption *o = CompOption::findOption (priv->opt, name);
    return o;
}

bool
ScaleScreen::setOption (const char        *name,
			CompOption::Value &value)
{
    CompOption   *o;
    unsigned int index;

    o = CompOption::findOption (priv->opt, name, &index);
    if (!o)
	return false;

    switch (index) {
	case SCALE_OPTION_OPACITY:
	    if (o->set (value))
	    {
		priv->opacity = (OPAQUE * o->value ().i ()) / 100;
		return true;
	    }
	default:
	    if (CompOption::setOption (*o, value))
		return true;
	    break;
    }

    return false;
}

class ScalePluginVTable :
    public CompPlugin::VTableForScreenAndWindow<ScaleScreen, ScaleWindow>
{
    public:

	const char * name () { return "scale"; };

	CompMetadata * getMetadata ();

	bool init ();

	void fini ();

	PLUGIN_OPTION_HELPER (ScaleScreen)
};

bool
ScalePluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
        !CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) ||
        !CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
	return false;

    CompPrivate p;
    p.uval = COMPIZ_SCALE_ABI;
    screen->storeValue ("scale_ABI", p);

    scaleMetadata = new CompMetadata (name (), scaleOptionInfo,
				      SCALE_OPTION_NUM);

    if (!scaleMetadata)
	return false;

    scaleMetadata->addFromFile (name ());

    return true;
}

void
ScalePluginVTable::fini ()
{
    screen->eraseValue ("scale_ABI");
    delete scaleMetadata;
}

CompMetadata *
ScalePluginVTable::getMetadata ()
{
    return scaleMetadata;
}

ScalePluginVTable scaleVTable;

CompPlugin::VTable *
getCompPluginInfo20080805 (void)
{
    return &scaleVTable;
}
