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

#include <composite/composite.h>
#include <opengl/opengl.h>

static CompMetadata *moveMetadata;

class MovePluginVTable : public CompPlugin::VTable
{
    public:

	const char * name () { return "move"; };

	CompMetadata * getMetadata ();

	bool init ();
	void fini ();

	bool initObject (CompObject *object);
	void finiObject (CompObject *object);

	CompOption::Vector & getObjectOptions (CompObject *object);

	bool setObjectOption (CompObject        *object,
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
	MoveDisplay (CompDisplay *display);
	~MoveDisplay ();
 
	CompOption::Vector & getOptions ();
	bool setOption (const char *name, CompOption::Value &value);	

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
	    screen (screen), grab(NULL)
	{
	    moveCursor = XCreateFontCursor (screen->display ()->dpy (),
					    XC_fleur);
	    if (CompositeScreen::get (screen))
		hasCompositing =
		    CompositeScreen::get (screen)->compositingActive ();
	}

	~MoveScreen ()
	{
	     if (moveCursor)
		XFreeCursor (screen->display ()->dpy (), moveCursor);
	}

        CompScreen *screen;
	CompScreen::grabHandle grab;

	Cursor moveCursor;

	unsigned int origState;

	int	snapOffY;
	int	snapBackY;

	bool hasCompositing;
};

class MoveWindow :
    public GLWindowInterface,
    public PrivateHandler<MoveWindow,CompWindow>
{
    public:
	MoveWindow (CompWindow *window) :
	    PrivateHandler<MoveWindow,CompWindow> (window),
	    window (window),
	    gWindow (GLWindow::get (window)),
	    cWindow (CompositeWindow::get (window))
	{
	    if (gWindow)
		GLWindowInterface::setHandler (gWindow);
	};

	bool glPaint (const GLWindowPaintAttrib &, const GLMatrix &, Region,
		      unsigned int);

	CompWindow      *window;
	GLWindow        *gWindow;
	CompositeWindow *cWindow;
};

#define MOVE_DISPLAY(d)	\
    MoveDisplay *md = MoveDisplay::get (d)

#define MOVE_SCREEN(s) \
    MoveScreen *ms = MoveScreen::get (s)

#define MOVE_WINDOW(w) \
    MoveWindow *mw = MoveWindow::get (w)

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
			       w->geometry ().x () + (w->width () / 2));
	y = CompOption::getIntOptionNamed (options, "y",
			       w->geometry ().y () + (w->height () / 2));

	button = CompOption::getIntOptionNamed (options, "button", -1);

	if (s->otherGrabExist ("move", 0))
	    return FALSE;

	if (md->w)
	    return FALSE;

	if (w->type () & (CompWindowTypeDesktopMask |
		       CompWindowTypeDockMask    |
		       CompWindowTypeFullscreenMask))
	    return FALSE;

	if (w->overrideRedirect ())
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

		xRoot = w->geometry ().x () + (w->width () / 2);
		yRoot = w->geometry ().y () + (w->height () / 2);

		s->warpPointer (xRoot - pointerX, yRoot - pointerY);
	    }

	    if (md->moveOpacity != OPAQUE)
	    {
		MOVE_WINDOW (w);

		if (mw->cWindow)
		    mw->cWindow->addDamage ();
	    }
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
	    md->w->move (md->savedX - md->w->geometry ().x (),
			 md->savedY - md->w->geometry ().y (),
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
	{
	    MOVE_WINDOW (md->w);

	    if (mw->cWindow)
		mw->cWindow->addDamage ();
	}

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
	    int x, y;

	    x = w->geometry (). x ();
	    y = w->geometry (). y ();

	    w->move (wX + dx - x, wY + dy - y, TRUE, FALSE);

	    if (md->opt[MOVE_DISPLAY_OPTION_LAZY_POSITIONING].value ().b () &&
	        MoveScreen::get (w->screen())->hasCompositing)
	    {
		/* FIXME: This form of lazy positioning is broken and should
		   be replaced asap. Current code exists just to avoid a
		   major performance regression in the 0.5.2 release. */
		w->serverGeometry ().setX (x);
		w->serverGeometry ().setY (y);
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
MoveWindow::glPaint (const GLWindowPaintAttrib &attrib,
		     const GLMatrix            &transform,
		     Region                    region,
		     unsigned int              mask)
{
    GLWindowPaintAttrib sAttrib = attrib;
    bool                status;

    MOVE_SCREEN (window->screen ());

    if (ms->grab)
    {
	MOVE_DISPLAY (window->screen ()->display ());

	if (md->w == window && md->moveOpacity != OPAQUE)
	{
	    /* modify opacity of windows that are not active */
	    sAttrib.opacity = (sAttrib.opacity * md->moveOpacity) >> 16;
	}
    }

    status = gWindow->glPaint (sAttrib, transform, region, mask);

    return status;
}

CompOption::Vector &
MoveDisplay::getOptions ()
{
    return opt;
}
 
bool
MoveDisplay::setOption (const char        *name,
		        CompOption::Value &value)
{
    CompOption *o;
    unsigned int index;
 
    o = CompOption::findOption (opt, name, &index);
    if (!o)
	return false;
 
     switch (index) {
     case MOVE_DISPLAY_OPTION_OPACITY:
 	if (o->set (value))
 	{
	    moveOpacity = (o->value ().i () * OPAQUE) / 100;
	    return true;
 	}
 	break;
     default:
	return CompOption::setDisplayOption (display, *o, value);
     }
 
    return false;
}

static const CompMetadata::OptionInfo moveDisplayOptionInfo[] = {
    { "initiate_button", "button", 0, moveInitiate, moveTerminate },
    { "initiate_key", "key", 0, moveInitiate, moveTerminate },
    { "opacity", "int", "<min>0</min><max>100</max>", 0, 0 },
    { "constrain_y", "bool", 0, 0, 0 },
    { "snapoff_maximized", "bool", 0, 0, 0 },
    { "lazy_positioning", "bool", 0, 0, 0 }
};

MoveDisplay::MoveDisplay (CompDisplay *d) :
    PrivateHandler<MoveDisplay,CompDisplay> (d),
    display (d),
    opt(MOVE_DISPLAY_OPTION_NUM),
    w (0),
    region (NULL),
    status (RectangleOut),
    releaseButton (0)
{
    if (!moveMetadata->initDisplayOptions (d, moveDisplayOptionInfo,
					    MOVE_DISPLAY_OPTION_NUM, opt))
    {
	setFailed ();
	return;
    }
 
    moveOpacity =
	(opt[MOVE_DISPLAY_OPTION_OPACITY].value ().i () * OPAQUE) / 100;
 
    for (unsigned int i = 0; i < NUM_KEYS; i++)
	key[i] = XKeysymToKeycode (d->dpy (), XStringToKeysym (mKeys[i].name));
 
    DisplayInterface::setHandler (display);
}


MoveDisplay::~MoveDisplay ()
{
    CompOption::finiDisplayOptions (display, opt);
}

bool
MovePluginVTable::initObject (CompObject *o)
{
    INIT_OBJECT (o,_,X,X,X,,MoveDisplay,MoveScreen,MoveWindow)
    return true;
}

void
MovePluginVTable::finiObject (CompObject *o)
{
    FINI_OBJECT (o,_,X,X,X,,MoveDisplay,MoveScreen,MoveWindow)
}

CompOption::Vector &
MovePluginVTable::getObjectOptions (CompObject *object)
{
    GET_OBJECT_OPTIONS (object,X,_,MoveDisplay,)
    return noOptions;
}

bool
MovePluginVTable::setObjectOption (CompObject        *object,
				   const char        *name,
				   CompOption::Value &value)
{
    SET_OBJECT_OPTION (object,X,_,MoveDisplay,)
    return false;
}

bool
MovePluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	 return false;

    moveMetadata = new CompMetadata (name (),
				     moveDisplayOptionInfo,
				     MOVE_DISPLAY_OPTION_NUM,
				     0, 0);
    if (!moveMetadata)
	return false;

    moveMetadata->addFromFile (name ());

    return true;
}

void
MovePluginVTable::fini ()
{
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
