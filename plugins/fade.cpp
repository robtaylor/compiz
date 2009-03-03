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

#include "fade.h"
#include <core/atoms.h>

COMPIZ_PLUGIN_20081216 (fade, FadePluginVTable);

void
FadeScreen::handleEvent (XEvent *event)
{
    CompWindow *w;

    if (event->type == screen->xkbEvent ())
    {
	XkbAnyEvent *xkbEvent = (XkbAnyEvent *) event;

	if (xkbEvent->xkb_type == XkbBellNotify)
	{
	    XkbBellNotifyEvent *xkbBellEvent = (XkbBellNotifyEvent *) xkbEvent;

	    w = screen->findWindow (xkbBellEvent->window);
	    if (!w)
		w = screen->findWindow (screen->activeWindow ());

	    if (w)
	    {
		if (opt[FADE_OPTION_VISUAL_BELL].value ().b ())
		{
		    if (opt[FADE_OPTION_FULLSCREEN_VISUAL_BELL].value ().b ())
		    {
			foreach (CompWindow *w, screen->windows ())
			{
			    if (w->destroyed ())
				continue;

			    if (!w->isViewable ())
				continue;

			    FadeWindow::get (w)->dim (false);
			}

			cScreen->damageScreen ();
		    }
		    else
		    {
			FadeWindow::get (w)->dim (true);
		    }
		}
	    }
	}
    }

    screen->handleEvent (event);

    if (event->type == PropertyNotify &&
	event->xproperty.atom == Atoms::winState)
    {
	w = screen->findWindow (event->xproperty.window);
	if (w && w->isViewable ())
	{
	    if (w->state () & CompWindowStateDisplayModalMask)
		FadeWindow::get (w)->addDisplayModal ();
	    else
		FadeWindow::get (w)->removeDisplayModal ();
	}
    }
}

void
FadeScreen::preparePaint (int msSinceLastPaint)
{
    int          steps;
    unsigned int mode = opt[FADE_OPTION_FADE_MODE].value ().i ();

    foreach (CompWindow *w, screen->windows ())
	FadeWindow::get (w)->paintStep (mode, msSinceLastPaint, steps);

    cScreen->preparePaint (msSinceLastPaint);
}

void
FadeWindow::dim (bool damage)
{
    if (!cWindow->damaged ())
	return;

    brightness = cWindow->brightness () / 2;

    if (damage)
	cWindow->addDamage ();
}

void
FadeWindow::addDisplayModal ()
{
    if (!(window->state () & CompWindowStateDisplayModalMask))
	return;

    if (dModal)
	return;

    dModal = true;

    fScreen->displayModals++;
    if (fScreen->displayModals == 1)
	fScreen->cScreen->damageScreen ();
}

void
FadeWindow::removeDisplayModal ()
{
    if (!dModal)
	return;

    dModal = false;

    fScreen->displayModals--;
    if (fScreen->displayModals == 0)
	fScreen->cScreen->damageScreen ();
}

void
FadeWindow::paintStep (unsigned int mode,
		       int          msSinceLastPaint,
		       int          step)
{
    if (mode == FADE_MODE_CONSTANTSPEED)
    {
	steps    = step;
	fadeTime = 0;
    }
    else if (mode == FADE_MODE_CONSTANTTIME)
    {
	if (fadeTime)
	{
	    steps     = 1;
	    fadeTime -= msSinceLastPaint;
	    if (fadeTime < 0)
		fadeTime = 0;
	}
	else
	{
	    steps = 0;
	}
    }
}

void
FadeWindow::windowNotify (CompWindowNotify n)
{
    window->windowNotify (n);

    if (n == CompWindowNotifyAliveChanged)
	cWindow->addDamage ();
}

