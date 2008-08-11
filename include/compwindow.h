#ifndef _COMPWINDOW_H
#define _COMPWINDOW_H

#include <compsize.h>
#include <comppoint.h>

class CompWindow;
class PrivateWindow;

#define GET_CORE_WINDOW(object) (dynamic_cast<CompWindow *> (object))
#define CORE_WINDOW(object) CompWindow *w = GET_CORE_WINDOW (object)

class WindowInterface : public WrapableInterface<CompWindow> {
    public:
	WindowInterface ();

	WRAPABLE_DEF(bool, paint, const WindowPaintAttrib *,
		     const CompTransform *, Region, unsigned int);
	WRAPABLE_DEF(bool, draw, const CompTransform *,
		     const FragmentAttrib *, Region, unsigned int);
	WRAPABLE_DEF(void, addGeometry, CompTextureMatrix *matrix,
		     int, Region, Region);
	WRAPABLE_DEF(void, drawTexture, CompTexture *texture,
		     const FragmentAttrib *, unsigned int);
	WRAPABLE_DEF(void, drawGeometry);
	WRAPABLE_DEF(bool, damageRect, bool, BoxPtr);

	WRAPABLE_DEF(void, getOutputExtents, CompWindowExtents *);
	WRAPABLE_DEF(void, getAllowedActions, unsigned int *,
		     unsigned int *);

	WRAPABLE_DEF(bool, focus);
	WRAPABLE_DEF(void, activate);
	WRAPABLE_DEF(bool, place, int, int, int*, int*);
	WRAPABLE_DEF(void, validateResizeRequest,
		     unsigned int *, XWindowChanges *);

	WRAPABLE_DEF(void, resizeNotify, int, int, int, int);
	WRAPABLE_DEF(void, moveNotify, int, int, bool);
	WRAPABLE_DEF(void, grabNotify, int, int,
		     unsigned int, unsigned int);
	WRAPABLE_DEF(void, ungrabNotify);
	WRAPABLE_DEF(void, stateChangeNotify, unsigned int);

};

class CompWindow : public WrapableHandler<WindowInterface>, public CompObject {

    public:

	class Geometry : public CompPoint, public CompSize {

	    public:
		Geometry ();
		Geometry (int, int, unsigned int, unsigned int, unsigned int);

		unsigned int border ();

		void set (int, int, unsigned int, unsigned int, unsigned int);

		void setBorder (unsigned int);

	    private:
		unsigned int mBorder;
	};
	
    public:
	CompWindow *next;
	CompWindow *prev;

    public:

	CompWindow (CompScreen *screen,
		    Window     id,
	            Window     aboveId);
	~CompWindow ();

	CompString name ();

	CompScreen *
	screen ();

	Window
	id ();

	Window
	frame ();

	unsigned int &
	wmType ();
	
	unsigned int
	type ();
	
	unsigned int &
	state ();
	
	unsigned int
	actions ();
	
	unsigned int &
	protocols ();

	XWindowAttributes
	attrib ();

	void
	close (Time serverTime);

	bool
	handlePingTimeout (int lastPing);

	void
	handlePing (int lastPing);

	bool
	overlayWindow ();

	Region
	region ();

	bool
	inShowDesktopMode ();

	void
	setShowDesktopMode (bool);

	bool &
	managed ();

	bool
	grabbed ();

	unsigned int &
	activeNum ();
	
	void
	setActiveNum (int);

	int
	mapNum ();

	CompStruts *
	struts ();

	int &
	saveMask ();

	XWindowChanges &
	saveWc ();

	void
	moveToViewportPosition (int x, int y, bool sync);

	char *
	startupId ();

	void
	applyStartupProperties (CompStartupSequence *s);

	unsigned int
	desktop ();

	Window &
	clientLeader ();

	void
	updateNormalHints ();

	void
	updateWmHints ();

	void
	updateClassHints ();

	void
	updateTransientHint ();

	void
	updateIconGeometry ();

	Window
	getClientLeader ();

	char *
	getStartupId ();

	void
	changeState (unsigned int newState);

	void
	recalcActions ();

	void
	recalcType ();

	void
	setWindowFrameExtents (CompWindowExtents *input);

	void
	updateWindowOutputExtents ();

	bool
	bind ();

	void
	release ();

	void
	damageTransformedRect (float  xScale,
			       float  yScale,
			       float  xTranslate,
			       float  yTranslate,
			       BoxPtr rect);

	void
	damageOutputExtents ();

	void
	addDamageRect (BoxPtr rect);

	void
	addDamage ();

	void
	updateRegion ();

	bool
	updateStruts ();

	void
	destroy ();

	void
	sendConfigureNotify ();

	void
	map ();

	void
	unmap ();

	bool
	resize (XWindowAttributes);
	
	bool
	resize (Geometry);

	bool
	resize (int x, int y, unsigned int width, unsigned int height,
		unsigned int border = 0);

	bool
	handleSyncAlarm ();

	void
	sendSyncRequest ();

	void
	configure (XConfigureEvent *ce);

	void
	circulate (XCirculateEvent *ce);

	void
	move (int dx, int dy, Bool damage, Bool immediate);

	void
	syncPosition ();

	void
	moveInputFocusTo ();

	void
	moveInputFocusToOtherWindow ();

	void
	configureXWindow (unsigned int valueMask,
			  XWindowChanges *xwc);


	unsigned int
	adjustConfigureRequestForGravity (XWindowChanges *xwc,
					  unsigned int   xwcm,
					  int            gravity);

	void
	moveResize (XWindowChanges *xwc,
		    unsigned int   xwcm,
		    int            gravity);

	void
	updateSize ();

	void
	raise ();

	void
	lower ();

	void
	restackAbove (CompWindow *sibling);

