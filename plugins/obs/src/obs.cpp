/*
 * Copyright © 2008 Danny Baumann
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Danny Baumann not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Danny Baumann makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * DANNY BAUMANN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL DENNIS KASPRZYK BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Danny Baumann <dannybaumann@web.de>
 */

#include "obs.h"

COMPIZ_PLUGIN_20081216 (obs, ObsPluginVTable);

void
ObsWindow::changePaintModifier (unsigned int modifier,
				int          direction)
{
    int value, step;

    if (window->overrideRedirect ())
	return;

    if (modifier == MODIFIER_OPACITY &&
	(window->type () & CompWindowTypeDesktopMask))
    {
	return;
    }

    step   = oScreen->stepOptions[modifier]->value ().i ();
    value  = customFactor[modifier] + (step * direction);

    value = MAX (MIN (value, 100), step);

    if (value != customFactor[modifier])
    {
	customFactor[modifier] = value;
	modifierChanged (modifier);
    }
}

void
ObsWindow::updatePaintModifier (unsigned int modifier)
{
    int lastFactor;

    lastFactor = customFactor[modifier];

    if (modifier == MODIFIER_OPACITY &&
	(window->type ()& CompWindowTypeDesktopMask))
    {
	customFactor[modifier] = 100;
	matchFactor[modifier]  = 100;
    }
    else
    {
	int                       i, min, lastMatchFactor;
	CompOption::Value::Vector *matches, *values;

	matches = &oScreen->matchOptions[modifier]->value ().list ();
	values  = &oScreen->valueOptions[modifier]->value ().list ();
	min     = MIN (matches->size (), values->size ());

	lastMatchFactor       = matchFactor[modifier];
	matchFactor[modifier] = 100;

	for (i = 0; i < min; i++)
	{
	    if (matches->at (i).match ().evaluate (window))
	    {
		matchFactor[modifier] = values->at (i).i ();
		break;
	    }
	}

	if (customFactor[modifier] == lastMatchFactor)
	    customFactor[modifier] = matchFactor[modifier];
    }

    if (customFactor[modifier] != lastFactor)
	modifierChanged (modifier);
}

void
ObsWindow::modifierChanged (unsigned int modifier)
{
    bool hasCustom = false;

    if (modifier == MODIFIER_OPACITY)
	gWindow->glPaintSetEnabled (this, customFactor[modifier] != 100);

    for (unsigned int i = 0; i < MODIFIER_COUNT; i++)
	if (customFactor[i] != 100)
	{
	    hasCustom = true;
	    break;
	}

    gWindow->glDrawSetEnabled (this, hasCustom);
    cWindow->addDamage ();
}

static bool
alterPaintModifier (CompAction          *action,
		    CompAction::State   state,
		    CompOption::Vector& options,
		    unsigned int        modifier,
		    int                 direction)
{
    CompWindow *w;
    Window     xid;

    xid = CompOption::getIntOptionNamed (options, "window", 0);
    w   = screen->findTopLevelWindow (xid);

    if (w)
	ObsWindow::get (w)->changePaintModifier (modifier, direction);

    return true;
}

bool
ObsWindow::glPaint (const GLWindowPaintAttrib& attrib,
		    const GLMatrix&            transform,
		    const CompRegion&          region,
		    unsigned int               mask)
{
    mask |= PAINT_WINDOW_TRANSLUCENT_MASK;

    return gWindow->glPaint (attrib, transform, region, mask);
}

/* Note: Normally plugins should wrap into PaintWindow to modify opacity,
	 brightness and saturation. As some plugins bypass paintWindow when
	 they draw windows and our custom values always need to be applied,
	 we wrap into DrawWindow here */

bool
ObsWindow::glDraw (const GLMatrix&     transform,
		   GLFragment::Attrib& attrib,
		   const CompRegion&   region,
		   unsigned int        mask)
{
    int factor;

    factor = customFactor[MODIFIER_OPACITY];
    if (factor != 100)
    {
	attrib.setOpacity (factor * attrib.getOpacity () / 100);
	mask |= PAINT_WINDOW_TRANSLUCENT_MASK;
    }

    factor = customFactor[MODIFIER_BRIGHTNESS];
    if (factor != 100)
	attrib.setBrightness (factor * attrib.getBrightness () / 100);

    factor = customFactor[MODIFIER_SATURATION];
    if (factor != 100)
	attrib.setSaturation (factor * attrib.getSaturation () / 100);

    return gWindow->glDraw (transform, attrib, region, mask);
}

void
ObsScreen::matchExpHandlerChanged ()
{
    screen->matchExpHandlerChanged ();

    /* match options are up to date after the call to matchExpHandlerChanged */
    foreach (CompWindow *w, screen->windows ())
    {
	for (unsigned int i = 0; i < MODIFIER_COUNT; i++)
	    ObsWindow::get (w)->updatePaintModifier (i);
    }
}

void
ObsScreen::matchPropertyChanged (CompWindow  *w)
{
    unsigned int i;

    for (i = 0; i < MODIFIER_COUNT; i++)
	ObsWindow::get (w)->updatePaintModifier (i);

    screen->matchPropertyChanged (w);
}

#define MODIFIERBIND(modifier, direction) \
    boost::bind (alterPaintModifier, _1, _2, _3, modifier, direction)

