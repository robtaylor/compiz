#ifndef _PRIVATESCREEN_H
#define _PRIVATESCREEN_H

#include <core/core.h>
#include <core/screen.h>
#include <core/size.h>
#include <core/point.h>
#include <core/timer.h>

extern bool shutDown;
extern bool restartSignal;

typedef struct _CompWatchFd {
    int               fd;
    FdWatchCallBack   callBack;
    CompWatchFdHandle handle;
} CompWatchFd;

extern CompWindow *lastFoundWindow;
extern CompWindow *lastDamagedWindow;
extern bool	  useDesktopHints;
extern bool       onlyCurrentScreen;

extern int  defaultRefreshRate;
extern const char *defaultTextureFilter;


#define COMP_OPTION_ACTIVE_PLUGINS                   0
#define COMP_OPTION_CLICK_TO_FOCUS                   1
#define COMP_OPTION_AUTORAISE                        2
#define COMP_OPTION_AUTORAISE_DELAY                  3
#define COMP_OPTION_CLOSE_WINDOW_KEY                 4
#define COMP_OPTION_CLOSE_WINDOW_BUTTON              5
#define COMP_OPTION_MAIN_MENU_KEY                    6
#define COMP_OPTION_RUN_DIALOG_KEY                   7
#define COMP_OPTION_COMMAND0                         8
#define COMP_OPTION_COMMAND1                         9
#define COMP_OPTION_COMMAND2                         10
#define COMP_OPTION_COMMAND3                         11
#define COMP_OPTION_COMMAND4                         12
#define COMP_OPTION_COMMAND5                         13
#define COMP_OPTION_COMMAND6                         14
#define COMP_OPTION_COMMAND7                         15
#define COMP_OPTION_COMMAND8                         16
#define COMP_OPTION_COMMAND9                         17
#define COMP_OPTION_COMMAND10                        18
#define COMP_OPTION_COMMAND11                        19
#define COMP_OPTION_RUN_COMMAND0_KEY                 20
#define COMP_OPTION_RUN_COMMAND1_KEY                 21
#define COMP_OPTION_RUN_COMMAND2_KEY                 22
#define COMP_OPTION_RUN_COMMAND3_KEY                 23
#define COMP_OPTION_RUN_COMMAND4_KEY                 24
#define COMP_OPTION_RUN_COMMAND5_KEY                 25
#define COMP_OPTION_RUN_COMMAND6_KEY                 26
#define COMP_OPTION_RUN_COMMAND7_KEY                 27
#define COMP_OPTION_RUN_COMMAND8_KEY                 28
#define COMP_OPTION_RUN_COMMAND9_KEY                 29
#define COMP_OPTION_RUN_COMMAND10_KEY                30
#define COMP_OPTION_RUN_COMMAND11_KEY                31
#define COMP_OPTION_RAISE_WINDOW_KEY                 32
#define COMP_OPTION_RAISE_WINDOW_BUTTON              33
#define COMP_OPTION_LOWER_WINDOW_KEY                 34
#define COMP_OPTION_LOWER_WINDOW_BUTTON              35
#define COMP_OPTION_UNMAXIMIZE_WINDOW_KEY            36
#define COMP_OPTION_MINIMIZE_WINDOW_KEY              37
#define COMP_OPTION_MINIMIZE_WINDOW_BUTTON           38
#define COMP_OPTION_MAXIMIZE_WINDOW_KEY              39
#define COMP_OPTION_MAXIMIZE_WINDOW_HORZ_KEY         40
#define COMP_OPTION_MAXIMIZE_WINDOW_VERT_KEY         41
#define COMP_OPTION_SCREENSHOT                       42
#define COMP_OPTION_RUN_SCREENSHOT_KEY               43
#define COMP_OPTION_WINDOW_SCREENSHOT                44
#define COMP_OPTION_RUN_WINDOW_SCREENSHOT_KEY        45
#define COMP_OPTION_WINDOW_MENU_BUTTON               46
#define COMP_OPTION_WINDOW_MENU_KEY                  47
#define COMP_OPTION_SHOW_DESKTOP_KEY                 48
#define COMP_OPTION_SHOW_DESKTOP_EDGE                49
#define COMP_OPTION_RAISE_ON_CLICK                   50
#define COMP_OPTION_AUDIBLE_BELL                     51
#define COMP_OPTION_TOGGLE_WINDOW_MAXIMIZED_KEY      52
#define COMP_OPTION_TOGGLE_WINDOW_MAXIMIZED_BUTTON   53
#define COMP_OPTION_TOGGLE_WINDOW_MAXIMIZED_HORZ_KEY 54
#define COMP_OPTION_TOGGLE_WINDOW_MAXIMIZED_VERT_KEY 55
#define COMP_OPTION_HIDE_SKIP_TASKBAR_WINDOWS        56
#define COMP_OPTION_TOGGLE_WINDOW_SHADED_KEY         57
#define COMP_OPTION_IGNORE_HINTS_WHEN_MAXIMIZED      58
#define COMP_OPTION_TERMINAL                         59
#define COMP_OPTION_RUN_TERMINAL_KEY                 60
#define COMP_OPTION_PING_DELAY                       61
#define COMP_OPTION_EDGE_DELAY                       62
#define COMP_OPTION_HSIZE                            63
#define COMP_OPTION_VSIZE                            64
#define COMP_OPTION_DEFAULT_ICON                     65
#define COMP_OPTION_NUMBER_OF_DESKTOPS               66
#define COMP_OPTION_DETECT_OUTPUTS                   67
#define COMP_OPTION_OUTPUTS                          68
#define COMP_OPTION_OVERLAPPING_OUTPUTS              69
#define COMP_OPTION_FOCUS_PREVENTION_LEVEL           70
#define COMP_OPTION_FOCUS_PREVENTION_MATCH           71
#define COMP_OPTION_NUM                              72

