#ifndef _COMPSCREEN_H
#define _COMPSCREEN_H

#include <compwindow.h>
#include <compoutput.h>
#include <compsession.h>

class CompScreen;
class PrivateScreen;
typedef std::list<CompWindow *> CompWindowList;

extern char       *backgroundImage;
extern bool       replaceCurrentWm;
extern bool       indirectRendering;
extern bool       strictBinding;
extern bool       noDetection;

/* camera distance from screen, 0.5 * tan (FOV) */
#define DEFAULT_Z_CAMERA 0.866025404f

#define OPAQUE 0xffff
#define COLOR  0xffff
#define BRIGHT 0xffff

#define GET_CORE_SCREEN(object) (dynamic_cast<CompScreen *> (object))
#define CORE_SCREEN(object) CompScreen *s = GET_CORE_SCREEN (object)

#define PAINT_SCREEN_REGION_MASK		   (1 << 0)
#define PAINT_SCREEN_FULL_MASK			   (1 << 1)
#define PAINT_SCREEN_TRANSFORMED_MASK		   (1 << 2)
#define PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK (1 << 3)
#define PAINT_SCREEN_CLEAR_MASK			   (1 << 4)
#define PAINT_SCREEN_NO_OCCLUSION_DETECTION_MASK   (1 << 5)
#define PAINT_SCREEN_NO_BACKGROUND_MASK            (1 << 6)



struct CompGroup {
    unsigned int      refCnt;
    Window	      id;
};

struct CompStartupSequence {
    SnStartupSequence		*sequence;
    unsigned int		viewportX;
    unsigned int		viewportY;
};


#define SCREEN_EDGE_LEFT	0
#define SCREEN_EDGE_RIGHT	1
#define SCREEN_EDGE_TOP		2
#define SCREEN_EDGE_BOTTOM	3
#define SCREEN_EDGE_TOPLEFT	4
#define SCREEN_EDGE_TOPRIGHT	5
#define SCREEN_EDGE_BOTTOMLEFT	6
#define SCREEN_EDGE_BOTTOMRIGHT 7
#define SCREEN_EDGE_NUM		8

struct CompScreenEdge {
    Window	 id;
    unsigned int count;
};


#define ACTIVE_WINDOW_HISTORY_SIZE 64
#define ACTIVE_WINDOW_HISTORY_NUM  32

struct CompActiveWindowHistory {
    Window id[ACTIVE_WINDOW_HISTORY_SIZE];
    int    x;
    int    y;
    int    activeNum;
};

class ScreenInterface : public WrapableInterface<CompScreen, ScreenInterface> {
    public:
	virtual void enterShowDesktopMode ();
	virtual void leaveShowDesktopMode (CompWindow *window);

	virtual void outputChangeNotify ();
};