bool
FadeWindow::glPaint (const GLWindowPaintAttrib& attrib,
		     const GLMatrix&            transform,
		     const CompRegion&          region,
		     unsigned int               mask)
{
    if (!GL::canDoSlightlySaturated)
	saturation = attrib.saturation;

    if (window->alive ()                &&
	opacity    == attrib.opacity    &&
	brightness == attrib.brightness &&
	saturation == attrib.saturation &&
	!fScreen->displayModals)
    {
	return gWindow->glPaint (attrib, transform, region, mask);
    }

    GLWindowPaintAttrib fAttrib = attrib;
    int                 mode;

    mode = fScreen->opt[FADE_OPTION_FADE_MODE].value ().i ();

    if (!window->alive () &&
	fScreen->opt[FADE_OPTION_DIM_UNRESPONSIVE].value ().b ())
    {
	GLuint value;

	value = fScreen->opt[FADE_OPTION_UNRESPONSIVE_BRIGHTNESS].value ().i ();
	if (value != 100)
	    fAttrib.brightness = fAttrib.brightness * value / 100;

	value = fScreen->opt[FADE_OPTION_UNRESPONSIVE_SATURATION].value ().i ();
	if (value != 100 && GL::canDoSlightlySaturated)
	    fAttrib.saturation = fAttrib.saturation * value / 100;
    }
    else if (fScreen->displayModals && !dModal)
    {
	fAttrib.brightness = 0xa8a8;
	fAttrib.saturation = 0;
    }

    if (mode == FADE_MODE_CONSTANTTIME)
    {
	if (fAttrib.opacity    != targetOpacity    ||
	    fAttrib.brightness != targetBrightness ||
	    fAttrib.saturation != targetSaturation)
	{
	    fadeTime = fScreen->opt[FADE_OPTION_FADE_TIME].value ().i ();
	    steps    = 1;

	    opacityDiff    = fAttrib.opacity - opacity;
	    brightnessDiff = fAttrib.brightness - brightness;
	    saturationDiff = fAttrib.saturation - saturation;

	    targetOpacity    = fAttrib.opacity;
	    targetBrightness = fAttrib.brightness;
	    targetSaturation = fAttrib.saturation;
	}
    }

    if (steps)
    {
	GLint newOpacity = OPAQUE;
	GLint newBrightness = BRIGHT;
	GLint newSaturation = COLOR;

	if (mode == FADE_MODE_CONSTANTSPEED)
	{
	    newOpacity = opacity;
	    if (fAttrib.opacity > opacity)
		newOpacity = MIN (opacity + steps, fAttrib.opacity);
	    else if (fAttrib.opacity < opacity)
		opacity = MAX (opacity - steps, fAttrib.opacity);

	    newBrightness = brightness;
	    if (fAttrib.brightness > brightness)
		newBrightness = MIN (brightness + (steps / 12),
				     fAttrib.brightness);
	    else if (fAttrib.brightness < brightness)
		newBrightness = MAX (brightness - (steps / 12),
				     fAttrib.brightness);

	    newSaturation = saturation;
	    if (fAttrib.saturation > saturation)
		saturation = MIN (saturation + (steps / 6),
				  fAttrib.saturation);
	    else if (fAttrib.saturation < saturation)
		saturation = MAX (saturation - (steps / 6),
				  fAttrib.saturation);
	}
	else if (mode == FADE_MODE_CONSTANTTIME)
	{
	    int opt           = FADE_OPTION_FADE_TIME;
	    int totalFadeTime = fScreen->opt[opt].value ().i ();

	    if (totalFadeTime == 0)
		totalFadeTime = fadeTime;

	    newOpacity = fAttrib.opacity -
		            (opacityDiff * fadeTime / totalFadeTime);
	    newBrightness = fAttrib.brightness -
		            (brightnessDiff * fadeTime / totalFadeTime);
	    newSaturation = fAttrib.saturation -
		            (saturationDiff * fadeTime / totalFadeTime);
	}

	steps = 0;

	if (newOpacity > 0)
	{
	    opacity    = newOpacity;
	    brightness = newBrightness;
	    saturation = newSaturation;

	    if (newOpacity    != fAttrib.opacity    ||
		newBrightness != fAttrib.brightness ||
		newSaturation != fAttrib.saturation)
	    {
		cWindow->addDamage ();
	    }
	}
	else
	{
	    opacity = 0;
	}
    }

    fAttrib.opacity    = opacity;
    fAttrib.brightness = brightness;
    fAttrib.saturation = saturation;

    return gWindow->glPaint (fAttrib, transform, region, mask);
}