	void
	restackBelow (CompWindow *sibling);

	void
	updateAttributes (CompStackingUpdateMode stackingMode);

	void
	hide ();

	void
	show ();

	void
	minimize ();

	void
	unminimize ();

	void
	maximize (int state);

	bool
	getUserTime (Time *time);

	void
	setUserTime (Time time);

	bool
	allowWindowFocus (unsigned int noFocusMask,
			  Time         timestamp);

	void
	unredirect ();

	void
	redirect ();

	void
	defaultViewport (int *vx, int *vy);

	CompIcon *
	getIcon (int width, int height);

	void
	freeIcons ();

	int
	outputDevice ();

	bool
	onCurrentDesktop ();

	void
	setDesktop (unsigned int desktop);

	bool
	onAllViewports ();

	void
	getMovementForOffset (int        offX,
			      int        offY,
			      int        *retX,
			      int        *retY);

	Window
	transientFor ();

	int &
	pendingUnmaps ();

	bool &
	minimized ();

	bool &
	placed ();

	bool
	shaded ();

	int
	height ();

	int
	width ();

	Geometry &
	serverGeometry ();

	CompWindowExtents
	input ();

	XSizeHints
	sizeHints ();

	void
	updateOpacity ();

	void
	updateBrightness ();

	void
	updateSaturation ();

	void
	updateMwmHints ();

	void
	updateStartupId ();

	void
	processMap ();

	void
	processDamage (XDamageNotifyEvent *de);

	XSyncAlarm
	syncAlarm ();

	bool
	destroyed ();

	bool
	damaged ();

	bool
	invisible ();

	bool
	redirected ();

	Region
	clip ();

	WindowPaintAttrib &
	paintAttrib ();

	bool
	moreVertices (int newSize);

	bool
	moreIndices (int newSize);
	
	static unsigned int
	constrainWindowState (unsigned int state,
			      unsigned int actions);

	static unsigned int
	windowTypeFromString (const char *str);

	static int
	compareWindowActiveness (CompWindow *w1,
				 CompWindow *w2);

	static int allocPrivateIndex ();
	static void freePrivateIndex (int index);

    	WRAPABLE_HND(bool, paint, const WindowPaintAttrib *,
		     const CompTransform *, Region, unsigned int);
	WRAPABLE_HND(bool, draw, const CompTransform *,
		     const FragmentAttrib *, Region, unsigned int);
	WRAPABLE_HND(void, addGeometry, CompTextureMatrix *matrix,
		     int, Region, Region);
	WRAPABLE_HND(void, drawTexture, CompTexture *texture,
		     const FragmentAttrib *, unsigned int);
	WRAPABLE_HND(void, drawGeometry);

	
	WRAPABLE_HND(bool, damageRect, bool, BoxPtr);

	WRAPABLE_HND(void, getOutputExtents, CompWindowExtents *);
	WRAPABLE_HND(void, getAllowedActions, unsigned int *,
		     unsigned int *);

	WRAPABLE_HND(bool, focus);
	WRAPABLE_HND(void, activate);
	WRAPABLE_HND(bool, place, int, int, int*, int*);
	WRAPABLE_HND(void, validateResizeRequest,
		     unsigned int *, XWindowChanges *);

	WRAPABLE_HND(void, resizeNotify, int, int, int, int);
	WRAPABLE_HND(void, moveNotify, int, int, bool);
	WRAPABLE_HND(void, grabNotify, int, int,
		     unsigned int, unsigned int);
	WRAPABLE_HND(void, ungrabNotify);
	WRAPABLE_HND(void, stateChangeNotify, unsigned int);

	friend class PrivateWindow;
	
    private:
	PrivateWindow *priv;


    public:

	// static action functions
	static bool
	closeWin (CompDisplay     *d,
		  CompAction      *action,
		  CompActionState state,
		  CompOption      *option,
		  int		  nOption);

	static bool
	unmaximize (CompDisplay     *d,
		    CompAction      *action,
		    CompActionState state,
		    CompOption      *option,
		    int		    nOption);

	static bool
	minimize (CompDisplay     *d,
		  CompAction      *action,
		  CompActionState state,
		  CompOption      *option,
		  int		  nOption);

	static bool
	maximize (CompDisplay     *d,
		  CompAction      *action,
		  CompActionState state,
		  CompOption      *option,
		  int		  nOption);

	static bool
	maximizeHorizontally (CompDisplay     *d,
			      CompAction      *action,
			      CompActionState state,
			      CompOption      *option,
			      int	      nOption);

	static bool
	maximizeVertically (CompDisplay     *d,
			    CompAction      *action,
			    CompActionState state,
			    CompOption      *option,
			    int		    nOption);

	static bool
	raiseInitiate (CompDisplay     *d,
		       CompAction      *action,
		       CompActionState state,
		       CompOption      *option,
		       int	       nOption);

	static bool
	lowerInitiate (CompDisplay     *d,
		       CompAction      *action,
		       CompActionState state,
		       CompOption      *option,
		       int	       nOption);

	static bool
	toggleMaximized (CompDisplay     *d,
			 CompAction      *action,
			 CompActionState state,
			 CompOption      *option,
			 int		 nOption);


	static bool
	toggleMaximizedHorizontally (CompDisplay     *d,
				     CompAction      *action,
				     CompActionState state,
				     CompOption      *option,
				     int	     nOption);

	static bool
	toggleMaximizedVertically (CompDisplay     *d,
				   CompAction      *action,
				   CompActionState state,
				   CompOption      *option,
				   int		   nOption);

	static bool
	shade (CompDisplay     *d,
	       CompAction      *action,
	       CompActionState state,
	       CompOption      *option,
	       int	       nOption);

		
};

#endif
