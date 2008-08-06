#ifndef _PRIVATECORE_H
#define _PRIVATECORE_H

#include <compiz-core.h>
#include <compcore.h>

class PrivateCore {

    public:
	PrivateCore (CompCore *core);
	~PrivateCore ();

	void
	addTimeout(CompTimeout *);

	short int
	watchFdEvents (CompWatchFdHandle handle);

	int
	doPoll (int timeout);

	void
	handleTimeouts (struct timeval *tv);

    public:
	CompCore    *core;
	CompDisplay *displays;

	std::list<CompFileWatch *>  fileWatch;
	CompFileWatchHandle         lastFileWatchHandle;

	std::list<CompTimeout *> timeouts;
	struct timeval           lastTimeout;
	CompTimeoutHandle        lastTimeoutHandle;

	std::list<CompWatchFd *> watchFds;
	CompWatchFdHandle        lastWatchFdHandle;
	struct pollfd            *watchPollFds;
	int                      nWatchFds;
};

#endif
