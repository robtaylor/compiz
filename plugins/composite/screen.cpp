#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include <sys/time.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/shape.h>

#include "privates.h"

CompositeScreen::CompositeScreen (CompScreen *s) :
    CompositePrivateHandler<CompositeScreen, CompScreen,
			    COMPIZ_COMPOSITE_ABI> (s),
    priv (new PrivateCompositeScreen (s, this))
{
    WRAPABLE_INIT_HND(preparePaint);
    WRAPABLE_INIT_HND(donePaint);
    WRAPABLE_INIT_HND(paint);
    WRAPABLE_INIT_HND(getWindowPaintList);

    priv->tmpRegion = XCreateRegion ();
    if (!priv->tmpRegion)
    {
	setFailed ();
	return;
    }

    priv->damage = XCreateRegion ();
    if (!priv->damage)
    {
	setFailed ();
	return;
    }

    if (!compositeMetadata->initScreenOptions
	 (s, compositeScreenOptionInfo, COMPOSITE_SCREEN_OPTION_NUM, priv->opt))
    {
	setFailed ();
	return;
    }

    priv->makeOutputWindow ();

    detectRefreshRate ();

    priv->slowAnimations = false;

    if (!priv->init ())
    {
        setFailed ();
    }

}

CompositeScreen::~CompositeScreen ()
{
    priv->paintTimer.stop ();

#ifdef USE_COW
    if (useCow)
	XCompositeReleaseOverlayWindow (priv->screen->display()->dpy (),
					priv->screen->root ());
#endif

    if (priv->damage)
	XDestroyRegion (priv->damage);

    delete priv;
}


PrivateCompositeScreen::PrivateCompositeScreen (CompScreen      *s,
						CompositeScreen *cs) :
    screen (s),
    cScreen (cs),
    damageMask (COMPOSITE_SCREEN_DAMAGE_ALL_MASK),
    overlay (None),
    output (None),
    exposeRects (),
    windowPaintOffset (0, 0),
    overlayWindowCount (0),
    nextRedraw (0),
    redrawTime (1000 / 50),
    optimalRedrawTime (1000 / 50),
    frameStatus (0),
    timeMult (1),
    idle (true),
    timeLeft (0),
    slowAnimations (false),
    tmpRegion (NULL),
    active (false),
    pHnd (NULL),
    opt (COMPOSITE_SCREEN_OPTION_NUM)
{
    gettimeofday (&lastRedraw, 0);
    // wrap outputChangeNotify
    ScreenInterface::setHandler (s);
}

PrivateCompositeScreen::~PrivateCompositeScreen ()
{
    if (tmpRegion)
	XDestroyRegion (tmpRegion);
}

bool
PrivateCompositeScreen::init ()
{
    Display              *dpy = screen->display ()->dpy ();
    Window               newCmSnOwner = None;
    Atom                 cmSnAtom = 0;
    Time                 cmSnTimestamp = 0;
    XEvent               event;
    XSetWindowAttributes attr;
    Window               currentCmSnOwner;
    char                 buf[128];

    sprintf (buf, "_NET_WM_CM_S%d", screen->screenNum ());
    cmSnAtom = XInternAtom (dpy, buf, 0);

    currentCmSnOwner = XGetSelectionOwner (dpy, cmSnAtom);

    if (currentCmSnOwner != None)
    {
	if (!replaceCurrentWm)
	{
	    compLogMessage (screen->display (), "composite",
			    CompLogLevelError,
			    "Screen %d on display \"%s\" already "
			    "has a compositing manager; try using the "
			    "--replace option to replace the current "
			    "compositing manager.",
			    screen->screenNum (), DisplayString (dpy));

	    return false;
	}
    }

    attr.override_redirect = TRUE;
    attr.event_mask        = PropertyChangeMask;

    newCmSnOwner =
	XCreateWindow (dpy, XRootWindow (dpy, screen->screenNum ()),
		       -100, -100, 1, 1, 0,
		       CopyFromParent, CopyFromParent,
		       CopyFromParent,
		       CWOverrideRedirect | CWEventMask,
		       &attr);

    XChangeProperty (dpy,
		     newCmSnOwner,
		     screen->display ()->atoms ().wmName,
		     screen->display ()->atoms ().utf8String, 8,
		     PropModeReplace,
		     (unsigned char *) PACKAGE,
		     strlen (PACKAGE));

    XWindowEvent (dpy, newCmSnOwner, PropertyChangeMask, &event);

    cmSnTimestamp = event.xproperty.time;


    XSetSelectionOwner (dpy, cmSnAtom, newCmSnOwner, cmSnTimestamp);

    if (XGetSelectionOwner (dpy, cmSnAtom) != newCmSnOwner)
    {
	compLogMessage (screen->display (), "composite", CompLogLevelError,
			"Could not acquire compositing manager "
			"selection on screen %d display \"%s\"",
			screen->screenNum (), DisplayString (dpy));

	return false;
    }

    return true;
}


