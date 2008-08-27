#include <compiz-core.h>
#include <compprivatehandler.h>
#include "privates.h"

const CompMetadata::OptionInfo glDisplayOptionInfo[GL_DISPLAY_OPTION_NUM] = {
    { "texture_filter", "int", RESTOSTRING (0, 2), 0, 0 },
};

const CompMetadata::OptionInfo glScreenOptionInfo[GL_SCREEN_OPTION_NUM] = {
    { "lighting", "bool", 0, 0, 0 },
    { "sync_to_vblank", "bool", 0, 0, 0 },
    { "texture_compression", "bool", 0, 0, 0 },
};

CompOption::Vector &
GLDisplay::getOptions ()
{
    return priv->opt;
}

CompOption::Vector &
GLScreen::getOptions ()
{
    return priv->opt;
}

bool
GLDisplay::setOption (const char        *name,
		      CompOption::Value &value)
{
    CompOption   *o;
    unsigned int index;

    o = CompOption::findOption (priv->opt, name, &index);
    if (!o)
	return false;

    switch (index) {
	case GL_DISPLAY_OPTION_TEXTURE_FILTER:
	    if (o->set (value))
	    {
		foreach (CompScreen *s, priv->display->screens ())
		    CompositeScreen::get (s)->damageScreen ();

		if (!o->value ().i ())
		    priv->textureFilter = GL_NEAREST;
		else
		    priv->textureFilter = GL_LINEAR;

		return true;
	    }
	    break;
	default:
	    if (CompOption::setDisplayOption (priv->display, *o, value))
		return true;
	    break;
    }

    return false;
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

    return CompOption::setScreenOption (priv->screen, *o, value);
}

CompMetadata *glMetadata;

class OpenglPluginVTable : public CompPlugin::VTable
{
    public:

	const char *
	name () { return "opengl"; };

	CompMetadata *
	getMetadata ();

	virtual bool
	init ();

	virtual void
	fini ();

	virtual bool
	initObject (CompObject *object);

	virtual void
	finiObject (CompObject *object);

	CompOption::Vector &
	getObjectOptions (CompObject *object);

	bool
	setObjectOption (CompObject        *object,
			 const char        *name,
			 CompOption::Value &value);
};

bool
OpenglPluginVTable::initObject (CompObject *o)
{
    INIT_OBJECT (o,_,X,X,X,,GLDisplay, GLScreen, GLWindow)
    return true;
}

void
OpenglPluginVTable::finiObject (CompObject *o)
{
    FINI_OBJECT (o,_,X,X,X,,GLDisplay, GLScreen, GLWindow)
}

CompOption::Vector &
OpenglPluginVTable::getObjectOptions (CompObject *o)
{
    GET_OBJECT_OPTIONS (o,X,X,GLDisplay, GLScreen)
    return noOptions;
}

bool
OpenglPluginVTable::setObjectOption (CompObject      *o,
				     const char      *name,
				     CompOption::Value &value)
{
    SET_OBJECT_OPTION (o,X,X,GLDisplay, GLScreen)
    return false;
}

bool
OpenglPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
        !CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI))
	return false;

    CompPrivate p;
    p.uval = COMPIZ_OPENGL_ABI;
    core->storeValue ("opengl_ABI", p);

    glMetadata = new CompMetadata
	(name (), glDisplayOptionInfo, GL_DISPLAY_OPTION_NUM,
	 glScreenOptionInfo, GL_SCREEN_OPTION_NUM);

    if (!glMetadata)
	return false;

    glMetadata->addFromFile (name ());

    return true;
}

void
OpenglPluginVTable::fini ()
{
    delete glMetadata;
}

CompMetadata *
OpenglPluginVTable::getMetadata ()
{
    return glMetadata;
}

OpenglPluginVTable openglVTable;

CompPlugin::VTable *
getCompPluginInfo20080805 (void)
{
    return &openglVTable;
}
