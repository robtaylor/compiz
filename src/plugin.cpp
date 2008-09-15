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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <dirent.h>
#include <list>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#include <compiz-core.h>
#include "privatescreen.h"

CompPlugin::Map pluginsMap;
CompPlugin::List plugins;

class CorePluginVTable : public CompPlugin::VTable
{
    public:

	const char * name () { return "core"; };

	CompMetadata * getMetadata ();

	bool init ();

	void fini ();

	CompOption::Vector & getOptions ();

	bool setOption (const char        *name,
			CompOption::Value &value);
};

bool
CorePluginVTable::init ()
{
    return true;
}

void
CorePluginVTable::fini ()
{
}

CompMetadata *
CorePluginVTable::getMetadata ()
{
    return coreMetadata;
}

CompOption::Vector &
CorePluginVTable::getOptions ()
{
    return screen->getOptions ();
}

bool
CorePluginVTable::setOption (const char        *name,
			     CompOption::Value &value)
{
    return screen->setOption (name, value);
}

CorePluginVTable coreVTable;

static bool
cloaderLoadPlugin (CompPlugin *p,
		   const char *path,
		   const char *name)
{
    if (path)
	return false;

    if (strcmp (name, coreVTable.name ()))
	return false;

    p->vTable	      = &coreVTable;
    p->devPrivate.ptr = NULL;
    p->devType	      = "cloader";

    return true;
}

static void
cloaderUnloadPlugin (CompPlugin *p)
{
}

static CompStringList
cloaderListPlugins (const char *path)
{
    CompStringList rv;

    if (path)
	return CompStringList ();

    rv.push_back (CompString (coreVTable.name ()));

    return rv;
}

static bool
dlloaderLoadPlugin (CompPlugin *p,
		    const char *path,
		    const char *name)
{
    char *file;
    void *dlhand;

    file = (char *) malloc ((path ? strlen (path) : 0) + strlen (name) + 8);
    if (!file)
	return false;

    if (path)
	sprintf (file, "%s/lib%s.so", path, name);
    else
	sprintf (file, "lib%s.so", name);

    dlhand = dlopen (file, RTLD_LAZY);
    if (dlhand)
    {
	PluginGetInfoProc getInfo;
	char		  *error;

	dlerror ();

	getInfo = (PluginGetInfoProc)
	    dlsym (dlhand, "getCompPluginInfo20080805");

	error = dlerror ();
	if (error)
	{
	    compLogMessage ("core", CompLogLevelError, "dlsym: %s", error);

	    getInfo = 0;
	}

	if (getInfo)
	{
	    p->vTable = (*getInfo) ();
	    if (!p->vTable)
	    {
		compLogMessage ("core", CompLogLevelError,
				"Couldn't get vtable from '%s' plugin", file);

		dlclose (dlhand);
		free (file);

		return false;
	    }
	}
	else
	{
	    dlclose (dlhand);
	    free (file);

	    return false;
	}
    }
    else
    {
	free (file);

	return cloaderLoadPlugin (p, path, name);
    }

    free (file);

    p->devPrivate.ptr = dlhand;
    p->devType	      = "dlloader";

    return true;
}

static void
dlloaderUnloadPlugin (CompPlugin *p)
{
    if (p->devType.compare ("dlloader") == 0)
	dlclose (p->devPrivate.ptr);
    else
	cloaderUnloadPlugin (p);
}

static int
dlloaderFilter (const struct dirent *name)
{
    int length = strlen (name->d_name);

    if (length < 7)
	return 0;

    if (strncmp (name->d_name, "lib", 3) ||
	strncmp (name->d_name + length - 3, ".so", 3))
	return 0;

    return 1;
}

static CompStringList
dlloaderListPlugins (const char *path)
{
    struct dirent **nameList;
    char	  name[1024];
    int		  length, nFile, i;

    CompStringList rv = cloaderListPlugins (path);

    if (!path)
	path = ".";

    nFile = scandir (path, &nameList, dlloaderFilter, alphasort);
    if (!nFile)
	return rv;

    for (i = 0; i < nFile; i++)
    {
	length = strlen (nameList[i]->d_name);

	strncpy (name, nameList[i]->d_name + 3, length - 6);
	name[length - 6] = '\0';

	rv.push_back (CompString (name));
    }

    return rv;
}

