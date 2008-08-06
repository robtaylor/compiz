#ifndef _COMPSCREEN_H
#define _COMPSCREEN_H

#include <compwindow.h>

class CompScreen;
class PrivateScreen;

class ScreenInterface : public WrapableInterface<CompScreen> {
    public:
	ScreenInterface ();

	WRAPABLE_DEF(void, preparePaint, int);
	WRAPABLE_DEF(void, donePaint);
	WRAPABLE_DEF(void, paint, CompOutput::ptrList &outputs, unsigned int);

	WRAPABLE_DEF(bool, paintOutput, const ScreenPaintAttrib *,
		     const CompTransform *, Region, CompOutput *,
		     unsigned int);
	WRAPABLE_DEF(void, paintTransformedOutput, const ScreenPaintAttrib *,
		     const CompTransform *, Region, CompOutput *,
		     unsigned int);
	WRAPABLE_DEF(void, applyTransform, const ScreenPaintAttrib *,
		     CompOutput *, CompTransform *);

	WRAPABLE_DEF(void, enableOutputClipping, const CompTransform *,
		     Region, CompOutput *);
	WRAPABLE_DEF(void, disableOutputClipping);

	WRAPABLE_DEF(void, enterShowDesktopMode);
	WRAPABLE_DEF(void, leaveShowDesktopMode, CompWindow *);

	WRAPABLE_DEF(void, outputChangeNotify);

	WRAPABLE_DEF(void, initWindowWalker, CompWalker *walker);

	WRAPABLE_DEF(void, paintCursor, CompCursor *, const CompTransform *,
		     Region, unsigned int);
	WRAPABLE_DEF(bool, damageCursorRect, CompCursor *, bool, BoxPtr);
};


class CompScreen : public WrapableHandler<ScreenInterface>, public CompObject {

    public:
	CompScreen  *next;
	char *windowPrivateIndices;
	int  windowPrivateLen;

    public:
	class Grab {
	    public:
		typedef std::list<Grab>::iterator* handle;

		friend class CompScreen;
		friend class PrivateScreen;
	    private:
		Cursor     cursor;
	    	const char *name;
	};

    public:
	CompScreen ();
	~CompScreen ();

	bool
	init (CompDisplay *, int);

	bool
	init (CompDisplay *, int, Window, Atom, Time);

	CompDisplay *
	display ();
	
	Window
	root ();

	int
	screenNum ();

	CompWindow *
	windows ();

	CompWindow *
	reverseWindows ();

	CompOption *
	getOption (const char *name);

	unsigned int
	showingDesktopMask ();
	
	bool
	handlePaintTimeout ();
	
	bool
	setOption (const char	 *name,
		   CompOptionValue *value);

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

	CompCursor *
	cursors ();

	void
	updateWorkareaForScreen ();

	void
	setDefaultViewport ();

	void
	damageScreen ();

	FuncPtr
	getProcAddress (const char *name);

	void
	showOutputWindow ();

	void
	hideOutputWindow ();

	void
	updateOutputWindow ();

	void
	damageRegion (Region);

	void
	damagePending ();

	void
	forEachWindow (ForEachWindowProc, void *);

	void
	focusDefaultWindow ();

	CompWindow *
	findWindow (Window id);

	CompWindow *
	findTopLevelWindow (Window id);

	void
	insertWindow (CompWindow *w, Window aboveId);

	void
	unhookWindow (CompWindow *w);

	Grab::handle
	pushGrab (Cursor cursor, const char *name);

	void
	updateGrab (Grab::handle handle, Cursor cursor);

	void
	removeGrab (Grab::handle handle, CompPoint *restorePointer);

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
	runCommand (const char *command);

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
	setTexEnvMode (GLenum mode);

	void
	setLighting (bool lighting);

	void
	enableEdge (int edge);

	void
	disableEdge (int edge);

	Window
	getTopWindow ();

	void
	makeCurrent ();

	void
	finishDrawing ();

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
	clearOutput (CompOutput *output, unsigned int mask);

	void
	viewportForGeometry (CompWindow::Geometry gm,
			     int                  *viewportX,
			     int                  *viewportY);

	int
	outputDeviceForGeometry (CompWindow::Geometry gm);

	bool
	updateDefaultIcon ();

	CompCursor *
	findCursor ();

	CompCursorImage *
	findCursorImage (unsigned long serial);

	void
	setCurrentActiveWindowHistory (int x, int y);

	void
	addToCurrentActiveWindowHistory (Window id);

	void
	setWindowPaintOffset (int x, int y);

	int
	getTimeToNextRedraw (struct timeval *tv);

	void
	waitForVideoSync ();

	int
	maxTextureSize ();

	unsigned int
	damageMask ();

	CompPoint vp ();

	CompSize vpSize ();

	CompSize size ();

	unsigned int &
	pendingDestroys ();

	unsigned int &
	mapNum ();

	int &
	desktopWindowCount ();

	int &
	overlayWindowCount ();

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

	Window
	overlay ();

	unsigned int &
	activeNum ();

	void
	handleExposeEvent (XExposeEvent *event);

	void
	detectRefreshRate ();

	void
	updateBackground ();

	bool
	textureNonPowerOfTwo ();

	bool
	textureCompression ();