extern bool inHandleEvent;

extern CompScreen *targetScreen;
extern CompOutput *targetOutput;


typedef struct _CompDelayedEdgeSettings
{
    CompAction::CallBack initiate;
    CompAction::CallBack terminate;

    unsigned int edge;
    unsigned int state;

    CompOption::Vector options;
} CompDelayedEdgeSettings;




#define OUTPUT_OVERLAP_MODE_SMART          0
#define OUTPUT_OVERLAP_MODE_PREFER_LARGER  1
#define OUTPUT_OVERLAP_MODE_PREFER_SMALLER 2
#define OUTPUT_OVERLAP_MODE_LAST           OUTPUT_OVERLAP_MODE_PREFER_SMALLER

#define FOCUS_PREVENTION_LEVEL_NONE     0
#define FOCUS_PREVENTION_LEVEL_LOW      1
#define FOCUS_PREVENTION_LEVEL_NORMAL   2
#define FOCUS_PREVENTION_LEVEL_HIGH     3
#define FOCUS_PREVENTION_LEVEL_VERYHIGH 4
#define FOCUS_PREVENTION_LEVEL_LAST     FOCUS_PREVENTION_LEVEL_VERYHIGH

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

struct CompGroup {
    unsigned int      refCnt;
    Window	      id;
};

struct CompStartupSequence {
    SnStartupSequence		*sequence;
    unsigned int		viewportX;
    unsigned int		viewportY;
};

extern const CompMetadata::OptionInfo
coreOptionInfo[COMP_OPTION_NUM];

class PrivateScreen {

    public:
	class KeyGrab {
	    public:
		int          keycode;
		unsigned int modifiers;
		int          count;
	};

	class ButtonGrab {
	    public:
		int          button;
		unsigned int modifiers;
		int          count;
	};

	class Grab {
	    public:

		friend class CompScreen;
	    private:
		Cursor                      cursor;
	    	const char                  *name;
	};

