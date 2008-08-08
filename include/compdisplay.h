#ifndef _COMPDISPLAY_H
#define _COMPDISPLAY_H

#include <list>
#include "wrapable.h"

class CompDisplay;
class CompScreen;
class PrivateDisplay;

#define GET_CORE_DISPLAY(object) (dynamic_cast<CompDisplay *> (object))
#define CORE_DISPLAY(object) CompDisplay *d = GET_CORE_DISPLAY (object)

class DisplayInterface : public WrapableInterface<CompDisplay> {
    public:
	DisplayInterface ();

    WRAPABLE_DEF(void, handleEvent, XEvent *event);
    WRAPABLE_DEF(void, handleCompizEvent, const char *,
		 const char *, CompOption *, int nOption);

    WRAPABLE_DEF(bool, fileToImage, const char *, const char *,
		 int *, int *, int *, void **data);
    WRAPABLE_DEF(bool, imageToFile, const char *, const char *,
		 const char *, int, int, int, void *);

	
    WRAPABLE_DEF(void, matchInitExp, CompMatchExp *, const char *);
    WRAPABLE_DEF(void, matchExpHandlerChanged)
    WRAPABLE_DEF(void, matchPropertyChanged, CompWindow *)

    WRAPABLE_DEF(void, logMessage, const char *, CompLogLevel, const char*)
};

class CompDisplay : public WrapableHandler<DisplayInterface>, public CompObject {

    public:
	CompDisplay *next;

	class Atoms {
	    public:
		Atom supported;
		Atom supportingWmCheck;

		Atom utf8String;

		Atom wmName;

		Atom winType;
		Atom winTypeDesktop;
		Atom winTypeDock;
		Atom winTypeToolbar;
		Atom winTypeMenu;
		Atom winTypeUtil;
		Atom winTypeSplash;
		Atom winTypeDialog;
		Atom winTypeNormal;
		Atom winTypeDropdownMenu;
		Atom winTypePopupMenu;
		Atom winTypeTooltip;
		Atom winTypeNotification;
		Atom winTypeCombo;
		Atom winTypeDnd;

		Atom winOpacity;
		Atom winBrightness;
		Atom winSaturation;
		Atom winActive;
		Atom winDesktop;

		Atom workarea;

		Atom desktopViewport;
		Atom desktopGeometry;
		Atom currentDesktop;
		Atom numberOfDesktops;

		Atom winState;
		Atom winStateModal;
		Atom winStateSticky;
		Atom winStateMaximizedVert;
		Atom winStateMaximizedHorz;
		Atom winStateShaded;
		Atom winStateSkipTaskbar;
		Atom winStateSkipPager;
		Atom winStateHidden;
		Atom winStateFullscreen;
		Atom winStateAbove;
		Atom winStateBelow;
		Atom winStateDemandsAttention;
		Atom winStateDisplayModal;

		Atom winActionMove;
		Atom winActionResize;
		Atom winActionStick;
		Atom winActionMinimize;
		Atom winActionMaximizeHorz;
		Atom winActionMaximizeVert;
		Atom winActionFullscreen;
		Atom winActionClose;
		Atom winActionShade;
		Atom winActionChangeDesktop;
		Atom winActionAbove;
		Atom winActionBelow;

		Atom wmAllowedActions;

		Atom wmStrut;
		Atom wmStrutPartial;

		Atom wmUserTime;

		Atom wmIcon;
		Atom wmIconGeometry;

		Atom clientList;
		Atom clientListStacking;

		Atom frameExtents;
		Atom frameWindow;

		Atom wmState;
		Atom wmChangeState;
		Atom wmProtocols;
		Atom wmClientLeader;

		Atom wmDeleteWindow;
		Atom wmTakeFocus;
		Atom wmPing;
		Atom wmSyncRequest;

		Atom wmSyncRequestCounter;

		Atom closeWindow;
		Atom wmMoveResize;
		Atom moveResizeWindow;
		Atom restackWindow;

		Atom showingDesktop;

		Atom xBackground[2];

		Atom toolkitAction;
		Atom toolkitActionMainMenu;
		Atom toolkitActionRunDialog;
		Atom toolkitActionWindowMenu;
		Atom toolkitActionForceQuitDialog;

		Atom mwmHints;

		Atom xdndAware;
		Atom xdndEnter;
		Atom xdndLeave;
		Atom xdndPosition;
		Atom xdndStatus;
		Atom xdndDrop;

		Atom manager;
		Atom targets;
		Atom multiple;
		Atom timestamp;
		Atom version;
		Atom atomPair;

		Atom startupId;
	};
	
    public:
	// functions
	CompDisplay ();
	~CompDisplay ();

	CompString name ();

	bool
	init (const char *name);

	bool
	addScreen (int screenNum);