LoadPluginProc   loaderLoadPlugin   = dlloaderLoadPlugin;
UnloadPluginProc loaderUnloadPlugin = dlloaderUnloadPlugin;
ListPluginsProc  loaderListPlugins  = dlloaderListPlugins;


static bool
initPlugin (CompPlugin *p)
{

    if (!p->vTable->init ())
    {
	compLogMessage ("core", CompLogLevelError,
			"InitPlugin '%s' failed", p->vTable->name ());
	return false;
    }

    if (screen)
    {
	if (!p->vTable->initScreen (screen))
	{
	    compLogMessage (p->vTable->name (), CompLogLevelError,
                            "initScreen failed");
	    p->vTable->fini ();
	    return false;
	}
	if (!screen->initPluginForScreen (p))
	{
	    p->vTable->fini ();
	    return false;
	}
    }

    return true;
}

static void
finiPlugin (CompPlugin *p)
{

    if (screen)
    {
	screen->finiPluginForScreen (p);
	p->vTable->finiScreen (screen);
    }

    p->vTable->fini ();
}

bool
CompScreen::initPluginForScreen (CompPlugin *p)
{
    WRAPABLE_HND_FUNC_RETURN(2, bool, initPluginForScreen, p)

    bool status               = true;
    CompWindowList::iterator it, fail;
    CompWindow               *w;

    it = fail = priv->windows.begin ();
    for (;it != priv->windows.end (); it++)
    {
	w = *it;
	if (!p->vTable->initWindow (w))
	{
	    compLogMessage (p->vTable->name (), CompLogLevelError,
                            "initWindow failed");
            fail   = it;
            status = false;
	}
    }

    it = priv->windows.begin ();
    for (;it != fail; it++)
    {
	w = *it;
	p->vTable->finiWindow (w);
    }

    return status;
}

void
CompScreen::finiPluginForScreen (CompPlugin *p)
{
    WRAPABLE_HND_FUNC(3, finiPluginForScreen, p)

    foreach (CompWindow *w, priv->windows)
	p->vTable->finiWindow (w);
}

bool
CompPlugin::screenInitPlugins (CompScreen *s)
{
    CompPlugin::List::reverse_iterator rit = plugins.rbegin ();

    CompPlugin *p = NULL;

    while (rit != plugins.rend ())
    {
	p = (*rit);

	if (p->vTable->initScreen (s))
	    s->initPluginForScreen (p);
	
	rit++;
    }

    return true;
}

void
CompPlugin::screenFiniPlugins (CompScreen *s)
{
    foreach (CompPlugin *p, plugins)
    {
	s->finiPluginForScreen (p);
	p->vTable->finiScreen (s);
    }

}

bool
CompPlugin::windowInitPlugins (CompWindow *w)
{
    bool status = true;

    foreach (CompPlugin *p, plugins)
    {
	p->vTable->finiWindow (w);
    }

    return status;
}

void
CompPlugin::windowFiniPlugins (CompWindow *w)
{
    foreach (CompPlugin *p, plugins)
    {
	p->vTable->finiWindow (w);
    }
}


CompPlugin *
CompPlugin::find (const char *name)
{
    CompPlugin::Map::iterator it = pluginsMap.find (name);

    if (it != pluginsMap.end ())
        return it->second;

    return NULL;
}

void
CompPlugin::unload (CompPlugin *p)
{
    (*loaderUnloadPlugin) (p);
    delete p;
}

CompPlugin *
CompPlugin::load (const char *name)
{
    CompPlugin *p;
    char       *home, *plugindir;
    Bool       status;

    p = new CompPlugin ();
    if (!p)
	return 0;

    p->devPrivate.uval = 0;
    p->devType	       = "";
    p->vTable	       = 0;

    home = getenv ("HOME");
    if (home)
    {
	plugindir = (char *) malloc (strlen (home) + strlen (HOME_PLUGINDIR) + 3);
	if (plugindir)
	{
	    sprintf (plugindir, "%s/%s", home, HOME_PLUGINDIR);
	    status = (*loaderLoadPlugin) (p, plugindir, name);
	    free (plugindir);

	    if (status)
		return p;
	}
    }

    status = (*loaderLoadPlugin) (p, PLUGINDIR, name);
    if (status)
	return p;

    status = (*loaderLoadPlugin) (p, NULL, name);
    if (status)
	return p;

    compLogMessage ("core", CompLogLevelError,
		    "Couldn't load plugin '%s'", name);

    return 0;
}

