#ifndef _COMPCORE_H
#define _COMPCORE_H

#include <list>
#include <boost/function.hpp>

#include "wrapable.h"

#include <compoption.h>
#include <compobject.h>
#include <compsession.h>

class PrivateCore;
class CompCore;
class CompDisplay;
class CompPlugin;
class CompMetadata;
typedef std::list<CompDisplay *> CompDisplayList;

extern CompCore     *core;
extern CompMetadata *coreMetadata;

#define GET_CORE_CORE(object) (dynamic_cast<CompCore *> (object))
#define CORE_CORE(object) CompCore *c = GET_CORE_SCREEN (object)

#define NOTIFY_CREATE_MASK (1 << 0)
#define NOTIFY_DELETE_MASK (1 << 1)
#define NOTIFY_MOVE_MASK   (1 << 2)
#define NOTIFY_MODIFY_MASK (1 << 3)

typedef boost::function<void ()> FdWatchCallBack;
typedef boost::function<void (const char *)> FileWatchCallBack;

typedef int CompFileWatchHandle;
typedef int CompWatchFdHandle;

struct CompFileWatch {
    char		*path;
    int			mask;
    FileWatchCallBack   callBack;
    CompFileWatchHandle handle;
};

class CoreInterface : public WrapableInterface<CompCore> {
    public:
	CoreInterface ();

    WRAPABLE_DEF(void, fileWatchAdded, CompFileWatch *)
    WRAPABLE_DEF(void, fileWatchRemoved, CompFileWatch *)

    WRAPABLE_DEF(bool, initPluginForObject, CompPlugin *, CompObject *)
    WRAPABLE_DEF(void, finiPluginForObject, CompPlugin *, CompObject *)

    WRAPABLE_DEF(bool, setOptionForPlugin, CompObject *, const char *, const char *, CompOption::Value &)

    WRAPABLE_DEF(void, objectAdd, CompObject *, CompObject *)
    WRAPABLE_DEF(void, objectRemove, CompObject *, CompObject *)

    WRAPABLE_DEF(void, sessionEvent, CompSession::Event, CompOption::Vector &)
};

class CompCore : public WrapableHandler<CoreInterface>, public CompObject {

    public:
	class Timer {

	    public:

		typedef boost::function<bool ()> CallBack;
		
		Timer ();
		~Timer ();

		bool active ();
		unsigned int minTime ();
		unsigned int maxTime ();
		unsigned int minLeft ();
		unsigned int maxLeft ();
		
		void setTimes (unsigned int min, unsigned int max = 0);
		void setCallback (CallBack callback);

		void start ();
		void start (unsigned int min, unsigned int max = 0);
		void start (CallBack callback,
			    unsigned int min, unsigned int max = 0);
		void stop ();

		friend class CompCore;
		friend class PrivateCore;
		
	    private:
		bool         mActive;
		unsigned int mMinTime;
		unsigned int mMaxTime;
		int          mMinLeft;
		int          mMaxLeft;
		CallBack     mCallBack;
	};

    // functions
    public:
	CompCore ();
	~CompCore ();

	CompString objectName ();

	bool
	init ();

	bool
	addDisplay (const char *name);

	void
	removeDisplay (CompDisplay *);

	void
	eventLoop ();

	CompDisplayList &
	displays();
	
	CompFileWatchHandle
	addFileWatch (const char        *path,
		      int               mask,
		      FileWatchCallBack callBack);

	void
	removeFileWatch (CompFileWatchHandle handle);
	
	CompWatchFdHandle
	addWatchFd (int	            fd,
		    short int       events,
		    FdWatchCallBack callBack);
	
	void
	removeWatchFd (CompWatchFdHandle handle);

	void storeValue (CompString key, CompPrivate value);
	bool hasValue (CompString key);
	CompPrivate getValue (CompString key);
	void eraseValue (CompString key);


	static int allocPrivateIndex ();
	static void freePrivateIndex (int index);

        // Wrapable interface

	WRAPABLE_HND(void, fileWatchAdded, CompFileWatch *)
	WRAPABLE_HND(void, fileWatchRemoved, CompFileWatch *)

	WRAPABLE_HND(bool, initPluginForObject, CompPlugin *, CompObject *)
	WRAPABLE_HND(void, finiPluginForObject, CompPlugin *, CompObject *)

	WRAPABLE_HND(bool, setOptionForPlugin, CompObject *, const char *, const char *, CompOption::Value &)

	WRAPABLE_HND(void, objectAdd, CompObject *, CompObject *)
	WRAPABLE_HND(void, objectRemove, CompObject *, CompObject *)

	WRAPABLE_HND(void, sessionEvent, CompSession::Event, CompOption::Vector &)

	friend class Timer;
    private:
	PrivateCore *priv;
};

#endif
