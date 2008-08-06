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

static CompMetadata moveMetadata;

class MovePluginVTable : public CompPluginVTable
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

	CompOption *
	getObjectOptions (CompObject *object, int *count);

	bool
	setObjectOption (CompObject *object,
			 const char *name,
			 CompOptionValue *value);
};



struct _MoveKeys {
    char *name;
    int  dx;
    int  dy;
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

#define MOVE_DISPLAY_OPTION_INITIATE_BUTTON   0
#define MOVE_DISPLAY_OPTION_INITIATE_KEY      1
#define MOVE_DISPLAY_OPTION_OPACITY	      2
#define MOVE_DISPLAY_OPTION_CONSTRAIN_Y	      3
#define MOVE_DISPLAY_OPTION_SNAPOFF_MAXIMIZED 4
#define MOVE_DISPLAY_OPTION_LAZY_POSITIONING  5
#define MOVE_DISPLAY_OPTION_NUM		      6

class MoveDisplay : public DisplayInterface {

    public:
	MoveDisplay (CompDisplay *display) : display (display)
	{
	    display->add (this);
	    DisplayInterface::setHandler (display);
	};

	void
	handleEvent (XEvent *);
	

	CompDisplay *display;
	int		    screenPrivateIndex;
	

	CompOption opt[MOVE_DISPLAY_OPTION_NUM];

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



class MoveScreen {
    public:
	
	MoveScreen (CompScreen *screen) : screen (screen) {};

	int windowPrivateIndex;

        CompScreen *screen;
	int grabIndex;

	Cursor moveCursor;

	unsigned int origState;

	int	snapOffY;
	int	snapBackY;
};

class MoveWindow : public WindowInterface {
    public:
	MoveWindow (CompWindow *window) : window (window)
	{
	    window->add (this);
	    WindowInterface::setHandler (window);
	};

	bool
	paint (const WindowPaintAttrib *, const CompTransform *, Region,
	       unsigned int);

