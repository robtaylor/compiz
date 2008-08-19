/*
 * Copyright © 2005 Novell, Inc.
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

#include <stdlib.h>
#include <string.h>

#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/Xfixes.h>

#include <compiz-core.h>
#include "privatedisplay.h"
#include "privatescreen.h"
#include "privatewindow.h"

static Window xdndWindow = None;
static Window edgeWindow = None;

void
PrivateWindow::handleDamageRect (CompWindow *w,
				 int	   x,
				 int	   y,
				 int	   width,
				 int	   height)
{
    REGION region;
    bool   initial = false;

    if (!w->priv->redirected || w->priv->bindFailed)
	return;

    if (!w->priv->damaged)
    {
	w->priv->damaged = initial = true;
	w->priv->invisible = WINDOW_INVISIBLE (w->priv);
    }

    region.extents.x1 = x;
    region.extents.y1 = y;
    region.extents.x2 = region.extents.x1 + width;
    region.extents.y2 = region.extents.y1 + height;

    if (!w->damageRect (initial, &region.extents))
    {
	region.extents.x1 += w->priv->attrib.x + w->priv->attrib.border_width;
	region.extents.y1 += w->priv->attrib.y + w->priv->attrib.border_width;
	region.extents.x2 += w->priv->attrib.x + w->priv->attrib.border_width;
	region.extents.y2 += w->priv->attrib.y + w->priv->attrib.border_width;

	region.rects = &region.extents;
	region.numRects = region.size = 1;

	w->priv->screen->damageRegion (&region);
    }

    if (initial)
	w->damageOutputExtents ();
}

bool
CompWindow::handleSyncAlarm ()
{
    if (priv->syncWait)
    {
	priv->syncWait = FALSE;

	if (resize (priv->syncGeometry))
	{
	    XRectangle *rects;
	    int	       nDamage;

	    nDamage = priv->nDamage;
	    rects   = priv->damageRects;
	    while (nDamage--)
	    {
		PrivateWindow::handleDamageRect (this,
						 rects[nDamage].x,
						 rects[nDamage].y,
						 rects[nDamage].width,
						 rects[nDamage].height);
	    }

	    priv->nDamage = 0;
	}
	else
	{
	    /* resizeWindow failing means that there is another pending
	       resize and we must send a new sync request to the client */
	    sendSyncRequest ();
	}
    }

    return false;
}


static bool
autoRaiseTimeout (CompDisplay *display)
{
    CompWindow  *w = display->findWindow (display->activeWindow ());

    if (display->autoRaiseWindow () == display->activeWindow () ||
	(w && (display->autoRaiseWindow () == w->transientFor ())))
    {
	w = display->findWindow (display->autoRaiseWindow ());
	if (w)
	    w->updateAttributes (CompStackingUpdateModeNormal);
    }

    return false;
}

#define REAL_MOD_MASK (ShiftMask | ControlMask | Mod1Mask | Mod2Mask | \
		       Mod3Mask | Mod4Mask | Mod5Mask | CompNoMask)

static bool
isCallBackBinding (CompOption	           &option,
		   CompAction::BindingType type,
		   CompAction::State       state)
{
    if (!option.isAction ())
	return false;

    if (!(option.value ().action ().type () & type))
	return false;

    if (!(option.value ().action ().state () & state))
	return false;

    return true;
}

static bool
isInitiateBinding (CompOption	           &option,
		   CompAction::BindingType type,
		   CompAction::State       state,
		   CompAction	           **action)
{
    if (!isCallBackBinding (option, type, state))
	return false;

    if (option.value ().action ().initiate ().empty ())
	return false;

    *action = &option.value ().action ();

    return true;
}

static bool
isTerminateBinding (CompOption	            &option,
		    CompAction::BindingType type,
		    CompAction::State       state,
		    CompAction              **action)
{
    if (!isCallBackBinding (option, type, state))
	return false;

    if (option.value ().action ().terminate ().empty ())
	return false;

    *action = &option.value ().action ();

    return true;
}

bool
PrivateDisplay::triggerButtonPressBindings (CompOption::Vector &options,
					    XEvent             *event,
					    CompOption::Vector &arguments)
{
    CompAction::State state = CompAction::StateInitButton;
    CompAction        *action;
    unsigned int      modMask = REAL_MOD_MASK & ~ignoredModMask;
    unsigned int      bindMods;
    unsigned int      edge = 0;

    if (edgeWindow)
    {
	CompScreen   *s;
	unsigned int i;

	s = display->findScreen (event->xbutton.root);
	if (!s)
	    return false;

	if (event->xbutton.window != edgeWindow)
	{
	    if (!s->hasGrab () || event->xbutton.window != s->root ())
		return false;
	}

	for (i = 0; i < SCREEN_EDGE_NUM; i++)
	{
	    if (edgeWindow == s->screenEdge (i).id)
	    {
		edge = 1 << i;
		arguments[1].value ().set ((int) display->activeWindow ());
		break;
	    }
	}
    }

    foreach (CompOption &option, options)
    {
	if (isInitiateBinding (option, CompAction::BindingTypeButton, state,
			       &action))
	{
	    if (action->button ().button () == event->xbutton.button)
	    {
		bindMods = display->virtualToRealModMask (
		    action->button ().modifiers ());

		if ((bindMods & modMask) == (event->xbutton.state & modMask))
		    if (action->initiate () (display, action, state,
					     arguments))
			return true;
	    }
	}

	if (edge)
	{
	    if (isInitiateBinding (option, CompAction::BindingTypeEdgeButton,
				   state | CompAction::StateInitEdge, &action))
	    {
		if ((action->button ().button () == event->xbutton.button) &&
		    (action->edgeMask () & edge))
		{
		    bindMods = display->virtualToRealModMask (
			action->button ().modifiers ());

		    if ((bindMods & modMask) ==
			(event->xbutton.state & modMask))
			if (action->initiate () (display, action, state |
						 CompAction::StateInitEdge,
						 arguments))
			    return true;
		}
	    }
	}
    }

    return false;
}

bool
PrivateDisplay::triggerButtonReleaseBindings (CompOption::Vector &options,
					      XEvent             *event,
					      CompOption::Vector &arguments)
{
    CompAction::State       state = CompAction::StateTermButton;
    CompAction::BindingType type  = CompAction::BindingTypeButton |
				    CompAction::BindingTypeEdgeButton;
    CompAction	            *action;

    foreach (CompOption &option, options)
    {
	if (isTerminateBinding (option, type, state, &action))
	{
	    if (action->button ().button () == event->xbutton.button)
	    {
		if (action->terminate () (display, action, state,
					  arguments))
		    return true;
	    }
	}
    }

    return false;
}

