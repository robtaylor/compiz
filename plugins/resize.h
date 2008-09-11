#ifndef _RESIZE_H
#define _RESIZE_H

#include <compiz-core.h>
#include <compprivatehandler.h>

#include <composite/composite.h>
#include <opengl/opengl.h>

#define RESIZE_DISPLAY_OPTION_INITIATE_NORMAL_KEY    0
#define RESIZE_DISPLAY_OPTION_INITIATE_OUTLINE_KEY   1
#define RESIZE_DISPLAY_OPTION_INITIATE_RECTANGLE_KEY 2
#define RESIZE_DISPLAY_OPTION_INITIATE_STRETCH_KEY   3
#define RESIZE_DISPLAY_OPTION_INITIATE_BUTTON	     4
#define RESIZE_DISPLAY_OPTION_INITIATE_KEY	     5
#define RESIZE_DISPLAY_OPTION_MODE	             6
#define RESIZE_DISPLAY_OPTION_BORDER_COLOR           7
#define RESIZE_DISPLAY_OPTION_FILL_COLOR             8
#define RESIZE_DISPLAY_OPTION_NORMAL_MATCH	     9
#define RESIZE_DISPLAY_OPTION_OUTLINE_MATCH	     10
#define RESIZE_DISPLAY_OPTION_RECTANGLE_MATCH	     11
#define RESIZE_DISPLAY_OPTION_STRETCH_MATCH	     12
#define RESIZE_DISPLAY_OPTION_NUM		     13

class ResizePluginVTable : public CompPlugin::VTable
{
    public:

	const char * name () { return "resize"; };

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

#define RESIZE_DISPLAY(s) ResizeDisplay *rd = ResizeDisplay::get(d)
#define RESIZE_SCREEN(s) ResizeScreen *rs = ResizeScreen::get(s)
#define RESIZE_WINDOW(w) ResizeWindow *rw = ResizeWindow::get(w)

#define ResizeUpMask    (1L << 0)
#define ResizeDownMask  (1L << 1)
#define ResizeLeftMask  (1L << 2)
#define ResizeRightMask (1L << 3)

#define RESIZE_MODE_NORMAL    0
#define RESIZE_MODE_OUTLINE   1
#define RESIZE_MODE_RECTANGLE 2
#define RESIZE_MODE_STRETCH   3
#define RESIZE_MODE_LAST      RESIZE_MODE_STRETCH

struct _ResizeKeys {
    const char	 *name;
    int		 dx;
    int		 dy;
    unsigned int warpMask;
    unsigned int resizeMask;
} rKeys[] = {
    { "Left",  -1,  0, ResizeLeftMask | ResizeRightMask, ResizeLeftMask },
    { "Right",  1,  0, ResizeLeftMask | ResizeRightMask, ResizeRightMask },
    { "Up",     0, -1, ResizeUpMask | ResizeDownMask,    ResizeUpMask },
    { "Down",   0,  1, ResizeUpMask | ResizeDownMask,    ResizeDownMask }
};

#define NUM_KEYS (sizeof (rKeys) / sizeof (rKeys[0]))

#define MIN_KEY_WIDTH_INC  24
#define MIN_KEY_HEIGHT_INC 24

class ResizeDisplay :
    public DisplayInterface,
    public PrivateHandler<ResizeDisplay,CompDisplay>
{
    public:
	ResizeDisplay (CompDisplay *d);
	~ResizeDisplay ();

	CompOption::Vector & getOptions ();
	bool setOption (const char *name, CompOption::Value &value);

	void handleEvent (XEvent *event);

	void getPaintRectangle (BoxPtr pBox);
	void getStretchRectangle (BoxPtr pBox);

	void sendResizeNotify ();
	void updateWindowProperty ();

	void finishResizing ();

	void updateWindowSize ();

    public:
	CompDisplay      *display;

	Atom resizeNotifyAtom;
	Atom resizeInformationAtom;

	CompWindow	 *w;
	int		 mode;
	XRectangle	 savedGeometry;
	XRectangle	 geometry;

	int		 releaseButton;
	unsigned int mask;
	int		 pointerDx;
	int		 pointerDy;
	KeyCode	 key[NUM_KEYS];

	CompOption::Vector opt;
};

class ResizeScreen :
    public PrivateHandler<ResizeScreen,CompScreen>,
    public GLScreenInterface
{
    public:
	ResizeScreen (CompScreen *s);
	~ResizeScreen ();

	bool glPaintOutput (const GLScreenPaintAttrib &,
			    const GLMatrix &, Region, CompOutput *,
			    unsigned int);

	void damageRectangle (BoxPtr pBox);
	Cursor cursorFromResizeMask (unsigned int mask);

	void handleKeyEvent (KeyCode keycode);
	void handleMotionEvent (int xRoot, int yRoot);

	void glPaintRectangle (const GLScreenPaintAttrib &sAttrib,
			       const GLMatrix            &transform,
			       CompOutput                *output,
			       unsigned short            *borderColor,
			       unsigned short            *fillColor);

    public:
	CompScreen      *screen;
	GLScreen        *gScreen;
	CompositeScreen *cScreen;
	ResizeDisplay   *rDisplay;

    	CompScreen::grabHandle grabIndex;

	Cursor leftCursor;
	Cursor rightCursor;
	Cursor upCursor;
	Cursor upLeftCursor;
	Cursor upRightCursor;
	Cursor downCursor;
	Cursor downLeftCursor;
	Cursor downRightCursor;
	Cursor middleCursor;
	Cursor cursor[NUM_KEYS];
};

class ResizeWindow :
    public WindowInterface,
    public CompositeWindowInterface,
    public GLWindowInterface,
    public PrivateHandler<ResizeWindow,CompWindow>
{
    public:
	ResizeWindow (CompWindow *w);
	~ResizeWindow ();
	
	void resizeNotify (int, int, int, int);

	bool damageRect (bool, BoxPtr);

	bool glPaint (const GLWindowPaintAttrib &, const GLMatrix &,
		      Region, unsigned int);

	void getStretchScale (BoxPtr pBox, float *xScale, float *yScale);

	
    public:

	CompWindow      *window;
	GLWindow        *gWindow;
	CompositeWindow *cWindow;
	ResizeScreen    *rScreen;
	ResizeDisplay   *rDisplay;
};

#endif