	CompWindow *window;
};

#define GET_MOVE_DISPLAY(d)					  \
    ((MoveDisplay *) (d)->privates[displayPrivateIndex].ptr)

#define MOVE_DISPLAY(d)		           \
    MoveDisplay *md = GET_MOVE_DISPLAY (d)

#define GET_MOVE_SCREEN(s, md)					      \
    ((MoveScreen *) (s)->privates[(md)->screenPrivateIndex].ptr)

#define MOVE_SCREEN(s)						        \
    MoveScreen *ms = GET_MOVE_SCREEN (s, GET_MOVE_DISPLAY (s->display ()))

#define GET_MOVE_WINDOW(w, ms)					      \
    ((MoveWindow *) (w)->privates[(ms)->windowPrivateIndex].ptr)

#define MOVE_WINDOW(w)						        \
    MoveWindow *mw = GET_MOVE_WINDOW (w, GET_MOVE_SCREEN (w->screen (), GET_MOVE_DISPLAY (w->screen ()->display())))

#define NUM_OPTIONS(s) (sizeof ((s)->opt) / sizeof (CompOption))

static bool
moveInitiate (CompDisplay     *d,
	      CompAction      *action,
	      CompActionState state,
	      CompOption      *option,
	      int	      nOption)
{
    CompWindow *w;
    Window     xid;

    MOVE_DISPLAY (d);

    xid = getIntOptionNamed (option, nOption, "window", 0);

    w = d->findWindow (xid);
    if (w && (w->actions () & CompWindowActionMoveMask))
    {
	XRectangle   workArea;
	unsigned int mods;
	int          x, y, button;

	CompScreen *s = w->screen ();

	MOVE_SCREEN (w->screen ());

	mods = getIntOptionNamed (option, nOption, "modifiers", 0);

	x = getIntOptionNamed (option, nOption, "x",
			       w->attrib ().x + (w->width () / 2));
	y = getIntOptionNamed (option, nOption, "y",
			       w->attrib ().y + (w->height () / 2));

	button = getIntOptionNamed (option, nOption, "button", -1);

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

	if (state & CompActionStateInitButton)
	    action->state |= CompActionStateTermButton;

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

	if (!ms->grabIndex)
	    ms->grabIndex = s->pushGrab (ms->moveCursor, "move");

	if (ms->grabIndex)
	{
	    md->w = w;

	    md->releaseButton = button;

	    w->grabNotify (x, y, mods,CompWindowGrabMoveMask |
			   CompWindowGrabButtonMask);

	    if (state & CompActionStateInitKey)
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
	       CompActionState state,
	       CompOption      *option,
	       int	       nOption)
{
    MOVE_DISPLAY (d);

    if (md->w)
    {
	MOVE_SCREEN (md->w->screen ());

	if (state & CompActionStateCancel)
	    md->w->move (md->savedX - md->w->attrib ().x,
			md->savedY - md->w->attrib ().y,
			TRUE, FALSE);

	md->w->syncPosition ();

	/* update window attributes as window constraints may have
	   changed - needed e.g. if a maximized window was moved
	   to another output device */
	md->w->updateAttributes (CompStackingUpdateModeNone);

	md->w->ungrabNotify ();

	if (ms->grabIndex)
	{
	    md->w->screen ()->removeGrab (ms->grabIndex, NULL);
	    ms->grabIndex = 0;
	}

	if (md->moveOpacity != OPAQUE)
	    md->w->addDamage ();

	md->w             = 0;
	md->releaseButton = 0;
    }

    action->state &= ~(CompActionStateTermKey | CompActionStateTermButton);

    return FALSE;
}

/* creates a region containing top and bottom struts. only struts that are
   outside the screen workarea are considered. */
static Region
moveGetYConstrainRegion (CompScreen *s)
{
    CompWindow *w;
    Region     region;
    REGION     r;
    XRectangle workArea;
    BoxRec     extents;
    int	       i;

    region = XCreateRegion ();
    if (!region)
	return NULL;

    r.rects    = &r.extents;
    r.numRects = r.size = 1;

    r.extents.x1 = MINSHORT;
    r.extents.y1 = 0;
    r.extents.x2 = 0;
    r.extents.y2 = s->height ();

    XUnionRegion (&r, region, region);

    r.extents.x1 = s->width ();
    r.extents.x2 = MAXSHORT;

    XUnionRegion (&r, region, region);

    for (i = 0; i < s->outputDevs ().size(); i++)
    {
	XUnionRegion (s->outputDevs ()[i].region (), region, region);

	s->getWorkareaForOutput (i, &workArea);
	extents = s->outputDevs ()[i].region ()->extents;

	for (w = s->windows (); w; w = w->next)
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

    if (ms->grabIndex)
    {
	int	   dx, dy;
	int	   wX, wY;
	int	   wWidth, wHeight;
	CompWindow *w;

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

	    if (md->opt[MOVE_DISPLAY_OPTION_CONSTRAIN_Y].value.b)
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

	    if (md->opt[MOVE_DISPLAY_OPTION_SNAPOFF_MAXIMIZED].value.b)
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
		if (wX > s->width () || wX + w->width () < 0)
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

	    if (md->opt[MOVE_DISPLAY_OPTION_LAZY_POSITIONING].value.b)
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

	    if (ms->grabIndex)
	    {
		if (releaseButton == -1 ||
		    releaseButton == event->xbutton.button)
		{
		    CompAction *action;

		    action = &opt[MOVE_DISPLAY_OPTION_INITIATE_BUTTON].value.action;
		    moveTerminate (display, action, CompActionStateTermButton,
				   NULL, 0);
		}
	    }
	}
	break;
    case KeyPress:
	s = display->findScreen (event->xkey.root);
	if (s)
	{
	    MOVE_SCREEN (s);

	    if (ms->grabIndex)
	    {
		int i;

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
		    CompOption o[5];
		    int	       xRoot, yRoot;
		    int	       option;

		    o[0].type    = CompOptionTypeInt;
		    o[0].name    = "window";
		    o[0].value.i = event->xclient.window;

		    if (event->xclient.data.l[2] == WmMoveResizeMoveKeyboard)
		    {
			option = MOVE_DISPLAY_OPTION_INITIATE_KEY;

			moveInitiate (display, &opt[option].value.action,
				      CompActionStateInitKey,
				      o, 1);
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
			    o[1].type	 = CompOptionTypeInt;
			    o[1].name	 = "modifiers";
			    o[1].value.i = mods;

			    o[2].type	 = CompOptionTypeInt;
			    o[2].name	 = "x";
			    o[2].value.i = event->xclient.data.l[0];

			    o[3].type	 = CompOptionTypeInt;
			    o[3].name	 = "y";
			    o[3].value.i = event->xclient.data.l[1];

			    o[4].type    = CompOptionTypeInt;
			    o[4].name    = "button";
			    o[4].value.i = event->xclient.data.l[3] ?
				           event->xclient.data.l[3] : -1;

			    moveInitiate (display,
					  &opt[option].value.action,
					  CompActionStateInitButton,
					  o, 5);

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
			   &opt[option].value.action,
			   0, NULL, 0);
	    option = MOVE_DISPLAY_OPTION_INITIATE_KEY;
	    moveTerminate (display,
			   &opt[option].value.action,
			   0, NULL, 0);
	}
	break;
    case UnmapNotify:
	if (w && w->id () == event->xunmap.window)
	{
	    int option;

	    option = MOVE_DISPLAY_OPTION_INITIATE_BUTTON;
	    moveTerminate (display,
			   &opt[option].value.action,
			   0, NULL, 0);
	    option = MOVE_DISPLAY_OPTION_INITIATE_KEY;
	    moveTerminate (display,
			   &opt[option].value.action,
			   0, NULL, 0);
	}
    default:
	break;
    }

    display->handleEvent (event);
}

bool
MoveWindow::paint (const WindowPaintAttrib *attrib,
		   const CompTransform	 *transform,
		   Region			 region,
		   unsigned int		 mask)
{
    WindowPaintAttrib sAttrib;
    bool	      status;

    MOVE_SCREEN (window->screen ());

    if (ms->grabIndex)
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

static CompOption *
moveGetDisplayOptions (CompObject  *object,
		       int	   *count)
{
    CORE_DISPLAY (object);
    MOVE_DISPLAY (d);

    *count = NUM_OPTIONS (md);
    return md->opt;
}

static bool
moveSetDisplayOption (CompObject      *object,
		      const char      *name,
		      CompOptionValue *value)
{
    CompOption *o;
    int	       index;

    CORE_DISPLAY (object);
    MOVE_DISPLAY (d);

    o = compFindOption (md->opt, NUM_OPTIONS (md), name, &index);
    if (!o)
	return FALSE;

    switch (index) {
    case MOVE_DISPLAY_OPTION_OPACITY:
	if (compSetIntOption (o, value))
	{
	    md->moveOpacity = (o->value.i * OPAQUE) / 100;
	    return TRUE;
	}
	break;
    default:
	return compSetDisplayOption (d, o, value);
    }

    return FALSE;
}

static const CompMetadataOptionInfo moveDisplayOptionInfo[] = {
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
    MoveDisplay *md;
    int	        i;

    CORE_DISPLAY (o);

    if (!checkPluginABI ("core", CORE_ABIVERSION))
	return false;

    md = new MoveDisplay (d);
    if (!md)
	return false;

    if (!compInitDisplayOptionsFromMetadata (d,
					     &moveMetadata,
					     moveDisplayOptionInfo,
					     md->opt,
					     MOVE_DISPLAY_OPTION_NUM))
    {
	delete md;
	return false;
    }

    md->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (md->screenPrivateIndex < 0)
    {
	compFiniDisplayOptions (d, md->opt, MOVE_DISPLAY_OPTION_NUM);
	delete md;
	return false;
    }

    md->moveOpacity =
	(md->opt[MOVE_DISPLAY_OPTION_OPACITY].value.i * OPAQUE) / 100;

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

    freeScreenPrivateIndex (d, md->screenPrivateIndex);

    compFiniDisplayOptions (d, md->opt, MOVE_DISPLAY_OPTION_NUM);

    delete md;
}

static bool
moveInitScreen (CompObject *o)
{
    MoveScreen *ms;

    CORE_SCREEN (o);
    MOVE_DISPLAY (s->display ());

    ms = new MoveScreen (s);
    if (!ms)
	return false;

    ms->windowPrivateIndex = allocateWindowPrivateIndex (s);
    if (ms->windowPrivateIndex < 0)
    {
	delete ms;
	return false;
    }

    ms->grabIndex = 0;

    ms->moveCursor = XCreateFontCursor (s->display ()->dpy (), XC_fleur);

    s->privates[md->screenPrivateIndex].ptr = ms;

    return true;
}

static void
moveFiniScreen (CompObject *o)
{
    CORE_SCREEN (o);
    MOVE_SCREEN (s);

    freeWindowPrivateIndex (s, ms->windowPrivateIndex);

    if (ms->moveCursor)
	XFreeCursor (s->display ()->dpy (), ms->moveCursor);

    delete ms;
}

static bool
moveInitWindow (CompObject *o)
{
    MoveWindow *mw;

    CORE_WINDOW (o);
    MOVE_SCREEN (w->screen ());

    mw = new MoveWindow (w);
    if (!mw)
	return false;

    w->privates[ms->windowPrivateIndex].ptr = mw;

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

CompOption *
MovePluginVTable::getObjectOptions (CompObject *object,
		      int	 *count)
{
    static GetPluginObjectOptionsProc dispTab[] = {
	(GetPluginObjectOptionsProc) 0, /* GetCoreOptions */
	(GetPluginObjectOptionsProc) moveGetDisplayOptions
    };

    RETURN_DISPATCH (object, dispTab, ARRAY_SIZE (dispTab),
		     (CompOption *) (*count = 0), (object, count));
}

bool
MovePluginVTable::setObjectOption (CompObject      *object,
		     const char      *name,
		     CompOptionValue *value)
{
    static SetPluginObjectOptionProc dispTab[] = {
	(SetPluginObjectOptionProc) 0, /* SetCoreOption */
	(SetPluginObjectOptionProc) moveSetDisplayOption
    };

    RETURN_DISPATCH (object, dispTab, ARRAY_SIZE (dispTab), FALSE,
		     (object, name, value));
}

bool
MovePluginVTable::init ()
{
    if (!compInitPluginMetadataFromInfo (&moveMetadata,
					 name (),
					 moveDisplayOptionInfo,
					 MOVE_DISPLAY_OPTION_NUM,
					 0, 0))
	return false;

    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
    {
	compFiniMetadata (&moveMetadata);
	return false;
    }

    compAddMetadataFromFile (&moveMetadata, name ());

    return true;
}

void
MovePluginVTable::fini ()
{
    freeDisplayPrivateIndex (displayPrivateIndex);
    compFiniMetadata (&moveMetadata);
}

CompMetadata *
MovePluginVTable::getMetadata ()
{
    return &moveMetadata;
}

MovePluginVTable moveVTable;

CompPluginVTable *
getCompPluginInfo20080805 (void)
{
    return &moveVTable;
}
