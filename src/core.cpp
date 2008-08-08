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

    CompPlugin *corePlugin = loadPlugin ("core");
    if (!corePlugin)
    {
	compLogMessage (0, "core", CompLogLevelFatal,
			"Couldn't load core plugin");
	return false;
    }

    if (!pushPlugin (corePlugin))
    {
	compLogMessage (0, "core", CompLogLevelFatal,
			"Couldn't activate core plugin");
	return false;
    }

    return true;
}

CompCore::~CompCore ()
{
    CompPlugin *p;

    while (priv->displays)
	removeDisplay (priv->displays);

    if (priv->watchPollFds)
	free (priv->watchPollFds);

    while ((p = popPlugin ()))
	unloadPlugin (p);

}

CompString
CompCore::name ()
{
    return CompString ("");
}


CompDisplay *
CompCore::displays()
{
    return priv->displays;
}

bool
CompCore::addDisplay (const char *name)
{

    CompDisplay *prev;
    CompDisplay *d = new CompDisplay();

    if (!d)
	return false;

    for (prev = priv->displays; prev && prev->next; prev = prev->next);

    if (prev)
	prev->next = d;
    else
	priv->displays = d;

    if (!d->init (name))
    {
	if (prev)
	    prev->next = NULL;
    	else
	    priv->displays = NULL;
	delete d;
	return false;
    }
    return true;
}

void
CompCore::removeDisplay (CompDisplay *d)
{
    CompDisplay *p;

    for (p = priv->displays; p; p = p->next)
	if (p->next == d)
	    break;

    if (p)
	p->next = d->next;
    else
	priv->displays = NULL;

    delete d;
}

void
CompCore::eventLoop ()
{
    struct timeval tv;
    CompDisplay    *d;
    CompTimeout    *t;
    int		   time;

    for (d = priv->displays; d; d = d->next)
	d->setWatchFdHandle (addWatchFd (ConnectionNumber (d->dpy()),
			     		 POLLIN, NULL, NULL));

    for (;;)
    {
	if (restartSignal || shutDown)
	    break;

	for (d = priv->displays; d; d = d->next)
	{
	    d->processEvents ();
	}

	if (!priv->timeouts.empty())
	{
	    gettimeofday (&tv, 0);
	    priv->handleTimeouts (&tv);

	    if ((*priv->timeouts.begin())->minLeft > 0)
	    {
		std::list<CompTimeout *>::iterator it = priv->timeouts.begin();

		t = (*it);
		time = t->maxLeft;
		while (it != priv->timeouts.end())
		{
		    t = (*it);
		    if (t->minLeft <= time)
			break;
		    if (t->maxLeft < time)
			time = t->maxLeft;
		    it++;
		}
		priv->doPoll (time);
	    }
	}
	else
	{
	    priv->doPoll (-1);
	}
    }

    for (d = priv->displays; d; d = d->next)
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
PrivateCore::addTimeout (CompTimeout *timeout)
{
    std::list<CompTimeout *>::iterator it;

    for (it = timeouts.begin(); it != timeouts.end(); it++)
    {
	if (timeout->minTime < (*it)->minLeft)
	    break;
    }

    timeout->minLeft = timeout->minTime;
    timeout->maxLeft = timeout->maxTime;

    timeouts.insert (it, timeout);
}

CompTimeoutHandle
CompCore::addTimeout (int	     minTime,
		      int	     maxTime,
		      CallBackProc   callBack,
		      void	     *closure)
{
    CompTimeout *timeout = new CompTimeout();

    if (!timeout)
	return 0;

    timeout->minTime  = minTime;
    timeout->maxTime  = (maxTime >= minTime) ? maxTime : minTime;
    timeout->callBack = callBack;
    timeout->closure  = closure;
    timeout->handle   = priv->lastTimeoutHandle++;

    if (priv->lastTimeoutHandle == MAXSHORT)
	priv->lastTimeoutHandle = 1;

    priv->addTimeout (timeout);

    return timeout->handle;
}

void *
CompCore::removeTimeout (CompTimeoutHandle handle)
{
    std::list<CompTimeout *>::iterator it;
    CompTimeout                        *t;
    void                               *closure = NULL;

    for (it = priv->timeouts.begin(); it != priv->timeouts.end(); it++)
	if ((*it)->handle == handle)
	    break;

    if (it == priv->timeouts.end())
	return NULL;

    t = (*it);
    priv->timeouts.erase (it);

    closure = t->closure;

    delete t;

    return closure;
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
PrivateCore::handleTimeouts (struct timeval *tv)
{
    CompTimeout                        *t;
    int		                       timeDiff;
    std::list<CompTimeout *>::iterator it;

    timeDiff = TIMEVALDIFF (tv, &lastTimeout);

    /* handle clock rollback */
    if (timeDiff < 0)
	timeDiff = 0;

    for (it = timeouts.begin(); it != timeouts.end(); it++)
    {
	t = (*it);
	t->minLeft -= timeDiff;
	t->maxLeft -= timeDiff;
    }

    while (timeouts.begin() != timeouts.end() &&
	   (*timeouts.begin())->minLeft <= 0)
    {
	t = (*timeouts.begin());
	timeouts.pop_front();

	if ((*t->callBack) (t->closure))
	{
	    addTimeout (t);
	}
	else
	{
	    delete t;
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
CompCore::setOptionForPlugin (CompObject      *object,
			      const char      *plugin,
			      const char      *name,
			      CompOptionValue *value)
{
    WRAPABLE_HND_FUNC_RETURN(bool, setOptionForPlugin,
			     object, plugin, name, value)

    CompPlugin *p = findActivePlugin (plugin);
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
CompCore::sessionEvent (CompSessionEvent event,
			     CompOption       *arguments,
			     unsigned int     nArguments)
    WRAPABLE_HND_FUNC(sessionEvent, event, arguments, nArguments)

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
CoreInterface::setOptionForPlugin (CompObject      *object,
				   const char      *plugin,
				   const char	   *name,
				   CompOptionValue *value)
    WRAPABLE_DEF_FUNC_RETURN(setOptionForPlugin,
			     object, plugin, name, value)

void
CoreInterface::objectAdd (CompObject *parent, CompObject *object)
    WRAPABLE_DEF_FUNC(objectAdd, parent, object)

void
CoreInterface::objectRemove (CompObject *parent, CompObject *object)
    WRAPABLE_DEF_FUNC(objectRemove, parent, object)

void
CoreInterface::sessionEvent (CompSessionEvent event,
			     CompOption       *arguments,
			     unsigned int     nArguments)
    WRAPABLE_DEF_FUNC(sessionEvent, event, arguments, nArguments)

	
PrivateCore::PrivateCore (CompCore *core) :
    core (core),
    displays (0),
    fileWatch (0),
    lastFileWatchHandle (1),
    timeouts (0),
    lastTimeoutHandle (1),
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
