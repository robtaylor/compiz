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

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include <core/core.h>
#include <decoration.h>
#include "decor.h"

#include <X11/Xatom.h>
#include <X11/extensions/shape.h>

static CompMetadata *decorMetadata;

bool
DecorWindow::glDraw (const GLMatrix     &transform,
		     GLFragment::Attrib &attrib,
		     Region             region,
		     unsigned int       mask)
{
    bool status;

    status = gWindow->glDraw (transform, attrib, region, mask);

    if (mask & PAINT_WINDOW_TRANSFORMED_MASK)
	region = &infiniteRegion;

    if (wd && region->numRects)
    {
	REGION	     box;

	mask |= PAINT_WINDOW_BLEND_MASK;

	box.rects	 = &box.extents;
	box.numRects = 1;

	gWindow->geometry ().reset ();

	for (int i = 0; i < wd->nQuad; i++)
	{
	    box.extents = wd->quad[i].box;

	    if (box.extents.x1 < box.extents.x2 &&
		box.extents.y1 < box.extents.y2)
	    {
		gWindow->glAddGeometry (&wd->quad[i].matrix, 1, &box, region);
	    }
	}

	if (gWindow->geometry ().vCount)
	    gWindow->glDrawTexture (wd->decor->texture, attrib, mask);
    }

    return status;
}

DecorTexture::DecorTexture (Pixmap pixmap) :
    GLTexture (),
    status (true),
    refCount (1),
    pixmap (pixmap),
    damage (None)
{
    unsigned int width, height, depth, ui;
    Window	 root;
    int		 i;

    if (!XGetGeometry (screen->dpy (), pixmap, &root,
		       &i, &i, &width, &height, &ui, &depth))
    {
        status = false;
	return;
    }

    if (!bindPixmap (pixmap, width, height, depth))
    {
        status = false;
	return;
    }

    if (!DecorScreen::get (screen)->opt[DECOR_OPTION_MIPMAP].value ().b ())
	mipmap () = false;

    damage = XDamageCreate (screen->dpy (), pixmap,
			     XDamageReportRawRectangles);
}

DecorTexture::~DecorTexture ()
{
    if (damage)
	XDamageDestroy (screen->dpy (), damage);
}

DecorTexture *
DecorScreen::getTexture (Pixmap pixmap)
{
    foreach (DecorTexture *t, textures)
	if (t->pixmap == pixmap)
	{
	    t->refCount++;
	    return t;
	}

    DecorTexture *texture = new DecorTexture (pixmap);

    if (!texture->status)
    {
	delete texture;
	return NULL;
    }

    textures.push_back (texture);

    return texture;
}


void
DecorScreen::releaseTexture (DecorTexture *texture)
{
    texture->refCount--;
    if (texture->refCount)
	return;

    std::list<DecorTexture *>::iterator it =
	std::find (textures.begin (), textures.end (), texture);

    if (it == textures.end ())
	return;

    textures.erase (it);
    delete texture;
}

static void
computeQuadBox (decor_quad_t *q,
		int	     width,
		int	     height,
		int	     *return_x1,
		int	     *return_y1,
		int	     *return_x2,
		int	     *return_y2,
		float        *return_sx,
		float        *return_sy)
{
    int   x1, y1, x2, y2;
    float sx = 1.0f;
    float sy = 1.0f;

    decor_apply_gravity (q->p1.gravity, q->p1.x, q->p1.y, width, height,
			 &x1, &y1);
    decor_apply_gravity (q->p2.gravity, q->p2.x, q->p2.y, width, height,
			 &x2, &y2);

    if (q->clamp & CLAMP_HORZ)
    {
	if (x1 < 0)
	    x1 = 0;
	if (x2 > width)
	    x2 = width;
    }

    if (q->clamp & CLAMP_VERT)
    {
	if (y1 < 0)
	    y1 = 0;
	if (y2 > height)
	    y2 = height;
    }

    if (q->stretch & STRETCH_X)
    {
	sx = (float)q->max_width / ((float)(x2 - x1));
    }
    else if (q->max_width < x2 - x1)
    {
	if (q->align & ALIGN_RIGHT)
	    x1 = x2 - q->max_width;
	else
	    x2 = x1 + q->max_width;
    }

    if (q->stretch & STRETCH_Y)
    {
	sy = (float)q->max_height / ((float)(y2 - y1));
    }
    else if (q->max_height < y2 - y1)
    {
	if (q->align & ALIGN_BOTTOM)
	    y1 = y2 - q->max_height;
	else
	    y2 = y1 + q->max_height;
    }

    *return_x1 = x1;
    *return_y1 = y1;
    *return_x2 = x2;
    *return_y2 = y2;

    if (return_sx)
	*return_sx = sx;
    if (return_sy)
	*return_sy = sy;
}

