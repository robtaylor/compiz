#include <compiz-core.h>
#include <compprivatehandler.h>
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

CompMetadata *glMetadata;

class OpenglPluginVTable :
    public CompPlugin::VTableForScreenAndWindow<GLScreen, GLWindow>
{
    public:

	const char * name () { return "opengl"; };

	CompMetadata * getMetadata ();

	bool init ();

	void fini ();

	PLUGIN_OPTION_HELPER (GLScreen)
};

bool
OpenglPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
        !CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI))
	return false;

    CompPrivate p;
    p.uval = COMPIZ_OPENGL_ABI;
    screen->storeValue ("opengl_ABI", p);

    glMetadata = new CompMetadata (name (), glOptionInfo, GL_OPTION_NUM);

    if (!glMetadata)
	return false;

    glMetadata->addFromFile (name ());

    return true;
}

void
OpenglPluginVTable::fini ()
{
    screen->eraseValue ("opengl_ABI");
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
