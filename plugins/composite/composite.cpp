#include <core/core.h>
#include <composite/composite.h>

#include "privates.h"

CompMetadata *compositeMetadata = NULL;

class CompositePluginVTable :
    public CompPlugin::VTableForScreenAndWindow<CompositeScreen, CompositeWindow>
{
    public:

	const char * name () { return "composite"; };

	CompMetadata * getMetadata ();

	bool init ();
	void fini ();

	PLUGIN_OPTION_HELPER (CompositeScreen)
};

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

    compositeMetadata = new CompMetadata
	(name (), compositeOptionInfo, COMPOSITE_OPTION_NUM);

    if (!compositeMetadata)
	return false;

    compositeMetadata->addFromFile (name ());

    return true;
}

void
CompositePluginVTable::fini ()
{
    screen->eraseValue ("composite_ABI");
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