static const CompMetadata::OptionInfo fadeOptionInfo[] = {
    { "fade_mode", "int", RESTOSTRING (0, FADE_MODE_MAX), 0, 0 },
    { "fade_speed", "float", "<min>0.1</min>", 0, 0 },
    { "fade_time", "int", "<min>1</min>", 0, 0 },
    { "window_match", "match", "<helper>true</helper>", 0, 0 },
    { "visual_bell", "bool", 0, 0, 0 },
    { "fullscreen_visual_bell", "bool", 0, 0, 0 },
    { "minimize_open_close", "bool", 0, 0, 0 },
    { "dim_unresponsive", "bool", 0, 0, 0 },
    { "unresponsive_brightness", "int", "<min>0</min><max>100</max>", 0, 0 },
    { "unresponsive_saturation", "int", "<min>0</min><max>100</max>", 0, 0 }
};

FadeScreen::FadeScreen (CompScreen *s) :
    PrivateHandler<FadeScreen, CompScreen> (s),
    cScreen (CompositeScreen::get (s)),
    displayModals (0)
{
    if (!fadeVTable->getMetadata ()->initOptions (fadeOptionInfo,
						  FADE_OPTION_NUM, opt))
    {
	setFailed ();
	return;
    }

    fadeTime = 1000.0f / opt[FADE_OPTION_FADE_SPEED].value ().f ();

    ScreenInterface::setHandler (screen);
    CompositeScreenInterface::setHandler (cScreen);
}

CompOption::Vector&
FadeScreen::getOptions ()
{
    return opt;
}

bool
FadeScreen::setOption (const char         *name,
		       CompOption::Value& value)
{
    CompOption   *o;
    unsigned int index;

    o = CompOption::findOption (opt, name, &index);
    if (!o)
	return false;

    switch (index) {
    case FADE_OPTION_FADE_SPEED:
	if (o->set (value))
	{
	    fadeTime = 1000.0f / o->value ().f ();
	    return true;
	}
	break;
    case FADE_OPTION_WINDOW_MATCH:
	if (o->set (value))
	{
	    cScreen->damageScreen ();
	    return true;
	}
	break;
    case FADE_OPTION_DIM_UNRESPONSIVE:
	if (o->set (value))
	{
	    foreach (CompWindow *w, screen->windows ())
		w->windowNotifySetEnabled (FadeWindow::get (w), value.b ());
	    return true;
	}
	break;
    default:
	return CompOption::setOption (*o, value);
	break;
    }

    return false;
}

FadeWindow::FadeWindow (CompWindow *w) :
    PrivateHandler<FadeWindow, CompWindow> (w),
    fScreen (FadeScreen::get (screen)),
    window (w),
    cWindow (CompositeWindow::get (w)),
    gWindow (GLWindow::get (w)),
    opacity (cWindow->opacity ()),
    brightness (cWindow->brightness ()),
    saturation (cWindow->saturation ()),
    targetOpacity (opacity),
    targetBrightness (brightness),
    targetSaturation (saturation),
    dModal (false),
    steps (0),
    fadeTime (0),
    opacityDiff (0),
    brightnessDiff (0),
    saturationDiff (0)
{
    if (window->isViewable ())
	addDisplayModal ();

    WindowInterface::setHandler (window, false);
    GLWindowInterface::setHandler (gWindow);

    if (fScreen->opt[FADE_OPTION_DIM_UNRESPONSIVE].value ().b ())
	window->windowNotifySetEnabled (this, true);
}

FadeWindow::~FadeWindow ()
{
    removeDisplayModal ();
}

bool
FadePluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION)           |
	!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) |
	!CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
    {
	return false;
    }

    getMetadata ()->addFromOptionInfo (fadeOptionInfo, FADE_OPTION_NUM);
    getMetadata ()->addFromFile (name ());

    return true;
}
