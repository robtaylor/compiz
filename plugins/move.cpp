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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/cursorfont.h>

#include <compiz-core.h>
#include <compprivatehandler.h>

static CompMetadata *moveMetadata;

class MovePluginVTable : public CompPlugin::VTable
{
    public:

	const char *
	name () { return "move"; };

	CompMetadata *
	getMetadata ();

	virtual bool
	init ();

	virtual void
	fini ();

	virtual bool
	initObject (CompObject *object);

	virtual void
	finiObject (CompObject *object);

	CompOption::Vector &
	getObjectOptions (CompObject *object);

	bool
	setObjectOption (CompObject        *object,
			 const char        *name,
			 CompOption::Value &value);
};



struct _MoveKeys {
    const char *name;
    int        dx;
    int        dy;
} mKeys[] = {
    { "Left",  -1,  0 },
    { "Right",  1,  0 },
    { "Up",     0, -1 },
    { "Down",   0,  1 }
};

#define NUM_KEYS (sizeof (mKeys) / sizeof (mKeys[0]))

#define KEY_MOVE_INC 24

#define SNAP_BACK 20
#define SNAP_OFF  100

static int displayPrivateIndex;
static int screenPrivateIndex;
static int windowPrivateIndex;


#define MOVE_DISPLAY_OPTION_INITIATE_BUTTON   0
#define MOVE_DISPLAY_OPTION_INITIATE_KEY      1
#define MOVE_DISPLAY_OPTION_OPACITY	      2
#define MOVE_DISPLAY_OPTION_CONSTRAIN_Y	      3
#define MOVE_DISPLAY_OPTION_SNAPOFF_MAXIMIZED 4
#define MOVE_DISPLAY_OPTION_LAZY_POSITIONING  5
#define MOVE_DISPLAY_OPTION_NUM		      6

class MoveDisplay :
    public DisplayInterface,
    public PrivateHandler<MoveDisplay,CompDisplay>
{

    public:
	MoveDisplay (CompDisplay *display) :
	    PrivateHandler<MoveDisplay,CompDisplay> (display),
	    display (display),
	    opt(MOVE_DISPLAY_OPTION_NUM)
	{
	    display->add (this);
	    DisplayInterface::setHandler (display);
	};

	void
	handleEvent (XEvent *);
	
	CompDisplay *display;

	CompOption::Vector opt;

	CompWindow *w;
	int	       savedX;
	int	       savedY;
	int	       x;
	int	       y;
	Region     region;
	int        status;
	KeyCode    key[NUM_KEYS];

	int releaseButton;

	GLushort moveOpacity;
};



class MoveScreen : public PrivateHandler<MoveScreen,CompScreen> {
    public:
	
	MoveScreen (CompScreen *screen) :
	    PrivateHandler<MoveScreen,CompScreen> (screen),
	    screen (screen) {};

        CompScreen *screen;
	CompScreen::grabHandle grab;

	Cursor moveCursor;

	unsigned int origState;

	int	snapOffY;
	int	snapBackY;
};

class MoveWindow :
    public WindowInterface,
    public PrivateHandler<MoveWindow,CompWindow>
{
    public:
	MoveWindow (CompWindow *window) :
	    PrivateHandler<MoveWindow,CompWindow> (window),
	    window (window)
	{
	    window->add (this);
	    WindowInterface::setHandler (window);
	};

	bool
	paint (const CompWindowPaintAttrib *, const CompTransform *, Region,
	       unsigned int);

	CompWindow *window;
};

#define GET_MOVE_DISPLAY(d)					  \
    static_cast<MoveDisplay *> ((d)->privates[displayPrivateIndex].ptr)

#define MOVE_DISPLAY(d)		           \
    MoveDisplay *md = GET_MOVE_DISPLAY (d)

#define GET_MOVE_SCREEN(s)					      \
    static_cast<MoveScreen *> ((s)->privates[screenPrivateIndex].ptr)

#define MOVE_SCREEN(s)						        \
    MoveScreen *ms = GET_MOVE_SCREEN (s)

#define GET_MOVE_WINDOW(w)					      \
    static_cast<MoveWindow *> ((w)->privates[windowPrivateIndex].ptr)

