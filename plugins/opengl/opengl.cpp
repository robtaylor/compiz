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
    switch (o->objectType ())
    {
	case COMP_OBJECT_TYPE_DISPLAY:
	    {
		GLDisplay *d = new GLDisplay (GET_CORE_DISPLAY (o));
		if (!d)
		    return false;
		if (d->loadFailed ())
		{
		    delete d;
		    return false;
		}
		return true;
	    }
	    break;
	case COMP_OBJECT_TYPE_SCREEN:
	    {
		GLScreen *s = new GLScreen (GET_CORE_SCREEN (o));
		if (!s)
		    return false;
		if (s->loadFailed ())
		{
		    delete s;
		    return false;
		}
		return true;
	    }
	    break;
	case COMP_OBJECT_TYPE_WINDOW:
	    {
		GLWindow *w = new GLWindow (GET_CORE_WINDOW (o));
		if (!w)
		    return false;
		if (w->loadFailed ())
		{
		    delete w;
		    return false;
		}
		return true;
	    }
	    break;
	default:
	    break;
    }
    return true;
}

void
OpenglPluginVTable::finiObject (CompObject *o)
{
    switch (o->objectType ())
    {
	case COMP_OBJECT_TYPE_DISPLAY:
	    {
		GLDisplay *d = GLDisplay::get (GET_CORE_DISPLAY (o));
		if (d)
		    delete d;
	    }
	    break;
	case COMP_OBJECT_TYPE_SCREEN:
	    {
		GLScreen *s = GLScreen::get (GET_CORE_SCREEN (o));
		if (s)
		    delete s;
	    }
	    break;
	case COMP_OBJECT_TYPE_WINDOW:
	    {
		GLWindow *w = GLWindow::get (GET_CORE_WINDOW (o));
		if (w)
		    delete w;
	    }
	    break;
	default:
	    break;
    }
}

CompOption::Vector &
OpenglPluginVTable::getObjectOptions (CompObject *o)
{
    switch (o->objectType ())
    {
	case COMP_OBJECT_TYPE_DISPLAY:
	    return GLDisplay::get (GET_CORE_DISPLAY (o))->getOptions ();
	case COMP_OBJECT_TYPE_SCREEN:
	    return GLScreen::get (GET_CORE_SCREEN (o))->getOptions ();
	default:
	    break;
    }
    return noOptions;
}

bool
OpenglPluginVTable::setObjectOption (CompObject      *o,
				     const char      *name,
				     CompOption::Value &value)
{
    switch (o->objectType ())
    {
	case COMP_OBJECT_TYPE_DISPLAY:
	    return GLDisplay::get (GET_CORE_DISPLAY (o))->
		setOption (name, value);
	case COMP_OBJECT_TYPE_SCREEN:
	    return GLScreen::get (GET_CORE_SCREEN (o))->setOption (name, value);
	default:
	    break;
    }
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
