#ifndef _COMPWINDOW_H
#define _COMPWINDOW_H

#include <compsize.h>
#include <comppoint.h>
#include <comptexture.h>
#include <compfragment.h>
#include <compmatrix.h>

class CompWindow;
class PrivateWindow;

#define GET_CORE_WINDOW(object) (dynamic_cast<CompWindow *> (object))
#define CORE_WINDOW(object) CompWindow *w = GET_CORE_WINDOW (object)

#define CompWindowProtocolDeleteMask	  (1 << 0)
#define CompWindowProtocolTakeFocusMask	  (1 << 1)
#define CompWindowProtocolPingMask	  (1 << 2)
#define CompWindowProtocolSyncRequestMask (1 << 3)

#define CompWindowTypeDesktopMask      (1 << 0)
#define CompWindowTypeDockMask         (1 << 1)
#define CompWindowTypeToolbarMask      (1 << 2)
#define CompWindowTypeMenuMask         (1 << 3)
#define CompWindowTypeUtilMask         (1 << 4)
#define CompWindowTypeSplashMask       (1 << 5)
#define CompWindowTypeDialogMask       (1 << 6)
#define CompWindowTypeNormalMask       (1 << 7)
#define CompWindowTypeDropdownMenuMask (1 << 8)
#define CompWindowTypePopupMenuMask    (1 << 9)
#define CompWindowTypeTooltipMask      (1 << 10)
#define CompWindowTypeNotificationMask (1 << 11)
#define CompWindowTypeComboMask	       (1 << 12)
#define CompWindowTypeDndMask	       (1 << 13)
#define CompWindowTypeModalDialogMask  (1 << 14)
#define CompWindowTypeFullscreenMask   (1 << 15)
#define CompWindowTypeUnknownMask      (1 << 16)

#define NO_FOCUS_MASK (CompWindowTypeDesktopMask | \
		       CompWindowTypeDockMask    | \
		       CompWindowTypeSplashMask)

#define CompWindowStateModalMask	    (1 <<  0)
#define CompWindowStateStickyMask	    (1 <<  1)
#define CompWindowStateMaximizedVertMask    (1 <<  2)
#define CompWindowStateMaximizedHorzMask    (1 <<  3)
#define CompWindowStateShadedMask	    (1 <<  4)
#define CompWindowStateSkipTaskbarMask	    (1 <<  5)
#define CompWindowStateSkipPagerMask	    (1 <<  6)
#define CompWindowStateHiddenMask	    (1 <<  7)
#define CompWindowStateFullscreenMask	    (1 <<  8)
#define CompWindowStateAboveMask	    (1 <<  9)
#define CompWindowStateBelowMask	    (1 << 10)
#define CompWindowStateDemandsAttentionMask (1 << 11)
#define CompWindowStateDisplayModalMask	    (1 << 12)

#define MAXIMIZE_STATE (CompWindowStateMaximizedHorzMask | \
			CompWindowStateMaximizedVertMask)

#define CompWindowActionMoveMask	  (1 << 0)
#define CompWindowActionResizeMask	  (1 << 1)
#define CompWindowActionStickMask	  (1 << 2)
#define CompWindowActionMinimizeMask      (1 << 3)
#define CompWindowActionMaximizeHorzMask  (1 << 4)
#define CompWindowActionMaximizeVertMask  (1 << 5)
#define CompWindowActionFullscreenMask	  (1 << 6)
#define CompWindowActionCloseMask	  (1 << 7)
#define CompWindowActionShadeMask	  (1 << 8)
#define CompWindowActionChangeDesktopMask (1 << 9)
#define CompWindowActionAboveMask	  (1 << 10)
#define CompWindowActionBelowMask	  (1 << 11)

#define MwmFuncAll      (1L << 0)
#define MwmFuncResize   (1L << 1)
#define MwmFuncMove     (1L << 2)
#define MwmFuncIconify  (1L << 3)
#define MwmFuncMaximize (1L << 4)
#define MwmFuncClose    (1L << 5)

#define MwmDecorHandle   (1L << 2)
#define MwmDecorTitle    (1L << 3)
#define MwmDecorMenu     (1L << 4)
#define MwmDecorMinimize (1L << 5)
#define MwmDecorMaximize (1L << 6)

#define MwmDecorAll      (1L << 0)
#define MwmDecorBorder   (1L << 1)
#define MwmDecorHandle   (1L << 2)
#define MwmDecorTitle    (1L << 3)
#define MwmDecorMenu     (1L << 4)
#define MwmDecorMinimize (1L << 5)
#define MwmDecorMaximize (1L << 6)