Decoration *
Decoration::create (Window     id,
		    Atom       decorAtom)
{
    Decoration	    *decoration;
    Atom	    actual;
    int		    result, format;
    unsigned long   n, nleft;
    unsigned char   *data;
    long	    *prop;
    Pixmap	    pixmap;
    decor_extents_t input;
    decor_extents_t maxInput;
    decor_quad_t    *quad;
    int		    nQuad;
    int		    minWidth;
    int		    minHeight;
    int		    left, right, top, bottom;
    int		    x1, y1, x2, y2;

    result = XGetWindowProperty (screen->dpy (), id,
				 decorAtom, 0L, 1024L, FALSE,
				 XA_INTEGER, &actual, &format,
				 &n, &nleft, &data);

    if (result != Success || !n || !data)
	return NULL;

    prop = (long *) data;

    if (decor_property_get_version (prop) != decor_version ())
    {
	compLogMessage ("decoration", CompLogLevelWarn,
			"Property ignored because "
			"version is %d and decoration plugin version is %d\n",
			decor_property_get_version (prop), decor_version ());

	XFree (data);
	return NULL;
    }

    nQuad = (n - BASE_PROP_SIZE) / QUAD_PROP_SIZE;

    quad = new decor_quad_t [nQuad];
    if (!quad)
    {
	XFree (data);
	return NULL;
    }

    nQuad = decor_property_to_quads (prop, n, &pixmap, &input, &maxInput,
				     &minWidth, &minHeight, quad);

    XFree (data);

    if (!nQuad)
    {
	delete [] quad;
	return NULL;
    }

    decoration = new Decoration ();
    if (!decoration)
    {
	delete [] quad;
	return NULL;
    }

    decoration->texture = DecorScreen::get (screen)->getTexture (pixmap);
    if (!decoration->texture)
    {
	delete decoration;
	delete [] quad;
	return NULL;
    }

    decoration->minWidth  = minWidth;
    decoration->minHeight = minHeight;
    decoration->quad	  = quad;
    decoration->nQuad	  = nQuad;

    left   = 0;
    right  = minWidth;
    top    = 0;
    bottom = minHeight;

    while (nQuad--)
    {
	computeQuadBox (quad, minWidth, minHeight, &x1, &y1, &x2, &y2,
			NULL, NULL);

	if (x1 < left)
	    left = x1;
	if (y1 < top)
	    top = y1;
	if (x2 > right)
	    right = x2;
	if (y2 > bottom)
	    bottom = y2;

	quad++;
    }

    decoration->output.left   = -left;
    decoration->output.right  = right - minWidth;
    decoration->output.top    = -top;
    decoration->output.bottom = bottom - minHeight;

    decoration->input.left   = input.left;
    decoration->input.right  = input.right;
    decoration->input.top    = input.top;
    decoration->input.bottom = input.bottom;

    decoration->maxInput.left   = maxInput.left;
    decoration->maxInput.right  = maxInput.right;
    decoration->maxInput.top    = maxInput.top;
    decoration->maxInput.bottom = maxInput.bottom;

    decoration->refCount = 1;

    return decoration;
}

void
Decoration::release (Decoration *decoration)
{
    decoration->refCount--;
    if (decoration->refCount)
	return;

    DecorScreen::get (screen)->releaseTexture (decoration->texture);

    delete [] decoration->quad;
    delete decoration;
}

void
DecorWindow::updateDecoration ()
{
    Decoration *decoration;

    decoration = Decoration::create (window->id (), dScreen->winDecorAtom);

    if (decor)
	Decoration::release (decor);

    decor = decoration;
}

WindowDecoration *
WindowDecoration::create (Decoration *d)
{
    WindowDecoration *wd;

    wd = new WindowDecoration ();
    if (!wd)
	return NULL;

    wd->quad = new ScaledQuad[d->nQuad];

    if (!wd->quad)
    {
	delete wd;
	return NULL;
    }

    d->refCount++;

    wd->decor = d;
    wd->nQuad = d->nQuad;

    return wd;
}

void
WindowDecoration::destroy (WindowDecoration *wd)
{
    Decoration::release (wd->decor);
    delete [] wd->quad;
    delete wd;
}

