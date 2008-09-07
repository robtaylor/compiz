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

#define __GET_PLUGIN_OBJECT_OPTIONS__(name, object)
#define __GET_PLUGIN_OBJECT_OPTIONS_X(name, object) \
    return name::get(object)->getOptions ();

#define __SET_PLUGIN_OBJECT_OPTION__(name, object, oName, oValue)
#define __SET_PLUGIN_OBJECT_OPTION_X(name, object, oName, oValue) \
    return name::get(object)->setOption (oName, oValue);

#define __INIT_PLUGIN_OBJECT__(name, object) 
#define __INIT_PLUGIN_OBJECT_X(name, object) \
{                                            \
    name *obj = new name (object);           \
    if (!obj)                                \
	return false;                        \
    if (obj->loadFailed ())                  \
    {                                        \
	delete obj;                          \
	return false;                        \
    }                                        \
    return true;                             \
}

#define __FINI_PLUGIN_OBJECT__(name, object)
#define __FINI_PLUGIN_OBJECT_X(name, object) \
{                                            \
    name *obj = name::get (object);          \
    if (obj)                                 \
        delete obj;                          \
}

#define INIT_OBJECT(obj, hasCore, hasDisplay, hasScreen, hasWindow, 			\
                    coreName, displayName, screenName, windowName) 			\
    switch ( obj ->objectType ())							\
    {											\
	case COMP_OBJECT_TYPE_CORE:							\
	    __INIT_PLUGIN_OBJECT_ ## hasCore (coreName, GET_CORE_CORE (obj))		\
	    break;									\
	case COMP_OBJECT_TYPE_DISPLAY:							\
	    __INIT_PLUGIN_OBJECT_ ## hasDisplay (displayName, GET_CORE_DISPLAY (obj)) 	\
	    break;									\
	case COMP_OBJECT_TYPE_SCREEN:							\
	    __INIT_PLUGIN_OBJECT_ ## hasScreen (screenName, GET_CORE_SCREEN (obj)) 	\
	    break;									\
	case COMP_OBJECT_TYPE_WINDOW:							\
	    __INIT_PLUGIN_OBJECT_ ## hasWindow (windowName, GET_CORE_WINDOW (obj)) 	\
	    break;									\
	default:									\
	    break;									\
    }

#define FINI_OBJECT(obj, hasCore, hasDisplay, hasScreen, hasWindow, 			\
                    coreName, displayName, screenName, windowName) 			\
    switch ( obj ->objectType ())							\
    {											\
	case COMP_OBJECT_TYPE_CORE:							\
	    __FINI_PLUGIN_OBJECT_ ## hasCore (coreName, GET_CORE_CORE (obj))		\
	    break;									\
	case COMP_OBJECT_TYPE_DISPLAY:							\
	    __FINI_PLUGIN_OBJECT_ ## hasDisplay (displayName, GET_CORE_DISPLAY (obj)) 	\
	    break;									\
	case COMP_OBJECT_TYPE_SCREEN:							\
	    __FINI_PLUGIN_OBJECT_ ## hasScreen (screenName, GET_CORE_SCREEN (obj))	\
	    break;									\
	case COMP_OBJECT_TYPE_WINDOW:							\
	    __FINI_PLUGIN_OBJECT_ ## hasWindow (windowName, GET_CORE_WINDOW (obj))	\
	    break;									\
	default:									\
	    break;									\
    }

#define GET_OBJECT_OPTIONS(obj, hasDisplay, hasScreen, displayName, screenName) \
    switch ( obj ->objectType ())						\
    {										\
	case COMP_OBJECT_TYPE_DISPLAY:						\
	    __GET_PLUGIN_OBJECT_OPTIONS_ ## hasDisplay				\
		(displayName, GET_CORE_DISPLAY (obj)) 				\
	    break;								\
	case COMP_OBJECT_TYPE_SCREEN:						\
	    __GET_PLUGIN_OBJECT_OPTIONS_ ## hasScreen 				\
		(screenName, GET_CORE_SCREEN (obj)) 				\
	    break;								\
	default:								\
	    break;								\
    }

#define SET_OBJECT_OPTION(obj, hasDisplay, hasScreen, displayName, screenName)	\
    switch ( obj ->objectType ())						\
    {										\
	case COMP_OBJECT_TYPE_DISPLAY:						\
	    __SET_PLUGIN_OBJECT_OPTION_ ## hasDisplay				\
		(displayName, GET_CORE_DISPLAY (obj), name, value) 		\
	    break;								\
	case COMP_OBJECT_TYPE_SCREEN:						\
	    __SET_PLUGIN_OBJECT_OPTION_ ## hasScreen				\
		(screenName, GET_CORE_SCREEN (obj), name, value) 		\
	    break;								\
	default:								\
	    break;								\
    }

class CompObject;
class CompMetadata;

typedef bool (*InitPluginObjectProc) (CompObject *object);
typedef void (*FiniPluginObjectProc) (CompObject *object);

typedef CompOption::Vector & (*GetPluginObjectOptionsProc) (CompObject *object);
typedef bool (*SetPluginObjectOptionProc) (CompObject        *object,
					   const char        *name,
					   CompOption::Value &value);

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

		virtual CompMetadata *
		getMetadata ();

		virtual bool
		init () = 0;

		virtual void
		fini () = 0;

		virtual bool
		initObject (CompObject *object);

		virtual void
		finiObject (CompObject *object);
 	
		virtual CompOption::Vector &
		getObjectOptions (CompObject *object);

		virtual bool
		setObjectOption (CompObject        *object,
				const char        *name,
				CompOption::Value &value);
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

	static bool objectInitPlugins (CompObject *o);

	static void objectFiniPlugins (CompObject *o);

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

typedef CompPlugin::VTable *(*PluginGetInfoProc) (void);

COMPIZ_BEGIN_DECLS

CompPlugin::VTable *
getCompPluginInfo20080805 (void);

COMPIZ_END_DECLS

#endif