bool
CompositeScreen::registerPaintHandler (PaintHandler *pHnd)
{
    Display *dpy = priv->screen->display ()->dpy ();

    if (priv->active)
	return false;

    CompDisplay::checkForError (dpy);

    XCompositeRedirectSubwindows (dpy, priv->screen->root (),
				  CompositeRedirectManual);

    priv->overlayWindowCount = 0;

    if (CompDisplay::checkForError (dpy))
    {
	compLogMessage (priv->screen->display (), "composite",
			CompLogLevelError,
			"Another composite manager is already "
			"running on screen: %d", priv->screen->screenNum ());

	return false;
    }

    foreach (CompWindow *w, priv->screen->windows ())
    {
	CompositeWindow *cw = CompositeWindow::get (w);
	cw->priv->overlayWindow = false;
	cw->priv->redirected = true;
    }

    priv->pHnd = pHnd;
    priv->active = true;

    showOutputWindow ();

    priv->paintTimer.start
	(boost::bind(&CompositeScreen::handlePaintTimeout, this),
	 priv->optimalRedrawTime, MAXSHORT);
    return true;
}

void
CompositeScreen::unregisterPaintHandler ()
{
    Display *dpy = priv->screen->display ()->dpy ();

    foreach (CompWindow *w, priv->screen->windows ())
    {
	CompositeWindow *cw = CompositeWindow::get (w);
	cw->priv->overlayWindow = false;
	cw->priv->redirected = false;
	cw->release ();
    }

    priv->overlayWindowCount = 0;

    XCompositeUnredirectSubwindows (dpy, priv->screen->root (),
				    CompositeRedirectManual);

    priv->pHnd = NULL;
    priv->active = false;
    priv->paintTimer.stop ();

    hideOutputWindow ();
}

bool
CompositeScreen::compositingActive ()
{
    return priv->active;
}

void
CompositeScreen::damageScreen ()
{
    if (priv->damageMask == 0)
	priv->paintTimer.setTimes (priv->paintTimer.minLeft ());

    priv->damageMask |= COMPOSITE_SCREEN_DAMAGE_ALL_MASK;
    priv->damageMask &= ~COMPOSITE_SCREEN_DAMAGE_REGION_MASK;
}

void
CompositeScreen::damageRegion (Region region)
{
    if (priv->damageMask & COMPOSITE_SCREEN_DAMAGE_ALL_MASK)
	return;

    if (priv->damageMask == 0)
	priv->paintTimer.setTimes (priv->paintTimer.minLeft ());

    XUnionRegion (priv->damage, region, priv->damage);

    priv->damageMask |= COMPOSITE_SCREEN_DAMAGE_REGION_MASK;

}

void
CompositeScreen::damagePending ()
{
    if (priv->damageMask == 0)
	priv->paintTimer.setTimes (priv->paintTimer.minLeft ());

    priv->damageMask |= COMPOSITE_SCREEN_DAMAGE_PENDING_MASK;
}

unsigned int
CompositeScreen::damageMask ()
{
    return priv->damageMask;
}