#define MOVE_WINDOW(w)						        \
    MoveWindow *mw = GET_MOVE_WINDOW (w)

#define NUM_OPTIONS(s) (sizeof ((s)->opt) / sizeof (CompOption))

static bool
moveInitiate (CompDisplay     *d,
	      CompAction      *action,
	      CompAction::State state,
	      CompOption::Vector &options)
{
    CompWindow *w;
    Window     xid;

    MOVE_DISPLAY (d);

    xid = CompOption::getIntOptionNamed (options, "window");

    w = d->findWindow (xid);
    if (w && (w->actions () & CompWindowActionMoveMask))
    {
	XRectangle   workArea;
	unsigned int mods;
	int          x, y, button;

	CompScreen *s = w->screen ();

	MOVE_SCREEN (w->screen ());

	mods = CompOption::getIntOptionNamed (options, "modifiers", 0);

	x = CompOption::getIntOptionNamed (options, "x",
			       w->attrib ().x + (w->width () / 2));
	y = CompOption::getIntOptionNamed (options, "y",
			       w->attrib ().y + (w->height () / 2));

	button = CompOption::getIntOptionNamed (options, "button", -1);

	if (s->otherGrabExist ("move", 0))
	    return FALSE;

	if (md->w)
	    return FALSE;

	if (w->type () & (CompWindowTypeDesktopMask |
		       CompWindowTypeDockMask    |
		       CompWindowTypeFullscreenMask))
	    return FALSE;

	if (w->attrib ().override_redirect)
	    return FALSE;

	if (state & CompAction::StateInitButton)
	    action->setState (action->state () | CompAction::StateTermButton);

	if (md->region)
	{
	    XDestroyRegion (md->region);
	    md->region = NULL;
	}

	md->status = RectangleOut;

	md->savedX = w->serverGeometry ().x ();
	md->savedY = w->serverGeometry ().y ();

	md->x = 0;
	md->y = 0;

	lastPointerX = x;
	lastPointerY = y;

	ms->origState = w->state ();

	s->getWorkareaForOutput (w->outputDevice (),
					&workArea);

	ms->snapBackY = w->serverGeometry ().y () - workArea.y;
	ms->snapOffY  = y - workArea.y;

	if (!ms->grab)
	    ms->grab = s->pushGrab (ms->moveCursor, "move");

	if (ms->grab)
	{
	    md->w = w;

	    md->releaseButton = button;

	    w->grabNotify (x, y, mods,CompWindowGrabMoveMask |
			   CompWindowGrabButtonMask);

	    if (state & CompAction::StateInitKey)
	    {
		int xRoot, yRoot;

		xRoot = w->attrib ().x + (w->width () / 2);
		yRoot = w->attrib ().y + (w->height () / 2);

		s->warpPointer (xRoot - pointerX, yRoot - pointerY);
	    }

	    if (md->moveOpacity != OPAQUE)
		w->addDamage ();
	}
    }

    return FALSE;
}

static bool
moveTerminate (CompDisplay     *d,
	       CompAction      *action,
	       CompAction::State state,
	       CompOption::Vector &options)
{
    MOVE_DISPLAY (d);

    if (md->w)
    {
	MOVE_SCREEN (md->w->screen ());

	if (state & CompAction::StateCancel)
	    md->w->move (md->savedX - md->w->attrib ().x,
			md->savedY - md->w->attrib ().y,
			TRUE, FALSE);

	md->w->syncPosition ();

	/* update window attributes as window constraints may have
	   changed - needed e.g. if a maximized window was moved
	   to another output device */
	md->w->updateAttributes (CompStackingUpdateModeNone);

	md->w->ungrabNotify ();

	if (ms->grab)
	{
	    md->w->screen ()->removeGrab (ms->grab, NULL);
	    ms->grab = NULL;
	}

	if (md->moveOpacity != OPAQUE)
	    md->w->addDamage ();

	md->w             = 0;
	md->releaseButton = 0;
    }

    action->setState (action->state () & ~(CompAction::StateTermKey | CompAction::StateTermButton));

    return FALSE;
}

