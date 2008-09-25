#include <core/core.h>
#include <core/privatehandler.h>

#include <composite/composite.h>
#include <opengl/opengl.h>

#define NUM_KEYS (sizeof (mKeys) / sizeof (mKeys[0]))

#define KEY_MOVE_INC 24

#define SNAP_BACK 20
#define SNAP_OFF  100

#define MOVE_OPTION_INITIATE_BUTTON   0
#define MOVE_OPTION_INITIATE_KEY      1
#define MOVE_OPTION_OPACITY	      2
#define MOVE_OPTION_CONSTRAIN_Y	      3
#define MOVE_OPTION_SNAPOFF_MAXIMIZED 4
#define MOVE_OPTION_LAZY_POSITIONING  5
#define MOVE_OPTION_NUM		      6

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

class MoveScreen :
    public ScreenInterface,
    public PrivateHandler<MoveScreen,CompScreen>
{
    public:
	
	MoveScreen (CompScreen *screen);
	~MoveScreen ();

	CompOption::Vector & getOptions ();
	bool setOption (const char *name, CompOption::Value &value);	

	void handleEvent (XEvent *);
	
	CompWindow *w;
	int        savedX;
	int        savedY;
	int        x;
	int        y;
	Region     region;
	int        status;
	KeyCode    key[NUM_KEYS];

	int releaseButton;

	GLushort moveOpacity;
	
	CompScreen::GrabHandle grab;

	Cursor moveCursor;

	unsigned int origState;

	int	snapOffY;
	int	snapBackY;

	bool hasCompositing;

	CompOption::Vector opt;
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

	bool glPaint (const GLWindowPaintAttrib &, const GLMatrix &,
		      const CompRegion &, unsigned int);

	CompWindow      *window;
	GLWindow        *gWindow;
	CompositeWindow *cWindow;
};

#define MOVE_SCREEN(s) \
    MoveScreen *ms = MoveScreen::get (s)

#define MOVE_WINDOW(w) \
    MoveWindow *mw = MoveWindow::get (w)


class MovePluginVTable :
    public CompPlugin::VTableForScreenAndWindow<MoveScreen, MoveWindow>
{
    public:

	const char * name () { return "move"; };

	CompMetadata * getMetadata ();

	bool init ();
	void fini ();

	PLUGIN_OPTION_HELPER (MoveScreen);

};