void
DecorWindow::setDecorationMatrices ()
{
    int		      i;
    float	      x0, y0;
    decor_matrix_t    a;
    GLTexture::Matrix b;

    if (!wd)
	return;

    for (i = 0; i < wd->nQuad; i++)
    {
	wd->quad[i].matrix = wd->decor->texture->matrix ();

	x0 = wd->decor->quad[i].m.x0;
	y0 = wd->decor->quad[i].m.y0;

	a = wd->decor->quad[i].m;
	b = wd->quad[i].matrix;

	wd->quad[i].matrix.xx = a.xx * b.xx + a.yx * b.xy;
	wd->quad[i].matrix.yx = a.xx * b.yx + a.yx * b.yy;
	wd->quad[i].matrix.xy = a.xy * b.xx + a.yy * b.xy;
	wd->quad[i].matrix.yy = a.xy * b.yx + a.yy * b.yy;
	wd->quad[i].matrix.x0 = x0 * b.xx + y0 * b.xy + b.x0;
	wd->quad[i].matrix.y0 = x0 * b.yx + y0 * b.yy + b.y0;

	wd->quad[i].matrix.xx *= wd->quad[i].sx;
	wd->quad[i].matrix.yx *= wd->quad[i].sx;
	wd->quad[i].matrix.xy *= wd->quad[i].sy;
	wd->quad[i].matrix.yy *= wd->quad[i].sy;

	if (wd->decor->quad[i].align & ALIGN_RIGHT)
	    x0 = wd->quad[i].box.x2 - wd->quad[i].box.x1;
	else
	    x0 = 0.0f;

	if (wd->decor->quad[i].align & ALIGN_BOTTOM)
	    y0 = wd->quad[i].box.y2 - wd->quad[i].box.y1;
	else
	    y0 = 0.0f;

	wd->quad[i].matrix.x0 -=
	    x0 * wd->quad[i].matrix.xx +
	    y0 * wd->quad[i].matrix.xy;

	wd->quad[i].matrix.y0 -=
	    y0 * wd->quad[i].matrix.yy +
	    x0 * wd->quad[i].matrix.yx;

	wd->quad[i].matrix.x0 -=
	    wd->quad[i].box.x1 * wd->quad[i].matrix.xx +
	    wd->quad[i].box.y1 * wd->quad[i].matrix.xy;

	wd->quad[i].matrix.y0 -=
	    wd->quad[i].box.y1 * wd->quad[i].matrix.yy +
	    wd->quad[i].box.x1 * wd->quad[i].matrix.yx;
    }
}

void
DecorWindow::updateDecorationScale ()
{
    int		     x1, y1, x2, y2;
    float            sx, sy;
    int		     i;

    if (!wd)
	return;

    for (i = 0; i < wd->nQuad; i++)
    {
	int x, y;

	computeQuadBox (&wd->decor->quad[i], window->size ().width (),
			window->size ().height (),
			&x1, &y1, &x2, &y2, &sx, &sy);

	x = window->geometry ().x ();
	y = window->geometry ().y ();

	wd->quad[i].box.x1 = x1 + x;
	wd->quad[i].box.y1 = y1 + y;
	wd->quad[i].box.x2 = x2 + x;
	wd->quad[i].box.y2 = y2 + y;
	wd->quad[i].sx     = sx;
	wd->quad[i].sy     = sy;
    }

    setDecorationMatrices ();
}

bool
DecorWindow::checkSize (Decoration *decor)
{
    return (decor->minWidth <= window->size ().width () &&
	    decor->minHeight <= window->size ().height ());
}

int
DecorWindow::shiftX ()
{
    switch (window->sizeHints ().win_gravity) {
	case WestGravity:
	case NorthWestGravity:
	case SouthWestGravity:
	    return window->input ().left;
	case EastGravity:
	case NorthEastGravity:
	case SouthEastGravity:
	    return -window->input ().right;
    }

    return 0;
}

int
DecorWindow::shiftY ()
{
    switch (window->sizeHints ().win_gravity) {
	case NorthGravity:
	case NorthWestGravity:
	case NorthEastGravity:
	    return window->input ().top;
	case SouthGravity:
	case SouthWestGravity:
	case SouthEastGravity:
	    return -window->input ().bottom;
    }

    return 0;
}

static bool
decorOffsetMove (CompWindow *w, XWindowChanges xwc, unsigned int mask)
{
    w->configureXWindow (mask, &xwc);
    return false;
}