/* creates a region containing top and bottom struts. only struts that are
   outside the screen workarea are considered. */
static Region
moveGetYConstrainRegion (CompScreen *s)
{
    CompWindow   *w;
    Region       region;
    REGION       r;
    XRectangle   workArea;
    BoxRec       extents;
    unsigned int i;

    region = XCreateRegion ();
    if (!region)
	return NULL;

    r.rects    = &r.extents;
    r.numRects = r.size = 1;

    r.extents.x1 = MINSHORT;
    r.extents.y1 = 0;
    r.extents.x2 = 0;
    r.extents.y2 = s->size ().height ();

    XUnionRegion (&r, region, region);

    r.extents.x1 = s->size ().width ();
    r.extents.x2 = MAXSHORT;

    XUnionRegion (&r, region, region);

    for (i = 0; i < s->outputDevs ().size(); i++)
    {
	XUnionRegion (s->outputDevs ()[i].region (), region, region);

	s->getWorkareaForOutput (i, &workArea);
	extents = s->outputDevs ()[i].region ()->extents;

	foreach (w, s->windows ())
	{
	    if (!w->mapNum ())
		continue;

	    if (w->struts ())
	    {
		r.extents.x1 = w->struts ()->top.x;
		r.extents.y1 = w->struts ()->top.y;
		r.extents.x2 = r.extents.x1 + w->struts ()->top.width;
		r.extents.y2 = r.extents.y1 + w->struts ()->top.height;

		if (r.extents.x1 < extents.x1)
		    r.extents.x1 = extents.x1;
		if (r.extents.x2 > extents.x2)
		    r.extents.x2 = extents.x2;
		if (r.extents.y1 < extents.y1)
		    r.extents.y1 = extents.y1;
		if (r.extents.y2 > extents.y2)
		    r.extents.y2 = extents.y2;

		if (r.extents.x1 < r.extents.x2 && r.extents.y1 < r.extents.y2)
		{
		    if (r.extents.y2 <= workArea.y)
			XSubtractRegion (region, &r, region);
		}

		r.extents.x1 = w->struts ()->bottom.x;
		r.extents.y1 = w->struts ()->bottom.y;
		r.extents.x2 = r.extents.x1 + w->struts ()->bottom.width;
		r.extents.y2 = r.extents.y1 + w->struts ()->bottom.height;

		if (r.extents.x1 < extents.x1)
		    r.extents.x1 = extents.x1;
		if (r.extents.x2 > extents.x2)
		    r.extents.x2 = extents.x2;
		if (r.extents.y1 < extents.y1)
		    r.extents.y1 = extents.y1;
		if (r.extents.y2 > extents.y2)
		    r.extents.y2 = extents.y2;

		if (r.extents.x1 < r.extents.x2 && r.extents.y1 < r.extents.y2)
		{
		    if (r.extents.y1 >= (workArea.y + workArea.height))
			XSubtractRegion (region, &r, region);
		}
	    }
	}
    }

    return region;
}