bool
CompPlugin::push (CompPlugin *p)
{
    const char *name = p->vTable->name ();

    std::pair<CompPlugin::Map::iterator, bool> insertRet =
        pluginsMap.insert (std::pair<const char *, CompPlugin *> (name, p));

    if (!insertRet.second)
    {
	compLogMessage ("core", CompLogLevelWarn,
			"Plugin '%s' already active",
			p->vTable->name ());

	return false;
    }

    plugins.push_front (p);

    if (!initPlugin (p))
    {
	compLogMessage ("core", CompLogLevelError,
			"Couldn't activate plugin '%s'", name);

        pluginsMap.erase (name);
	plugins.pop_front ();

	return false;
    }

    return true;
}

CompPlugin *
CompPlugin::pop (void)
{
    if (plugins.empty ())
	return NULL;

    CompPlugin *p = plugins.front ();

    if (!p)
	return 0;

    pluginsMap.erase (p->vTable->name ());

    finiPlugin (p);

    plugins.pop_front ();

    return p;
}

CompPlugin::List &
CompPlugin::getPlugins (void)
{
    return plugins;
}

static bool
stringExist (CompStringList &list,
	     CompString     s)
{
    foreach (CompString &l, list)
	if (s.compare (l) == 0)
	    return true;

    return false;
}

CompStringList
CompPlugin::availablePlugins ()
{
    char *home, *plugindir;
    CompStringList list, currentList, pluginList, homeList;

    home = getenv ("HOME");
    if (home)
    {
	plugindir = (char *) malloc (strlen (home) + strlen (HOME_PLUGINDIR) + 3);
	if (plugindir)
	{
	    sprintf (plugindir, "%s/%s", home, HOME_PLUGINDIR);
	    homeList = (*loaderListPlugins) (plugindir);
	    free (plugindir);
	}
    }

    pluginList  = (*loaderListPlugins) (PLUGINDIR);
    currentList = (*loaderListPlugins) (NULL);

    if (!homeList.empty ())
    {
	foreach (CompString &s, homeList)
	    if (!stringExist (list, s))
		list.push_back (s);
    }

    if (!pluginList.empty ())
    {
	foreach (CompString &s, pluginList)
	    if (!stringExist (list, s))
		list.push_back (s);
    }

    if (!currentList.empty ())
    {
	foreach (CompString &s, currentList)
	    if (!stringExist (list, s))
		list.push_back (s);
    }

    return list;
}

int
CompPlugin::getPluginABI (const char *name)
{
    CompPlugin *p = find (name);
    CompString s = name;

    if (!p)
	return 0;

    s += "_ABI";

    if (!screen->hasValue (s))
	return 0;

    return screen->getValue (s).uval;
}

bool
CompPlugin::checkPluginABI (const char *name,
			    int        abi)
{
    int pluginABI;

    pluginABI = getPluginABI (name);
    if (!pluginABI)
    {
	compLogMessage ("core", CompLogLevelError,
			"Plugin '%s' not loaded.\n", name);
	return false;
    }
    else if (pluginABI != abi)
    {
	compLogMessage ("core", CompLogLevelError,
			"Plugin '%s' has ABI version '%d', expected "
			"ABI version '%d'.\n",
			name, pluginABI, abi);
	return false;
    }

    return true;
}

CompPlugin::VTable::~VTable ()
{
}

CompMetadata *
CompPlugin::VTable::getMetadata ()
{
    return NULL;
}


bool
CompPlugin::VTable::initScreen (CompScreen *)
{
    return true;
}

void
CompPlugin::VTable::finiScreen (CompScreen *)
{
}

bool
CompPlugin::VTable::initWindow (CompWindow *)
{
    return true;
}

void
CompPlugin::VTable::finiWindow (CompWindow *)
{
}
	
CompOption::Vector &
CompPlugin::VTable::getOptions ()
{
    return noOptions;
}

bool
CompPlugin::VTable::setOption (const char        *name,
			       CompOption::Value &value)
{
    return false;
}