void
CompositeScreen::showOutputWindow ()
{
#ifdef USE_COW
    if (useCow && priv->active)
    {
	Display       *dpy = priv->screen->display ()->dpy ();
	XserverRegion region;

	region = XFixesCreateRegion (dpy, NULL, 0);

	XFixesSetWindowShapeRegion (dpy,
				    priv->output,
				    ShapeBounding,
				    0, 0, 0);
	XFixesSetWindowShapeRegion (dpy,
				    priv->output,
				    ShapeInput,
				    0, 0, region);

	XFixesDestroyRegion (dpy, region);

	damageScreen ();
    }
#endif

}

void
CompositeScreen::hideOutputWindow ()
{
#ifdef USE_COW
    if (useCow)
    {
	Display       *dpy = priv->screen->display ()->dpy ();
	XserverRegion region;

	region = XFixesCreateRegion (dpy, NULL, 0);

	XFixesSetWindowShapeRegion (dpy,
				    priv->output,
				    ShapeBounding,
				    0, 0, region);

	XFixesDestroyRegion (dpy, region);
    }
#endif

}

void
CompositeScreen::updateOutputWindow ()
{
#ifdef USE_COW
    if (useCow && priv->active)
    {
	Display       *dpy = priv->screen->display ()->dpy ();
	XserverRegion region;
	static Region tmpRegion = NULL;

	if (!tmpRegion)
	{
	    tmpRegion = XCreateRegion ();
	    if (!tmpRegion)
		return;
	}

	XSubtractRegion (priv->screen->region (), &emptyRegion, tmpRegion);

	
	for (CompWindowList::reverse_iterator rit =
	     priv->screen->windows ().rbegin ();
	     rit != priv->screen->windows ().rend (); rit++)
	    if (CompositeWindow::get (*rit)->overlayWindow ())
	    {
		XSubtractRegion (tmpRegion, (*rit)->region (), tmpRegion);
	    }
	
	XShapeCombineRegion (dpy, priv->output, ShapeBounding,
			     0, 0, tmpRegion, ShapeSet);


	region = XFixesCreateRegion (dpy, NULL, 0);

	XFixesSetWindowShapeRegion (dpy,
				    priv->output,
				    ShapeInput,
				    0, 0, region);

	XFixesDestroyRegion (dpy, region);
    }
#endif

}

void
PrivateCompositeScreen::makeOutputWindow ()
{
#ifdef USE_COW
    if (useCow)
    {
	overlay = XCompositeGetOverlayWindow (screen->display ()->dpy (),
					      screen->root ());
	output  = overlay;

	XSelectInput (screen->display ()->dpy (), output, ExposureMask);
    }
    else
#endif
	output = overlay = screen->root ();

    cScreen->hideOutputWindow ();
}

Window
CompositeScreen::output ()
{
    return priv->output;
}

Window
CompositeScreen::overlay ()
{
    return priv->overlay;
}

int &
CompositeScreen::overlayWindowCount ()
{
    return priv->overlayWindowCount;
}

void
CompositeScreen::setWindowPaintOffset (int x, int y)
{
    priv->windowPaintOffset = CompPoint (x, y);
}

CompPoint
CompositeScreen::windowPaintOffset ()
{
    return priv->windowPaintOffset;
}

void
CompositeScreen::detectRefreshRate ()
{
    if (!noDetection &&
	priv->opt[COMPOSITE_SCREEN_OPTION_DETECT_REFRESH_RATE].value ().b ())
    {
	CompString        name;
	CompOption::Value value;

	value.set ((int) 0);

	if (priv->screen->display ()->XRandr())
	{
	    XRRScreenConfiguration *config;

	    config  = XRRGetScreenInfo (priv->screen->display ()->dpy (),
					priv->screen->root ());
	    value.set ((int) XRRConfigCurrentRate (config));

	    XRRFreeScreenConfigInfo (config);
	}

	if (value.i () == 0)
	    value.set ((int) 50);

	name = priv->opt[COMPOSITE_SCREEN_OPTION_REFRESH_RATE].name ();

	priv->opt[COMPOSITE_SCREEN_OPTION_DETECT_REFRESH_RATE].value ().set (false);
	core->setOptionForPlugin (priv->screen, "composite", name.c_str (), value);
	priv->opt[COMPOSITE_SCREEN_OPTION_DETECT_REFRESH_RATE].value ().set (true);
    }
    else
    {
	priv->redrawTime = 1000 /
	    priv->opt[COMPOSITE_SCREEN_OPTION_REFRESH_RATE].value ().i ();
	priv->optimalRedrawTime = priv->redrawTime;
    }
}