bool
PrivateDisplay::triggerKeyPressBindings (CompOption::Vector &options,
					 XEvent             *event,
					 CompOption::Vector &arguments)
{
    CompAction::State state = 0;
    CompAction	      *action;
    unsigned int      modMask = REAL_MOD_MASK & ~ignoredModMask;
    unsigned int      bindMods;

    if (event->xkey.keycode == escapeKeyCode)
	state = CompAction::StateCancel;
    else if (event->xkey.keycode == returnKeyCode)
	state = CompAction::StateCommit;

    if (state)
    {
	foreach (CompOption &o, options)
	{
	    if (o.isAction ())
	    {
		if (!o.value ().action ().terminate ().empty ())
		    o.value ().action ().terminate () (display,
						       &o.value ().action (),
						       state, noOptions);
	    }
	}

	if (state == CompAction::StateCancel)
	    return false;
    }

    state = CompAction::StateInitKey;
    foreach (CompOption &option, options)
    {
	if (isInitiateBinding (option, CompAction::BindingTypeKey,
			       state, &action))
	{
	    bindMods = display->virtualToRealModMask (
		action->key ().modifiers ());

	    if (action->key ().keycode () == event->xkey.keycode)
	    {
		if ((bindMods & modMask) == (event->xkey.state & modMask))
		    if (action->initiate () (display, action, state,
					     arguments))
			return true;
	    }
	    else if (!xkbEvent && action->key ().keycode () == 0)
	    {
		if (bindMods == (event->xkey.state & modMask))
		    if (action->initiate () (display, action, state,
					     arguments))
			return true;
	    }
	}
    }

    return false;
}

bool
PrivateDisplay::triggerKeyReleaseBindings (CompOption::Vector &options,
					   XEvent             *event,
					   CompOption::Vector &arguments)
{
    if (!xkbEvent)
    {
	CompAction::State state = CompAction::StateTermKey;
	CompAction        *action;
	unsigned int      modMask = REAL_MOD_MASK & ~ignoredModMask;
	unsigned int      bindMods;
	unsigned int      mods;

	mods = display->keycodeToModifiers (event->xkey.keycode);
	if (mods == 0)
	    return false;

	foreach (CompOption &option, options)
	{
	    if (isTerminateBinding (option, CompAction::BindingTypeKey,
				    state, &action))
	    {
		bindMods =
		    display->virtualToRealModMask (action->key ().modifiers ());

		if ((mods & modMask & bindMods) != bindMods)
		{
		    if (action->terminate () (display, action, state,
					      arguments))
			return true;
		}
	    }
	}
    }

    return false;
}

bool
PrivateDisplay::triggerStateNotifyBindings (CompOption::Vector  &options,
					    XkbStateNotifyEvent *event,
					    CompOption::Vector  &arguments)
{
    CompAction::State state;
    CompAction        *action;
    unsigned int      modMask = REAL_MOD_MASK & ~ignoredModMask;
    unsigned int      bindMods;

    if (event->event_type == KeyPress)
    {
	state = CompAction::StateInitKey;

	foreach (CompOption &option, options)
	{
	    if (isInitiateBinding (option, CompAction::BindingTypeKey,
				   state, &action))
	    {
		if (action->key ().keycode () == 0)
		{
		    bindMods = display->virtualToRealModMask (
			action->key ().modifiers ());

		    if ((event->mods & modMask & bindMods) == bindMods)
		    {
			if (action->initiate () (display, action, state,
						 arguments))
			    return true;
		    }
		}
	    }
	}
    }
    else
    {
	state = CompAction::StateTermKey;

	foreach (CompOption &option, options)
	{
	    if (isTerminateBinding (option, CompAction::BindingTypeKey,
				    state, &action))
	    {
		bindMods =
		    display->virtualToRealModMask (action->key ().modifiers ());

		if ((event->mods & modMask & bindMods) != bindMods)
		{
		    if (action->terminate () (display, action, state,
					      arguments))
			return true;
		}
	    }
	}
    }

    return false;
}

static bool
isBellAction (CompOption        &option,
	      CompAction::State state,
	      CompAction        **action)
{
    if (option.type () != CompOption::TypeAction &&
	option.type () != CompOption::TypeBell)
	return false;

    if (!option.value ().action ().bell ())
	return false;

    if (!(option.value ().action ().state () & state))
	return false;

    if (option.value ().action ().initiate ().empty ())
	return false;

    *action = &option.value ().action ();

    return true;
}

static bool
triggerBellNotifyBindings (CompDisplay        *d,
			   CompOption::Vector &options,
			   CompOption::Vector &arguments)
{
    CompAction::State state = CompAction::StateInitBell;
    CompAction        *action;

    foreach (CompOption &option, options)
    {
	if (isBellAction (option, state, &action))
	{
	    if (action->initiate () (d, action, state, arguments))
		return true;
	}
    }

    return false;
}

static bool
isEdgeAction (CompOption        &option,
	      CompAction::State state,
	      unsigned int      edge)
{
    if (option.type () != CompOption::TypeAction &&
	option.type () != CompOption::TypeButton &&
	option.type () != CompOption::TypeEdge)
	return false;

    if (!(option.value ().action ().edgeMask () & edge))
	return false;

    if (!(option.value ().action ().state () & state))
	return false;

    return true;
}

static bool
isEdgeEnterAction (CompOption        &option,
		   CompAction::State state,
		   CompAction::State delayState,
		   unsigned int      edge,
		   CompAction        **action)
{
    if (!isEdgeAction (option, state, edge))
	return false;

    if (option.value ().action ().type () & CompAction::BindingTypeEdgeButton)
	return false;

    if (option.value ().action ().initiate ().empty ())
	return false;

    if (delayState)
    {
	if ((option.value ().action ().state () &
	     CompAction::StateNoEdgeDelay) !=
	    (delayState & CompAction::StateNoEdgeDelay))
	{
	    /* ignore edge actions which shouldn't be delayed when invoking
	       undelayed edges (or vice versa) */
	    return false;
	}
    }


    *action = &option.value ().action ();

    return true;
}

static bool
isEdgeLeaveAction (CompOption        &option,
		   CompAction::State state,
		   unsigned int      edge,
		   CompAction        **action)
{
    if (!isEdgeAction (option, state, edge))
	return false;

    if (option.value ().action ().terminate ().empty ())
	return false;

    *action = &option.value ().action ();

    return true;
}