#define WmMoveResizeSizeTopLeft      0
#define WmMoveResizeSizeTop          1
#define WmMoveResizeSizeTopRight     2
#define WmMoveResizeSizeRight        3
#define WmMoveResizeSizeBottomRight  4
#define WmMoveResizeSizeBottom       5
#define WmMoveResizeSizeBottomLeft   6
#define WmMoveResizeSizeLeft         7
#define WmMoveResizeMove             8
#define WmMoveResizeSizeKeyboard     9
#define WmMoveResizeMoveKeyboard    10

/*
  window paint flags

  bit 1-16 are used for read-only flags and they provide
  information that describe the screen rendering pass
  currently in process.

  bit 17-32 are writable flags and they provide information
  that is used to optimize rendering.
*/

/*
  this flag is present when window is being painted
  on a transformed screen.
*/
#define PAINT_WINDOW_ON_TRANSFORMED_SCREEN_MASK (1 << 0)

/*
  this flag is present when window is being tested
  for occlusion of other windows.
*/
#define PAINT_WINDOW_OCCLUSION_DETECTION_MASK   (1 << 1)

/*
  this flag indicates that the window ist painted with
  an offset
*/
#define PAINT_WINDOW_WITH_OFFSET_MASK           (1 << 2)

/*
  flag indicate that window is translucent.
*/
#define PAINT_WINDOW_TRANSLUCENT_MASK           (1 << 16)

/*
  flag indicate that window is transformed.
*/
#define PAINT_WINDOW_TRANSFORMED_MASK           (1 << 17)

/*
  flag indicate that core PaintWindow function should
  not draw this window.
*/
#define PAINT_WINDOW_NO_CORE_INSTANCE_MASK	(1 << 18)

/*
  flag indicate that blending is required.
*/
#define PAINT_WINDOW_BLEND_MASK			(1 << 19)

#define CompWindowGrabKeyMask    (1 << 0)
#define CompWindowGrabButtonMask (1 << 1)
#define CompWindowGrabMoveMask   (1 << 2)
#define CompWindowGrabResizeMask (1 << 3)


class WindowInterface : public WrapableInterface<CompWindow> {
    public:
	WindowInterface ();

	WRAPABLE_DEF(bool, paint, const WindowPaintAttrib *,
		     const CompTransform *, Region, unsigned int);
	WRAPABLE_DEF(bool, draw, const CompTransform *,
		     CompFragment::Attrib &, Region, unsigned int);
	WRAPABLE_DEF(void, addGeometry, CompTexture::Matrix *matrix,
		     int, Region, Region);
	WRAPABLE_DEF(void, drawTexture, CompTexture *texture,
		     CompFragment::Attrib &, unsigned int);
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
		     CompFragment::Attrib &, Region, unsigned int);
	WRAPABLE_HND(void, addGeometry, CompTexture::Matrix *matrix,
		     int, Region, Region);
	WRAPABLE_HND(void, drawTexture, CompTexture *texture,
		     CompFragment::Attrib &, unsigned int);
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
	closeWin (CompDisplay        *d,
		  CompAction         *action,
		  CompAction::State  state,
		  CompOption::Vector &options);

	static bool
	unmaximizeAction (CompDisplay        *d,
			  CompAction         *action,
			  CompAction::State  state,
			  CompOption::Vector &options);

	static bool
	minimizeAction (CompDisplay        *d,
			CompAction         *action,
			CompAction::State  state,
			CompOption::Vector &options);

	static bool
	maximizeAction (CompDisplay        *d,
			CompAction         *action,
			CompAction::State  state,
			CompOption::Vector &options);

	static bool
	maximizeHorizontally (CompDisplay        *d,
			      CompAction         *action,
			      CompAction::State  state,
			      CompOption::Vector &options);

	static bool
	maximizeVertically (CompDisplay        *d,
			    CompAction         *action,
			    CompAction::State  state,
			    CompOption::Vector &options);
	static bool
	raiseInitiate (CompDisplay        *d,
		       CompAction         *action,
		       CompAction::State  state,
		       CompOption::Vector &options);
	static bool
	lowerInitiate (CompDisplay        *d,
		       CompAction         *action,
		       CompAction::State  state,
		       CompOption::Vector &options);

	static bool
	toggleMaximized (CompDisplay        *d,
			 CompAction         *action,
			 CompAction::State  state,
			 CompOption::Vector &options);

	static bool
	toggleMaximizedHorizontally (CompDisplay        *d,
				     CompAction         *action,
				     CompAction::State  state,
				     CompOption::Vector &options);

	static bool
	toggleMaximizedVertically (CompDisplay        *d,
				   CompAction         *action,
				   CompAction::State  state,
				   CompOption::Vector &options);

	static bool
	shade (CompDisplay        *d,
	       CompAction         *action,
	       CompAction::State  state,
	       CompOption::Vector &options);
		
};

#endif