int
CompositeScreen::getTimeToNextRedraw (struct timeval *tv)
{
    int diff, next;

    diff = TIMEVALDIFF (tv, &priv->lastRedraw);

    /* handle clock rollback */
    if (diff < 0)
	diff = 0;

    if (priv->idle || (priv->pHnd && priv->pHnd->hasVSync ()))
    {
	if (priv->timeMult > 1)
	{
	    priv->frameStatus = -1;
	    priv->redrawTime = priv->optimalRedrawTime;
	    priv->timeMult--;
	}
    }
    else
    {
	if (diff > priv->redrawTime)
	{
	    if (priv->frameStatus > 0)
		priv->frameStatus = 0;

	    next = priv->optimalRedrawTime * (priv->timeMult + 1);
	    if (diff > next)
	    {
		priv->frameStatus--;
		if (priv->frameStatus < -1)
		{
		    priv->timeMult++;
		    priv->redrawTime = diff = next;
		}
	    }
	}
	else if (diff < priv->redrawTime)
	{
	    if (priv->frameStatus < 0)
		priv->frameStatus = 0;

	    if (priv->timeMult > 1)
	    {
		next = priv->optimalRedrawTime * (priv->timeMult - 1);
		if (diff < next)
		{
		    priv->frameStatus++;
		    if (priv->frameStatus > 4)
		    {
			priv->timeMult--;
			priv->redrawTime = next;
		    }
		}
	    }
	}
    }
    if (diff >= priv->redrawTime)
	return 1;

    return priv->redrawTime - diff;
}

bool
CompositeScreen::handlePaintTimeout ()
{
    int         timeDiff;
    struct      timeval tv;

    gettimeofday (&tv, 0);

    if (priv->damageMask)
    {
	if (priv->pHnd)
	    priv->pHnd->prepareDrawing ();

	timeDiff = TIMEVALDIFF (&tv, &priv->lastRedraw);

	/* handle clock rollback */
	if (timeDiff < 0)
	    timeDiff = 0;

	if (priv->slowAnimations)
	{
	    preparePaint (priv->idle ? 2 : (timeDiff * 2) / priv->redrawTime);
	}
	else
	    preparePaint (priv->idle ? priv->redrawTime : timeDiff);

	/* substract top most overlay window region */
	if (priv->overlayWindowCount)
	{
	    for (CompWindowList::reverse_iterator rit =
		 priv->screen->windows ().rbegin ();
	         rit != priv->screen->windows ().rend (); rit++)
	    {
		CompWindow *w = (*rit);

		if (w->destroyed () || w->invisible ())
		    continue;

		if (!CompositeWindow::get (w)->redirected ())
		    XSubtractRegion (priv->damage, w->region (),
				     priv->damage);

		break;
	    }

	    if (priv->damageMask & COMPOSITE_SCREEN_DAMAGE_ALL_MASK)
	    {
		priv->damageMask &= ~COMPOSITE_SCREEN_DAMAGE_ALL_MASK;
		priv->damageMask |= COMPOSITE_SCREEN_DAMAGE_REGION_MASK;
	    }
	}

	if (priv->damageMask & COMPOSITE_SCREEN_DAMAGE_REGION_MASK)
	{
	    XIntersectRegion (priv->damage, priv->screen->region (),
			      priv->tmpRegion);

	    if (priv->tmpRegion->numRects  == 1	  &&
		priv->tmpRegion->rects->x1 == 0	  &&
		priv->tmpRegion->rects->y1 == 0	  &&
		priv->tmpRegion->rects->x2 ==
		    (int) priv->screen->size ().width () &&
		priv->tmpRegion->rects->y2 ==
		    (int) priv->screen->size ().height ())
		damageScreen ();
	}

	EMPTY_REGION (priv->damage);

	int mask = priv->damageMask;
	priv->damageMask = 0;

	CompOutput::ptrList outputs (0);
	
	if (priv->opt[COMPOSITE_SCREEN_OPTION_FORCE_INDEPENDENT].value ().b ()
	    || !priv->screen->hasOverlappingOutputs ())
	{
	    foreach (CompOutput &o, priv->screen->outputDevs ())
		outputs.push_back (&o);
	}
	else
	    outputs.push_back (&priv->screen->fullscreenOutput ());

	paint (outputs, mask);
	
	priv->lastRedraw = tv;

	donePaint ();

	foreach (CompWindow *w, priv->screen->windows ())
	{
	    if (w->destroyed ())
	    {
		CompositeWindow::get (w)->addDamage ();
		break;
	    }
	}

	priv->idle = false;
    }
    else
    {
	priv->idle = true;
    }

    gettimeofday (&tv, 0);

    priv->paintTimer.setTimes (getTimeToNextRedraw (&tv), MAXSHORT);
    return true;
}