bool
DecorWindow::update (bool allowDecoration)
{
    Decoration	     *old, *decoration = NULL;
    bool	     decorate = false;
    CompMatch	     *match;
    int		     moveDx, moveDy;
    int		     oldShiftX = 0;
    int		     oldShiftY  = 0;

    old = (wd) ? wd->decor : NULL;

    switch (window->type ()) {
	case CompWindowTypeDialogMask:
	case CompWindowTypeModalDialogMask:
	case CompWindowTypeUtilMask:
	case CompWindowTypeMenuMask:
	case CompWindowTypeNormalMask:
	    if (window->mwmDecor () & (MwmDecorAll | MwmDecorTitle))
		decorate = window->managed ();
	default:
	    break;
    }

    if (window->overrideRedirect ())
	decorate = false;

    if (decorate)
    {
	match =
	    &dScreen->opt[DECOR_OPTION_DECOR_MATCH].value ().match ();
	if (!match->evaluate (window))
	    decorate = false;
    }

    if (decorate)
    {
	if (decor && checkSize (decor))
	{
	    decoration = decor;
	}
	else
	{
	    if (window->id () == screen->activeWindow ())
		decoration = dScreen->decor[DECOR_ACTIVE];
	    else
		decoration = dScreen->decor[DECOR_NORMAL];
	}
    }
    else
    {
	match = &dScreen->opt[DECOR_OPTION_SHADOW_MATCH].value ().match ();
	if (match->evaluate (window))
	{
	    if (window->region ()->numRects == 1 && !window->alpha ())
		decoration = dScreen->decor[DECOR_BARE];

	    /* no decoration on windows with below state */
	    if (window->state () & CompWindowStateBelowMask)
		decoration = NULL;

	    if (decoration)
	    {
		if (!checkSize (decoration))
		    decoration = NULL;
	    }
	}
    }

    if (!dScreen->dmWin || !allowDecoration)
	decoration = NULL;

    if (decoration == old)
	return false;

    cWindow->damageOutputExtents ();

    if (old)
    {
	oldShiftX = shiftX ();
	oldShiftY = shiftY ();

	WindowDecoration::destroy (wd);

	wd = NULL;
    }

    if (decoration)
    {
	wd = WindowDecoration::create (decoration);
	if (!wd)
	    return false;

	if ((window->state () & MAXIMIZE_STATE) == MAXIMIZE_STATE)
	    window->setWindowFrameExtents (&wd->decor->maxInput);
	else
	    window->setWindowFrameExtents (&wd->decor->input);

	moveDx = shiftX () - oldShiftX;
	moveDy = shiftY () - oldShiftY;

	updateFrame ();
	window->updateWindowOutputExtents ();
	cWindow->damageOutputExtents ();
	updateDecorationScale ();
    }
    else
    {
	CompWindowExtents emptyExtents;
	wd = NULL;

	memset (&emptyExtents, 0, sizeof (CompWindowExtents));

	window->setWindowFrameExtents (&emptyExtents);

	moveDx = -oldShiftX;
	moveDy = -oldShiftY;

	updateFrame ();
    }

    if (window->placed () && !window->overrideRedirect () &&
	(moveDx || moveDy))
    {
	XWindowChanges xwc;
	unsigned int   mask = CWX | CWY;

	memset (&xwc, 0, sizeof (XWindowChanges));

	xwc.x = window->serverGeometry ().x () + moveDx;
	xwc.y = window->serverGeometry ().y () + moveDy;

	if (window->state () & CompWindowStateFullscreenMask)
	    mask &= ~(CWX | CWY);

	if (window->state () & CompWindowStateMaximizedHorzMask)
	    mask &= ~CWX;

	if (window->state () & CompWindowStateMaximizedVertMask)
	    mask &= ~CWY;

	if (window->saveMask () & CWX)
	    window->saveWc ().x += moveDx;

	if (window->saveMask () & CWY)
	    window->saveWc ().y += moveDy;

	if (mask)
	    moveUpdate.start (boost::bind (decorOffsetMove, window, xwc, mask), 0);
    }

    return true;
}

