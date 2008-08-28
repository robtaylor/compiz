#include "privates.h"

CompositeWindow::CompositeWindow (CompWindow *w) :
    CompositePrivateHandler<CompositeWindow, CompWindow,
			    COMPIZ_COMPOSITE_ABI> (w),
    priv (new PrivateCompositeWindow (w, this))
{
    WRAPABLE_INIT_HND(damageRect);

    CompDisplay *d = w->screen ()->display ();

    if (w->attrib ().c_class != InputOnly)
    {
	priv->damage = XDamageCreate (d->dpy (), w->id (),
				      XDamageReportRawRectangles);
    }
    else
    {
	priv->damage = None;
    }

    priv->opacity = OPAQUE;
    if (!(w->type () & CompWindowTypeDesktopMask))
 	priv->opacity = d->getWindowProp32 (w->id (),
					    d->atoms ().winOpacity, OPAQUE);

    priv->brightness = d->getWindowProp32 (w->id (),
					   d->atoms ().winBrightness, BRIGHT);

    priv->saturation = d->getWindowProp32 (w->id (),
					   d->atoms ().winSaturation, COLOR);
	
    if (w->attrib ().map_state == IsViewable)
    {
	priv->damaged   = true;
    }
}

CompositeWindow::~CompositeWindow ()
{

    if (priv->damage)
	XDamageDestroy (priv->screen->display ()->dpy (), priv->damage);

     if (!priv->redirected)
    {
	priv->cScreen->overlayWindowCount ()--;

	if (priv->cScreen->overlayWindowCount () < 1)
	    priv->cScreen->showOutputWindow ();
    }

    release ();

    addDamage ();

    if (lastDamagedWindow == priv->window)
	lastDamagedWindow = NULL;

    delete priv;
}

PrivateCompositeWindow::PrivateCompositeWindow (CompWindow      *w,
						CompositeWindow *cw) :
    window (w),
    cWindow (cw),
    screen (w->screen ()),
    cScreen (CompositeScreen::get (screen)),
    pixmap (None),
    damage (None),
    damaged (false),
    redirected (cScreen->compositingActive ()),
    overlayWindow (false),
    bindFailed (false),
    opacity (OPAQUE),
    brightness (BRIGHT),
    saturation (COLOR),
    damageRects (0),
    sizeDamage (0),
    nDamage (0)
{
    WindowInterface::setHandler (w);
}

PrivateCompositeWindow::~PrivateCompositeWindow ()
{

    if (sizeDamage)
	free (damageRects);
}

bool
CompositeWindow::bind ()
{
    if (!priv->cScreen->compositingActive ())
	return false;

    redirect ();
    if (!priv->pixmap)
    {
	XWindowAttributes attr;

	/* don't try to bind window again if it failed previously */
	if (priv->bindFailed)
	    return false;

	/* We have to grab the server here to make sure that window
	   is mapped when getting the window pixmap */
	XGrabServer (priv->screen->display ()->dpy ());
	XGetWindowAttributes (priv->screen->display ()->dpy (),
			      priv->window->id (), &attr);
	if (attr.map_state != IsViewable)
	{
	    XUngrabServer (priv->screen->display ()->dpy ());
	    priv->bindFailed = true;
	    return false;
	}

	priv->pixmap = XCompositeNameWindowPixmap
	    (priv->screen->display ()->dpy (), priv->window->id ());

	XUngrabServer (priv->screen->display ()->dpy ());
    }
    return true;
}

void
CompositeWindow::release ()
{
    if (priv->pixmap)
    {
	XFreePixmap (priv->screen->display ()->dpy (), priv->pixmap);
	priv->pixmap = None;
    }
}

Pixmap
CompositeWindow::pixmap ()
{
    return priv->pixmap;
}

void
CompositeWindow::redirect ()
{
    if (priv->redirected || !priv->cScreen->compositingActive ())
	return;

    XCompositeRedirectWindow (priv->screen->display ()->dpy (),
			      priv->window->id (),
			      CompositeRedirectManual);

    priv->redirected = true;

    if (priv->overlayWindow)
    {
	priv->cScreen->overlayWindowCount ()--;
	priv->overlayWindow = false;
    }

    if (priv->cScreen->overlayWindowCount () < 1)
	priv->cScreen->showOutputWindow ();
    else
	priv->cScreen->updateOutputWindow ();
}