static void
moveHandleMotionEvent (CompScreen *s,
		       int	  xRoot,
		       int	  yRoot)
{
    MOVE_SCREEN (s);

    if (ms->grab)
    {
	int	     dx, dy;
	int	     wX, wY;
	unsigned int wWidth, wHeight;
	CompWindow   *w;

	MOVE_DISPLAY (s->display ());

	w = md->w;
	
	wX      = w->serverGeometry ().x ();
	wY      = w->serverGeometry ().y ();
	wWidth  = w->serverGeometry ().width () +
		  w->serverGeometry ().border () * 2;
	wHeight = w->serverGeometry ().height () +
		  w->serverGeometry ().border () * 2;

	md->x += xRoot - lastPointerX;
	md->y += yRoot - lastPointerY;

	if (w->type () & CompWindowTypeFullscreenMask)
	{
	    dx = dy = 0;
	}
	else
	{
	    XRectangle workArea;
	    int	       min, max;

	    dx = md->x;
	    dy = md->y;

	    s->getWorkareaForOutput (w->outputDevice (), &workArea);

	    if (md->opt[MOVE_DISPLAY_OPTION_CONSTRAIN_Y].value ().b ())
	    {
		if (!md->region)
		    md->region = moveGetYConstrainRegion (s);

		/* make sure that the top frame extents or the top row of
		   pixels are within what is currently our valid screen
		   region */
		if (md->region)
		{
		    int x, y, width, height;
		    int status;

		    x	   = wX + dx - w->input ().left;
		    y	   = wY + dy - w->input ().top;
		    width  = wWidth + w->input ().left + w->input ().right;
		    height = w->input ().top ? w->input ().top : 1;

		    status = XRectInRegion (md->region, x, y, width, height);

		    /* only constrain movement if previous position was valid */
		    if (md->status == RectangleIn)
		    {
			int xStatus = status;

			while (dx && xStatus != RectangleIn)
			{
			    xStatus = XRectInRegion (md->region,
						     x, y - dy,
						     width, height);

			    if (xStatus != RectangleIn)
				dx += (dx < 0) ? 1 : -1;

			    x = wX + dx - w->input ().left;
			}

			while (dy && status != RectangleIn)
			{
			    status = XRectInRegion (md->region,
						    x, y,
						    width, height);

			    if (status != RectangleIn)
				dy += (dy < 0) ? 1 : -1;

			    y = wY + dy - w->input ().top;
			}
		    }
		    else
		    {
			md->status = status;
		    }
		}
	    }

	    if (md->opt[MOVE_DISPLAY_OPTION_SNAPOFF_MAXIMIZED].value ().b ())
	    {
		if (w->state () & CompWindowStateMaximizedVertMask)
		{
		    if (abs ((yRoot - workArea.y) - ms->snapOffY) >= SNAP_OFF)
		    {
			if (!s->otherGrabExist ("move", 0))
			{
			    int width = w->serverGeometry ().width ();

			    w->saveMask () |= CWX | CWY;

			    if (w->saveMask ()& CWWidth)
				width = w->saveWc ().width;

			    w->saveWc ().x = xRoot - (width >> 1);
			    w->saveWc ().y = yRoot + (w->input ().top >> 1);

			    md->x = md->y = 0;

			    w->maximize (0);

			    ms->snapOffY = ms->snapBackY;

			    return;
			}
		    }
		}
		else if (ms->origState & CompWindowStateMaximizedVertMask)
		{
		    if (abs ((yRoot - workArea.y) - ms->snapBackY) < SNAP_BACK)
		    {
			if (!s->otherGrabExist ("move", 0))
			{
			    int wy;

			    /* update server position before maximizing
			       window again so that it is maximized on
			       correct output */
			    w->syncPosition ();

			    w->maximize (ms->origState);

			    wy  = workArea.y + (w->input ().top >> 1);
			    wy += w->sizeHints ().height_inc >> 1;

			    s->warpPointer (0, wy - pointerY);

			    return;
			}
		    }
		}
	    }

	    if (w->state () & CompWindowStateMaximizedVertMask)
	    {
		min = workArea.y + w->input ().top;
		max = workArea.y + workArea.height - w->input ().bottom - wHeight;

		if (wY + dy < min)
		    dy = min - wY;
		else if (wY + dy > max)
		    dy = max - wY;
	    }

	    if (w->state () & CompWindowStateMaximizedHorzMask)
	    {
		if (wX > (int) s->size ().width () || wX + w->width () < 0)
		    return;

		if (wX + wWidth < 0)
		    return;

		min = workArea.x + w->input ().left;
		max = workArea.x + workArea.width - w->input ().right - wWidth;

		if (wX + dx < min)
		    dx = min - wX;
		else if (wX + dx > max)
		    dx = max - wX;
	    }
	}

	if (dx || dy)
	{
	    w->move (wX + dx - w->attrib ().x,
			wY + dy - w->attrib ().y,
			TRUE, FALSE);

	    if (md->opt[MOVE_DISPLAY_OPTION_LAZY_POSITIONING].value ().b ())
	    {
		/* FIXME: This form of lazy positioning is broken and should
		   be replaced asap. Current code exists just to avoid a
		   major performance regression in the 0.5.2 release. */
		w->serverGeometry ().setX (w->attrib ().x);
		w->serverGeometry ().setY (w->attrib ().y);
	    }
	    else
	    {
		w->syncPosition ();
	    }

	    md->x -= dx;
	    md->y -= dy;
	}
    }
}

