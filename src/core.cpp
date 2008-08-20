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

#include <string.h>
#include <sys/poll.h>
#include <assert.h>
#include <algorithm>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#include <compiz-core.h>
#include "privatecore.h"

CompCore *core;

CompObject::indices corePrivateIndices (0);

int
CompCore::allocPrivateIndex ()
{
    return CompObject::allocatePrivateIndex (COMP_OBJECT_TYPE_CORE,
					     &corePrivateIndices);
}

void
CompCore::freePrivateIndex (int index)
{
    CompObject::freePrivateIndex (COMP_OBJECT_TYPE_CORE,
				  &corePrivateIndices, index);
}

#define TIMEVALDIFF(tv1, tv2)						   \
    ((tv1)->tv_sec == (tv2)->tv_sec || (tv1)->tv_usec >= (tv2)->tv_usec) ? \
    ((((tv1)->tv_sec - (tv2)->tv_sec) * 1000000) +			   \
     ((tv1)->tv_usec - (tv2)->tv_usec)) / 1000 :			   \
    ((((tv1)->tv_sec - 1 - (tv2)->tv_sec) * 1000000) +			   \
     (1000000 + (tv1)->tv_usec - (tv2)->tv_usec)) / 1000

CompCore::CompCore () :
    CompObject (COMP_OBJECT_TYPE_CORE, "core")
{
    priv = new PrivateCore (this);
    assert (priv);
}

bool
CompCore::init ()
{
    WRAPABLE_INIT_HND(fileWatchAdded);
    WRAPABLE_INIT_HND(fileWatchRemoved);
    WRAPABLE_INIT_HND(initPluginForObject);
    WRAPABLE_INIT_HND(finiPluginForObject);
    WRAPABLE_INIT_HND(setOptionForPlugin);
    WRAPABLE_INIT_HND(objectAdd);
    WRAPABLE_INIT_HND(objectRemove);
    WRAPABLE_INIT_HND(sessionEvent);

    CompPlugin *corePlugin = CompPlugin::load ("core");
    if (!corePlugin)
    {
	compLogMessage (0, "core", CompLogLevelFatal,
			"Couldn't load core plugin");
	return false;
    }

    if (!CompPlugin::push (corePlugin))
    {
	compLogMessage (0, "core", CompLogLevelFatal,
			"Couldn't activate core plugin");
	return false;
    }

    return true;
}

CompCore::~CompCore ()
{
    CompPlugin  *p;

    while (!priv->displays.empty ())
    {
	removeDisplay (priv->displays.front ());
	priv->displays.pop_front ();
    }

    if (priv->watchPollFds)
	free (priv->watchPollFds);

    while ((p = CompPlugin::pop ()))
	CompPlugin::unload (p);

}

CompString
CompCore::name ()
{
    return CompString ("");
}


CompDisplayList &
CompCore::displays()
{
    return priv->displays;
}

bool
CompCore::addDisplay (const char *name)
{
    CompDisplay *d = new CompDisplay();

    if (!d)
	return false;

    priv->displays.push_back (d);

    if (!d->init (name))
    {
	priv->displays.pop_back ();
	delete d;
	return false;
    }
    return true;
}

void
CompCore::removeDisplay (CompDisplay *d)
{
    CompDisplayList::iterator it;
    it = std::find (priv->displays.begin (), priv->displays.end (), d);
    priv->displays.erase (it);

    delete d;
}