void
DecorWindow::updateFrame ()
{
    if (wd && (window->input ().left || window->input ().right ||
	window->input ().top || window->input ().bottom))
    {
	XRectangle           rects[4];
	int	             x, y, width, height;
	int	             i = 0;
	CompWindow::Geometry server = window->serverGeometry ();
	int                  bw = server.border () * 2;
	CompWindowExtents    input;

	if ((window->state () & MAXIMIZE_STATE) == MAXIMIZE_STATE)
	    input = wd->decor->maxInput;
	else
	    input = wd->decor->input;
	
	x      = window->input ().left - input.left;
	y      = window->input ().top - input.top;
	width  = server.width () + input.left + input.right + bw;
	height = server.height ()+ input.top  + input.bottom + bw;

	if (window->shaded ())
	    height = input.top + input.bottom;

	if (!inputFrame)
	{
	    XSetWindowAttributes attr;

	    attr.event_mask	   = StructureNotifyMask;
	    attr.override_redirect = TRUE;

	    inputFrame = XCreateWindow (screen->dpy (), window->frame (),
					x, y, width, height, 0, CopyFromParent,
					InputOnly, CopyFromParent,
					CWOverrideRedirect | CWEventMask,
					&attr);

	    XGrabButton (screen->dpy (), AnyButton, AnyModifier, inputFrame,
			 TRUE, ButtonPressMask | ButtonReleaseMask |
			 ButtonMotionMask, GrabModeSync, GrabModeSync, None,
			 None);

	    XMapWindow (screen->dpy (), inputFrame);

	    XChangeProperty (screen->dpy (), window->id (),
			     dScreen->inputFrameAtom, XA_WINDOW, 32,
			     PropModeReplace, (unsigned char *) &inputFrame, 1);

	    if (screen->XShape ())
        	XShapeSelectInput (screen->dpy (), inputFrame,
				   ShapeNotifyMask);

	    oldX = 0;
	    oldY = 0;
	    oldWidth  = 0;
	    oldHeight = 0;
	}

	if (x != oldX || y != oldY || width != oldWidth || height != oldHeight)
	{
	    oldX = x;
	    oldY = y;
	    oldWidth  = width;
	    oldHeight = height;

	    XMoveResizeWindow (screen->dpy (), inputFrame, x, y,
			       width, height);
	    XLowerWindow (screen->dpy (), inputFrame);


	    rects[i].x	= 0;
	    rects[i].y	= 0;
	    rects[i].width  = width;
	    rects[i].height = input.top;

	    if (rects[i].width && rects[i].height)
		i++;

	    rects[i].x	= 0;
	    rects[i].y	= input.top;
	    rects[i].width  = input.left;
	    rects[i].height = height - input.top - input.bottom;

	    if (rects[i].width && rects[i].height)
		i++;

	    rects[i].x	= width - input.right;
	    rects[i].y	= input.top;
	    rects[i].width  = input.right;
	    rects[i].height = height - input.top - input.bottom;

	    if (rects[i].width && rects[i].height)
		i++;

	    rects[i].x	= 0;
	    rects[i].y	= height - input.bottom;
	    rects[i].width  = width;
	    rects[i].height = input.bottom;

	    if (rects[i].width && rects[i].height)
		i++;

	    XShapeCombineRectangles (screen->dpy (), inputFrame,
				     ShapeInput, 0, 0, rects, i,
				     ShapeSet, YXBanded);

	    EMPTY_REGION (frameRegion);
	}
    }
    else
    {
	if (inputFrame)
	{
	    XDeleteProperty (screen->dpy (), window->id (),
			     dScreen->inputFrameAtom);
	    XDestroyWindow (screen->dpy (), inputFrame);
	    inputFrame = None;
	    EMPTY_REGION (frameRegion);

	    oldX = 0;
	    oldY = 0;
	    oldWidth  = 0;
	    oldHeight = 0;
	}
    }
}

void
DecorScreen::checkForDm (bool updateWindows)
{
    Atom	  actual;
    int		  result, format;
    unsigned long n, left;
    unsigned char *data;
    Window	  dmWin = None;

    result = XGetWindowProperty (screen->dpy (), screen->root (),
				 supportingDmCheckAtom, 0L, 1L, FALSE,
				 XA_WINDOW, &actual, &format,
				 &n, &left, &data);

    if (result == Success && n && data)
    {
	XWindowAttributes attr;

	memcpy (&dmWin, data, sizeof (Window));
	XFree (data);

	CompScreen::checkForError (screen->dpy ());

	XGetWindowAttributes (screen->dpy (), dmWin, &attr);

	if (CompScreen::checkForError (screen->dpy ()))
	    dmWin = None;
    }

    if (dmWin != this->dmWin)
    {
	int i;

	if (dmWin)
	{
	    for (i = 0; i < DECOR_NUM; i++)
		decor[i] = Decoration::create (screen->root (), decorAtom[i]);
	}
	else
	{
	    for (i = 0; i < DECOR_NUM; i++)
	    {
		if (decor[i])
		{
		    Decoration::release (decor[i]);
		    decor[i] = 0;
		}
	    }

	    foreach (CompWindow *w, screen->windows ())
	    {
		DecorWindow *dw = DecorWindow::get (w);

		if (dw->decor)
		{
		    Decoration::release (dw->decor);
		    dw->decor = 0;
		}
	    }
	}

	this->dmWin = dmWin;

	if (updateWindows)
	{
	    foreach (CompWindow *w, screen->windows ())
		if (w->shaded () || w->isViewable ())
		    DecorWindow::get (w)->update (true);
	}
    }
}

