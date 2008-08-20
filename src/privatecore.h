#ifndef _PRIVATECORE_H
#define _PRIVATECORE_H

#include <compiz-core.h>
#include <compcore.h>

extern bool shutDown;
extern bool restartSignal;

typedef struct _CompWatchFd {
    int               fd;
    FdWatchCallBack   callBack;
    CompWatchFdHandle handle;
} CompWatchFd;

class PrivateCore {

    public:
	PrivateCore (CompCore *core);
	~PrivateCore ();

	short int
	watchFdEvents (CompWatchFdHandle handle);

	int
	doPoll (int timeout);

	void
	handleTimers (struct timeval *tv);

	void addTimer (CompCore::Timer *timer);
	void removeTimer (CompCore::Timer *timer);


    public:
	CompCore        *core;
	CompDisplayList displays;

	std::list<CompFileWatch *>  fileWatch;
	CompFileWatchHandle         lastFileWatchHandle;

	std::list<CompCore::Timer *> timers;
	struct timeval               lastTimeout;

	std::list<CompWatchFd *> watchFds;
	CompWatchFdHandle        lastWatchFdHandle;
	struct pollfd            *watchPollFds;
	int                      nWatchFds;
};

#endif