void
MoveDisplay::handleEvent (XEvent *event)
{
    CompScreen *s;

    switch (event->type) {
    case ButtonPress:
    case ButtonRelease:
	s = display->findScreen (event->xbutton.root);
	
	if (s)
	{
	    MOVE_SCREEN (s);

	    if (ms->grab)
	    {
		if (releaseButton == -1 ||
		    releaseButton == (int) event->xbutton.button)
		{
		    CompAction *action;

		    action = &opt[MOVE_DISPLAY_OPTION_INITIATE_BUTTON].value ().action ();
		    moveTerminate (display, action, CompAction::StateTermButton,
				   noOptions);
		}
	    }
	}
	break;
    case KeyPress:
	s = display->findScreen (event->xkey.root);
	if (s)
	{
	    MOVE_SCREEN (s);

	    if (ms->grab)
	    {
		unsigned int i;

		for (i = 0; i < NUM_KEYS; i++)
		{
		    if (event->xkey.keycode == key[i])
		    {
			XWarpPointer (display->dpy (), None, None, 0, 0, 0, 0,
				      mKeys[i].dx * KEY_MOVE_INC,
				      mKeys[i].dy * KEY_MOVE_INC);
			break;
		    }
		}
	    }
	}
	break;
    case MotionNotify:
	s = display->findScreen (event->xmotion.root);
	if (s)
	    moveHandleMotionEvent (s, pointerX, pointerY);
	break;
    case EnterNotify:
    case LeaveNotify:
	s = display->findScreen (event->xcrossing.root);
	if (s)
	    moveHandleMotionEvent (s, pointerX, pointerY);
	break;
    case ClientMessage:
	if (event->xclient.message_type == display->atoms ().wmMoveResize)
	{
	    CompWindow *w;

	    if (event->xclient.data.l[2] == WmMoveResizeMove ||
		event->xclient.data.l[2] == WmMoveResizeMoveKeyboard)
	    {
		w = display->findWindow (event->xclient.window);
		if (w)
		{
		    CompOption::Vector o;
		    int	       xRoot, yRoot;
		    int	       option;

		    o.push_back (CompOption ("window", CompOption::TypeInt));
		    o[0].value ().set ((int) event->xclient.window);

		    if (event->xclient.data.l[2] == WmMoveResizeMoveKeyboard)
		    {
			option = MOVE_DISPLAY_OPTION_INITIATE_KEY;

			moveInitiate (display, &opt[option].value ().action (),
				      CompAction::StateInitKey, o);
		    }
		    else
		    {
			unsigned int mods;
			Window	     root, child;
			int	     i;

			option = MOVE_DISPLAY_OPTION_INITIATE_BUTTON;

			XQueryPointer (display->dpy (), w->screen ()->root (),
				       &root, &child, &xRoot, &yRoot,
				       &i, &i, &mods);

			/* TODO: not only button 1 */
			if (mods & Button1Mask)
			{
			    o.push_back (CompOption ("modifiers", CompOption::TypeInt));
			    o[1].value ().set ((int) mods);

			    o.push_back (CompOption ("x", CompOption::TypeInt));
			    o[2].value ().set ((int) event->xclient.data.l[0]);

			    o.push_back (CompOption ("y", CompOption::TypeInt));
			    o[3].value ().set ((int) event->xclient.data.l[1]);

			    o.push_back (CompOption ("button", CompOption::TypeInt));
			    o[4].value ().set ((int) (event->xclient.data.l[3] ?
				           event->xclient.data.l[3] : -1));

			    moveInitiate (display,
					  &opt[option].value ().action (),
					  CompAction::StateInitButton, o);

			    moveHandleMotionEvent (w->screen (), xRoot, yRoot);
			}
		    }
		}
	    }
	}
	break;
    case DestroyNotify:
	if (w && w->id () == event->xdestroywindow.window)
	{
	    int option;

	    option = MOVE_DISPLAY_OPTION_INITIATE_BUTTON;
	    moveTerminate (display,
			   &opt[option].value ().action (),
			   0, noOptions);
	    option = MOVE_DISPLAY_OPTION_INITIATE_KEY;
	    moveTerminate (display,
			   &opt[option].value ().action (),
			   0, noOptions);
	}
	break;
    case UnmapNotify:
	if (w && w->id () == event->xunmap.window)
	{
	    int option;

	    option = MOVE_DISPLAY_OPTION_INITIATE_BUTTON;
	    moveTerminate (display,
			   &opt[option].value ().action (),
			   0, noOptions);
	    option = MOVE_DISPLAY_OPTION_INITIATE_KEY;
	    moveTerminate (display,
			   &opt[option].value ().action (),
			   0, noOptions);
	}
    default:
	break;
    }

    display->handleEvent (event);
}

