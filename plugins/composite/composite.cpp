#include <compiz-core.h>
#include <composite/composite.h>

#include "privates.h"

CompMetadata *compositeMetadata = NULL;

class CompositePluginVTable : public CompPlugin::VTable
{
    public:

	const char * name () { return "composite"; };

	CompMetadata * getMetadata ();

	bool init ();
	void fini ();

	bool initObject (CompObject *object);
	void finiObject (CompObject *object);

	CompOption::Vector & getObjectOptions (CompObject *object);

	bool setObjectOption (CompObject        *object,
			      const char        *name,
			      CompOption::Value &value);
};

const CompMetadata::OptionInfo
    compositeDisplayOptionInfo[COMPOSITE_DISPLAY_OPTION_NUM] = {
    { "slow_animations_key", "key", 0,
        CompositeScreen::toggleSlowAnimations, 0 }
};

const CompMetadata::OptionInfo
   compositeScreenOptionInfo[COMPOSITE_SCREEN_OPTION_NUM] = {
    { "detect_refresh_rate", "bool", 0, 0, 0 },
    { "refresh_rate", "int", "<min>1</min>", 0, 0 },
    { "unredirect_fullscreen_windows", "bool", 0, 0, 0 },
    { "force_independent_output_painting", "bool", 0, 0, 0 }
};

CompOption::Vector &
CompositeDisplay::getOptions ()
{
    return priv->opt;
}

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
CompositeDisplay::setOption (const char        *name,
			     CompOption::Value &value)
{
    CompOption   *o;
    unsigned int index;

    o = CompOption::findOption (priv->opt, name, &index);
    if (!o)
	return false;

    return CompOption::setDisplayOption (priv->display, *o, value);
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
	case COMPOSITE_SCREEN_OPTION_DETECT_REFRESH_RATE:
	    if (o->set (value))
	    {
		if (value.b ())
		    detectRefreshRate ();

		return true;
	    }
	    break;
	case COMPOSITE_SCREEN_OPTION_REFRESH_RATE:
	    if (priv->opt[COMPOSITE_SCREEN_OPTION_DETECT_REFRESH_RATE].
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
	    if (CompOption::setScreenOption (priv->screen, *o, value))
		return true;
	break;
    }

    return false;
}




bool
CompositePluginVTable::initObject (CompObject *o)
{
    switch (o->objectType ())
    {
	case COMP_OBJECT_TYPE_DISPLAY:
	    {
		CompositeDisplay *d =
		    new CompositeDisplay (GET_CORE_DISPLAY (o));
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
		CompositeScreen *s =
		    new CompositeScreen (GET_CORE_SCREEN (o));
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
		CompositeWindow *w =
		    new CompositeWindow (GET_CORE_WINDOW (o));
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
CompositePluginVTable::finiObject (CompObject *o)
{
    switch (o->objectType ())
    {
	case COMP_OBJECT_TYPE_DISPLAY:
	    {
		CompositeDisplay *d =
		    CompositeDisplay::get (GET_CORE_DISPLAY (o));
		if (d)
		    delete d;
	    }
	    break;
	case COMP_OBJECT_TYPE_SCREEN:
	    {
		CompositeScreen *s =
		    CompositeScreen::get (GET_CORE_SCREEN (o));
		if (s)
		    delete s;
	    }
	    break;
	case COMP_OBJECT_TYPE_WINDOW:
	    {
		CompositeWindow *w =
		    CompositeWindow::get (GET_CORE_WINDOW (o));
		if (w)
		    delete w;
	    }
	    break;
	default:
	    break;
    }
}

CompOption::Vector &
CompositePluginVTable::getObjectOptions (CompObject *o)
{
    switch (o->objectType ())
    {
	case COMP_OBJECT_TYPE_DISPLAY:
	    return CompositeDisplay::get (GET_CORE_DISPLAY (o))->getOptions ();
	case COMP_OBJECT_TYPE_SCREEN:
	    return CompositeScreen::get (GET_CORE_SCREEN (o))->getOptions ();
	default:
	    break;
    }
    return noOptions;
}

bool
CompositePluginVTable::setObjectOption (CompObject      *o,
					const char      *name,
					CompOption::Value &value)
{
    switch (o->objectType ())
    {
	case COMP_OBJECT_TYPE_DISPLAY:
	    return CompositeDisplay::get (GET_CORE_DISPLAY (o))->
		setOption (name, value);
	case COMP_OBJECT_TYPE_SCREEN:
	    return CompositeScreen::get (GET_CORE_SCREEN (o))->
		setOption (name, value);
	default:
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
    core->storeValue ("composite_ABI", p);

    compositeMetadata = new CompMetadata
	(name (), compositeDisplayOptionInfo, COMPOSITE_DISPLAY_OPTION_NUM,
	 compositeScreenOptionInfo, COMPOSITE_SCREEN_OPTION_NUM);

    if (!compositeMetadata)
	return false;

    compositeMetadata->addFromFile (name ());

    return true;
}

void
CompositePluginVTable::fini ()
{
    delete compositeMetadata;
}

CompMetadata *
CompositePluginVTable::getMetadata ()
{
    return compositeMetadata;
}

CompositePluginVTable compositeVTable;

CompPlugin::VTable *
getCompPluginInfo20080805 (void)
{
    return &compositeVTable;
}