    public:
	PrivateScreen (CompScreen *screen);
	~PrivateScreen ();

	void processEvents ();

	void removeDestroyed ();

	void updatePassiveGrabs ();

	short int
	watchFdEvents (CompWatchFdHandle handle);

	int
	doPoll (int timeout);

	void
	handleTimers (struct timeval *tv);

	void addTimer (CompTimer *timer);
	void removeTimer (CompTimer *timer);

		void
	updatePlugins ();

	bool
	triggerButtonPressBindings (CompOption::Vector &options,
				    XButtonEvent       *event,
				    CompOption::Vector &arguments);

	bool
	triggerButtonReleaseBindings (CompOption::Vector &options,
				      XButtonEvent       *event,
				      CompOption::Vector &arguments);

	bool
	triggerKeyPressBindings (CompOption::Vector &options,
				 XKeyEvent          *event,
				 CompOption::Vector &arguments);

	bool
	triggerKeyReleaseBindings (CompOption::Vector &options,
				   XKeyEvent          *event,
				   CompOption::Vector &arguments);

	bool
	triggerStateNotifyBindings (CompOption::Vector  &options,
				    XkbStateNotifyEvent *event,
				    CompOption::Vector  &arguments);
	bool
	triggerEdgeEnter (unsigned int       edge,
			  CompAction::State  state,
     			  CompOption::Vector &arguments);

	void
	setAudibleBell (bool audible);

	bool
	handlePingTimeout ();
	
	bool
	handleActionEvent (XEvent *event);

	void
	handleSelectionRequest (XEvent *event);

	void
	handleSelectionClear (XEvent *event);
	
	bool
	desktopHintEqual (unsigned long *data,
			  int           size,
			  int           offset,
			  int           hintSize);

	void
	setDesktopHints ();

	void
	setVirtualScreenSize (int hsize, int vsize);

	void
	updateOutputDevices ();

	void
	detectOutputDevices ();

	void
	updateStartupFeedback ();

	void
	updateScreenEdges ();

	void
	reshape (int w, int h);

	bool
	handleStartupSequenceTimeout();

	void
	addSequence (SnStartupSequence *sequence);

	void
	removeSequence (SnStartupSequence *sequence);

	void
	setSupportingWmCheck ();

	void
	setSupported ();

	void
	getDesktopHints ();

	void
	grabUngrabOneKey (unsigned int modifiers,
			  int          keycode,
			  bool         grab);


	bool
	grabUngrabKeys (unsigned int modifiers,
			int          keycode,
			bool         grab);

	bool
	addPassiveKeyGrab (CompAction::KeyBinding &key);

	void
	removePassiveKeyGrab (CompAction::KeyBinding &key);

	void
	updatePassiveKeyGrabs ();

	bool
	addPassiveButtonGrab (CompAction::ButtonBinding &button);

	void
	removePassiveButtonGrab (CompAction::ButtonBinding &button);

	void
	computeWorkareaForBox (BoxPtr pBox, XRectangle *area);

	void updateScreenInfo ();


	void updateModifierMappings ();

	unsigned int virtualToRealModMask (unsigned int modMask);

	unsigned int keycodeToModifiers (int keycode);

	Window getActiveWindow (Window root);

	int getWmState (Window id);

	void setWmState (int state, Window id);

	unsigned int windowStateMask (Atom state);

	static unsigned int windowStateFromString (const char *str);

	unsigned int getWindowState (Window id);

	void setWindowState (unsigned int state, Window id);

	unsigned int getWindowType (Window id);

	void getMwmHints (Window       id,
			  unsigned int *func,
			  unsigned int *decor);

	unsigned int getProtocols (Window id);

	bool readWindowProp32 (Window         id,
			       Atom           property,
			       unsigned short *returnValue);

	void setCurrentOutput (unsigned int outputNum);

	
	void configure (XConfigureEvent *ce);

	void eraseWindowFromMap (Window id);

	void updateWorkarea ();

	void updateClientList ();

