#ifndef _COMPDISPLAY_H
#define _COMPDISPLAY_H

#include <list>

#include <X11/Xlib-xcb.h>
#include <X11/Xutil.h>
#include <X11/Xregion.h>
#include <X11/extensions/Xinerama.h>

#include <compobject.h>
#include <compmatch.h>
#include <compcore.h>
#include <compaction.h>
#include <compwrapsystem.h>

class CompDisplay;
class CompScreen;
class CompOutput;
class PrivateDisplay;
typedef std::list<CompScreen *> CompScreenList;

extern REGION     emptyRegion;
extern REGION     infiniteRegion;

extern int lastPointerX;
extern int lastPointerY;
extern int pointerX;
extern int pointerY;

#define GET_CORE_DISPLAY(object) (dynamic_cast<CompDisplay *> (object))
#define CORE_DISPLAY(object) CompDisplay *d = GET_CORE_DISPLAY (object)

class DisplayInterface :
    public WrapableInterface<CompDisplay, DisplayInterface>
{
    public:

	virtual void handleEvent (XEvent *event);
        virtual void handleCompizEvent (const char * plugin, const char *event,
					CompOption::Vector &options);

        virtual bool fileToImage (const char *path, const char *name,
				  int *width, int *height,
				  int *stride, void **data);
	virtual bool imageToFile (const char *path, const char *name,
				  const char *format, int width, int height,
				  int stride, void *data);

	virtual CompMatch::Expression *matchInitExp (const CompString value);

	virtual void matchExpHandlerChanged ();
	virtual void matchPropertyChanged (CompWindow *window);

	virtual void logMessage (const char   *componentName,
				 CompLogLevel level,
				 const char   *message);		
};

class CompDisplay :
    public WrapableHandler<DisplayInterface, 8>,
    public CompObject
{

    public:

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

	CompString objectName ();

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
	
	CompScreenList &
	screens();

	CompOption *
	getOption (const char *);

	bool
	setOption (const char        *name,
		   CompOption::Value &value);

	std::vector<XineramaScreenInfo> &
	screenInfo ();

	bool
	XRandr ();

	int randrEvent ();
	
	bool
	XShape ();

	int shapeEvent ();

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

	CompWindow *
	findWindow (Window id);

	CompWindow *
	findTopLevelWindow (Window id);



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

	static int checkForError (Display *dpy);
	
	// wrapable interface
	WRAPABLE_HND (0, DisplayInterface, void, handleEvent, XEvent *event)
	WRAPABLE_HND (1, DisplayInterface, void, handleCompizEvent,
		      const char *, const char *, CompOption::Vector &)

	WRAPABLE_HND (2, DisplayInterface, bool, fileToImage, const char *,
		     const char *,  int *, int *, int *, void **data)
	WRAPABLE_HND (3, DisplayInterface, bool, imageToFile, const char *,
		      const char *, const char *, int, int, int, void *)

	
	WRAPABLE_HND (4, DisplayInterface, CompMatch::Expression *,
		      matchInitExp, const CompString);
	WRAPABLE_HND (5, DisplayInterface, void, matchExpHandlerChanged)
	WRAPABLE_HND (6, DisplayInterface, void, matchPropertyChanged,
		      CompWindow *)

	WRAPABLE_HND (7, DisplayInterface, void, logMessage, const char *,
		      CompLogLevel, const char*)

    private:

	PrivateDisplay *priv;

    public:

	static bool
	runCommandDispatch (CompDisplay        *d,
			    CompAction         *action,
			    CompAction::State  state,
			    CompOption::Vector &options);

	static bool
	runCommandScreenshot(CompDisplay        *d,
			     CompAction         *action,
			     CompAction::State  state,
			     CompOption::Vector &options);

	static bool
	runCommandWindowScreenshot(CompDisplay        *d,
				   CompAction         *action,
				   CompAction::State  state,
				   CompOption::Vector &options);

	static bool
	runCommandTerminal (CompDisplay        *d,
			    CompAction         *action,
			    CompAction::State  state,
			    CompOption::Vector &options);

	static CompOption::Vector &
	getDisplayOptions (CompObject  *object);

	static bool setDisplayOption (CompObject        *object,
				      const char        *name,
				      CompOption::Value &value);
};


#endif