void
CompositeWindow::unredirect ()
{
    if (!priv->redirected || !priv->cScreen->compositingActive ())
	return;

    release ();

    XCompositeUnredirectWindow (priv->screen->display ()->dpy (),
				priv->window->id (),
				CompositeRedirectManual);

    priv->redirected   = false;
    priv->overlayWindow = true;
    priv->cScreen->overlayWindowCount ()++;

    if (priv->cScreen->overlayWindowCount () > 0)
	priv->cScreen->updateOutputWindow ();
}

bool
CompositeWindow::redirected ()
{
    return priv->redirected;
}

bool
CompositeWindow::overlayWindow ()
{
    return priv->overlayWindow;
}

void
CompositeWindow::damageTransformedRect (float  xScale,
				   float  yScale,
				   float  xTranslate,
				   float  yTranslate,
				   BoxPtr rect)
{
    REGION reg;

    reg.rects    = &reg.extents;
    reg.numRects = 1;

    reg.extents.x1 = (short) (rect->x1 * xScale) - 1;
    reg.extents.y1 = (short) (rect->y1 * yScale) - 1;
    reg.extents.x2 = (short) (rect->x2 * xScale + 0.5f) + 1;
    reg.extents.y2 = (short) (rect->y2 * yScale + 0.5f) + 1;

    reg.extents.x1 += (short) xTranslate;
    reg.extents.y1 += (short) yTranslate;
    reg.extents.x2 += (short) (xTranslate + 0.5f);
    reg.extents.y2 += (short) (yTranslate + 0.5f);

    if (reg.extents.x2 > reg.extents.x1 && reg.extents.y2 > reg.extents.y1)
    {
	XWindowAttributes attrib = priv->window->attrib ();

	reg.extents.x1 += attrib.x + attrib.border_width;
	reg.extents.y1 += attrib.y + attrib.border_width;
	reg.extents.x2 += attrib.x + attrib.border_width;
	reg.extents.y2 += attrib.y + attrib.border_width;

	priv->cScreen->damageRegion (&reg);
    }
}

void
CompositeWindow::damageOutputExtents ()
{
    if (priv->cScreen->damageMask () & COMPOSITE_SCREEN_DAMAGE_ALL_MASK)
	return;

    if (priv->window->shaded () ||
	(priv->window->attrib ().map_state == IsViewable && priv->damaged))
    {
	BoxRec box;

	XWindowAttributes attrib = priv->window->attrib ();
	CompWindowExtents output = priv->window->output ();

	/* top */
	box.x1 = -output.left - attrib.border_width;
	box.y1 = -output.top - attrib.border_width;
	box.x2 = priv->window->width () + output.right - attrib.border_width;
	box.y2 = -attrib.border_width;

	if (box.x1 < box.x2 && box.y1 < box.y2)
	    addDamageRect (&box);

	/* bottom */
	box.y1 = priv->window->height () - attrib.border_width;
	box.y2 = box.y1 + output.bottom - attrib.border_width;

	if (box.x1 < box.x2 && box.y1 < box.y2)
	    addDamageRect (&box);

	/* left */
	box.x1 = -output.left - attrib.border_width;
	box.y1 = -attrib.border_width;
	box.x2 = -attrib.border_width;
	box.y2 = priv->window->height () - attrib.border_width;

	if (box.x1 < box.x2 && box.y1 < box.y2)
	    addDamageRect (&box);

	/* right */
	box.x1 = priv->window->width () - attrib.border_width;
	box.x2 = box.x1 + output.right - attrib.border_width;

	if (box.x1 < box.x2 && box.y1 < box.y2)
	    addDamageRect (&box);
    }
}

void
CompositeWindow::addDamageRect (BoxPtr rect)
{
    REGION region;

    if (priv->cScreen->damageMask () & COMPOSITE_SCREEN_DAMAGE_ALL_MASK)
	return;

    region.extents = *rect;

    if (!damageRect (false, &region.extents))
    {
	XWindowAttributes attrib = priv->window->attrib ();
	region.extents.x1 += attrib.x + attrib.border_width;
	region.extents.y1 += attrib.y + attrib.border_width;
	region.extents.x2 += attrib.x + attrib.border_width;
	region.extents.y2 += attrib.y + attrib.border_width;

	region.rects = &region.extents;
	region.numRects = region.size = 1;

	priv->cScreen->damageRegion (&region);
    }
}