static bool
triggerEdgeEnterBindings (CompDisplay	     *d,
			  CompOption::Vector &options,
			  CompAction::State  state,
			  CompAction::State  delayState,
			  unsigned int	     edge,
			  CompOption::Vector &arguments)
{
    CompAction *action;

    foreach (CompOption &option, options)
    {
	if (isEdgeEnterAction (option, state, delayState, edge, &action))
	{
	    if (action->initiate () (d, action, state, arguments))
		return true;
	}
    }

    return false;
}

static bool
triggerEdgeLeaveBindings (CompDisplay	     *d,
			  CompOption::Vector &options,
			  CompAction::State  state,
			  unsigned int	     edge,
			  CompOption::Vector &arguments)
{
    CompAction *action;

    foreach (CompOption &option, options)
    {
	if (isEdgeLeaveAction (option, state, edge, &action))
	{
	    if (action->terminate () (d, action, state, arguments))
		return true;
	}
    }

    return false;
}

static bool
triggerAllEdgeEnterBindings (CompDisplay        *d,
			     CompAction::State  state,
			     CompAction::State  delayState,
			     unsigned int       edge,
			     CompOption::Vector &arguments)
{
    CompPlugin *p;

    for (p = getPlugins (); p; p = p->next)
    {
	CompOption::Vector &options = p->vTable->getObjectOptions (d);
	if (triggerEdgeEnterBindings (d, options, state, delayState, edge,
				      arguments))
	{
	    return true;
	}
    }
    return false;
}

static bool
delayedEdgeTimeout (CompDisplay *d, CompDelayedEdgeSettings *settings)
{
    triggerAllEdgeEnterBindings (d,
				 settings->state,
				 ~CompAction::StateNoEdgeDelay,
				 settings->edge,
				 settings->options);

    return false;
}

bool
PrivateDisplay::triggerEdgeEnter (unsigned int       edge,
				  CompAction::State  state,
				  CompOption::Vector &arguments)
{
    int                     delay;

    delay = opt[COMP_DISPLAY_OPTION_EDGE_DELAY].value ().i ();

    if (delay > 0)
    {
	CompAction::State delayState;
	edgeDelaySettings.edge    = edge;
	edgeDelaySettings.state   = state;
	edgeDelaySettings.options = arguments;

	edgeDelayTimer.start  (
	    boost::bind (delayedEdgeTimeout, display, &edgeDelaySettings),
			 delay, (float) delay * 1.2);

	delayState = CompAction::StateNoEdgeDelay;
	if (triggerAllEdgeEnterBindings (display, state, delayState,
					 edge, arguments))
	    return true;
    }
    else
    {
	if (triggerAllEdgeEnterBindings (display, state, 0, edge,
					 arguments))
	    return true;
    }

    return false;
}

