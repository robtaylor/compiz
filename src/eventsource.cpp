/*
 * Copyright © 2010 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Jason Smith <jason.smith@canonical.com>
 * 	      : Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#include "privatescreen.h"

Glib::RefPtr <CompEventSource>
CompEventSource::create ()
{
    return Glib::RefPtr <CompEventSource> (new CompEventSource ());
}

sigc::connection
CompEventSource::connect (const sigc::slot <bool> &slot)
{
    return connect_generic (slot);
}

CompEventSource::CompEventSource () :
    Glib::Source (),
    mDpy (screen->dpy ()),
    mConnectionFD (ConnectionNumber (screen->dpy ()))
{
    mPollFD.set_fd (mConnectionFD);
    mPollFD.set_events (Glib::IO_IN);

    set_priority (G_PRIORITY_DEFAULT);
    add_poll (mPollFD);
    set_can_recurse (true);

    connect (sigc::mem_fun <bool, CompEventSource> (this, &CompEventSource::callback));
}

CompEventSource::~CompEventSource ()
{
}

bool
CompEventSource::callback ()
{
    if (restartSignal || shutDown)
    {
	screen->priv->mainloop->quit ();
	return false;
    }
    else
	screen->priv->processEvents ();
    return true;
}

bool
CompEventSource::prepare (int &timeout)
{
    timeout = -1;
    return XPending (mDpy);
}

bool
CompEventSource::check ()
{
    if (mPollFD.get_revents () & Glib::IO_IN)
	return XPending (mDpy);

    return false;
}

bool
CompEventSource::dispatch (sigc::slot_base *slot)
{
    return (*static_cast <sigc::slot <bool> *> (slot)) ();
}