void
DecorWindow::updateFrameRegion (Region region)
{
    window->updateFrameRegion (region);
    if (wd)
    {
	if (REGION_NOT_EMPTY (frameRegion))
	{
	    int x, y;

	    x = window->geometry (). x ();
	    y = window->geometry (). y ();

	    XOffsetRegion (frameRegion,
			   x - window->input ().left,
			   y - window->input ().top);
	    XUnionRegion (frameRegion, region, region);
	    XOffsetRegion (frameRegion,
			   - (x - window->input ().left),
			   - (y - window->input ().top));
	}
	else
	{
	    XUnionRegion (&infiniteRegion, region, region);
	}
    }

}

void
DecorScreen::handleEvent (XEvent *event)
{
    Window  activeWindow = screen->activeWindow ();
    CompWindow *w;

    switch (event->type) {
	case DestroyNotify:
	    w = screen->findWindow (event->xdestroywindow.window);
	    if (w)
	    {
		if (w->id () == dmWin)
		    checkForDm (true);
	    }
	    break;
	case MapRequest:
	    w = screen->findWindow (event->xdestroywindow.window);
	    if (w)
		DecorWindow::get (w)->update (true);
	    break;
	default:
	    if (event->type == cScreen->damageEvent () + XDamageNotify)
	    {
		XDamageNotifyEvent *de = (XDamageNotifyEvent *) event;
		
		foreach (DecorTexture *t, textures)
		{
		    if (t->pixmap == de->drawable)
		    {
			t->GLTexture::damage ();

			foreach (CompWindow *w, screen->windows ())
			{
			    if (w->shaded () || w->mapNum ())
			    {
				DECOR_WINDOW (w);

				if (dw->wd && dw->wd->decor->texture == t)
				    dw->cWindow->damageOutputExtents ();
			    }
			}
			return;
		    }
		}
	    }
	    break;
    }

    screen->handleEvent (event);

    if (screen->activeWindow () != activeWindow)
    {
	w = screen->findWindow (activeWindow);
	if (w)
	    DecorWindow::get (w)->update (true);

	w = screen->findWindow (screen->activeWindow ());
	if (w)
	    DecorWindow::get (w)->update (true);
    }

    switch (event->type) {
	case PropertyNotify:
	    if (event->xproperty.atom == winDecorAtom)
	    {
		w = screen->findWindow (event->xproperty.window);
		if (w)
		{
		    DECOR_WINDOW (w);
		    dw->updateDecoration ();
		    dw->update (true);
		}
	    }
	    else if (event->xproperty.atom == Atoms::mwmHints)
	    {
		w = screen->findWindow (event->xproperty.window);
		if (w)
		    DecorWindow::get (w)->update (true);
	    }
	    else
	    {
		if (event->xproperty.window == screen->root ())
		{
		    if (event->xproperty.atom == supportingDmCheckAtom)
		    {
			checkForDm (true);
		    }
		    else
		    {
			int i;

			for (i = 0; i < DECOR_NUM; i++)
			{
			    if (event->xproperty.atom == decorAtom[i])
			    {
				if (decor[i])
				    Decoration::release (decor[i]);

				decor[i] =
				    Decoration::create (screen->root (),
							decorAtom[i]);

				foreach (CompWindow *w, screen->windows ())
				    DecorWindow::get (w)->update (true);
			    }
			}
		    }
		}
	    }
	    break;
	case ConfigureNotify:
	    w = screen->findTopLevelWindow (event->xconfigure.window);
	    if (w)
	    {
		DECOR_WINDOW (w);
		if (dw->decor)
		{
		    dw->updateFrame ();
		}
	    }
	    break;
	case DestroyNotify:
	    w = screen->findTopLevelWindow (event->xproperty.window);
	    if (w)
	    {
		DECOR_WINDOW (w);
		if (dw->inputFrame &&
		    dw->inputFrame == event->xdestroywindow.window)
		{
		    XDeleteProperty (screen->dpy (), w->id (),
				     inputFrameAtom);
		    dw->inputFrame = None;
		}
	    }
	    break;
	default:
	    if (screen->XShape () && event->type ==
		screen->shapeEvent () + ShapeNotify)
	    {
		w = screen->findWindow (((XShapeEvent *) event)->window);
		if (w)
		    DecorWindow::get (w)->update (true);
		else
		{
		    foreach (w, screen->windows ())
		    {
			DECOR_WINDOW (w);
			if (dw->inputFrame ==
			    ((XShapeEvent *) event)->window)
			{
			    XRectangle *shapeRects = 0;
			    int order, n;

			    EMPTY_REGION (dw->frameRegion);

			    shapeRects =
				XShapeGetRectangles (screen->dpy (),
				    dw->inputFrame, ShapeInput,
				    &n, &order);
			    if (!n || !shapeRects)
				break;

			    for (int i = 0; i < n; i++)
				XUnionRectWithRegion (&shapeRects[i],
						      dw->frameRegion,
						      dw->frameRegion);

			    w->updateFrameRegion ();

			    XFree (shapeRects);
			}
		    }
		}
	    }
	    break;
    }
}