bool
PrivateDisplay::handleActionEvent (XEvent *event)
{
    CompObject *obj = display;
    CompOption::Vector o (0);
    CompPlugin *p;

    o.push_back (CompOption ("event_window", CompOption::TypeInt));
    o.push_back (CompOption ("window", CompOption::TypeInt));
    o.push_back (CompOption ("modifiers", CompOption::TypeInt));
    o.push_back (CompOption ("x", CompOption::TypeInt));
    o.push_back (CompOption ("y", CompOption::TypeInt));
    o.push_back (CompOption ("root", CompOption::TypeInt));

    switch (event->type) {
    case ButtonPress:
	o[0].value ().set ((int) event->xbutton.window);
	o[1].value ().set ((int) event->xbutton.window);
	o[2].value ().set ((int) event->xbutton.state);
	o[3].value ().set ((int) event->xbutton.x_root);
	o[4].value ().set ((int) event->xbutton.y_root);
	o[5].value ().set ((int) event->xbutton.root);

	o.push_back (CompOption ("button", CompOption::TypeInt));
	o.push_back (CompOption ("time", CompOption::TypeInt));

	o[6].value ().set ((int) event->xbutton.button);
	o[7].value ().set ((int) event->xbutton.time);

	for (p = getPlugins (); p; p = p->next)
	{
	    CompOption::Vector &options = p->vTable->getObjectOptions (obj);
	    if (triggerButtonPressBindings (options, event, o))
		return true;
	}
	break;
    case ButtonRelease:
	o[0].value ().set ((int) event->xbutton.window);
	o[1].value ().set ((int) event->xbutton.window);
	o[2].value ().set ((int) event->xbutton.state);
	o[3].value ().set ((int) event->xbutton.x_root);
	o[4].value ().set ((int) event->xbutton.y_root);
	o[5].value ().set ((int) event->xbutton.root);

	o.push_back (CompOption ("button", CompOption::TypeInt));
	o.push_back (CompOption ("time", CompOption::TypeInt));

	o[6].value ().set ((int) event->xbutton.button);
	o[7].value ().set ((int) event->xbutton.time);

	for (p = getPlugins (); p; p = p->next)
	{
	    CompOption::Vector &options = p->vTable->getObjectOptions (obj);
	    if (triggerButtonReleaseBindings (options, event, o))
		return true;
	}
	break;
    case KeyPress:
	o[0].value ().set ((int) event->xkey.window);
	o[1].value ().set ((int) activeWindow);
	o[2].value ().set ((int) event->xkey.state);
	o[3].value ().set ((int) event->xkey.x_root);
	o[4].value ().set ((int) event->xkey.y_root);
	o[5].value ().set ((int) event->xkey.root);

	o.push_back (CompOption ("keycode", CompOption::TypeInt));
	o.push_back (CompOption ("time", CompOption::TypeInt));

	o[6].value ().set ((int) event->xkey.keycode);
	o[7].value ().set ((int) event->xkey.time);

	for (p = getPlugins (); p; p = p->next)
	{
	    CompOption::Vector &options = p->vTable->getObjectOptions (obj);
	    if (triggerKeyPressBindings (options, event, o))
		return true;
	}
	break;
    case KeyRelease:
	o[0].value ().set ((int) event->xkey.window);
	o[1].value ().set ((int) activeWindow);
	o[2].value ().set ((int) event->xkey.state);
	o[3].value ().set ((int) event->xkey.x_root);
	o[4].value ().set ((int) event->xkey.y_root);
	o[5].value ().set ((int) event->xkey.root);

	o.push_back (CompOption ("keycode", CompOption::TypeInt));
	o.push_back (CompOption ("time", CompOption::TypeInt));

	o[6].value ().set ((int) event->xkey.keycode);
	o[7].value ().set ((int) event->xkey.time);

	for (p = getPlugins (); p; p = p->next)
	{
	    CompOption::Vector &options = p->vTable->getObjectOptions (obj);
	    if (triggerKeyReleaseBindings (options, event, o))
		return true;
	}
	break;
    case EnterNotify:
	if (event->xcrossing.mode   != NotifyGrab   &&
	    event->xcrossing.mode   != NotifyUngrab &&
	    event->xcrossing.detail != NotifyInferior)
	{
	    CompScreen	      *s;
	    unsigned int      edge, i;
	    CompAction::State state;

	    s = display->findScreen (event->xcrossing.root);
	    if (!s)
		return false;

	    if (edgeDelayTimer.active ())
		edgeDelayTimer.stop ();

	    if (edgeWindow && edgeWindow != event->xcrossing.window)
	    {
		state = CompAction::StateTermEdge;
		edge  = 0;

		for (i = 0; i < SCREEN_EDGE_NUM; i++)
		{
		    if (edgeWindow == s->screenEdge (i).id)
		    {
			edge = 1 << i;
			break;
		    }
		}

		edgeWindow = None;

		o[0].value ().set ((int) event->xcrossing.window);
		o[1].value ().set ((int) activeWindow);
		o[2].value ().set ((int) event->xcrossing.state);
		o[3].value ().set ((int) event->xcrossing.x_root);
		o[4].value ().set ((int) event->xcrossing.y_root);
		o[5].value ().set ((int) event->xcrossing.root);

		o.push_back (CompOption ("time", CompOption::TypeInt));
		o[6].value ().set ((int) event->xcrossing.time);

		for (p = getPlugins (); p; p = p->next)
		{
		    CompOption::Vector &options =
			p->vTable->getObjectOptions (obj);
		    if (triggerEdgeLeaveBindings (display, options,
						  state, edge, o))
			return true;
		}
	    }

	    edge = 0;

	    for (i = 0; i < SCREEN_EDGE_NUM; i++)
	    {
		if (event->xcrossing.window == s->screenEdge (i).id)
		{
		    edge = 1 << i;
		    break;
		}
	    }

	    if (edge)
	    {
		state = CompAction::StateInitEdge;

		edgeWindow = event->xcrossing.window;

		o[0].value ().set ((int) event->xcrossing.window);
		o[1].value ().set ((int) activeWindow);
		o[2].value ().set ((int) event->xcrossing.state);
		o[3].value ().set ((int) event->xcrossing.x_root);
		o[4].value ().set ((int) event->xcrossing.y_root);
		o[5].value ().set ((int) event->xcrossing.root);

		o.push_back (CompOption ("time", CompOption::TypeInt));
		o[6].value ().set ((int) event->xcrossing.time);

		if (triggerEdgeEnter (edge, state, o))
		    return true;
	    }
	}
	break;
    case ClientMessage:
	if (event->xclient.message_type == atoms.xdndEnter)
	{
	    xdndWindow = event->xclient.window;
	}
	else if (event->xclient.message_type == atoms.xdndLeave)
	{
	    unsigned int      edge = 0;
	    CompAction::State state;
	    Window            root = None;

	    if (!xdndWindow)
	    {
		CompWindow *w;

		w = display->findWindow (event->xclient.window);
		if (w)
		{
		    CompScreen   *s = w->screen ();
		    unsigned int i;

		    for (i = 0; i < SCREEN_EDGE_NUM; i++)
		    {
			if (event->xclient.window == s->screenEdge (i).id)
			{
			    edge = 1 << i;
			    root = s->root ();
			    break;
			}
		    }
		}
	    }

	    if (edge)
	    {
		state = CompAction::StateTermEdgeDnd;

		o[0].value ().set ((int) event->xclient.window);
		o[1].value ().set ((int) activeWindow);
		o[2].value ().set ((int) 0); /* fixme */
		o[3].value ().set ((int) 0); /* fixme */
		o[4].value ().set ((int) 0); /* fixme */
		o[5].value ().set ((int) root);

		for (p = getPlugins (); p; p = p->next)
		{
		    CompOption::Vector &options =
			p->vTable->getObjectOptions (obj);
		    if (triggerEdgeLeaveBindings (display, options,
						  state, edge, o))
			return true;
		}
	    }
	}
	else if (event->xclient.message_type == atoms.xdndPosition)
	{
	    unsigned int      edge = 0;
	    CompAction::State state;
	    Window            root = None;

	    if (xdndWindow == event->xclient.window)
	    {
		CompWindow *w;

		w = display->findWindow (event->xclient.window);
		if (w)
		{
		    CompScreen   *s = w->screen ();
		    unsigned int i;

		    for (i = 0; i < SCREEN_EDGE_NUM; i++)
		    {
			if (xdndWindow == s->screenEdge (i).id)
			{
			    edge = 1 << i;
			    root = s->root ();
			    break;
			}
		    }
		}
	    }

	    if (edge)
	    {
		state = CompAction::StateInitEdgeDnd;

		o[0].value ().set ((int) event->xclient.window);
		o[1].value ().set ((int) activeWindow);
		o[2].value ().set ((int) 0); /* fixme */
		o[3].value ().set ((int) event->xclient.data.l[2] >> 16);
		o[4].value ().set ((int) event->xclient.data.l[2] & 0xffff);
		o[5].value ().set ((int) root);

		if (triggerEdgeEnter (edge, state, o))
		    return true;
	    }

	    xdndWindow = None;
	}
	break;
    default:
	if (event->type == xkbEvent)
	{
	    XkbAnyEvent *xkbEvent = (XkbAnyEvent *) event;

	    if (xkbEvent->xkb_type == XkbStateNotify)
	    {
		XkbStateNotifyEvent *stateEvent = (XkbStateNotifyEvent *) event;

		o[0].value ().set ((int) activeWindow);
		o[1].value ().set ((int) activeWindow);
		o[2].value ().set ((int) stateEvent->mods);

		o[3] = CompOption ("time", CompOption::TypeInt);
		o[3].value ().set ((int) xkbEvent->time);

		o.resize (4);

		for (p = getPlugins (); p; p = p->next)
		{
		    CompOption::Vector &options =
			p->vTable->getObjectOptions (obj);
		    if (triggerStateNotifyBindings (options, stateEvent, o))
			return true;
		}
	    }
	    else if (xkbEvent->xkb_type == XkbBellNotify)
	    {
		o[0].value ().set ((int) activeWindow);
		o[1].value ().set ((int) activeWindow);

		o[2] = CompOption ("time", CompOption::TypeInt);
		o[2].value ().set ((int) xkbEvent->time);

		o.resize (3);

		for (p = getPlugins (); p; p = p->next)
		{
		    CompOption::Vector &options =
			p->vTable->getObjectOptions (obj);
		    if (triggerBellNotifyBindings (display, options, o))
			return true;
		}
	    }
	}
	break;
    }

    return false;
}

