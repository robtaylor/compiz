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

#include "private.h"

COMPIZ_PLUGIN_20090315 (glib, GlibPluginVTable);

GlibScreen::GlibScreen (CompScreen *screen) :
    PluginClassHandler <GlibScreen, CompScreen> (screen),
    mFds (0),
    mFdsSize (0),
    mNFds (0),
    mWatch (0)
{
    mTimer.setCallback (boost::bind (&GlibScreen::dispatchAndPrepare, this));

    mNotifyAtom = XInternAtom (screen->dpy (), "_COMPIZ_GLIB_NOTIFY", 0);

    prepare (g_main_context_default ());

    ScreenInterface::setHandler (screen, true);
}

GlibScreen::~GlibScreen ()
{
    dispatch (g_main_context_default ());

    if (mFds)
	delete mFds;
    if (mWatch)
	delete mWatch;
}

bool
GlibPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	return false;

    return true;
}


void
GlibScreen::dispatch (GMainContext *context)
{
    int i;

    g_main_context_check (context, mMaxPriority, mFds, mNFds);
    g_main_context_dispatch (context);

    for (i = 0; i < mNFds; i++)
	screen->removeWatchFd (mWatch[i].handle);
}

void
GlibScreen::prepare (GMainContext *context)
{
    int nFds = 0;
    int timeout = -1;
    int i;

    g_main_context_prepare (context, &mMaxPriority);

    do
    {
	if (nFds > mFdsSize)
	{
	    if (mFds)
		delete mFds;
	    if (mWatch)
		delete mWatch;

	    mFds = new GPollFD [nFds];
	    if (!mFds)
	    {
		mFdsSize = 0;
		break;
	    }

	    mWatch = new GLibWatch [nFds];
	    if (!mWatch)
	    {
		mFdsSize = 0;
		break;
	    }

	    mFdsSize = nFds;
	}

	nFds = g_main_context_query (context,
				     mMaxPriority,
				     &timeout,
				     mFds,
				     mFdsSize);
    } while (nFds > mFdsSize);

    if (timeout < 0)
	timeout = INT_MAX;

    for (i = 0; i < nFds; i++)
    {
	mWatch[i].index   = i;
	mWatch[i].handle  =
	    screen->addWatchFd (mFds[i].fd, mFds[i].events,
				boost::bind (&GlibScreen::collectEvents, this,
					     _1, &mWatch[i]));
    }

    mNFds = nFds;
    mTimer.start (timeout);
}



bool
GlibScreen::dispatchAndPrepare ()
{
    GMainContext *context = g_main_context_default ();

    dispatch (context);
    prepare (context);

    return false;
}

void
GlibScreen::wakeup ()
{
    mTimer.start (0);
}

void
GlibScreen::collectEvents (short int revents,
			   GLibWatch *w)
{
    mFds[w->index].revents |= revents;

    wakeup ();
}

void
GlibScreen::handleEvent (XEvent *event)
{
    if (event->type == ClientMessage)
    {
	if (event->xclient.message_type == mNotifyAtom)
	    wakeup ();
    }

    screen->handleEvent (event);
}