	CompGroup * addGroup (Window id);

	void removeGroup (CompGroup *group);

	CompGroup * findGroup (Window id);

	void applyStartupProperties (CompWindow *window);

	Window getTopWindow ();

	void setNumberOfDesktops (unsigned int nDesktop);

	void setCurrentDesktop (unsigned int desktop);

	bool updateDefaultIcon ();

	void setCurrentActiveWindowHistory (int x, int y);

	void addToCurrentActiveWindowHistory (Window id);

	void enableEdge (int edge);

	void disableEdge (int edge);

	void addScreenActions ();

    public:

	PrivateScreen *priv;

	std::list<CompFileWatch *>  fileWatch;
	CompFileWatchHandle         lastFileWatchHandle;

	std::list<CompTimer *> timers;
	struct timeval               lastTimeout;

	std::list<CompWatchFd *> watchFds;
	CompWatchFdHandle        lastWatchFdHandle;
	struct pollfd            *watchPollFds;
	int                      nWatchFds;

	std::map<CompString, CompPrivate> valueMap;

	xcb_connection_t *connection;

	Display    *dpy;

	int syncEvent, syncError;

	bool randrExtension;
	int  randrEvent, randrError;

	bool shapeExtension;
	int  shapeEvent, shapeError;

	bool xkbExtension;
	int  xkbEvent, xkbError;

	bool xineramaExtension;
	int  xineramaEvent, xineramaError;

	std::vector<XineramaScreenInfo> screenInfo;

	SnDisplay *snDisplay;

	unsigned int      lastPing;
	CompTimer   pingTimer;

	Window activeWindow;

	Window below;
	char   displayString[256];

	XModifierKeymap *modMap;
	unsigned int    modMask[CompModNum];
	unsigned int    ignoredModMask;

	KeyCode escapeKeyCode;
	KeyCode returnKeyCode;

	CompTimer autoRaiseTimer;
	Window	      autoRaiseWindow;

	CompTimer edgeDelayTimer;
	CompDelayedEdgeSettings edgeDelaySettings;

	CompOption::Value plugin;
	bool	          dirtyPluginList;
	
	CompScreen  *screen;

	CompWindowList windows;
	CompWindow::Map windowsMap;

	Colormap	      colormap;
	int		      screenNum;

	CompSize              size;
	CompPoint             vp;
	CompSize              vpSize;
	unsigned int      nDesktop;
	unsigned int      currentDesktop;
	CompRegion   region;

	Window	      root;

	XWindowAttributes attrib;
	Window	      grabWindow;

	Cursor	      invisibleCursor;

	int		      desktopWindowCount;
	unsigned int      mapNum;
	unsigned int      activeNum;

	CompOutput::vector outputDevs;
	int	           currentOutputDev;
	CompOutput         fullscreenOutput;
	bool               hasOverlappingOutputs;

	XRectangle lastViewport;

	CompActiveWindowHistory history[ACTIVE_WINDOW_HISTORY_NUM];
	int			    currentHistory;

	CompScreenEdge screenEdge[SCREEN_EDGE_NUM];

	SnMonitorContext                 *snContext;
	std::list<CompStartupSequence *> startupSequences;
	CompTimer                  startupSequenceTimer;

	std::list<CompGroup *> groups;

	CompIcon *defaultIcon;

	Window wmSnSelectionWindow;
	Atom   wmSnAtom;
	Time   wmSnTimestamp;

	Cursor normalCursor;
	Cursor busyCursor;

	CompWindow **clientList;
	int	       nClientList;

	std::list<ButtonGrab> buttonGrabs;	
	std::list<KeyGrab> keyGrabs;

	std::list<Grab *> grabs;

	unsigned int    pendingDestroys;
	

	XRectangle workArea;

	unsigned int showingDesktopMask;

	unsigned long *desktopHintData;
	int           desktopHintSize;

        bool initialized;

	CompOption::Vector opt;

};

#endif