void
CompScreen::handleExposeEvent (XExposeEvent *event)
{
    if (priv->output == event->window)
	return;

    priv->exposeRects.push_back (CompRect (event->x, event->x + event->width,
				 	   event->y, event->y + event->height));

    if (event->count == 0)
    {
	CompRect rect;
	while (!priv->exposeRects.empty())
	{
	    rect = priv->exposeRects.front ();
	    priv->exposeRects.pop_front ();

	    damageRegion (rect.region ());
	}
    }
}

void
CompDisplay::handleCompizEvent (const char  *plugin,
				const char  *event,
				CompOption  *option,
				int         nOption)
    WRAPABLE_HND_FUNC(handleCompizEvent, plugin, event, option, nOption)

void
CompDisplay::handleEvent (XEvent *event)
{
    WRAPABLE_HND_FUNC(handleEvent, event)

    CompScreen *s;
    CompWindow *w;

    switch (event->type) {
    case ButtonPress:
	s = findScreen (event->xbutton.root);
	if (s)
	    s->setCurrentOutput (
		s->outputDeviceForPoint (event->xbutton.x_root,
					 event->xbutton.y_root));
	break;
    case MotionNotify:
	s = findScreen (event->xmotion.root);
	if (s)
	    s->setCurrentOutput (
		s->outputDeviceForPoint (event->xmotion.x_root,
					 event->xmotion.y_root));
	break;
    case KeyPress:
	w = findWindow (priv->activeWindow);
	if (w)
	    w->screen ()->setCurrentOutput (w->outputDevice ());
    default:
	break;
    }

    if (priv->handleActionEvent (event))
    {
	if (!priv->screens->hasGrab ())
	    XAllowEvents (priv->dpy, AsyncPointer, event->xbutton.time);

	return;
    }

    switch (event->type) {
    case Expose:
	for (s = priv->screens; s; s = s->next)
	    s->handleExposeEvent (&event->xexpose);
	break;
    case SelectionRequest:
	priv->handleSelectionRequest (event);
	break;
    case SelectionClear:
	priv->handleSelectionClear (event);
	break;
    case ConfigureNotify:
	w = findWindow (event->xconfigure.window);
	if (w)
	{
	    w->configure (&event->xconfigure);
	}
	else
	{
	    s = findScreen (event->xconfigure.window);
	    if (s)
		s->configure (&event->xconfigure);
	}
	break;
    case CreateNotify:
	s = findScreen (event->xcreatewindow.parent);
	if (s)
	{
	    /* The first time some client asks for the composite
	     * overlay window, the X server creates it, which causes
	     * an errorneous CreateNotify event.  We catch it and
	     * ignore it. */
	    if (s->overlay () != event->xcreatewindow.window)
		new CompWindow (s, event->xcreatewindow.window,
				s->getTopWindow ());
	}
	break;
    case DestroyNotify:
	w = findWindow (event->xdestroywindow.window);
	if (w)
	{
	    w->moveInputFocusToOtherWindow ();
	    w->destroy ();
	}
	break;
    case MapNotify:
	w = findWindow (event->xmap.window);
	if (w)
	{
	    if (!w->attrib ().override_redirect)
		w->managed () = true;

	    /* been shaded */
	    if (w->height () == 0)
	    {
		if (w->id () == priv->activeWindow)
		    w->moveInputFocusTo ();
	    }

	    w->map ();
	}
	break;
    case UnmapNotify:
	w = findWindow (event->xunmap.window);
	if (w)
	{
	    /* Normal -> Iconic */
	    if (w->pendingUnmaps ())
	    {
		setWmState (IconicState, w->id ());
		w->pendingUnmaps ()--;
	    }
	    else /* X -> Withdrawn */
	    {
		/* Iconic -> Withdrawn */
		if (w->state () & CompWindowStateHiddenMask)
		{
		    w->minimized () = false;

		    w->changeState (w->state () & ~CompWindowStateHiddenMask);

		    w->screen () ->updateClientList ();
		}

		if (!w->attrib ().override_redirect)
		    setWmState (WithdrawnState, w->id ());

		w->placed  () = false;
		w->managed () = false;
	    }

	    w->unmap ();

	    if (!w->shaded ())
		w->moveInputFocusToOtherWindow ();
	}
	break;
    case ReparentNotify:
	w = findWindow (event->xreparent.window);
	s = findScreen (event->xreparent.parent);
	if (s && !w)
	{
	    new CompWindow (s, event->xreparent.window, s->getTopWindow ());
	}
	else if (w)
	{
	    /* This is the only case where a window is removed but not
	       destroyed. We must remove our event mask and all passive
	       grabs. */
	    XSelectInput (priv->dpy, w->id (), NoEventMask);
	    XShapeSelectInput (priv->dpy, w->id (), NoEventMask);
	    XUngrabButton (priv->dpy, AnyButton, AnyModifier, w->id ());

	    w->moveInputFocusToOtherWindow ();

	    w->destroy ();
	}
	break;
    case CirculateNotify:
	w = findWindow (event->xcirculate.window);
	if (w)
	    w->circulate (&event->xcirculate);
	break;
    case ButtonPress:
	s = findScreen (event->xbutton.root);
	if (s)
	{
	    if (event->xbutton.button == Button1 ||
		event->xbutton.button == Button2 ||
		event->xbutton.button == Button3)
	    {
		w = s->findTopLevelWindow (event->xbutton.window);
		if (w)
		{
		    if (priv->opt[COMP_DISPLAY_OPTION_RAISE_ON_CLICK].
			value ().b ())
			w->updateAttributes (
					CompStackingUpdateModeAboveFullscreen);

		    if (w->id () != priv->activeWindow)
			if (!(w->type () & CompWindowTypeDockMask))
			    w->moveInputFocusTo ();
		}
	    }

	    if (!s->hasGrab ())
		XAllowEvents (priv->dpy, ReplayPointer, event->xbutton.time);
	}
	break;
    case PropertyNotify:
	if (event->xproperty.atom == priv->atoms.winType)
	{
	    w = findWindow (event->xproperty.window);
	    if (w)
	    {
		unsigned int type;

		type = getWindowType (w->id ());

		if (type != w->wmType ())
		{
		    if (w->attrib ().map_state == IsViewable)
		    {
			if (w->type () == CompWindowTypeDesktopMask)
			    w->screen ()->desktopWindowCount ()--;
			else if (type == CompWindowTypeDesktopMask)
			    w->screen ()->desktopWindowCount ()++;
		    }

		    w->wmType () = type;

		    w->recalcType ();
		    w->recalcActions ();

		    if (type & (CompWindowTypeDockMask |
				CompWindowTypeDesktopMask))
			w->setDesktop (0xffffffff);

		    w->screen ()->updateClientList ();

		    matchPropertyChanged (w);
		}
	    }
	}
	else if (event->xproperty.atom == priv->atoms.winState)
	{
	    w = findWindow (event->xproperty.window);
	    if (w && !w->managed ())
	    {
		unsigned int state;

		state = getWindowState (w->id ());
		state = CompWindow::constrainWindowState (state, w->actions ());

		if (state != w->state ())
		{
		    w->state () = state;

		    w->recalcType ();
		    w->recalcActions ();

		    matchPropertyChanged (w);
		}
	    }
	}
	else if (event->xproperty.atom == XA_WM_NORMAL_HINTS)
	{
	    w = findWindow (event->xproperty.window);
	    if (w)
	    {
		w->updateNormalHints ();
		w->recalcActions ();
	    }
	}
	else if (event->xproperty.atom == XA_WM_HINTS)
	{
	    w = findWindow (event->xproperty.window);
	    if (w)
		w->updateWmHints ();
	}
	else if (event->xproperty.atom == XA_WM_TRANSIENT_FOR)
	{
	    w = findWindow (event->xproperty.window);
	    if (w)
	    {
		w->updateTransientHint ();
		w->recalcActions ();
	    }
	}
	else if (event->xproperty.atom == priv->atoms.wmClientLeader)
	{
	    w = findWindow (event->xproperty.window);
	    if (w)
		w->clientLeader () = w->getClientLeader ();
	}
	else if (event->xproperty.atom == priv->atoms.wmIconGeometry)
	{
	    w = findWindow (event->xproperty.window);
	    if (w)
		w->updateIconGeometry ();
	}
	else if (event->xproperty.atom == priv->atoms.winOpacity)
	{
	    w = findWindow (event->xproperty.window);
	    if (w)
		w->updateOpacity ();
	}
	else if (event->xproperty.atom == priv->atoms.winBrightness)
	{
	    w = findWindow (event->xproperty.window);
	    if (w)
		w->updateBrightness ();
	}
	else if (event->xproperty.atom == priv->atoms.winSaturation)
	{
	    w = findWindow (event->xproperty.window);
	    if (w)
		w->updateSaturation ();
	}
	else if (event->xproperty.atom == priv->atoms.xBackground[0] ||
		 event->xproperty.atom == priv->atoms.xBackground[1])
	{
	    s = findScreen (event->xproperty.window);
	    if (s)
		s->updateBackground ();
	}
	else if (event->xproperty.atom == priv->atoms.wmStrut ||
		 event->xproperty.atom == priv->atoms.wmStrutPartial)
	{
	    w = findWindow (event->xproperty.window);
	    if (w)
	    {
		if (w->updateStruts ())
		    w->screen ()->updateWorkarea ();
	    }
	}
	else if (event->xproperty.atom == priv->atoms.mwmHints)
	{
	    w = findWindow (event->xproperty.window);
	    if (w)
		w->updateMwmHints ();
	}
	else if (event->xproperty.atom == priv->atoms.wmProtocols)
	{
	    w = findWindow (event->xproperty.window);
	    if (w)
		w->protocols () = getProtocols (w->id ());
	}
	else if (event->xproperty.atom == priv->atoms.wmIcon)
	{
	    w = findWindow (event->xproperty.window);
	    if (w)
		w->freeIcons ();
	}
	else if (event->xproperty.atom == priv->atoms.startupId)
	{
	    w = findWindow (event->xproperty.window);
	    if (w)
		w->updateStartupId ();
	}
	else if (event->xproperty.atom == XA_WM_CLASS)
	{
	    w = findWindow (event->xproperty.window);
	    if (w)
		w->updateClassHints ();
	}
	break;
    case MotionNotify:
	break;
    case ClientMessage:
	if (event->xclient.message_type == priv->atoms.winActive)
	{
	    w = findWindow (event->xclient.window);
	    if (w)
	    {
		/* use focus stealing prevention if request came from an
		   application (which means data.l[0] is 1 */
		if (event->xclient.data.l[0] != 1 ||
		    w->allowWindowFocus (0, event->xclient.data.l[1]))
		{
		    w->activate ();
		}
	    }
	}
	else if (event->xclient.message_type == priv->atoms.winOpacity)
	{
	    w = findWindow (event->xclient.window);
	    if (w && (w->type () & CompWindowTypeDesktopMask) == 0)
	    {
		GLushort opacity = event->xclient.data.l[0] >> 16;

		setWindowProp32 (w->id (), priv->atoms.winOpacity, opacity);
	    }
	}
	else if (event->xclient.message_type == priv->atoms.winBrightness)
	{
	    w = findWindow (event->xclient.window);
	    if (w)
	    {
		GLushort brightness = event->xclient.data.l[0] >> 16;

		setWindowProp32 (w->id (), priv->atoms.winBrightness,
				 brightness);
	    }
	}
	else if (event->xclient.message_type == priv->atoms.winSaturation)
	{
	    w = findWindow (event->xclient.window);
	    if (w)
	    {
		GLushort saturation = event->xclient.data.l[0] >> 16;

		setWindowProp32 (w->id (), priv->atoms.winSaturation,
				 saturation);
	    }
	}
	else if (event->xclient.message_type == priv->atoms.winState)
	{
	    w = findWindow (event->xclient.window);
	    if (w)
	    {
		unsigned long wState, state;
		int	      i;

		wState = w->state ();

		for (i = 1; i < 3; i++)
		{
		    state = windowStateMask (event->xclient.data.l[i]);
		    if (state & ~CompWindowStateHiddenMask)
		    {

#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD    1
#define _NET_WM_STATE_TOGGLE 2

			switch (event->xclient.data.l[0]) {
			case _NET_WM_STATE_REMOVE:
			    wState &= ~state;
			    break;
			case _NET_WM_STATE_ADD:
			    wState |= state;
			    break;
			case _NET_WM_STATE_TOGGLE:
			    wState ^= state;
			    break;
			}
		    }
		}

		wState = CompWindow::constrainWindowState (wState,
							   w->actions ());
		if (w->id () == priv->activeWindow)
		    wState &= ~CompWindowStateDemandsAttentionMask;

		if (wState != w->state ())
		{
		    CompStackingUpdateMode stackingUpdateMode;
		    unsigned long          dState = wState ^ w->state ();

		    stackingUpdateMode = CompStackingUpdateModeNone;

		    /* raise the window whenever its fullscreen state,
		       above/below state or maximization state changed */
		    if (dState & (CompWindowStateFullscreenMask |
				  CompWindowStateAboveMask |
				  CompWindowStateBelowMask |
				  CompWindowStateMaximizedHorzMask |
				  CompWindowStateMaximizedVertMask))
			stackingUpdateMode = CompStackingUpdateModeNormal;

		    w->changeState (wState);

		    w->updateAttributes (stackingUpdateMode);
		}
	    }
	}
	else if (event->xclient.message_type == priv->atoms.wmProtocols)
	{
	    if (event->xclient.data.l[0] == priv->atoms.wmPing)
	    {
		w = findWindow (event->xclient.data.l[2]);
		if (w)
		    w->handlePing (priv->lastPing);
	    }
	}
	else if (event->xclient.message_type == priv->atoms.closeWindow)
	{
	    w = findWindow (event->xclient.window);
	    if (w)
		w->close (event->xclient.data.l[0]);
	}
	else if (event->xclient.message_type == priv->atoms.desktopGeometry)
	{
	    s = findScreen (event->xclient.window);
	    if (s)
	    {
		CompOption::Value value;

		value.set ((int) (event->xclient.data.l[0] /
			   s->size ().width ()));

		core->setOptionForPlugin (s, "core", "hsize", value);

		value.set ((int) (event->xclient.data.l[1] /
			   s->size ().height ()));

		core->setOptionForPlugin (s, "core", "vsize", value);
	    }
	}
	else if (event->xclient.message_type == priv->atoms.moveResizeWindow)
	{
	    w = findWindow (event->xclient.window);
	    if (w)
	    {
		unsigned int   xwcm = 0;
		XWindowChanges xwc;
		int            gravity;

		memset (&xwc, 0, sizeof (xwc));

		if (event->xclient.data.l[0] & (1 << 8))
		{
		    xwcm |= CWX;
		    xwc.x = event->xclient.data.l[1];
		}

		if (event->xclient.data.l[0] & (1 << 9))
		{
		    xwcm |= CWY;
		    xwc.y = event->xclient.data.l[2];
		}

		if (event->xclient.data.l[0] & (1 << 10))
		{
		    xwcm |= CWWidth;
		    xwc.width = event->xclient.data.l[3];
		}

		if (event->xclient.data.l[0] & (1 << 11))
		{
		    xwcm |= CWHeight;
		    xwc.height = event->xclient.data.l[4];
		}

		gravity = event->xclient.data.l[0] & 0xFF;

		w->moveResize (&xwc, xwcm, gravity);
	    }
	}
	else if (event->xclient.message_type == priv->atoms.restackWindow)
	{
	    w = findWindow (event->xclient.window);
	    if (w)
	    {
		/* TODO: other stack modes than Above and Below */
		if (event->xclient.data.l[1])
		{
		    CompWindow *sibling;

		    sibling = findWindow (event->xclient.data.l[1]);
		    if (sibling)
		    {
			if (event->xclient.data.l[2] == Above)
			    w->restackAbove (sibling);
			else if (event->xclient.data.l[2] == Below)
			    w->restackBelow (sibling);
		    }
		}
		else
		{
		    if (event->xclient.data.l[2] == Above)
			w->raise ();
		    else if (event->xclient.data.l[2] == Below)
			w->lower ();
		}
	    }
	}
	else if (event->xclient.message_type == priv->atoms.wmChangeState)
	{
	    w = findWindow (event->xclient.window);
	    if (w)
	    {
		if (event->xclient.data.l[0] == IconicState)
		{
		    if (w->actions () & CompWindowActionMinimizeMask)
			w->minimize ();
		}
		else if (event->xclient.data.l[0] == NormalState)
		    w->unminimize ();
	    }
	}
	else if (event->xclient.message_type == priv->atoms.showingDesktop)
	{
	    for (s = priv->screens; s; s = s->next)
	    {
		if (event->xclient.window == s->root () ||
		    event->xclient.window == None)
		{
		    if (event->xclient.data.l[0])
			s->enterShowDesktopMode ();
		    else
			s->leaveShowDesktopMode (NULL);
		}
	    }
	}
	else if (event->xclient.message_type == priv->atoms.numberOfDesktops)
	{
	    s = findScreen (event->xclient.window);
	    if (s)
	    {
		CompOption::Value value;

		value.set ((int) event->xclient.data.l[0]);

		core->setOptionForPlugin (s, "core", "number_of_desktops",
					  value);
	    }
	}
	else if (event->xclient.message_type == priv->atoms.currentDesktop)
	{
	    s = findScreen (event->xclient.window);
	    if (s)
		s->setCurrentDesktop (event->xclient.data.l[0]);
	}
	else if (event->xclient.message_type == priv->atoms.winDesktop)
	{
	    w = findWindow (event->xclient.window);
	    if (w)
		w->setDesktop (event->xclient.data.l[0]);
	}
	break;
    case MappingNotify:
	updateModifierMappings ();
	break;
    case MapRequest:
	w = findWindow (event->xmaprequest.window);
	if (w)
	{
	    XWindowAttributes attr;
	    bool              doMapProcessing = true;

	    /* We should check the override_redirect flag here, because the
	       client might have changed it while being unmapped. */
	    if (XGetWindowAttributes (priv->dpy, w->id (), &attr))
	    {
		if (w->attrib ().override_redirect != attr.override_redirect)
		{
		    w->attrib ().override_redirect = attr.override_redirect;
		    w->recalcType ();
		    w->recalcActions ();

		    matchPropertyChanged (w);
		}
	    }

	    w->managed () = true;

	    if (w->state () & CompWindowStateHiddenMask)
		if (!w->minimized () && !w->inShowDesktopMode ())
		    doMapProcessing = false;

	    if (doMapProcessing)
		w->processMap ();

	    setWindowProp (w->id (), priv->atoms.winDesktop, w->desktop ());
	}
	else
	{
	    XMapWindow (priv->dpy, event->xmaprequest.window);
	}
	break;
    case ConfigureRequest:
	w = findWindow (event->xconfigurerequest.window);
	if (w && w->managed ())
	{
	    XWindowChanges xwc;

	    memset (&xwc, 0, sizeof (xwc));

	    xwc.x	     = event->xconfigurerequest.x;
	    xwc.y	     = event->xconfigurerequest.y;
	    xwc.width	     = event->xconfigurerequest.width;
	    xwc.height       = event->xconfigurerequest.height;
	    xwc.border_width = event->xconfigurerequest.border_width;

	    w->moveResize (&xwc, event->xconfigurerequest.value_mask, 0);

	    if (event->xconfigurerequest.value_mask & CWStackMode)
	    {
		Window     above    = None;
		CompWindow *sibling = NULL;

		if (event->xconfigurerequest.value_mask & CWSibling)
		{
		    above   = event->xconfigurerequest.above;
		    sibling = findTopLevelWindow (above);
		}

		switch (event->xconfigurerequest.detail) {
		case Above:
		    if (w->allowWindowFocus (NO_FOCUS_MASK, 0))
		    {
			if (above)
			{
			    if (sibling)
				w->restackAbove (sibling);
			}
			else
			    w->raise ();
		    }
		    break;
		case Below:
		    if (above)
		    {
			if (sibling)
			    w->restackBelow (sibling);
		    }
		    else
			w->lower ();
		    break;
		default:
		    /* no handling of the TopIf, BottomIf, Opposite cases -
		       there will hardly be any client using that */
		    break;
		}
	    }
	}
	else
	{
	    XWindowChanges xwc;
	    unsigned int   xwcm;

	    xwcm = event->xconfigurerequest.value_mask &
		(CWX | CWY | CWWidth | CWHeight | CWBorderWidth);

	    xwc.x	     = event->xconfigurerequest.x;
	    xwc.y	     = event->xconfigurerequest.y;
	    xwc.width	     = event->xconfigurerequest.width;
	    xwc.height	     = event->xconfigurerequest.height;
	    xwc.border_width = event->xconfigurerequest.border_width;

	    if (w)
		w->configureXWindow (xwcm, &xwc);
	    else
		XConfigureWindow (priv->dpy, event->xconfigurerequest.window,
				  xwcm, &xwc);
	}
	break;
    case CirculateRequest:
	break;
    case FocusIn:
	if (event->xfocus.mode != NotifyGrab)
	{
	    w = findTopLevelWindow (event->xfocus.window);
	    if (w && w->managed ())
	    {
		unsigned int state = w->state ();

		if (w->id () != priv->activeWindow)
		{
		    priv->activeWindow = w->id ();
		    w->activeNum ()    = w->screen ()->activeNum ()++;

		    w->screen ()->addToCurrentActiveWindowHistory (w->id ());

		    XChangeProperty (priv->dpy , w->screen ()->root (),
				     priv->atoms.winActive,
				     XA_WINDOW, 32, PropModeReplace,
				     (unsigned char *) &priv->activeWindow, 1);
		}

		state &= ~CompWindowStateDemandsAttentionMask;
		w->changeState (state);
	    }
	}
	break;
    case EnterNotify:
	if (!priv->screens->hasGrab ()		    &&
	    event->xcrossing.mode   != NotifyGrab   &&
	    event->xcrossing.mode   != NotifyUngrab &&
	    event->xcrossing.detail != NotifyInferior)
	{
	    Bool raise;
	    int  delay;

	    raise = priv->opt[COMP_DISPLAY_OPTION_AUTORAISE].value ().b ();
	    delay =
		priv->opt[COMP_DISPLAY_OPTION_AUTORAISE_DELAY].value ().i ();

	    s = findScreen (event->xcrossing.root);
	    if (s)
	    {
		w = s->findTopLevelWindow (event->xcrossing.window);
	    }
	    else
		w = NULL;

	    if (w && w->id () != priv->below)
	    {
		priv->below = w->id ();

		if (!priv->opt[COMP_DISPLAY_OPTION_CLICK_TO_FOCUS].
		    value ().b ())
		{
		    if (priv->autoRaiseTimer.active () &&
			priv->autoRaiseWindow != w->id ())
		    {
			priv->autoRaiseTimer.stop ();
		    }

		    if (w->type () & ~(CompWindowTypeDockMask |
				       CompWindowTypeDesktopMask))
		    {
			w->moveInputFocusTo ();

			if (raise)
			{
			    if (delay > 0)
			    {
				priv->autoRaiseWindow = w->id ();
				priv->autoRaiseTimer.start (
				    boost::bind (autoRaiseTimeout, this),
				    delay, (float) delay * 1.2);
			    }
			    else
			    {
				CompStackingUpdateMode mode =
				    CompStackingUpdateModeNormal;

				w->updateAttributes (mode);
			    }
			}
		    }
		}
	    }
	}
	break;
    case LeaveNotify:
	if (event->xcrossing.detail != NotifyInferior)
	{
	    if (event->xcrossing.window == priv->below)
		priv->below = None;
	}
	break;
    default:
	if (event->type == priv->damageEvent + XDamageNotify)
	{
	    XDamageNotifyEvent *de = (XDamageNotifyEvent *) event;

	    if (lastDamagedWindow && de->drawable == lastDamagedWindow->id ())
	    {
		w = lastDamagedWindow;
	    }
	    else
	    {
		w = findWindow (de->drawable);
		if (w)
		    lastDamagedWindow = w;
	    }

	    if (w)
		w->processDamage (de);
	}
	else if (priv->shapeExtension &&
		 event->type == priv->shapeEvent + ShapeNotify)
	{
	    w = findWindow (((XShapeEvent *) event)->window);
	    if (w)
	    {
		if (w->mapNum ())
		{
		    w->addDamage ();
		    w->updateRegion ();
		    w->addDamage ();
		}
	    }
	}
	else if (priv->randrExtension &&
		 event->type == priv->randrEvent + RRScreenChangeNotify)
	{
	    XRRScreenChangeNotifyEvent *rre;

	    rre = (XRRScreenChangeNotifyEvent *) event;

	    s = findScreen (rre->root);
	    if (s)
		s->detectRefreshRate ();
	}
	else if (event->type == priv->syncEvent + XSyncAlarmNotify)
	{
	    XSyncAlarmNotifyEvent *sa;

	    sa = (XSyncAlarmNotifyEvent *) event;

	    for (s = priv->screens; s; s = s->next)
	    {
		for (w = s->windows (); w; w = w->next)
		{
		    if (w->syncAlarm () == sa->alarm)
		    {
			w->handleSyncAlarm ();
			return;
		    }
		}
	    }
	}
	break;
    }
}
