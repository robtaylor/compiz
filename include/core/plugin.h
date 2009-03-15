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
#include <core/option.h>

class CompScreen;
extern CompScreen *screen;

#include <map>

#define HOME_PLUGINDIR ".compiz/plugins"

#define COMPIZ_PLUGIN_20090315(name, classname)                           \
    CompPlugin::VTable * name##VTable = NULL;                             \
    extern "C" {                                                          \
        CompPlugin::VTable * getCompPluginVTable20090315_##name ()        \
	{                                                                 \
	    if (!name##VTable)                                            \
	    {                                                             \
	        name##VTable = new classname ();                          \
	        name##VTable->initVTable (TOSTRING(name), &name##VTable); \
                return name##VTable;                                      \
	    }                                                             \
            else                                                          \
                return name##VTable;                                      \
	}                                                                 \
    }

class CompPlugin;

typedef bool (*LoadPluginProc) (CompPlugin *p,
				const char *path,
				const char *name);

typedef void (*UnloadPluginProc) (CompPlugin *p);

typedef CompStringList (*ListPluginsProc) (const char *path);

extern LoadPluginProc   loaderLoadPlugin;
extern UnloadPluginProc loaderUnloadPlugin;
extern ListPluginsProc  loaderListPlugins;

union CompPrivate {
    void	  *ptr;
    long	  val;
    unsigned long uval;
    void	  *(*fptr) (void);
};

class CompPlugin {
    public:
	class VTable {
	    public:
		VTable ();
		virtual ~VTable ();

		void initVTable (CompString         name,
				 CompPlugin::VTable **self = NULL);
		
		const CompString name () const;

		virtual bool init () = 0;

		virtual void fini ();

		virtual bool initScreen (CompScreen *s);

		virtual void finiScreen (CompScreen *s);

		virtual bool initWindow (CompWindow *w);

		virtual void finiWindow (CompWindow *w);
 	
		virtual CompOption::Vector & getOptions () const;

		virtual bool setOption (const CompString  &name,
					CompOption::Value &value);
	    private:
		CompString   mName;
		VTable       **mSelf;
        };

	template <typename T, typename T2>
	class VTableForScreenAndWindow : public VTable {
	    bool initScreen (CompScreen *s);

	    void finiScreen (CompScreen *s);

	    bool initWindow (CompWindow *w);

	    void finiWindow (CompWindow *w);

	    CompOption::Vector & getOptions () const;

	    bool setOption (const CompString &name, CompOption::Value &value);
	};

	template <typename T>
	class VTableForScreen : public VTable {
	    bool initScreen (CompScreen *s);

	    void finiScreen (CompScreen *s);

	    CompOption::Vector & getOptions () const;

	    bool setOption (const CompString &name, CompOption::Value &value);
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
bool CompPlugin::VTableForScreenAndWindow<T,T2>::initScreen (CompScreen *s)
{
    T * ps = new T (s);
    if (ps->loadFailed ())
    {
	delete ps;
	return false;
    }
    return true;
}

template <typename T, typename T2>
void CompPlugin::VTableForScreenAndWindow<T,T2>::finiScreen (CompScreen *s)
{
    T * ps = T::get (s);
    delete ps;
}

template <typename T, typename T2>
bool CompPlugin::VTableForScreenAndWindow<T,T2>::initWindow (CompWindow *w)
{
    T2 * pw = new T2 (w);
    if (pw->loadFailed ())
    {
	delete pw;
	return false;
    }
    return true;
}

template <typename T, typename T2>
void CompPlugin::VTableForScreenAndWindow<T,T2>::finiWindow (CompWindow *w)
{
    T2 * pw = T2::get (w);
    delete pw;
}

template <typename T, typename T2>
CompOption::Vector & CompPlugin::VTableForScreenAndWindow<T,T2>::getOptions () const
{
    CompOption::Class *oc = dynamic_cast<CompOption::Class *>(T::get (screen));
    if (!oc)
	return noOptions;
    return oc->getOptions ();
}

template <typename T, typename T2>
bool CompPlugin::VTableForScreenAndWindow<T,T2>::setOption (const CompString  &name,
							    CompOption::Value &value)
{
    CompOption::Class *oc = dynamic_cast<CompOption::Class *>(T::get (screen));
    if (!oc)
	return false;
    return oc->setOption (name, value);
}

template <typename T>
bool CompPlugin::VTableForScreen<T>::initScreen (CompScreen *s)
{
    T * ps = new T (s);
    if (ps->loadFailed ())
    {
	delete ps;
	return false;
    }
    return true;
}

template <typename T>
void CompPlugin::VTableForScreen<T>::finiScreen (CompScreen *s)
{
    T * ps = T::get (s);
    delete ps;
}

template <typename T>
CompOption::Vector & CompPlugin::VTableForScreen<T>::getOptions () const
{
    CompOption::Class *oc = dynamic_cast<CompOption::Class *>(T::get (screen));
    if (!oc)
	return noOptions;
    return oc->getOptions ();
}

template <typename T>
bool CompPlugin::VTableForScreen<T>::setOption (const CompString  &name,
						CompOption::Value &value)
{
    CompOption::Class *oc = dynamic_cast<CompOption::Class *>(T::get (screen));
    if (!oc)
	return false;
    return oc->setOption (name, value);
}

typedef CompPlugin::VTable *(*PluginGetInfoProc) (void);

#endif