bool
MoveWindow::paint (const CompWindowPaintAttrib *attrib,
		   const CompTransform	 *transform,
		   Region			 region,
		   unsigned int		 mask)
{
    CompWindowPaintAttrib sAttrib;
    bool	      status;

    MOVE_SCREEN (window->screen ());

    if (ms->grab)
    {
	MOVE_DISPLAY (window->screen ()->display ());

	if (md->w == window && md->moveOpacity != OPAQUE)
	{
	    /* modify opacity of windows that are not active */
	    sAttrib = *attrib;
	    attrib  = &sAttrib;

	    sAttrib.opacity = (sAttrib.opacity * md->moveOpacity) >> 16;
	}
    }

    status = window->paint (attrib, transform, region, mask);

    return status;
}

static CompOption::Vector &
moveGetDisplayOptions (CompObject  *object)
{
    CORE_DISPLAY (object);
    MOVE_DISPLAY (d);
    return md->opt;
}

static bool
moveSetDisplayOption (CompObject      *object,
		      const char      *name,
		      CompOption::Value &value)
{
    CompOption *o;
    unsigned int index;

    CORE_DISPLAY (object);
    MOVE_DISPLAY (d);

    o = CompOption::findOption (md->opt, name, &index);
    if (!o)
	return FALSE;

    switch (index) {
    case MOVE_DISPLAY_OPTION_OPACITY:
	if (o->set (value))
	{
	    md->moveOpacity = (o->value ().i () * OPAQUE) / 100;
	    return TRUE;
	}
	break;
    default:
	return CompOption::setDisplayOption (d, *o, value);
    }

    return FALSE;
}

static const CompMetadata::OptionInfo moveDisplayOptionInfo[] = {
    { "initiate_button", "button", 0, moveInitiate, moveTerminate },
    { "initiate_key", "key", 0, moveInitiate, moveTerminate },
    { "opacity", "int", "<min>0</min><max>100</max>", 0, 0 },
    { "constrain_y", "bool", 0, 0, 0 },
    { "snapoff_maximized", "bool", 0, 0, 0 },
    { "lazy_positioning", "bool", 0, 0, 0 }
};

static bool
moveInitDisplay (CompObject *o)
{
    MoveDisplay  *md;
    unsigned int i;

    CORE_DISPLAY (o);

    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	return false;

    md = new MoveDisplay (d);
    if (!md)
	return false;

    if (!moveMetadata->initDisplayOptions (d, moveDisplayOptionInfo,
					   MOVE_DISPLAY_OPTION_NUM, md->opt))
    {
	delete md;
	return false;
    }

    md->moveOpacity =
	(md->opt[MOVE_DISPLAY_OPTION_OPACITY].value ().i () * OPAQUE) / 100;

    md->w             = 0;
    md->region        = NULL;
    md->status        = RectangleOut;
    md->releaseButton = 0;

    for (i = 0; i < NUM_KEYS; i++)
	md->key[i] = XKeysymToKeycode (d->dpy (),
				       XStringToKeysym (mKeys[i].name));


    d->privates[displayPrivateIndex].ptr = md;

    return true;
}

static void
moveFiniDisplay (CompObject *o)
{
    CORE_DISPLAY (o);
    MOVE_DISPLAY (d);

    CompOption::finiDisplayOptions (d, md->opt);

    delete md;
}