bool
DecorWindow::damageRect (bool initial, BoxPtr rect)
{
    if (initial)
	update (true);

    return cWindow->damageRect (initial, rect);
}

void
DecorWindow::getOutputExtents (CompWindowExtents *output)
{
    window->getOutputExtents (output);

    if (wd)
    {
	CompWindowExtents *e = &wd->decor->output;

	if (e->left > output->left)
	    output->left = e->left;
	if (e->right > output->right)
	    output->right = e->right;
	if (e->top > output->top)
	    output->top = e->top;
	if (e->bottom > output->bottom)
	    output->bottom = e->bottom;
    }
}

CompOption::Vector &
DecorScreen::getOptions ()
{
    return opt;
}
 
bool
DecorScreen::setOption (const char        *name,
			CompOption::Value &value)
{
    CompOption   *o;
    unsigned int index;

    o = CompOption::findOption (opt, name, &index);
    if (!o)
	return false;

    switch (index) {
    case DECOR_OPTION_COMMAND:
	if (o->set(value))
	{

	    if (!dmWin)
		screen->runCommand (o->value ().s ());
	    return true;
	}
	break;
    case DECOR_OPTION_DECOR_MATCH:
    case DECOR_OPTION_SHADOW_MATCH:
	if (o->set(value))
	{
	    foreach (CompWindow *w, screen->windows ())
		DecorWindow::get (w)->update (true);
	}
	break;
    default:
	return CompOption::setOption (*o, value);
    }

    return false;
}

void
DecorWindow::moveNotify (int dx, int dy, bool immediate)
{
    if (wd)
    {
	int		 i;

	for (i = 0; i < wd->nQuad; i++)
	{
	    wd->quad[i].box.x1 += dx;
	    wd->quad[i].box.y1 += dy;
	    wd->quad[i].box.x2 += dx;
	    wd->quad[i].box.y2 += dy;
	}

	setDecorationMatrices ();
    }

    window->moveNotify (dx, dy, immediate);
}

bool
DecorWindow::resizeTimeout ()
{
    update (true);
    return false;
}

void
DecorWindow::resizeNotify (int dx, int dy, int dwidth, int dheight)
{
    /* FIXME: we should not need a timer for calling decorWindowUpdate,
       and only call updateWindowDecorationScale if decorWindowUpdate
       returns FALSE. Unfortunately, decorWindowUpdate may call
       updateWindowOutputExtents, which may call WindowResizeNotify. As
       we never should call a wrapped function that's currently
       processed, we need the timer for the moment. updateWindowOutputExtents
       should be fixed so that it does not emit a resize notification. */
    resizeUpdate.start (boost::bind (&DecorWindow::resizeTimeout, this), 0);
    updateDecorationScale ();

    window->resizeNotify (dx, dy, dwidth, dheight);
}

void
DecorWindow::stateChangeNotify (unsigned int lastState)
{
    if (!update (true))
    {
	if (wd && wd->decor)
	{
	    if ((window->state () & MAXIMIZE_STATE) == MAXIMIZE_STATE)
		window->setWindowFrameExtents (&wd->decor->maxInput);
	    else
		window->setWindowFrameExtents (&wd->decor->input);

	    updateFrame ();
	}
    }

    window->stateChangeNotify (lastState);
}

