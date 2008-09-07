#ifndef _PRIVATESCREEN_H
#define _PRIVATESCREEN_H

#include <compiz-core.h>
#include <compscreen.h>
#include <compsize.h>
#include <comppoint.h>

extern CompWindow *lastFoundWindow;
extern CompWindow *lastDamagedWindow;
extern bool	  useDesktopHints;
extern bool       onlyCurrentScreen;

extern int  defaultRefreshRate;
extern const char *defaultTextureFilter;

#define COMP_SCREEN_OPTION_HSIZE		  0
#define COMP_SCREEN_OPTION_VSIZE		  1
#define COMP_SCREEN_OPTION_DEFAULT_ICON		  2
#define COMP_SCREEN_OPTION_NUMBER_OF_DESKTOPS	  3
#define COMP_SCREEN_OPTION_DETECT_OUTPUTS	  4
#define COMP_SCREEN_OPTION_OUTPUTS		  5
#define COMP_SCREEN_OPTION_OVERLAPPING_OUTPUTS	  6
#define COMP_SCREEN_OPTION_FOCUS_PREVENTION_LEVEL 7
#define COMP_SCREEN_OPTION_FOCUS_PREVENTION_MATCH 8
#define COMP_SCREEN_OPTION_NUM		          9

#define OUTPUT_OVERLAP_MODE_SMART          0
#define OUTPUT_OVERLAP_MODE_PREFER_LARGER  1
#define OUTPUT_OVERLAP_MODE_PREFER_SMALLER 2
#define OUTPUT_OVERLAP_MODE_LAST           OUTPUT_OVERLAP_MODE_PREFER_SMALLER

#define FOCUS_PREVENTION_LEVEL_NONE     0
#define FOCUS_PREVENTION_LEVEL_LOW      1
#define FOCUS_PREVENTION_LEVEL_HIGH     2
#define FOCUS_PREVENTION_LEVEL_VERYHIGH 3
#define FOCUS_PREVENTION_LEVEL_LAST     FOCUS_PREVENTION_LEVEL_VERYHIGH

extern const CompMetadata::OptionInfo
coreScreenOptionInfo[COMP_SCREEN_OPTION_NUM];

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



    public:

	CompScreen  *screen;
	CompDisplay *display;
	CompWindowList windows;
	CompWindow::Map windowsMap;

	Colormap	      colormap;
	int		      screenNum;

	CompSize              size;
	CompPoint             vp;
	CompSize              vpSize;
	unsigned int      nDesktop;
	unsigned int      currentDesktop;
	REGION	      region;

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
	CompCore::Timer                  startupSequenceTimer;

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



	CompOption::Vector opt;

};

#endif