	void
	removeScreen (CompScreen *);

	Atoms
	atoms();
	
	Display *
	dpy();
	
	CompScreen *
	screens();

	GLenum
	textureFilter ();

	CompOption *
	getOption (const char *);

	bool
	setOption (const char      *name,
		   CompOptionValue *value);

	std::vector<XineramaScreenInfo> &
	screenInfo ();

	bool
	XRandr ();
	
	bool
	XShape ();

	SnDisplay *
	snDisplay ();

	Window
	below ();
	
	Window
	activeWindow ();
	
	Window
	autoRaiseWindow ();

	XModifierKeymap *
	modMap ();
	
	unsigned int
	ignoredModMask ();

	const char *
	displayString ();

	unsigned int
	lastPing ();

	CompWatchFdHandle
	getWatchFdHandle ();

	void
	setWatchFdHandle (CompWatchFdHandle);
	
	void
	processEvents ();

	void
	updateModifierMappings ();

	unsigned int
	virtualToRealModMask (unsigned int modMask);

	unsigned int
	keycodeToModifiers (int keycode);

	CompScreen *
	findScreen (Window root);

	void
	forEachWindow (ForEachWindowProc proc,
		       void	         *closure);

	CompWindow *
	findWindow (Window id);

	CompWindow *
	findTopLevelWindow (Window id);


	void
	clearTargetOutput (unsigned int mask);

	bool
	readImageFromFile (const char  *name,
			   int	       *width,
			   int	       *height,
			   void	       **data);

	bool
	writeImageToFile (const char  *path,
			  const char  *name,
			  const char  *format,
			  int	      width,
			  int	      height,
			  void	      *data);

	CompCursor *
	findCursor ();

	void
	updateScreenInfo ();

	Window
	getActiveWindow (Window root);

	int
	getWmState (Window id);

	void
	setWmState (int state, Window id);

	unsigned int
	windowStateMask (Atom state);


	static unsigned int
	windowStateFromString (const char *str);

	unsigned int
	getWindowState (Window id);

	void
	setWindowState (unsigned int state, Window id);

	unsigned int
	getWindowType (Window id);

	void
	getMwmHints (Window	  id,
		     unsigned int *func,
		     unsigned int *decor);


	unsigned int
	getProtocols (Window      id);


	unsigned int
	getWindowProp (Window	    id,
		       Atom	    property,
		       unsigned int defaultValue);


	void
	setWindowProp (Window       id,
		       Atom	    property,
		       unsigned int value);


	bool
	readWindowProp32 (Window	 id,
			  Atom		 property,
			  unsigned short *returnValue);


	unsigned short
	getWindowProp32 (Window		id,
			 Atom		property,
			 unsigned short defaultValue);


	void
	setWindowProp32 (Window         id,
			 Atom		property,
			 unsigned short value);

	void
	addScreenActions (CompScreen *s);

	static int allocPrivateIndex ();
	static void freePrivateIndex (int index);
	
	// wrapable interface
	WRAPABLE_HND(void, handleEvent, XEvent *event)
	WRAPABLE_HND(void, handleCompizEvent, const char *,
		     const char *, CompOption *, int nOption)

	WRAPABLE_HND(bool, fileToImage, const char *, const char *,
		     int *, int *, int *, void **data)
	WRAPABLE_HND(bool, imageToFile, const char *, const char *,
		     const char *, int, int, int, void *)

	
	WRAPABLE_HND(void, matchInitExp, CompMatchExp *, const char *);
	WRAPABLE_HND(void, matchExpHandlerChanged)
	WRAPABLE_HND(void, matchPropertyChanged, CompWindow *)

	WRAPABLE_HND(void, logMessage, const char *, CompLogLevel, const char*)

    public:
	Region mTmpRegion;
	Region mOutputRegion;

    private:

	PrivateDisplay *priv;

    public:

	static bool
	runCommandDispatch (CompDisplay     *d,
			    CompAction      *action,
			    CompActionState state,
			    CompOption      *option,
			    int		    nOption);

	static bool
	runCommandScreenshot (CompDisplay     *d,
			      CompAction      *action,
			      CompActionState state,
			      CompOption      *option,
			      int	      nOption);

	static bool
	runCommandWindowScreenshot (CompDisplay     *d,
				    CompAction      *action,
				    CompActionState state,
				    CompOption      *option,
				    int	            nOption);

	static bool
	runCommandTerminal (CompDisplay     *d,
			    CompAction      *action,
			    CompActionState state,
			    CompOption      *option,
			    int	            nOption);

	static CompOption *
	getDisplayOptions (CompObject  *object,
			   int	       *count);

	static bool
	pingTimeout (void *closure);
};

extern Bool inHandleEvent;

extern CompScreen *targetScreen;
extern CompOutput *targetOutput;

#endif
