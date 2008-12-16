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
#include <core/privatehandler.h>
#include "privates.h"

const CompMetadata::OptionInfo glOptionInfo[GL_OPTION_NUM] = {
    { "texture_filter", "int", RESTOSTRING (0, 2), 0, 0 },
    { "lighting", "bool", 0, 0, 0 },
    { "sync_to_vblank", "bool", 0, 0, 0 },
    { "texture_compression", "bool", 0, 0, 0 },
};

CompOption::Vector &
GLScreen::getOptions ()
{
    return priv->opt;
}

bool
GLScreen::setOption (const char        *name,
		     CompOption::Value &value)
{
    CompOption   *o;
    unsigned int index;

    o = CompOption::findOption (priv->opt, name, &index);
    if (!o)
	return false;

    switch (index) {
	case GL_OPTION_TEXTURE_FILTER:
	    if (o->set (value))
	    {
		priv->cScreen->damageScreen ();

		if (!o->value ().i ())
		    priv->textureFilter = GL_NEAREST;
		else
		    priv->textureFilter = GL_LINEAR;

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

class OpenglPluginVTable :
    public CompPlugin::VTableForScreenAndWindow<GLScreen, GLWindow>
{
    public:

	bool init ();
	void fini ();

	PLUGIN_OPTION_HELPER (GLScreen)
};

COMPIZ_PLUGIN_20081216 (opengl, OpenglPluginVTable)

bool
OpenglPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
        !CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI))
	return false;

    CompPrivate p;
    p.uval = COMPIZ_OPENGL_ABI;
    screen->storeValue ("opengl_ABI", p);

    getMetadata ()->addFromOptionInfo (glOptionInfo, GL_OPTION_NUM);
    getMetadata ()->addFromFile (name ());

    return true;
}

void
OpenglPluginVTable::fini ()
{
    screen->eraseValue ("opengl_ABI");
}