void
CompCore::eventLoop ()
{
    struct timeval  tv;
    CompCore::Timer *t;
    int		    time;

    foreach (CompDisplay *d, priv->displays)
	d->setWatchFdHandle (addWatchFd (ConnectionNumber (d->dpy()),
			     		 POLLIN, NULL, NULL));

    for (;;)
    {
	if (restartSignal || shutDown)
	    break;

	foreach (CompDisplay *d, priv->displays)
	    d->processEvents ();

	if (!priv->timers.empty())
	{
	    gettimeofday (&tv, 0);
	    priv->handleTimers (&tv);

	    if (priv->timers.front()->mMinLeft > 0)
	    {
		std::list<CompCore::Timer *>::iterator it = priv->timers.begin();

		t = (*it);
		time = t->mMaxLeft;
		while (it != priv->timers.end())
		{
		    t = (*it);
		    if (t->mMinLeft <= time)
			break;
		    if (t->mMaxLeft < time)
			time = t->mMaxLeft;
		    it++;
		}
		priv->doPoll (time);
		gettimeofday (&tv, 0);
		priv->handleTimers (&tv);
	    }
	}
	else
	{
	    priv->doPoll (-1);
	}
    }

    foreach (CompDisplay *d, priv->displays)
	removeWatchFd (d->getWatchFdHandle());
}



CompFileWatchHandle
CompCore::addFileWatch (const char	      *path,
			int		      mask,
			FileWatchCallBackProc callBack,
			void		      *closure)
{
    CompFileWatch *fileWatch = new CompFileWatch();
    if (!fileWatch)
	return 0;

    fileWatch->path	= strdup (path);
    fileWatch->mask	= mask;
    fileWatch->callBack = callBack;
    fileWatch->closure  = closure;
    fileWatch->handle   = priv->lastFileWatchHandle++;

    if (priv->lastFileWatchHandle == MAXSHORT)
	priv->lastFileWatchHandle = 1;

    priv->fileWatch.push_front(fileWatch);

    fileWatchAdded (fileWatch);

    return fileWatch->handle;
}

void
CompCore::removeFileWatch (CompFileWatchHandle handle)
{
    std::list<CompFileWatch *>::iterator it;
    CompFileWatch                        *w;

    for (it = priv->fileWatch.begin(); it != priv->fileWatch.end(); it++)
	if ((*it)->handle == handle)
	    break;

    if (it == priv->fileWatch.end())
	return;

    w = (*it);
    priv->fileWatch.erase (it);

    fileWatchRemoved (w);

    delete w;
}

void
PrivateCore::addTimer (CompCore::Timer *timer)
{
    std::list<CompCore::Timer *>::iterator it;

    it = std::find (timers.begin (), timers.end (), timer);

    if (it != timers.end ())
	return;

    for (it = timers.begin(); it != timers.end(); it++)
    {
	if ((int) timer->mMinTime < (*it)->mMinLeft)
	    break;
    }

    timer->mMinLeft = timer->mMinTime;
    timer->mMaxLeft = timer->mMaxTime;

    timers.insert (it, timer);
}

void
PrivateCore::removeTimer (CompCore::Timer *timer)
{
    std::list<CompCore::Timer *>::iterator it;

    it = std::find (timers.begin (), timers.end (), timer);

    if (it == timers.end ())
	return;

    timers.erase (it);
}

CompWatchFdHandle
CompCore::addWatchFd (int	   fd,
		      short int    events,
		      CallBackProc callBack,
		      void	   *closure)
{
    CompWatchFd *watchFd = new CompWatchFd();

    if (!watchFd)
	return 0;

    watchFd->fd	      = fd;
    watchFd->callBack = callBack;
    watchFd->closure  = closure;
    watchFd->handle   = priv->lastWatchFdHandle++;

    if (priv->lastWatchFdHandle == MAXSHORT)
	priv->lastWatchFdHandle = 1;

    priv->watchFds.push_front (watchFd);

    priv->nWatchFds++;

    priv->watchPollFds = (struct pollfd *) realloc (priv->watchPollFds,
			  priv->nWatchFds * sizeof (struct pollfd));

    priv->watchPollFds[priv->nWatchFds - 1].fd     = fd;
    priv->watchPollFds[priv->nWatchFds - 1].events = events;

    return watchFd->handle;
}

