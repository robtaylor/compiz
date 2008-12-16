/*
 * Copyright © 2008 Dennis Kasprzyk
 * Copyright © 2007 Novell, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Dennis Kasprzyk not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Dennis Kasprzyk makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * DENNIS KASPRZYK DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL DENNIS KASPRZYK BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors: Dennis Kasprzyk <onestone@compiz-fusion.org>
 *          David Reveman <davidr@novell.com>
 */

#include <core/core.h>
#include <composite/composite.h>

#include "privates.h"

class CompositePluginVTable :
    public CompPlugin::VTableForScreenAndWindow<CompositeScreen, CompositeWindow>
{
    public:

	bool init ();
	void fini ();

	PLUGIN_OPTION_HELPER (CompositeScreen)
};

COMPIZ_PLUGIN_20081216 (composite, CompositePluginVTable)

const CompMetadata::OptionInfo
    compositeOptionInfo[COMPOSITE_OPTION_NUM] = {
    { "slow_animations_key", "key", 0,
        CompositeScreen::toggleSlowAnimations, 0 },
    { "detect_refresh_rate", "bool", 0, 0, 0 },
    { "refresh_rate", "int", "<min>1</min>", 0, 0 },
    { "unredirect_fullscreen_windows", "bool", 0, 0, 0 },
    { "force_independent_output_painting", "bool", 0, 0, 0 }
};

CompOption::Vector &
CompositeScreen::getOptions ()
{
    return priv->opt;
}

CompOption *
CompositeScreen::getOption (const char *name)
{
    CompOption *o = CompOption::findOption (priv->opt, name);
    return o;
}

bool
CompositeScreen::setOption (const char        *name,
			    CompOption::Value &value)
{
    CompOption   *o;
    unsigned int index;

    o = CompOption::findOption (priv->opt, name, &index);
    if (!o)
	return false;

    switch (index) {
	case COMPOSITE_OPTION_DETECT_REFRESH_RATE:
	    if (o->set (value))
	    {
		if (value.b ())
		    detectRefreshRate ();

		return true;
	    }
	    break;
	case COMPOSITE_OPTION_REFRESH_RATE:
	    if (priv->opt[COMPOSITE_OPTION_DETECT_REFRESH_RATE].
		value ().b ())
		return false;
	    if (o->set (value))
	    {
		priv->redrawTime = 1000 / o->value ().i ();
		priv->optimalRedrawTime = priv->redrawTime;
		return true;
	    }
	    break;
	default:
	    if (CompOption::setOption (*o, value))
		return true;
	break;
    }

    return false;
}

bool
CompositePluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	return false;

    CompPrivate p;
    p.uval = COMPIZ_COMPOSITE_ABI;
    screen->storeValue ("composite_ABI", p);

    getMetadata ()->addFromOptionInfo (compositeOptionInfo,
				       COMPOSITE_OPTION_NUM);
    getMetadata ()->addFromFile (name ());

    return true;
}

void
CompositePluginVTable::fini ()
{
    screen->eraseValue ("composite_ABI");
}