static const CompMetadata::OptionInfo obsOptionInfo[] = {
    { "opacity_increase_key", "key", 0,
      MODIFIERBIND (MODIFIER_OPACITY, 1), 0 },
    { "opacity_increase_button", "button", 0,
      MODIFIERBIND (MODIFIER_OPACITY, 1), 0 },
    { "opacity_decrease_key", "key", 0,
      MODIFIERBIND (MODIFIER_OPACITY, -1), 0 },
    { "opacity_decrease_button", "button", 0,
      MODIFIERBIND (MODIFIER_OPACITY, -1), 0 },
    { "saturation_increase_key", "key", 0,
      MODIFIERBIND (MODIFIER_SATURATION, 1), 0 },
    { "saturation_increase_button", "button", 0,
       MODIFIERBIND (MODIFIER_SATURATION, 1), 0 },
    { "saturation_decrease_key", "key", 0,
      MODIFIERBIND (MODIFIER_SATURATION, -1), 0 },
    { "saturation_decrease_button", "button", 0,
      MODIFIERBIND (MODIFIER_SATURATION, -1), 0 },
    { "brightness_increase_key", "key", 0,
      MODIFIERBIND (MODIFIER_BRIGHTNESS, 1), 0 },
    { "brightness_increase_button", "button", 0,
      MODIFIERBIND (MODIFIER_BRIGHTNESS, 1), 0 },
    { "brightness_decrease_key", "key", 0,
      MODIFIERBIND (MODIFIER_BRIGHTNESS, -1), 0 },
    { "brightness_decrease_button", "button", 0,
      MODIFIERBIND (MODIFIER_BRIGHTNESS, -1), 0 },
    { "opacity_step", "int", 0, 0, 0 },
    { "saturation_step", "int", 0, 0, 0 },
    { "brightness_step", "int", 0, 0, 0 },
    { "opacity_matches", "list", "<type>match</type>", 0, 0 },
    { "opacity_values", "list",
      "<type>int</type><min>0</min><max>100</max>", 0, 0 },
    { "saturation_matches", "list", "<type>match</type>", 0, 0 },
    { "saturation_values", "list",
      "<type>int</type><min>0</min><max>100</max>", 0, 0 },
    { "brightness_matches", "list", "<type>match</type>", 0, 0 },
    { "brightness_values", "list",
      "<type>int</type><min>0</min><max>100</max>", 0, 0 }
};

ObsScreen::ObsScreen (CompScreen *s) :
    PluginClassHandler<ObsScreen, CompScreen> (s),
    opt (OBS_OPTION_NUM)
{
    unsigned int mod;

    if (!obsVTable->getMetadata ()->initOptions (obsOptionInfo,
						 OBS_OPTION_NUM, opt))
    {
	setFailed ();
	return;
    }

    ScreenInterface::setHandler (screen);

    mod = MODIFIER_OPACITY;
    stepOptions[mod]  = &opt[OBS_OPTION_OPACITY_STEP];
    matchOptions[mod] = &opt[OBS_OPTION_OPACITY_MATCHES];
    valueOptions[mod] = &opt[OBS_OPTION_OPACITY_VALUES];

    mod = MODIFIER_SATURATION;
    stepOptions[mod]  = &opt[OBS_OPTION_SATURATION_STEP];
    matchOptions[mod] = &opt[OBS_OPTION_SATURATION_MATCHES];
    valueOptions[mod] = &opt[OBS_OPTION_SATURATION_VALUES];

    mod = MODIFIER_BRIGHTNESS;
    stepOptions[mod]  = &opt[OBS_OPTION_BRIGHTNESS_STEP];
    matchOptions[mod] = &opt[OBS_OPTION_BRIGHTNESS_MATCHES];
    valueOptions[mod] = &opt[OBS_OPTION_BRIGHTNESS_VALUES];
}

CompOption::Vector &
ObsScreen::getOptions ()
{
    return opt;
}

bool
ObsScreen::setOption (const char         *name,
		      CompOption::Value& value)
{
   CompOption   *o;
  unsigned  int i;

    o = CompOption::findOption (opt, name, NULL);
    if (!o)
        return false;

    for (i = 0; i < MODIFIER_COUNT; i++)
    {
	if (o == matchOptions[i])
	{
	    if (o->set (value))
	    {
		foreach (CompOption::Value &item, o->value ().list ())
		    item.match ().update ();

		foreach (CompWindow *w, screen->windows ())
		    ObsWindow::get (w)->updatePaintModifier (i);

		return true;
	    }
	}
	else if (o == valueOptions[i])
	{
	    if (o->set (value))
	    {
		foreach (CompWindow *w, screen->windows ())
		    ObsWindow::get (w)->updatePaintModifier (i);

		return true;
	    }
	}
    }

    return CompOption::setOption (*o, value);
}

ObsWindow::ObsWindow (CompWindow *w) :
    PluginClassHandler<ObsWindow, CompWindow> (w),
    window (w),
    cWindow (CompositeWindow::get (w)),
    gWindow (GLWindow::get (w)),
    oScreen (ObsScreen::get (screen))
{
    GLWindowInterface::setHandler (gWindow, false);

    for (unsigned int i = 0; i < MODIFIER_COUNT; i++)
    {
	customFactor[i] = 100;
	matchFactor[i]  = 100;

	updatePaintModifier (i);
    }
}

bool
ObsPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
        !CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) ||
        !CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
    {
	 return false;
    }

    getMetadata ()->addFromOptionInfo (obsOptionInfo, OBS_OPTION_NUM);
    getMetadata ()->addFromFile (name ());

    return true;
}