void
CompCore::removeWatchFd (CompWatchFdHandle handle)
{
    std::list<CompWatchFd *>::iterator it;
    CompWatchFd                        *w;
    int                                i;

    for (it = priv->watchFds.begin(), i = priv->nWatchFds - 1;
	 it != priv->watchFds.end(); it++, i--)
	if ((*it)->handle == handle)
	    break;

    if (it == priv->watchFds.end())
	return;

    w = (*it);
    priv->watchFds.erase (it);

    priv->nWatchFds--;

    if (i < priv->nWatchFds)
	memmove (&priv->watchPollFds[i], &priv->watchPollFds[i + 1],
		 (priv->nWatchFds - i) * sizeof (struct pollfd));

    delete w;
}

short int
PrivateCore::watchFdEvents (CompWatchFdHandle handle)
{
    std::list<CompWatchFd *>::iterator it;
    int                                i;

    for (it = watchFds.begin(), i = nWatchFds - 1; it != watchFds.end();
	 it++, i--)
	if ((*it)->handle == handle)
	    return watchPollFds[i].revents;

    return 0;
}

int
PrivateCore::doPoll (int timeout)
{
    int rv;

    rv = poll (watchPollFds, nWatchFds, timeout);
    if (rv)
    {
	std::list<CompWatchFd *>::iterator it;
	int                                i;

	for (it = watchFds.begin(), i = nWatchFds - 1; it != watchFds.end();
	    it++, i--)
	    if (watchPollFds[i].revents != 0 && (*it)->callBack)
		(*(*it)->callBack) ((*it)->closure);
    }

    return rv;
}

void
PrivateCore::handleTimers (struct timeval *tv)
{
    CompCore::Timer                        *t;
    int		                           timeDiff;
    std::list<CompCore::Timer *>::iterator it;

    timeDiff = TIMEVALDIFF (tv, &lastTimeout);

    /* handle clock rollback */
    if (timeDiff < 0)
	timeDiff = 0;

    for (it = timers.begin(); it != timers.end(); it++)
    {
	t = (*it);
	t->mMinLeft -= timeDiff;
	t->mMaxLeft -= timeDiff;
    }

    while (timers.begin() != timers.end() &&
	   (*timers.begin())->mMinLeft <= 0)
    {
	t = (*timers.begin());
	timers.pop_front();

	t->mActive = false;
	if (t->mCallBack ())
	{
	    addTimer (t);
	    t->mActive = true;
	}
    }

    lastTimeout = *tv;
}


void
CompCore::fileWatchAdded (CompFileWatch *watch)
    WRAPABLE_HND_FUNC(fileWatchAdded, watch)

void
CompCore::fileWatchRemoved (CompFileWatch *watch)
    WRAPABLE_HND_FUNC(fileWatchRemoved, watch)

bool
CompCore::initPluginForObject (CompPlugin *plugin, CompObject *object)
{
    WRAPABLE_HND_FUNC_RETURN(bool, initPluginForObject, plugin, object)
    return true;
}

void
CompCore::finiPluginForObject (CompPlugin *plugin, CompObject *object)
    WRAPABLE_HND_FUNC(finiPluginForObject, plugin, object)

	
bool
CompCore::setOptionForPlugin (CompObject        *object,
			      const char        *plugin,
			      const char        *name,
			      CompOption::Value &value)
{
    WRAPABLE_HND_FUNC_RETURN(bool, setOptionForPlugin,
			     object, plugin, name, value)

    CompPlugin *p = CompPlugin::find (plugin);
    if (p)
	return p->vTable->setObjectOption (object, name, value);

    return false;
}

void
CompCore::objectAdd (CompObject *parent, CompObject *object)
    WRAPABLE_HND_FUNC(objectAdd, parent, object)


void
CompCore::objectRemove (CompObject *parent, CompObject *object)
    WRAPABLE_HND_FUNC(objectRemove, parent, object)

void
CompCore::sessionEvent (CompSession::Event event,
			CompOption::Vector &arguments)
    WRAPABLE_HND_FUNC(sessionEvent, event, arguments)

CoreInterface::CoreInterface ()
{
    WRAPABLE_INIT_FUNC(fileWatchAdded);
    WRAPABLE_INIT_FUNC(fileWatchRemoved);
    WRAPABLE_INIT_FUNC(initPluginForObject);
    WRAPABLE_INIT_FUNC(finiPluginForObject);
    WRAPABLE_INIT_FUNC(setOptionForPlugin);
    WRAPABLE_INIT_FUNC(objectAdd);
    WRAPABLE_INIT_FUNC(objectRemove);
    WRAPABLE_INIT_FUNC(sessionEvent);
}