void
CompositeWindow::addDamage (bool force)
{
    if (priv->cScreen->damageMask () & COMPOSITE_SCREEN_DAMAGE_ALL_MASK)
	return;

    if (priv->window->shaded () || force ||
	(priv->window->attrib ().map_state == IsViewable && priv->damaged))
    {
	BoxRec box;

	box.x1 = -priv->window->output ().left -
		 priv->window->attrib ().border_width;
	box.y1 = -priv->window->output ().top -
		 priv->window->attrib ().border_width;
	box.x2 = priv->window->width () +
		 priv->window->output ().right;
	box.y2 = priv->window->height () + priv->window->output ().bottom;

	addDamageRect (&box);
    }
}

bool
CompositeWindow::damaged ()
{
    return priv->damaged;
}

void
CompositeWindow::processDamage (XDamageNotifyEvent *de)
{
    if (priv->window->syncWait ())
    {
	if (priv->nDamage == priv->sizeDamage)
	{
	    priv->damageRects = (XRectangle *) realloc (priv->damageRects,
				 (priv->sizeDamage + 1) *
				 sizeof (XRectangle));
	    priv->sizeDamage += 1;
	}

	priv->damageRects[priv->nDamage].x      = de->area.x;
	priv->damageRects[priv->nDamage].y      = de->area.y;
	priv->damageRects[priv->nDamage].width  = de->area.width;
	priv->damageRects[priv->nDamage].height = de->area.height;
	priv->nDamage++;
    }
    else
    {
        priv->handleDamageRect (this, de->area.x, de->area.y,
				de->area.width, de->area.height);
    }
}

void
PrivateCompositeWindow::handleDamageRect (CompositeWindow *w,
					  int             x,
					  int             y,
					  int             width,
					  int             height)
{
    REGION region;
    bool   initial = false;

    if (!w->priv->redirected)
	return;

    if (!w->priv->damaged)
    {
	w->priv->damaged = initial = true;
    }

    region.extents.x1 = x;
    region.extents.y1 = y;
    region.extents.x2 = region.extents.x1 + width;
    region.extents.y2 = region.extents.y1 + height;

    if (!w->damageRect (initial, &region.extents))
    {
	region.extents.x1 += w->priv->window->attrib ().x +
			     w->priv->window->attrib ().border_width;
	region.extents.y1 += w->priv->window->attrib ().y +
			     w->priv->window->attrib ().border_width;
	region.extents.x2 += w->priv->window->attrib ().x +
			     w->priv->window->attrib ().border_width;
	region.extents.y2 += w->priv->window->attrib ().y +
			     w->priv->window->attrib ().border_width;

	region.rects = &region.extents;
	region.numRects = region.size = 1;

	w->priv->cScreen->damageRegion (&region);
    }

    if (initial)
	w->damageOutputExtents ();
}

void
CompositeWindow::updateOpacity ()
{
    unsigned short opacity;

    if (priv->window->type () & CompWindowTypeDesktopMask)
	return;

    opacity = priv->screen->display ()->getWindowProp32 (priv->window->id (),
	priv->screen->display ()->atoms ().winOpacity, OPAQUE);

    if (opacity != priv->opacity)
    {
	priv->opacity = opacity;
	addDamage ();
    }
}

void
CompositeWindow::updateBrightness ()
{
    unsigned short brightness;

    brightness = priv->screen->display ()->getWindowProp32 (priv->window->id (),
	priv->screen->display ()->atoms ().winBrightness, BRIGHT);

    if (brightness != priv->brightness)
    {
	priv->brightness = brightness;
	addDamage ();
    }
}

void
CompositeWindow::updateSaturation ()
{
    unsigned short saturation;

    saturation = priv->screen->display ()->getWindowProp32 (priv->window->id (),
	priv->screen->display ()->atoms ().winSaturation, COLOR);

    if (saturation != priv->saturation)
    {
	priv->saturation = saturation;
	addDamage ();
    }
}