static bool
moveInitScreen (CompObject *o)
{
    MoveScreen *ms;

    CORE_SCREEN (o);

    ms = new MoveScreen (s);
    if (!ms)
	return false;

    ms->grab = NULL;

    ms->moveCursor = XCreateFontCursor (s->display ()->dpy (), XC_fleur);

    s->privates[screenPrivateIndex].ptr = ms;

    return true;
}

static void
moveFiniScreen (CompObject *o)
{
    CORE_SCREEN (o);
    MOVE_SCREEN (s);

    if (ms->moveCursor)
	XFreeCursor (s->display ()->dpy (), ms->moveCursor);

    delete ms;
}

static bool
moveInitWindow (CompObject *o)
{
    MoveWindow *mw;

    printf("Init window %d %s (%s)\n",o->type (),o->typeName (),o->name().c_str());
    CORE_WINDOW (o);

    printf("Init window %d %s (%s)\n",w->type (),w->typeName (),w->name().c_str());
    mw = new MoveWindow (w);
    if (!mw)
	return false;

    w->privates[windowPrivateIndex].ptr = mw;

    return true;
}

static void
moveFiniWindow (CompObject *o)
{
    CORE_WINDOW (o);
    MOVE_WINDOW (w);

    delete mw;
}

bool
MovePluginVTable::initObject (CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) moveInitDisplay,
	(InitPluginObjectProc) moveInitScreen,
	(InitPluginObjectProc) moveInitWindow
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (o));
}

void
MovePluginVTable::finiObject (CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) moveFiniDisplay,
	(FiniPluginObjectProc) moveFiniScreen,
	(FiniPluginObjectProc) moveFiniWindow
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (o));
}

CompOption::Vector &
MovePluginVTable::getObjectOptions (CompObject *object)
{
    static GetPluginObjectOptionsProc dispTab[] = {
	(GetPluginObjectOptionsProc) 0, /* GetCoreOptions */
	(GetPluginObjectOptionsProc) moveGetDisplayOptions
    };

    RETURN_DISPATCH (object, dispTab, ARRAY_SIZE (dispTab),
		     noOptions, (object));
}

bool
MovePluginVTable::setObjectOption (CompObject      *object,
		     const char      *name,
		     CompOption::Value &value)
{
    static SetPluginObjectOptionProc dispTab[] = {
	(SetPluginObjectOptionProc) 0, /* SetCoreOption */
	(SetPluginObjectOptionProc) moveSetDisplayOption
    };

    RETURN_DISPATCH (object, dispTab, ARRAY_SIZE (dispTab), false,
		     (object, name, value));
}

bool
MovePluginVTable::init ()
{
    moveMetadata = new CompMetadata (name (),
				     moveDisplayOptionInfo,
				     MOVE_DISPLAY_OPTION_NUM,
				     0, 0);
    if (!moveMetadata)
	return false;

    displayPrivateIndex = CompDisplay::allocPrivateIndex ();
    if (displayPrivateIndex < 0)
    {
	delete moveMetadata;
	return false;
    }
    screenPrivateIndex = CompScreen::allocPrivateIndex ();
    if (screenPrivateIndex < 0)
    {
	CompDisplay::freePrivateIndex (displayPrivateIndex);
	delete moveMetadata;
	return false;
    }
    windowPrivateIndex = CompWindow::allocPrivateIndex ();
    if (windowPrivateIndex < 0)
    {
	CompDisplay::freePrivateIndex (displayPrivateIndex);
	CompScreen::freePrivateIndex (screenPrivateIndex);
	delete moveMetadata;
	return false;
    }

    moveMetadata->addFromFile (name ());

    return true;
}

void
MovePluginVTable::fini ()
{
    CompDisplay::freePrivateIndex (displayPrivateIndex);
    CompScreen::freePrivateIndex (screenPrivateIndex);
    CompWindow::freePrivateIndex (windowPrivateIndex);

    delete moveMetadata;
}

CompMetadata *
MovePluginVTable::getMetadata ()
{
    return moveMetadata;
}

MovePluginVTable moveVTable;

CompPlugin::VTable *
getCompPluginInfo20080805 (void)
{
    return &moveVTable;
}