	bool
	bindPixmapToTexture (CompTexture *texture,
			     Pixmap	 pixmap,
			     int	 width,
			     int	 height,
			     int	 depth);

	bool
	canDoSaturated ();

	bool
	canDoSlightlySaturated ();

	bool
	lighting ();

	int
	filter (int);

	CompFunction *&
	fragmentFunctions ();

	CompProgram *&
	fragmentPrograms ();

	int &
	lastFunctionId ();

	int &
	getSaturateFunction (int, int);

	bool
	fragmentProgram ();
	
	void
	releasePixmapFromTexture (CompTexture *texture);

	void
	enableTexture (CompTexture	 *texture,
		       CompTextureFilter filter);

	void
	disableTexture (CompTexture *texture);

	void
	addCursor ();

	CompCursorImage *&
	cursorImages ();

	WRAPABLE_HND(void, preparePaint, int);
	WRAPABLE_HND(void, donePaint);
	WRAPABLE_HND(void, paint, CompOutput::ptrList &outputs, unsigned int);

	WRAPABLE_HND(bool, paintOutput, const ScreenPaintAttrib *,
		     const CompTransform *, Region, CompOutput *,
		     unsigned int);
	WRAPABLE_HND(void, paintTransformedOutput, const ScreenPaintAttrib *,
		     const CompTransform *, Region, CompOutput *,
		     unsigned int);
	WRAPABLE_HND(void, applyTransform, const ScreenPaintAttrib *,
		     CompOutput *, CompTransform *);

	WRAPABLE_HND(void, enableOutputClipping, const CompTransform *,
		     Region, CompOutput *);
	WRAPABLE_HND(void, disableOutputClipping);

	WRAPABLE_HND(void, enterShowDesktopMode);
	WRAPABLE_HND(void, leaveShowDesktopMode, CompWindow *);

	WRAPABLE_HND(void, outputChangeNotify);

	WRAPABLE_HND(void, initWindowWalker, CompWalker *walker);

	WRAPABLE_HND(void, paintCursor, CompCursor *, const CompTransform *,
		     Region, unsigned int);
	WRAPABLE_HND(bool, damageCursorRect, CompCursor *, bool, BoxPtr);

	GLXBindTexImageProc      bindTexImage;
	GLXReleaseTexImageProc   releaseTexImage;
	GLXQueryDrawableProc     queryDrawable;
	GLXCopySubBufferProc     copySubBuffer;
	GLXGetVideoSyncProc      getVideoSync;
	GLXWaitVideoSyncProc     waitVideoSync;
	GLXGetFBConfigsProc      getFBConfigs;
	GLXGetFBConfigAttribProc getFBConfigAttrib;
	GLXCreatePixmapProc      createPixmap;

	GLActiveTextureProc       activeTexture;
	GLClientActiveTextureProc clientActiveTexture;
	GLMultiTexCoord2fProc     multiTexCoord2f;

	GLGenProgramsProc	     genPrograms;
	GLDeleteProgramsProc     deletePrograms;
	GLBindProgramProc	     bindProgram;
	GLProgramStringProc	     programString;
	GLProgramParameter4fProc programEnvParameter4f;
	GLProgramParameter4fProc programLocalParameter4f;
	GLGetProgramivProc       getProgramiv;

	GLGenFramebuffersProc        genFramebuffers;
	GLDeleteFramebuffersProc     deleteFramebuffers;
	GLBindFramebufferProc        bindFramebuffer;
	GLCheckFramebufferStatusProc checkFramebufferStatus;
	GLFramebufferTexture2DProc   framebufferTexture2D;
	GLGenerateMipmapProc         generateMipmap;

    private:
	PrivateScreen *priv;

    public :
	static bool
	mainMenu (CompDisplay     *d,
		  CompAction      *action,
		  CompActionState state,
		  CompOption      *option,
		  int		  nOption);

	static bool
	runDialog (CompDisplay     *d,
		   CompAction      *action,
		   CompActionState state,
		   CompOption      *option,
		   int		   nOption);

	static bool
	showDesktop (CompDisplay     *d,
		     CompAction      *action,
		     CompActionState state,
		     CompOption      *option,
		     int             nOption);

	static bool
	toggleSlowAnimations (CompDisplay     *d,
			      CompAction      *action,
			      CompActionState state,
			      CompOption      *option,
			      int	      nOption);

	static bool
	windowMenu (CompDisplay     *d,
		    CompAction      *action,
		    CompActionState state,
		    CompOption      *option,
		    int		    nOption);

	static CompOption *
	getScreenOptions ( CompObject *object,
			  int	     *count);

	static bool
	startupSequenceTimeout (void *data);

	static void
	compScreenSnEvent (SnMonitorEvent *event,
			   void           *userData);
};

bool
paintTimeout (void *closure);

int
getTimeToNextRedraw (CompScreen     *s,
		     struct timeval *tv,
		     struct timeval *lastTv,
		     Bool	    idle);

void
waitForVideoSync (CompScreen *s);

Bool
initScreen (CompScreen *s,
	   CompDisplay *display,
	   int	       screenNum,
	   Window      wmSnSelectionWindow,
	   Atom	       wmSnAtom,
	   Time	       wmSnTimestamp);

void
finiScreen (CompScreen *s);

#endif