unsigned short
CompositeWindow::opacity ()
{
    return priv->opacity;
}

unsigned short
CompositeWindow::brightness ()
{
    return priv->brightness;
}

unsigned short
CompositeWindow::saturation ()
{
    return priv->saturation;
}

bool
CompositeWindow::damageRect (bool       initial,
 			     BoxPtr     rect)
{
    WRAPABLE_HND_FUNC_RETURN(bool, damageRect, initial, rect)
    return false;
}

void
PrivateCompositeWindow::windowNotify (CompWindowNotify n)
{
    switch (n)
    {
	case CompWindowNotifyMap:
	    bindFailed = false;
	    damaged = false;
	    break;
	case CompWindowNotifyUnmap:
	    cWindow->addDamage (true);
	    cWindow->release ();

	    if (!redirected && cScreen->compositingActive ())
		cWindow->redirect ();
	    break;
	case CompWindowNotifyRestack:
	case CompWindowNotifyHide:
	case CompWindowNotifyShow:
	case CompWindowNotifyAliveChanged:
	    cWindow->addDamage (true);
	    break;
	case CompWindowNotifySyncAlarm:
	{
	    XRectangle *rects;

	    rects   = damageRects;
	    while (nDamage--)
	    {
		PrivateCompositeWindow::handleDamageRect (cWindow,
							  rects[nDamage].x,
							  rects[nDamage].y,
							  rects[nDamage].width,
							  rects[nDamage].height);
	    }
	    break;
	}
	default:
	    break;
	
    }

    window->windowNotify (n);
}

void
PrivateCompositeWindow::resizeNotify (int dx, int dy, int dwidth, int dheight)
{
    window->resizeNotify (dx, dy, dwidth, dheight);

    Pixmap pixmap = None;

    if (window->shaded () ||
	(window->attrib ().map_state == IsViewable && damaged))
    {
	REGION region;

	XWindowAttributes attrib = window->attrib ();
	region.extents.x1 = attrib.x - window->output ().left - dx;
	region.extents.y1 = attrib.y - window->output ().top - dy;
	region.extents.x2 = attrib.x + window->width () +
			    window->output ().right - dx - dwidth;
	region.extents.y2 = attrib.y + window->height () +
			    window->output ().bottom - dy - dheight;

	region.rects = &region.extents;
	region.numRects = region.size = 1;

	cScreen->damageRegion (&region);
    }

    if (window->mapNum () && redirected)
    {
	unsigned int actualWidth, actualHeight, ui;
	Window	     root;
	Status	     result;
	int	     i;

	pixmap = XCompositeNameWindowPixmap (screen->display ()->dpy (),
					     window->id ());
	result = XGetGeometry (screen->display ()->dpy (), pixmap, &root,
			       &i, &i, &actualWidth, &actualHeight,
			       &ui, &ui);

	if (!result || (int) actualWidth != window->width () ||
	    (int) actualHeight != window->height ())
	{
	    XFreePixmap (screen->display ()->dpy (), pixmap);
	    return;
	}
    }

    cWindow->addDamage ();

    cWindow->release ();
    this->pixmap = pixmap;
}

void
PrivateCompositeWindow::moveNotify (int dx, int dy, bool now)
{
    if (window->shaded () ||
	(window->attrib ().map_state == IsViewable && damaged))
    {
	REGION region;

	XWindowAttributes attrib = window->attrib ();
	region.extents.x1 = attrib.x - window->output ().left - dx;
	region.extents.y1 = attrib.y - window->output ().top - dy;
	region.extents.x2 = attrib.x + window->width () +
			    window->output ().right - dx;
	region.extents.y2 = attrib.y + window->height () +
			    window->output ().bottom - dy;

	

	region.rects = &region.extents;
	region.numRects = region.size = 1;

	cScreen->damageRegion (&region);
    }
    cWindow->addDamage ();

    window->moveNotify (dx, dy, now);
}

CompositeWindowInterface::CompositeWindowInterface ()
{
    WRAPABLE_INIT_FUNC(damageRect);
}

bool
CompositeWindowInterface::damageRect (bool initial, BoxPtr rect)
    WRAPABLE_DEF_FUNC_RETURN(damageRect, initial, rect)










