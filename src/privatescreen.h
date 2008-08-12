#ifndef _PRIVATESCREEN_H
#define _PRIVATESCREEN_H

#include <compiz-core.h>
#include <compscreen.h>
#include <compsize.h>
#include <comppoint.h>
#include <comptexture.h>

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

	void
	updateScreenBackground (CompTexture *texture);

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
	makeOutputWindow ();

	void
	grabUngrabOneKey (unsigned int modifiers,
			  int          keycode,
			  bool         grab);


	bool
	grabUngrabKeys (unsigned int modifiers,
			int          keycode,
			bool         grab);

	bool
	addPassiveKeyGrab (CompKeyBinding *key);

	void
	removePassiveKeyGrab (CompKeyBinding *key);

	void
	updatePassiveKeyGrabs ();

	bool
	addPassiveButtonGrab (CompButtonBinding *button);

	void
	removePassiveButtonGrab (CompButtonBinding *button);

	void
	computeWorkareaForBox (BoxPtr pBox, XRectangle *area);

	void
	paintBackground (Region	      region,
			 bool	      transformed);

	void
	paintOutputRegion (const CompTransform *transform,
			   Region	       region,
			   CompOutput	       *output,
			   unsigned int	       mask);

    public:

	CompScreen  *screen;
	CompDisplay *display;
	CompWindow	*windows;
	CompWindow	*reverseWindows;


	Colormap	      colormap;
	int		      screenNum;

	CompSize              size;
	CompPoint             vp;
	CompSize              vpSize;
	unsigned int      nDesktop;
	unsigned int      currentDesktop;
	REGION	      region;
	Region	      damage;
	unsigned long     damageMask;
	Window	      root;
	Window	      overlay;
	Window	      output;
	XWindowAttributes attrib;
	Window	      grabWindow;
	CompFBConfig      glxPixmapFBConfigs[MAX_DEPTH + 1];
	int		      textureRectangle;
	int		      textureNonPowerOfTwo;
	int		      textureEnvCombine;
	int		      textureEnvCrossbar;
	int		      textureBorderClamp;
	int		      textureCompression;
	GLint	      maxTextureSize;
	int		      fbo;
	int		      fragmentProgram;
	int		      maxTextureUnits;
	Cursor	      invisibleCursor;
	std::list <CompRect> exposeRects;
	CompTexture       backgroundTexture;
	Bool	      backgroundLoaded;
	unsigned int      pendingDestroys;
	int		      desktopWindowCount;
	unsigned int      mapNum;
	unsigned int      activeNum;

	CompOutput::vector outputDevs;
	int	           currentOutputDev;
	CompOutput         fullscreenOutput;
	bool               hasOverlappingOutputs;


	CompPoint windowPaintOffset;

	XRectangle lastViewport;

	CompActiveWindowHistory history[ACTIVE_WINDOW_HISTORY_NUM];
	int			    currentHistory;

	int overlayWindowCount;

	CompScreenEdge screenEdge[SCREEN_EDGE_NUM];

	SnMonitorContext    *snContext;
	CompStartupSequence *startupSequences;
	CompCore::Timer     startupSequenceTimer;

	CompTexture::Filter filter[3];

	CompGroup *groups;

	CompIcon *defaultIcon;

	Bool canDoSaturated;
	Bool canDoSlightlySaturated;

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

	CompPoint rasterPos;
	struct timeval lastRedraw;
	int		   nextRedraw;
	int		   redrawTime;
	int		   optimalRedrawTime;
	int		   frameStatus;
	int		   timeMult;
	Bool	   idle;
	int		   timeLeft;
	Bool	   pendingCommands;

	int lastFunctionId;

	CompFunction *fragmentFunctions;
	CompProgram  *fragmentPrograms;

	int saturateFunction[2][64];

	GLfloat projection[16];

	Bool clearBuffers;

	Bool lighting;
	Bool slowAnimations;

	XRectangle workArea;

	unsigned int showingDesktopMask;

	unsigned long *desktopHintData;
	int           desktopHintSize;

	CompCursor      *cursors;
	CompCursorImage *cursorImages;

	GLXContext ctx;

	CompOption opt[COMP_SCREEN_OPTION_NUM];


	CompCore::Timer paintTimer;

	GLXGetProcAddressProc    getProcAddress;

};

#endif