class CompScreen :
    public WrapableHandler<ScreenInterface, 3>,
    public CompObject
{

    public:
	typedef void* grabHandle;

    public:
	CompScreen ();
	~CompScreen ();

	CompString objectName ();

	bool
	init (CompDisplay *, int);

	bool
	init (CompDisplay *, int, Window, Atom, Time);

	CompDisplay *
	display ();
	
	Window
	root ();

	XWindowAttributes
	attrib ();

	int
	screenNum ();

	CompWindowList &
	windows ();

	CompOption *
	getOption (const char *name);

	unsigned int
	showingDesktopMask ();
	
	bool
	setOption (const char       *name,
		   CompOption::Value &value);

	void
	setCurrentOutput (unsigned int outputNum);

	void
	configure (XConfigureEvent *ce);

	bool
	hasGrab ();

	void
	warpPointer (int dx, int dy);

	Time
	getCurrentTime ();

	Atom
	selectionAtom ();

	Window
	selectionWindow ();

	Time
	selectionTimestamp ();

	void
	updateWorkareaForScreen ();

	void
	forEachWindow (CompWindow::ForEach);

	void
	focusDefaultWindow ();

	CompWindow *
	findWindow (Window id);

	CompWindow *
	findTopLevelWindow (Window id, bool override_redirect = false);

	void
	insertWindow (CompWindow *w, Window aboveId);

	void
	unhookWindow (CompWindow *w);

	grabHandle
	pushGrab (Cursor cursor, const char *name);

	void
	updateGrab (grabHandle handle, Cursor cursor);

	void
	removeGrab (grabHandle handle, CompPoint *restorePointer);

	bool
	otherGrabExist (const char *, ...);

	bool
	addAction (CompAction *action);

	void
	removeAction (CompAction *action);

	void
	updatePassiveGrabs ();

	void
	updateWorkarea ();

	void
	updateClientList ();

	void
	toolkitAction (Atom	  toolkitAction,
		       Time       eventTime,
		       Window	  window,
		       long	  data0,
		       long	  data1,
		       long	  data2);

	void
	runCommand (CompString command);

	void
	moveViewport (int tx, int ty, bool sync);

	CompGroup *
	addGroup (Window id);

	void
	removeGroup (CompGroup *group);

	CompGroup *
	findGroup (Window id);

	void
	applyStartupProperties (CompWindow *window);

	void
	sendWindowActivationRequest (Window id);


	void
	enableEdge (int edge);

	void
	disableEdge (int edge);

	Window
	getTopWindow ();

	int
	outputDeviceForPoint (int x, int y);

	void
	getCurrentOutputExtents (int *x1, int *y1, int *x2, int *y2);

	void
	setNumberOfDesktops (unsigned int nDesktop);

	void
	setCurrentDesktop (unsigned int desktop);

	void
	getWorkareaForOutput (int output, XRectangle *area);


	void
	viewportForGeometry (CompWindow::Geometry gm,
			     int                  *viewportX,
			     int                  *viewportY);

	int
	outputDeviceForGeometry (CompWindow::Geometry gm);

	bool
	updateDefaultIcon ();

	void
	setCurrentActiveWindowHistory (int x, int y);

	void
	addToCurrentActiveWindowHistory (Window id);

	CompPoint vp ();

	CompSize vpSize ();

	CompSize size ();

	unsigned int &
	pendingDestroys ();

	void
	removeDestroyed ();

	unsigned int &
	mapNum ();

	int &
	desktopWindowCount ();

	CompOutput::vector &
	outputDevs ();

	XRectangle
	workArea ();

	unsigned int
	currentDesktop ();

	unsigned int
	nDesktop ();

	CompActiveWindowHistory *
	currentHistory ();

	CompScreenEdge &
	screenEdge (int);

	unsigned int &
	activeNum ();

	Region region ();

	bool hasOverlappingOutputs ();

	CompOutput & fullscreenOutput ();

	static int allocPrivateIndex ();
	static void freePrivateIndex (int index);

	WRAPABLE_HND (0, ScreenInterface, void, enterShowDesktopMode);
	WRAPABLE_HND (1, ScreenInterface, void, leaveShowDesktopMode,
		      CompWindow *);

	WRAPABLE_HND (2, ScreenInterface, void, outputChangeNotify);


    private:
	PrivateScreen *priv;

    public :
	static bool
	mainMenu (CompDisplay        *d,
		  CompAction         *action,
		  CompAction::State  state,
		  CompOption::Vector &options);

	static bool
	runDialog (CompDisplay        *d,
		   CompAction         *action,
		   CompAction::State  state,
		   CompOption::Vector &options);

	static bool
	showDesktop (CompDisplay        *d,
		     CompAction         *action,
		     CompAction::State  state,
		     CompOption::Vector &options);

	static bool
	windowMenu (CompDisplay        *d,
		    CompAction         *action,
		    CompAction::State  state,
		    CompOption::Vector &options);

	static CompOption::Vector &
	getScreenOptions (CompObject *object);

	static bool setScreenOption (CompObject        *object,
				     const char        *name,
				     CompOption::Value &value);

	static void
	compScreenSnEvent (SnMonitorEvent *event,
			   void           *userData);
};

#endif
