#ifndef _COMPCORE_H
#define _COMPCORE_H

#include <list>
#include <boost/function.hpp>
#include "wrapable.h"

#include <compoption.h>


class PrivateCore;
class CompCore;
class CompDisplay;

#define NOTIFY_CREATE_MASK (1 << 0)
#define NOTIFY_DELETE_MASK (1 << 1)
#define NOTIFY_MOVE_MASK   (1 << 2)
#define NOTIFY_MODIFY_MASK (1 << 3)

typedef void (*FileWatchCallBackProc) (const char *name,
				       void	  *closure);

typedef int CompFileWatchHandle;

typedef struct _CompFileWatch {
    char		  *path;
    int			  mask;
    FileWatchCallBackProc callBack;
    void		  *closure;
    CompFileWatchHandle   handle;
} CompFileWatch;

typedef struct _CompWatchFd {
    int			fd;
    CallBackProc	callBack;
    void		*closure;
    CompWatchFdHandle   handle;
} CompWatchFd;

int
allocCoreObjectPrivateIndex (CompObject *parent);

void
freeCoreObjectPrivateIndex (CompObject *parent,
			    int	       index);

CompBool
forEachCoreObject (CompObject	     *parent,
		   ObjectCallBackProc proc,
		   void		     *closure);

char *
nameCoreObject (CompObject *object);

CompObject *
findCoreObject (CompObject *parent,
		const char *name);

int
allocateCorePrivateIndex (void);

void
freeCorePrivateIndex (int index);


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

    WRAPABLE_DEF(void, sessionEvent, CompSessionEvent, CompOption::Vector &)
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

	CompString name ();

	bool
	init ();

	bool
	addDisplay (const char *name);

	void
	removeDisplay (CompDisplay *);

	void
	eventLoop ();

	CompDisplay *
	displays();
	
	CompFileWatchHandle
	addFileWatch (const char	    *path,
		      int		    mask,
		      FileWatchCallBackProc callBack,
		      void		    *closure);

	void
	removeFileWatch (CompFileWatchHandle handle);
	
	CompWatchFdHandle
	addWatchFd (int	         fd,
		    short int    events,
		    CallBackProc callBack,
		    void	 *closure);
	
	void
	removeWatchFd (CompWatchFdHandle handle);


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

	WRAPABLE_HND(void, sessionEvent, CompSessionEvent, CompOption::Vector &)

	friend class Timer;
    private:
	PrivateCore *priv;
};

#endif
