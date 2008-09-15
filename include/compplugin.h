/*
 * Copyright © 2007 Novell, Inc.
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

#ifndef _COMPIZ_PLUGIN_H
#define _COMPIZ_PLUGIN_H

#include <compiz.h>
#include <compoption.h>

#include <map>

class CompMetadata;

#define HOME_PLUGINDIR ".compiz/plugins"

class CompPlugin;

typedef bool (*LoadPluginProc) (CompPlugin *p,
				const char *path,
				const char *name);

typedef void (*UnloadPluginProc) (CompPlugin *p);

typedef CompStringList (*ListPluginsProc) (const char *path);

extern LoadPluginProc   loaderLoadPlugin;
extern UnloadPluginProc loaderUnloadPlugin;
extern ListPluginsProc  loaderListPlugins;

class CompPlugin {
    public:
	class VTable {
	    public:
		virtual ~VTable ();
 	
		virtual const char * name () = 0;

		virtual CompMetadata * getMetadata ();

		virtual bool init () = 0;

		virtual void fini () = 0;

		virtual bool initScreen (CompScreen *screen);

		virtual void finiScreen (CompScreen *screen);

		virtual bool initWindow (CompWindow *window);

		virtual void finiWindow (CompWindow *window);
 	
		virtual CompOption::Vector & getOptions ();

		virtual bool setOption (const char        *name,
					CompOption::Value &value);
        };

	template <typename T, typename T2>
	class VTableForScreenAndWindow : public VTable {
	    bool initScreen (CompScreen *screen);

	    void finiScreen (CompScreen *screen);

	    bool initWindow (CompWindow *window);

	    void finiWindow (CompWindow *window);
	};

	template <typename T>
	class VTableForScreen : public VTable {
	    bool initScreen (CompScreen *screen);

	    void finiScreen (CompScreen *screen);
	};

	struct cmpStr
	{
	    bool operator() (const char *a, const char *b) const
	    {
		return strcmp (a, b) < 0;
	    }
	};

	typedef std::map<const char *, CompPlugin *, cmpStr> Map;
	typedef std::list<CompPlugin *> List;

    public:
        CompPrivate devPrivate;
        CompString  devType;
        VTable      *vTable;

    public:

	static bool screenInitPlugins (CompScreen *s);

	static void screenFiniPlugins (CompScreen *s);

	static bool windowInitPlugins (CompWindow *w);

	static void windowFiniPlugins (CompWindow *w);

	static CompPlugin *find (const char *name);

	static CompPlugin *load (const char *plugin);

	static void unload (CompPlugin *p);

	static bool push (CompPlugin *p);

	static CompPlugin *pop (void);

	static List & getPlugins ();

	static std::list<CompString> availablePlugins ();

	static int getPluginABI (const char *name);

	static bool checkPluginABI (const char *name,
				    int	        abi);

};

template <typename T, typename T2>
bool CompPlugin::VTableForScreenAndWindow<T,T2>::initScreen (CompScreen *screen)
{
    T * ps = new T (screen);
    if (ps->loadFailed ())
    {
	delete ps;
	return false;
    }
    return true;
}

template <typename T, typename T2>
void CompPlugin::VTableForScreenAndWindow<T,T2>::finiScreen (CompScreen *screen)
{
    T * ps = T::get (screen);
    delete ps;
}

template <typename T, typename T2>
bool CompPlugin::VTableForScreenAndWindow<T,T2>::initWindow (CompWindow *window)
{
    T2 * pw = new T2 (window);
    if (pw->loadFailed ())
    {
	delete pw;
	return false;
    }
    return true;
}

template <typename T, typename T2>
void CompPlugin::VTableForScreenAndWindow<T,T2>::finiWindow (CompWindow *window)
{
    T2 * pw = T2::get (window);
    delete pw;
}

template <typename T>
bool CompPlugin::VTableForScreen<T>::initScreen (CompScreen *screen)
{
    T * ps = new T (screen);
    if (ps->loadFailed ())
    {
	delete ps;
	return false;
    }
    return true;
}

template <typename T>
void CompPlugin::VTableForScreen<T>::finiScreen (CompScreen *screen)
{
    T * ps = T::get (screen);
    delete ps;
}

typedef CompPlugin::VTable *(*PluginGetInfoProc) (void);

COMPIZ_BEGIN_DECLS

CompPlugin::VTable *
getCompPluginInfo20080805 (void);

COMPIZ_END_DECLS

#endif