void
CoreInterface::fileWatchAdded (CompFileWatch *watch)
    WRAPABLE_DEF_FUNC(fileWatchAdded, watch)

void
CoreInterface::fileWatchRemoved (CompFileWatch *watch)
    WRAPABLE_DEF_FUNC(fileWatchRemoved, watch)

bool
CoreInterface::initPluginForObject (CompPlugin *plugin, CompObject *object)
    WRAPABLE_DEF_FUNC_RETURN(initPluginForObject, plugin, object)

void
CoreInterface::finiPluginForObject (CompPlugin *plugin, CompObject *object)
    WRAPABLE_DEF_FUNC(finiPluginForObject, plugin, object)

	
bool
CoreInterface::setOptionForPlugin (CompObject        *object,
				   const char        *plugin,
				   const char	     *name,
				   CompOption::Value &value)
    WRAPABLE_DEF_FUNC_RETURN(setOptionForPlugin,
			     object, plugin, name, value)

void
CoreInterface::objectAdd (CompObject *parent, CompObject *object)
    WRAPABLE_DEF_FUNC(objectAdd, parent, object)

void
CoreInterface::objectRemove (CompObject *parent, CompObject *object)
    WRAPABLE_DEF_FUNC(objectRemove, parent, object)

void
CoreInterface::sessionEvent (CompSession::Event event,
			     CompOption::Vector &arguments)
    WRAPABLE_DEF_FUNC(sessionEvent, event, arguments)

	
PrivateCore::PrivateCore (CompCore *core) :
    core (core),
    displays (0),
    fileWatch (0),
    lastFileWatchHandle (1),
    timers (0),
    watchFds (0),
    lastWatchFdHandle (1),
    watchPollFds (0),
    nWatchFds (0)
{
    gettimeofday (&lastTimeout, 0);
}

PrivateCore::~PrivateCore ()
{
}

CompCore::Timer::Timer () :
    mActive (false),
    mMinTime (0),
    mMaxTime (0),
    mMinLeft (0),
    mMaxLeft (0),
    mCallBack (NULL)
{
}

CompCore::Timer::~Timer ()
{
    if (mActive)
	core->priv->removeTimer (this);
}

void
CompCore::Timer::setTimes (unsigned int min, unsigned int max)
{
    bool wasActive = mActive;
    if (mActive)
	stop ();
    mMinTime = min;
    mMaxTime = (min <= max)? max : min;

    if (wasActive)
	start ();
}
	
void
CompCore::Timer::setCallback (CompCore::Timer::CallBack callback)
{
    bool wasActive = mActive;
    if (mActive)
	stop ();
    mCallBack = callback;

    if (wasActive)
	start ();
}

void
CompCore::Timer::start ()
{
    stop ();
    mActive = true;
    core->priv->addTimer (this);
}

void
CompCore::Timer::start (unsigned int min, unsigned int max)
{
    stop ();
    setTimes (min, max);
    start ();
}

void
CompCore::Timer::start (CompCore::Timer::CallBack callback,
			unsigned int min, unsigned int max)
{
    stop ();
    setTimes (min, max);
    setCallback (callback);
    start ();
}

void
CompCore::Timer::stop ()
{
    mActive = false;
    core->priv->removeTimer (this);
}

unsigned int
CompCore::Timer::minTime ()
{
    return mMinTime;
}

unsigned int
CompCore::Timer::maxTime ()
{
    return mMaxTime;
}

unsigned int
CompCore::Timer::minLeft ()
{
    return (mMinLeft < 0)? 0 : mMinLeft;
}

unsigned int
CompCore::Timer::maxLeft ()
{
    return (mMaxLeft < 0)? 0 : mMaxLeft;
}

bool
CompCore::Timer::active ()
{
    return mActive;
}