void
CompositeScreen::preparePaint (int msSinceLastPaint)
    WRAPABLE_HND_FUNC(preparePaint, msSinceLastPaint)

void
CompositeScreen::donePaint ()
    WRAPABLE_HND_FUNC(donePaint)

void
CompositeScreen::paint (CompOutput::ptrList &outputs,
		        unsigned int        mask)
{
    WRAPABLE_HND_FUNC(paint, outputs, mask)

    if (priv->pHnd)
	priv->pHnd->paintOutputs (outputs, mask, priv->tmpRegion);
}

CompWindowList
CompositeScreen::getWindowPaintList ()
{
    WRAPABLE_HND_FUNC_RETURN (CompWindowList, getWindowPaintList)

    return priv->screen->windows ();
}

void
PrivateCompositeScreen::handleExposeEvent (XExposeEvent *event)
{
    if (output == event->window)
	return;

    exposeRects.push_back (CompRect (event->x, event->x + event->width,
			   event->y, event->y + event->height));

    if (event->count == 0)
    {
	CompRect rect;
	foreach (CompRect rect, exposeRects)
	{
	    cScreen->damageRegion (rect.region ());
	}
	exposeRects.clear ();
    }
}

void
PrivateCompositeScreen::outputChangeNotify ()
{
    screen->outputChangeNotify ();
#ifdef USE_COW
    if (useCow)
	XMoveResizeWindow (screen->display ()->dpy (), overlay, 0, 0,
			   screen->size ().width (), screen->size ().height ());
#endif
    cScreen->damageScreen ();
}

bool
CompositeScreen::toggleSlowAnimations (CompDisplay        *d,
				      CompAction         *action,
				      CompAction::State  state,
				      CompOption::Vector &options)
{
    CompScreen *s;
    Window     xid;

    xid = CompOption::getIntOptionNamed (options, "root");
    s = d->findScreen (xid);
    if (s)
    {
	CompositeScreen *cs = CompositeScreen::get (s);
	if (cs)
	    cs->priv->slowAnimations = !cs->priv->slowAnimations;
    }

    return true;
}

CompositeScreenInterface::CompositeScreenInterface ()
{
    WRAPABLE_INIT_FUNC(preparePaint);
    WRAPABLE_INIT_FUNC(donePaint);
    WRAPABLE_INIT_FUNC(paint);

    WRAPABLE_INIT_FUNC(getWindowPaintList);
}

void
CompositeScreenInterface::preparePaint (int msSinceLastPaint)
    WRAPABLE_DEF_FUNC(preparePaint, msSinceLastPaint)

void
CompositeScreenInterface::donePaint ()
    WRAPABLE_DEF_FUNC(donePaint)

void
CompositeScreenInterface::paint (CompOutput::ptrList &outputs,
				 unsigned int        mask)
    WRAPABLE_DEF_FUNC(paint, outputs, mask)

CompWindowList
CompositeScreenInterface::getWindowPaintList ()
    WRAPABLE_DEF_FUNC_WITH_RETURN(CompWindowList (), getWindowPaintList)