void
DecorScreen::matchPropertyChanged (CompWindow *w)
{
    DecorWindow::get (w)->update (true);

    screen->matchPropertyChanged (w);
}

static const CompMetadata::OptionInfo decorOptionInfo[] = {
    { "shadow_radius", "float", "<min>0.0</min><max>48.0</max>", 0, 0 },
    { "shadow_opacity", "float", "<min>0.0</min>", 0, 0 },
    { "shadow_color", "color", 0, 0, 0 },
    { "shadow_x_offset", "int", "<min>-16</min><max>16</max>", 0, 0 },
    { "shadow_y_offset", "int", "<min>-16</min><max>16</max>", 0, 0 },
    { "command", "string", 0, 0, 0 },
    { "mipmap", "bool", 0, 0, 0 },
    { "decoration_match", "match", 0, 0, 0 },
    { "shadow_match", "match", 0, 0, 0 }
};

DecorScreen::DecorScreen (CompScreen *s) :
    PrivateHandler<DecorScreen,CompScreen> (s),
    cScreen (CompositeScreen::get (s)),
    textures (),
    dmWin (None),
    opt (DECOR_OPTION_NUM)
{
    if (!decorMetadata->initOptions (decorOptionInfo, DECOR_OPTION_NUM, opt))
    {
	setFailed ();
	return;
    }

    supportingDmCheckAtom =
	XInternAtom (s->dpy (), DECOR_SUPPORTING_DM_CHECK_ATOM_NAME, 0);
    winDecorAtom =
	XInternAtom (s->dpy (), DECOR_WINDOW_ATOM_NAME, 0);
    decorAtom[DECOR_BARE] =
	XInternAtom (s->dpy (), DECOR_BARE_ATOM_NAME, 0);
    decorAtom[DECOR_NORMAL] =
	XInternAtom (s->dpy (), DECOR_NORMAL_ATOM_NAME, 0);
    decorAtom[DECOR_ACTIVE] =
	XInternAtom (s->dpy (), DECOR_ACTIVE_ATOM_NAME, 0);
    inputFrameAtom =
	XInternAtom (s->dpy (), DECOR_INPUT_FRAME_ATOM_NAME, 0);

    for (unsigned int i = 0; i < DECOR_NUM; i++)
	decor[i] = NULL;

    checkForDm (false);

    if (!dmWin)
	s->runCommand (opt[DECOR_OPTION_COMMAND].value ().s ());

    ScreenInterface::setHandler (s);
}

DecorScreen::~DecorScreen ()
{
    for (unsigned int i = 0; i < DECOR_NUM; i++)
	if (decor[i])
	    delete decor[i];
}

DecorWindow::DecorWindow (CompWindow *w) :
    PrivateHandler<DecorWindow,CompWindow> (w),
    window (w),
    gWindow (GLWindow::get (w)),
    cWindow (CompositeWindow::get (w)),
    dScreen (DecorScreen::get (screen)),
    wd (NULL),
    decor (NULL),
    inputFrame (None)
{
    frameRegion = XCreateRegion ();
    if (!frameRegion)
    {
	setFailed ();
	return;
    }

    if (!w->overrideRedirect ())
	updateDecoration ();

    if (w->shaded () || w->isViewable ())
	update (true);

    WindowInterface::setHandler (window);
    CompositeWindowInterface::setHandler (cWindow);
    GLWindowInterface::setHandler (gWindow);
}


DecorWindow::~DecorWindow ()
{
    if (!window->destroyed ())
	update (false);

    if (wd)
	WindowDecoration::destroy (wd);

    if (decor)
	Decoration::release (decor);

    if (frameRegion)
	XDestroyRegion (frameRegion);
}

bool
DecorPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
        !CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) ||
        !CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
	 return false;

    decorMetadata = new CompMetadata (name (), decorOptionInfo,
				      DECOR_OPTION_NUM);
    if (!decorMetadata)
	return false;

    decorMetadata->addFromFile (name ());

    return true;
}

void
DecorPluginVTable::fini ()
{
    delete decorMetadata;
}

CompMetadata *
DecorPluginVTable::getMetadata ()
{
    return decorMetadata;
}

DecorPluginVTable decorVTable;

CompPlugin::VTable *
getCompPluginInfo20080805 (void)
{
    return &decorVTable;
}
