#include "privates.h"

#include <X11/extensions/shape.h>
#include <X11/extensions/Xrandr.h>

CompWindow *lastDamagedWindow = 0;


CompositeDisplay::CompositeDisplay (CompDisplay *d) :
    CompositePrivateHandler<CompositeDisplay, CompDisplay,
			    COMPIZ_COMPOSITE_ABI> (d),
    priv (new PrivateCompositeDisplay (d, this))
{
    int	compositeMajor, compositeMinor;

    if (!compositeMetadata->initDisplayOptions
	 (d, compositeDisplayOptionInfo,
	  COMPOSITE_DISPLAY_OPTION_NUM, priv->opt))
    {
	setFailed ();
	return;
    }

    if (!XQueryExtension (d->dpy (),
			  COMPOSITE_NAME,
			  &priv->compositeOpcode,
			  &priv->compositeEvent,
			  &priv->compositeError))
    {
	compLogMessage (d, "core", CompLogLevelFatal,
		        "No composite extension");
	setFailed ();
	return;
    }

    XCompositeQueryVersion (d->dpy (), &compositeMajor, &compositeMinor);
    if (compositeMajor == 0 && compositeMinor < 2)
    {
	compLogMessage (d, "core", CompLogLevelFatal,
		        "Old composite extension");
	setFailed ();
	return;
    }

    if (!XDamageQueryExtension (d->dpy (), &priv->damageEvent,
	 			&priv->damageError))
    {
	compLogMessage (d, "core", CompLogLevelFatal,
		        "No damage extension");
	setFailed ();
	return;
    }

    if (!XFixesQueryExtension (d->dpy (), &priv->fixesEvent, &priv->fixesError))
    {
	compLogMessage (d, "core", CompLogLevelFatal,
		        "No fixes extension");
	setFailed ();
	return;
    }

    priv->shapeExtension = XShapeQueryExtension (d->dpy (), &priv->shapeEvent,
						 &priv->shapeError);
    priv->randrExtension = XRRQueryExtension (d->dpy (), &priv->randrEvent,
					      &priv->randrError);
}

CompositeDisplay::~CompositeDisplay ()
{
    delete priv;
}

PrivateCompositeDisplay::PrivateCompositeDisplay (CompDisplay      *d,
						  CompositeDisplay *cd) :
    display (d),
    cDisplay (cd),
    opt (COMPOSITE_DISPLAY_OPTION_NUM)
{
    // Wrap handleEvent
    d->add (this);
    DisplayInterface::setHandler (d);
}

PrivateCompositeDisplay::~PrivateCompositeDisplay ()
{
}

void
PrivateCompositeDisplay::handleEvent (XEvent *event)
{
    CompScreen      *s;
    CompWindow      *w;

    switch (event->type) {

	case CreateNotify:
	    s = display->findScreen (event->xcreatewindow.parent);
	    if (s)
	    {
		/* The first time some client asks for the composite
		 * overlay window, the X server creates it, which causes
		 * an errorneous CreateNotify event.  We catch it and
		 * ignore it. */
		if (CompositeScreen::get (s)->overlay () ==
		    event->xcreatewindow.window)
		    return;
	    }
	    break;
	case PropertyNotify:
	    if (event->xproperty.atom == display->atoms ().winOpacity)
	    {
		w = display->findWindow (event->xproperty.window);
		if (w)
		    CompositeWindow::get (w)->updateOpacity ();
	    }
	    else if (event->xproperty.atom == display->atoms ().winBrightness)
	    {
		w = display->findWindow (event->xproperty.window);
		if (w)
		    CompositeWindow::get (w)->updateBrightness ();
	    }
	    else if (event->xproperty.atom == display->atoms ().winSaturation)
	    {
		w = display->findWindow (event->xproperty.window);
		if (w)
		    CompositeWindow::get (w)->updateSaturation ();
	    }
	    break;
	default:
	    if (shapeExtension &&
		event->type == shapeEvent + ShapeNotify)
	    {
		w = display->findWindow (((XShapeEvent *) event)->window);
		if (w)
		{
		    if (w->mapNum ())
		    {
		        CompositeWindow::get (w)->addDamage ();
		    }
		}
	    }
	    break;
    }
	
    display->handleEvent (event);

    switch (event->type) {
	case Expose:
	    foreach (s, display->screens ())
		CompositeScreen::get (s)->priv->
		    handleExposeEvent (&event->xexpose);
	break;
	case ClientMessage:
	    if (event->xclient.message_type == display->atoms ().winOpacity)
	    {
		w = display->findWindow (event->xclient.window);
		if (w && (w->type () & CompWindowTypeDesktopMask) == 0)
		{
		    unsigned short opacity = event->xclient.data.l[0] >> 16;

		    display->setWindowProp32 (w->id (),
			display->atoms ().winOpacity, opacity);
		}
	    }
	    else if (event->xclient.message_type ==
		     display->atoms ().winBrightness)
	    {
		w = display->findWindow (event->xclient.window);
		if (w)
		{
		    unsigned short brightness = event->xclient.data.l[0] >> 16;

		    display->setWindowProp32 (w->id (),
			display->atoms ().winBrightness, brightness);
		}
	    }
	    else if (event->xclient.message_type ==
		     display->atoms ().winSaturation)
	    {
		w = display->findWindow (event->xclient.window);
		if (w)
		{
		    unsigned short saturation = event->xclient.data.l[0] >> 16;

		    display->setWindowProp32 (w->id (),
			display->atoms ().winSaturation, saturation);
		}
	    }
	    break;
	default:
	    if (event->type == damageEvent + XDamageNotify)
	    {
		XDamageNotifyEvent *de = (XDamageNotifyEvent *) event;

		if (lastDamagedWindow && de->drawable == lastDamagedWindow->id ())
		{
		    w = lastDamagedWindow;
		}
		else
		{
		    w = display->findWindow (de->drawable);
		    if (w)
			lastDamagedWindow = w;
		}

		if (w)
		    CompositeWindow::get (w)->processDamage (de);
	    }
	    else if (shapeExtension &&
		     event->type == shapeEvent + ShapeNotify)
	    {
		w = display->findWindow (((XShapeEvent *) event)->window);
		if (w)
		{
		    if (w->mapNum ())
		    {
		        CompositeWindow::get (w)->addDamage ();
		    }
		}
	    }
	    else if (randrExtension &&
		     event->type == randrEvent + RRScreenChangeNotify)
	    {
		XRRScreenChangeNotifyEvent *rre;

		rre = (XRRScreenChangeNotifyEvent *) event;

		s = display->findScreen (rre->root);
		if (s)
		    CompositeScreen::get (s)->detectRefreshRate ();
	    }
	    break;
    }
}
